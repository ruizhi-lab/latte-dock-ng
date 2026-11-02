# R2c-range-loop-detach-j: Range Loops Iterate a Const View or a Const Local

Status: Pending validation. Part of [R2c](R2c.md) fix batch 3, group J: the
`range-loop-detach` slice, 12 files and 72 of its findings. One commit on
`claude/r2c-batch-j-a88568`. Date: 2026-11-02. Start HEAD: `e62767af2` (the
integration branch, with batches E, F and H merged). Not pushed. The gate does
not yet include `range-loop-detach`; the main session adds it after all groups
are merged.

## Defect

clazy 1.17 reports "c++11 range-loop might detach Qt container" for a range-based
`for` over a non-const Qt container, or over a call that returns a container by
value. The loop calls the non-const `begin()`, which copies the container when its
data is shared with another owner. The copy is an allocation plus a copy of every
element; no functional defect or symptom is known. The 72 findings in these 12
files are 34 loops over a non-const lvalue (a member, a parameter or a local) and
38 loops over a call that returns a container by value.

## Fix

- **34 loops wrapped.** `for (DECL : container)` became
  `for (DECL : std::as_const(container))`. Each body was read: it does not write
  the container, directly, through a callee, or through a signal (table below).
  Every loop variable is a copy or a const reference, so each loop compiles with
  const elements. No include was added.
- **38 loops hoisted.** `for (DECL : call())` became
  `for (const auto NAME = call(); DECL : NAME)`, a C++20 range-for init-statement.
  The local is const, so the loop uses a const `begin()`. The call runs once, at
  the same point, and the local lives as long as the loop, as the temporary did.
  Each name was grepped in its enclosing function, and none existed there. No
  call in these loops returns a reference.
- **Nothing else changed.** The 72 changed lines are loop headers only: no body,
  no include, no comment and no `clazy:exclude`. `git clang-format --diff HEAD`
  on the 12 files has no hunk on these lines.
- **Needs a decision: none.** Four wrapped loops rest on a call-graph trace
  rather than on a body that is visibly read-only, and clazy reports no finding
  for six other loops that were left alone. Both groups are listed below.

## Wrapped loops (34)

Each row gives the iterated container and the reason the body does not write it.

