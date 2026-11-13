# R2c: clazy Inventory and Zero-Finding Gate

Status: Complete. CI run 40345972905 at `09b030e61` ran both clazy steps of the
Clang Debug job and passed. Fix batches [1](#fix-batch-1) and [2](#fix-batch-2)
are merged into `main` and passed CI run 40683601580 at `14d705a5d`, which ran
the 43-check gate ([CI validation](#ci-validation)). [Fix batch 3](#fix-batch-3)
fixes the last six checks; it is in `main` (`6b51b43a6`) and passed CI run
41452548625 (23 jobs, after one rerun of a Mageia mirror failure) and the host
desktop smoke, except for the Meta hold check, which fails on `v1.2.62` as well
([follow-ups](#follow-ups), item 10, which records its fix in `61963df10`). The
[follow-up fixes](#follow-up-fixes) (items 3, 5, 6, 7, 9 and 12) passed the local checks
and a host desktop check each: at `131c87dc8` for items 3, 5, 6, 7 and 9, and at `e839808c4`
for item 12 ([follow-up 12](#follow-up-12-apply-file-moves)).
Date: 2026-10-26. Branch: `claude/r2c-clazy-inventory-3a703c`. Start HEAD:
`fc925bfe9`. Implementation commit: `1feb770dc`.

## Defect

M4a gated two clang-tidy checks on one source file. clazy's Qt-specific checks
(container detaches, signal and slot signatures, string allocations, API misuse)
were never run on the tree, because clazy was missing on the review host and in
the CI image. Nothing stopped a change from adding a pattern that one of those
checks rejects.

## Result

- **Inventory.** clazy's level0 and level1 sets (52 checks) over all 153
  production sources found 570 distinct findings in 84 files, all from 19
  checks. The other 33 checks found nothing.
- **Gate.** `scripts/test-clazy.py` gates the zero-finding checks whose fixture
  fires under the project flags, in the Clang Debug CI job after the build: 30
  at the inventory, 31 since `R2c-auto-unexpected-qstringbuilder`, 37 since fix
  batch 1, 43 since fix batch 2 and 49 since fix batch 3. A gated check cannot
  be vacuous: the runner proves it on its own fixture first.
- **Not gated, zero findings.** `foreach` and `qstring-comparison-to-implicit-char`
  cannot fire under the project flags; no trigger was found for `qstring-left`.
- **Fix slices.** The 19 checks with findings became `R2c-<check>` slices.
  The first, [R2c-auto-unexpected-qstringbuilder](R2c-auto-unexpected-qstringbuilder.md),
  fixed a dangling `QStringBuilder` on Qt 6.6 and older. [Fix batch 1](#fix-batch-1)
  fixed six more checks (81 findings), [fix batch 2](#fix-batch-2) another six
  (130 findings) and [fix batch 3](#fix-batch-3) the last six (327 findings). The
  570 findings are therefore: 1 + 81 + 130 + 327, plus 31 loops that batch 2 fixed
  as a side effect. 558 are fixed in the code and 12 declarations are reviewed
  exceptions with a reason beside them (`// clazy:exclude=`). The
  [follow-ups](#follow-ups) are not findings.
- The inventory commit changed no production code, build rule or dependency of
  the product; the slice commits that follow it change one file each.

## Handoff

- **Batch / slice and selected scope:** R2c, a clazy inventory of production
  code and a CI gate for the checks with zero findings.
- **Status:** Complete; CI run 40345972905 passed.
- **Source baseline / start HEAD / implementation HEAD:** `fc925bfe9` /
  `fc925bfe9` / `1feb770dc`.
- **Branch / existing user edits preserved:** `claude/r2c-clazy-inventory-3a703c`
  in its own worktree; the tree was clean at the start and no other edits exist.
- **Dependencies and evidence links:** M4a ([report](M4a.md), the clang-tidy
  runner this one follows); R2b for the context-object rules behind
  `connect-3arg-lambda`.
- **Files and externally observable behavior changed:** new
  `scripts/test-clazy.py` and 30 fixtures in `autotests/clazy/` (one more per
  finished slice); two steps in
  the Clang Debug job of `.github/workflows/build.yml`; a "clazy gate" section
  in [the testing guide](../development-testing-guide.md). The product, its
  tests and its dependencies are unchanged. The Clang Debug job gains an apt
  install of `clazy` and the analysis (97 s with eight local jobs). Dependency
  check: only that job needs clazy; no CMake option, package definition,
  distribution Dockerfile or Nix file changes.
- **State authority / invalidation / lifetime / fallback constraints:**
  `GATED_CHECKS` is the only list of gated checks; clazy is asked which checks
  it enabled and the answer must equal the list, because clazy accepts an unknown
  check name silently. `compile_commands.json` is the authority for flags and for
  which sources exist. A failed parse, a timeout or a diagnostic the runner
  cannot read fails the run; none is treated as "no finding".
- **Toolchain, import environment and artifact identity:** see Environment;
  local image `latte-r2c-clazy:noble` (ID `8e9528e34f50`, not durable).
- **Commands executed with exit status:** see Evidence.
- **GCC/Clang Debug/Release warning/error results:** no C++ changed, so only
  the Clang Debug preset was rebuilt. Host (Clang 22.1.8) and container
  (Clang 18.1.3 with Qt 6.11.1) `clang-debug` builds: 0 compiler warnings, 0 errors
  (the build's qmllint step prints the known QML warnings, which the baseline
  gate tracks).
  GCC and Release builds were not rerun.
- **Tests registered / run / passed:** host `ctest --preset clang-debug`: 57
  registered, 57 run, 57 passed, 0 skipped, 0 failed. No test was added or
  changed. `packagingcontracttest`, `sourcecontracttest` and
  `releaseversiontest`, which read `build.yml` and the testing guide, pass.
- **Production behavior vs copied logic vs source-contract evidence:** the gate
  runs clazy on the production sources themselves. No logic is copied. The
  runner's failure paths are covered by the controlled mutations below, not by
  an automated test.
- **QML diagnostics added/removed:** none; no QML file changed.
- **Negative fixtures or controlled mutation evidence:** 30 fixtures at this
  commit, one per gated check, and five mutations of the tree or the runner (see Evidence).
- **Desktop user feedback / log review / clean-quit and coredump evidence:**
  not applicable, there is no runtime change.
- **Performance:** the gate takes 97 s with eight jobs on an idle 16-core host
  (92 s for the level0+level1 inventory); 154 s once, with the host busy. The
  analysis is CPU bound, so the CI runner needs longer: in CI run 40345972905
  `Install clazy` took 6 s and `Run gated clazy checks` 4 min 31 s.
- **CI, distribution and Nix checks:** the runner passes in a container that
  reproduces the Clang Debug job (below), and in CI run 40345972905 at
  `09b030e61`, where both clazy steps of `Build (Clang Debug)` and every other
  job passed. Distribution and Nix jobs are not affected.
- **Unresolved failures, review assumptions and unavailable checks:** see
  CI validation.
- **Next exact action:** none for the gate. The fix slices continue in batches;
  see [Fix batch 1](#fix-batch-1) and [Next actions](#next-actions).
- **Commit approval status / push approval status:** committed on the feature
  branch; merged into `main` in `8d1fe1568` and pushed with `09b030e61`.

## Environment

The container reproduces the Clang Debug job of `build.yml`: `ubuntu:24.04`,
the KDE Neon `user noble` repository and the job's apt package list, then
`apt-get install clazy`. The recipe is at the end of this report.

| Component | Version |
| --- | --- |
| OS | Ubuntu 24.04.5 LTS (container) |
| clazy | `clazy-standalone` 1.17 from package `clazy 1.17.1-0zneon+24.04+noble+release+build14` |
| clazy frontend | LLVM 19.1.1 (`clang-19 1:19.1.1-1ubuntu1~24.04.2`, pulled in by clazy) |
| Compiler that built the tree and the database | Ubuntu clang 18.1.3 (`1ubuntu1`), `CC=clang CXX=clang++` |
| Qt | 6.11.1 (`qt6-base-dev 6.11.1-0zneon+24.04+noble+release+build162`) |
| KDE Frameworks, Plasma, ECM | 6.31.0, 6.8.1, 6.31.0 |
| CMake, Python | 3.30.5 (Neon build), 3.12.3 |
| Host used for the final `ctest` | Clang 22.1.8, GCC 16.2.1, CMake 4.3.4, Qt 6.11.2, Python 3.14.7 |

apt picks clazy from the Neon repository, not from Ubuntu: the archive holds
`1.11-4ubuntu2` (universe), Neon holds 1.17 and 1.17.1 builds and apt installs the
highest version. The Neon build uses LLVM 19 while the tree is compiled with
clang 18, so clazy parses a database written by a different compiler. The
database flags were accepted without a frontend error. The CI job installs
whatever Neon offers when it runs; the runner prints the clazy version and
checks the enabled set, and the fixtures fail if a clazy update stops a check
from firing.

## What is analyzed

`compile_commands.json` of the `clang-debug` preset holds 429 entries. The runner
selects 153 sources and skips the rest:

| Skipped | Entries | Reason |
| --- | ---: | --- |
| Sources under `autotests/` | 55 | Test code. The production sources that test targets rebuild are covered through their production entry |
| Duplicate compile commands of a selected source | 134 | 133 are rebuilds of production sources by test targets (for example `app/data/*.cpp` with `-DQT_TESTLIB_LIB`), 1 is `app/declarativeimports/interfaces.cpp` in a second production target. Each source is analyzed once, with the flags of its production target |
| Generated sources in the build directory | 87 | 63 AUTOMOC `mocs_compilation.cpp`, 8 Wayland protocol glue, 6 rcc `qrc_*.cpp`, 4 QML type registrations, 4 QML plugin entry points, 1 D-Bus adaptor, 1 `configure_file()` copy of `app/settings/generic/generictools.cpp` (its original is analyzed) |
| Third-party code | 0 | Nothing is vendored: every source carries the project's SPDX headers and `compat/` holds forwarding headers and QML only |

`declarativeimports/core/extras.cpp` (one `#include`) is compiled only by a test
target, so it keeps that entry. The 153 selected sources equal the 153 tracked C
and C++ sources outside `autotests/` (60 files) and `cmake/` (3 configure-time
probes that are not in the database); `git ls-files` reports no source missing
and none extra.

Headers are analyzed through the sources that include them; only locations under
the source tree outside `build/` and `autotests/` count. The 18
`non-pod-global-static` findings in the generated copy of `contextmenudata.h`
are ignored (the original is counted). clazy prints a header under the path its
includer spelled, so one location arrives as
`app/layouts/../settings/settingsdialog/../../data/contextmenudata.h` and
`app/data/contextmenudata.h`. The runner normalizes the path and counts each
`(file, line, column, check)` once; the first inventory without that step
reported 1323 findings for the same 570 locations.

## Inventory

All 52 checks of clazy's level0 and level1 sets. "Locations" are distinct
`(file, line, column)` findings in production code. The gated checks are zero
and fire on their fixture.

| Check | Level | Locations | Files | Disposition |
| --- | ---: | ---: | ---: | --- |
| `range-loop-detach` | 1 | 305 | 47 | Slice `R2c-range-loop-detach` |
| `detaching-temporary` | 1 | 43 | 6 | Slice `R2c-detaching-temporary` |
| `container-anti-pattern` | 0 | 38 | 13 | Slice `R2c-container-anti-pattern` |
| `qfileinfo-exists` | 0 | 35 | 11 | Slice `R2c-qfileinfo-exists` |
| `non-pod-global-static` | 1 | 31 | 6 | Slice `R2c-non-pod-global-static` |
| `fully-qualified-moc-types` | 0 | 21 | 9 | Slice `R2c-fully-qualified-moc-types` |
| `range-loop-reference` | 1 | 15 | 6 | Slice `R2c-range-loop-reference` |
| `no-module-include` | 0 | 14 | 14 | Slice `R2c-no-module-include` |
| `unused-non-trivial-variable` | 0 | 14 | 10 | Slice `R2c-unused-non-trivial-variable` |
| `overloaded-signal` | 0 | 11 | 5 | Slice `R2c-overloaded-signal` |
| `connect-not-normalized` | 0 | 10 | 4 | Slice `R2c-connect-not-normalized` |
| `const-signal-or-slot` | 1 | 10 | 4 | Slice `R2c-const-signal-or-slot` |
| `connect-3arg-lambda` | 1 | 8 | 5 | Slice `R2c-connect-3arg-lambda` |
| `use-static-qregularexpression` | 0 | 6 | 2 | Slice `R2c-use-static-qregularexpression` |
| `incorrect-emit` | 1 | 4 | 3 | Slice `R2c-incorrect-emit` |
| `overridden-signal` | 1 | 2 | 2 | Slice `R2c-overridden-signal` |
| `auto-unexpected-qstringbuilder` | 1 | 1 | 1 | Fixed and gated: [R2c-auto-unexpected-qstringbuilder](R2c-auto-unexpected-qstringbuilder.md) |
| `empty-qstringliteral` | 0 | 1 | 1 | Slice `R2c-empty-qstringliteral` |
| `qproperty-without-notify` | 1 | 1 | 1 | Slice `R2c-qproperty-without-notify` |
| `foreach` | 1 | 0 | 0 | Zero, unreachable: `QT_NO_FOREACH` rejects the construct and clazy disables the check for Qt 5.9 and later |
| `qstring-comparison-to-implicit-char` | 0 | 0 | 0 | Zero, unreachable: `text == 5000` does not compile under the project flags with Qt 6.11 |
| `qstring-left` | 1 | 0 | 0 | Zero, no trigger found: `left(1)` on a parameter, a local, a temporary and in a comparison stayed silent |
| `child-event-qobject-cast` | 1 | 0 | 0 | Gated |
| `connect-by-name` | 0 | 0 | 0 | Gated |
| `connect-non-signal` | 0 | 0 | 0 | Gated |
| `install-event-filter` | 1 | 0 | 0 | Gated |
| `lambda-in-connect` | 0 | 0 | 0 | Gated |
| `lambda-unique-connection` | 0 | 0 | 0 | Gated |
| `lowercase-qml-type-name` | 0 | 0 | 0 | Gated |
| `mutable-container-key` | 0 | 0 | 0 | Gated |
| `post-event` | 1 | 0 | 0 | Gated |
| `qcolor-from-literal` | 0 | 0 | 0 | Gated |
| `qdatetime-utc` | 0 | 0 | 0 | Gated |
| `qdeleteall` | 1 | 0 | 0 | Gated |
| `qenums` | 0 | 0 | 0 | Gated |
| `qgetenv` | 0 | 0 | 0 | Gated |
| `qhash-namespace` | 1 | 0 | 0 | Gated |
| `qlatin1string-non-ascii` | 1 | 0 | 0 | Gated |
| `qmap-with-pointer-key` | 0 | 0 | 0 | Gated |
| `qstring-arg` | 0 | 0 | 0 | Gated |
| `qstring-insensitive-allocation` | 0 | 0 | 0 | Gated |
| `qt-macros` | 0 | 0 | 0 | Gated |
| `readlock-detaching` | 1 | 0 | 0 | Gated |
| `returning-data-from-temporary` | 1 | 0 | 0 | Gated |
| `rule-of-two-soft` | 1 | 0 | 0 | Gated |
| `skipped-base-method` | 1 | 0 | 0 | Gated |
| `strict-iterators` | 0 | 0 | 0 | Gated |
| `temporary-iterator` | 0 | 0 | 0 | Gated |
| `virtual-signal` | 1 | 0 | 0 | Gated |
| `writing-to-temporary` | 0 | 0 | 0 | Gated |
| `wrong-qevent-cast` | 0 | 0 | 0 | Gated |
| `wrong-qglobalstatic` | 0 | 0 | 0 | Gated |

### Findings per check and per file

Count after each path is the number of locations in that file (omitted for one).

#### `range-loop-detach`: 305 locations in 47 files

`app/layouts/storage.cpp` 64, `app/layout/genericlayout.cpp` 28, `app/layouts/synchronizer.cpp` 26, `app/layouts/importer.cpp` 16, `app/view/containmentinterface.cpp` 15, `app/lattecorona.cpp` 13, `app/wm/tracker/windowstracker.cpp` 13, `containment/plugin/layoutmanager.cpp` 11, `app/shortcuts/globalshortcuts.cpp` 9, `app/knscompat.cpp` 8, `app/declarativeimports/contextmenulayerquickitem.cpp` 7, `app/view/originalview.cpp` 7, `app/layouts/syncedlaunchers.cpp` 6, `app/main.cpp` 6, `app/layouts/appletids.cpp` 5, `app/layouts/manager.cpp` 5, `app/screenpool.cpp` 5, `app/settings/viewsdialog/viewscontroller.cpp` 4, `app/view/positioner.cpp` 4, `app/plasma/extended/backgroundcache.cpp` 3, `app/plasma/extended/screengeometries.cpp` 3, `app/settings/settingsdialog/delegates/activitiesdelegate.cpp` 3, `app/settings/settingsdialog/layoutscontroller.cpp` 3, `app/settings/universalsettings.cpp` 3, `app/view/clonedview.cpp` 3, `app/view/settings/subconfigview.cpp` 3, `app/view/view.cpp` 3, `app/wm/abstractwindowinterface.cpp` 3, `app/wm/plasmapaneltracker.cpp` 3, `app/plasma/extended/screenpool.cpp` 2, `app/shortcuts/modifiertracker.cpp` 2, `app/view/effects.cpp` 2, `app/wm/tasktools.cpp` 2, `containmentactions/contextmenu/menu.cpp` 2, `app/alternativeshelper.cpp`, `app/indicator/factory.cpp`, `app/plasma/extended/theme.cpp`, `app/settings/detailsdialog/schemesmodel.cpp`, `app/settings/settingsdialog/layoutsmodel.cpp`, `app/settings/settingsdialog/tablayoutshandler.cpp`, `app/shortcuts/shortcutstracker.cpp`, `app/tools/commontools.cpp`, `app/view/eventssink.cpp`, `app/view/indicator/indicatorresources.cpp`, `app/view/visualitemsearch.h`, `app/wm/waylandinterface.cpp`, `declarativeimports/core/iconitem.cpp`

#### `detaching-temporary`: 43 locations in 6 files

`containment/plugin/layoutmanager.cpp` 36, `app/layout/genericlayout.cpp` 2, `containmentactions/contextmenu/menu.cpp` 2, `app/lattecorona.cpp`, `app/plasma/extended/screengeometries.cpp`, `app/wm/abstractwindowinterface.cpp`

#### `container-anti-pattern`: 38 locations in 13 files

`app/wm/tracker/windowstracker.cpp` 9, `app/layout/genericlayout.cpp` 6, `app/plasma/extended/backgroundcache.cpp` 6, `app/settings/viewsdialog/viewscontroller.cpp` 3, `app/layouts/synchronizer.cpp` 2, `app/settings/settingsdialog/layoutscontroller.cpp` 2, `app/shortcuts/modifiertracker.cpp` 2, `app/view/containmentinterface.cpp` 2, `containmentactions/contextmenu/menu.cpp` 2, `app/layouts/importer.cpp`, `app/layouts/storage.cpp`, `app/plasma/extended/screenpool.cpp`, `app/settings/universalsettings.cpp`

#### `qfileinfo-exists`: 35 locations in 11 files

`app/wm/schemecolors.cpp` 10, `app/indicator/factory.cpp` 7, `app/layouts/importer.cpp` 4, `app/layouts/storage.cpp` 3, `app/plasma/extended/theme.cpp` 3, `app/tools/commontools.cpp` 3, `app/lattecorona.cpp`, `app/layout/abstractlayout.cpp`, `app/plasma/extended/backgroundcache.cpp`, `app/settings/settingsdialog/layoutscontroller.cpp`, `app/view/indicator/indicatorresources.cpp`

#### `non-pod-global-static`: 31 locations in 6 files

`app/data/contextmenudata.h` 18, `plasmoid/plugin/previewprocess.cpp` 7, `app/layouts/appletids.cpp` 2, `app/wm/plasmapaneltracker.cpp` 2, `app/data/viewstable.cpp`, `app/layout/abstractlayout.h`

#### `fully-qualified-moc-types`: 21 locations in 9 files

`app/wm/abstractwindowinterface.h` 6, `app/wm/tracker/windowstracker.h` 4, `app/wm/tracker/lastactivewindow.h` 3, `app/layouts/synchronizer.h` 2, `app/settings/settingsdialog/layoutscontroller.h` 2, `app/settings/viewsdialog/viewshandler.h`, `app/view/settings/primaryconfigview.h`, `app/view/view.h`, `app/wm/tracker/schemes.h`

#### `range-loop-reference`: 15 locations in 6 files

`app/layouts/synchronizer.cpp` 8, `app/view/effects.cpp` 2, `containmentactions/contextmenu/menu.cpp` 2, `app/layout/genericlayout.cpp`, `app/layouts/storage.cpp`, `app/settings/universalsettings.cpp`

#### `no-module-include`: 14 locations in 14 files

`app/declarativeimports/contextmenulayerquickitem.h`, `app/declarativeimports/interfaces.h`, `app/plasma/extended/backgroundtracker.h`, `app/plasma/extended/screengeometries.cpp`, `app/wm/abstractwindowinterface.cpp`, `containment/plugin/layoutmanager.h`, `containment/plugin/types.h`, `containmentactions/contextmenu/menu.cpp`, `declarativeimports/core/dialog.h`, `declarativeimports/core/environment.h`, `declarativeimports/core/quickwindowsystem.h`, `declarativeimports/core/tools.h`, `plasmoid/plugin/contextmenuactionsbackend.h`, `plasmoid/plugin/types.h`

#### `unused-non-trivial-variable`: 14 locations in 10 files

`containmentactions/contextmenu/menu.cpp` 3, `app/layouts/storage.cpp` 2, `app/settings/settingsdialog/layoutscontroller.cpp` 2, `app/layouts/synchronizer.cpp`, `app/plasma/extended/screenpool.cpp`, `app/settings/screensdialog/delegates/checkboxdelegate.cpp`, `app/settings/settingsdialog/layoutsmodel.cpp`, `app/view/containmentinterface.cpp`, `app/view/positioner.cpp`, `containment/plugin/layoutmanager.cpp`

#### `overloaded-signal`: 11 locations in 5 files

`app/view/view.h` 5, `app/settings/viewsdialog/viewsmodel.h` 2, `app/view/settings/primaryconfigview.h` 2, `app/settings/settingsdialog/layoutsmodel.h`, `declarativeimports/core/iconitem.h`

#### `connect-not-normalized`: 10 locations in 4 files

`app/view/containmentinterface.cpp` 4, `app/declarativeimports/contextmenulayerquickitem.cpp` 3, `app/view/view.cpp` 2, `containment/plugin/layoutmanager.cpp`

#### `const-signal-or-slot`: 10 locations in 4 files

`containment/plugin/layoutmanager.h` 7, `app/layout/genericlayout.h`, `app/plasma/extended/screengeometries.h`, `app/view/visibilitymanager.h`

#### `connect-3arg-lambda`: 8 locations in 5 files

`containmentactions/contextmenu/menu.cpp` 4, `app/lattecorona.cpp`, `app/layouts/manager.cpp`, `app/layouts/synchronizer.cpp`, `app/wm/waylandinterface.cpp`

#### `use-static-qregularexpression`: 6 locations in 2 files

`app/knscompat.cpp` 5, `app/wm/tasktools.cpp`

#### `incorrect-emit`: 4 locations in 3 files

`app/view/windowstracker/windowstracker.cpp` 2, `app/wm/tracker/trackedgeneralinfo.cpp`, `app/wm/waylandinterface.cpp`

#### `overridden-signal`: 2 locations in 2 files

`app/view/view.h`, `declarativeimports/core/iconitem.h`

#### `auto-unexpected-qstringbuilder`: 1 location in 1 file

`app/plasma/extended/backgroundcache.cpp`

#### `empty-qstringliteral`: 1 location in 1 file

`app/settings/actionsdialog/actionshandler.cpp`

#### `qproperty-without-notify`: 1 location in 1 file

`app/settings/universalsettings.h`

## Gate design

- **Explicit list.** `GATED_CHECKS` names 30 checks at the inventory and one
  more per finished slice. A level (`level0`, `level1`)
  would add checks silently when clazy is updated, and clazy ignores a misspelled
  name, so the runner lists the enabled checks with `--list-checks` and fails on
  any difference.
- **One fixture per check.** `autotests/clazy/<check>.cpp` holds the smallest
  violation that compiles under the project flags. The runner compiles each
  fixture with the flags of `app/lattecorona.cpp` (it sees the Qt modules the
  fixtures use and the project's `QT_NO_KEYWORDS`, `QT_NO_FOREACH`,
  `QT_NO_CONTEXTLESS_CONNECT` and `QT_STRICT_ITERATORS`), runs it with the whole
  gated list and requires the fixture's own check among the findings. The
  directory must hold exactly the gated checks. The fixtures run first, so a
  clean production result means the checks can fire.
- **Fails closed.** A finding in a gated check, a compiler error, a timeout, a
  nonzero exit and a clazy diagnostic the pattern cannot read all fail the run.
  `-Wno-error` keeps findings as warnings (the strict-warning flags in the
  database would turn them into errors) and `-w` drops compiler warnings, which
  `LATTE_STRICT_WARNINGS` owns. `CLAZY_*` environment variables are removed so a
  caller's shell cannot change what clazy reports.
- **Inventory mode.** `--inventory CHECKS [--report FILE]` counts findings of any
  checks, including levels, and never fails on findings. Each later slice uses
  it to confirm its check reached zero.
- **Why not all zero-finding checks.** Three have no live trigger here:
  `foreach` (rejected by `QT_NO_FOREACH`, and clazy disables it from Qt 5.9),
  `qstring-comparison-to-implicit-char` (`text == 5000` does not compile under
  the project flags with Qt 6.11) and `qstring-left` (`left(1)` on a parameter,
  a local, a temporary and inside a comparison stayed silent in clazy 1.17).
  They stay out until a fixture fires; `strict-iterators` stays in because its
  fixture fires although `QT_STRICT_ITERATORS` is defined.

## Evidence

The runner commands ran in the container from the repository root, with the
checkout mounted read-only and the build tree on a separate mount. The last two
rows ran on the host. Container build logs: 0 compiler warnings in configure and build.

| Command | Result |
| --- | --- |
| `docker build --network host --build-arg BASE_IMAGE=docker.m.daocloud.io/library/ubuntu:24.04 --build-arg UBUNTU_MIRROR=http://mirrors.ustc.edu.cn/ubuntu/ --tag latte-r2c-clazy:noble --file Dockerfile .` | Exit 0 after 5 min 39 s. A first attempt with `--progress=plain` exited 125: the legacy builder rejects the flag |
| `cmake --preset clang-debug -DVERSION=1.2.59` and `cmake --build --preset clang-debug` | Exit 0; configure 21 s, build 4 min 46 s; 63 first-party targets under the strict gate |
| `python3 scripts/test-clazy.py --build-dir build/modernization/clang-debug --jobs 8 --inventory level0,level1 --report FILE` | Exit 0 in 92 s; run twice, the findings are identical |
| `python3 scripts/test-clazy.py --build-dir build/modernization/clang-debug --jobs 8` | **Exit 0** in 97 s: 30 fixtures rejected, 153 sources with no finding from 30 checks |
| Same, with `qgetenv("X").isEmpty()` appended to `app/data/errordata.cpp` | **Exit 1** in 97 s: `app/data/errordata.cpp:39:12: [qgetenv]` |
| Same, with a syntax error appended to `app/data/errordata.cpp` | **Exit 1** in 96 s: "1 translation unit(s) could not be analyzed" |
| Same, with `autotests/clazy/qgetenv.cpp` replaced by a clean file | **Exit 1** in 8 s: "the gate did not reject its negative fixture(s): qgetenv.cpp" |
| Same, with `"no-such-check"` added to `GATED_CHECKS` | **Exit 1** immediately: "clazy rejected the check selection" (clazy itself exits 0 and prints "Invalid check") |
| Same, with `autotests/clazy` empty | **Exit 1** immediately: one fixture missing per gated check |
| Host `cmake --preset clang-debug`, `cmake --build --preset clang-debug`, `ctest --preset clang-debug` | Exit 0, 0 compiler warnings, 57/57 tests passed |
| `.github/workflows/build.yml` parsed with PyYAML | Valid; both clazy steps are in the `build` job after the build and the clang-tidy step, gated on `matrix.preset == 'clang-debug'` |

The mutations replaced one file inside the container (read-only bind mount);
the working tree was not edited. The runner has no automated test of its failure
paths; the table above is the evidence for them.

## CI validation

- CI run 40345972905 at `09b030e61` (2026-10-26): `Install clazy` and
  `Run gated clazy checks` in `Build (Clang Debug)` passed, in 6 s and
  4 min 31 s, and so did every other job.
- CI run [40683601580](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/40683601580)
  at `14d705a5d` (2026-10-28, push to `main`, first attempt, 19 minutes): all 23
  jobs passed. It is the first run that contains fix batches 1 and 2 and the
  43-check gate (43 names and 43 fixtures in `scripts/test-clazy.py` and
  `autotests/clazy/` at that commit). In `Build (Clang Debug)`, `Install clazy`
  took 4 s, `Run gated clazy checks` 2 min 49 s, `Test` 21 s and `Deep QML lint`
  8 s, and every step that ran passed (the sanitizer steps and the failure
  upload are skipped in that job). The gate printed clazy 1.17 on LLVM 19.1.1,
  "43 negative fixtures: each is rejected by its own gated check" and "153
  production sources: no findings from 43 gated clazy checks"; `ctest` passed 56
  of 56 (the host registers one more test, `appstreamtest`, which ECM adds when
  `appstreamcli` is installed). `Build (GCC Debug)`, `Build (GCC Release)`,
  `Build (Clang Release)`, `QML lint (qmllint)`, `NixOS release check`, the
  seven `Install verify` jobs and the seven `Package install verify` jobs (Arch,
  Debian 13, Debian testing, Fedora, Mageia, openSUSE, Ubuntu) passed too.
  `Install verify (debian13)` builds against the Debian 13 floor: CMake 3.31.6
  and Qt 6.8.2.
- The only annotations of that run are 23 warnings "Node.js 20 is deprecated"
  for `actions/checkout`, `upload-artifact` and `download-artifact`; commit
  `6b0ea7648` moves the actions to Node 24 (see
  [GitHub Actions versions](../release-workflow.md#github-actions-versions)).
  Its Build run 40699083721 passed all 23 jobs without any annotation. The
  `Automatic Release` run 40686088082 for `14d705a5d` found that `v1.2.59`
  already points at another commit and released nothing; run 40701918106 for
  `6b0ea7648` reached the same decision.
- CI run [41452548625](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/41452548625)
  at `6b51b43a6` (2026-11-02, push to `main`; fix batch 3 and the 49-check gate): all
  23 jobs passed, after a rerun of one. In attempt 1, 22 jobs passed and `Install
  verify (mageia)` failed: the Mageia Cauldron mirror (`mirrors.kernel.org`) answered
  404 for packages its metadata listed, and all three builds of
  `scripts/retry-mageia-mirror-build.sh` ended on a different missing package. The
  rerun of the failed job (attempt 2, 953 s) passed; `Bind validated packages to
  version and source` ran in attempt 2. The `Package install verify (mageia)` job
  needed one retry and passed in attempt 1. Attempt 1 carries one failure annotation
  and three retry warnings ("Mageia Cauldron package mirrors are temporarily out of
  sync"); attempt 2 has no annotation. `Build (Clang Debug)` (828 s, `Run gated clazy
  checks` 199 s) printed clazy 1.17 on LLVM 19.1.1, "49 negative fixtures: each is
  rejected by its own gated check" and "153 production sources: no findings from 49
  gated clazy checks"; `ctest` passed 57 of 57. `Build (GCC Debug)`, `Build (GCC
  Release)`, `Build (Clang Release)`, `QML lint (qmllint)`, `NixOS release check` and
  the seven `Install verify` and seven `Package install verify` jobs passed too. It is
  the third Cauldron mirror failure since 2026-10-28 (see [Follow-ups](#follow-ups),
  item 11). The `Automatic Release` runs 41455008114 (skipped, for the failed attempt) and
  41457574418 (success) released nothing: `v1.2.59` already points at another commit,
  and the latest release is still `v1.2.62`.
- The earlier run 40508173790 at `eba91a3e6` does not count for the gate: its
  four Build jobs stopped at the KDE Neon key download before compiling (see
  [Fix batch 1](#fix-batch-1)).
- The container matches the job's image, but package versions float: Neon's
  clazy and Qt move independently of this repository, so a later run can meet a
  check that changed meaning.
- Ubuntu's own `clazy 1.11` was not tried; the job does not select it.
- The runner needs Python 3.9 or later (`Path.is_relative_to`), like the
  clang-tidy runner needs 3.8 for `shlex.join`; CI uses 3.12.
- GCC jobs are unchanged and do not run clazy: it needs a Clang compilation
  database.

## Fix batch 1

Status: Complete. Every check on `main` and the desktop smoke passed. CI run
40508173790 at `eba91a3e6` passed 18 jobs, among them every distribution install
and package check, the NixOS check and QML lint, but its four Build jobs, which
hold the clazy gate, failed before compiling: `wget` got an error response for
the KDE Neon signing key (the key URL answered 502 at the time and 200 later),
and the package binding job was skipped. CI run 40683601580 at `14d705a5d`, the
next push, passed all 23 jobs ([CI validation](#ci-validation)).

Two sessions implemented the batch of the
[implementation plan](../architecture-modernization-implementation.md#r2c-clazy-inventory-and-zero-finding-gate)
in parallel from `09b030e61`, on disjoint source files. Batch A
(`claude/r2c-batch-a-a5a9fa`) merged into `main` in `1dd485bc1` and batch B
(`claude/r2c-batch-b-c113e5`) in `fc8b803d0`. The only file both changed is
`scripts/test-clazy.py`, where each added names to `GATED_CHECKS`; git merged it
without a conflict. The gate now holds 37 checks with 37 fixtures.

| Slice | Locations / files | Batch | Commit |
| --- | ---: | --- | --- |
| [R2c-qfileinfo-exists](R2c-qfileinfo-exists.md) | 35 / 11 | A | `4a08e472e` |
| [R2c-range-loop-reference](R2c-range-loop-reference.md) | 15 / 6 | A | `e9bf4acf5` |
| [R2c-no-module-include](R2c-no-module-include.md) | 14 / 14 | A | `1c87da632` |
| [R2c-empty-qstringliteral](R2c-empty-qstringliteral.md) | 1 / 1 | B | `cd966836e` |
| [R2c-use-static-qregularexpression](R2c-use-static-qregularexpression.md) | 6 / 2 | B | `4bafbe6d9` |
| [R2c-connect-not-normalized](R2c-connect-not-normalized.md) | 10 / 4 | B | `a4cfaa80c` |

### Checks on `main` at `fc8b803d0`

| Command or environment | Result |
| --- | --- |
| Host `cmake --preset P`, `cmake --build --preset P`, `ctest --preset P` for `gcc-debug`, `clang-debug`, `gcc-release` and `clang-release` (GCC 16.2.1, Clang 22.1.8, CMake 4.3.4, Qt 6.11.2) | Exit 0 each; 0 compiler, linker or CMake warnings; 57/57 tests passed in each preset |
| `python3 -I autotests/installscriptbehavior.py` | Pass |
| `latte-r2c-clazy:noble` container (clazy 1.17, Qt 6.11.1): fresh Clang Debug build, then `test-clazy.py --build-dir build/modernization/clang-debug --jobs 8` | Exit 0 in 370 s including the build; 0 compiler warnings; 37 fixtures rejected by their own checks; no finding in 153 production sources |
| Same container, `--inventory` of the 12 checks that remain | 488 findings, each check at its inventory count |
| Debian 13 container from `docker/Dockerfile.debian` (local mirrors), `verify-install.sh debian13` as in the CI job: CMake 3.31.6, GCC 14.2.0, Qt 6.8.2, KF 6.13.0, libplasma 6.3.5, plasma-workspace 6.3.6 | Exit 0 in 219 s: system and user build, install and uninstall; 0 compiler warnings. This is the first build of the batch A include changes against a Qt older than 6.11 |
| Fedora 44 VM (Qt 6.11.2, KF 6.31.0, Plasma 6.7.5, GCC 16.2.1): `install.sh --user --jobs 4 Debug` from a `git archive` of `fc8b803d0` in `~/latte-r2c-src` | Exit 0; 0 compiler warnings |

The Debian container's uninstall steps printed 1388 "skipping manifest path
outside install prefix" lines. The read-only source mount held the host's
`build-user-ruizhi/install_manifest.txt`, and the uninstaller refused its paths
outside the container's prefix as designed; a CI checkout has no such
directory.

### Desktop smoke

The user-mode Debug build of `fc8b803d0` ran from the Plasma session
environment (`systemd-run --user`) on the Gentoo host (Plasma 6.7.5, Wayland,
two screens) and in the Fedora 44 VM (Wayland, `--log-file` with
`QT_LOGGING_RULES=latte*=true`). The Fedora dock configuration was backed up
first, with checksums, to `~/.local/share/latte-r2c-validation/` in the VM. The
user ran the checks; the log column is what the debug logs show.

| Check | Host | Host log | Fedora |
| --- | --- | --- | --- |
| 1. Startup: dock, widgets, launchers, system tray | Pass | Layout `我的布局` loaded its two docks on `DP-2` and `HDMI-A-1`; the layout has no system tray | Pass |
| 2. Dock context menu, normal and in Edit Dock mode | Pass | No entry | Pass |
| 3. Switch to another layout and back | Pass | Switch to `Default` at 20:38:53 and back at 20:39:00, views recreated | Not in the VM subset |
| 4. Actions editor (Configure Latte → Preferences → Context Menu → Actions...): separator rows without an icon | Pass | No entry | Not in the VM subset |
| 5. Remove a widget and press Undo; remove one for good | Pass | Widget 43 left the dock order at 20:39:08 and returned at 20:39:09 (Undo); widget 47 left it at 20:13:03 and was destroyed at 20:14:02, when the notification expired | Pass by the user's report; the VM log has no widget event |
| 6. Add widgets by dragging and without dragging | Pass | One widget created (`org.kde.plasma.konsoleprofiles`, id 47, at index 1); the log does not tell a drop from the default index | Pass by the user's report; the VM log has no widget event |
| 7. Colors, a color scheme switch, blur, hover zoom | Pass | The scheme change at 19:23:55 reapplied the palette | Pass |
| 8. Indicator switch on the Effects page | Pass | No entry | Not in the VM subset |
| 9. Launchers: icons, task merge, pin and unpin | Pass | No entry | Not in the VM subset |

- **Logs:** the host log has 994 lines (984 debug, 6 info, 4 startup lines
  without a prefix) and no warning, critical or fatal line; neither log has
  `No such method` or a `QObject::connect` warning. The Fedora log has 281
  lines (279 debug, 2 info) and no warning; its stdout holds six Mesa
  `failed to create dri2 screen` warnings, the VM graphics stack that
  [P0](P0.md) recorded.
- **Cores:** `coredumpctl` shows no `latte-dock-ng` core since the restarts
  (host 19:14:26, Fedora 19:11:49). Each VM has one `plasmashell` core at
  19:04:28 from a `plasmashell --version` probe over SSH without a display,
  before the dock restart; `verify-install.sh` disables core dumps for the same
  probe.
- **Paths seen running:** the install had removed the generated KNS
  compatibility overrides, so both docks regenerated them at startup with the
  five static patterns of `R2c-use-static-qregularexpression`. On both machines
  the templates `qmldir` lost the four directive lines of the system file and
  the legacy controls `qmldir` only its `prefer` line. The D-Bus screen geometry
  publication of `screengeometries.cpp`, one of the files whose includes
  changed, logged the available screen rectangles on both.
- **Not reached:** `serviceFromIdOrDesktopEntry()`, because both layouts use
  only `preferred://browser` and `preferred://filemanager` launchers and those
  take other branches; `layoutmanager.cpp:731`, the last fallback of the applet
  item lookup; a build on Qt 6.6 (the floor) or 6.7, since the oldest Qt built
  is 6.8.2.
- **Not run:** the Debian 13 VM, which has no build tools, so a build there needs
  packages installed as root.

## Fix batch 2

Status: Complete. Every check on `main` passed, the host desktop smoke passed except
for check 9, whose Meta reveal could not be reproduced later (see the table), and CI
run 40683601580 at `14d705a5d` passed all 23 jobs ([CI validation](#ci-validation)).

Two sessions implemented the batch of the
[implementation plan](../architecture-modernization-implementation.md#r2c-clazy-inventory-and-zero-finding-gate)
in parallel from `eba91a3e6`, on disjoint files. Batch C
(`claude/r2c-batch-c-bca59f`, 20 sources) merged into `main` in `181b7f871` and
batch D (`claude/r2c-batch-d-a6a9f7`, 13 headers and 3 sources) in `382e55cd2`.
Neither branch changed a file outside its list. Both added names next to each
other in `GATED_CHECKS`; the conflict was resolved by keeping the list sorted.
The gate now holds 43 checks with 43 fixtures.

| Slice | Locations / files | Batch | Commit |
| --- | ---: | --- | --- |
| [R2c-unused-non-trivial-variable](R2c-unused-non-trivial-variable.md) | 14 / 10 | C | `cec126a31` |
| [R2c-detaching-temporary](R2c-detaching-temporary.md) | 43 / 6 | C | `0ca056cc7` |
| [R2c-container-anti-pattern](R2c-container-anti-pattern.md) | 38 / 13 | C | `17248e996` |
| [R2c-const-signal-or-slot](R2c-const-signal-or-slot.md) | 10 / 4 | D | `a315abf65` |
| [R2c-incorrect-emit](R2c-incorrect-emit.md) | 4 / 3 | D | `ae9fe447b` |
| [R2c-fully-qualified-moc-types](R2c-fully-qualified-moc-types.md) | 21 / 9 | D | `a50614ff2` |

### Checks on `main` at `382e55cd2`

| Command or environment | Result |
| --- | --- |
| Host `cmake --preset P`, `cmake --build --preset P`, `ctest --preset P` for `gcc-debug`, `clang-debug`, `gcc-release` and `clang-release` (GCC 16.2.1, Clang 22.1.8, CMake 4.3.4, Qt 6.11.2) | Exit 0 each; 0 compiler, linker or CMake warnings; 57/57 tests passed in each preset |
| `python3 -I autotests/installscriptbehavior.py` | Pass |
| `latte-r2c-clazy:noble` container (clazy 1.17, Qt 6.11.1): incremental Clang Debug build, then `test-clazy.py --build-dir build/modernization/clang-debug --jobs 8` | Exit 0; 0 compiler warnings; 43 fixtures rejected by their own checks; no finding in 153 production sources |
| Same container, `--inventory` of the 6 checks that remain | 327 findings. `range-loop-detach` fell from 305 in 47 files to 274 in 45, because batch C rewrote 31 of its loops; the other five checks keep their inventory counts |
| Debian 13 container, `verify-install.sh debian13` as in the CI job: CMake 3.31.6, GCC 14.2.0, Qt 6.8.2, KF 6.13.0, libplasma 6.3.5, plasma-workspace 6.3.6 | Exit 0 in 210 s: system and user build, install and uninstall; 0 compiler warnings |

The Debian 13 VM still has no build tools, so the Qt 6.8 build ran in the
container, which uses the same Debian 13 packages.

### Desktop smoke

The user-mode Debug build of `382e55cd2` ran on the host from the Plasma session
environment (`systemd-run --user`, started at 08:40:06), with both docks of
layout `我的布局` in AutoHide mode. The Fedora VM was left to the R5 measurement.
The user ran the checks; the log column is what the debug log shows.

| Check | Result | Log |
| --- | --- | --- |
| 1. Startup: both docks, widgets and launchers | Pass | Containment 1 on `DP-2` and 13 on `HDMI-A-1` loaded |
| 2. Window tracking with "Dodge Maximized": maximize, restore, switch focus | Pass by the user's report | Window-tracker hint updates ran all day; no switch to DodgeMaximized appears. One dock changed to AlwaysVisible at 08:43:16 and back to AutoHide at 19:17:10, and the `Default` layout's dock ran DodgeActive from 19:21:55 to 19:23:22 |
| 3. A maximized window stops at an always-visible dock | Pass | AlwaysVisible from 08:43:16 to 19:17:10 |
| 4. Open and close the app launcher popup | Pass | No entry |
| 5. Drag a widget and back; add and remove a widget | Pass by the user's report | No widget or order event outside the two layout switches |
| 6. Context menu "Layouts" and "Add Dock" submenus | Pass | No entry |
| 7. Switch to another layout and back | Pass | To `Default` at 19:21:54, back at 19:23:22 |
| 8. Docks dialog and Screens dialog | Pass | Docks dialog opened at 19:30:50. The Screens dialog offers only screens that are disconnected and unused; both of the host's screens are connected and carry a dock, so none can be selected, as designed |
| 9. Hold Meta | Reported, not reproducible | The user reported that holding Meta showed the hidden dock and that it hid again after release. `ModifierTracker` cannot have caused it: on Wayland it drops `Qt::Key_Meta`, so its timer never starts. The same hold on 2026-11-02 showed nothing in the batch 3 build or in `v1.2.62` ([Fix batch 3](#fix-batch-3), [Follow-ups](#follow-ups) item 10), and what the user saw in this smoke is unexplained. No number badge can show: Meta+1–9 belong to Plasma's "Activate Task Manager Entry" shortcuts, and the log's badge list leaves slots 1–10 empty |
| 10. Wallpaper change (optional) | Not reported | — |

- **Logs:** 1328 lines (1282 debug, 6 info, 36 warnings), no `No such method`
  or `QObject::connect` warning and none from Latte's code. 35 warnings are Qt
  SVG messages at 19:17:36–37 ("Could not resolve property", for gradient ids
  an icon file references but does not define, as breeze's `rocs.svg` does).
  They are icon artwork, most likely drawn when the app launcher showed its icon
  grid. The other is the PulseAudio message about the webcam microphone, which
  other processes log at the same moments.
- **Cores:** `coredumpctl` shows no core since the restart at 08:40:06. The
  batch 1 build ran before it for 13 h 24 min with no core; its only two
  warnings were that PulseAudio message.
- **Not reached:** Justify alignment and its splitters, a vertical and a
  horizontal dock touching, cut and paste of docks between layouts in the Docks
  dialog, renaming an active layout, a layout import of the old linked format, a
  template export, multiple-layouts mode, and a build on Qt 6.6 or 6.7.
- **Not run:** the Fedora VM (left to R5); the Debian 13 VM (no build tools).

## Fix batch 3

Status: Complete. Every check on the branch `claude/r2c-batch3-integration` passed,
CI run 41452548625 at `6b51b43a6` passed all 23 jobs after one rerun of a mirror
failure ([CI validation](#ci-validation)), and the host desktop smoke passed except
check 8, hold Meta, which fails identically on `v1.2.62` and is not an effect of the
batch ([Follow-ups](#follow-ups), item 10).

Six sessions, all Claude Haiku 5.5 at medium effort, implemented the batch in two
waves. The main session fixed the file sets and the design decisions in the briefs
and reviewed every branch before merging it. Wave 1 started from `95e342f72` on
disjoint files: batch E (signals and the property), batch F (non-POD statics and
context-less timers) and batches G and H (the loop check in two file groups). Wave 2
started from the integration branch after E and F were merged, on the files those two
had to touch: batch I (`app/layouts/storage.cpp`) and batch J (12 files). The merge
commits on the integration branch are E `4e403af80`, F `3a180839b`, H `e62767af2`, G
`c0f64b9ee`, I `98b00c657`, J `b7fc199db`, and the gate commit is `8a30d3fa9`. `main`
was fast-forwarded to the records commit `6b51b43a6` (18 commits, `8bfacd80a..6b51b43a6`)
and pushed on 2026-11-02; that commit differs from `8a30d3fa9` only in five files
under `docs/`, so the checks below apply to the code that ran in the smoke.

| Slice | Locations / files | Batch | Commits |
| --- | ---: | --- | --- |
| [R2c-overloaded-signal](R2c-overloaded-signal.md) | 11 / 5 | E | `b30ae731a` |
| [R2c-overridden-signal](R2c-overridden-signal.md) | 2 / 2 | E | `67180cbda` |
| [R2c-qproperty-without-notify](R2c-qproperty-without-notify.md) | 1 / 1 | E | `b3ac2960c` |
| [R2c-connect-3arg-lambda](R2c-connect-3arg-lambda.md) | 8 / 5 | F | `085fbd190` |
| [R2c-non-pod-global-static](R2c-non-pod-global-static.md) | 31 / 6 | F | `50ba3074b` |
| [R2c-range-loop-detach](R2c-range-loop-detach.md) | 274 / 45 | G, H, I, J | `a18a905bc`, `d8b07b428`, `6f42e5ed2`, `8a998f1b8`, `4116888bb` |

What each slice did:

- **Signals (E).** Three internal model signals were renamed so they no longer share
  a name with a `QAbstractItemModel` signal: `Layouts::rowsInserted()` is
  `layoutsInserted()`, `Views::rowsInserted()` and `rowsRemoved()` are
  `viewsInserted()` and `viewsRemoved()`. The `IconItem::enabledChanged()` slot is
  `onEnabledChanged()`. Ten declarations stay and carry `// clazy:exclude=` with a
  reason: the NOTIFY signals of the QML-visible `width`, `height`, `visibility`, `x`
  and `y` of `View`, `x` and `y` of `PrimaryConfigView`, `smooth` of `IconItem`,
  `View::screenGeometryChanged` and the `UniversalSettings::screens` list. QML
  change handlers resolve those signals by name, so a rename would break them
  silently.
- **Timers and constants (F).** Seven `QTimer::singleShot` callbacks got a context
  object that owns their lifetime (`qGuiApp` for the process-wide quit, the info view,
  the Wayland surface, the context menu itself, and `QCoreApplication::instance()`
  for the two D-Bus calls that must outlive the menu). The eighth finding sat in
  `Synchronizer::pauseLayout()`, which nothing called and whose timers ran an empty
  lambda; it is deleted. The 31 namespace-scope `QString` and `QStringList`
  constants became function-local-static accessors, so none is built during static
  initialization.
- **Loops (G, H, I, J).** See [the umbrella report](R2c-range-loop-detach.md): 107
  loops iterate a `std::as_const` view, 164 loops over a by-value result hold it in
  a const local through a range-for init-statement, one loop uses `removeIf`, and two
  keep their live iteration under documented exceptions.

### Checks on the integration branch at `8a30d3fa9`

| Command or environment | Result |
| --- | --- |
| Host `cmake --preset P`, `cmake --build --preset P -j8`, `ctest --preset P` for `gcc-debug`, `clang-debug`, `gcc-release` and `clang-release` (GCC 16.2.1, Clang 22.1.8, CMake 4.3.4, Qt 6.11.2) | Exit 0 each; 0 compiler, linker or CMake diagnostics; 58/58 tests passed in each preset (CI registers one fewer, without `appstreamtest`) |
| `python3 -I autotests/installscriptbehavior.py` | Pass |
| `latte-r2c-clazy:noble` container (clazy 1.17, Qt 6.11.1): fresh Clang Debug build, then `test-clazy.py --build-dir build/modernization/clang-debug --jobs 12` | Exit 0 in 251 s including the build; 49 fixtures rejected by their own checks; no finding in 153 production sources |
| Same gate with the pre-fix `app/screenpool.cpp` and `app/shortcuts/globalshortcuts.cpp` mounted over the fixed files | Exit 1: 14 `range-loop-detach` findings, 5 and 9. A first attempt with the pre-fix `storage.cpp` failed to compile against batch F's accessors and proved nothing |
| Debian 13 container, `verify-install.sh debian13` as in the CI job: CMake 3.31.6, GCC 14.2.0, Qt 6.8.2, KF 6.13.0, libplasma 6.3.5, plasma-workspace 6.3.6 | Exit 0 in 179 s: system and user build, install and uninstall; 0 compiler warnings |

Before the merge the sessions ran the same container gate and the host GCC and Clang
Debug builds on their own branches; their reports hold the commands.

### Desktop smoke

The user-mode Debug build of `6b51b43a6` ran on the Gentoo host (Plasma 6.7.5,
Wayland, two screens) from the Plasma session environment (`systemd-run --user`,
started 21:27:33) with layout `我的布局`: one dock at the bottom edge of `DP-2`
(containment 1) and one of `HDMI-A-1` (containment 13). The user ran the checks; the
log column is what the debug log shows. Checks 8 and 10 stayed open in that session
(the user could not find the "Get new" entry, and Meta hold did nothing) and were run
again in a second session of the same build, started at 22:10:56 after a reinstall.

| Check | Result | Log |
| --- | --- | --- |
| 1. Startup: both docks, widgets and launchers | Pass | Containment 1 on `DP-2` and 13 on `HDMI-A-1` loaded at 21:27:33 and 21:27:35. `KnsCompat: setting up KNS dialog compatibility overrides (v 9)` and `done` follow at startup: the install had removed the generated overrides, so the changed loops of `ensureKnsCompat()` ran against the real system QML tree |
| 2. Dock context menu, normal and in Edit Dock mode | Pass | Edit mode opened the dock settings window (`canvasconfigview` and `primaryconfigview` at 21:39:08) and the widget explorer (21:39:24) |
| 3. Actions editor: separator rows, its default button, Cancel | Pass | No entry |
| 4. New layout: Apply becomes enabled, then Cancel | Pass | No entry |
| 5. Docks dialog: both docks listed, remove or cut one, Apply becomes enabled, Cancel | Pass | No entry |
| 6. Layout switch to another layout and back | Pass | `Default` at 21:40:10.969 and back to `我的布局` at 21:40:19.136. The two docks were deleted at 21:40:11 and the dock of `Default` (containment 1 on `DP-2`) created; that one was deleted at 21:40:19 and the two docks recreated |
| 7. Screens dialog (File menu or Ctrl+Alt+S) | Pass | No entry |
| 8. Hold Meta | No reveal, and the same on `v1.2.62` | See below |
| 9. Tasks: start an app from a launcher, merge, pin and unpin | Pass | No entry |
| 10. A "Get new" dialog | Pass in the second session | A KNS store dialog opened and closed at about 22:20 (the entry point was not recorded). 33 warnings, none from Latte; see below |
| 11. Export a layout (optional) | Pass | The template export dialog was cancelled at 21:40:06; backup exports at 21:45:51 and 21:46:08; a template export at 21:46:10 |
| 12. Quit Latte from the menu, last | Pass | `Corona::onAboutToQuit` at 21:46:32.841 and `Latte Corona - deleted...` at 21:46:33.003; all five docks created during the run were destroyed; no Fatal or ASSERT |

- **Check 8, hold Meta.** Nothing happened in the first session. `lattedockrc` held
  `metaPressAndHoldEnabled=false`, which disables the feature by itself
  (`TabPreferences::save()` writes `false` on every apply of the Preferences page, and
  at startup that value makes `ModifierTracker` ignore the Super keys). Setting it to
  `true` and restarting the dock changed nothing: holding Meta for about 1.5 s still did
  not show the hidden dock, and Plasma's own launcher opened on release (KWin's default
  Meta-only shortcut). The system dock `v1.2.62` with the same file showed nothing
  either, for a 3.8 s hold. A probe built on `KModifierKeyInfo` printed `Key_Meta`
  pressed at 22:32:23.625 and released at 22:32:27.407, so KWin delivers the key and
  `ModifierTracker` drops `Qt::Key_Meta`. `modifierIsTracked()` and `normalizeKey()` are
  identical in `v1.2.62`, `382e55cd2` and `6b51b43a6`. Batch 3 changed no line of that
  path, and the failure is not an effect of it. [Follow-ups](#follow-ups), item 10, has
  the details, the end-to-end check with a throwaway build that accepts `Key_Meta`, and
  the effect on the batch 2 record.
- **Logs:** the first session's log has 1427 lines (1385 debug, 6 info, 32 warnings and
  4 startup lines without a prefix), no critical or fatal line, no `No such method` and
  no `QObject::connect` warning. None of the 32 warnings comes from Latte's code: 24 are
  "No node found for item that was just removed" from Qt's file dialog during the
  exports (21:45–21:46), 6 are QML errors of the World Clock widget the user added
  (`org.kde.plasma.worldclock`, 21:39:28), one is the missing
  `plasma5support/dataengine/plasma_engine_geolocation` plugin and one is
  `QObject::disconnect: Unexpected nullptr parameter`, printed among Marble's map
  messages. The second session's log has 33 warnings, all from the store dialog at
  22:20:14–26: 20 "操作被取消" (network requests cancelled when the dialog closed), five
  `ActionToolBar.qml: Object or context destroyed during incubation` with five
  `Could not create delegate for ToolBarLayout`, one `TypeError: Cannot read property
  'width' of null` each in the Qt-embedded copies of Kirigami's `OverlayDrawer.qml:225`
  and `DrawerHandle.qml:118`, and one `QIODevice::read (QSslSocket): device not open`.
  They come from Kirigami and KNS QML (the embedded copy of Kirigami's templates is the
  conflict the KNS compatibility layer works around), not from the loops batch 3
  changed; no earlier log on this machine has a store dialog to compare with.
- **Cores:** `coredumpctl` shows no `latte-dock-ng` core after 21:00 on 2026-11-02.
  Both user-mode sessions quit with the teardown marker (`Latte Corona - deleted...`),
  and the stops of the system dock that followed left no core either.
- **Paths seen running:** the KNS compatibility setup at startup, `switchToLayout()`
  twice with the docks recreated (`Synchronizer` and the layout sync), the layout export
  paths and the shutdown timers.
- **Not reached:** the `removeIf` case in `OriginalView::syncClonesToScreens()` (it
  needs two secondary screens whose clones are created together); the info window
  (`Manager::showInfoWindow()`), which shows when layouts are activated in
  multiple-layouts mode; applying a new layout or a Docks dialog edit (checks 4 and 5
  ended in Cancel); importing a layout.

### Not run

A build on Qt 6.6 or 6.7 (the oldest Qt built is 6.8.2); the Fedora VM, which is left to
R5; the Debian 13 VM, which has no build tools.

## Follow-ups

None of these is a finding. They are decisions or defects the fix slices and their
validation found and deliberately left alone:

1. **Mutation during iteration in `GenericLayout::syncLatteViewsToScreens()`.** The
   loop over `m_latteViews` can reach `m_latteViews.take()` and the loop over
   `m_containments` can reach `m_containments.append()` from its body (paths in
   [the umbrella report](R2c-range-loop-detach.md#exceptions)). Both iterate the live
   container, as before. A fix needs a decision (snapshot, restart or index loop) and
   a test.
2. **`OriginalView::syncClonesToScreens()`.** The `removeIf` is a behavior change. The
   host smoke could not reach it (it needs a dock in "All secondary screens" mode and
   two secondary screens), so it is read from the code and not observed; if a user
   report shows a difference, see
   [the group G report](R2c-range-loop-detach-g.md#behavior-change-originalviewcpp225).
3. **`app/main.cpp` is invisible to `non-pod-global-static` (fixed, `6ee4ff685`).**
   clazy 1.17 skips a namespace-scope `static` without an initializer in a file named
   exactly `main.cpp`, so `s_userLocalQmlPaths` and `s_userLocalPluginPaths` were not
   reported. The two lists are now function-local statics behind accessors, and the
   source contract `mainCppKeepsQtContainerStaticsFunctionLocal` rejects a new
   namespace-scope Qt string, list, hash or map static in that file; the contract fails
   on the old `main.cpp`.
4. **`UniversalSettings::screens` has no NOTIFY.** A `screensCountChanged` signal
   exists; deciding whether the list property should notify on screen hotplug changes
   when QML bindings re-evaluate.
5. **`IconItem::setSmooth()` did not emit `smoothChanged()` (fixed, `c1879072f`)**, so
   bindings on `smooth`, such as `smooth: taskIconItem.smooth` in `TaskIcon.qml`, were
   not re-evaluated when it changed (found by batch E). The setter now emits after a real
   change; a `QSignalSpy` test and a QML binding test in `declarativecoreunittest` fail
   without the emit.
6. **Three getters nothing calls (removed, `2a905e805`)**, found by batch D:
   `GenericLayout::qmlFreeEdges`, `VisibilityManager::isValidMode` and
   `LayoutManager::isModernDockStyle`, with the helper only the last one used. A consumer
   trace (C++ callers and overrides, QML and JS, string-based calls, D-Bus XML, source
   contracts, documented third-party APIs) found none; `git grep` on the result finds no
   reference outside the historical records. `Indicator::isModernDockStyle` and the QML
   property of the same name are different things and stay.
7. **Variables with lost intent**, recorded by batch C in
   [R2c-unused-non-trivial-variable](R2c-unused-non-trivial-variable.md#lost-intent).
   Reviewed on 2026-11-03 by reading the code and the upstream history; the review ran
   nothing and reproduced nothing ([R2c-lost-intent-review](R2c-lost-intent-review.md)).
   Six of the eight places are harmless leftovers. The other places lead to four defects
   next to them and one decision. F1 to F4 are fixed ([Follow-up fixes](#follow-up-fixes)):
   F1, an out-of-range read of the activity table for an unknown activity id (`f03b319a3`:
   both `GenericTable::operator[](const QString &)` overloads return a default record for
   an unknown id, and the Activities column skips ids that have no row); and F2 to F4
   (`00aaf6ad5`): layout ids corrupted by a rename chain or swap in one Apply, a renamed
   locked layout that lost its lock, and synchronizer ids that stayed at their pre-rename
   paths. F5 is open: the Move To Layout menu no longer marks running layouts, and the
   review could not tell whether the removal in 2021 was intended. The choice is to
   restore the marking or to accept the menu as it is.
8. **Loops clazy does not report** and that were left alone are listed in the
   [umbrella report](R2c-range-loop-detach.md#left-alone-not-findings).
9. **Hardening of the runner (done, `43bf2317a`):** `clazyrunnertest` runs the real
   `scripts/test-clazy.py` in a temporary checkout against a fake `clazy-standalone`
   (22 cases: finding, unreadable diagnostic, compiler error, non-zero exit, hung unit,
   a clazy that ignores the check selection, an enabled-checks list that differs from
   `GATED_CHECKS`, an unrejected negative fixture, a missing clazy, a database that is not
   from a Clang build), and `knscompatunittest` has a case in which the system templates
   `qmldir` carries `linktarget`, `optional plugin`, `classname` and `prefer` lines that
   `ensureKnsCompat()` must drop while it keeps the module, typeinfo and type lines. Each
   new test fails under a mutation of the code it guards.
10. **Meta press-and-hold could not start on Wayland (fixed on the branch
    `claude/meta-hold-wayland`, see the last paragraph).** `ModifierTracker` listens to
    `KModifierKeyInfo::keyPressed`. On Wayland that class reports the Meta key as
    `Qt::Key_Meta` (kguiaddons 6.29.0, `KModifierKeyInfoProviderWayland`, over the
    `org_kde_kwin_keystate` protocol), but `modifierIsTracked()` accepts only
    `Key_Super_L`, `Key_Super_R`, `Key_Control`, `Key_Alt` and `Key_Shift`, and
    `normalizeKey()` maps only the two Super keys. The press is dropped before the 700 ms
    timer starts, so holding Meta never calls `showViews()`. On 2026-11-02 a probe built on
    `KModifierKeyInfo` printed `Key_Meta` pressed and released for a 3.8 s hold, and
    neither the batch 3 build nor the system dock `v1.2.62` showed the hidden dock, with
    `metaPressAndHoldEnabled=true`. The logic is the same in `v1.2.62`, `382e55cd2` and
    `6b51b43a6`. Three more facts: `TabPreferences::save()` writes
    `setMetaPressAndHoldEnabled(false)` ("Wayland-only build: keep this disabled") on every
    apply of the Preferences page, which has no control for it, while the runtime default
    is `true`; Plasma's own launcher opens when Meta is released (KWin's default Meta-only
    shortcut), so a working hold would end with two actions; and the batch 2 smoke reported
    a reveal that this code cannot have caused (the row is corrected above). Options, none
    done: accept `Qt::Key_Meta` in both functions and decide what to do about the launcher;
    remove the dead hold feature after tracing its consumers (rule 5 of `AGENTS.md`); or
    keep it and write the limit next to `ModifierTracker` and `TabPreferences::save()`. The
    loops that batch 3 changed in `GlobalShortcuts` can still be exercised on Wayland through
    the global shortcuts Meta+` ("Show Latte Dock", `showViews()`) and Meta+A
    (`showSettings()`), which the batch 3 smoke did not press.

    Checked end to end on 2026-11-02 with a throwaway build that was not committed:
    `normalizeKey()` also maps `Qt::Key_Meta` to `Key_Super_L`, and two `qDebug()` lines log
    each modifier event and the moment the hold fires. With `metaPressAndHoldEnabled=true`,
    nine holds each logged `Meta held, calling showViews()` 680 to 736 ms after the press,
    the four presses shorter than 700 ms (570, 228, 166 and 6 ms) logged no hold, and the
    user saw the hidden dock appear. So Plasma's launcher shortcut does not block the hold;
    it only acts when Meta is released. The reveal also logged 90 warnings from the badge
    QML, "QML MultiEffect: Cannot anchor to an item that isn't a parent or sibling": 72 at
    `declarativeimports/abilities/items/basicitem/ShortcutBadge.qml:56` and 18 at
    `containment/package/contents/ui/applet/ShortcutBadge.qml:72`. The shadow `MultiEffect`
    sits inside a `Loader` and is anchored to `taskNumber` or `appletNumber`, a sibling of the
    `Loader` and not of the effect. That code never ran on Wayland before, so the shadow
    behind a badge is probably misplaced or missing; anchoring the effect to its parent (the
    `Loader` already follows the badge text) is the obvious change, to be judged on screen.
    Any fix of the hold has to handle this too.

    Fixed on the branch `claude/meta-hold-wayland`: `normalizeKey()` maps `Qt::Key_Meta` to
    `Key_Super_L`; the Preferences page has its option "Press and Hold ⌘ to show shortcuts
    badges" again (the original string, so all 40 translations apply), off by default because
    of the launcher and no longer forced off by `TabPreferences::save()`; the setting is
    stored under the new key `metaPressHoldShowsBadges`, since the old `metaPressAndHoldEnabled`
    holds `true` from every save and `false` from every apply and records no choice; and both
    badge shadow effects anchor to their `Loader`. `ModifierTracker::handleKeyPressed()` is
    public so that `shortcutunittest` drives it: the two tests that need the mapping fail
    without it. A desktop check on 2026-11-03 passed: with the option off a hold did nothing;
    after enabling and applying it, holds showed the hidden dock and logged `Meta held`;
    applying another option left it enabled; the badge shadows looked right and the 90
    `MultiEffect` warnings were gone. Plasma's launcher still opens on release unless its
    Meta shortcut is changed in System Settings; the help guides say so.
11. **Mageia Cauldron mirror failures.** `Install verify (mageia)` failed on Cauldron mirror
    404s three times since 2026-10-28; run 41452548625 attempt 1 is the third. Each time
    the rerun of the failed job passed. `scripts/retry-mageia-mirror-build.sh` already
    repeats the Docker build three times, but every repeat starts a new `dnf install` that
    downloads all packages again and meets a different missing one. A hardening that is
    not done: a retry loop around `dnf install -y --refresh` inside the `RUN` of
    `docker/Dockerfile.mageia`, which should keep the packages already downloaded between
    tries (dnf keeps them in its cache until the transaction succeeds; not verified here,
    because the experiment in a `mageia:cauldron` container was interrupted).
    `autotests/packagingcontracttest.cpp` requires that Dockerfile to keep the text
    `dnf install -y --refresh` and to avoid `dnf upgrade`.
12. **`Layouts::save()` did not check its file moves (fixed in `6a8d104b4` and
    `e839808c4`).** Apply moved each renamed layout file into a `QTemporaryDir` and from
    there to `~/.config/latte`, ignored the result of both `QFile::rename()` calls and
    never asked `QTemporaryDir::isValid()`. The temporary directory was in `/tmp`, a tmpfs
    on the development host, and the layouts are on btrfs, so each move was a copy followed
    by a delete, and the copy fails when `/tmp` is full. The model ids were rewritten to the
    new paths whether or not the move had happened, so the model and the files could
    disagree, and a later Apply would then write a layout's properties into the file the
    model names. That was read from the code and fits one observation; it was not
    reproduced. During the desktop check of the follow-up fixes on 2026-11-03 an unrelated
    download had filled the 2 GB `/tmp`: the dock log, which is written there, stops inside
    the first Apply at 19:50:24, and `df` showed 0 bytes free at 19:53. The user renamed
    `LattePlayA` to `LattePlayA1`, `LattePlayB` to `LattePlayA` and `LattePlayA1` to
    `LattePlayB`, with one Apply for each, and afterwards the file `LattePlayB` had gained
    the unknown activity id of `LattePlayA`; `LattePlayA` was untouched and no names were
    swapped. The failing call was not captured. The same renames done as a chain in one
    Apply, with free space in `/tmp`, gave the right files. The three separate Applies were
    not repeated with free space on the pre-fix build, so the full `/tmp` is the likely
    cause, not a proven one. The fix moves the files through a directory next to the
    layouts, checks every move and keeps the layouts that could not move as they were; the
    same three separate Applies then gave the right files on the fixed build
    ([follow-up 12](#follow-up-12-apply-file-moves)).

## Follow-up fixes

Status: Complete. Follow-ups 3, 5, 6 and 9, and F1 to F4 of follow-up 7, are fixed on the
branch `claude/r2c-lost-intent-integration` (code at `131c87dc8`), and follow-up 12 on top
of it (`6a8d104b4` and `e839808c4`, merged in `3f0761964`). Each passed the checks below
and a host desktop check ([Desktop check](#desktop-check),
[Follow-up 12](#follow-up-12-apply-file-moves)). Follow-up 10 was fixed earlier
(`61963df10`) and is not part of this section.

Seven sessions, all Claude Haiku 5.5, implemented the fixes, each on its own branch: six
at medium effort from `61963df10`, and the one for follow-up 12 from `1cce2c0e8`, which the
app reports at low effort. The main session wrote the briefs and the file sets, read every
branch and merged them with `--no-ff` in three rounds: follow-ups 3, 5, 6 and 9 on the
branch `claude/r2c-followups-integration` (checked at `f9073543e`), then F1 and F2 to F4
on top of it on `claude/r2c-lost-intent-integration` (checked at `131c87dc8`), then
follow-up 12 on the same branch (checked at `e839808c4`).

| Follow-up | Change | Commit | Merge |
| --- | --- | --- | --- |
| 3. `app/main.cpp` statics | Function-local statics behind accessors; a source contract | `6ee4ff685` | `0971a4ba9` |
| 5. `IconItem::setSmooth()` | Emits `smoothChanged()`; two tests | `c1879072f` | `62ce69bf8` |
| 6. Three getters nothing calls | Removed after a consumer trace | `2a905e805` | `e0b6e6fe8` |
| 9. Runner hardening | `clazyrunnertest` (22 cases), one `knscompatunittest` case | `43bf2317a` | `f9073543e` |
| 7, F1. Unknown activity id | `GenericTable` lookup and `Activities::paint()` | `f03b319a3` | `2fdc4d259` |
| 7, F2 to F4. Apply after a rename | Row ids, lock and synchronizer table | `00aaf6ad5` | `131c87dc8` |
| 12. Apply file moves | Temporary directory next to the layouts, checked moves with rollback, failed layouts keep their id and name; the directory held by a `std::unique_ptr` | `6a8d104b4`, `e839808c4` | `3f0761964` |

What F1 and F2 to F4 changed (the other fixes are described in the follow-up items, and follow-up 12 below):

- **F1 (`f03b319a3`).** `GenericTable::operator[](const QString &)` found the row with its
  own loop and then indexed the list with -1 when the id was missing. Qt debug builds
  assert on that index and release builds read out of range. A layout file can name an
  activity this machine does not have (deleted while Latte was not running, or copied from
  another computer), and `Activities::paint()` looked up every assigned id without a
  check. Both overloads now use `indexOf()` and return a default record for an unknown id.
  The non-const overload must return a reference, so it returns one shared record that is
  reset on each unknown lookup: a write through it is discarded, a reference kept across a
  second unknown lookup sees the reset record, and `containsId()` is how a caller tells a
  missing id from a row. `paint()` skips ids that have no row. The result for an id that
  exists is unchanged. Tests: two `dataunittest` cases and one `sourcecontracttest`
  contract.
- **F2 to F4 (`00aaf6ad5`).** `Layouts::save()` maps the id each renamed layout had to its
  final file once (`newFileForOldId`), and the new `LayoutsTable::withRenamedIds()`
  rewrites every row from that one snapshot, so a chain or a swap cannot match a row twice
  (F2). The lock of a renamed layout goes to the file the layout has after the rename,
  through the new path-based helper `Latte::Layouts::LayoutFileLock`
  (`app/layouts/layoutfilelock.{h,cpp}`), which `Storage::lock()` and `unlock()` use as
  well (F3). The synchronizer receives `m_model->currentLayoutsData()` taken after the
  rename, `switchToLayoutInMultipleModeBasedOnActivities()` no longer writes an empty
  stored layout back into its table, and the two lookups of explicit layouts through
  `idForName()` (in `onActivityRemoved()` and in the loop over the layouts assigned to the
  current activity in `switchToLayoutInMultipleModeBasedOnActivities()`) skip names that
  are not in the table (F4). Tests: two `dataunittest` cases (a name chain and a swap), two
  `storagelogictest` cases (a lock at the new path; locking a missing file changes
  nothing) and four `sourcecontracttest` contracts (the Apply order and the synchronizer
  guards). Only the last two contracts cover F4, and a source contract does not run the
  code.

### Checks on the integration branch at `131c87dc8`

The first round was checked the same way at `f9073543e`; the table gives both results.

| Command or environment | Result |
| --- | --- |
| Host `cmake --preset P`, `cmake --build --preset P -j8`, `ctest --preset P` for `gcc-debug`, `clang-debug`, `gcc-release` and `clang-release` (GCC 16.2.1, Clang 22.1.8, CMake 4.3.4, Qt 6.11.2) | Exit 0 each at both commits; 0 compiler, linker or CMake diagnostics; 59/59 tests passed in each preset (CI registers one fewer, without `appstreamtest`) |
| `python3 -I autotests/installscriptbehavior.py` | Pass at both commits |
| `latte-r2c-clazy:noble` container (clazy 1.17, Qt 6.11.1): fresh Clang Debug build, then `test-clazy.py --build-dir build/modernization/clang-debug --jobs 4` | Exit 0 at both (852 s and 709 s including the build); 49 fixtures rejected by their own checks; no finding in 153 production sources at `f9073543e` and in 154 at `131c87dc8` (the new `layoutfilelock.cpp`) |
| Debian 13 container, `verify-install.sh debian13` as in the CI job: GCC 14.2.0, Qt 6.8.2, KF 6.13.0, libplasma 6.3.5 | Exit 0 at both (543 s and 531 s): system and user build, install and uninstall; 0 compiler warnings |

### Mutation checks

The main session ran one mutation per guarded behavior of F1 to F4 on `131c87dc8`
(gcc-debug build, 2026-11-03). Each mutation breaks one production file,
rebuilds the test target when the test compiles that file (the source contracts read the
sources when they run), runs the test and restores the file. Unmutated, `dataunittest` (47
cases), `storagelogictest` (24) and `sourcecontracttest` (225) pass, and they pass again
after the files are restored. The tests of follow-ups 3, 5 and 9 were mutated by their own
sessions; the results are in the items above.

| Mutation | Failing test |
| --- | --- |
| The non-const `GenericTable::operator[]` without the unknown-id branch | `DataUnitTest::activitiesTableLookupOfMissingIdReturnsEmptyActivity` (the Qt assertion stops the run) |
| The const `operator[]` without the unknown-id branch | The same test, the same assertion |
| `Activities::paint()` as before `f03b319a3` | `SourceContractTest::activitiesDelegatePaintSkipsUnknownActivityIds` |
| `LayoutsTable::withRenamedIds()` rewriting the live table one rename after the other | `layoutsTableRenamesSwappedNamesKeepingIdsUnique` in both key orders tried; `layoutsTableRenamesNameChainKeepingIdsUnique` in the descending order only |
| `LayoutFileLock::lock()` never locking | `StorageLogicTest::renamedLockedLayoutIsLockedAtItsNewPath` |
| `layoutscontroller.cpp` as before `00aaf6ad5` | `layoutsSaveRewritesRenamedIdsFromOneSnapshot`, `layoutsSaveLocksRenamedLayoutsAtTheirNewFile` and `layoutsSaveHandsSynchronizerTableAfterRename` |
| `synchronizer.cpp` as before `00aaf6ad5` | `synchronizerGuardsStoredLayoutLookups` |

### Desktop check

The user-mode Debug build of `131c87dc8` (`install.sh --user`, so Qt assertions are on) ran
on the Gentoo host (Plasma 6.7.5, Wayland, two screens) from the Plasma session
environment, with the layout `我的布局` and two throwaway layouts that the main session
created in `~/.config/latte/` and removed afterwards: `LattePlayA`, a copy of `Empty` with
the color green and an activity id this machine does not have
(`00000000-0000-4000-8000-00000000dead`), and `LattePlayB`, a copy of `Default` with the
color blue. The Layouts page does not show colors, so the main session read the layout
files on disk after the steps; the user ran the steps. The checksums of the three real
layouts were recorded before the run and are unchanged after it.

| Check | Result | Evidence |
| --- | --- | --- |
| 1. Startup: both docks, widgets, launchers, task icons | Pass | User report; the start log has no warning, no `No such method` and no core |
| 2. Task icons: hover zoom with smooth edges (follow-up 5) | Pass | User report. The fix is a change notification, so this shows no regression, not the fix itself |
| 3. Meta+A (cycle through the dock settings windows) | Not reachable | The user reports that Meta+A is Plasma's desktop-switching shortcut on this setup, so the check could not be done; the conflict was not examined further |
| 4. Preferences > Layouts opens with a layout that names an unknown activity (F1) | Pass in both runs | The page painted `LattePlayA` and the details dialog opened twice (19:49:20 and 19:49:26 in the first run); no assertion and no fatal line |
| 5. One Apply exchanges the names of `LattePlayA` and `LattePlayB` through a temporary name (F2) | Pass in the second run | One Apply at 20:05:19 moved both files through the temporary directory. Afterwards `LattePlayA.layout.latte` held the content of the former `LattePlayB` (blue, no activity, 1424 bytes) and `LattePlayB.layout.latte` that of the former `LattePlayA` (green, the unknown activity id, 289 bytes) |
| 6. A second Apply in the same session: the row now named `LattePlayB` is set read-only and renamed to `LattePlayC` (F3, and a check that the ids after step 5 are right) | Pass in the second run | Apply at 20:11:24: `LattePlayC.layout.latte` exists with mode 0444, green, with the unknown activity id; `LattePlayB.layout.latte` is gone; `LattePlayA.layout.latte` is unchanged (modified at 20:05:19) |

- **First run, full `/tmp`.** From 19:37 the user did checks 1, 2 and 4 (check 3 was not
  reachable) and then, because the Layouts page refuses two rows with the same name, a
  variant of step 5 with three single renames (`LattePlayA` to `LattePlayA1`, `LattePlayB`
  to `LattePlayA`, `LattePlayA1` to `LattePlayB`), one Apply each. `/tmp` was full by then:
  the dock log, written to `/tmp`, stops at 19:50:24 inside the first Apply. On disk
  `LattePlayA` was unchanged (modified at 19:37:11) and `LattePlayB` had been rewritten at
  19:50:49 and carried the unknown activity id of `LattePlayA`; nothing was exchanged. The
  three real layouts were unchanged, and `coredumpctl` shows no core.
  [Follow-up 12](#follow-ups) holds the explanation that fits this and what is not proven.
- **Second run, free `/tmp` (89 MB).** The test layouts were created again and steps 5 and
  6 were done once each, with the three renames of step 5 made in the table and a single
  Apply after them. The log has 485 lines, no warning and no `No such method`; the dock
  quit with the teardown marker `Latte Corona - deleted...`, no Fatal or ASSERT, and no
  core followed.
- **What the two runs show and what they do not.** The runs differ in more than the free
  space: the first applied three single renames one after the other, the second applied
  the chain in one Apply. With free space, the chain, the lock plus rename that followed
  it, and the unknown activity id all behave correctly. The three separate Applies were
  not repeated with free space on that build, so the full `/tmp` is the likely cause of the
  first run's result, not a proven one. They were run later on the build with the fix for
  follow-up 12, which no longer moves files through `/tmp`
  ([Follow-up 12](#follow-up-12-apply-file-moves)).
- **Not reached.** F4 (it needs the multiple-layouts mode and a D-Bus `switchToLayout`
  call to a renamed inactive layout) and Meta+A. None of F1 to F4 was reproduced on a
  desktop build before the fix, so the desktop check shows the right behavior of the fixed
  build, not the defect.

### Follow-up 12: Apply file moves

Code at `6a8d104b4` (the session's branch `claude/stoic-mirzakhani-83f0c1`, branched from
`1cce2c0e8`), merged on the integration branch in `3f0761964`. The main session changed one
thing in review, in `e839808c4`: `Layouts::save()` replaced its `QTemporaryDir` by
assignment when the first creation failed. That needs the move assignment of
`QTemporaryDir`, which exists in Qt 6.8.2 (the Debian 13 floor) and later (checked in the
headers of the Debian 13, Qt 6.10 and Qt 6.11 container images) but was not checked for the
declared minimum of Qt 6.6. The directory is now held by a `std::unique_ptr` and replaced
through `std::make_unique`, and the contracts that pin its code follow the new spelling.
The session's checks ran on `6a8d104b4`; the main session's ran on `e839808c4`.

- **Temporary directory.** `Layouts::save()` creates its `QTemporaryDir` as
  `<layouts directory>/.apply-XXXXXX`, so each move is a rename on one filesystem. The name
  is hidden, and the layout scanners list `QDir::Files` only, so the directory is never a
  layout. The default location is the fallback. Both creations are checked with `isValid()`;
  when neither works, Apply stops before it changes anything and says so. The directory is
  removed on every return from `save()`, except when a file could not be moved back: then it
  is kept, and the message names the file.
- **Checked moves.** `LayoutFileMove::moveFiles()` (`app/layouts/layoutfilemove.{h,cpp}`)
  parks each renamed file in the temporary directory and places it on its final path. A
  placement that fails undoes the placements of that round, returns the failed file to its
  old path and places the others again. Every rename refuses an existing target.
- **Layouts that did not move.** Their ids are left out of `newFileForOldId`, so the rows keep
  their ids. The row gets its old name back, the active layout object is not renamed, and the
  page shows the reason with `showInlineMessage()`. A new layout whose file stayed in `/tmp`
  keeps that path in its message. Read from the code, not tested: `applyData()` makes that row
  an original row, so the next Apply does not move it again; the file has to be moved by hand,
  or the layout created again.

Checks of the session on `6a8d104b4`:

| Command or environment | Result |
| --- | --- |
| Host `cmake --preset P`, `cmake --build --preset P -j8`, `ctest --preset P` for `gcc-debug`, `clang-debug`, `gcc-release` and `clang-release` (GCC 16.2.1, Clang 22.1.8, CMake 4.3.4, Qt 6.11.2; `TMPDIR` inside the ignored `build/tmp` of the worktree) | Exit 0 each; no compiler warning or error in the four build logs (the debug builds also print qmllint warnings for QML files this change does not touch); 59/59 tests passed in each preset |
| `latte-r2c-clazy:noble` (clazy 1.17, Ubuntu LLVM 19.1.1): fresh Clang Debug build in a new build tree, then `test-clazy.py --build-dir build/modernization/clang-debug --jobs 8` | Exit 0: 155 production sources (the new `layoutfilemove.cpp` included), no finding from the 49 gated checks, 49 negative fixtures each rejected by its own check. The run came before the last edit of one comment line in `layoutscontroller.cpp`; the gcc-debug rebuild and its 59 tests were repeated after that edit |
| `storagelogictest` (31 cases, 7 of them new) and `sourcecontracttest` (230 cases, 5 of them new) in `gcc-debug` | Pass |

A first run of the presets with `TMPDIR` under the home directory failed `layoutunittest`
(`abstractLayoutPersistsCustomSchemeFile`): the layout code writes paths below the home
directory as `~/...`. That came from the scratch directory, not from this change, and the
runs in the table use a `TMPDIR` outside the home directory.

Checks of the main session on `e839808c4`, the merged tree:

| Command or environment | Result |
| --- | --- |
| Host `cmake --preset P`, `cmake --build --preset P -j8`, `ctest --preset P` for `gcc-debug`, `clang-debug`, `gcc-release` and `clang-release` (the toolchain above; `TMPDIR` in the ignored `build/tmp` of the worktree) | Exit 0 each; 0 compiler, linker or CMake diagnostics; the changed sources were rebuilt in each preset; 59/59 tests passed in each |
| `python3 -I autotests/installscriptbehavior.py` | Pass |
| `latte-r2c-clazy:noble` container: fresh Clang Debug build in a new build tree, then `test-clazy.py --build-dir build/modernization/clang-debug --jobs 4` | Exit 0 in 688 s including the build: 49 fixtures rejected by their own checks, 155 production sources, no finding |
| Debian 13 container, `verify-install.sh debian13` (GCC 14.2.0, Qt 6.8.2, KF 6.13.0, libplasma 6.3.5) | Exit 0 in 412 s: build, install and uninstall of the system and the user variant; 0 compiler warnings |

Mutation checks of the session (`gcc-debug`, one mutation at a time, each file restored byte for byte):

| Mutation | Result |
| --- | --- |
| P1. The undo of a failed round is skipped | Killed by `layoutMoveUndoesRoundAndRetriesOthers`, `layoutMoveRestoresEveryFailedFile` and `layoutMoveChainBlockedByExistingTargetKeepsEveryFile` |
| P2. No first stage: each file is renamed straight to its final path | Killed by `layoutMoveChainRenamesEveryFile` and `layoutMoveSwapExchangesFiles`, and by five more tests through `unrecovered` |
| P3. The existence check before `QFile::rename()` is removed | Survived the runtime tests: Qt 6.11.2 already refused the stray targets. Killed only by the source contract C10 |
| P4. A failed file is not returned to its old path | Killed by `layoutMoveIntoUnwritableDirectoryKeepsSourceFile`, `layoutMoveUndoesRoundAndRetriesOthers`, `layoutMoveRestoresEveryFailedFile` and `layoutMoveChainBlockedByExistingTargetKeepsEveryFile` |
| P5. A failed first-stage rename is treated as parked | Killed by `layoutMoveOfMissingSourceFailsAlone` (through `unrecovered`) |
| P6. The round does not stop at its first failure | Survived: every test gives the same outcome, the round only retries more often. The stop is kept to bound the number of renames |
| C1. Temporary directory at the default location | Killed by `layoutsSaveMovesFilesThroughCheckedTemporaryDir` |
| C2. Default-location fallback removed | Killed by `layoutsSaveMovesFilesThroughCheckedTemporaryDir` |
| C3. Second `isValid()` check removed | Killed by `layoutsSaveMovesFilesThroughCheckedTemporaryDir` |
| C4. Bare `.rename(` added to `save()` | Killed by `layoutsSaveMovesFilesThroughCheckedTemporaryDir` |
| C5. Id rewrite placed before the moves | Killed by `layoutsSaveMovesBeforeRewritingIds` |
| C6. Failed moves mapped and renamed | Killed by `layoutsSaveRenamesOnlyMovedLayouts` |
| C7. Temporary directory always removed | Killed by `layoutsSaveMovesFilesThroughCheckedTemporaryDir` |
| C8. Failed row keeps the new name | Killed by `layoutsSaveRenamesOnlyMovedLayouts` |
| C9. Layout scanner lists directories | Killed by `layoutScannersListOnlyFiles` |
| C10. No-overwrite guard removed | Killed by `layoutFileMoveRenamesOnlyThroughNoOverwriteCheck` |

The three existing contracts of Apply (`layoutsSaveRewritesRenamedIdsFromOneSnapshot`,
`layoutsSaveLocksRenamedLayoutsAtTheirNewFile` and `layoutsSaveHandsSynchronizerTableAfterRename`)
pass unchanged. The rollback behavior of `save()` itself has no runtime test: it needs a
Corona. The source contracts only check its text, so the desktop check below is the evidence
for it.

The main session ran five mutations again on `e839808c4`, because the pointer change
re-spelled the contracts that guard them: the first creation at the default location, the
fallback removed, the second `isValid()` check removed, the temporary directory always
removed, and the directory path no longer handed to `moveFiles()`. Each one failed
`layoutsSaveMovesFilesThroughCheckedTemporaryDir` (the last also
`layoutsSaveMovesBeforeRewritingIds`); unmutated and after the restore, `sourcecontracttest`
passed its 230 cases.

Desktop check on 2026-11-03: the user-mode Debug build of `e839808c4` on the Gentoo host
(Plasma 6.7.5, Wayland, two screens), started from the Plasma session environment, with the
layout `我的布局`, two throwaway layouts that the main session created in
`~/.config/latte/` and removed afterwards (`LattePlayA`, green, with an activity id this
machine does not have, and `LattePlayB`, blue) and two empty blocker files. A watcher script
logged every change of the layout files four times a second, so the state between two
Applies can be read afterwards. The user ran the steps.

| Step | Result | Evidence |
| --- | --- | --- |
| Rename `LattePlayA` to `Stray`, whose empty file existed when the dock started | Refused by the page before Apply: "Layout Stray is already used, please provide a different name..." | No Apply line in the log and no file changed. The page lists the files the dock found at startup, so this exercised the existing duplicate-name check, not the new code |
| Failed move: rename `LattePlayA` to `Blocked1`, whose empty file was created after the dock started (the page does not list it), and Apply | Pass | The page showed "Layout LattePlayA could not be renamed to Blocked1, its name and file are unchanged, rename it again to retry..." (user report; English because the strings are not translated yet). The Apply at 22:02:34 used `~/.config/latte/.apply-AhHktK`, which was gone afterwards. `LattePlayA`, `LattePlayB`, `Stray` and `Blocked1` kept their checksums and modification times |
| Swap: rename `LattePlayA` to `LattePlayA1`, `LattePlayB` to `LattePlayA` and `LattePlayA1` to `LattePlayB` in the table, then Apply once | Pass | Apply at 22:05:14: `LattePlayA.layout.latte` held the former content of `LattePlayB` (blue, 1424 bytes) and `LattePlayB.layout.latte` that of the former `LattePlayA` (green, the unknown activity id, 289 bytes), each with the checksum of the original |
| The same three renames as three separate Applies, the sequence of the observation in follow-up 12 | Pass | Applies at 22:05:30, 22:05:39 and 22:05:51. After each, the watcher shows the right file under the right name: `LattePlayA1` blue with `LattePlayB` green; then `LattePlayA` green with `LattePlayA1` blue; then `LattePlayA` green with `LattePlayB` blue. The final files are byte-identical to the originals |

- **Log.** Five Applies, each with its own `.apply-XXXXXX` directory next to the layouts. The
  log has 474 lines, no warning, no `No such method`, no Fatal or ASSERT; the dock quit with
  the teardown marker `Latte Corona - deleted...` and no core followed. The three real
  layouts have the same checksums as before the run, and no `.apply-*` directory was left.
- **What it shows.** The move-failure path of `save()` works end to end on the desktop: the
  row keeps its name and its file, the user gets the reason, and nothing is left behind.
  The swap and the three separate renames give the right files when `/tmp` has free space,
  and the fixed Apply does not use `/tmp` for them. The three separate Applies were not run
  on the pre-fix build with free space, so the full `/tmp` as the cause of the observation
  in follow-up 12 stays likely and not proven.
- **Not reached.** A new layout (file still in `/tmp`) whose move fails, the message for a
  file that could not be moved back, the multiple-layouts mode, and F4.

### Not run in the follow-up fixes

A build on Qt 6.6 or 6.7 (the oldest Qt built is 6.8.2); the Fedora VM, which is left to
R5; the Debian 13 VM, which has no build tools (the Debian 13 container build ran); the
multiple-layouts mode; a desktop run of the pre-fix build; the three separate Applies on the
pre-fix build with free space in `/tmp`; a new layout whose move fails; the message for a
file that could not be moved back.

## Next actions

The R2c fix slices are complete: each of the 19 checks of the inventory is fixed or
excepted with a reason, the gate holds 49 checks, and [fix batch 3](#fix-batch-3) passed
CI and the desktop smoke. What is left is the [follow-ups](#follow-ups): decisions and
defects the slices found on the way, not findings. Item 1 and item 4 were resolved by
R7b and R7d; F5 of item 7 was resolved by R7c. Items 3, 5, 6, 9, 10 and 12 and F1 to F4
of item 7 are fixed ([follow-up fixes](#follow-up-fixes)). Item 2 remains unobserved in
its required multi-screen scenario. Item 11's retry implementation was completed
by [R7a](R7a.md); real Mageia cache reuse was not verified in that local record.

## Reproducing the environment

The recipe is not part of the repository because the CI job installs the same
packages directly. Mirrors are local build arguments only; the defaults are the
upstream sources.

```dockerfile
# Local mirror of the "Build (Clang Debug)" job in .github/workflows/build.yml,
# plus clazy. The package list below was extracted from the workflow's
# "Install build dependencies" step; the workflow stays authoritative.
# Mirrors are build arguments only: the defaults are the upstream sources.
ARG BASE_IMAGE=ubuntu:24.04
FROM ${BASE_IMAGE}

ENV DEBIAN_FRONTEND=noninteractive
ARG UBUNTU_MIRROR=

RUN if [ -n "$UBUNTU_MIRROR" ]; then \
      sed -i -E "s#https?://(archive|security)\.ubuntu\.com/ubuntu/?#${UBUNTU_MIRROR}#g" \
        /etc/apt/sources.list.d/ubuntu.sources ; \
    fi

# "Install KDE Neon repository" step
RUN apt-get update && \
    apt-get install -y --no-install-recommends ca-certificates wget gnupg && \
    wget -qO /etc/apt/trusted.gpg.d/neon.asc https://archive.neon.kde.org/public.key && \
    echo "deb https://archive.neon.kde.org/user noble main" > /etc/apt/sources.list.d/neon.list && \
    apt-get update

# "Install build dependencies" step
RUN apt-get install -y --no-install-recommends \
      build-essential clang clang-tidy cmake extra-cmake-modules pkgconf \
      dbus-daemon git python3 qt6-base-dev qt6-declarative-dev qt6-declarative-private-dev \
      qt6-wayland-dev qt6-tools-dev libkf6archive-dev libkf6config-dev libkf6coreaddons-dev libkf6crash-dev \
      libkf6dbusaddons-dev libkf6declarative-dev libkf6globalaccel-dev libkf6guiaddons-dev libkf6i18n-dev libkf6iconthemes-dev \
      libkf6kio-dev libkf6itemmodels-dev libkf6newstuff-dev libkf6notifications-dev libkf6package-dev libkf6windowsystem-dev \
      libkf6xmlgui-dev kf6-ksvg-dev libplasma-dev libkpipewire-dev libkf6wayland-dev plasma-activities-dev \
      plasma-activities-stats-dev plasma-workspace-dev plasma-pa plasma-wayland-protocols breeze-icon-theme libqt6svg6 \
      libwayland-dev liblayershellqtinterface-dev

# Added by R2c: the new "Install clazy" step in the Clang Debug job
RUN apt-get install -y --no-install-recommends clazy

ENV CC=clang CXX=clang++
WORKDIR /src
```

```bash
docker build --network host \
    --build-arg BASE_IMAGE=docker.m.daocloud.io/library/ubuntu:24.04 \
    --build-arg UBUNTU_MIRROR=http://mirrors.ustc.edu.cn/ubuntu/ \
    --tag latte-r2c-clazy:noble .

# Build tree on its own mount so a host build of the same checkout stays intact.
mkdir -p "$HOME/r2c-build"
docker run --rm -v "$PWD":/src:ro -v "$HOME/r2c-build":/src/build -w /src latte-r2c-clazy:noble bash -c '
    set -euo pipefail
    export LATTE_RELEASE_VERSION="$(python3 scripts/release-version.py resolve)"
    cmake --preset clang-debug -DVERSION="$LATTE_RELEASE_VERSION"
    cmake --build --preset clang-debug
    python3 scripts/test-clazy.py --build-dir build/modernization/clang-debug --jobs 8'
```

Without the mirror arguments the build uses `ubuntu:24.04` and the upstream
Ubuntu archive.
