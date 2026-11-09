# R8f: the duplicate source contracts of R8a, checked by mutation

Status: Complete on branch `claude/r8f-duplicate-contracts`. Tests and documents only;
no production source changed.

Batch / slice and selected scope: R8f, the follow-up that the [R8a record](R8a.md)
names as its next action. Each of the 14 contracts that the
[R8a inventory](R8a-inventory.md) classed D (duplicate) was checked against the test it
was said to duplicate. A contract part was removed only when that test runs the
production code the part reads and fails under a scratch mutation of exactly what the
part protects.

Source baseline and start HEAD: `79881e5ac` (local and remote `main`, R8a merged).
Branch `claude/r8f-duplicate-contracts`, worktree
`.claude/worktrees/r8f-duplicate-contracts`. Commits:

| Commit | Subject |
|---|---|
| 328838b81 | test: check that the shutdown monitor sets the ending flag before it hides views |
| 575bba7cb | test: drop the source-contract parts that behavioural tests cover |
| (this commit) | docs: record the R8f duplicate-contract check |

## Summary

- One contract removed (row 201), four trimmed to the parts no test covers (rows 23, 24,
  68 and 223), two of them renamed after what they still check. Nine kept and
  reclassified, because their "twin" cannot fail when the production code they read
  breaks.
- One twin strengthened: the shutdown monitor's fake session records whether the
  session-ending flag was set when the hide hook ran, so the order the removed text
  check protected is now a behavioural assertion.
- Contract functions: 224 before, 223 after. QVERIFY/QCOMPARE lines: 1922 before, 1906
  after (`grep -c 'QVERIFY\|QCOMPARE'`). AGENTS.md rule 6 expects more than 170
  source-contract checks; the count stays above it.
- CTest: 61 registered, 61 run, 61 passed on GCC Debug and on Clang Debug, 0 compiler
  warnings in both full builds.

## Finding: the D class was too broad

Nine of the 14 D rows do not duplicate a test of the production code, for one of two
reasons that the inventory did not separate:

1. The twin builds no production source, so no production mutation can fail it.
   `importerlogictest` and `synchronizerlogictest` compile only their own `.cpp`, and
   the `availableId` cases of `storagelogictest` call `StorageLogic::availableId`, a copy
   defined in the test, because `app/layouts/storage.cpp` is not part of that target
   (see `autotests/CMakeLists.txt`). For these rows the contract is the only check of
   the production text.
2. The twin tests a helper while the contract checks a call site or a second copy.
   `appletidsunittest` runs `AppletIds` but not its callers in `Corona` and `Storage`;
   `containmentactionmenuunittest` runs `isCurrentLayoutName()` but not the
   `setCurrent()` call in `Menu::populateMoveToLayouts()` (mutation M172 below);
   `dataunittest` runs `Latte::PluginId` (`app/pluginids.h`), while
   `app/view/containmentinterface.cpp` keeps its own separator ids and its own
   `isSeparatorPluginId()`; `packageunittest` resolves file definitions but never loads
   a package with another plugin id, the case `Package::pathChanged()` branches on
   (mutation M106).

## Per-row outcome

Row numbers are those of the R8a inventory (the file at `6bdd43086`). The new class
uses the inventory's legend.

