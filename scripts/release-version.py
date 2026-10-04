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


def package_hashes(directory):
    packages = sorted(path for path in directory.iterdir() if path.is_file())
    if len(packages) != 7 or any(not path.name.endswith((".rpm", ".deb", ".pkg.tar.zst")) for path in packages):
        raise ValueError("expected exactly seven native release packages")
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
