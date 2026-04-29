Installation
============

> This fork targets **KDE Plasma 6.6+ on Wayland only**. X11 is not supported.
> All dependency packages below are Qt6 / KF6. Legacy Qt5/KF5 package names from upstream will not work.

## Kubuntu / KDE Neon (24.04+)

```bash
sudo apt install \
  cmake extra-cmake-modules \
  qt6-base-dev qt6-declarative-dev qt6-wayland-dev \
  libkf6plasma-dev libkf6plasmaquick-dev libkf6activities-dev \
  libkf6coreaddons-dev libkf6guiaddons-dev libkf6dbusaddons-dev \
  libkf6declarative-dev libkf6xmlgui-dev libkf6iconthemes-dev \
  libkf6kio-dev libkf6i18n-dev libkf6notifications-dev \
  libkf6newstuff-dev libkf6archive-dev libkf6globalaccel-dev \
  libkf6crash-dev libkf6windowsystem-dev \
  plasma-wayland-protocols libwayland-dev \
  gettext build-essential git
```

## Arch Linux

```bash
sudo pacman -Syu
sudo pacman -S \
  cmake extra-cmake-modules \
  qt6-base qt6-declarative qt6-wayland \
  kf6-plasma kf6-kactivities kf6-kcoreaddons kf6-kguiaddons \
  kf6-kdbusaddons kf6-kdeclarative kf6-kxmlgui kf6-kiconthemes \
  kf6-kio kf6-ki18n kf6-knotifications kf6-knewstuff kf6-karchive \
  kf6-kglobalaccel kf6-kcrash kf6-kwindowsystem \
  plasma-wayland-protocols wayland
```

## Fedora / RHEL

```bash
sudo dnf install \
  cmake extra-cmake-modules \
  qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtwayland-devel \
  kf6-plasma-devel kf6-kactivities-devel kf6-kcoreaddons-devel \
  kf6-kguiaddons-devel kf6-kdbusaddons-devel kf6-kdeclarative-devel \
  kf6-kxmlgui-devel kf6-kiconthemes-devel kf6-kio-devel \
  kf6-ki18n-devel kf6-knotifications-devel kf6-knewstuff-devel \
  kf6-karchive-devel kf6-kglobalaccel-devel kf6-kcrash-devel \
  kf6-kwindowsystem-devel \
  plasma-wayland-protocols-devel wayland-devel \
  gcc-c++ gettext git
```

## openSUSE Tumbleweed

```bash
sudo zypper install \
  cmake extra-cmake-modules \
  qt6-base-devel qt6-declarative-devel qt6-wayland-devel \
  kf6-plasma-devel kf6-kactivities-devel kf6-kcoreaddons-devel \
  kf6-kguiaddons-devel kf6-kdbusaddons-devel kf6-kdeclarative-devel \
  kf6-kxmlgui-devel kf6-kiconthemes-devel kf6-kio-devel \
  kf6-ki18n-devel kf6-knotifications-devel kf6-knewstuff-devel \
  kf6-karchive-devel kf6-kglobalaccel-devel kf6-kcrash-devel \
  kf6-kwindowsystem-devel \
  plasma-wayland-protocols wayland-devel \
  gcc-c++ gettext git
```

## Building and Installing

```bash
git clone https://github.com/ruizhi-lab/latte-dock-ng.git
cd latte-dock-ng
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make -j$(nproc)
sudo make install
```

Or using the provided install script:

```bash
bash install.sh
```

Install script options:

```bash
bash install.sh --help
bash install.sh Debug
bash install.sh RelWithDebInfo --enable-make-unique
bash install.sh --translations
bash install.sh --translations-stable
```

## Uninstall

```bash
bash uninstall.sh
```

Uninstall script options:

```bash
bash uninstall.sh --help
bash uninstall.sh --dry-run
bash uninstall.sh --manifest build/install_manifest.txt --dry-run
```
