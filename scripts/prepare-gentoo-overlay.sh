#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 2 ]]; then
    echo "Usage: $0 <release-version> <overlay-checkout>" >&2
    exit 2
fi

version="$1"
overlay_dir=$(realpath "$2")
package_dir="$overlay_dir/kde-misc/latte-dock-ng"
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
distdir="${DISTDIR:-$(mktemp -d "${TMPDIR:-/tmp}/latte-distfiles.XXXXXX")}"
distfile="latte-dock-ng-${version}.tar.gz"

[[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]
test -d "$package_dir"
test -f "$package_dir/metadata.xml"
test -f "$package_dir/Manifest"

mapfile -t release_ebuilds < <(find "$package_dir" -maxdepth 1 -type f -name 'latte-dock-ng-*.ebuild' ! -name 'latte-dock-ng-9999.ebuild' -print | sort -V)
test "${#release_ebuilds[@]}" -gt 0

new_ebuild="$package_dir/latte-dock-ng-${version}.ebuild"
if [[ ! -f "$new_ebuild" ]]; then
    cp "${release_ebuilds[-1]}" "$new_ebuild"
fi
grep -Fq 'SRC_URI="https://github.com/ruizhi-lab/latte-dock-ng/archive/refs/tags/v${PV}.tar.gz -> ${P}.tar.gz"' "$new_ebuild"
python3 "$script_dir/release-version.py" ebuild --file "$new_ebuild"

for old_ebuild in "${release_ebuilds[@]}"; do
    [[ "$old_ebuild" == "$new_ebuild" ]] || rm -f "$old_ebuild"
done

install -d -m 0755 "$distdir"
archive="$distdir/$distfile"
curl --fail --location --retry 3 --retry-all-errors --retry-delay 5 \
    --output "$archive" \
    "https://github.com/ruizhi-lab/latte-dock-ng/archive/refs/tags/v${version}.tar.gz"
tar -tzf "$archive" >/dev/null

# Native distro jobs provide the tested application build/install matrix. The
# release path only needs the exact final tag archive and its Gentoo Manifest;
# the maintainer's Gentoo system remains the ebuild runtime-validation host.
python3 "$script_dir/gentoo_manifest.py" "$package_dir" "$archive"
python3 -c 'import sys, xml.etree.ElementTree as ET; ET.parse(sys.argv[1])' "$package_dir/metadata.xml"
grep -Fq 'SRC_URI="https://github.com/ruizhi-lab/latte-dock-ng/archive/refs/tags/v${PV}.tar.gz -> ${P}.tar.gz"' "$new_ebuild"
grep -Fq -- '-DVERSION="${PV}"' "$new_ebuild"

grep -Fq "$distfile" "$package_dir/Manifest"
git -C "$overlay_dir" diff --check
