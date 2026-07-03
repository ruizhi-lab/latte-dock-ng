Installation
============

> This fork targets **KDE Plasma 6.5+ on Wayland only** (amd64/x86\_64 architecture). X11 and other architectures are not supported.
> All dependency packages below are Qt6 / KF6. Legacy Qt5/KF5 package names from upstream will not work.

## Kubuntu / KDE Neon (26.04+)

```bash
sudo apt install \
  cmake extra-cmake-modules \
  qt6-base-dev qt6-base-dev-tools qt6-declarative-dev qt6-wayland-dev \
  libplasma-dev libplasmaactivities-dev libplasmaactivitiesstats-dev plasma-workspace-dev kwayland-dev \
  libkf6config-dev libkf6coreaddons-dev libkf6guiaddons-dev libkf6dbusaddons-dev \
  libkf6declarative-dev libkf6itemmodels-dev libkf6xmlgui-dev libkf6iconthemes-dev \
  libkf6kio-dev libkf6i18n-dev libkf6notifications-dev \
  libkf6newstuff-dev libkf6archive-dev libkf6globalaccel-dev \
  libkf6crash-dev libkf6windowsystem-dev libkf6package-dev libkf6svg-dev \
  plasma-wayland-protocols libwayland-dev \
  liblayershellqtinterface-dev \
  gettext build-essential git pkgconf
```

## Debian (Testing / Unstable)

```bash
sudo apt install \
  cmake extra-cmake-modules \
  qt6-base-dev qt6-base-dev-tools qt6-declarative-dev qt6-wayland-dev \
  libplasma-dev libplasmaactivities-dev libplasmaactivitiesstats-dev plasma-workspace-dev kwayland-dev \
  libkf6config-dev libkf6coreaddons-dev libkf6guiaddons-dev libkf6dbusaddons-dev \
  libkf6declarative-dev libkf6itemmodels-dev libkf6xmlgui-dev libkf6iconthemes-dev \
  libkf6kio-dev libkf6i18n-dev libkf6notifications-dev \
  libkf6newstuff-dev libkf6archive-dev libkf6globalaccel-dev \
  libkf6crash-dev libkf6windowsystem-dev libkf6package-dev libkf6svg-dev \
  plasma-wayland-protocols libwayland-dev \
  liblayershellqtinterface-dev \
  gettext build-essential git pkgconf
```

## Arch Linux

```bash
sudo pacman -Syu
sudo pacman -S \
  cmake extra-cmake-modules \
  qt6-base qt6-declarative qt6-wayland \
  libplasma plasma-activities plasma-activities-stats plasma-workspace kwayland \
  kconfig kcoreaddons kguiaddons kdbusaddons \
  kdeclarative kitemmodels kxmlgui kiconthemes kio ki18n knotifications \
  knewstuff karchive kpackage kglobalaccel kcrash kwindowsystem ksvg \
  plasma-wayland-protocols wayland \
  layer-shell-qt \
  gcc gettext git pkgconf
```

## Fedora (44+)

```bash
sudo dnf install \
  cmake extra-cmake-modules \
  qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtwayland-devel \
  kf6-plasma-devel plasma-activities-devel plasma-activities-stats-devel plasma-workspace-devel kwayland-devel \
  kf6-kconfig-devel kf6-kcoreaddons-devel kf6-kguiaddons-devel kf6-kdbusaddons-devel \
  kf6-kdeclarative-devel kf6-kitemmodels-devel kf6-kxmlgui-devel kf6-kiconthemes-devel \
  kf6-kio-devel kf6-ki18n-devel kf6-knotifications-devel \
  kf6-knewstuff-devel kf6-karchive-devel kf6-kglobalaccel-devel \
  kf6-kcrash-devel kf6-kwindowsystem-devel kf6-kpackage-devel kf6-ksvg-devel \
  plasma-wayland-protocols-devel wayland-devel \
  layer-shell-qt-devel \
  gcc-c++ gettext git pkgconf-pkg-config
```

## openSUSE Tumbleweed

```bash
sudo zypper install \
  cmake extra-cmake-modules \
  qt6-base-devel qt6-declarative-devel qt6-wayland-devel \
  libplasma6-devel plasma6-activities-devel plasma6-activities-stats-devel plasma6-workspace-devel kwayland6-devel \
  kf6-kconfig-devel kf6-kcoreaddons-devel kf6-kguiaddons-devel kf6-kdbusaddons-devel \
  kf6-kdeclarative-devel kf6-kitemmodels-devel kf6-kxmlgui-devel kf6-kiconthemes-devel \
  kf6-kio-devel kf6-ki18n-devel kf6-knotifications-devel \
  kf6-knewstuff-devel kf6-karchive-devel kf6-kglobalaccel-devel \
  kf6-kcrash-devel kf6-kwindowsystem-devel kf6-kpackage-devel kf6-ksvg-devel \
  plasma-wayland-protocols wayland-devel \
  layer-shell-qt6-devel \
  gcc-c++ gettext git pkgconf
```

## Mageia (Cauldron)

