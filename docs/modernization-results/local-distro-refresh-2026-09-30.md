# Refreshed local distribution image validation

Date: 2026-09-30

The local Compose images were refreshed before validation. Compose uses
`docker.m.daocloud.io` for its upstream base images, so image pulls do not rely
on direct Docker Hub access. The global Docker daemon mirror list was empty;
the Compose-level mirror was sufficient for these builds.

## Image refresh

Each command refreshed upstream image layers and rebuilt distro images without
reusing build layers:

```sh
docker compose -f docker/docker-compose.yml build --pull --no-cache arch fedora
docker compose -f docker/docker-compose.yml build --pull --no-cache debian ubuntu
docker compose -f docker/docker-compose.yml build --pull --no-cache opensuse mageia
docker compose -f docker/docker-compose.yml build --pull --no-cache nixos
docker compose -f docker/docker-compose.yml build --pull gentoo
```

The Gentoo image build compiled its current Qt/Plasma dependency stack from
source. Its image build completed successfully with Qt 6.11.2, KDE Frameworks
6.29.0 and Plasma 6.7.5. The other refreshed rolling/release images were
validated below; package versions are those present in the refreshed images on
the validation date.

## Install and uninstall matrix

The full project container script was run in each image after refresh:

```sh
JOBS=8 docker compose -f docker/docker-compose.yml run --rm --no-deps --pull never <service>
```

| Service | Result | Observed tool/framework versions |
| --- | --- | --- |
| `debian` | Passed system install, manifest uninstall, manifestless uninstall, user install and uninstall | Debian 13.7; CMake 3.31.6-2; Qt 6.8.2+dfsg-9+deb13u2; KCoreAddons 6.13.0; Plasma 6.3.6 |
| `arch` | Passed full project script | CMake 4.4.3; Qt 6.11.2; KCoreAddons 6.30.0 |
| `fedora` | Passed full project script | CMake 4.3.0; Qt 6.11.2; KDE Frameworks 6.30.0 |
| `opensuse` | Passed full project script | CMake 4.4.3; Qt 6.11.2; KDE Frameworks 6.30.0 |
| `mageia` | Passed full project script | CMake 4.1.3; Qt 6.10.0; KDE Frameworks 6.22.0 |
| `ubuntu` | Passed full project script | CMake 4.2.3; Qt 6.10.2; KDE Frameworks 6.24.0 |
| `gentoo` | Passed ebuild configure, compile and install using this branch's source archive | Qt 6.11.2; KDE Frameworks 6.29.0; Plasma 6.7.5 |

Gentoo requires selecting the repository snapshot rather than the ebuild's
default live `9999` source:

```sh
docker compose -f docker/docker-compose.yml run --rm --no-deps --pull never \
    -e VERSION=1.2.51 gentoo
```

The Arch container's Plasma workspace runtime is unavailable in its headless
image; its build/install script still passed. Gentoo's ebuild workflow verifies
configure, compilation and installation, and does not provide the manifest
uninstall sequence used by the other distro script.

## NixOS validation

The refreshed local NixOS image completed the package install/uninstall,
`nix flake check`, `.#default` build and GCC Debug development-preset build.
The local image uses Tsinghua's nixpkgs mirror and binary cache first, with
`cache.nixos.org` as fallback. These commands verified the generated Compose
settings:

```sh
docker compose -f docker/docker-compose.yml build --pull nixos
docker compose -f docker/docker-compose.yml run --rm --no-deps --pull never nixos \
    sh -lc 'nix-channel --list && nix --extra-experimental-features nix-command config show | grep "^substituters"'
```

The channel output was
`https://mirrors.tuna.tsinghua.edu.cn/nix-channels/nixpkgs-unstable`; the
substituter output listed
`https://mirrors.tuna.tsinghua.edu.cn/nix-channels/store` before
`https://cache.nixos.org/`. The local verification passed all 46 registered
CTest targets. GitHub run `36668396118` passed all 20 jobs, including NixOS
install verification. This closes the earlier QtQml `qmlplugin` runtime-path
failures.

## Updated local VMs

Both VMs were refreshed before runtime validation. Fedora 44 had a successful
80-package `dnf update` transaction on 2026-09-30; a subsequent
`dnf check-update --refresh` refreshed repository metadata and reported no
pending package updates. The VM reports CMake 4.3.0, Qt 6.11.2, KDE Frameworks
6.30, Plasma 6.7.5 and kernel 7.2.7.

Debian 13 still had 14 security packages pending after the user's earlier
manual update. Its main and updates repositories already used USTC, while the
security entries still used `security.debian.org`. The verified Chinese mirror
responded successfully, so the security entries were switched to HTTPS USTC
after preserving `/etc/apt/sources.list.codex-backup`. The exact refresh and
upgrade commands were:

```sh
sudo apt-get update
sudo DEBIAN_FRONTEND=noninteractive apt-get upgrade -y
sudo apt-get -s full-upgrade
sudo DEBIAN_FRONTEND=noninteractive apt-get full-upgrade -y
```

The first upgrade installed 13 security updates. The reviewed full upgrade
added only the Debian 13.7 security kernel `6.12.111-1` and updated its meta
package; it removed no packages. The 108 MB kernel archive downloaded from
`https://mirrors.ustc.edu.cn/debian-security` in two seconds. The VM was
rebooted to activate it, `uname -r` reported `6.12.111+deb13-amd64`, and
`apt list --upgradable` returned no packages.

The subsequent Fedora UI profile attempt is recorded separately in
[P0](P0.md); its partial samples are invalid and are not acceptance evidence.
