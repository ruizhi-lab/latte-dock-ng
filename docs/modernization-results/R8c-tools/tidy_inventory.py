#!/usr/bin/env python3
"""Count clang-tidy findings per check over the production sources (R8c inventory).

Report only. The repository's .clang-tidy, which the CI gate
(scripts/test-clang-tidy.py) reads, is bypassed with --config, so a run never
changes what the gate checks and never fails on findings. The source set is the
clazy gate's (scripts/test-clazy.py, imported, not copied): one compile command per
production source, autotests/ and generated sources skipped, and a finding in a
header counted once however many translation units include it.

    python3 docs/modernization-results/R8c-tools/tidy_inventory.py \\
        --build-dir build/modernization/clang-debug --jobs 8 \\
        --report /data/tmp/r8c/inventory.json.gz --summary /data/tmp/r8c/summary.tsv

Exit status: 0 when every selected source was analyzed, 1 when one or more could not
be (the report is still written and names them), 2 for usage errors.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import gzip
import importlib.util
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import time
from collections import defaultdict
from dataclasses import dataclass, field
from pathlib import Path
from types import ModuleType

TOOL_ROOT = Path(__file__).resolve().parents[3]

# The inventoried families. The others target other code bases (abseil, altera,
# android, boost, darwin, fuchsia, google, linuxkernel, llvm, llvmlibc, mpi, objc,
# openmp, zircon) or are mostly aliases of these (hicpp).
DEFAULT_CHECKS = ",".join(
    [
        "-*",
        "bugprone-*",
        "cert-*",
        "clang-analyzer-*",
        "concurrency-*",
        "cppcoreguidelines-*",
        "misc-*",
        "modernize-*",
        "performance-*",
        "portability-*",
        "readability-*",
    ]
)

# A translation unit must finish well inside this; the static analyzer is the slow part.
TRANSLATION_UNIT_TIMEOUT = 1800

# "<file>:<line>:<column>: warning: <text> [<check>[,<alias>...]]". An alias that
# reports the same diagnostic is listed in the same brackets; aliases can also report
# different locations when their default options differ, so each name is counted.
FINDING = re.compile(
    r"^(?P<path>/.+?):(?P<line>\d+):(?P<column>\d+): (?P<severity>warning|error): "
    r"(?P<message>.*) \[(?P<checks>[A-Za-z0-9_.-]+(?:,[A-Za-z0-9_.-]+)*)\]$"
)
# A compiler error or a clang-tidy failure means the unit was not analyzed.
FAILURE = re.compile(r"(?:^|: )(?:fatal )?error: |^Error while processing ")


@dataclass(frozen=True)
class Finding:
    """Same fields as test-clazy.py's Finding, so its unique() and first_party() apply."""

    check: str
    path: str
    line: int
    column: int
    message: str


@dataclass
class UnitResult:
    source: str
    seconds: float = 0.0
    findings: list[Finding] = field(default_factory=list)
    problems: list[str] = field(default_factory=list)


def load_source_selection(source_root: Path) -> ModuleType:
    """Import the clazy gate's runner of `source_root`, whose ROOT is that checkout."""
    path = source_root / "scripts/test-clazy.py"
    # The module must be in sys.modules while it runs: its dataclasses resolve their
    # postponed annotations through it. One name per checkout keeps two apart.
    name = "latte_test_clazy_" + re.sub(r"\W", "_", str(path))
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot import the clazy gate's source selection from {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def tidy_config(checks: str, source_root: Path) -> str:
    """A complete configuration, so no .clang-tidy file is read; JSON is valid YAML."""
    return json.dumps(
        {
            "Checks": checks,
            "WarningsAsErrors": "",
            "HeaderFilterRegex": "^" + re.escape(str(source_root)) + "/",
        }
    )


def parse_output(text: str) -> tuple[list[Finding], list[str]]:
    findings: list[Finding] = []
    problems: list[str] = []
    for line in text.splitlines():
        match = FINDING.match(line)
        if match:
            names = match["checks"].split(",")
            if any(name.startswith("clang-diagnostic-") for name in names):
                # Compiler diagnostics are not inventoried; only an error stops the analysis.
                if match["severity"] == "error":
                    problems.append(line)
                continue
            # A header is reached under several spellings ("app/a/../b/x.h").
            path = os.path.realpath(match["path"])
            for name in names:
                findings.append(Finding(name, path, int(match["line"]), int(match["column"]), match["message"]))
        elif FAILURE.search(line):
            problems.append(line)
    return findings, problems