| Row | Contract | Twin | Outcome | New class | Evidence |
|---|---|---|---|---|---|
| 23 | `sessionShutdownMonitorHidesViewsAndRestoresOnCancel`, renamed `sessionShutdownHooksHideViewsAndSuspendHelperWindows` | shutdownmonitortest | trimmed: the monitor part is removed; the hooks in `main.cpp`, the synchronizer and the visibility manager parts stay | B3 | M23a, M23b, M23c |
| 24 | `coronaTeardownSkipsFactoryFreedByQuitCleanup` | teardowntest | trimmed: the `teardown.h` part is removed; the Corona header and destructor parts stay | B3 | M24 |
| 68 | `autostartPreservesEntryFields`, renamed `autostartStaysXdgOnly` | autostartlogictest | trimmed: the two Hidden writes are removed; the checks against a copy by modification time (dropped in 85e17681a) and a systemd unit (dropped in a8c0d2337) stay, because the twin passes a missing source path and cannot see either | K | M68a, M68b |
| 105 | `containmentInterfaceHasSeparatorPluginConstantsAndHelper` | dataunittest | kept: the twin tests `Latte::PluginId`, the contract the separate copy in `containmentinterface.cpp` | K | build sources |
| 107 | `lattePackageShellPluginNameBranchingIsCorrect` | packageunittest | kept: no test loads a package with another plugin id | B1 | M106 |
| 132 | `storageAvailableIdUpperBoundExcludesId32000` | storagelogictest | kept: the twin calls a copy | B2 | build sources |
| 135 | `importerFileVersionDetectsLayoutVersion2AndConfigVersion1` | importerlogictest | kept: the twin builds no production source | B2 | build sources |
| 136 | `importerLayoutCanBeImportedRejectsVersionBelow2` | importerlogictest | kept: as row 135 | B2 | build sources |
| 169 | `isolatedWindowPreviewProcessIsFailClosed` | previewprocessunittest | kept: of its 85 assertions only a few identifier checks overlap the twin; the rest read the helper's `main.cpp`, QML and CMake files, which no test builds | K | reading |
| 176 | `contextMenuMarksCurrentLayoutsInMoveMenu` | containmentactionmenuunittest | kept: the twin tests the helper, not the call | B2 | M172 |
| 197 | `synchronizerGuardsStoredLayoutLookups` | synchronizerlogictest | kept: the twin builds no production source | B2 | build sources |
| 201 | `layoutFileMoveRenamesOnlyThroughNoOverwriteCheck` | storagelogictest | removed | (gone) | M196a, M196b |
| 216 | `importedAppletIdsStayUniqueIncludingNestedApplets` | appletidsunittest | kept: the order of the calls in Corona and Storage | B3 | build sources |
| 223 | `appletAlternativesUseLattesOwnDialogOnPlasma65` | packageunittest | trimmed: the package part is removed; the dialog's QML text, the Corona call order and the helper parts stay | K | M218a, M218b |

Class counts over the 223 functions after R8f: K 117, B1 2, B2 50, B3 48, D 0, S 6
(after R8a they were K 113, B1 1, B2 45, B3 45, D 14, S 6 over 224). "Build sources"
means the target's source list in `autotests/CMakeLists.txt` shows that the twin cannot
reach the production text, so no mutation run was needed.

## Mutations

Each mutation was applied in the worktree by `/data/tmp/r8f/mutate.py` (exact text
replacement that refuses to write unless each replaced text occurs once), the twin
target was rebuilt and run with `ctest -R '^<twin>$'`, and the file was restored with
`git checkout --`, after which `git status --porcelain` was empty, the twin was rebuilt
and passed again (exit 0 for all twelve). The script and the logs are scratch files
outside the repository (`/data/tmp/r8f/`); the table keeps their content.

| ID | Row | Scratch edit | Twin | Expected | Result |
|---|---|---|---|---|---|
| M23a | 23 | `app/session/shutdownmonitor.cpp`: delete `m_hooks.hideViews();` | shutdownmonitortest | fail | fail, exit 8: `confirmedLogoutHidesViewsOnce`, `confirmedLogoutWaitsWhilePlasmaShutdownRuns`, `cancelledLogoutRestoresViewsWhenServiceLeaves` and `unansweredQueryKeepsTheLogoutState` report "Compared values are not the same" (10 passed, 4 failed) |
| M23b | 23 | delete `m_hooks.restoreViews();` | shutdownmonitortest | fail | fail, exit 8: `cancelledLogoutRestoresViewsWhenServiceLeaves` |
| M23c | 23 | call `m_hooks.hideViews()` before `m_hooks.markSessionEnding()` | shutdownmonitortest | fail | fail, exit 8: `confirmedLogoutHidesViewsOnce`, "'session.sessionEndingWhenHidden' returned FALSE"; this is the assertion commit 328838b81 adds |
| M24 | 24 | `app/session/teardown.h`: delete the `if (!service) return;` guard | teardowntest | fail | fail: "Received signal 11 (SIGSEGV)", accessing address 0x8, in `deleteNowSkipsServiceFreedByQuitCleanup` |
| M68a | 68 | `app/settings/autostart.cpp`: `enable()` writes `Hidden` true | autostartlogictest | fail | fail, exit 8: `enableUnhidesEntryAndPreservesFields`, "'Latte::Autostart::isEnabled(path)' returned FALSE" |
| M68b | 68 | `disable()` writes `Hidden` false | autostartlogictest | fail | fail, exit 8: `disableHidesEntryAndPreservesFields`, "'!Latte::Autostart::isEnabled(path)' returned FALSE" |
| M106 | 107 | `app/package/lattepackage.cpp`: `pathChanged()` compares with `org.kde.latte.shellX` | packageunittest | pass (no coverage) | pass, exit 0 |
| M172 | 176 | `containmentactions/contextmenu/menu.cpp`: `setCurrent(!isCurrentLayoutName(...))` | containmentactionmenuunittest | pass (no coverage) | pass, exit 0 |
| M196a | 201 | `app/layouts/layoutfilemove.cpp`: `renameWithoutOverwrite()` removes the target, then renames | storagelogictest | fail | fail, exit 8: `layoutMoveUndoesRoundAndRetriesOthers`, `layoutMoveRestoresEveryFailedFile` and `layoutMoveChainBlockedByExistingTargetKeepsEveryFile` ("Compared lists differ"; 28 passed, 3 failed) |
| M196b | 201 | `renameWithoutOverwrite()` drops its `QFileInfo::exists()` check | storagelogictest | recorded | pass, exit 0: `QFile::rename()` itself refuses an existing target, so the explicit check is defensive; an actual overwrite fails three cases (M196a) |
| M218a | 223 | `app/package/lattepackage.cpp`: delete the `appletalternativesui` definition | packageunittest | fail | fail, exit 8: `appletAlternativesDialogMatchesPlasmaRelease`, "Compared values are not the same" |
| M218b | 223 | raise the definition's guard to `QT_VERSION_CHECK(6, 99, 0)` | packageunittest | fail | fail, exit 8: the same case; the resolved dialog path is empty |

