# T4k genuine CMake 4.5 validation

Status: CMake 4.5 install-preparation and staged-install checks passed on
Fedora 45. This record is separate from the P4-owned Fedora baseline evidence
under `T4k-data/fedora/`.

## Tool and source identity

- Fedora guest: `fedora@192.168.122.128`, kernel
  `7.2.15-300.fc45.x86_64`.
- The guest system CMake remains 4.3.0. This check used a portable Kitware
  CMake 4.5.0 binary unpacked under `/tmp`; no system package or path changed.
- SourceForge's `cmake.mirror` is documented as an exact CMake project mirror.
  The Linux x86_64 release archive was 67,002,501 bytes; SHA-256
  `81f0d5b0960fb6adc8197030ffc827fba937640d51e73e39e0e9e40d88c66f6f`,
  matching Kitware's official [CMake 4.5.0 release digest](https://github.com/Kitware/CMake/releases/tag/v4.5.0).
  Its project page is [SourceForge CMake mirror](https://sourceforge.net/projects/cmake.mirror/files/v4.5.0/).
- Extracted binary SHA-256:
  `04a55a063e59825a56a96ce0ffc74b41b5e6b0f086676b398ddfd947791371de`;
  `--version` reports `cmake version 4.5.0`.
- The verified archive was downloaded on the development host to
  `/tmp/T4k-cmake45/` on 2026-11-17 07:22:29–07:23:20 CST, and extracted by
  07:23:35 CST. It was transferred to Fedora only after P4 released the guest.
- Tsinghua and USTC GitHub-release mirror paths returned 404. SourceForge was
  used as the exact CMake project mirror; the official SHA-256 check supplies
  the integrity verification.
- Source snapshot is base HEAD
  `0a6974f5cd1b13116764a2320e3d882674f5fed3` plus only the four T4k module
  `CMakeLists.txt` changes. Archive SHA-256:
  `92607311df689f379ea27a56dcfcd9d7180db9131b80d2a97a797dc8c3665fc9`.
- While P4 validation was running, repository HEAD advanced to
  `ebb736602c4f5f529a127784c29720f762c59007`, which records P4 documentation
  and evidence. `git diff 0a6974f5..HEAD -- app declarativeimports containment
  plasmoid` is empty, so the application and CMake source in the frozen T4k
  snapshot matches current `main` apart from the four T4k edits themselves.

The isolated Fedora tree is `/tmp/T4k-cmake45-validation/`:

- Source: `src/`
- CMake 4.5.0: `cmake-4.5.0-linux-x86_64/bin/cmake`
- Build: `build/`
- Configured install prefix: `prefix/`

## Commands and results

The build directory was fresh. Before the preparation build, each of the four
`.qmltypes` files and its corresponding plugin `.so` was absent. Configure:

```sh
/tmp/T4k-cmake45-validation/cmake-4.5.0-linux-x86_64/bin/cmake \
  -S /tmp/T4k-cmake45-validation/src \
  -B /tmp/T4k-cmake45-validation/build -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/tmp/T4k-cmake45-validation/prefix \
  -DBUILD_TESTING=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

CMake generated `install_prep/all` and `install_prep/lattedock` in the Unix
Makefiles. Configure exited 0 and emitted zero `CMake Warning` lines. The
clean preparation command was:

```sh
/tmp/T4k-cmake45-validation/cmake-4.5.0-linux-x86_64/bin/cmake \
  --build /tmp/T4k-cmake45-validation/build \
  --target install_prep/all --parallel 2
```

It exited 0 and linked `latte-dock-ng`. The complete log contains zero
`warning:` or `error:` lines. Two jobs follow the Fedora VM's documented
four-CPU, approximately 8 GiB memory limit to avoid starving its graphical
session. The build used GCC `16.2.1 20261029 (Red Hat 16.2.1-3)`; this Fedora
run did not build with Clang. Fedora lacks `clang`/`clang++`, so a separate
fresh host Release tree using Clang 22.1.8 and the same CMake 4.5.0 binary is
recorded in [`clang-host/README.md`](clang-host/README.md). That host emitted
one non-compiler `Warning:` during Qt QML type registration and two configure
status messages for its missing Plasma QML tooling module; the C++ compiler
warning/error count remained zero. Existing host GCC and Clang Debug builds on
CMake 4.3.4 both passed with zero source-located C++ warning/error diagnostics.

The staged install used the configured scratch prefix, with no `DESTDIR`:

```sh
/tmp/T4k-cmake45-validation/cmake-4.5.0-linux-x86_64/bin/cmake \
  --install /tmp/T4k-cmake45-validation/build
```

It exited 0. The 11 MiB prefix is under `/tmp`; no user/system prefix or P4
artifact was used. The [metadata verification output](metadata-verification.txt)
confirms all eight installed module/root combinations have nonempty typeinfo
and plugin files, with both `typeinfo` and `plugin` entries in their `qmldir`.
The verified roots are:

- `/tmp/T4k-cmake45-validation/prefix/lib64/qml`
- `/tmp/T4k-cmake45-validation/prefix/lib64/qt6/qml`

Captured evidence files: [`configure.log.gz`](configure.log.gz),
[`install-prep.log.gz`](install-prep.log.gz), [`install.log`](install.log),
and [`install_manifest.txt`](install_manifest.txt). Build-tree executable
SHA-256 is `a94ff63b34f6481da7abc10904f72ff261ed40e627f2f6ccee68cd88dc7b69c2`;
installed executable SHA-256 is
`7f9fc8e4678826a39832777e7723ef99ab6d698f355782f97ee1dc4c6a94cf50`. CMake
rewrites the executable's install RUNPATH as expected. The install manifest
SHA-256 is `8c0c0386e45ee669e905ded1e08d06f9f9b1af9b1dcd431de8fd0a5a0d3f2b9e`.

No runtime behavior or performance improvement is claimed. The coordinator
reviewed both compiler logs and independently verified all installed
module/root pairs. No desktop retest is applicable to this build/install
dependency change.
