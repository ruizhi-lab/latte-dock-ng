#!/usr/bin/env python3
"""Run the gated clang-tidy checks over the production sources and prove each one with a fixture.

The gate analyzes the production sources that scripts/test-clazy.py selects (one compile
command per source; autotests/ and generated sources skipped). It runs one clang-tidy process
per source, from a compile database that holds only the selected commands. Each check is named
in GATED_CHECKS and has a negative fixture, autotests/clang-tidy/<check>.cpp, which the runner
must see rejected under the production flags. clang-tidy accepts a check name it does not know,
so the runner also compares `clang-tidy --list-checks` with GATED_CHECKS.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import importlib.util
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass, field
from pathlib import Path
from types import ModuleType


ROOT = Path(__file__).resolve().parents[1]
CONFIG = ROOT / ".clang-tidy"
FIXTURE_DIR = ROOT / "autotests/clang-tidy"
# The fixtures are compiled with this production source's flags, as the clazy gate's are.
FIXTURE_FLAGS_SOURCE = ROOT / "app/lattecorona.cpp"

# The checks that CONFIG enables, in alphabetical order, with no family wildcard: clang-tidy 22
# enables checks that 18 lacks, so a wildcard would change with the CI image. A check joins this
# list only after a clean run over the production sources and with its fixture.
GATED_CHECKS: tuple[str, ...] = (
    "bugprone-assignment-in-if-condition",
    "bugprone-bad-signal-to-kill-thread",
    "bugprone-bool-pointer-implicit-conversion",
    "bugprone-casting-through-void",
    "bugprone-compare-pointer-to-member-virtual-function",
    "bugprone-copy-constructor-init",
    "bugprone-forwarding-reference-overload",
    "bugprone-inaccurate-erase",
    "bugprone-incorrect-enable-if",
    "bugprone-lambda-function-name",
    "bugprone-misplaced-pointer-arithmetic-in-alloc",
    "bugprone-move-forwarding-reference",
    "bugprone-optional-value-conversion",
    "bugprone-parent-virtual-call",
    "bugprone-posix-return",
    "bugprone-redundant-branch-condition",
    "bugprone-shared-ptr-array-mismatch",
    "bugprone-sizeof-container",
    "bugprone-spuriously-wake-up-functions",
    "bugprone-string-constructor",
    "bugprone-string-integer-assignment",
    "bugprone-string-literal-with-embedded-nul",
    "bugprone-suspicious-enum-usage",
    "bugprone-suspicious-memory-comparison",
    "bugprone-suspicious-missing-comma",
    "bugprone-suspicious-realloc-usage",
    "bugprone-terminating-continue",
    "bugprone-throw-keyword-missing",
    "bugprone-too-small-loop-variable",
    "bugprone-undefined-memory-manipulation",
    "bugprone-undelegated-constructor",
    "bugprone-unhandled-self-assignment",
    "bugprone-unique-ptr-array-mismatch",
    "bugprone-unused-return-value",
    "bugprone-use-after-move",
    "concurrency-thread-canceltype-asynchronous",
    "misc-misleading-bidirectional",
    "misc-redundant-expression",
    "misc-uniqueptr-reset-release",
    "misc-unused-alias-decls",
    "performance-avoid-endl",
    "performance-faster-string-find",
    "performance-for-range-copy",
    "performance-implicit-conversion-in-loop",
    "performance-inefficient-algorithm",
    "performance-inefficient-string-concatenation",
    "performance-inefficient-vector-operation",
    "performance-move-constructor-init",
    "performance-trivially-destructible",
)

# One translation unit must finish well inside this; a hung clang-tidy fails the gate.
TRANSLATION_UNIT_TIMEOUT = 900

# "<file>:<line>:<column>: <severity>: <text> [<checks>]". WarningsAsErrors promotes a finding to
# an error and adds "-warnings-as-errors" to the bracket list; that marker is not a check name.
FINDING = re.compile(
    r"^(?P<path>/.+?):(?P<line>\d+):(?P<column>\d+): (?P<severity>warning|error): "
    r"(?P<message>.*) \[(?P<checks>[A-Za-z0-9_.-]+(?:,[A-Za-z0-9_.-]+)*)\]$"
)
# An error line that is not a finding, such as a failed translation unit or a missing input.
FAILURE = re.compile(r"^(?:\S.*: )?(?:fatal )?error: |^Error while processing ")
# A diagnostic with a check bracket that FINDING cannot read, for example one with a relative
# path. It may be a gated finding, so it must not pass as a clean run.
UNREADABLE_FINDING = re.compile(r": (?:warning|error): .*\[[^\]]+\]$")
WARNINGS_AS_ERRORS_MARKER = "-warnings-as-errors"


@dataclass(frozen=True)
class Finding:
    """The fields that test-clazy.py's unique() and first_party() read, plus the severity."""

    check: str
    path: str
    line: int
    column: int
    message: str
    severity: str


