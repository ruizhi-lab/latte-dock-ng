#!/usr/bin/env python3
"""Run the gated clazy checks over the production sources and prove each one with a fixture.

The gate names every check explicitly and each name has a fixture in
autotests/clazy/<check>.cpp that the runner must see rejected, so a check can
neither join the gate unproven nor stay in it after clazy stops reporting it.
clazy accepts an unknown check name silently (it prints "No checks were
requested!" and exits 0), and a level such as level1 changes meaning when clazy
is upgraded, so the runner also asks clazy which checks it enabled and rejects
any difference from GATED_CHECKS. A check joins the list in its own slice, after
a clean --inventory run and with its fixture.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
FIXTURE_DIR = ROOT / "autotests/clazy"
# The fixtures are compiled with this production source's flags. It sees the Qt modules
# (Core, Gui, Widgets, Qml, DBus) that the fixtures include and the project's Qt
# defines: QT_NO_KEYWORDS, QT_NO_FOREACH, QT_NO_CONTEXTLESS_CONNECT and QT_STRICT_ITERATORS.
FIXTURE_FLAGS_SOURCE = ROOT / "app/lattecorona.cpp"

# Checks with zero findings on the production tree whose fixture fires under the
# production flags, in alphabetical order. See docs/modernization-results/R2c.md
# for the inventory and for the zero-finding checks that stay out because the project
# flags make them unreachable (foreach, qstring-comparison-to-implicit-char) or
# no trigger could be found for them (qstring-left).
GATED_CHECKS: tuple[str, ...] = (
    "auto-unexpected-qstringbuilder",
    "child-event-qobject-cast",
    "connect-by-name",
    "connect-non-signal",
    "connect-not-normalized",
    "const-signal-or-slot",
    "empty-qstringliteral",
    "incorrect-emit",
    "install-event-filter",
    "lambda-in-connect",
    "lambda-unique-connection",
    "lowercase-qml-type-name",
    "mutable-container-key",
    "no-module-include",
    "post-event",
    "qcolor-from-literal",
    "qdatetime-utc",
    "qdeleteall",
    "qenums",
    "qfileinfo-exists",
    "qgetenv",
    "qhash-namespace",
    "qlatin1string-non-ascii",
    "qmap-with-pointer-key",
    "qstring-arg",
    "qstring-insensitive-allocation",
    "qt-macros",
    "range-loop-reference",
    "readlock-detaching",
    "returning-data-from-temporary",
    "rule-of-two-soft",
    "skipped-base-method",
    "strict-iterators",
    "temporary-iterator",
    "use-static-qregularexpression",
    "virtual-signal",
    "writing-to-temporary",
    "wrong-qevent-cast",
    "wrong-qglobalstatic",
)

# One translation unit must finish well inside this; a hung clazy fails the gate.
TRANSLATION_UNIT_TIMEOUT = 900

# clazy reports a finding as "<file>:<line>:<column>: <severity>: <text> [-Wclazy-<check>]".
FINDING = re.compile(
    r"^(?P<path>/.+?):(?P<line>\d+):(?P<column>\d+): (?P<severity>warning|error): "
    r"(?P<message>.*) \[-Wclazy-(?P<check>[a-z0-9-]+)\]$"
)
COMPILER_ERROR = re.compile(r"^.*: (?:fatal )?error: ")

# CLAZY_CHECKS, CLAZY_HEADER_FILTER, CLAZY_EXTRA_OPTIONS and friends change what clazy
# reports; the gate must not depend on the caller's shell.
ENVIRONMENT = {name: value for name, value in os.environ.items() if not name.startswith("CLAZY_")}


@dataclass(frozen=True)
class Finding:
    check: str
    path: str
    line: int
    column: int
    message: str


@dataclass
class UnitResult:
    source: str
    findings: list[Finding] = field(default_factory=list)
    problems: list[str] = field(default_factory=list)


def run(command: list[str], *, label: str) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(command, check=False, text=True, capture_output=True, env=ENVIRONMENT)
    if result.returncode != 0:
        raise RuntimeError(f"{label} returned {result.returncode}:\n{result.stdout}{result.stderr}")
    return result


def enabled_checks(clazy: str, checks: str) -> list[str]:
    output = run([clazy, f"--checks={checks}", "--list-checks"], label="clazy check listing")
    text = output.stdout + output.stderr
    if "Invalid check" in text or "No checks were requested" in text:
        raise RuntimeError(f"clazy rejected the check selection {checks!r}:\n{text}")
    names = [line.strip() for line in text.splitlines() if line.startswith("    ")]
    if not names:
        raise RuntimeError(f"clazy enabled no checks for {checks!r}:\n{text}")
    return names


def unit_path(entry: dict[str, str]) -> Path:
    return (Path(entry["directory"]) / entry["file"]).resolve()


def retarget(entry: dict[str, str], source: Path) -> dict[str, str]:
    """Compile `source` with the flags of an existing database entry."""
    if entry["file"] not in entry["command"]:
        raise RuntimeError(f"cannot derive a compile command for {source} from {entry['file']}")
    retargeted = dict(entry)
    retargeted["file"] = str(source)
    retargeted["command"] = entry["command"].replace(entry["file"], str(source))
    retargeted["output"] = f"{source.name}.o"
    return retargeted


def select_translation_units(
    entries: list[dict[str, str]], build_dir: Path
) -> tuple[list[dict[str, str]], Counter[str]]:
    """Pick one compile command per production source and count what is skipped.

    Production sources are the sources outside autotests/. Files inside the build
    directory are generated (AUTOMOC, rcc, QML type registration, plugin entry
    points, D-Bus adaptors, Wayland protocol glue and the configure_file() copies
    of production sources): their originals are analyzed through their own
    translation unit. A production source compiled by several targets, including
    the test targets that rebuild it, is analyzed once, preferably with the flags
    of the production target.
    """
    skipped: Counter[str] = Counter()
    candidates: dict[Path, list[dict[str, str]]] = {}
    for entry in entries:
        path = unit_path(entry)
        if path.is_relative_to(build_dir):
            skipped["generated source in the build directory"] += 1
        elif path.is_relative_to(ROOT / "autotests"):
            skipped["autotest source"] += 1
        elif path.is_relative_to(ROOT):
            candidates.setdefault(path, []).append(entry)
        else:
            raise RuntimeError(f"compile database entry is outside the source tree and the build directory: {path}")

    selected = []
    for path in sorted(candidates):
        commands = candidates[path]
        production = [item for item in commands if Path(item.get("output", "")).parts[:1] != ("autotests",)]
        selected.append((production or commands)[0])
        skipped["duplicate compile command of a selected source"] += len(commands) - 1
    if not selected:
        raise RuntimeError("the compile database holds no production source; check --build-dir and the checkout path")
    return selected, skipped


def analyze_unit(clazy: str, database: Path, source: Path, checks: str, header_filter: str) -> UnitResult:
    result = UnitResult(source=str(source))
    # The strict-warning flags in the database turn clazy findings into errors, and
    # the clazy frontend is a different LLVM than the compiler that built the tree.
    # -Wno-error keeps findings as warnings and -w drops compiler warnings, which
    # the strict-warning gate owns; any remaining "error:" is a failed analysis.
    command = [
        clazy,
        "-p", str(database),
        f"--checks={checks}",
        f"--header-filter={header_filter}",
        "--extra-arg=-Wno-error",
        "--extra-arg=-w",
        str(source),
    ]
    try:
        completed = subprocess.run(
            command, check=False, text=True, capture_output=True, timeout=TRANSLATION_UNIT_TIMEOUT, env=ENVIRONMENT
        )
    except subprocess.TimeoutExpired:
        result.problems.append(f"timed out after {TRANSLATION_UNIT_TIMEOUT} seconds")
        return result

    output = completed.stdout + completed.stderr
    for line in output.splitlines():
        match = FINDING.match(line)
        if match:
            # clazy prints a header as the includer spelled it ("app/a/../b/x.h"), so the same
            # location arrives under several names; normalize before counting or comparing.
            path = os.path.realpath(match["path"])
            result.findings.append(Finding(match["check"], path, int(match["line"]), int(match["column"]), match["message"]))
        elif "[-Wclazy-" in line:
            # A finding the pattern cannot read must not pass as a clean run.
            result.problems.append(f"unparsed clazy diagnostic: {line}")
        elif COMPILER_ERROR.match(line):
            result.problems.append(line)
    if "Invalid check" in output or "No checks were requested" in output:
        result.problems.append("clazy ignored the check selection")
    if completed.returncode != 0 and not result.problems:
        result.problems.append(f"clazy exited with {completed.returncode}:\n{output.strip()}")
    return result


def analyze(
    clazy: str, entries: list[dict[str, str]], checks: str, header_filter: str, jobs: int
) -> list[UnitResult]:
    """Run clazy over the given compile commands with one process per source."""
    with tempfile.TemporaryDirectory(prefix="latte-clazy-db-") as database:
        Path(database, "compile_commands.json").write_text(json.dumps(entries), encoding="utf-8")
        with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
            futures = [
                pool.submit(analyze_unit, clazy, Path(database), unit_path(entry), checks, header_filter)
                for entry in entries
            ]
            return [future.result() for future in futures]


def first_party(path: str, build_dir: Path) -> bool:
    location = Path(path)
    return (
        location.is_relative_to(ROOT)
        and not location.is_relative_to(build_dir)
        and not location.is_relative_to(ROOT / "autotests")
    )


def relative(path: str) -> str:
    location = Path(path)
    return str(location.relative_to(ROOT)) if location.is_relative_to(ROOT) else path


def unique(findings: list[Finding]) -> list[Finding]:
    # A header finding repeats in every translation unit that includes the header.
    first: dict[tuple[str, int, int, str], Finding] = {}
    for item in sorted(findings, key=lambda item: (item.path, item.line, item.column, item.check, item.message)):
        first.setdefault((item.path, item.line, item.column, item.check), item)
    return list(first.values())


def report_problems(results: list[UnitResult]) -> None:
    failed = [item for item in results if item.problems]
    if not failed:
        return
    lines = [f"{len(failed)} translation unit(s) could not be analyzed:"]
    for item in failed:
        lines.append(f"  {relative(item.source)}")
        lines.extend(f"    {problem}" for problem in item.problems[:5])
    raise RuntimeError("\n".join(lines))


def split_findings(results: list[UnitResult], build_dir: Path) -> tuple[list[Finding], list[Finding]]:
    raw = [item for result in results for item in result.findings]
    return (
        unique([item for item in raw if first_party(item.path, build_dir)]),
        unique([item for item in raw if not first_party(item.path, build_dir)]),
    )


def prove_fixtures(clazy: str, flags_entry: dict[str, str], jobs: int) -> None:
    """Every gated check must reject its own fixture, and every fixture must belong to a gated check."""
    fixtures = sorted(FIXTURE_DIR.glob("*.cpp"))
    if [path.stem for path in fixtures] != sorted(GATED_CHECKS):
        raise RuntimeError(
            f"{FIXTURE_DIR.relative_to(ROOT)} must hold exactly one <check>.cpp per gated check;\n"
            f"missing: {sorted(set(GATED_CHECKS) - {path.stem for path in fixtures})}\n"
            f"unexpected: {sorted({path.stem for path in fixtures} - set(GATED_CHECKS))}"
        )

    with tempfile.TemporaryDirectory(prefix="latte-clazy-fixtures-") as temp_dir:
        entries = []
        for fixture in fixtures:
            copy = Path(temp_dir, fixture.name)
            copy.write_bytes(fixture.read_bytes())
            entries.append(retarget(flags_entry, copy))
        # Each fixture runs with the whole gated list, as production does.
        results = analyze(clazy, entries, ",".join(GATED_CHECKS), f"^{re.escape(temp_dir)}/", jobs)
    report_problems(results)

    unrejected = [
        fixture.name for fixture, result in zip(fixtures, results) if fixture.stem not in {item.check for item in result.findings}
    ]
    if unrejected:
        raise RuntimeError("the gate did not reject its negative fixture(s): " + ", ".join(unrejected))
    print(f"{len(fixtures)} negative fixtures: each is rejected by its own gated check")


def run_gate(clazy: str, entries: list[dict[str, str]], build_dir: Path, jobs: int) -> None:
    selected, skipped = select_translation_units(entries, build_dir)
    print(
        f"selected {len(selected)} production sources; skipped "
        + ", ".join(f"{count} {reason}" for reason, count in sorted(skipped.items()))
    )

    checks = ",".join(GATED_CHECKS)
    enabled = enabled_checks(clazy, checks)
    if sorted(enabled) != sorted(GATED_CHECKS):
        raise RuntimeError(f"clazy enabled {sorted(enabled)}, but GATED_CHECKS names {sorted(GATED_CHECKS)}")

    # Prove the checks can fire before trusting a clean production result.
    flags_entry = next((entry for entry in selected if unit_path(entry) == FIXTURE_FLAGS_SOURCE), None)
    if flags_entry is None:
        raise RuntimeError(f"fixture flags source is absent from the compile database: {FIXTURE_FLAGS_SOURCE}")
    prove_fixtures(clazy, flags_entry, jobs)

    results = analyze(clazy, selected, checks, f"^{re.escape(str(ROOT))}/", jobs)
    report_problems(results)
    kept, ignored = split_findings(results, build_dir)
    if kept:
        lines = [f"{len(kept)} clazy finding(s) in gated checks:"]
        lines.extend(f"  {relative(item.path)}:{item.line}:{item.column}: [{item.check}] {item.message}" for item in kept)
        raise RuntimeError("\n".join(lines))
    print(
        f"{len(selected)} production sources: no findings from {len(GATED_CHECKS)} gated clazy checks"
        + (f" ({len(ignored)} in generated or test code were not counted)" if ignored else "")
    )


def run_inventory(
    clazy: str, entries: list[dict[str, str]], build_dir: Path, jobs: int, checks: str, report: Path | None
) -> None:
    selected, skipped = select_translation_units(entries, build_dir)
    enabled = enabled_checks(clazy, checks)
    print(f"{len(enabled)} checks enabled by {checks}; analyzing {len(selected)} production sources")
    results = analyze(clazy, selected, checks, f"^{re.escape(str(ROOT))}/", jobs)
    report_problems(results)

    kept, ignored = split_findings(results, build_dir)
    per_check = Counter(item.check for item in kept)
    for check in enabled:
        files = {item.path for item in kept if item.check == check}
        print(f"{check:40} {per_check[check]:5} findings in {len(files):3} files")
    print(f"{len(kept)} findings in first-party code; {len(ignored)} in generated or test code were not counted")

    if report:
        document = {
            "clazy": " ".join(run([clazy, "--version"], label="clazy version").stdout.split()),
            "checks": enabled,
            "sources": [relative(str(unit_path(entry))) for entry in selected],
            "skipped": dict(sorted(skipped.items())),
            "findings": [
                {"check": item.check, "file": relative(item.path), "line": item.line, "column": item.column, "message": item.message}
                for item in kept
            ],
            "ignored_findings": [
                {"check": item.check, "file": relative(item.path), "line": item.line, "column": item.column}
                for item in ignored
            ],
        }
        report.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
        print(f"wrote {report}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--build-dir", required=True, type=Path, help="Clang Debug build containing compile_commands.json")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 1, help="parallel clazy processes (default: CPU count)")
    parser.add_argument(
        "--inventory",
        metavar="CHECKS",
        help="count findings of these checks (for example level0,level1) instead of gating; never fails on findings",
    )
    parser.add_argument("--report", type=Path, help="write the --inventory findings as JSON")
    args = parser.parse_args()
    if args.report and not args.inventory:
        parser.error("--report needs --inventory")
    if args.jobs < 1:
        parser.error("--jobs must be at least 1")

    clazy = shutil.which("clazy-standalone")
    if clazy is None:
        parser.error("clazy-standalone was not found in PATH")

    build_dir = args.build_dir.resolve()
    database = build_dir / "compile_commands.json"
    if not database.is_file():
        parser.error(f"compilation database not found: {database}")
    entries = json.loads(database.read_text(encoding="utf-8"))
    for entry in entries:
        entry.setdefault("command", shlex.join(entry.get("arguments", [])))
    if not any(Path(shlex.split(entry["command"])[0]).name.startswith("clang") for entry in entries):
        parser.error("selected compile database is not from a Clang build")

    print(" ".join(run([clazy, "--version"], label="clazy version").stdout.split()))

    if args.inventory:
        run_inventory(clazy, entries, build_dir, args.jobs, args.inventory, args.report)
    else:
        run_gate(clazy, entries, build_dir, args.jobs)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as error:
        print(error, file=sys.stderr)
        raise SystemExit(1)
