#!/usr/bin/env python3
"""Resolve the includes named by compat/ shims against the host's system search path.

Read-only evidence for the R8e compat/ inventory: each include named in a shim
(plain, __has_include or include_next) is preprocessed with g++ -M, using only
the -isystem directories that a latte-dock-ng translation unit sees and the
implicit system directories. The project's own -I entries are left out, so
compat/ cannot answer for itself. A resolved path is the upstream header this
host would use; "absent" means no system directory provides that spelling.

Usage (from the worktree root, after cmake --preset gcc-debug):
    python3 -I docs/modernization-results/R8e-tools/compat_probe.py . \\
        build/modernization/gcc-debug/compile_commands.json
"""

import json
import os
import re
import shlex
import subprocess
import sys

INCLUDE = re.compile(
    r"#\s*(?:if|elif)?\s*(?:__has_include(?:_next)?\s*\(\s*|include(?:_next)?\s+)<([^>]+)>"
)


def system_flags(database: str) -> list[str]:
    """Return the -isystem pairs of the executable's first compile command."""
    entries = json.load(open(database, encoding="utf-8"))
    main = next(e for e in entries if "latte-dock-ng.dir/main.cpp.o" in e["output"])
    tokens = shlex.split(main["command"])
    flags: list[str] = []
    for index, token in enumerate(tokens[:-1]):
        if token == "-isystem":
            flags += ["-isystem", tokens[index + 1]]
    return flags


def resolve(target: str, flags: list[str]) -> str:
    result = subprocess.run(
        ["g++", "-M", "-x", "c++", "-std=c++20", *flags, "-"],
        input=f"#include <{target}>\n",
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        return "absent"
    leaf = target.split("/")[-1]
    for token in result.stdout.split():
        if token.startswith("/") and token.endswith(leaf):
            return token
    return "resolved"


def main() -> int:
    root, database = sys.argv[1], sys.argv[2]
    flags = system_flags(database)
    compat = os.path.join(root, "compat")
    for dirpath, _, names in sorted(os.walk(compat)):
        if dirpath.endswith("qml") or "/qml/" in dirpath + "/":
            continue
        for name in sorted(names):
            path = os.path.join(dirpath, name)
            with open(path, encoding="utf-8") as handle:
                text = handle.read()
            targets: list[str] = []
            for match in INCLUDE.finditer(text):
                if match.group(1) not in targets:
                    targets.append(match.group(1))
            print(f"== {os.path.relpath(path, root)}")
            for target in targets:
                print(f"   <{target}> -> {resolve(target, flags)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
