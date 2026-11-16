# T4k Fedora scratch validation and P4 artifact

This is an isolated Fedora 45 Release build and staged install for P4. It is
not the CMake 4.5 install-preparation acceptance: the VM's system CMake is
4.3.0, below the version that added `install_prep/` and `BUILD_DEPENDS`.
The successful stage confirms the matching source builds and installs its
metadata under both roots; it does not validate the new 4.5 dependency edges
or establish that the four 4.5 author warnings are gone.

## Snapshot and toolchain

- Base HEAD: `0a6974f5cd1b13116764a2320e3d882674f5fed3`.
- Source: that HEAD plus only the four T4k module `CMakeLists.txt` files.
- Source archive SHA-256: `92607311df689f379ea27a56dcfcd9d7180db9131b80d2a97a797dc8c3665fc9`.
- `app/declarativeimports/CMakeLists.txt` SHA-256:
  `05c5169f9efec38182230d29ce792c742e59a7a7c8c6e6dc46faa56d4c4bea94`.
- `containment/CMakeLists.txt` SHA-256:
  `c8cbae6caff50945b2ec1c4ffb975f0ae5f4556fa38d5d557aafe1f663941297`.
- `declarativeimports/core/CMakeLists.txt` SHA-256:
  `03be69d92f016fa95a361eba675738e13bc9797ab47692a29e8f5fe5eeb500bc`.
- `plasmoid/CMakeLists.txt` SHA-256:
  `35a50ce61a4f383e428cf08b2a5ea4c6dee4d1c395c0f2f02c8298916f3ae454`.
- Fedora guest: `fedora@192.168.122.128`, kernel `7.2.15-300.fc45.x86_64`.
- CMake: `4.3.0`; GCC: `16.2.1 20261029 (Red Hat 16.2.1-3)`.
- Release, Unix Makefiles, `BUILD_TESTING=OFF`, `CMAKE_EXPORT_COMPILE_COMMANDS=ON`.
- The local VM instructions specify four CPUs, approximately 8 GiB RAM, and
  two build jobs to avoid starving the graphical session; build used
  `--parallel 2`.

The source archive and build tree are on the guest under
`/tmp/T4k-fedora-20261116/`. Configure command:

```sh
cmake -S /tmp/T4k-fedora-20261116/src \
  -B /tmp/T4k-fedora-20261116/build -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/tmp/T4k-fedora-20261116/prefix \
  -DBUILD_TESTING=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

Build command: `cmake --build /tmp/T4k-fedora-20261116/build --parallel 2`.
Install command: `cmake --install /tmp/T4k-fedora-20261116/build`.

The configure, application build, and `cmake --install
/tmp/T4k-fedora-20261116/build` all exited 0. The build completed at 100%,
with zero `warning:` or `error:` lines. `install_manifest.txt` has 700 entries
(699 newline characters because its final entry has no trailing newline) and
SHA-256 `5c91bfd3e032d69930f71c09720bae24146323f1fa30f17d6b178313d6e13ff9`.
The installed prefix is 14 MiB.

Build-tree executable SHA-256 is
`11e1c689f1fa562565f034acfaefb3592ee5e8d795adea4fcce6f2f4633a7c64`;
installed `/tmp/T4k-fedora-20261116/prefix/bin/latte-dock-ng` SHA-256 is
`a86461b57ea69b4c2be145f4612a483e25fac329c11db6b3f978e44aaa675d60`.
The files have the same size and ELF BuildID; CMake's install step rewrote the
RUNPATH from the build-tree placeholder to `/tmp/T4k-fedora-20261116/prefix/lib64`.
This is expected install-time RPATH rewriting, not a post-handoff mutation.

The scratch prefix is also the configured install prefix, so the installed
artifact paths match that build. No `DESTDIR`, user prefix, system prefix,
desktop configuration or live Dock process was changed.

For P4, the worker can use guest prefix
`/tmp/T4k-fedora-20261116/prefix` and build tree
`/tmp/T4k-fedora-20261116/build`. The complete compressed build log,
configure/install logs and install manifest are retained beside this README.

## Metadata install verification

For each root below, all four nonempty typeinfo files and matching plugin
libraries exist. Each `qmldir` contains the matching `typeinfo` and `plugin`
entries. The check exited 0 with `VERIFIED_4_MODULES_IN_2_ROOTS`.

- `/tmp/T4k-fedora-20261116/prefix/lib64/qml`
- `/tmp/T4k-fedora-20261116/prefix/lib64/qt6/qml`

## CMake 4.5 package attempt

The official CMake download page lists CMake 4.5.0 and the Linux x86_64
tarball; the official API reference confirms that `BUILD_DEPENDS` was added
in 4.5. Fedora's system package is 4.3.0. The host's Python has no pip, and
official PyPI metadata has no 4.5.0 wheel (latest is 4.4.5).

- Command: `curl --fail --location --silent --show-error
  https://github.com/Kitware/CMake/releases/download/v4.5.0/cmake-4.5.0-linux-x86_64.tar.gz
  --output /tmp/T4k-cmake45/cmake-4.5.0-linux-x86_64.tar.gz`. It downloaded
  about 2.5 MiB, then made no progress for about 60 seconds. The transfer was
  interrupted (exit 130); the partial archive was not used.
- Command: `curl --fail --head --location --silent --show-error
  --connect-timeout 10 --max-time 15
  https://cmake.org/files/v4.5/cmake-4.5.0-linux-x86_64.tar.gz`. The HEAD
  request to Kitware's official cmake.org release path succeeded and
  reported a 67,002,501-byte file. The bounded download reached about 1.4 MiB
  without useful progress and was interrupted (exit 130). No archive was
  obtained from this cmake.org transfer.