The host has Plasma 6.7.5, so `packageunittest` takes its `PLASMA_VERSION >= 6.5` branch;
on the Debian 13 floor (Plasma 6.3) it checks the opposite branch, which a removed guard
also fails.

## Evidence: commands and exit status

Run by a Haiku sub-agent from the worktree root and checked against its logs by the main
session. Build trees: `build/modernization/gcc-debug` and `build/modernization/clang-debug`
inside the worktree, configured fresh for this slice.

1. `cmake --preset gcc-debug`: exit 0; "Strict compiler warnings enabled for 63
   first-party compile targets".
2. Build of `shutdownmonitortest teardowntest autostartlogictest packageunittest
   containmentactionmenuunittest storagelogictest sourcecontracttest`: exit 0, 0
   `warning:` lines. Baseline `ctest -R` of the seven: exit 0, 7 of 7 passed.
3. The twelve mutations above; every restore left `git status --porcelain` empty and the
   restored twin passed.
4. `cmake --build --preset gcc-debug`: exit 0, 0 `warning:` lines. `ctest --preset
   gcc-debug`: exit 0, "100% tests passed, 0 tests failed out of 61".
5. `cmake --preset clang-debug` (Clang 22.1.8): exit 0. `cmake --build --preset
   clang-debug`: exit 0, 0 `warning:` lines. `ctest --preset clang-debug`: exit 0, "100%
   tests passed, 0 tests failed out of 61".
6. Afterwards `git status --porcelain` was empty and HEAD was still `575bba7cb`.

The build logs also contain four lines with " error:" in each compiler: they are
`console.warn("ContextMenu component load error:", ...)` source text that the qmllint step
quotes, not compiler diagnostics, as R8a recorded. The qmllint step's capitalised
`Warning:` lines (5970 in each log) are QML lint output outside the compiler gate; this
slice changes no QML.

## Final slice review

1. Scope: `autotests/shutdownmonitortest.cpp`, `autotests/sourcecontracttest.cpp` and
   this record; no production, CMake or QML file. The two renamed contracts and the
   trimmed ones carry a comment that names the test which now covers the removed part.
2. Each removed part has a twin that runs the production code and fails under a
   mutation of that part (M23a-c, M24, M68a-b, M196a, M218a-b). M106 and M172 show the
   opposite for two kept rows.
3. GCC and Clang Debug were configured fresh and built in full; all 61 tests ran.
4. No runtime path changed.
5. Not run: the Release presets (they run on `main` after the merge), the clazy gate
   (production files are unchanged and clazy reads only production translation units),
   deep qmllint (no QML changed), the Debian 13 build (deferred by the user until the
   refactor is complete) and any desktop check (none applies to a tests-only change).

## Follow-ups

- `ContainmentInterface` could use `Latte::PluginId::isSeparatorPluginId()` and
  `Latte::PluginId::kSplitter` instead of its own copies (`containmentinterface.cpp`,
  anonymous namespace). That is a production change with a desktop check of separators;
  afterwards row 105 can become a no-copy contract next to the `dataunittest` case.
- Row 107 (B1) can become a behavioural case: load a package whose metadata has another
  plugin id and check that `Package::pathChanged()` gives it the Latte shell fallback.
- Rows 132, 135, 136 and 197 have twins that test copies of the production algorithm. A
  conversion needs the production function to be reachable from a test target first.

## Next exact action

None for R8f. The next R8 slice is the user's choice (R8b is in progress on its own
branch; R8c is not started).
