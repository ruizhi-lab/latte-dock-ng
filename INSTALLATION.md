Installation
============

> This fork targets **KDE Plasma 6.3+ on Wayland only** (amd64/x86\_64 architecture). Development happens on Plasma 6.5+ / Qt 6.11; Plasma 6.3 (Debian 13 trixie) is verified as the minimum supported version. X11 and other architectures are not supported.
> All dependency packages below are Qt6 / KF6. Legacy Qt5/KF5 package names from upstream will not work.
>
> Two .deb variants are attached to each release because the libplasma
> soname changed from 6 to 7 between Plasma 6.5 and 6.6 (Debian package
> `libplasma6` → `libplasma7`), which splits the supported range into two
> binary-incompatible camps:
> - **`..._amd64.deb`** (plain name) — built on **Debian sid**, links `libplasma7`; for **Debian testing / sid and Ubuntu 26.04+**.
> - **`...-1+deb13u1_amd64.deb`** — built on **Debian 13 (trixie)**, links `libplasma6`; the `+deb13u1` revision marks the **Debian 13 (stable)** build.
> Do not build either variant on a newer distro than its target — versioned
> dependencies (`dpkg-shlibdeps`) would raise the lower bound and break
> installation.
>
> Debian 13 (trixie) is the current stable baseline. The CI matrix checks
> Debian sid, Fedora, openSUSE, Mageia, Ubuntu, Arch, and NixOS on every
> `main` push. Native package installation is tested separately from source
> installation, so a package that only builds but cannot be installed is rejected.
> Gentoo is best verified on a native Gentoo host, where Portage can reuse its
> configured signed binhost instead of rebuilding the full Plasma stack.
>
> **openSUSE CI status (2026-10-03):** the latest recorded main-branch run
> failed while installing dependencies in the openSUSE packaging and
> verification container images, before compiling or installing the RPM.
> `zypper` reported a Tumbleweed dependency conflict involving
> `glib2-stage1-devel` and `libpcre2-8-0`. See the
> [failed run](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/37113104585).
> The Build workflow now uses the upstream openSUSE repositories, matching the
> Release workflow; rerun CI to confirm the fix.
> This CI image failure does not establish whether the separately configured
> OBS release-tag automation works; that remains to be checked after a new tag.
> Fedora COPR builds and the Debian/Ubuntu APT repository are verified. Mageia
> has a release RPM asset but no maintained package repository; follow the
> release workflow notes for repository status.

## Kubuntu / KDE Neon (26.04+)

```bash
sudo apt install \
  cmake extra-cmake-modules \
  qt6-base-dev qt6-base-dev-tools qt6-declarative-dev qt6-wayland-dev \
  libplasma-dev libplasmaactivities-dev libplasmaactivitiesstats-dev plasma-workspace-dev kwayland-dev \
  libkf6config-dev libkf6coreaddons-dev libkf6guiaddons-dev libkf6dbusaddons-dev \
  libkf6kcmutils-dev \
  libkf6declarative-dev libkf6itemmodels-dev libkf6xmlgui-dev libkf6iconthemes-dev \
  libkf6kio-dev libkf6i18n-dev libkf6notifications-dev \
  libkf6newstuff-dev libkf6archive-dev libkf6globalaccel-dev \
  libkf6crash-dev libkf6windowsystem-dev libkf6package-dev libkf6svg-dev \
  plasma-wayland-protocols libwayland-dev \
  liblayershellqtinterface-dev \
  gettext build-essential git pkgconf
```

## Debian and Ubuntu (13 trixie / Testing / Ubuntu 26.04+)

Debian 13 (trixie) ships Plasma 6.3.6 and is the minimum supported version; the same build instructions apply to testing and sid.

