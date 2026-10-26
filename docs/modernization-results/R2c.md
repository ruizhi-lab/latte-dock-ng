# R2c: clazy Inventory and Zero-Finding Gate

Status: Pending validation. Every local check passes; the new CI steps have not
run because nothing is pushed.
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
  at the inventory, 31 since `R2c-auto-unexpected-qstringbuilder`. A gated
  check cannot be vacuous: the runner proves it on its own fixture first.
- **Not gated, zero findings.** `foreach` and `qstring-comparison-to-implicit-char`
  cannot fire under the project flags; no trigger was found for `qstring-left`.
- **Next actions.** The 19 checks with findings become `R2c-<check>` slices.
  The first, [R2c-auto-unexpected-qstringbuilder](R2c-auto-unexpected-qstringbuilder.md),
  is done: its finding was a dangling `QStringBuilder` on Qt 6.6 and older.
- The inventory commit changed no production code, build rule or dependency of
  the product; the slice commits that follow it change one file each.

## Handoff

- **Batch / slice and selected scope:** R2c, a clazy inventory of production
  code and a CI gate for the checks with zero findings.
- **Status:** Pending validation (remote CI).
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
  analysis is CPU bound, so a 4-vCPU CI runner will need longer; the actual CI
  time is not yet measured.
- **CI, distribution and Nix checks:** the runner passes in a container that
  reproduces the Clang Debug job (below); the CI run itself is unavailable until
  a push. Distribution and Nix jobs are not affected.
- **Unresolved failures, review assumptions and unavailable checks:** see
  Pending validation.
- **Next exact action:** review this branch, then (if a push is approved) read
  the `Install clazy` and `Run gated clazy checks` steps of the Clang Debug job
  and record the run URL here. Then take the next row of the slice table. Do not start
  another batch implicitly.
- **Commit approval status / push approval status:** committed on the feature
  branch as the session brief allows; not pushed; `main` untouched.

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

## Pending validation

- The CI run of `Install clazy` and `Run gated clazy checks` (needs a push). The
  container matches the job's image, but package versions float: Neon's clazy
  and Qt move independently of this repository.
- CI wall time of the gate on a 4-vCPU runner.
- Ubuntu's own `clazy 1.11` was not tried; the job does not select it.
- The runner needs Python 3.9 or later (`Path.is_relative_to`), like the
  clang-tidy runner needs 3.8 for `shlex.join`; CI uses 3.12.
- GCC jobs are unchanged and do not run clazy: it needs a Clang compilation
  database.

## Next actions

Each slice fixes all findings of one check, confirms zero with
`--inventory CHECK`, adds `autotests/clazy/CHECK.cpp` and the name in
`GATED_CHECKS`. A reviewed exception may use `// clazy:exclude=CHECK` with a
reason instead of a rewrite. Done: `R2c-auto-unexpected-qstringbuilder`, see
[its report](R2c-auto-unexpected-qstringbuilder.md). Suggested order for the rest,
smallest and most mechanical first:

| Slice | Locations / files | Notes |
| --- | ---: | --- |
| `R2c-empty-qstringliteral` | 1 / 1 | `app/settings/actionsdialog/actionshandler.cpp:82`, `QIcon::fromTheme(QStringLiteral(""))` |
| `R2c-qfileinfo-exists` | 35 / 11 | `QFileInfo(path).exists()` to the static `QFileInfo::exists(path)` |
| `R2c-use-static-qregularexpression` | 6 / 2 | `app/knscompat.cpp` 5, `app/wm/tasktools.cpp` 1; hoist into a function-local `static const` |
| `R2c-range-loop-reference` | 15 / 6 | Bind non-trivial loop variables (`QString`, `QRegion`, one record) by `const &` |
| `R2c-unused-non-trivial-variable` | 14 / 10 | Delete after confirming construction has no side effect |
| `R2c-no-module-include` | 14 / 14 | Replace `<QtQml>` (11) and `<QtDBus>` (3) module includes with the headers actually used |
| `R2c-connect-not-normalized` | 10 / 4 | `QObject *` to `QObject*` in string signatures; check each against the call it feeds |
| `R2c-range-loop-detach` | 305 / 47 | Mainly `std::as_const()` around the range. Largest: `app/layouts/storage.cpp` 64, `app/layout/genericlayout.cpp` 28, `app/layouts/synchronizer.cpp` 26. A loop that modifies its container needs a copy or an index loop, not `as_const`; read each loop. Split by directory |
| `R2c-detaching-temporary` | 43 / 6 | 36 of the 43 are in `containment/plugin/layoutmanager.cpp`. 38 call `QList::operator[]` and 5 call `QList::first()` on a temporary; use `at()`, `constFirst()` or a named const reference |
| `R2c-container-anti-pattern` | 38 / 13 | Temporary containers from `keys()`, `values()` and similar; iterate the source directly. `app/wm/tracker/windowstracker.cpp` 9 |
| `R2c-connect-3arg-lambda` | 8 / 5 | All are `QTimer::singleShot(ms, lambda)` without a context object (`lattecorona.cpp`, `layouts/manager.cpp`, `layouts/synchronizer.cpp`, `wm/waylandinterface.cpp`, `contextmenu/menu.cpp` 4). Needs the per-site lifetime note R2b used |
| `R2c-incorrect-emit` | 4 / 3 | Three emits in constructors (`app/view/windowstracker/windowstracker.cpp` 2, `app/wm/tracker/trackedgeneralinfo.cpp`) that clazy expects to have no receiver yet, and one `windowRemoved` call without `Q_EMIT` in `app/wm/waylandinterface.cpp:838`. Review each |
| `R2c-qproperty-without-notify` | 1 / 1 | `app/settings/universalsettings.h:61`, the `screens` list property has neither NOTIFY nor CONSTANT; needs a change signal design |
| `R2c-const-signal-or-slot` | 10 / 4 | Const getters in slot sections (`GenericLayout::qmlFreeEdges`, `LayoutManager::isModernDockStyle` and others) that QML probably calls; the equivalent fix is `Q_INVOKABLE`. Check QML callers first |
| `R2c-non-pod-global-static` | 31 / 6 | Namespace-scope `QString` constants with dynamic initialization (`Latte::Data::ContextMenu::*`, `Latte::Layout::MULTIPLELAYOUTSHIDDENNAME`, `previewprocess.cpp` types). Needs a constant-initialized replacement and a check for static-initialization-order users |
| `R2c-fully-qualified-moc-types` | 21 / 9 | Signal and slot parameter types spelled inside namespaces. Changes the moc signature strings that string connects and QML may use; add a test per touched signal |
| `R2c-overloaded-signal` | 11 / 5 | Signals that overload a base-class signal (`xChanged`, `yChanged`, `rowsInserted`, `visibilityChanged`); renaming changes API, so expect `// clazy:exclude=overloaded-signal` with a reason |
| `R2c-overridden-signal` | 2 / 2 | `View::screenGeometryChanged` and `IconItem::enabledChanged`; same decision as above |

Optional hardening after the slices: an offline unit test of the runner's
parsing, selection and fail-closed paths with a fake `clazy-standalone`, so
those paths have an automated check on hosts without clazy.

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
