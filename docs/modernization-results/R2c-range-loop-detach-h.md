# R2c-range-loop-detach-h: Range Loops Iterate a Const View or a Const Local

Status: Pending validation. Part of [R2c](R2c.md) fix batch 3, group H: the
second group of the `range-loop-detach` slice, 20 files and 65 of its 274
findings. Two commits on `claude/r2c-batch-h-065380`: `d8b07b428` wraps 23 loops
in `std::as_const()`; the second commit (the one that rewrites the other 42 loops
and updates this report) follows the main session's decision. Date: 2026-10-29.
Start HEAD: `95e342f72` (`main`). Not pushed. The gate does not yet include
`range-loop-detach`; the main session adds it after all groups are merged.

## Defect

clazy 1.17 reports "c++11 range-loop might detach Qt container" for a range-based
`for` over a non-const Qt container. The loop calls the non-const `begin()`, which
copies the container when its data is shared with another owner. The copy is an
allocation plus a copy of every element; no functional defect or symptom is known.
The 65 findings in these 20 files are 23 loops over a container the loop can
iterate as const, and 42 loops whose range is a call that returns a container by
value.

## Fix

- **23 loops wrapped (first commit).** Each `for (... : container)` became
  `for (... : std::as_const(container))`. No body in this group modifies the
  container it iterates (table below), and every loop variable is a `const`
  reference or a copy, so the wrapped loop compiles and iterates the same
  elements in the same order.
- **42 loops rewritten (second commit).** Each range is a call that returns a
  container by value, and `std::as_const` cannot bind a temporary (its rvalue
  overload is deleted). Each header now declares a const local that holds the
  call result, in a C++20 range-for init-statement:
  `for (const auto NAME = CALL; DECL : NAME)`. The local is const, so the loop
  uses a const `begin()`; its lifetime is the loop's, as the temporary's was; the
  call runs once, at the same point. The loop variable and the body are unchanged.
  The main session decided on this form instead of a `clazy:exclude`. The table
  is under "Rewritten loops".
- **0 loops left unchanged for mutation.** None of the 65 flagged loops binds a
  non-const reference, and no wrapped or rewritten body writes to the container
  it iterates.
- **Diff rules.** The first commit is wrap-only: the brief's check
  (`only-wrap.py`, a scratch script, not committed) prints nothing, and
  `git diff --stat` shows 23 insertions and 23 deletions. The second commit
  rewrites 42 `for` headers and splits one single-line loop after its header
  (`alternativeshelper.cpp`), and it puts spaces inside three braced lists in
  `knscompat.cpp`; clang-format asked for these on lines the batch changes. A
  script checks that each rewritten header, once the local and those spaces are
  removed, equals the original. No include was added.

### Wrapped loops (first commit, `d8b07b428`)

