# R8a: source-contract cases converted to behavioural tests

Status: Complete on branch `claude/r8a-contract-tests-43d5f7`. Not pushed, not merged.
Start HEAD: 6bdd43086 (local `main`, with the Qt 6.7.0 and KF6 6.10.0 floor raise documented). The version floors in the root `CMakeLists.txt` are untouched.
Scope: tests and docs only. No production source changed; `git diff --name-only 6bdd43086..HEAD` lists only files under `docs/` and `autotests/`.

## Summary

- Inventory: all 229 functions of `autotests/sourcecontracttest.cpp` are classed in [R8a-inventory.md](R8a-inventory.md): K 113, B1 6, B2 45, B3 45, D 14, S 6.
- Conversions: five B1 cases, one commit each. Each new case calls production code, and each was proven with a scratch mutation that made it fail.
- Contracts removed: 5 (the five converted ones). Contracts kept: 224.
- CTest: 61 before, 61 after. No CTest registration changed; the new cases live in existing targets.
- Warnings: the GCC Debug and Clang Debug full builds each print 0 `warning:` lines and no compiler error (see [Warnings](#warnings)).

## Commits

| Commit | Subject |
|---|---|
| ea178f51e | docs: add the source-contract inventory tool and its unittest |
| 7e73af018 | docs: classify the 229 source-contract functions for R8a |
| b3b91f682 | test: check that servicesFromCmdLine stops on a bare runtime word |
| ab024dbdb | test: check that ScreenPool treats a detached mapping as inactive |
| 01a1e0b8a | test: check that ScreenPool::screenForId falls back to the primary screen |
| 5e6aae1de | test: check that the indicator factory never lists built-ins as custom |
| 07d5814bf | test: check the primary-connector sentinel of the extended ScreenPool |
| (this commit) | docs: record the R8a conversions and the inventory evidence |

## Files

- `docs/modernization-results/R8a-tools/list_contracts.py`: inventory tool; standard library only; deterministic.
- `docs/modernization-results/R8a-tools/test_list_contracts.py`: unittest on a synthetic sample. Run by hand, not registered in CTest.
- `docs/modernization-results/R8a-tools/contracts-before.md`: tool output for the file at 6bdd43086.
- `docs/modernization-results/R8a-tools/classes.tsv` and `gen_inventory.py`: the classification source and the generator of `R8a-inventory.md`.
- `docs/modernization-results/R8a-inventory.md`: one row per function (229) with class and reason.
- `autotests/wmunittest.cpp`, `autotests/corecomponentstest.cpp`, `autotests/indicatorunittest.cpp`: new cases.
- `autotests/sourcecontracttest.cpp`: five functions removed.

## Conversions

Each conversion removes one contract whose only assertions were source-text checks of the same behaviour. The mutation was made in a scratch edit of the production file, the test binary was rebuilt and run, and the file was restored with `git checkout --` before the commit. `git status` showed no change under `app/` at each restore.

### C1: bare runtime word does not recurse (contract row 182)

- Removed: `tasktoolsServicesFromCmdLineGuardsFirstSpace`, which searched `app/wm/tasktools.cpp` for `firstSpace >= 0`.
- New case: `WindowSystemUnitTest::servicesFromCmdLineSingleWordRuntimeDoesNotRecurse` in `wmunittest`. A runtime word from `[Settings]TryIgnoreRuntimes` given with no arguments must return no service and must return at all.
- Production code: `Latte::WindowSystem::servicesFromCmdLine()` in `app/wm/tasktools.cpp`, compiled into `wmunittest`. The failure mode is an endless recursion; fix `1d051010c` added the guard.
- Mutation: `if (ignore && firstSpace >= 0) {` changed to `if (ignore) {`.
  Result: `wmunittest` died with `Received signal 11 (SIGSEGV)` in the recursion; `ctest` reported `0% tests passed` and exit 8. Restored: `100% tests passed`, exit 0.

### C2: detached mapping is not active (contract row 140)

- Removed: `screenPoolIsScreenActiveReturnsFalseForStaleDisconnectedId`, which only checked for the names `isScreenActive` and `qGuiApp->screens()`.
- New case: `CoreComponentsTest::screenPoolIsScreenActiveRequiresAttachedScreen` in `corecomponentstest`, using `TestScreenPool`. A mapping whose connector is not attached stays in the table but is not active; an unmapped id and the sentinel id are not active.
- Mutation: the name test in `ScreenPool::isScreenActive` changed to `if (scr->name() == scrName || !scrName.isEmpty()) {`.
  Result: `FAIL! : ... '!pool.isScreenActive(detachedId)' returned FALSE`, at `corecomponentstest.cpp:180`; exit 8. Restored: `100% tests passed`.
- Not covered here: the attached branch. The offscreen platform gives its only screen an empty name, which `insertScreenMapping()` refuses. A live desktop check is needed for that branch.

### C3: primary-screen fallback (contract row 138)

- Removed: `screenPoolScreenForIdFallsBackToPrimaryWhenScreenAbsent`, which only checked for `screenForId` and `primaryScreen()`.
- New case: `CoreComponentsTest::screenPoolScreenForIdFallsBackToPrimaryScreen`. A detached mapping and an unmapped id both return `primaryScreen()`, never nullptr.
- Mutation: `QScreen *screen{primaryScreen()};` changed to `QScreen *screen{nullptr};`.
  Result: `FAIL! : ... Compared QObject pointers are not the same`, `Actual (pool.screenForId(detachedId)): <null>`, at `corecomponentstest.cpp:205`; exit 8. Restored: `100% tests passed`.

### C4: built-in indicators are not custom (contract row 99)

- Removed: `indicatorFactoryExcludesBuiltinPluginsFromCustomLists`, which checked the three ids and the name `isCustomType` in `factory.cpp`.
- New case: `IndicatorUnitTest::factoryBuiltInIndicatorsAreNotCustom` in `indicatorunittest`. `Factory::isCustomType()` is false for `org.kde.latte.default`, `org.kde.latte.plasma` and `org.kde.latte.plasmatabstyle`, and true for `org.example.custom`.
- Mutation: the `org.kde.latte.plasmatabstyle` exemption in `Factory::isCustomType` changed to `org.kde.latte.plasmatabstyleX`.
  Result: `FAIL! : ... '!factory.isCustomType(QStringLiteral("org.kde.latte.plasmatabstyle"))' returned FALSE`, at `indicatorunittest.cpp:135`; exit 8. Restored: `100% tests passed`.

### C5: primary-connector sentinel of the extended pool (contract row 139)

- Removed: `plasmaExtendedScreenPoolIdReturnsZeroForPrimaryConnector`, which checked for the text `primaryScreen()->name() == connector`.
- New case: `CoreComponentsTest::plasmaExtendedScreenPoolMapsPrimaryConnectorToIdZero`. `id(primary)` is 0 without the primary being stored, `connector(0)` returns the primary name, and an unmapped name gives -1.
- Mutation: the sentinel in `Latte::PlasmaExtended::ScreenPool::id()` changed from `? 0 : -1` to `? 7 : -1`.
  Result: `FAIL! : ... Actual (pool.id(primary)): 7`, `Expected (0): 0`, at `corecomponentstest.cpp:223`; exit 8. Restored: `100% tests passed`.
- Limit: the offscreen primary screen is unnamed, so the case compares the empty name through the same branch a named screen takes.

## Inventory result (before R8a)

Class counts over the 229 functions at 6bdd43086: K 113, B1 6, B2 45, B3 45, D 14, S 6.

- B1 (6): rows 99, 138, 139, 140 and 182 are converted above. Row 226 `dataRecordsKeepRuleOfZero` (dataunittest) stays: a one-class case cannot replace its scan of every `app/data` header.
- D (14): duplicates named in the inventory and kept, because this slice does not remove them: rows 23, 24, 68, 105, 107, 132, 135, 136, 169, 176, 197, 201, 216 and 223. Two of them (135 and 223) are partly duplicated and also hold text checks, so each needs a judgment when removed.
- S (6): rows 2, 3, 21, 53 and 54 are forwarding stubs that only check that a qmlsmoketest case still exists; row 156 pins the line count of `main()`.
- B2 (45) need a fixture or fake (for example LayoutManager, LayoutsController, ClonedView, UniversalSettings). B3 (45) need a Corona, a view, a display or a session.
- The hinted names were checked. `pointerWindowTrackerRemovesDestroyedWindows` (row 29) and `positionerPlasmaPanelGeometryIntersectionGuardsEmptyList` (row 131) are B3: the code sits in `View` and under the Corona window manager. `layoutManagerRestoreInvalidAppletCleanupPreservesValidIds` (row 122) and `layoutsSaveHandsSynchronizerTableAfterRename` (row 196) are B2. None of the four was converted.
- Two source-text cases look like B1 but are not: `contextMenuHandlesIncompleteDbusData` (row 175) decodes a DBus reply into private state, and `panelShadowRoundnessRejectsIncompleteThemeAssets` (row 209) calls a private updater on a KSvg theme.

## Contract count

- Contract functions: 229 before, 224 after (`grep -c '^    void [A-Za-z0-9_]*();'`).
- QVERIFY/QCOMPARE lines: 1937 before, 1922 after (`grep -c 'QVERIFY\|QCOMPARE'`). AGENTS.md rule 6 expects more than 170 source-contract checks; the count stays above it.
- No file other than the inventory names a removed contract (`git grep`).

## Evidence: commands and exit status

Run from the worktree root, with `build/modernization/gcc-debug` and `build/modernization/clang-debug` as build trees.

1. Baseline, before any edit: `cmake --preset gcc-debug && cmake --build --preset gcc-debug --parallel 8 && ctest --preset gcc-debug --output-on-failure`. Exit 0. `100% tests passed, 0 tests failed out of 61`. `warning:` lines in the build log: 0.
2. `python3 docs/modernization-results/R8a-tools/test_list_contracts.py -v`. Exit 0: `Ran 9 tests`, `OK`.
3. `python3 docs/modernization-results/R8a-tools/list_contracts.py autotests/sourcecontracttest.cpp > contracts-before.md`. Exit 0; 229 rows. A second run is byte-identical (`cmp`, exit 0).
4. `python3 docs/modernization-results/R8a-tools/gen_inventory.py docs/modernization-results/R8a-tools/contracts-before.md docs/modernization-results/R8a-tools/classes.tsv docs/modernization-results/R8a-inventory.md`. First run exit 1 (one reason had 16 words); the reason was shortened. Final run exit 0 and `cmp` against the committed inventory: identical.
5. Per conversion, with `<t>` the target and the case name run directly under `QT_QPA_PLATFORM=offscreen`:
   - `cmake --build build/modernization/gcc-debug --target <t> --parallel 8`: exit 0, 0 `warning:` lines.
   - The new case: `PASS`.
   - `ctest --test-dir build/modernization/gcc-debug -R '^<t>$'`: exit 0.
   - Mutant build and `ctest`: exit 0 and exit 8 (failure), as listed under each conversion. Restore, rebuild, `ctest`: exit 0.
   - `ctest -R '^sourcecontracttest$'` after each removal: exit 0.
6. Full GCC Debug: `cmake --build build/modernization/gcc-debug --parallel 8 && ctest --preset gcc-debug --output-on-failure`. Exit 0. `100% tests passed, 0 tests failed out of 61`, including `sourcecontracttest`, `wmunittest`, `corecomponentstest` and `indicatorunittest`. `warning:` lines: 0.
7. Clang Debug (compiler `/usr/lib/llvm/22/bin/clang++`, fresh configure): `cmake --preset clang-debug && cmake --build --preset clang-debug --parallel 8 && ctest --preset clang-debug --output-on-failure`. Exit 0. `100% tests passed, 0 tests failed out of 61`, including `sourcecontracttest`, `wmunittest`, `corecomponentstest` and `indicatorunittest`. `warning:` lines in the build log: 0.

## Test deltas

- CTest: 61 registered, 61 run, 61 passed, 0 skipped, 0 failed, before and after.
- Source-contract functions: 229 to 224 (5 removed).
- New behavioural cases: 5 (`wmunittest` +1, `corecomponentstest` +3, `indicatorunittest` +1).

## Warnings

- GCC Debug: 0 `warning:` lines and no error in the full build, both at baseline and after the conversions.
- Clang Debug: 0 `warning:` lines in the full build and no compiler error. Four log lines match ` error:`, but they are QML `console.warn` text (`ContextMenu component load error:`) echoed by the QML lint output, not diagnostics.
- qmllint: the build also prints `Warning:` lines from the QML lint step, 5491 in the baseline GCC log, 5490 in the final GCC log and 5491 in the Clang log. They are QML lint messages, not compiler warnings, the same before and after this slice, and this branch changes no QML. They are outside the GCC/Clang gate.

## Pending and unavailable checks

- Debian 13 verification: deferred by the user until the refactor is complete.
- clazy (49 checks) and the deep qmllint gate are CI-only and were not run here. The new cases follow clazy's rules by hand: no lambdas, no container iteration, no `clazy:exclude`.
- Desktop checks: none. No dock was started and no user session was touched. The attached-screen branches of ScreenPool (C2, C3) and the named primary-screen sentinel (C5) need a live desktop.
- `scripts/test-logout-simulation.py`: not run. No runtime path changed.
- Editor diagnostics: the IDE's clang checker has no compile flags and reported unknown types in the touched test files. The compiler builds report 0 warnings and every test passes, so the editor output is not used as evidence.
- Hand-over of commit approval: the branch is committed as requested. It is not pushed and not merged.

## Next exact action

In a follow-up slice, remove the D contract `coronaTeardownSkipsFactoryFreedByQuitCleanup` (row 24) only after confirming that `teardowntest` `deleteNowSkipsServiceFreedByQuitCleanup` fails when the skip in `app/session/teardown.h` is removed in a scratch edit; then repeat that check for the other D rows, starting with row 68 (`autostartPreservesEntryFields`, covered by `autostartlogictest`).
