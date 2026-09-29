# M1b: Run CI through the shared presets

Date: 2026-09-29

Branch: `codex/modernization-m0-baseline`

Starting revision: `043d0e91a0e2b824500929d7495c450f5a8d8123`

Scope: `.github/workflows/build.yml` and the progress ledger only. No distro
installation/package jobs, dependency versions, existing action pins, runtime
code, or QML lint policy changed.

## Implementation

The existing GCC/Clang build matrix now covers Debug and Release for each
compiler and maps all four jobs directly to `gcc-debug`, `gcc-release`,
`clang-debug` and `clang-release`. Each job logs CMake/compiler versions,
configures with `cmake --preset`, builds the application/plugins and
`latte-autotests` through the preset's fixed eight-job target list, and runs
`ctest --preset`. Deep QML lint remains in each compiled job and uses that
preset's build directory. The separate syntax lint and distro/package
verification jobs are unchanged. A failure uploads configure/build/test logs,
the CMake cache, compile commands, CTest failure log and deep-lint log for
seven days.

## Validation

- PyYAML parsed `.github/workflows/build.yml`. A mapping check verified the
  four matrix names/compiler pairs/preset names match the four configure,
  build and test presets, including eight build jobs and `noTestsAction: error`.
- `bash scripts/qmllint.sh`: passed for all 236 tracked QML files.
- `bash scripts/qmllint-deep.sh build/modernization/<preset>`: passed in all
  four build directories. Every run reported the M0 category counts: 5572
  unqualified, 1268 missing-property, 18 Quick.anchor-combinations, 6
  incompatible-type, 5 unused-imports, 4 unresolved-type, 4
  Quick.property-changes-parsed, 3 stale-property-read, 2 import and 2
  Quick.layout-positioning.
- M1a local equivalents configure and build all four matrix jobs; all four
  `ctest --preset` runs passed 43/43, and sorted registered test names matched
  the M0 fingerprint. Logs are retained under the ignored
  `build-modernization-baseline/logs/` directory; deep-lint logs are in the
  corresponding ignored `build/modernization/<preset>/` directories.
- `git diff --check`: passed. `actionlint` is unavailable; workflow syntax and
  required matrix/preset relationships were checked with PyYAML and a focused
  semantic assertion instead.

Remote GitHub Actions validation is **Pending validation**. Initial run
[36598722902](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/36598722902)
was triggered after the feature workflow was wired to `codex/**`. All four
preset build jobs stopped in Configure because GitHub Actions selected `sh`
for multiline steps containing Bash's `set -o pipefail`; this patch sets
`shell: bash` on Configure, Build and Test. The Debian 13 install verification
also exposed a mismatch between Plasma desktop 6.3.6 and Debian's actual
`libplasma-dev` 6.3.5-1; the package checks now distinguish the two. The
Debian 13 package-install job and other distro install/package jobs passed.
Rerun the affected matrix after this correction. M1a's CMake 3.20.6 Debian-stack
configure and build now pass; its one icon-dependent CTest failure is recorded
there.

## Handoff

Next action: push the focused CI/package-floor corrections, confirm the four
build jobs and Debian 13 stack check pass remotely, and use those jobs as the
M1b/M3b evidence. The user authorized automatic commits and pushes on this
branch only.