```bash
sudo dnf install \
  cmake extra-cmake-modules \
  qtbase6-common-devel lib64qt6base6-devel \
  lib64qt6qml-devel lib64qt6quick-devel lib64qt6quickwidgets-devel \
  lib64qt6wayland-devel lib64qt6waylandclient-devel \
  lib64plasma-devel lib64plasmaactivities-devel lib64plasmaactivitiesstats-devel \
  lib64plasma-workspace-devel lib64kwayland-devel \
  lib64kf6config-devel lib64kf6coreaddons-devel lib64kf6guiaddons-devel lib64kf6dbusaddons-devel \
  lib64kf6declarative-devel lib64kf6itemmodels-devel lib64kf6xmlgui-devel lib64kf6iconthemes-devel \
  lib64kf6kio-devel lib64kf6i18n-devel lib64kf6notifications-devel \
  lib64kf6newstuff-devel lib64kf6archive-devel lib64kf6globalaccel-devel \
  lib64kf6crash-devel lib64kf6windowsystem-devel lib64kf6package-devel lib64kf6svg-devel \
  plasma-wayland-protocols-devel lib64wayland-devel lib64glvnd-devel \
  lib64layer-shell-qt-devel \
  gcc-c++ gettext git pkgconf-pkg-config
```

## NixOS

This repo ships a `default.nix` at the repository root, so no separate
packaging is needed — `callPackage` handles all Qt6/KF6 dependencies.

### Ad hoc, without cloning

```bash
nix-build -E 'with import <nixpkgs> {}; callPackage (fetchTarball "https://github.com/whimbree/latte-dock-ng/archive/refs/heads/main.tar.gz") {}'
./result/bin/latte-dock-ng
```

### From a local clone

```bash
git clone https://github.com/whimbree/latte-dock-ng.git
cd latte-dock-ng
nix-build
./result/bin/latte-dock-ng
```

### Adding it to your NixOS flake

Add it as a non-flake input, then build it with `callPackage` in an overlay:

```nix
# flake.nix
inputs.latte-dock-ng-src = {
  url = "github:whimbree/latte-dock-ng";
  flake = false;
};
```

```nix
# in a NixOS module, e.g. configuration.nix
{ inputs, pkgs, ... }: {
  nixpkgs.overlays = [
    (final: prev: {
      latte-dock-ng = final.callPackage inputs.latte-dock-ng-src { };
    })
  ];

  environment.systemPackages = [ pkgs.latte-dock-ng ];
}
```

Pin to a specific commit for reproducibility by using
`github:whimbree/latte-dock-ng/<commit-sha>` as the `url` instead of `main`.

## Building and Installing

**Recommended: use the install script.** It auto-detects available memory to prevent
out-of-memory failures on systems with limited RAM:

```bash
git clone https://github.com/ruizhi-lab/latte-dock-ng.git
cd latte-dock-ng
bash install.sh
```

The install script automatically:
- Picks an optimal number of parallel compile jobs based on available RAM
- Handles user-level (`~/.local`) vs system-level (`/usr`) installation
- Runs pre-install cleanup to avoid stale file conflicts

### Memory limits and OOM

Compiling C++ with many parallel jobs can consume 1-2 GiB per job. On machines with
limited RAM (VMs, single-board computers), cap the parallelism:

```bash
bash install.sh --jobs 1    # safest, slowest
bash install.sh --jobs 2    # good for 4 GiB VMs
```

### Manual build (not recommended)

If you prefer to build manually:

```bash
git clone https://github.com/ruizhi-lab/latte-dock-ng.git
cd latte-dock-ng
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
cmake --build . --parallel $(nproc)   # may OOM on small systems
sudo cmake --install .
```

### Install script options

```bash
bash install.sh --help
bash install.sh Debug                  # debug build
bash install.sh --no-clean             # skip pre-install cleanup
bash install.sh --clean --purge-user-data
bash install.sh --user Debug           # install to ~/.local for testing
bash install.sh --jobs 2               # cap at 2 parallel compile jobs
```

## Uninstall

```bash
bash uninstall.sh
```

Uninstall script options:

```bash
bash uninstall.sh --help
bash uninstall.sh --dry-run
bash uninstall.sh --purge-user-data
bash uninstall.sh --manifest build/install_manifest.txt --dry-run
```

## Docker Build Verification

Docker images are provided to verify the build on each supported distribution.
All images use USTC mirrors for fast package downloads in China.

```bash
cd docker
docker compose run --rm arch       # Arch Linux
docker compose run --rm fedora     # Fedora 44
docker compose run --rm opensuse   # openSUSE Tumbleweed
docker compose run --rm mageia     # Mageia Cauldron
docker compose run --rm ubuntu     # Ubuntu 26.04
docker compose run --rm debian     # Debian Testing
```

Each command builds a container with all dependencies installed, then runs
the full verification pipeline:
1. cmake configure + compile
2. cmake --install (system install to /usr)
3. Verify key installed files exist
4. Uninstall dry-run + actual uninstall
5. Verify files are removed

A successful run ends with `=== <DISTRO>: BUILD + INSTALL + UNINSTALL SUCCESS ===`.
```