| File:line | Container | Why the body does not modify it |
| --- | --- | --- |
| `app/shortcuts/globalshortcuts.cpp:353` | local `sortedViews` (copy of `sortedCurrentViews()`) | Reads `isPreferredForShortcuts()` and breaks. No callee receives the local. |
| `app/shortcuts/globalshortcuts.cpp:363` | local `sortedViews` | Calls `activateEntryForView()`, which writes `m_hideViews` and the view's visibility state, never the local. The local is not passed on. |
| `app/shortcuts/globalshortcuts.cpp:377` | local `views` (from `currentViews()`) | Calls `updateBadgeForLatteTask()`, which cannot reach the local. |
| `app/shortcuts/globalshortcuts.cpp:398` | local `sortedViews` | Reads `isPreferredForShortcuts()` and sets a flag. |
| `app/shortcuts/globalshortcuts.cpp:405` | local `sortedViews` | Assigns `viewWithTasks` and breaks. |
| `app/shortcuts/globalshortcuts.cpp:452` | local `viewsWithShortcuts` | Calls `showShortcutBadges()` and appends to `m_hideViews`, another container. |
| `app/shortcuts/globalshortcuts.cpp:475` | member `m_hideViews` | Calls `Synchronizer::latteViewExists()`, a const query, and returns on the first missing view. |
| `app/shortcuts/globalshortcuts.cpp:532` | member `m_hideViews`, in the `initParameters` lambda | Calls `removeBlockHidingEvent()`, `hideShortcutBadges()`, `isSidebar()`, `isHidden()` and `toggleHiddenState()`. No callee reaches a writer of `m_hideViews` synchronously: the writers (`activateEntry()`, `activateEntryForView()`, `showViews()`, `hideViewsTimerSlot()`) run from `QAction::triggered`, `ModifierTracker::metaModifierPressed`, the timer or the D-Bus method `activateLauncherMenu()` (`app/dbus/org.kde.LatteDock.xml`). The clear at line 542 follows the loop. |
| `app/main.cpp:304` | local `layouts` | Prints each entry. |
| `app/main.cpp:322` | local `templates` | Prints each entry. |
| `app/main.cpp:340` | local `templates` | Prints each entry. |
| `app/main.cpp:768` | static `s_userLocalQmlPaths` | Calls `QQmlEngine::addImportPath()`, which changes only the engine's list. The static is written only by `collectUserLocalPaths()`. |
| `app/main.cpp:813` | local `qmlCandidates` | Appends to `s_userLocalQmlPaths`, another container, and tests each file. |
| `app/main.cpp:845` | static `s_userLocalPluginPaths` | Calls `QCoreApplication::addLibraryPath()`, which changes Qt's search path only. |
| `app/wm/tracker/windowstracker.cpp:245` | local `orphanedLayouts` | Removes from `m_layouts`, another container. |
| `app/wm/tracker/windowstracker.cpp:1006` | member `m_windows` (a `QMap`), first pass of `updateHints(View*)` | Calls only queries: `isShowingDesktop()`, `inCurrentDesktopActivity()`, `hasBlockedTracking()`, the view helpers (`isActiveInViewScreen()` and `isMaximizedInViewScreen()` index `m_views`, not `m_windows`) and the touching tests. The one writer in the function, `cleanupFaultyWindows()`, runs after the loop. |
| `app/wm/tracker/windowstracker.cpp:1070` | member `m_windows`, second pass of `updateHints(View*)` | Calls `inCurrentDesktopActivity()`, `hasBlockedTracking()` and `isTouchingView()`. The `m_windows[activeWinId]` before the loop runs before iteration starts. |
| `app/wm/tracker/windowstracker.cpp:1152` | member `m_windows`, `updateHints(GenericLayout*)` | Calls `isShowingDesktop()`, `inCurrentDesktopActivity()`, `hasBlockedTracking()` and `isActive()`. `cleanupFaultyWindows()` runs after the loop. |
| `app/indicator/factory.cpp:77` | member `m_mainPaths` | Calls `KDirWatch::addDir()`. The only writes to `m_mainPaths` are at lines 69 and 72, before the loop. |
| `app/settings/detailsdialog/schemesmodel.cpp:56` | local `standardPaths` | Lists the directory, appends to `registeredSchemes` and calls `insertSchemeInList()`. None of them reaches `standardPaths`. |
| `app/wm/tasktools.cpp:92` | local `candidates` | Compares names and returns. |
| `app/wm/tasktools.cpp:150` | local `candidates` | Assigns `service` and breaks. |
| `app/tools/commontools.cpp:102` | local `paths` | Returns the first path that exists. |

Constraint for later edits: a body added to one of these loops that inserts,
removes, or assigns the container it walks must copy the container first, as the
snapshot loops of `windowstracker.cpp` (`cleanupFaultyWindows()`,
`updateExtraViewHints()`) do. Without a copy, the const view keeps walking the
old data and the body's writes are invisible to it.

## Rewritten loops (second commit)

Decision: the main session. Its brief asked for the form below instead of
reporting these loops or excluding them with `clazy:exclude`. Each header now
reads `for (const auto NAME = CALL; DECL : NAME)`; `DECL` and the body are the
same as before. Each name is the noun of the call's result, and no loop body uses
its name, so no body sees a different binding (checked for every loop). None of
the 42 calls returns a reference, so no loop was left out for that reason.

The local keeps the call's result for the length of the loop, so the iteration is
over the same elements as before. A result that shares its data with the object
it came from shares that data through Qt's implicit sharing: a body that changes
the object detaches the object, not the local, and the loop keeps walking the
elements the result held. A list the call builds fresh is not shared, and
nothing changes for it.

