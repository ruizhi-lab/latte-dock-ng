# Debian 13 Compatibility Validation

Status: Complete. Date: 2026-10-01. Source: commit `f7e5de55f` plus the
uncommitted preview lifetime fix that was subsequently committed as
`58912719a` (the transferred source includes that commit).

## Environment

Debian GNU/Linux 13 (trixie), Qt 6.8.2, KDE Frameworks 6.13.0, Plasma 6.3.5,
GCC 14.2.0 and Clang 19.1.7. CMake configured both compiler builds with strict
warnings enabled for 54 first-party targets.

## Results

- GCC built `latte-dock-ng`, `latte-dock-ng-preview` and `latte-autotests`.
  The `latteprivateappplugin` target is excluded from the ordinary host build
  and must be requested explicitly for its host smoke test. After building
  that target, all 46 CTest targets passed.
- Clang built the same targets and the explicitly excluded
  `latteprivateappplugin`; all 46 CTest targets passed.
- Neither build log contained compiler warning or error diagnostics.
- The Fedora production-component QML fixtures passed on Debian Qt Quick Test
  6.8.2: 7 passed, zero failed, covering ScrollArea direction/throttle,
  ComboBox selection/bounds, transparent background wheel, and wheel disable.

The first GCC CTest run had one infrastructure/setup failure because the
excluded host plugin had not been explicitly built. Its dedicated smoke test
and the complete suite passed after building it. No package installation or
user configuration change was needed on this VM.