- Download command: `curl --fail --location --silent --show-error
  --connect-timeout 10 --max-time 120
  https://cmake.org/files/v4.5/cmake-4.5.0-linux-x86_64.tar.gz
  --output /tmp/T4k-cmake45/cmake-4.5.0-linux-x86_64.tar.gz`.
- The host's Python is missing `pip`, so an isolated PyPI install could not be
  attempted (`python3 -m pip download ...` exited 1: `No module named pip`).
- A Python standard-library request to
  `https://pypi.org/pypi/cmake/4.5.0/json` returned HTTP 404. The official
  project metadata at `https://pypi.org/pypi/cmake/json` reported latest
  version `4.4.5` and zero files for `4.5.0`. There is no official 4.5.0 wheel
  filename to match against a China-hosted PyPI mirror, so no wheel download or
  extraction was attempted.

A later bounded request through SourceForge's `cmake.mirror` succeeded. Its
[project page](https://sourceforge.net/projects/cmake.mirror/files/v4.5.0/)
identifies it as an exact CMake project mirror. The local file
`/tmp/T4k-cmake45/cmake-4.5.0-linux-x86_64.tar.gz` is 67,002,501 bytes and its
SHA-256 `81f0d5b0960fb6adc8197030ffc827fba937640d51e73e39e0e9e40d88c66f6f`
matches the official [Kitware/CMake v4.5.0 release digest](https://github.com/Kitware/CMake/releases/tag/v4.5.0).
The extracted binary SHA-256 is
`04a55a063e59825a56a96ce0ffc74b41b5e6b0f086676b398ddfd947791371de`; it
reports `cmake version 4.5.0`. The download completed on the development host
at 2026-11-17 07:23:20 CST and extraction at 07:23:34 CST. Tsinghua and USTC
GitHub-release mirror paths both returned 404:
`https://mirrors.tuna.tsinghua.edu.cn/github-release/Kitware/CMake/v4.5.0/cmake-4.5.0-linux-x86_64.tar.gz`
and
`https://mirrors.ustc.edu.cn/github-release/Kitware/CMake/v4.5.0/cmake-4.5.0-linux-x86_64.tar.gz`.
The successful SourceForge command was `curl --fail --location --silent
--show-error --connect-timeout 8 --max-time 120
https://downloads.sourceforge.net/project/cmake.mirror/v4.5.0/cmake-4.5.0-linux-x86_64.tar.gz
--output /tmp/T4k-cmake45/cmake-4.5.0-linux-x86_64.tar.gz`; the local archive
was created at 07:22:29.870 CST and fully written at 07:23:20.617 CST.

The verified package is still only in development-host `/tmp`; it has not
been copied to Fedora. The P4 worker owns the guest during its sampling window.
Wait for coordinator clearance before transferring the package or starting
any install-preparation build.

No version macro was forced and no install-preparation behavior is inferred
from the 4.3.0 Fedora results. T4k remains **Pending validation** until clean
CMake 4.5 `install_prep/all`, staged install, zero four author warnings, and
both-root checks run with the verified genuine CMake 4.5 binary.