| File:line | Call | Local |
| --- | --- | --- |
| `app/shortcuts/globalshortcuts.cpp:495` | `m_corona->layoutsManager()->currentLayouts()` | `layouts` |
| `app/knscompat.cpp:239` | `paths.split(QDir::listSeparator(), Qt::SkipEmptyParts)` | `parts` |
| `app/knscompat.cpp:304` | `systemQmlBaseCandidates()` | `candidates` |
| `app/knscompat.cpp:478` | `srcDir.entryList(QDir::Files \| QDir::Dirs \| QDir::NoDotAndDotDot)` | `entries` |
| `app/knscompat.cpp:590` | `sysDir.entryList({QStringLiteral("*.qml"), QStringLiteral("*.qmltypes")}, QDir::Files)` | `entries` |
| `app/knscompat.cpp:596` | `sysPriv.entryList(privFilter, QDir::Files)` | `entries` |
| `app/knscompat.cpp:609` | `nsDir.entryList({QStringLiteral("*.qml"), QStringLiteral("*.qmltypes")}, QDir::Files)` | `entries` |
| `app/knscompat.cpp:620` | `nsPriv.entryList(QDir::Filters(QDir::Files \| QDir::Dirs \| QDir::NoDotAndDotDot))` | `privEntries` (the body declares `entries`) |
| `app/knscompat.cpp:660` | `scDir.entryList({QStringLiteral("*.qml"), QStringLiteral("*.qmltypes")}, QDir::Files)` | `entries` |
| `app/declarativeimports/contextmenulayerquickitem.cpp:257` | `m_latteView->containment()->applets()` | `applets` |
| `app/declarativeimports/contextmenulayerquickitem.cpp:286` | `p->childItems()` | `childItems` |
| `app/declarativeimports/contextmenulayerquickitem.cpp:395` | `desktopMenu->actions()` | `actions` |
| `app/declarativeimports/contextmenulayerquickitem.cpp:418` | `m_latteView->contentItem()->childItems()` | `childItems` |
| `app/declarativeimports/contextmenulayerquickitem.cpp:445` | `applet->contextualActions()` | `contextualActions` |
| `app/declarativeimports/contextmenulayerquickitem.cpp:519` | `containmentAction->menu()->actions()` | `actions` |
| `app/declarativeimports/contextmenulayerquickitem.cpp:648` | `m_latteView->corona()->containments()` | `containments` |
| `app/layouts/syncedlaunchers.cpp:158` | `clients(lName, senderId, group, launcherGroupId)` | `matchingClients` |
| `app/layouts/syncedlaunchers.cpp:178` | `clients(lName, senderId, group, launcherGroupId)` | `matchingClients` |
| `app/layouts/syncedlaunchers.cpp:198` | `clients(lName, senderId, group, launcherGroupId)` | `matchingClients` |
| `app/layouts/syncedlaunchers.cpp:218` | `clients(lName, senderId, group, launcherGroupId)` | `matchingClients` |
| `app/layouts/syncedlaunchers.cpp:238` | `clients(lName, senderId, group, launcherGroupId)` | `matchingClients` |
| `app/layouts/syncedlaunchers.cpp:258` | `clients(lName, senderId, group, launcherGroupId)` | `matchingClients` |
| `app/screenpool.cpp:50` | `m_configGroup.keyList()` | `keys` |
| `app/screenpool.cpp:71` | `qGuiApp->screens()` | `screens` |
| `app/screenpool.cpp:172` | `qGuiApp->screens()` | `screens` |
| `app/screenpool.cpp:216` | `qGuiApp->screens()` | `screens` |
| `app/screenpool.cpp:265` | `qGuiApp->screens()` | `screens` |
| `app/plasma/extended/screengeometries.cpp:126` | `qGuiApp->screens()` | `screens` |
| `app/plasma/extended/screengeometries.cpp:200` | `qGuiApp->screens()` | `screens` |
| `app/plasma/extended/screengeometries.cpp:227` | `qGuiApp->screens()` | `screens` |
| `app/settings/settingsdialog/delegates/activitiesdelegate.cpp:171` | `button->menu()->actions()` | `actions` |
| `app/settings/settingsdialog/delegates/activitiesdelegate.cpp:260` | `button->menu()->actions()` | `actions` |
| `app/settings/settingsdialog/delegates/activitiesdelegate.cpp:373` | `button->menu()->actions()` | `actions` |
| `app/wm/abstractwindowinterface.cpp:156` | `qGuiApp->screens()` | `screens` |
| `app/wm/abstractwindowinterface.cpp:176` | `qGuiApp->screens()` | `screens` |
| `app/wm/abstractwindowinterface.cpp:211` | `qGuiApp->screens()` | `screens` |
| `app/alternativeshelper.cpp:61` | `val.toArray()` | `jsonArray` |
| `app/plasma/extended/backgroundcache.cpp:136` | `plasmaConfigContainments.groupList()` | `groupNames` |
| `app/plasma/extended/screenpool.cpp:71` | `m_screensGroup.keyList()` | `keys` |
| `app/plasma/extended/theme.cpp:258` | `reversedPtr->groupList()` | `groupNames` |
| `app/shortcuts/shortcutstracker.cpp:159` | `latteGroup.keyList()` | `keys` |
| `app/settings/settingsdialog/tablayoutshandler.cpp:479` | `entry.installedFiles()` | `installedFiles` |

