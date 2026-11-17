# T4k CMake 4.5 Clang host alternative

Fedora 45 has no `clang` or `clang++` executable available. To verify the
second compiler with the same genuine CMake 4.5.0 binary, a separate clean
Release tree was built on the development host with Clang 22.1.8. This tree
does not reuse or overwrite either the Fedora GCC validation directory or the
P4 artifact.

## Identity and toolchain

- Source snapshot: base HEAD `0a6974f5cd1b13116764a2320e3d882674f5fed3` plus
  only the four T4k module CMake files; archive SHA-256
  `92607311df689f379ea27a56dcfcd9d7180db9131b80d2a97a797dc8c3665fc9`.
- CMake: `/tmp/T4k-cmake45/cmake-4.5.0-linux-x86_64/bin/cmake`, version
  `4.5.0`, SHA-256
  `04a55a063e59825a56a96ce0ffc74b41b5e6b0f086676b398ddfd947791371de`.
- C and C++ compilers: Clang `22.1.8` (`clang`, `clang++`).
- Host: 16 CPUs, 31 GiB memory; build used eight jobs.
- Build tree: `/tmp/T4k-clang45-host/build`.
- Scratch install prefix: `/tmp/T4k-clang45-host/prefix`.

Configure command:

```sh
/tmp/T4k-cmake45/cmake-4.5.0-linux-x86_64/bin/cmake \
  -S /tmp/T4k-clang45-host/src \
  -B /tmp/T4k-clang45-host/build -G "Unix Makefiles" \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/tmp/T4k-clang45-host/prefix \
  -DBUILD_TESTING=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

Configure exited 0 and emitted zero `CMake Warning` lines. It did emit the
environment status messages `qmlplugindump failed for org.kde.plasma.core`
and `Could NOT find org.kde.plasma.core-QMLModule`. This host lacked that
Plasma QML tooling module; those messages are not counted as compiler
diagnostics or evidence that the module is installed. The T4k metadata checks
below independently cover the four Latte modules.

The fresh Makefiles contained `install_prep/all` and `install_prep/lattedock`.
All four generated `.qmltypes` files and their plugin libraries were absent
before the clean preparation build:

```sh
/tmp/T4k-cmake45/cmake-4.5.0-linux-x86_64/bin/cmake \
  --build /tmp/T4k-clang45-host/build \
  --target install_prep/all --parallel 8
```

The target exited 0 and linked the application. There were zero compiler
`warning:` or `error:` lines in the full Clang build log. It contains one
separate Qt QML type-registration status warning:
`Warning: dialog.h:25: PlasmaQuick::Dialog is used as base type but cannot be found.`
This is not a compiler diagnostic; the configure log also records the missing
`org.kde.plasma.core-QMLModule` lookup noted above. The relevant host tooling
metadata is unavailable here, so do not count these messages as a successful
Plasma module check.

Staged install used the configured scratch prefix:

```sh
/tmp/T4k-cmake45/cmake-4.5.0-linux-x86_64/bin/cmake \
  --install /tmp/T4k-clang45-host/build
```

It exited 0. The [metadata verification output](metadata-verification.txt)
confirms all eight module/root combinations have nonempty typeinfo/plugin
files and matching `qmldir` references under:

- `/tmp/T4k-clang45-host/prefix/lib64/qml`
- `/tmp/T4k-clang45-host/prefix/lib64/qt6/qml`

[`configure.log.gz`](configure.log.gz), [`install-prep.log.gz`](install-prep.log.gz),
[`install.log`](install.log), [`install_manifest.txt`](install_manifest.txt),
and [`metadata-verification.txt`](metadata-verification.txt) retain the
configure, complete build, install, manifest and metadata check evidence.
Install-manifest SHA-256 is
`4958f5b0b4a683f471ebf3062b3f1a80a0cc973b629535dd79da0d0a2fc511d9`.
Build-tree executable SHA-256 is
`261204a1d940639775c9ece183e0ad0256fa1ef81872e50f35f69b7f6c4bf9a2`;
installed executable SHA-256 is
`6fda605f31049b33fac76598891cd2b5367669a6e49526b5f4b4b32a00be9949`.

No Release CTest run or desktop retest was performed. The host GCC and Clang
Debug CTest suites on CMake 4.3.4 each passed 64/64. This is a CMake dependency
and install-preparation check; it claims no runtime behavior or performance
improvement.
