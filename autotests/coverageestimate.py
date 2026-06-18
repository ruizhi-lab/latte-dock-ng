#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

from __future__ import annotations

import pathlib
import re
import subprocess


def git_files(pattern: str) -> list[str]:
    return subprocess.check_output(["git", "ls-files", pattern], text=True).splitlines()


def main() -> None:
    all_cpp = git_files("*.cpp")
    prod_cpp = [path for path in all_cpp if not path.startswith("autotests/")]
    cmake = pathlib.Path("autotests/CMakeLists.txt").read_text(encoding="utf-8")

    covered = set()
    for match in re.finditer(r"\.\./([^\s)]+\.cpp)", cmake):
        path = match.group(1)
        if pathlib.Path(path).exists():
            covered.add(path)

    # qmlsmoketest loads the built lattecoreplugin through the QML import path.
    if pathlib.Path("declarativeimports/core/lattecoreplugin.cpp").exists():
        covered.add("declarativeimports/core/lattecoreplugin.cpp")

    print(f"{len(covered)}/{len(prod_cpp)} = {len(covered) / len(prod_cpp) * 100:.1f}%")


if __name__ == "__main__":
    main()