## Not flagged, not changed (for the gate's later review)

- `app/wm/tasktools.cpp:291` iterates the `services` reference parameter, and its
  body calls `services.prepend(service)` before returning. clazy 1.17 does not
  report it, so it is not in this slice. It is a mutation during iteration, and
  the main session may want to review it.
- `app/plasma/extended/screengeometries.cpp:272`, `app/plasma/extended/theme.cpp:374`
  and `app/shortcuts/shortcutstracker.cpp:139` bind a non-const reference loop
  variable. clazy 1.17 does not report them, and `std::as_const` cannot bind a
  non-const reference, so they are untouched.

## Evidence

Container commands are those of [R2c](R2c.md#reproducing-the-environment) with
the worktree mounted read-only, a build tree of its own
(`$HOME/r2c-build-batch-h`) and `-j4`/`--jobs 4`. The worktree needs an existing
`build/` directory as the mount point for the build tree; it is git-ignored
(`build*/`). The first attempt failed in Docker before any build ran
(`read-only file system` while creating `/src/build`), and `mkdir build` in the
worktree fixed it.

First commit (`d8b07b428`, 23 wraps):

| Command | Result |
| --- | --- |
| Container, `cmake --preset clang-debug`, `cmake --build --preset clang-debug -j4`, `test-clazy.py --inventory range-loop-detach --report`, before the fix (`main` at `95e342f72`) | Exit 0. The build log has no `warning:` line. Inventory: **274 findings in 45 files**; **65 in the 20 owned files**, per file as the brief lists them |
| Container, the same build after the 23 wraps, and `test-clazy.py --inventory range-loop-detach --report` | Exit 0. Incremental build: 15 objects, **0 `warning:` lines**, 0 compiler errors (the log's `error` matches are QML `console.warn` text). Inventory: **251 findings in 39 files**; **42 in the 20 owned files**, the by-value set |
| Per file, before and after (owned files): globalshortcuts 9 to 1, main 6 to 0, windowstracker 4 to 0, tasktools 2 to 0, factory 1 to 0, schemesmodel 1 to 0, commontools 1 to 0; the other 13 files unchanged | Sum 65 to 42 in the owned files. The tree's other 209 findings are the same set before and after (compared line by line) |
| Wrap check: the `only-wrap.py` script of the brief, run on the committed diff against `95e342f72` | No output, exit 0; 23 changed-line pairs checked, each the old line with `std::as_const(...)` added |
| `git diff --stat` | 7 files, 23 insertions, 23 deletions |
| `git clang-format --style=file --diff HEAD` on the 7 changed files | One hunk, a blank line removed in `viewsToHideAreValid()` at `globalshortcuts.cpp:479`. That line is not one of the changed lines, so the hunk is not applied (`formatter.sh` was not run) |
| Full gate, container: `test-clazy.py --build-dir build/modernization/clang-debug --jobs 4` | **Exit 0**: 43 negative fixtures each rejected by their own gated check; 153 production sources with no finding from 43 gated checks |
| Host GCC Debug (first commit): `cmake --preset gcc-debug && cmake --build --preset gcc-debug -j4 && ctest --preset gcc-debug --output-on-failure`, on a fresh tree | Exit 0 for configure, build and ctest. Build: 0 `warning:` lines, 0 CMake warnings. CTest: **100% passed, 0 failed out of 58** |
| Host Clang Debug (first commit): `cmake --preset clang-debug && cmake --build --preset clang-debug -j4 && ctest --preset clang-debug --output-on-failure`, on a fresh tree | Exit 0 for configure, build and ctest. Build: 0 `warning:` lines, 0 CMake warnings. CTest: **100% passed, 0 failed out of 58** |

Second commit (the 42 rewritten loops):

| Command | Result |
| --- | --- |
| Header check: every changed pair of lines in `git diff -U0`, the new header with the local and the spaces inside braces removed, compared with the old line | **42 verified, 0 problems** (43 insertions = 42 headers and the one split line; 42 deletions) |
| Container, `cmake --build --preset clang-debug -j4` after the rewrites (incremental) | Exit 0. 21 objects rebuilt, **0 `warning:` lines**, 0 compiler errors (the four `error` matches are the QML `console.warn` text already in the log) |
| Container, `test-clazy.py --inventory range-loop-detach --report` after the rewrites | Exit 0. **209 findings in the tree, 0 in the 20 owned files** (42 before this commit). The 209 are in files of the other groups |
| Container, full gate: `test-clazy.py --build-dir build/modernization/clang-debug --jobs 4` | **Exit 0**: 43 negative fixtures rejected by their own gated checks; 153 production sources with no finding from 43 gated checks |
| `git clang-format --style=file --diff HEAD` on the 14 changed files | Applied: the split of the `alternativeshelper.cpp` loop and the spaces inside the braced lists of three `knscompat.cpp` headers (changed lines). Not applied, because they change lines this batch did not touch: `contextmenulayerquickitem.cpp:285` (the `std::function` line) and `:295` (`if (f) return f;`), and the Allman brace line of `screengeometries.cpp:127`, which the loop keeps as it was |
| Host GCC Debug (second commit), the same commands as above on the existing build tree (incremental) | Exit 0 for configure, build and ctest. Incremental build: 21 objects rebuilt, **0 `warning:` lines**, 0 CMake warnings. CTest: **100% passed, 0 failed out of 58** |
| Host Clang Debug (second commit), the same commands as above on the existing build tree (incremental) | Exit 0 for configure, build and ctest. Incremental build: 21 objects rebuilt, **0 `warning:` lines**, 0 CMake warnings. CTest: **100% passed, 0 failed out of 58** |

Toolchains. Container: image `latte-r2c-clazy:noble` (ID `8e9528e34f50`), Ubuntu
clang 18.1.3 (LLVM 19.1.1), clazy 1.17.1 (KDE Neon build), Qt 6.11.1 (Neon
`qt6-base-dev`). Host: GCC 16.2.1 (Gentoo, `/usr/bin/g++`), Clang 22.1.8
(`/usr/lib/llvm/22/bin/clang++`), CMake 4.3.4, Qt 6.11.2.

Durable logs are outside the repository in `$HOME/r2c-build-batch-h/evidence/`:
`pre-fix-run.log` and `post-fix-run.log` (first commit), `second-container-run.log`
(second commit), the host logs `host-*` and `second-host-*` with their summaries,
and the inventory reports `rld.json` (before), `rld-post.json` (after the wraps) and
`rld-second.json` (after the rewrites).

Existing autotests compile some of the changed files: `indicator/factory.cpp`,
`settings/detailsdialog/schemesmodel.cpp`, `wm/tasktools.cpp` and
`tools/commontools.cpp` (`autotests/CMakeLists.txt`). Global shortcuts, the window
tracker and `main.cpp` are compiled into no autotest target. The tests therefore
cover the compile and, for those four files, whatever their existing cases reach;
the reach of the changed loops was not checked.

## Handoff

- **Batch / slice and selected scope:** R2c fix batch 3, group H:
  `range-loop-detach`, 20 files, 65 findings in the inventory. All 65 are fixed in
  these files: 23 wrapped in `std::as_const()` (first commit) and 42 rewritten with
  a const local (second commit, the main session's decision). The tree still has
  209 findings in other groups' files.
- **Status:** Pending validation. The container build and gate pass, and the host
  results are in the Evidence table; the desktop smoke and CI have not run.
- **Source baseline / start HEAD / implementation HEAD:** `95e342f72` / `95e342f72` /
  `d8b07b428` (wraps) and the second commit (rewrites).
- **Branch / existing user edits preserved:** `claude/r2c-batch-h-065380` in its
  own worktree; clean at the start of each commit; no other edits.
- **Dependencies and evidence links:** [R2c](R2c.md), the reference slice
  [R2c-range-loop-reference](R2c-range-loop-reference.md) and
  [R2c-container-anti-pattern](R2c-container-anti-pattern.md) (batch C, the same
  kind of work for other checks). Other groups of this slice work on other files
  of the same check.
- **Files and externally observable behavior changed:** all 20 owned production
  sources. The first commit changes 7 (23 wrapped loops); the second commit changes
  14 (42 loop headers, one split line in `alternativeshelper.cpp`, braced-list
  spacing in three `knscompat.cpp` headers). `globalshortcuts.cpp` is in both. No
  observable change is intended: the same elements, in the same order, from the
  same snapshot as before. This report. Not changed: the gate, the fixtures,
  `R2c.md`, the ledger.
- **State authority / invalidation / lifetime / fallback constraints:** each
  wrapped loop reads the container that its owner keeps; the table gives the writer
  or the reason none can run. A later edit that inserts into, removes from or
  assigns the container inside one of these loops must copy it first (constraint
  above). The rewritten loops iterate a snapshot: a body that needs to see changes
  that it makes to the object the call came from must iterate that object, not the
  local. The `m_windows` loops of `windowstracker.cpp` assume that the first pass of
  `updateHints(View*)` writes nothing until `cleanupFaultyWindows()` runs after it;
  AGENTS.md rule 7 asks for a note beside that code. This batch adds none, because
  the brief and the main session's form changed only the loop headers; the main
  session may add one.
- **Toolchain, import environment and artifact identity:** see Toolchains above.
- **Commands executed with exit status and durable log/artifact locations:** see
  Evidence and the logs listed there.
- **GCC/Clang Debug/Release warning/error results:** first commit: GCC 16.2.1 and
  Clang 22.1.8 Debug, 0 warnings each, 0 CMake warnings; second commit: container
  Clang 18 incremental build 0 warnings, 0 errors; host GCC 16.2.1 and Clang 22.1.8 Debug, 0 warnings each. Release presets
  were not built.
- **Tests registered / run / passed / skipped / failed and name deltas:** 58
  registered on this tree; first commit: 58 run, 58 passed on GCC and on Clang Debug;
  second commit: 58 run, 58 passed on GCC and on Clang Debug. No test added, changed or skipped; no gate name added.
  (The R2c reports of earlier trees count 57.)
- **Production behavior vs copied logic vs source-contract evidence:** production
  code is compiled and analysed; the claims about the bodies rest on reading the
  callees (table) and the rewritten loops on the snapshot argument above, not on a
  test.
- **QML diagnostics added/removed and environment compatibility:** none; no QML
  changed.
- **Negative fixtures or controlled mutation evidence:** none for this check yet:
  `autotests/clazy/range-loop-detach.cpp` is added by the main session with the
  gate entry.
- **Desktop user feedback / log review / clean-quit and coredump evidence:** not
  run. The main session's desktop smoke after merging should cover the loops that
  now run through a local: startup (`knscompat.cpp`, `screenpool.cpp`,
  `main.cpp`, `shortcutstracker.cpp`); a Meta key hold (`globalshortcuts.cpp`,
  `showViews`, `hideViewsTimerSlot`); a window moved or maximized
  (`windowstracker.cpp`, `updateHints`); the dock context menu, with its applet and
  child-item loops (`contextmenulayerquickitem.cpp`); the app launcher; the task
  manager launchers, adding and removing (`syncedlaunchers.cpp`); the activities
  menus (`activitiesdelegate.cpp`); a layout download (`tablayoutshandler.cpp`); and
  a theme or background change (`theme.cpp`, `backgroundcache.cpp`).
- **Performance:** not measured. The rewrite and the wrap add no work; each removes
  the possible detach at `begin()`. A detach happens only when the container is
  shared.
- **CI, distribution and Nix checks (passed / failed / unavailable):** CI not run;
  the Clang Debug job runs the gate after the merge. Debian 13 and Nix not run.
- **Unresolved failures, review assumptions and unavailable checks:** no failure in
  the checks run. The wrapped bodies are checked by reading, and the rewritten loops
  rest on the snapshot argument and the name checks, not on a test. Host builds
  were incremental on the existing build trees; the warning count of the unchanged
  files comes from the first commit's full builds. Desktop smoke and CI not run.
- **Next exact action:** the main session merges the groups. `range-loop-detach`
  joins `GATED_CHECKS` with `autotests/clazy/range-loop-detach.cpp` once the
  tree-wide inventory is zero (209 findings remain in the other groups' files).
- **Commit approval status / push approval status:** two commits on
  `claude/r2c-batch-h-065380` (`d8b07b428` and the second commit); not pushed;
  `main` untouched.
