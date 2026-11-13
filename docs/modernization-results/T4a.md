# T4a: gate `misc-redundant-expression`

Status: local acceptance complete on the new user-commit baseline. Both full
production gates and the four compiler/configuration test suites pass. Changes
remain uncommitted; remote acceptance of the expanded gate is pending.

Date: 2026-11-13. Source baseline: `main` at
`7963444958bc8cfc4dc51a6788cb42e455c4a459`. The working tree also contains the
uncommitted T3 forty-six-check slice; this slice adds one check and does not edit
or remove any T3 fixture.

## Scope

Added `misc-redundant-expression` to `.clang-tidy` and the alphabetically ordered
`GATED_CHECKS` tuple, with the required negative fixture
`autotests/clang-tidy/misc-redundant-expression.cpp`. The `.clang-tidy` list's
first comment remains in place. The check was selected because R8c recorded the
duplicate `bright2 <= 255` bound in `BackgroundCache::areaIsBusy()`; R8c-fix
corrected it to test `bright1 <= 255` and added a behavioral regression test.

The fixture mirrors that specific failure shape: its `inBounds` expression
checks `bright2 <= 255` twice and omits the upper bound for `bright1`. This is a
negative check fixture, so it intentionally retains the defect pattern without
being production code.

## Fixture proof

Used the production Clang Debug compile command for `app/lattecorona.cpp` from
`build/modernization/clang-debug/compile_commands.json`, selected by the
existing clazy source-selection helper. The existing `prove_fixtures()` runner
used the production flags and unchanged `WarningsAsErrors: '*'` configuration;
the run verifies that every configured fixture is rejected by its own check.
No runner logic or warning suppression changed.

| Tool | Version | Result |
|---|---|---|
| Host clang-tidy | 22.1.8 | 47 negative fixtures rejected by their own checks |
| Isolated clang-tidy | 18.1.3 | 47 negative fixtures rejected by their own checks |

Each proof used four jobs and completed successfully. The isolated LLVM 18
wrapper uses the existing production compile database and GCC 13 C++ headers;
it provides tool-version coverage, not CI parity.

## Initial worker handoff (superseded by coordinator verification below)

The full production-source gate is intentionally pending because the production
tree is still being rebuilt with newer user commits. Once the source/build
freeze is signaled, run the existing clang-tidy runner with both tool versions
and record source count, findings, skipped sources and timings here. This result
does not claim that the new check is clean across production sources. No
production source, build input, dependency, package definition or workflow was
changed in this slice.

## Current-HEAD coordinator validation

The coordinator reconciled user commits `7b3e038c8`, `2e7302712`, `6fe63197d`
and `796344495`: appearance controls, vertical Modern gaps, whole-Dock preset
restore and locale completion are retained as the new source basis. They do
not provide P5 effect-cost measurements or close A4's full mapping-chain cases.
T3's earlier observations are superseded for this source revision by the
following 47-check runs, which include every T3 check.

| Local tool | Exit | Fixture proof | Production sources | Findings | Wall seconds |
| --- | ---: | ---: | ---: | ---: | ---: |
| clang-tidy 22.1.8 | 0 | 47/47 | 156 | 0 | 71.147 |
| Isolated clang-tidy 18.1.3 | 0 | 47/47 | 156 | 0 | 98.460 |

Both ran `python3 scripts/test-clang-tidy.py --build-dir
build/modernization/clang-debug --jobs 4`, with the appropriate tool first in
PATH. Both skipped 58 autotest sources, 139 duplicate commands and 89 generated
sources. Evidence: [timings](T4a-data/timings.json),
[LLVM 22](T4a-data/clang-tidy22-final.txt) and
[LLVM 18](T4a-data/clang-tidy18-final.txt). The coordinator reviewed the actual
configuration diff and duplicate-bound fixture; failure handling is unchanged.

For each of `gcc-debug`, `clang-debug`, `gcc-release` and `clang-release`, the
coordinator ran configure, build (`--parallel 8`) and full CTest sequentially
in a private D-Bus session with offscreen Qt and isolated XDG test paths.
Every stage exited 0, every build had zero compiler warnings and each suite
passed 64/64 targets without skips or registration removals.
[Matrix](T4a-data/validation-summary.json),
[module/test summary](T4a-data/module-test-summary.json) and
[full logs](T4a-data/build-test-logs.tar.gz) record results and commands.
Syntax lint passed all 243 tracked QML files; generated-module checks passed
in all four trees. The private.app host smoke passed within each CTest suite.

The local deep-lint run is not a pass: host qmllint 6.11.3 differs from the
reviewed 6.11.1 fingerprint. Current commit CI independently failed its
coverage comparison against the old 241-file baseline. The reviewed 243-file
refresh and source-boundary audit are recorded in the
[QML baseline review](QML-baseline-2026-11-13.md); no fingerprint check was
disabled. Nineteen parser tests pass after that data refresh. New remote CI
after publication is still required.

No production behavior, build input, dependency, package or workflow was
changed by T4a. No desktop restart was needed for these analysis/data changes;
old A4 desktop feedback is not claimed as validation of newly added UI
features. The user commits' behavior remains intact.