| File:line | Container | Why the body does not modify it |
| --- | --- | --- |
| `app/layouts/synchronizer.cpp:405` | member `m_centralLayouts` in `screenForContainment()` | Calls `contains()` and `screenForContainment()` on each layout and returns. No Synchronizer writer is called. |
| `app/layouts/synchronizer.cpp:416` | member `m_centralLayouts` in `viewForContainment(uint)` | Calls `viewForContainment(id)` on each layout and returns. No Synchronizer writer is called. |
| `app/layouts/synchronizer.cpp:429` | member `m_centralLayouts` in `viewForContainment(Plasma::Containment *)` | Same as `:416`. |
| `app/layouts/synchronizer.cpp:472` | local `freelayoutnames` in `onActivityRemoved()` | Calls `centralLayout()` and emits `activitiesChanged()`. The list is passed to no callee. |
| `app/layouts/synchronizer.cpp:484` | member `m_centralLayouts` in `hideAllViews()` | Emits `currentLayoutIsSwitching()` only. Traced; see Needs a decision. |
| `app/layouts/synchronizer.cpp:492` | member `m_centralLayouts` in `syncActiveLayoutsToOriginalFiles()` | Calls `syncToLayoutFile()`, which writes the layout file through `Storage`. Traced; see Needs a decision. |
| `app/layouts/synchronizer.cpp:500` | member `m_centralLayouts` in `syncLatteViewsToScreens()` | Calls `GenericLayout::syncLatteViewsToScreens()`, which adds, deletes and reconsiders the views of that layout. Traced; see Needs a decision. |
| `app/layouts/synchronizer.cpp:536` | local `files` in `initLayouts()` | Calls `onLayoutAdded()`, which changes `m_layouts`. The list is passed to no callee. |
| `app/layouts/synchronizer.cpp:570` | `m_layouts[i].activities` in `reloadAssignedLayouts()` | Writes `m_assignedLayouts` and reads `m_layouts[i].name`. The activities list itself is not written. |
| `app/layouts/synchronizer.cpp:603` | member `m_centralLayouts` in `unloadPreloadedLayouts()` | Appends each layout name to the local `currentnames`, and nothing else. |
| `app/layouts/synchronizer.cpp:609` | local `preloadednames` in `unloadPreloadedLayouts()` | Reads the local `currentnames` and calls `moveToLayoutFile()`. The list is passed to no callee. |
| `app/layouts/synchronizer.cpp:684` | member `m_centralLayouts` in `initMultipleMode()` | Emits `currentLayoutIsSwitching()` only. Traced; see Needs a decision. |
| `app/layouts/synchronizer.cpp:815` | local `freelayoutnames` in `switchToLayoutInMultipleModeBasedOnActivities()` | Same as `:472`. |
| `app/layouts/synchronizer.cpp:914` | member `m_centralLayouts` in `syncMultipleLayoutsToActivities()` | Reads each `name()` and adds to the local lists `layoutNamesToLoad` and `layoutNamesToUnload`. |
| `app/layouts/synchronizer.cpp:920` | parameter `preloadedLayouts` in `syncMultipleLayoutsToActivities()` | Adds to the local `layoutNamesToUnload`. The parameter is passed to no callee. |
| `app/layouts/synchronizer.cpp:940` | local `layoutNamesToLoad` in `syncMultipleLayoutsToActivities()` | Creates and adds layouts, which changes `m_centralLayouts` and not this list, and emits signals. The list is passed to no callee. |
| `app/layouts/synchronizer.cpp:984` | local `layoutNamesToUnload` in `syncMultipleLayoutsToActivities()` | Emits `currentLayoutIsSwitching(layoutname)` only. The timer lambda after the loop takes its own copy. |
| `app/layouts/importer.cpp:77` | local `userLayouts` in `updateOldConfiguration()` | Calls `importOldConfiguration(userConfig)`. The list is passed to no callee. |
| `app/layouts/importer.cpp:215` | local `paths` in `standardPath()` | Calls `QFileInfo::exists()` and returns. |
| `app/layouts/importer.cpp:627` | local `files` in `availableLayouts()` | Appends to the local `layoutNames`. |
| `app/layouts/importer.cpp:643` | local `files` in `availableViewTemplates()` | Appends to the local `templates`. |
| `app/layouts/importer.cpp:650` | local `sfiles` in `availableViewTemplates()` | Reads each name and appends to `templates`. |
| `app/layouts/importer.cpp:670` | local `files` in `availableLayoutTemplates()` | Appends to the local `templates`. |
| `app/layouts/importer.cpp:677` | local `sfiles` in `availableLayoutTemplates()` | Reads each name and appends to `templates`. |
| `app/lattecorona.cpp:919` | local `views` in `availableScreenRegionWithCriteria()` | Reads the geometry and visibility of each view. `views` is used only for `isEmpty()` and this loop. |
| `app/lattecorona.cpp:1091` | local `views` in `availableScreenRectWithCriteria()` | Same as `:919`. |
| `app/layouts/manager.cpp:352` | local `deprecatedActionGroup` in `cleanupOnStartup()` | Deletes config groups. The list is not written. |
| `app/layouts/manager.cpp:429` | local `removeContaimentsList` in `cleanupOnStartup()` | Deletes config groups. The list is not written. |
| `app/wm/plasmapaneltracker.cpp:170` | member `m_panels` in `refreshGeometries()` | Reads each panel and appends to the local `next`. `m_panels` is written only in `clear()` (line 84) and `updatePanels()` (line 161), and the body calls neither. |
| `app/settings/settingsdialog/layoutscontroller.cpp:92` | member `m_tempDirectories` in `~Layouts()` | Removes each directory through the local `tDir`. The list is not written. |
| `app/settings/universalsettings.cpp:81` | local `colorsScriptPaths` in `load()` | Calls `KDirWatch::addDir()` on each path. |
| `app/settings/settingsdialog/layoutsmodel.cpp:907` | local `activities` in `Layouts::initActivities()` | Builds a `KActivities::Info` and calls `onActivityAdded(id)`. The list is passed to no callee. |
| `app/settings/viewsdialog/viewscontroller.cpp:403` | member `m_currentLayoutConnections` in `Views::onCurrentLayoutChanged()` | Calls `QObject::disconnect(var)`. The list is refilled after the loop. |
| `declarativeimports/core/iconitem.cpp:639` | member `m_overlays` in `IconItem::loadPixmap()` | Passes the list to `KIconUtils::addOverlays()`, which does not change `m_overlays`, and breaks out of the loop. |

## Hoisted loops (38)

Each row gives the call and the name of the const local that now holds its
result for the length of the loop.