def enabled_checks(clang_tidy: str, config: str) -> list[str]:
    with tempfile.TemporaryDirectory(prefix="latte-tidy-list-") as cwd:
        result = subprocess.run(
            [clang_tidy, f"--config={config}", "--list-checks"], check=False, text=True, capture_output=True, cwd=cwd
        )
    if result.returncode != 0:
        raise RuntimeError(f"clang-tidy --list-checks returned {result.returncode}:\n{result.stdout}{result.stderr}")
    names = [line.strip() for line in result.stdout.splitlines() if line.startswith("    ")]
    if not names:
        raise RuntimeError("clang-tidy enabled no checks:\n" + result.stdout + result.stderr)
    return names


def analyze_unit(
    clang_tidy: str, database: Path, source: Path, config: str, timeout: int, profile: Path | None = None
) -> UnitResult:
    result = UnitResult(source=str(source))
    # -Wno-error keeps the strict-warning flags of the database from turning compiler
    # warnings into errors; the checks' own diagnostics are not affected.
    command = [clang_tidy, "-p", str(database), f"--config={config}", "--extra-arg=-Wno-error", "--quiet"]
    if profile is not None:
        command += ["--enable-check-profile", f"--store-check-profile={profile}"]
    command.append(str(source))
    start = time.monotonic()
    try:
        completed = subprocess.run(command, check=False, text=True, capture_output=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        result.problems.append(f"timed out after {timeout} seconds")
        result.seconds = time.monotonic() - start
        return result
    result.seconds = time.monotonic() - start
    result.findings, result.problems = parse_output(completed.stdout + completed.stderr)
    if completed.returncode != 0 and not result.problems:
        result.problems.append(f"clang-tidy exited with {completed.returncode}")
    return result


def analyze(
    clang_tidy: str, entries: list[dict], unit_path, config: str, jobs: int, timeout: int, profile: Path | None = None
) -> list[UnitResult]:
    """One clang-tidy process per selected compile command, from a database that holds only those."""
    with tempfile.TemporaryDirectory(prefix="latte-tidy-db-") as database:
        Path(database, "compile_commands.json").write_text(json.dumps(entries), encoding="utf-8")
        with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
            futures = [
                pool.submit(analyze_unit, clang_tidy, Path(database), unit_path(entry), config, timeout, profile)
                for entry in entries
            ]
            return [future.result() for future in futures]


def profile_seconds(directory: Path) -> tuple[dict[str, float], dict[str, float]]:
    """Wall seconds per check and per source from clang-tidy's --store-check-profile files.

    The static analyzer runs outside the check framework and is not in the profile, so
    a run of clang-analyzer-* checks shows it only in the total time of each unit.
    """
    per_check: dict[str, float] = defaultdict(float)
    per_source: dict[str, float] = defaultdict(float)
    for path in sorted(directory.glob("*.json")):
        document = json.loads(path.read_text(encoding="utf-8"))
        for key, value in document.get("profile", {}).items():
            if key.startswith("time.clang-tidy.") and key.endswith(".wall"):
                per_check[key[len("time.clang-tidy.") : -len(".wall")]] += value
                per_source[os.path.realpath(document["file"])] += value
    return dict(per_check), dict(per_source)


def per_check_counts(checks: list[str], findings: list[Finding]) -> dict[str, dict[str, int]]:
    files: dict[str, set[str]] = defaultdict(set)
    counts: dict[str, int] = defaultdict(int)
    for item in findings:
        counts[item.check] += 1
        files[item.check].add(item.path)
    names = sorted(set(checks) | set(counts))
    return {name: {"findings": counts[name], "files": len(files[name])} for name in names}


def write_json(path: Path, document: dict) -> None:
    text = json.dumps(document, indent=1, sort_keys=False) + "\n"
    if path.suffix == ".gz":
        # mtime=0 keeps the archive bytes stable for the same content.
        with gzip.GzipFile(path, "wb", mtime=0) as handle:
            handle.write(text.encode("utf-8"))
    else:
        path.write_text(text, encoding="utf-8")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--build-dir", required=True, type=Path, help="Clang build with compile_commands.json")
    parser.add_argument("--source-root", type=Path, default=TOOL_ROOT, help="checkout the build was configured from")
    parser.add_argument("--checks", default=DEFAULT_CHECKS, help="clang-tidy check selection; write --checks=-*,... because it starts with a dash (default: the R8c families)")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 1, help="parallel clang-tidy processes")
    parser.add_argument("--timeout", type=int, default=TRANSLATION_UNIT_TIMEOUT, help="seconds per translation unit")
    parser.add_argument("--clang-tidy", default="clang-tidy", help="clang-tidy executable")
    parser.add_argument("--report", type=Path, help="write the findings as JSON (.json or .json.gz)")
    parser.add_argument("--summary", type=Path, help="write per-check counts as TSV")
    parser.add_argument("--check-profile", type=Path, help="store clang-tidy's per-check timing in this new directory")
    args = parser.parse_args(argv)
    if args.jobs < 1 or args.timeout < 1:
        parser.error("--jobs and --timeout must be at least 1")

    clang_tidy = shutil.which(args.clang_tidy)
    if clang_tidy is None:
        parser.error(f"{args.clang_tidy} was not found in PATH")
    source_root = args.source_root.resolve()
    build_dir = args.build_dir.resolve()
    database = build_dir / "compile_commands.json"
    if not database.is_file():
        parser.error(f"compilation database not found: {database}")
    profile = args.check_profile.resolve() if args.check_profile else None
    if profile is not None:
        if profile.exists():
            parser.error(f"--check-profile directory already exists: {profile}")
        profile.mkdir(parents=True)

    selection = load_source_selection(source_root)
    if Path(selection.ROOT).resolve() != source_root:
        parser.error(f"{source_root}/scripts/test-clazy.py resolves its ROOT to {selection.ROOT}")
    entries = json.loads(database.read_text(encoding="utf-8"))
    for entry in entries:
        entry.setdefault("command", shlex.join(entry.get("arguments", [])))
    selected, skipped = selection.select_translation_units(entries, build_dir)

    version = subprocess.run([clang_tidy, "--version"], check=False, text=True, capture_output=True).stdout
    version = " ".join(version.split())
    config = tidy_config(args.checks, source_root)
    checks = enabled_checks(clang_tidy, config)
    print(version)
    print(
        f"{len(checks)} checks enabled by {args.checks}; analyzing {len(selected)} production sources; skipped "
        + ", ".join(f"{count} {reason}" for reason, count in sorted(skipped.items()))
    )

    start = time.monotonic()
    results = analyze(clang_tidy, selected, selection.unit_path, config, args.jobs, args.timeout, profile)
    elapsed = time.monotonic() - start

    raw = [finding for result in results for finding in result.findings]
    kept = selection.unique([item for item in raw if selection.first_party(item.path, build_dir)])
    ignored = selection.unique([item for item in raw if not selection.first_party(item.path, build_dir)])
    counts = per_check_counts(checks, kept)
    failed = [result for result in results if result.problems]

    ranked = sorted(counts.items(), key=lambda pair: (-pair[1]["findings"], pair[0]))
    for name, value in ranked:
        if value["findings"]:
            print(f"{name:60} {value['findings']:6} findings in {value['files']:4} files")
    zero = [name for name, value in counts.items() if not value["findings"]]
    print(
        f"{len(kept)} findings in first-party code from {len(counts) - len(zero)} checks; {len(zero)} checks with none; "
        f"{len(ignored)} in generated or test code not counted; {elapsed:.0f} s with {args.jobs} jobs"
    )

    check_seconds: dict[str, float] = {}
    source_check_seconds: dict[str, float] = {}
    if profile is not None:
        check_seconds, source_check_seconds = profile_seconds(profile)
        print(f"check profile: {sum(check_seconds.values()):.0f} s in {len(check_seconds)} checks; the most expensive:")
        for name, seconds in sorted(check_seconds.items(), key=lambda pair: -pair[1])[:15]:
            print(f"  {name:60} {seconds:8.1f} s")

    if args.summary:
        lines = ["check\tfindings\tfiles"] + [f"{name}\t{value['findings']}\t{value['files']}" for name, value in sorted(counts.items())]
        args.summary.write_text("\n".join(lines) + "\n", encoding="utf-8")
    if args.report:
        write_json(
            args.report,
            {
                "clang_tidy": version,
                "check_selection": args.checks,
                "checks": checks,
                "sources": [selection.relative(str(selection.unit_path(entry))) for entry in selected],
                "skipped": dict(sorted(skipped.items())),
                "jobs": args.jobs,
                "elapsed_s": round(elapsed, 1),
                "unit_seconds": {selection.relative(result.source): round(result.seconds, 1) for result in results},
                "failed_sources": {selection.relative(result.source): result.problems[:5] for result in failed},
                "per_check": counts,
                "check_wall_s": {name: round(value, 3) for name, value in sorted(check_seconds.items())},
                "unit_check_wall_s": {
                    selection.relative(name): round(value, 2) for name, value in sorted(source_check_seconds.items())
                },
                "ignored_findings": len(ignored),
                "findings": [
                    {
                        "check": item.check,
                        "file": selection.relative(item.path),
                        "line": item.line,
                        "column": item.column,
                        "message": item.message,
                    }
                    for item in kept
                ],
            },
        )

    if failed:
        print(f"{len(failed)} source(s) could not be analyzed:", file=sys.stderr)
        for result in failed:
            print(f"  {selection.relative(result.source)}", file=sys.stderr)
            for problem in result.problems[:3]:
                print(f"    {problem}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except RuntimeError as error:
        print(error, file=sys.stderr)
        sys.exit(1)
