#!/usr/bin/env bash
# Gentoo ebuild verification used by docker-compose and release CI.
set -euo pipefail

version="${VERSION:-9999}"

export CCACHE_DISABLE=1
export PORTAGE_TMPDIR="${PORTAGE_TMPDIR:-/tmp/latte-portage-tmp}"
export DISTDIR="${DISTDIR:-/tmp/latte-distfiles}"

overlay="/tmp/latte-overlay"
package_dir="${overlay}/kde-misc/latte-dock-ng"
dist_name="latte-dock-ng-${version}.tar.gz"

rm -rf "${overlay}" "${PORTAGE_TMPDIR}"
mkdir -p "${package_dir}" "${DISTDIR}" "${PORTAGE_TMPDIR}" "${overlay}/metadata" "${overlay}/profiles"
# pkgdev identifies the synthetic repository through profiles/repo_name;
# Portage's repos.conf entry alone leaves manifest generation outside an ebuild repo.
printf '%s\n' 'latte-overlay' > "${overlay}/profiles/repo_name"

cat > "${overlay}/metadata/layout.conf" <<'EOF'
masters = gentoo
thin-manifests = true
EOF

cat > "${package_dir}/metadata.xml" <<'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE pkgmetadata SYSTEM "https://www.gentoo.org/dtd/metadata.dtd">
<pkgmetadata>
  <maintainer type="person">
    <email>ruizhi.zhong88@gmail.com</email>
    <name>Ruizhi Zhong</name>
  </maintainer>
</pkgmetadata>
EOF

if [[ "${version}" == "9999" ]]; then
    inherit_line="inherit ecm git-r3 xdg"
    source_block='EGIT_REPO_URI="https://github.com/ruizhi-lab/latte-dock-ng.git"
EGIT_BRANCH="main"'
    keywords_line='KEYWORDS=""'
    properties_line='PROPERTIES="live"'
    git_bdepend=$'\tdev-vcs/git'
else
    inherit_line="inherit ecm xdg"
    keywords_line='KEYWORDS="~amd64"'
    properties_line=""
    git_bdepend=""

    if [[ "${GENTOO_VERIFY_FETCH_TAG:-false}" == "true" ]]; then
        source_block='SRC_URI="https://github.com/ruizhi-lab/latte-dock-ng/archive/refs/tags/v${PV}.tar.gz -> ${P}.tar.gz"'
    else
        tar --exclude-vcs --exclude='./build*' --exclude='./.cache' \
            --transform "s#^#latte-dock-ng-${version}/#" \
            -czf "${DISTDIR}/${dist_name}" -C /src .
        source_block='SRC_URI="file://'"${DISTDIR}/${dist_name}"' -> ${P}.tar.gz"'
    fi
fi

ebuild_file="${package_dir}/latte-dock-ng-${version}.ebuild"
if [[ -n "${GENTOO_EBUILD_TEMPLATE:-}" ]]; then
    cp "$GENTOO_EBUILD_TEMPLATE" "$ebuild_file"
    if [[ -n "${GENTOO_METADATA_TEMPLATE:-}" ]]; then
        cp "$GENTOO_METADATA_TEMPLATE" "${package_dir}/metadata.xml"
    fi
    if [[ "${GENTOO_VERIFY_FETCH_TAG:-false}" != "true" ]]; then
        sed -i "s|^SRC_URI=.*|SRC_URI=\"file://${DISTDIR}/${dist_name} -> \${P}.tar.gz\"|" "$ebuild_file"
    fi
else
cat > "$ebuild_file" <<EOF
# Copyright 1999-2026 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=8

${inherit_line}

DESCRIPTION="Wayland-first Latte Dock NG for Plasma 6.5+"
HOMEPAGE="https://github.com/ruizhi-lab/latte-dock-ng"
${source_block}

LICENSE="GPL-2+ GPL-3+ LGPL-2+ || ( LGPL-2.1 LGPL-3 )"
SLOT="0"
${keywords_line}
${properties_line}

COMMON_DEPEND="
	>=dev-libs/plasma-wayland-protocols-1.6
	>=dev-libs/wayland-1.22
	>=dev-qt/qtbase-6.6:6[dbus,gui,widgets]
	>=dev-qt/qtdeclarative-6.6:6
	>=dev-qt/qtwayland-6.6:6
	>=kde-frameworks/karchive-6.0:6
	>=kde-frameworks/kcmutils-6.0:6
	>=kde-frameworks/kconfig-6.0:6
	>=kde-frameworks/kcoreaddons-6.0:6
	>=kde-frameworks/kcrash-6.0:6
	>=kde-frameworks/kdbusaddons-6.0:6
	>=kde-frameworks/kdeclarative-6.0:6
	>=kde-frameworks/kglobalaccel-6.0:6
	>=kde-frameworks/kguiaddons-6.0:6
	>=kde-frameworks/ki18n-6.0:6
	>=kde-frameworks/kiconthemes-6.0:6
	>=kde-frameworks/kio-6.0:6
	>=kde-frameworks/kirigami-6.0:6
	>=kde-frameworks/kitemmodels-6.0:6
	>=kde-frameworks/knewstuff-6.0:6
	>=kde-frameworks/knotifications-6.0:6
	>=kde-frameworks/kpackage-6.0:6
	>=kde-frameworks/ksvg-6.0:6
	>=kde-frameworks/kwindowsystem-6.0:6
	>=kde-frameworks/kxmlgui-6.0:6
	>=kde-plasma/kpipewire-6.5:6
	>=kde-plasma/kscreenlocker-6.3:6
	>=kde-plasma/kwayland-6.5:6
	>=kde-plasma/layer-shell-qt-6.5:6
	>=kde-plasma/libplasma-6.5:6
	>=kde-plasma/plasma-activities-6.5:6
	>=kde-plasma/plasma-activities-stats-6.5:6
	>=kde-plasma/plasma-workspace-6.5:6
"
RDEPEND="\${COMMON_DEPEND}"
DEPEND="\${COMMON_DEPEND}"
BDEPEND="
${git_bdepend}
	>=dev-qt/qtbase-6.6:6
	>=kde-frameworks/extra-cmake-modules-6.0
	virtual/pkgconfig
"

src_configure() {
	local mycmakeargs=(
		-DBUILD_TESTING=OFF
	)

	cmake_src_configure
}

src_install() {
	cmake_src_install
}

pkg_postinst() {
	xdg_pkg_postinst
}

pkg_postrm() {
	xdg_pkg_postrm
}
EOF
fi

cd "${overlay}"
if [[ "$version" != "9999" ]]; then
    python3 /src/scripts/release-version.py ebuild --file "$ebuild_file"
fi
if [[ -n "${GENTOO_EBUILD_TEMPLATE:-}" ]]; then
    cat > /etc/portage/repos.conf/latte-overlay.conf <<EOF
[latte-overlay]
location = ${overlay}
masters = gentoo
auto-sync = no
EOF
    pkgdev manifest -d "${DISTDIR}" "${package_dir}"
    pkgcheck scan --repo "${overlay}" kde-misc/latte-dock-ng
    xmllint --noout "${package_dir}/metadata.xml"
else
    ebuild "${package_dir}/latte-dock-ng-${version}.ebuild" manifest
fi
ebuild "${package_dir}/latte-dock-ng-${version}.ebuild" clean configure compile install

echo "=== Gentoo ebuild verification succeeded for latte-dock-ng-${version} ==="