| File:line | Call (enclosing function) | Local |
| --- | --- | --- |
| `app/layouts/synchronizer.cpp:79` | `layout->latteViews()` in `latteViewExists()` | `views` |
| `app/layouts/synchronizer.cpp:155` | `runningActivities()` in `freeRunningActivities()` | `activityIds` |
| `app/layouts/synchronizer.cpp:344` | `currentLayouts()` in `currentViews()` | `currents` |
| `app/layouts/synchronizer.cpp:355` | `currentLayouts()` in `currentOriginalViews()` | `currents` |
| `app/layouts/synchronizer.cpp:366` | `currentLayouts()` in `currentViewsWithPlasmaShortcuts()` | `currents` |
| `app/layouts/synchronizer.cpp:387` | `centralLayoutsForActivity(id)` in `viewsBasedOnActivityId()` | `layoutsForActivity` |
| `app/layouts/synchronizer.cpp:907` | `runningActivities()` in `syncMultipleLayoutsToActivities()` | `activityIds` |
| `app/layouts/importer.cpp:106` | `containments.groupList()` in `importOldLayout()` | `groupNames` |
| `app/layouts/importer.cpp:126` | `applets.groupList()`, nested in the `:106` loop | `appletIds` |
| `app/layouts/importer.cpp:151` | `containments.groupList()` in `importOldLayout()` | `groupNames` |
| `app/layouts/importer.cpp:313` | `rootDir->entries()` in `importOldConfiguration()` | `entries` |
| `app/layouts/importer.cpp:389` | `availableLayouts()` in `exportFullConfiguration()` | `layoutNames` |
| `app/layouts/importer.cpp:794` | `linkedContainments.groupList()` in `checkRepairMultipleLayoutsLinkedFile()` | `groupNames` |
| `app/layouts/importer.cpp:827` | `linkedContainments.groupList()` in `checkRepairMultipleLayoutsLinkedFile()` | `groupNames` |
| `app/lattecorona.cpp:97` | `containment->applets()` in `appendLoadedAppletIds()` | `applets` |
| `app/lattecorona.cpp:581` | `qGuiApp->screens()` in `Corona::load()` | `screens` |
| `app/lattecorona.cpp:678` | `containmentsEntries.groupList()` in `Corona::cleanConfig()` | `containmentIds` |
| `app/lattecorona.cpp:688` | `appletsEntries.groupList()`, nested in the `:678` loop | `appletIds` |
| `app/lattecorona.cpp:706` | `containments()` in `Corona::containmentExists()` | `containmentList` |
| `app/lattecorona.cpp:719` | `containments()` in `Corona::appletExists()` | `containmentList` |
| `app/lattecorona.cpp:730` | `containment->applets()` in `Corona::appletExists()` | `applets` |
| `app/lattecorona.cpp:1391` | `containments()` in `Corona::containmentsIds()` | `containmentList` |
| `app/lattecorona.cpp:1402` | `containments()` in `Corona::appletsIds()` | `containmentList` |
| `app/lattecorona.cpp:1544` | `m_layoutsManager->synchronizer()->menuLayouts()` in `Corona::contextMenuData()` | `layoutNames` |
| `app/lattecorona.cpp:1670` | `m_layoutsManager->currentLayouts()` in `Corona::toggleHiddenState()` | `layouts` |
| `app/layouts/appletids.cpp:81` | `appletIdListEntries()` in `updateIdLists()` | `idListEntries` |
| `app/layouts/appletids.cpp:111` | `applets.groupList()` in `resolveApplets()` | `appletIds` |
| `app/layouts/appletids.cpp:136` | `applets.groupList()` in `resolveNestedApplets()` | `appletIds` |
| `app/layouts/appletids.cpp:159` | `appletsGroup.groupList()` in `collect()` | `appletIds` |
| `app/layouts/appletids.cpp:174` | `collect(containment.group(APPLETSGROUP()))`, nested in the `:173` loop of `makeUnique()` | `appletIds` |
| `app/layouts/manager.cpp:344` | `actionGroups.groupList()` in `cleanupOnStartup()` | `actionGroupNames` |
| `app/layouts/manager.cpp:361` | `containmentGroups.groupList()` in `cleanupOnStartup()` | `containmentGroupNames` |
| `app/layouts/manager.cpp:495` | `qGuiApp->screens()` in `showInfoWindow()` | `screens` |
| `app/wm/plasmapaneltracker.cpp:54` | `qGuiApp->screens()` in the constructor | `screens` |
| `app/wm/plasmapaneltracker.cpp:174` | `qGuiApp->screens()`, nested in the `:170` loop of `refreshGeometries()` | `screens` |
| `app/settings/settingsdialog/layoutscontroller.cpp:862` | `archiveRootDir->entries()` in `Layouts::importLayoutsFromV1ConfigFile()` | `entryNames` |
| `app/settings/universalsettings.cpp:494` | `m_screenScalesGroup.keyList()` in `loadScalesConfig()` | `screenNames` |
| `app/wm/waylandinterface.cpp:73` | `containmentsGroup.groupList()` in `plasmaPanelGeometriesFromConfig()` | `containmentIds` |

