# T4h: make non-enum switch fallbacks explicit

Baseline: `d77eef90245a8571b05d489fbc10d6cd9f94998d` (T4g). A focused clang-tidy 22.1.8 inventory found eight `bugprone-switch-missing-default-case` diagnostics in six files. Every switch is over an `int`: model `role`/`column` arguments or `View::alignment()`, whose public accessor returns `int`.

The model switches now add `default: break;` and retain their existing fallback after each switch. Unknown Qt roles continue to produce the same invalid `QVariant` or base-model result; unsupported model columns and roles continue to return `false`. The model column enums and their existing bounds checks are unchanged.

The two geometry switches also use `default: break;`. `View::alignment()` exposes an integer, and persisted alignment values are cast to the enum without range validation. An unknown or opposite-axis value previously left the coordinate at its initialized zero; the explicit defaults keep that behavior. Mapping such values elsewhere would change available-screen geometry, so these defaults preserve the existing fallback.

The negative fixture uses a standard C++ `switch (int)` with explicit cases and a post-switch fallback. It was rejected by clang-tidy 18.1.3 and 22.1.8 under production compile flags with the expected named finding and no compiler error. Focused checks on all six affected translation units exited successfully with no remaining `bugprone-switch-missing-default-case` diagnostics on both versions. The Clang 18 run printed aggregate `warnings generated` progress summaries without diagnostic lines; the Clang 22 run had no such summaries. The check is registered in `.clang-tidy` and `GATED_CHECKS`; full gate and build/test matrix results follow.

## Qt 6.12 follow-up

The genuine Arch Qt 6.12 / Plasma 6.8 gate exposed one additional T4g
`performance-no-automatic-move` finding in `Environment::iconSourceForTheme()`.
Qt 6.12 provides `QVariant(QString &&)`; the local icon name was const and
prevented that conversion from moving. Removing only that const preserves the
returned QVariant type and value, the empty-name fallback, and the QML-facing
signature. Qt 6.11 retains its available const-reference overload; no dependency
floor changed. The nearby source comment was added after the native and VM
checks; it does not change the compiled statements. `coreunittest` exercises themed icon names and ordinary-source
fallback on both environments.

## Coordinator acceptance

GCC and Clang Debug/Release builds completed with zero compiler warnings and
each passed 64/64 tests. The 64 registered tests and all 53 previously gated
checks remain. LLVM 18.1.3 and 22.1.8 rejected all 54 fixtures and reported no
findings in 156 production sources; focused clazy 1.17 was clean on all six
switch translation units and the Environment source. The QML syntax gate
passed for 243 files; module metadata checks passed for all four builds. [Build/test results](T4h-data/validation-summary.json)
and [gate output](T4h-data/tidy18-final.log) record these results.

The one LLVM 18 diagnostic outside first-party production scope was rechecked
at `/usr/include/Plasma/plasma/applet.h:45`; it is the system Plasma SDK
declaration previously audited in T4f. The scope filter is unchanged.
[Scope evidence](T4h-data/tidy18-package-scope.json) records the finding. No dependency, QML interface, persisted value, or fallback behavior
changed. No Dock restart was performed; the relevant tests ran in isolated sessions.
Arch Plasma 6.8.2 / Qt 6.12.0 passed all 54 fixture checks and the full
156-source gate after the follow-up fix. The actual Qt 6.12 core unit test
passed the icon-name and plain-source cases. The offscreen run skipped the
desktop-only icon-theme watcher; that case subsequently passed in the isolated
Wayland session, with the VM environment warnings recorded in its log. No Dock
was installed or restarted. Four CMake 4.5 author warnings and 5,827 QML-tool
warning lines were observed in Arch; the latter were not compared to a same-SDK
baseline and are not counted as C++ compiler warnings. A clean CMake 4.5
`install_prep/all` and isolated staged install passed, with all four referenced
metadata files and plugins present in both install roots; the four author
warnings remain. See the [Arch evidence](T4h-data/arch/README.md) and
[clean install preparation](T4h-data/arch/cmake45/install-prep-qmltypes.md).
Remote CI is separate.

A separate branch-clone audit traced the private Views sorting-key helper to
state priorities 10000, 20000 and 30000 only. Its existing uneven prefix widths
can reverse order outside that range; a nearby comment records the current
caller constraint. No sorting behavior or out-of-range contract was changed.

The preceding T4g source `d77eef902` passed all 23 jobs in [run 43474161451](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/43474161451).
[Recorded acceptance](T4h-data/ci-d77eef902.json) covers the 53-check source, not this 54-check slice.