The signed [Latte Dock NG APT repository](https://ruizhi-lab.github.io/latte-dock-ng/)
supports amd64. Add its signing key:

```bash
sudo install -d -m 0755 /etc/apt/keyrings
curl -fsSL https://ruizhi-lab.github.io/latte-dock-ng/latte-dock-ng-archive-keyring.gpg \
  | sudo tee /etc/apt/keyrings/latte-dock-ng.gpg >/dev/null
sudo chmod 0644 /etc/apt/keyrings/latte-dock-ng.gpg
```

Add **one** source line matching your OS release:

```bash
# Debian 13 (trixie)
echo 'deb [arch=amd64 signed-by=/etc/apt/keyrings/latte-dock-ng.gpg] https://ruizhi-lab.github.io/latte-dock-ng trixie main' \
  | sudo tee /etc/apt/sources.list.d/latte-dock-ng.list

# OR Debian testing
echo 'deb [arch=amd64 signed-by=/etc/apt/keyrings/latte-dock-ng.gpg] https://ruizhi-lab.github.io/latte-dock-ng testing main' \
  | sudo tee /etc/apt/sources.list.d/latte-dock-ng.list

# OR Ubuntu 26.04+
echo 'deb [arch=amd64 signed-by=/etc/apt/keyrings/latte-dock-ng.gpg] https://ruizhi-lab.github.io/latte-dock-ng ubuntu main' \
  | sudo tee /etc/apt/sources.list.d/latte-dock-ng.list
```

Then install or upgrade:

```bash
sudo apt update
sudo apt install latte-dock-ng
```

The Ubuntu suite uses the Debian testing package, which CI verifies on Ubuntu
26.04. Do not add multiple suites to the same system. Maintainer instructions
for the repository are in [`docs/debian-apt-repository.md`](docs/debian-apt-repository.md).

Alternatively, download a prebuilt `.deb` from the
[GitHub release](https://github.com/ruizhi-lab/latte-dock-ng/releases) — see
the two-variant note at the top of this page:

```bash
# Debian testing / sid (and Ubuntu 26.04+)
sudo apt install ./latte-dock-ng_<ver>-1_amd64.deb
# Debian 13 (stable)
sudo apt install ./latte-dock-ng_<ver>-1+deb13u1_amd64.deb
```

```bash
sudo apt install \
  cmake extra-cmake-modules \
  qt6-base-dev qt6-base-dev-tools qt6-declarative-dev qt6-wayland-dev \
  libplasma-dev libplasmaactivities-dev libplasmaactivitiesstats-dev plasma-workspace-dev kwayland-dev \
  libkf6config-dev libkf6coreaddons-dev libkf6guiaddons-dev libkf6dbusaddons-dev \
  libkf6kcmutils-dev \
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
  kcmutils \
  kdeclarative kitemmodels kxmlgui kiconthemes kio ki18n knotifications \
  knewstuff karchive kpackage kglobalaccel kcrash kwindowsystem ksvg \
  plasma-wayland-protocols wayland \
  layer-shell-qt \
  gcc gettext git pkgconf
```

## Gentoo

Install from the personal overlay:

```bash
eselect repository add ruizhi-overlay git https://github.com/ruizhi-lab/gentoo-overlay.git
emaint sync -r ruizhi-overlay
emerge -av kde-misc/latte-dock-ng
```

## Fedora (44+)

Install the prebuilt package from Fedora COPR:

```bash
sudo dnf copr enable ruizhi-lab/latte-dock-ng
sudo dnf install latte-dock-ng
```

The COPR build and installation have been verified on Fedora 44. To build
from source instead, install these dependencies:

```bash
sudo dnf install \
  cmake extra-cmake-modules \
  qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtwayland-devel \
  kf6-plasma-devel plasma-activities-devel plasma-activities-stats-devel plasma-workspace-devel kwayland-devel \
  kf6-kconfig-devel kf6-kcoreaddons-devel kf6-kguiaddons-devel kf6-kdbusaddons-devel \
  kf6-kcmutils-devel \
  kf6-kdeclarative-devel kf6-kitemmodels-devel kf6-kxmlgui-devel kf6-kiconthemes-devel \
  kf6-kio-devel kf6-ki18n-devel kf6-knotifications-devel \
  kf6-knewstuff-devel kf6-karchive-devel kf6-kglobalaccel-devel \
  kf6-kcrash-devel kf6-kwindowsystem-devel kf6-kpackage-devel kf6-ksvg-devel \
  plasma-wayland-protocols-devel wayland-devel \
  layer-shell-qt-devel \
  gcc-c++ gettext git pkgconf-pkg-config
```

## openSUSE Tumbleweed

Install the prebuilt package from OBS:

```bash
sudo zypper addrepo --refresh \
  https://download.opensuse.org/repositories/home:/ruizhi-lab/openSUSE_Tumbleweed/home:ruizhi-lab.repo
sudo zypper refresh
sudo zypper install latte-dock-ng
```

This repository targets Tumbleweed x86_64 only; do not use it on Leap. The
[OBS package page](https://build.opensuse.org/package/show/home:ruizhi-lab/latte-dock-ng)
shows its current build status. To build from source instead, install these
dependencies:

```bash
sudo zypper install \
  cmake extra-cmake-modules \
  qt6-base-devel qt6-declarative-devel qt6-wayland-devel \
  libplasma6-devel plasma6-activities-devel plasma6-activities-stats-devel plasma6-workspace-devel kwayland6-devel \
  kf6-kconfig-devel kf6-kcoreaddons-devel kf6-kguiaddons-devel kf6-kdbusaddons-devel \
  kf6-kcmutils-devel \
  kf6-kdeclarative-devel kf6-kitemmodels-devel kf6-kxmlgui-devel kf6-kiconthemes-devel \
  kf6-kio-devel kf6-ki18n-devel kf6-knotifications-devel \
  kf6-knewstuff-devel kf6-karchive-devel kf6-kglobalaccel-devel \
  kf6-kcrash-devel kf6-kwindowsystem-devel kf6-kpackage-devel kf6-ksvg-devel \
  plasma-wayland-protocols wayland-devel \
  layer-shell-qt6-devel \
  gcc-c++ gettext git pkgconf
```

## Mageia (10+)

Mageia has no maintained package repository. Download the Mageia-specific RPM
from [GitHub Releases](https://github.com/ruizhi-lab/latte-dock-ng/releases)
and install that release asset with Mageia's package manager. Do not use the
Fedora or openSUSE RPM. To build from source, install these dependencies:

```bash
sudo dnf install \
  cmake extra-cmake-modules \
  qtbase6-common-devel lib64qt6base6-devel \
  lib64qt6qml-devel lib64qt6quick-devel lib64qt6quickwidgets-devel \
  lib64qt6wayland-devel lib64qt6waylandclient-devel \
  lib64plasma-devel lib64plasmaactivities-devel lib64plasmaactivitiesstats-devel \
  lib64plasma-workspace-devel lib64kwayland-devel \
  lib64kf6config-devel lib64kf6coreaddons-devel lib64kf6guiaddons-devel lib64kf6dbusaddons-devel \
  lib64kcmutils-devel \
  lib64kf6declarative-devel lib64kf6itemmodels-devel lib64kf6xmlgui-devel lib64kf6iconthemes-devel \
  lib64kf6kio-devel lib64kf6i18n-devel lib64kf6notifications-devel \
  lib64kf6newstuff-devel lib64kf6archive-devel lib64kf6globalaccel-devel \
  lib64kf6crash-devel lib64kf6windowsystem-devel lib64kf6package-devel lib64kf6svg-devel \
  plasma-wayland-protocols-devel lib64wayland-devel lib64glvnd-devel \
  lib64layer-shell-qt-devel \
  gcc-c++ gettext git pkgconf-pkg-config
```

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

## NixOS

### Prerequisites

Unlike the distros above, there's no dependency-install step. `default.nix`
lists every Qt6/KF6 dependency and Nix builds them all in one derivation.
Have Nix installed and skip straight to building.

For getting it installed on your system rather than building it locally,
add the flake as an input and apply its module:

```nix
# flake.nix
inputs.latte-dock-ng.url = "github:ruizhi-lab/latte-dock-ng";

# in your nixosSystem call
nixpkgs.lib.nixosSystem {
  system = "x86_64-linux";
  modules = [
    inputs.latte-dock-ng.nixosModules.default
    ./configuration.nix
  ];
};
```

The module exposes `pkgs.latte-dock-ng`; add it to system packages:

```nix
# configuration.nix
{ pkgs, ... }: {
  environment.systemPackages = [ pkgs.latte-dock-ng ];
}
```

Or build/run it without adding it as a flake input:

```bash
nix build github:ruizhi-lab/latte-dock-ng
nix run github:ruizhi-lab/latte-dock-ng
```

### Ad hoc, without cloning

```bash
nix-build -E 'with import <nixpkgs> {}; callPackage (fetchTarball "https://github.com/ruizhi-lab/latte-dock-ng/archive/refs/heads/main.tar.gz") {}'
./result/bin/latte-dock-ng
```

### From a local clone

```bash
git clone https://github.com/ruizhi-lab/latte-dock-ng.git
cd latte-dock-ng
nix-build
./result/bin/latte-dock-ng
```

## Docker Build Verification

Docker images are provided to verify the build on each supported distribution.
Local Docker runs use USTC mirrors by default for fast package downloads in
China; GitHub Actions explicitly uses each distribution's official mirrors.

```bash
cd docker
docker compose run --rm arch       # Arch Linux
docker compose run --rm fedora     # Fedora 44
docker compose run --rm opensuse   # openSUSE Tumbleweed
docker compose run --rm mageia     # Mageia 10
docker compose run --rm ubuntu     # Ubuntu 26.04
docker compose run --rm debian     # Debian 13 (current stable)
docker compose run --rm nixos      # NixOS (nixos-unstable)
```

For Debian sid and Gentoo, run the corresponding verifier directly:

```bash
docker build -f Dockerfile.debian-sid -t latte-debian-sid .
docker run --rm -v "$PWD/..:/src:ro" --tmpfs /build:exec -w /build \
  latte-debian-sid bash /src/docker/verify-install.sh debian-sid
docker build -f Dockerfile.gentoo -t latte-gentoo .
docker run --rm -v "$PWD/..:/src:ro" --tmpfs /build:exec -w /build \
  latte-gentoo bash /src/docker/verify-ebuild-gentoo.sh
```

Each command builds a container with all dependencies installed, then runs
the full verification pipeline:
1. cmake configure + compile
2. cmake --install (system install to /usr)
3. Verify key installed files exist
4. Uninstall dry-run + actual uninstall
5. Verify files are removed

A successful run ends with `=== <DISTRO>: BUILD + INSTALL + UNINSTALL SUCCESS ===`.

The GitHub Actions package job additionally builds and installs the native
`pkg.tar.zst`, RPM, or DEB with the target distribution's package manager.

`nixos` follows a different pipeline, since there's no apt/cmake step: it
builds `default.nix` with `nix-build`, then installs and uninstalls it with
`nix-env` instead of `install.sh`/`uninstall.sh`. `Dockerfile.nixos` points
at `nixos-unstable` explicitly and refreshes it at build time, rather than
relying on whatever channel ships with the base image.