## Needs a decision

None. The two fixes cleared all 72 findings. Four wrapped loops are cleared by a
call-graph trace rather than by a visibly read-only body. The main session may
want to review that trace:

- `app/layouts/synchronizer.cpp:484` (`hideAllViews()`) and `:684`
  (`initMultipleMode()`) emit `currentLayoutIsSwitching()`. The connected slots
  are `Positioner::onCurrentLayoutIsSwitching()`, which hides the view through
  `slideOutDuringExit()`, and the lambda in `PrimaryConfigView` that calls
  `hideConfigWindow()`. The Manager re-emits the signal (`manager.cpp:53`). No
  QML connects to it (grep of the tree). Neither slot reaches a writer of
  `m_centralLayouts`.
- `:492` (`syncActiveLayoutsToOriginalFiles()`) and `:500`
  (`syncLatteViewsToScreens()`) call `GenericLayout::syncToLayoutFile()`, which
  writes the layout file, and `GenericLayout::syncLatteViewsToScreens()`, which
  adds, deletes and reconsiders views and touches only the layout's own
  containers. The writers of `m_centralLayouts` are `addLayout()`,
  `unloadCentralLayout()`, both `unloadLayouts()` overloads and
  `syncMultipleLayoutsToActivities()`. They are reached only from
  `Synchronizer::switchToLayout()`, `unloadLayouts()` and
  `syncMultipleLayoutsToActivities()` (the last one from the constructor's
  KActivities lambdas and from `switchToLayoutInMultipleMode()`). Those entry
  points are called from Corona's D-Bus methods, from the settings dialogs, from
  `Manager::unload()` (called by `Corona::onAboutToQuit()` and `~Corona()`) and
  from the timers, and none of them is called from these loop bodies. This is a
  reading of the call graph, not a test.

Not findings, left unchanged. clazy 1.17 reports no finding for the loops below,
so they are outside the 72 and were not edited:

- `app/layouts/synchronizer.cpp:456` and `:798`, `for (const auto &explicitlayoutname : explicits)`,
  over a local copy of an assigned list. `app/wm/waylandinterface.cpp:108`, over the local
  `panels`. `app/layouts/manager.cpp:451`, over the by-value parameter `containmentsIds`.
  Why clazy stays silent here was not traced.
- `app/layouts/appletids.cpp:91` (`for (auto &id : ids)`) and
  `app/settings/settingsdialog/layoutscontroller.cpp:142` (`for (auto &file : files)`).
  The loop variable is a non-const reference, and the body writes each element, so
  a const view cannot apply. A fix would need another loop form, which is outside
  the two fixes.

## Evidence

Toolchains. Container: image `latte-r2c-clazy:noble` (ID `8e9528e34f50`), Ubuntu
clang 18.1.3 (LLVM 19.1.1), clazy 1.17.1 (KDE Neon build), Qt 6.11.1
(`qt6-base-dev 6.11.1`). Host: GCC 16.2.1 (Gentoo), Clang 22.1.8
(`/usr/lib/llvm/22/bin/clang++`), CMake 4.3.4, Qt 6.11.2.

