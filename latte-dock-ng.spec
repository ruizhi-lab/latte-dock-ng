# SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

Name:           latte-dock-ng
Version:        {{{ latte_git_version }}}
Release:        1%{?dist}
Summary:        Wayland-first dock for KDE Plasma 6
License:        GPL-3.0-or-later
URL:            https://github.com/ruizhi-lab/latte-dock-ng
Source0:        {{{ latte_git_archive }}}

%if 0%{?suse_version}
BuildRequires:  cmake, extra-cmake-modules, gcc-c++, gettext, make, pkg-config
BuildRequires:  qt6-base-devel, qt6-declarative-devel, qt6-wayland-devel
BuildRequires:  libplasma6-devel, plasma6-activities-devel
BuildRequires:  plasma6-activities-stats-devel, plasma6-workspace-devel
BuildRequires:  kwayland6-devel
BuildRequires:  kf6-kconfig-devel, kf6-kcoreaddons-devel, kf6-kguiaddons-devel
BuildRequires:  kf6-kdbusaddons-devel, kf6-kdeclarative-devel, kf6-kitemmodels-devel
BuildRequires:  kf6-kxmlgui-devel, kf6-kiconthemes-devel, kf6-kio-devel
BuildRequires:  kf6-ki18n-devel, kf6-knotifications-devel, kf6-knewstuff-devel
BuildRequires:  kf6-karchive-devel, kf6-kglobalaccel-devel, kf6-kcrash-devel
BuildRequires:  kf6-kwindowsystem-devel, kf6-kpackage-devel, kf6-ksvg-devel
BuildRequires:  plasma-wayland-protocols, wayland-devel, layer-shell-qt6-devel
Requires:       kf6-kirigami, kf6-kcmutils, kf6-knewstuff
%elif 0%{?mdkversion}
BuildRequires:  cmake, extra-cmake-modules, gcc-c++, gettext, make, pkgconf-pkg-config
BuildRequires:  qtbase6-common-devel, lib64qt6base6-devel
BuildRequires:  lib64qt6qml-devel, lib64qt6quick-devel, lib64qt6quickwidgets-devel
BuildRequires:  lib64qt6wayland-devel, lib64qt6waylandclient-devel
BuildRequires:  lib64plasma-devel, lib64plasmaactivities-devel
BuildRequires:  lib64plasmaactivitiesstats-devel, lib64plasma-workspace-devel
BuildRequires:  lib64kwayland-devel
BuildRequires:  lib64kf6config-devel, lib64kf6coreaddons-devel, lib64kf6guiaddons-devel
BuildRequires:  lib64kf6dbusaddons-devel, lib64kf6declarative-devel
BuildRequires:  lib64kf6itemmodels-devel, lib64kf6xmlgui-devel, lib64kf6iconthemes-devel
BuildRequires:  lib64kf6kio-devel, lib64kf6i18n-devel, lib64kf6notifications-devel
BuildRequires:  lib64kf6newstuff-devel, lib64kf6archive-devel, lib64kf6globalaccel-devel
BuildRequires:  lib64kf6crash-devel, lib64kf6windowsystem-devel
BuildRequires:  lib64kf6package-devel, lib64kf6svg-devel
BuildRequires:  plasma-wayland-protocols-devel, lib64wayland-devel, lib64glvnd-devel
BuildRequires:  lib64layer-shell-qt-devel
Requires:       lib64kirigami6, lib64kf6kcmutils6, lib64kf6newstuffcore6
%else
BuildRequires:  cmake, extra-cmake-modules, gcc-c++, gettext, make, pkgconf-pkg-config
BuildRequires:  qt6-qtbase-devel, qt6-qtdeclarative-devel, qt6-qtwayland-devel
BuildRequires:  kf6-plasma-devel, plasma-activities-devel
BuildRequires:  plasma-activities-stats-devel, plasma-workspace-devel, kwayland-devel
BuildRequires:  kf6-kconfig-devel, kf6-kcoreaddons-devel, kf6-kguiaddons-devel
BuildRequires:  kf6-kdbusaddons-devel, kf6-kdeclarative-devel, kf6-kitemmodels-devel
BuildRequires:  kf6-kxmlgui-devel, kf6-kiconthemes-devel, kf6-kio-devel
BuildRequires:  kf6-ki18n-devel, kf6-knotifications-devel, kf6-knewstuff-devel
BuildRequires:  kf6-karchive-devel, kf6-kglobalaccel-devel, kf6-kcrash-devel
BuildRequires:  kf6-kwindowsystem-devel, kf6-kpackage-devel, kf6-ksvg-devel
BuildRequires:  plasma-wayland-protocols-devel, wayland-devel, layer-shell-qt-devel
Requires:       kf6-kirigami, kf6-kcmutils, kf6-knewstuff
%endif

%description
Latte Dock NG is a Wayland-first dock for KDE Plasma 6.3 and newer. It
provides an animated dock for tasks and widgets.

%prep
{{{ latte_git_setup_macro }}}

%build
%cmake -DCMAKE_INSTALL_PREFIX=%{_prefix} \
       -DCMAKE_BUILD_TYPE=Release \
       -DBUILD_TESTING=OFF \
       -DLATTE_RPM_PACKAGE_REQUIRES="kf6-kirigami, kf6-kcmutils, kf6-knewstuff"
%cmake_build

%post
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -q -t /usr/share/icons/hicolor 2>/dev/null || :
fi
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database -q 2>/dev/null || :
fi

%postun
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -q -t /usr/share/icons/hicolor 2>/dev/null || :
fi
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database -q 2>/dev/null || :
fi

# The CMake install manifest follows the selected distro's library/QML paths.
%install
# Remove DESTDIR if CMake recorded it in the manifest; RPM paths are relative
# to the package root. CMake records /usr/... paths when DESTDIR is not present.
%cmake_install
manifest_file="$(find . -type f -name install_manifest.txt -print -quit)"
if [ -z "$manifest_file" ]; then
    echo "CMake install_manifest.txt was not found" >&2
    exit 1
fi
sed -i 's#^%{buildroot}##' "$manifest_file"
# RPM treats whitespace as a file-list separator; escape spaces in installed paths.
sed -i 's/ /\\ /g' "$manifest_file"
cp "$manifest_file" latte-dock-ng-install-manifest.txt

%files -f latte-dock-ng-install-manifest.txt

%changelog
* Sat Oct 03 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com> - 1.2.54-1
- Initial COPR packaging.
