# R2a: Deprecation Gate at the Supported Floors

Status: Complete.
Date: 2026-10-25. Branch: `main`. Implementation commit: `a087f3511`.

## Defect

`ecm_set_disabled_deprecation_versions()` was not used, so nothing stopped
Latte from calling a Qt or KF API that is deprecated at or below the supported
floors (Qt 6.6, KF 6.0). A deprecation warning can be suppressed, ignored or
filtered by a third-party header, and the call then survives until the API is
finally removed. The project would only find out when a distribution raised the
floor.

## Fix

`CMakeLists.txt` includes `ECMDeprecationSettings` and calls
`ecm_set_disabled_deprecation_versions(QT ${QT_MIN_VERSION} KF ${KF6_MIN_VERSION})`
before the first `add_subdirectory()`, so every target in the tree inherits the
definitions it adds:

| Definition | Effect |
| --- | --- |
| `QT_DISABLE_DEPRECATED_BEFORE=0x60600` | Qt derives `QT_DISABLE_DEPRECATED_UP_TO` from it and compiles out APIs deprecated in Qt 6.6 or earlier |
| `KF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x60000` | Every KF6 export header compiles out APIs deprecated in KF 6.0 or earlier |
| `QT_DEPRECATED_WARNINGS_SINCE=0x70000`, `KF_DEPRECATED_WARNINGS_SINCE=0x70000` | Deprecations newer than the floors still warn, and `LATTE_STRICT_WARNINGS` turns the warning into an error |

No call site had to change: the existing sources build against the gate on
both compilers, so the project does not use an API deprecated at or below the
floors.

## Evidence

Fresh preset trees on the review host: GCC 16.2.1, Clang 22.1.8, CMake 4.3.4,
Qt 6.11.2, KF and ECM 6.29.0.

| Check | Result |
| --- | --- |
| `gcc-debug`, `gcc-release`, `clang-debug`, `clang-release` (`LATTE_STRICT_WARNINGS=ON`, `-j8`) | All four built, 0 compiler warnings or errors |
| CTest in each of the four presets | 56/56 passed |
| Gate definitions in `compile_commands.json` | Present in 427 of 427 translation units in both Debug trees |
| Fixture calling `QVariant::type()` (deprecated in Qt 6.0) with the application's flags | Rejected by GCC and Clang: the member is compiled out |
| Fixture calling `KPasswordLineEdit::setRevealPasswordAvailable()` (deprecated in KF 6.0) | Rejected by GCC and Clang: the member is compiled out |
| Fixture using `operator""_qba` (deprecated in Qt 6.8, newer than the floor) | Still declared; fails with `-Werror=deprecated-declarations` on both compilers |
| CI run 40180566388 at `3023bddc3` | GCC and Clang Debug/Release jobs passed; the Debian 13 install job built Qt 6.8.2 / KF 6.13 with 10 strict targets and passed |
| Replaced APIs | None |

The first version of this report listed only the Debug builds, named the GCC
version as 6.x and left the Release and Debian 13 rows to CI without a run
reference. The table above replaces it.

## Next action

None for R2a. If a future floor bump hides an API that the project needs, add
the replacement and explain it beside that change, not by disabling the gate.
