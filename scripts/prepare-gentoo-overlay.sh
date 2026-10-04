#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 2 ]]; then
    echo "Usage: $0 <release-version> <overlay-checkout>" >&2
    exit 2
fi

version="$1"
overlay_dir=$(realpath "$2")
package_dir="$overlay_dir/kde-misc/latte-dock-ng"
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
python3 "$(dirname "$0")/release-version.py" ebuild --file "$new_ebuild"

for old_ebuild in "${release_ebuilds[@]}"; do
    [[ "$old_ebuild" == "$new_ebuild" ]] || rm -f "$old_ebuild"
done

install -d -m 0755 "$distdir"
export DISTDIR="$distdir"
export XDG_CACHE_HOME="${XDG_CACHE_HOME:-${TMPDIR:-/tmp}/latte-pkgcheck-cache}"
install -d -m 0755 /etc/portage/repos.conf
cat > /etc/portage/repos.conf/ruizhi-overlay.conf <<EOF
[ruizhi-overlay]
location = $overlay_dir
masters = gentoo
auto-sync = no
EOF

git config --global --add safe.directory "$overlay_dir"
pkgdev manifest -d "$DISTDIR" "$package_dir"
pkgcheck scan --repo "$overlay_dir" kde-misc/latte-dock-ng
xmllint --noout "$package_dir/metadata.xml"
ebuild "$new_ebuild" clean configure compile install

grep -Fq "$distfile" "$package_dir/Manifest"
git -C "$overlay_dir" diff --check
