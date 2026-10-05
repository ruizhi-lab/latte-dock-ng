#!/usr/bin/env python3
"""Update a Gentoo package Manifest for the final tagged source archive."""

import argparse
import hashlib
from pathlib import Path
import re


RELEASE_ARCHIVE = re.compile(r"latte-dock-ng-(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)\.tar\.gz")


def archive_record(archive: Path) -> str:
    if not RELEASE_ARCHIVE.fullmatch(archive.name):
        raise ValueError(f"unexpected release archive name: {archive.name}")
    if not archive.is_file() or archive.stat().st_size == 0:
        raise ValueError(f"release archive is missing or empty: {archive}")

    blake2b = hashlib.blake2b(digest_size=64)
    sha512 = hashlib.sha512()
    with archive.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            blake2b.update(block)
            sha512.update(block)

    return (f"DIST {archive.name} {archive.stat().st_size} "
            f"BLAKE2B {blake2b.hexdigest()} SHA512 {sha512.hexdigest()}")


def checksum_record(kind: str, name: str, path: Path) -> str:
    blake2b = hashlib.blake2b(digest_size=64)
    sha512 = hashlib.sha512()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            blake2b.update(block)
            sha512.update(block)
    return (f"{kind} {name} {path.stat().st_size} "
            f"BLAKE2B {blake2b.hexdigest()} SHA512 {sha512.hexdigest()}")


def update_manifest(package_dir: Path, archive: Path) -> str:
    manifest = package_dir / "Manifest"
    if not manifest.is_file():
        raise ValueError(f"Gentoo package Manifest is missing: {manifest}")

    lines = [archive_record(archive)]
    for ebuild in sorted(package_dir.glob("*.ebuild")):
        lines.append(checksum_record("EBUILD", ebuild.name, ebuild))

    metadata = package_dir / "metadata.xml"
    if metadata.is_file():
        lines.append(checksum_record("MISC", metadata.name, metadata))
    files_dir = package_dir / "files"
    if files_dir.is_dir():
        for path in sorted(item for item in files_dir.rglob("*") if item.is_file()):
            lines.append(checksum_record("AUX", path.relative_to(files_dir).as_posix(), path))

    manifest.write_text("\n".join(sorted(lines, key=lambda row: tuple(row.split()[:2]))) + "\n")
    return lines[0]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("package_dir", type=Path)
    parser.add_argument("release_archive", type=Path)
    args = parser.parse_args()
    try:
        print(update_manifest(args.package_dir, args.release_archive))
    except (OSError, ValueError) as error:
        parser.exit(1, f"Gentoo Manifest update failed: {error}\n")


if __name__ == "__main__":
    main()
