#!/usr/bin/env python3
"""Resolve a release input and bind verified packages to their source/run."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re


def validate_version(value):
    if not re.fullmatch(r"(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)", value):
        raise ValueError("release_version must be X.Y.Z without a v prefix or leading zeros")
    return value


def resolve_version(root, override):
    if override:
        return validate_version(override)
    cmake = re.search(r"^\s*set\(VERSION\s+([^\s)]+)\)", (root / "CMakeLists.txt").read_text(), re.M)
    nix = re.search(r'^\s*version\s*=\s*"([^"]+)";', (root / "default.nix").read_text(), re.M)
    if not cmake or not nix or cmake[1] != nix[1]:
        raise ValueError("default CMake and Nix versions must match")
    return validate_version(cmake[1])


def configure_ebuild(path):
    if path.name == "latte-dock-ng-9999.ebuild":
        raise ValueError("the live ebuild must not be rewritten for a release")
    source = path.read_text()
    if re.search(r"-DVERSION=", source):
        if '-DVERSION="${PV}"' not in source:
            raise ValueError("unexpected version override in release ebuild")
        return
    # The released tag can retain a development default. PV owns the installed
    # Gentoo version, so pass it into CMake instead of inheriting that default.
    source, count = re.subn(r"(local\s+mycmakeargs=\(\s*\n)", r'\1\t\t-DVERSION="${PV}"\n', source)
    if count != 1:
        raise ValueError("expected one mycmakeargs array in release ebuild")
    path.write_text(source)


# Every supported distribution must contribute exactly one native package. A
# bare count would let one distribution stand in for another, so each one is
# matched by its own packaged file name.
DISTRO_PACKAGE_PATTERNS = {
    "arch": r"latte-dock-ng-.*\.pkg\.tar\.zst",
    "fedora": r"latte-dock-ng-.*-fedora\.rpm",
    "opensuse": r"latte-dock-ng-.*-opensuse\.rpm",
    "mageia": r"latte-dock-ng-.*-mageia\.rpm",
    "debian13": r"latte-dock-ng_.*\+deb13u1_amd64\.deb",
    "debian-testing": r"latte-dock-ng_.*-1_amd64\.deb",
    "ubuntu": r"latte-dock-ng_.*ubuntu1_amd64\.deb",
}
# Cauldron temporarily shipped qttools6 built against LLVM sonames it no longer
# provided, which stopped the Mageia leg from producing a package. Cauldron has
# since recovered and both Mageia jobs build and verify the package, so the full
# distribution set is required again. Add a name here only for a documented
# upstream fault, and remove it as soon as that fault clears.
TOLERATED_ABSENT_DISTROS = frozenset()


def package_hashes(directory):
    packages = sorted(path for path in directory.iterdir() if path.is_file())
    if any(not path.name.endswith((".rpm", ".deb", ".pkg.tar.zst")) for path in packages):
        raise ValueError("native release packages must be .rpm, .deb or .pkg.tar.zst")

    names = [path.name for path in packages]
    missing = [distro for distro, pattern in DISTRO_PACKAGE_PATTERNS.items()
               if distro not in TOLERATED_ABSENT_DISTROS
               and not any(re.fullmatch(pattern, name) for name in names)]
    if missing:
        raise ValueError(f"missing native release packages for: {', '.join(sorted(missing))}")
    return {path.name: hashlib.sha256(path.read_bytes()).hexdigest() for path in packages}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("resolve", "metadata", "verify", "ebuild"))
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument("--version", default=os.environ.get("LATTE_RELEASE_VERSION", ""))
    parser.add_argument("--sha")
    parser.add_argument("--run-id")
    parser.add_argument("--packages", type=Path)
    parser.add_argument("--file", type=Path)
    args = parser.parse_args()
    try:
        if args.command == "ebuild":
            configure_ebuild(args.file)
            return
        version = resolve_version(args.root, args.version)
        if args.command == "resolve":
            print(version)
            return
        if not args.sha or not re.fullmatch(r"[0-9a-f]{40}", args.sha) or not args.run_id or not args.run_id.isdecimal():
            raise ValueError("a full source SHA and numeric Build run ID are required")
        record = {"version": version, "source_sha": args.sha, "build_run_id": args.run_id,
                  "packages": package_hashes(args.packages)}
        if args.command == "metadata":
            args.file.write_text(json.dumps(record, indent=2) + "\n")
        elif json.loads(args.file.read_text()) != record:
            raise ValueError("release version/source/run or package checksums differ from the validated Build")
    except (ValueError, OSError) as error:
        parser.exit(1, f"Release validation failed: {error}\n")


if __name__ == "__main__":
    main()
