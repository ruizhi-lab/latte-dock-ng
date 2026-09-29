#!/usr/bin/env python3
"""Prove that the configured ASan and UBSan runtimes report their diagnostics."""

import argparse
import os
import pathlib
import subprocess
import tempfile


def run(command, *, env=None, check=True):
    result = subprocess.run(
        command,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        env=env,
    )
    if check and result.returncode != 0:
        raise RuntimeError(
            f"Command exited {result.returncode}: {' '.join(map(str, command))}\n"
            f"{result.stdout}"
        )
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", default=os.environ.get("CXX", "g++"))
    args = parser.parse_args()

    source = pathlib.Path(__file__).resolve().parents[1] / "autotests/sanitizers/fault-fixture.cpp"
    with tempfile.TemporaryDirectory(prefix="latte-sanitizer-fixture-") as temp_dir:
        executable = pathlib.Path(temp_dir) / "fault-fixture"
        run(
            [
                args.compiler,
                "-std=c++20",
                "-g",
                "-O1",
                "-fno-omit-frame-pointer",
                "-fsanitize=address,undefined",
                str(source),
                "-o",
                str(executable),
            ]
        )

        for mode, diagnostic in (
            ("address", "AddressSanitizer: heap-buffer-overflow"),
            ("undefined", "runtime error: signed integer overflow"),
        ):
            environment = os.environ.copy()
            environment["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
            environment["UBSAN_OPTIONS"] = (
                "halt_on_error=1:print_stacktrace=1"
                if mode == "undefined"
                else "halt_on_error=0:print_stacktrace=1"
            )
            result = run([str(executable), mode], env=environment, check=False)
            if result.returncode == 0 or diagnostic not in result.stdout:
                raise RuntimeError(
                    f"{mode} fault was not rejected with the expected diagnostic "
                    f"(exit={result.returncode}):\n{result.stdout}"
                )
            print(f"{mode} fault rejected with {diagnostic}")


if __name__ == "__main__":
    main()