| Command | Result |
| --- | --- |
| Container, start HEAD `e62767af2`: `cmake --preset clang-debug -DVERSION=...`, `cmake --build --preset clang-debug -j4`, `test-clazy.py --build-dir build/modernization/clang-debug --jobs 4 --inventory range-loop-detach --report build/rld-before.json` | Exit 0. Full build: 425 objects, **0 `warning:` lines**. Inventory: **209 findings in 25 files**. The 12 owned files hold **72**: lattecorona 13, importer 14, synchronizer 24, appletids 5, manager 5, plasmapaneltracker 3, layoutscontroller 2, universalsettings 2, layoutsmodel 1, viewscontroller 1, waylandinterface 1, iconitem 1 |
| `apply-edits.py` on the 12 files: 72 exact-match edits, each asserting the loop header text on its line; `python3 -I loop-diff-check.py e62767af2 <12 files>` | `72 loops changed as expected; 0 lines to review`. `git diff --stat`: 12 files, 72 insertions, 72 deletions |
| Container, after the edits (same configure; incremental build; `--inventory range-loop-detach --report build/rld-after.json`) | Exit 0. Incremental build: 14 objects, **0 `warning:` lines**, no compiler error. Inventory: **137 findings in 13 files; 0 in the 12 owned files**. The other 137 are identical to the baseline's 137 on file, line and message (`compare-rld.py`: `identical=True`) |
| `git clang-format --style=file --diff HEAD -- <12 files>` | `clang-format did not modify any files`: no hunk on a changed line |
| Full gate, container: `test-clazy.py --build-dir build/modernization/clang-debug --jobs 4`, after the configure and build of the same run (its build: 0 `warning:` lines) | **Exit 0**: 48 negative fixtures each rejected by their own gated check; 153 production sources with no finding from 48 gated checks |
| Host GCC Debug on a fresh tree: `cmake --preset gcc-debug && cmake --build --preset gcc-debug -j4 && ctest --preset gcc-debug --output-on-failure` | Exit 0 for each step. Configure: 0 CMake warnings. Build: 425 objects, **0 `warning:` lines**. The 4 `error` matches in the log are the QML `console.warn("ContextMenu component load error:")` text, not compiler output. CTest: **100% passed, 0 failed out of 58** |
| Host Clang Debug on a fresh tree: the same commands with `clang-debug` | Exit 0 for each step. Configure: 0 CMake warnings. Build: 425 objects, **0 `warning:` lines**. The 4 `error` matches are the same QML text. CTest: **100% passed, 0 failed out of 58** |

Logs. The container logs (`build.log`, `build-after.log`, `build-gate.log`,
`configure*.log`), the inventory reports `rld-before.json` and `rld-after.json`,
the run logs, the gate log, `apply-edits.py`, `compare-rld.py` and the host logs
are kept in `$HOME/r2c-build-batch-j/evidence/` (outside the repository). The
host build trees are in the worktree's git-ignored `build/modernization/`.

Coverage of the changed code by existing tests: the source lists in
`autotests/CMakeLists.txt` name two of the twelve files, `app/layouts/appletids.cpp`
(target `appletidsunittest`) and `declarativeimports/core/iconitem.cpp` (target
`declarativecoreunittest`). The other ten are compiled only into the application.
Whether the cases of those two targets reach the changed loops was not checked.

## Handoff

- **Batch / slice and selected scope:** R2c fix batch 3, group J:
  `range-loop-detach`, 12 files, 72 findings in the inventory of `e62767af2`.
  All 72 are fixed: 34 wrapped in `std::as_const()` and 38 hoisted into a const
  range-init local. The tree still has 137 findings, all in other groups' files.
- **Status:** Pending validation. The container build, the inventory, the full
  gate and the host GCC and Clang Debug builds and CTest pass; the desktop smoke
  and CI have not run.
- **Source baseline / start HEAD / implementation HEAD:** `e62767af2` /
  `e62767af2` / the single commit that adds this report and the 12 files.
- **Branch / existing user edits preserved:** `claude/r2c-batch-j-a88568` in its
  own worktree; clean at the start. No other file is touched. `main` is untouched.
- **Dependencies and evidence links:** [R2c](R2c.md),
  [R2c-range-loop-detach-h](R2c-range-loop-detach-h.md) (the same kind of work,
  group H) and [R2c-container-anti-pattern](R2c-container-anti-pattern.md). Other
  groups own the other files of the check; this group does not touch them.
- **Files and externally observable behavior changed:** 12 production sources:
  `app/layouts/synchronizer.cpp`, `app/layouts/importer.cpp`,
  `app/lattecorona.cpp`, `app/layouts/appletids.cpp`, `app/layouts/manager.cpp`,
  `app/wm/plasmapaneltracker.cpp`,
  `app/settings/settingsdialog/layoutscontroller.cpp`,
  `app/settings/universalsettings.cpp`,
  `app/settings/settingsdialog/layoutsmodel.cpp`,
  `app/settings/viewsdialog/viewscontroller.cpp`, `app/wm/waylandinterface.cpp`
  and `declarativeimports/core/iconitem.cpp`; plus this report. No observable change
  is intended: the same elements, in the same order, from the same snapshot.
