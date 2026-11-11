#!/usr/bin/env python3
"""Build the R7j activity-menu harness (repro.cpp) for one PersistentMenu variant.

Scratch tooling for the R7j investigation (docs/modernization-results/R7j.md).

The harness links the repository's own activitiesdelegate.cpp and the toolsunittest objects of
an existing autotests build, so the compile and link flags are the repository's. The
PersistentMenu sources are taken from a variant directory (default: the repository's
app/settings/generic). The variant's own moc output is linked first: the toolsunittest objects
were built from the repository's header, so the vtable of PersistentMenu they contain lacks any
override that a variant adds, and --allow-multiple-definition keeps the variant's copy.

Usage (after `cmake --preset gcc-debug` and building the toolsunittest target):

    python3 build.py --scratch <scratch dir> [--variant DIR] [--output NAME]

Arguments:
    --scratch DIR   object files and the harness binary go here (required, created)
    --variant DIR   directory with persistentmenu.h and persistentmenu.cpp
                    (default: app/settings/generic of this checkout)
    --autotests DIR autotests directory of a configured build tree
                    (default: build/modernization/gcc-debug/autotests of this checkout)
    --output NAME   binary name inside --scratch (default: r7j-repro)
"""

from __future__ import annotations

import argparse
import os
import re
import shlex
import shutil
import subprocess
import sys
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
REPO = TOOLS.parents[3]
REPLACED_OBJECTS = (
    "CMakeFiles/toolsunittest.dir/toolsunittest.cpp.o",
    "CMakeFiles/toolsunittest.dir/__/app/settings/generic/persistentmenu.cpp.o",
    "CMakeFiles/toolsunittest.dir/__/app/settings/settingsdialog/delegates/activitiesdelegate.cpp.o",
)


def flag_value(flags_text: str, name: str) -> list[str]:
    match = re.search(rf"^{re.escape(name)} = (.*)$", flags_text, re.MULTILINE)
    if not match:
        sys.exit(f"{name} not found in flags.make")
    return shlex.split(match.group(1))


def run(command: list[str], cwd: Path, what: str) -> None:
    print(what, flush=True)
    result = subprocess.run(command, cwd=cwd, capture_output=True, text=True)
    if result.returncode != 0:
        sys.stderr.write(result.stderr[-6000:])
        sys.exit(f"{what} failed")


def moc_binary() -> str:
    found = shutil.which("moc")
    if found:
        return found
    libexec = subprocess.run(["qmake6", "-query", "QT_INSTALL_LIBEXECS"], capture_output=True, text=True).stdout.strip()
    candidate = os.path.join(libexec, "moc")
    if not os.path.exists(candidate):
        sys.exit("moc not found: install the Qt 6 tools or put moc on PATH")
    return candidate


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--scratch", required=True, type=Path)
    parser.add_argument("--variant", type=Path, default=REPO / "app/settings/generic")
    parser.add_argument("--autotests", type=Path, default=REPO / "build/modernization/gcc-debug/autotests")
    parser.add_argument("--output", default="r7j-repro")
    args = parser.parse_args()

    autotests = args.autotests.resolve()
    tools_dir = autotests / "CMakeFiles/toolsunittest.dir"
    flags_text = (tools_dir / "flags.make").read_text(encoding="utf-8")
    defines = flag_value(flags_text, "CXX_DEFINES")
    includes = flag_value(flags_text, "CXX_INCLUDES")
    cxx_flags = [flag for flag in flag_value(flags_text, "CXX_FLAGS") if flag != "-Werror"]

    scratch = args.scratch.resolve()
    obj_dir = scratch / "obj"
    obj_dir.mkdir(parents=True, exist_ok=True)
    variant = args.variant.resolve()
    harness_dir = TOOLS
    output = scratch / args.output

    # Include order: the variant directory first, so that "persistentmenu.h" resolves to it.
    variant_include = ["-I" + str(variant)]
    compile_flags = defines + variant_include + includes + cxx_flags + ["-O0"]

    moc_flags: list[str] = []
    tokens = defines + variant_include + includes
    index = 0
    while index < len(tokens):
        token = tokens[index]
        if token == "-isystem" and index + 1 < len(tokens):
            moc_flags.append("-I" + tokens[index + 1])
            index += 2
            continue
        if token.startswith(("-D", "-I")):
            moc_flags.append(token)
        index += 1
    moc_output = obj_dir / "moc_variant.cpp"
    run([moc_binary(), *moc_flags, "-o", str(moc_output), str(variant / "persistentmenu.h")], autotests, "moc (variant)")

    sources = {
        "repro": harness_dir / "repro.cpp",
        "persistentmenu_variant": variant / "persistentmenu.cpp",
        "moc_variant": moc_output,
        "activitiesdelegate": REPO / "app/settings/settingsdialog/delegates/activitiesdelegate.cpp",
    }
    objects = []
    for name, source in sources.items():
        target = obj_dir / f"{name}.o"
        run(["g++", "-c", "-o", str(target), str(source), *compile_flags], autotests, f"compile {name}")
        objects.append(str(target))

    link_text = (tools_dir / "link.txt").read_text(encoding="utf-8")
    tokens = shlex.split(link_text.strip())
    kept: list[str] = []
    skip_next = False
    for token in tokens:
        if skip_next:
            skip_next = False
            continue
        if token == "-o":
            kept += ["-o", str(output)]
            skip_next = True
            continue
        if token in REPLACED_OBJECTS:
            continue
        kept.append(token)
    first_object = next(index for index, token in enumerate(kept) if token.endswith(".o"))
    kept[first_object:first_object] = objects
    kept[1:1] = ["-Wl,--allow-multiple-definition"]
    run(kept, autotests, f"link {output}")
    print(f"built {output}", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