@dataclass
class UnitResult:
    source: str
    findings: list[Finding] = field(default_factory=list)
    problems: list[str] = field(default_factory=list)


def load_clazy_gate() -> ModuleType:
    """Import scripts/test-clazy.py, whose source selection and de-duplication this gate shares."""
    path = ROOT / "scripts/test-clazy.py"
    spec = importlib.util.spec_from_file_location("latte_test_clazy", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot import the clazy gate's source selection from {path}")
    module = importlib.util.module_from_spec(spec)
    # Registered before it runs: its dataclasses resolve their annotations through sys.modules.
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


CLAZY_GATE = load_clazy_gate()


def run(command: list[str], *, label: str) -> str:
    result = subprocess.run(command, check=False, text=True, capture_output=True)
    if result.returncode != 0:
        raise RuntimeError(f"{label} returned {result.returncode}:\n{result.stdout}{result.stderr}")
    return result.stdout + result.stderr


def enabled_checks(clang_tidy: str) -> list[str]:
    """The checks CONFIG enables. --config-file is explicit, so no .clang-tidy is discovered."""
    output = run([clang_tidy, f"--config-file={CONFIG}", "--list-checks"], label="clang-tidy check listing")
    names = [line.strip() for line in output.splitlines() if line.startswith("    ")]
    if not names:
        raise RuntimeError(f"clang-tidy enabled no checks for {CONFIG}:\n{output}")
    return names


def analyze_unit(clang_tidy: str, database: Path, source: Path) -> UnitResult:
    result = UnitResult(source=str(source))
    command = [
        clang_tidy,
        "-p",
        str(database),
        f"--config-file={CONFIG}",
        # A production command can carry -Werror. -Wno-error keeps a compiler warning from
        # becoming a compile error; compiler warnings belong to the strict-warning gate, and only
        # a compile error (clang-diagnostic-error) stops this gate.
        "--extra-arg=-Wno-error",
        "--quiet",
        str(source),
    ]
    try:
        completed = subprocess.run(
            command, check=False, text=True, capture_output=True, timeout=TRANSLATION_UNIT_TIMEOUT
        )
    except subprocess.TimeoutExpired:
        result.problems.append(f"timed out after {TRANSLATION_UNIT_TIMEOUT} seconds")
        return result

    output = completed.stdout + completed.stderr
    for line in output.splitlines():
        match = FINDING.match(line)
        if match:
            # A header is reached under several spellings ("app/a/../b/x.h"); normalize before counting.
            path = os.path.realpath(match["path"])
            names = [name for name in match["checks"].split(",") if name != WARNINGS_AS_ERRORS_MARKER]
            if any(name.startswith("clang-diagnostic-") for name in names):
                if "clang-diagnostic-error" in names:
                    result.problems.append(line)
                continue
            for name in names:
                result.findings.append(
                    Finding(
                        name,
                        path,
                        int(match["line"]),
                        int(match["column"]),
                        match["message"],
                        match["severity"],
                    )
                )
        elif FAILURE.match(line) or UNREADABLE_FINDING.search(line):
            result.problems.append(line)
    # A failing exit without any finding is a failed analysis, not a clean run.
    if completed.returncode != 0 and not result.findings and not result.problems:
        result.problems.append(f"clang-tidy exited with {completed.returncode}:\n{output.strip()}")
    return result


def analyze(clang_tidy: str, entries: list[dict[str, str]], jobs: int) -> list[UnitResult]:
    """One clang-tidy process per compile command, from a database that holds only those commands."""
    with tempfile.TemporaryDirectory(prefix="latte-clang-tidy-db-") as database:
        Path(database, "compile_commands.json").write_text(json.dumps(entries), encoding="utf-8")
        with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
            futures = [
                pool.submit(analyze_unit, clang_tidy, Path(database), CLAZY_GATE.unit_path(entry))
                for entry in entries
            ]
            return [future.result() for future in futures]


def report_problems(results: list[UnitResult]) -> None:
    failed = [item for item in results if item.problems]
    if not failed:
        return
    lines = [f"{len(failed)} translation unit(s) could not be analyzed:"]
    for item in failed:
        lines.append(f"  {CLAZY_GATE.relative(item.source)}")
        lines.extend(f"    {problem}" for problem in item.problems[:5])
    raise RuntimeError("\n".join(lines))


def prove_fixtures(clang_tidy: str, flags_entry: dict[str, str], jobs: int) -> None:
    """Every gated check must reject its own fixture as an error, and every fixture must belong to a gated check."""
    fixtures = sorted(FIXTURE_DIR.glob("*.cpp"))
    if [path.stem for path in fixtures] != sorted(GATED_CHECKS):
        raise RuntimeError(
            f"{FIXTURE_DIR.relative_to(ROOT)} must hold exactly one <check>.cpp per gated check;\n"
            f"missing: {sorted(set(GATED_CHECKS) - {path.stem for path in fixtures})}\n"
            f"unexpected: {sorted({path.stem for path in fixtures} - set(GATED_CHECKS))}"
        )

    with tempfile.TemporaryDirectory(prefix="latte-clang-tidy-fixtures-") as temp_dir:
        entries = []
        for fixture in fixtures:
            copy = Path(temp_dir, fixture.name)
            copy.write_bytes(fixture.read_bytes())
            entries.append(CLAZY_GATE.retarget(flags_entry, copy))
        results = analyze(clang_tidy, entries, jobs)
    report_problems(results)

    unrejected = [
        fixture.name
        for fixture, result in zip(fixtures, results)
        if not any(item.check == fixture.stem and item.severity == "error" for item in result.findings)
    ]
    if unrejected:
        raise RuntimeError("the gate did not reject its negative fixture(s): " + ", ".join(unrejected))
    print(f"{len(fixtures)} negative fixtures: each is rejected as an error by its own gated check")


def run_gate(clang_tidy: str, entries: list[dict[str, str]], build_dir: Path, jobs: int) -> None:
    selected, skipped = CLAZY_GATE.select_translation_units(entries, build_dir)
    print(
        f"selected {len(selected)} production sources; skipped "
        + ", ".join(f"{count} {reason}" for reason, count in sorted(skipped.items()))
    )

    enabled = enabled_checks(clang_tidy)
    if sorted(enabled) != sorted(GATED_CHECKS):
        raise RuntimeError(f"clang-tidy enabled {sorted(enabled)}, but GATED_CHECKS names {sorted(GATED_CHECKS)}")

    # Prove the checks can fire before trusting a clean production result.
    flags_entry = next((entry for entry in selected if CLAZY_GATE.unit_path(entry) == FIXTURE_FLAGS_SOURCE), None)
    if flags_entry is None:
        raise RuntimeError(f"fixture flags source is absent from the compile database: {FIXTURE_FLAGS_SOURCE}")
    if "clang" not in flags_entry["command"]:
        raise RuntimeError("the production compile command for the fixture flags is not from a Clang build")
    prove_fixtures(clang_tidy, flags_entry, jobs)

    results = analyze(clang_tidy, selected, jobs)
    report_problems(results)
    raw = [item for result in results for item in result.findings]
    kept = CLAZY_GATE.unique([item for item in raw if CLAZY_GATE.first_party(item.path, build_dir)])
    ignored = CLAZY_GATE.unique([item for item in raw if not CLAZY_GATE.first_party(item.path, build_dir)])
    if kept:
        lines = [f"{len(kept)} clang-tidy finding(s) in gated checks:"]
        lines.extend(
            f"  {CLAZY_GATE.relative(item.path)}:{item.line}:{item.column}: [{item.check}] {item.message}"
            for item in kept
        )
        raise RuntimeError("\n".join(lines))
    print(
        f"{len(selected)} production sources: no findings from {len(GATED_CHECKS)} gated clang-tidy checks"
        + (f" ({len(ignored)} in generated or test code were not counted)" if ignored else "")
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--build-dir", required=True, type=Path, help="Clang Debug build containing compile_commands.json")
    parser.add_argument(
        "--jobs", type=int, default=os.cpu_count() or 1, help="parallel clang-tidy processes (default: CPU count)"
    )
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be at least 1")

    clang_tidy = shutil.which("clang-tidy")
    if clang_tidy is None:
        parser.error("clang-tidy was not found in PATH")

    build_dir = args.build_dir.resolve()
    database = build_dir / "compile_commands.json"
    if not database.is_file():
        parser.error(f"compilation database not found: {database}")
    entries = json.loads(database.read_text(encoding="utf-8"))
    for entry in entries:
        entry.setdefault("command", shlex.join(entry.get("arguments", [])))

    print(" ".join(run([clang_tidy, "--version"], label="clang-tidy version").split()))
    run_gate(clang_tidy, entries, build_dir, args.jobs)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as error:
        print(error, file=sys.stderr)
        raise SystemExit(1)