- **State authority / invalidation / lifetime / fallback constraints:** each
  wrapped loop reads a container that its owner keeps, and the table gives the
  writer or the reason none can run. A hoisted loop iterates the snapshot that the
  call returned at the loop's start, as before. A body that must see a change made
  during the loop has to iterate the object itself, not the local. For
  `synchronizer.cpp` `:484`, `:492`, `:500` and `:684`, a later edit that makes a
  callee add to or remove from `m_centralLayouts` synchronously (for example a
  signal receiver that calls `switchToLayout()` in multiple-layouts mode) must copy
  the list first, as the snapshot loops of `windowstracker.cpp` do. AGENTS.md
  rule 7 asks for a note beside that code. This batch adds none, because the brief
  keeps the diff to the loop headers; the main session may add one.
- **Toolchain, import environment and artifact identity:** see Evidence.
- **Commands executed with exit status and durable log/artifact locations:** see
  Evidence and the logs listed there.
- **GCC/Clang Debug/Release warning/error results:** GCC 16.2.1 and Clang 22.1.8
  Debug: 0 `warning:` lines and 0 CMake warnings each. Container Clang 18.1.3
  Debug: 0 `warning:` lines. Release presets were not built.
- **Tests registered / run / passed / skipped / failed and name deltas:** 58
  registered; 58 run and 58 passed on GCC Debug and on Clang Debug; 0 skipped,
  0 failed. No test added, changed or skipped; no gate name added.
- **Production behavior vs copied logic vs source-contract evidence:** production
  code is compiled and analysed. The claims about the bodies rest on reading the
  callees (tables) and on the call-graph trace (Needs a decision), not on a test.
  The autotests that compile the changed files pass, but that does not show that
  they reach the changed loops.
- **QML diagnostics added/removed and environment compatibility:** none; no QML
  changed.
- **Negative fixtures or controlled mutation evidence:** none for this check here.
  `autotests/clazy/range-loop-detach.cpp` and the `GATED_CHECKS` entry are for
  the main session, after the tree-wide inventory is zero.
- **Desktop user feedback / log review / clean-quit and coredump evidence:** not
  run. The main session's desktop smoke should cover the paths that now run through
  the changed loops: layout startup and switching (`synchronizer.cpp`:
  `initLayouts()`, `syncMultipleLayoutsToActivities()`, `unloadPreloadedLayouts()`);
  Corona load and quit (`lattecorona.cpp`: `load()`, `cleanConfig()`, the
  `hideAllViews()` call at quit); a dock that dodges others, which runs the
  available-screen loops (`lattecorona.cpp:919` and `:1091`); the dock context
  menu (`contextMenuData()`), and a hidden or shown dock with the Meta key
  (`toggleHiddenState()`); a layout import, an export and the linked-file repair
  (`importer.cpp`); applet id resolution on import (`appletids.cpp`); the
  Layouts and Views settings dialogs and the universal settings scale load; the
  KWin panel geometry paths (`waylandinterface.cpp`, `plasmapaneltracker.cpp`); the
  icon overlays (`iconitem.cpp`); and `cleanupOnStartup()` and `showInfoWindow()`
  in `manager.cpp`.
- **Performance:** not measured. The wrap and the hoist add no work; each removes a
  possible detach at `begin()`. A detach happens only when the container is shared.
- **CI, distribution and Nix checks (passed / failed / unavailable):** CI not run;
  the Clang Debug job runs the gate after the merge. Debian 13 and Nix not run.
- **Unresolved failures, review assumptions and unavailable checks:** no failure in
  the checks run. Review assumptions: the four call-graph traces in Needs a
  decision, and the six loops clazy does not report, left unchanged. Not run: the
  desktop smoke, CI, Release presets, Qt 6.6 or 6.8 builds.
- **Next exact action:** the main session merges the groups. When the tree-wide
  inventory is zero, it adds `range-loop-detach` to `GATED_CHECKS` with
  `autotests/clazy/range-loop-detach.cpp` (137 findings remain in other groups'
  files), runs the full gate and the desktop smoke listed above, and decides on the
  note for the `m_centralLayouts` snapshot constraint.
- **Commit approval status / push approval status:** one commit on
  `claude/r2c-batch-j-a88568`; not pushed; `main` untouched.
