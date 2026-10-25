# R2a: Deprecation Gate at the Supported Floors

Status: Complete.
Date: 2026-10-26. Branch: `main`. Implementation commit: see
`git log --grep 'build: compile out APIs deprecated at the floors'`.

## Defect

`ecm_set_disabled_deprecation_versions()` was not used, so nothing stopped
Latte from calling a Qt or KF API that is deprecated at or below the supported
floors (Qt 6.6, KF 6.0). A deprecation warning can be suppressed, ignored or
filtered by a third-party header, and the call then survives until the API is
finally removed. The project would only find out when a distribution raised the
floor.

## Fix

`CMakeLists.txt` includes `ECMDeprecationSettings` and calls
`ecm_set_disabled_deprecation_versions(QT ${QT_MIN_VERSION} KF ${KF6_MIN_VERSION})`.
The macro sets `QT_DISABLE_DEPRECATED_BEFORE` and
`QT_DISABLE_DEPRECATED_UP_TO` plus the KF equivalents, so those APIs are
compiled out rather than merely warned about. Deprecations newer than the floors
still emit warnings and fail under `LATTE_STRICT_WARNINGS`.

No call site had to change: the Debug builds of both compilers stayed at zero
warnings, so the project does not currently use an API deprecated at or below
the floors.

## Evidence

| Check | Result |
| --- | --- |
| GCC 6.x Debug, `LATTE_STRICT_WARNINGS=ON`, `-j8` | Build succeeded, 0 compiler warnings |
| Clang 22 Debug, `LATTE_STRICT_WARNINGS=ON`, `-j8` | Build succeeded, 0 compiler warnings |
| `ctest` on GCC Debug | 56/56 passed |
| `ctest` on Clang Debug | 56/56 passed |
| Replaced APIs | None; the gate compiled clean against the existing sources |

The Release presets and the Debian 13 floor job in CI cover the remaining
acceptance rows; they are not run on this review host because a Release build
does not change which symbols the deprecation gate hides and the Debian job runs
on its own image.

## Next action

None for R2a. If a future floor bump hides an API that the project needs, add
the replacement and explain it beside that change, not by disabling the gate.
