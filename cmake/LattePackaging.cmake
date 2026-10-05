# SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# Generates RPM (Fedora/openSUSE) or DEB (Debian/Ubuntu) packages.
# Usage: cmake --build . && cpack -G RPM   (or -G DEB)
set(CPACK_PACKAGE_NAME "latte-dock-ng")
set(CPACK_PACKAGE_VERSION "${VERSION}")
set(CPACK_PACKAGE_VENDOR "${AUTHOR}")
set(CPACK_PACKAGE_CONTACT "${EMAIL}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Latte Dock NG - Plasma 6 desktop dock")
# Long description with lines kept under 79 chars; rpmlint flags
# description-line-too-long otherwise. Replaces the CPack placeholder text.
# CPACK_RPM_PACKAGE_DESCRIPTION is set explicitly: CPackRPM does not fall
# back to CPACK_PACKAGE_DESCRIPTION on all versions.
set(CPACK_PACKAGE_DESCRIPTION "Latte Dock NG is a Wayland-first dock for KDE Plasma 6.3+
that provides an elegant and intuitive experience for your tasks and
widgets. It animates its contents using a parabolic zoom effect and
stays out of the way when not needed.")
set(CPACK_PACKAGE_HOMEPAGE_URL "${WEBSITE}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSES/GPL-3.0.txt")

# Map the build host architecture to the package architecture names. RPM and
# DEB use different schemes (x86_64 vs amd64), so both are derived here
# instead of being hardcoded to the current x86_64 runner.
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "aarch64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^armv[67]l$")
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "armv7hl")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "armhf")
else()
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
endif()

# RPM
set(CPACK_RPM_PACKAGE_NAME "latte-dock-ng")
# The built RPM requires glibc >= 2.34 (__libc_start_main@GLIBC_2.34 is a crt1
# artifact of every modern toolchain, so the floor cannot be lowered by
# switching build hosts). It does not block the current Plasma 6.3+ package
# targets. openSUSE Leap 15.x is unsupported for shipping Plasma 5.27, not
# because of glibc.
# A single RPM is built once per version on the Fedora container and must
# install on Fedora, openSUSE and Mageia alike, so no %{?dist} dist tag is
# used and the file name stays distro-neutral. If the build is ever split
# into one RPM per distro, switch the release to "1%{?dist}" and the file
# name to "RPM-DEFAULT" to keep the NEVRA distinct. Whenever the same version
# must be re-issued (e.g. a broken package was published), bump the release
# number first — package managers treat an identical NEVRA as "already
# installed".
set(CPACK_RPM_PACKAGE_RELEASE "1")
set(CPACK_RPM_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}.${CPACK_RPM_PACKAGE_ARCHITECTURE}.rpm")
set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-or-later")
set(CPACK_RPM_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION}")
set(CPACK_RPM_PACKAGE_URL "${WEBSITE}")
set(CPACK_RPM_PACKAGE_AUTOREQ ON)
set(CPACK_RPM_PACKAGE_AUTOPROV ON)
# Refresh the icon/desktop databases on install and uninstall so new icons
# and .desktop entries appear without a session restart. The same script is
# embedded into both %post and %postun.
set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/docker/rpm-post.sh")
set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/docker/rpm-post.sh")
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
    /usr/share/applications
    /usr/share/icons
    /usr/share/latte
    /usr/share/plasma
)
# QML modules are not captured by .so auto-detection:
# - kirigami: loaded via QML import, no direct .so link from latte
# - kcmutils: loaded by the configuration shell QML
# - knewstuff: QML plugin files needed at runtime
#
# RPM capabilities for these modules are not portable across distributions:
# Mageia exposes the libraries through lib64* package names, while Fedora and
# openSUSE expose them through kf6-* packages. A bare SONAME looks plausible
# but is not a capability on Mageia/openSUSE, so the RPM becomes uninstallable.
# The packaging workflow supplies the native package names for each RPM build.
set(LATTE_RPM_PACKAGE_REQUIRES "" CACHE STRING
    "Native RPM packages required by the QML modules")
set(CPACK_RPM_PACKAGE_REQUIRES "${LATTE_RPM_PACKAGE_REQUIRES}")

# DEB
set(CPACK_DEBIAN_PACKAGE_NAME "latte-dock-ng")
# Default release; the release workflow overrides it per distro
# (-DCPACK_DEBIAN_PACKAGE_RELEASE=1+deb13u1 for the trixie build).
if(NOT CPACK_DEBIAN_PACKAGE_RELEASE)
    set(CPACK_DEBIAN_PACKAGE_RELEASE "1")
endif()
# architecture comes from the mapping at the top of this file
set(CPACK_DEBIAN_FILE_NAME "${CPACK_DEBIAN_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}-${CPACK_DEBIAN_PACKAGE_RELEASE}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")
set(CPACK_DEBIAN_PACKAGE_SECTION "kde")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${WEBSITE}")
set(CPACK_DEBIAN_PACKAGE_SHLIBS ON)
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)
# Versioned runtime library dependencies via dpkg-shlibdeps. The lower
# bound is whatever the build machine ships, so the deb must be built on
# the oldest supported distro (Debian 13 trixie) for the package to stay
# installable on Debian stable/testing/sid and Ubuntu 26.04+.
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
# QML modules not captured by .so auto-detection
set(CPACK_DEBIAN_PACKAGE_DEPENDS "qml6-module-org-kde-kirigami, qml6-module-org-kde-kcmutils, qml6-module-org-kde-newstuff")

include(CPack)
