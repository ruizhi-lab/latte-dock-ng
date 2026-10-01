#!/usr/bin/env python3
"""Verify that the strict-warning helper rejects warnings and accepts clean code."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import tempfile
from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
FIXTURE_SOURCE = REPOSITORY_ROOT / "autotests/cmake/strict-warning-fixture"
STRICT_WARNINGS_MODULE = REPOSITORY_ROOT / "cmake/LatteStrictWarnings.cmake"


def run(command: list[str], *, expect_success: bool, label: str) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(command, check=False, text=True, capture_output=True)
    if (result.returncode == 0) != expect_success:
        output = result.stdout + result.stderr
        raise RuntimeError(f"{label} returned {result.returncode}, expected success={expect_success}:\n{output}")
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", required=True, help="C++ compiler executable")
    parser.add_argument("--build-type", required=True, choices=("Debug", "Release"))
    args = parser.parse_args()

    compiler = shutil.which(args.compiler)
    if compiler is None:
        parser.error(f"C++ compiler not found: {args.compiler}")

    with tempfile.TemporaryDirectory(prefix="latte-strict-warning-") as temp_dir:
        root = Path(temp_dir)
        common = [
            "cmake",
            "-S",
            str(FIXTURE_SOURCE),
            "-DCMAKE_BUILD_TYPE=" + args.build_type,
            "-DCMAKE_CXX_COMPILER=" + compiler,
            "-DLATTE_STRICT_WARNINGS_MODULE=" + str(STRICT_WARNINGS_MODULE),
            "-DLATTE_STRICT_WARNINGS=ON",
        ]
        failing_build = root / "warning-build"
        run(common + ["-B", str(failing_build)], expect_success=True, label="warning fixture configure")
        rejected = run(
            ["cmake", "--build", str(failing_build), "--parallel", "2"],
            expect_success=False,
            label="intentional warning build",
        )
        output = rejected.stdout + rejected.stderr
        if "warning" not in output.lower() or "error" not in output.lower():
            raise RuntimeError("warning fixture failed without a compiler warning promoted to an error:\n" + output)

        clean_build = root / "clean-build"
        run(
            common + ["-B", str(clean_build), "-DFIXTURE_ENABLE_WARNING=OFF"],
            expect_success=True,
            label="clean fixture configure",
        )
        run(
            ["cmake", "--build", str(clean_build), "--parallel", "2"],
            expect_success=True,
            label="clean fixture build",
        )

    print(f"{compiler} {args.build_type}: intentional warning rejected; corrected fixture passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as error:
        raise SystemExit(str(error))
