#!/usr/bin/env python3
"""Run the scoped clang-tidy checks and prove their warning gate with a fixture."""

from __future__ import annotations

import argparse
import json
import shlex
import shutil
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CONFIG = ROOT / ".clang-tidy"
PRODUCTION_SOURCE = ROOT / "app/data/errordata.cpp"
WARNING_FIXTURE = ROOT / "autotests/clang-tidy/unused-return-value.cpp"


def run(command: list[str], *, expect_success: bool, label: str) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(command, check=False, text=True, capture_output=True)
    if (result.returncode == 0) != expect_success:
        raise RuntimeError(
            f"{label} returned {result.returncode}, expected success={expect_success}:\n"
            + result.stdout
            + result.stderr
        )
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", required=True, type=Path, help="Clang Debug build containing compile_commands.json")
    args = parser.parse_args()

    clang_tidy = shutil.which("clang-tidy")
    if clang_tidy is None:
        parser.error("clang-tidy was not found in PATH")

    build_dir = args.build_dir.resolve()
    database = build_dir / "compile_commands.json"
    if not database.is_file():
        parser.error(f"compilation database not found: {database}")

    entries = json.loads(database.read_text(encoding="utf-8"))
    source = str(PRODUCTION_SOURCE.resolve())
    entry = next((item for item in entries if str(Path(item["file"]).resolve()) == source), None)
    if entry is None:
        parser.error(f"production source is absent from the selected compile database: {source}")
    command = entry.get("command") or shlex.join(entry["arguments"])
    if "clang" not in command:
        parser.error("selected compile database entry is not from a Clang build")

    version = run([clang_tidy, "--version"], expect_success=True, label="clang-tidy version")
    print(version.stdout.strip())

    production = run(
        [clang_tidy, "--config-file", str(CONFIG), "-p", str(build_dir), "--quiet", source],
        expect_success=True,
        label="targeted production analysis",
    )
    print("app/data/errordata.cpp: no selected clang-tidy diagnostics")

    with tempfile.TemporaryDirectory(prefix="latte-clang-tidy-") as temp_dir:
        fixture = Path(temp_dir) / WARNING_FIXTURE.name
        fixture.write_bytes(WARNING_FIXTURE.read_bytes())
        rejected = run(
            [clang_tidy, "--config-file", str(CONFIG), "--quiet", str(fixture), "--", "-std=c++20"],
            expect_success=False,
            label="intentional unused-return-value fixture",
        )
        output = rejected.stdout + rejected.stderr
        if "bugprone-unused-return-value" not in output:
            raise RuntimeError("fixture failed without the expected bugprone-unused-return-value diagnostic:\n" + output)
        print("intentional unused return value rejected by the configured warning gate")

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as error:
        raise SystemExit(str(error))
