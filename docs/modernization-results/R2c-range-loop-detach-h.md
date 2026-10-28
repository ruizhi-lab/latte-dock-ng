# R2c-range-loop-detach-h: Range Loops Iterate a Const View Where the Body Does Not Modify the Container

Status: Pending validation. Part of [R2c](R2c.md) fix batch 3, group H: the
second group of the `range-loop-detach` slice, 20 files and 65 of its 274
findings. Date: 2026-10-29. Branch: `claude/r2c-batch-h-065380`. Start HEAD:
`95e342f72` (`main`). Implementation commit: the commit that adds this report.
Not pushed. The gate does not yet include `range-loop-detach`; the main session
adds it after all groups are merged.

## Defect

clazy 1.17 reports "c++11 range-loop might detach Qt container" for a range-based
`for` over a non-const Qt container. The loop calls the non-const `begin()`, which
copies the container when its data is shared with another owner. The copy is an
allocation plus a copy of every element; no functional defect or symptom is known.
The 65 findings in these 20 files are 23 loops over a container the loop can
iterate as const, and 42 loops whose range is a call that returns a container by
value.

## Fix

- **23 loops wrapped.** Each `for (... : container)` became
  `for (... : std::as_const(container))`. No body in this group modifies the
  container it iterates (table below), and every loop variable is a `const`
  reference or a copy, so the wrapped loop compiles and iterates the same
  elements in the same order.
- **42 loops not wrapped.** Their range is a call that returns a container by
  value. `std::as_const` cannot bind a temporary (its rvalue overload is
  deleted), and the brief says to report these rather than hoist them. See
  "Left unchanged".
- **0 loops left unchanged for mutation.** None of the 65 flagged loops binds a
  non-const reference, and none of the 23 bodies writes to the container it
  iterates.
- **Diff rule.** The only changes are the wraps. The brief's wrap-only check
  (`only-wrap.py`, a scratch script, not committed) prints nothing, and `git diff --stat`
  shows 7 files with 23 insertions and 23 deletions. No include was added; the
  container build compiled all seven files without one.

### Changed loops

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

## Left unchanged

### Range is a call returning by value (42): the main session decides

`std::as_const` cannot bind a temporary, so these loops are reported, not
wrapped, as the brief requires. Two options for the main session, neither applied
here:

1. Hoist the call into a `const` local and iterate it, for example
   `const auto actions = desktopMenu->actions();` then
   `for (auto action : std::as_const(actions))`. The loop body, order and elements
   are unchanged. The hoist is not a wrap, so it needs its own approval. When the
   returned copy shares data with its owner, the non-const `begin()` detaches it,
   and a const local does not. Whether a given call shares data depends on its
   implementation, so the main session should check each call.
2. For a list the function builds fresh (`QDir::entryList()`, `QString::split()`,
   `KConfigGroup::keyList()`), the returned list is not shared and the non-const
   `begin()` does not copy it. A reviewed `// clazy:exclude=range-loop-detach` with
   that reason is then an option.

| File | Lines | Range expression |
| --- | --- | --- |
| `app/shortcuts/globalshortcuts.cpp` | 495 | `currentLayouts()` (`LayoutsManager`, returns `QList<CentralLayout *>`) |
| `app/knscompat.cpp` | 239, 304, 478, 590, 596, 609, 620, 660 | `QString::split()`, `systemQmlBaseCandidates()`, six `QDir::entryList()` calls |
| `app/declarativeimports/contextmenulayerquickitem.cpp` | 257, 286, 395, 418, 445, 519, 648 | `applets()`, `childItems()` (twice), `actions()` (twice), `contextualActions()`, `containments()` |
| `app/layouts/syncedlaunchers.cpp` | 158, 178, 198, 218, 238, 258 | `clients(lName, senderId, group, launcherGroupId)` (six calls) |
| `app/screenpool.cpp` | 50, 71, 172, 216, 265 | `m_configGroup.keyList()`, `qGuiApp->screens()` (four calls) |
| `app/plasma/extended/screengeometries.cpp` | 126, 200, 227 | `qGuiApp->screens()` |
| `app/settings/settingsdialog/delegates/activitiesdelegate.cpp` | 171, 260, 373 | `button->menu()->actions()` |
| `app/wm/abstractwindowinterface.cpp` | 156, 176, 211 | `qGuiApp->screens()` |
| `app/alternativeshelper.cpp` | 61 | `val.toArray()` (`QJsonArray`) |
| `app/plasma/extended/backgroundcache.cpp` | 136 | `plasmaConfigContainments.groupList()` |
| `app/plasma/extended/screenpool.cpp` | 71 | `m_screensGroup.keyList()` |
| `app/plasma/extended/theme.cpp` | 258 | `reversedPtr->groupList()` |
| `app/shortcuts/shortcutstracker.cpp` | 159 | `latteGroup.keyList()` |
| `app/settings/settingsdialog/tablayoutshandler.cpp` | 479 | `entry.installedFiles()` |

### Mutation (0)

No flagged loop was left unchanged for mutation: all 65 loops bind a `const`
reference or a copy, and the 23 wrapped bodies were read for writes (table above).

### Not flagged, not changed (for the gate's later review)

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

| Command | Result |
| --- | --- |
| Container, `cmake --preset clang-debug`, `cmake --build --preset clang-debug -j4`, `test-clazy.py --inventory range-loop-detach --report`, before the fix (`main` at `95e342f72`) | Exit 0. The build log has no `warning:` line. Inventory: **274 findings in 45 files**; **65 in the 20 owned files**, per file as the brief lists them |
| Container, the same build after the 23 wraps, and `test-clazy.py --inventory range-loop-detach --report` | Exit 0. Incremental build: 15 objects, **0 `warning:` lines**, 0 compiler errors (the log's `error` matches are QML `console.warn` text). Inventory: **251 findings in 39 files**; **42 in the 20 owned files**, the by-value set above |
| Per file, before and after (owned files): globalshortcuts 9 to 1, main 6 to 0, windowstracker 4 to 0, tasktools 2 to 0, factory 1 to 0, schemesmodel 1 to 0, commontools 1 to 0; the other 13 files unchanged | Sum 65 to 42 in the owned files. The tree's other 209 findings are the same set before and after (compared line by line) |
| Wrap check after the edits: the `only-wrap.py` script of the brief, run from the worktree before the commit | No output, exit 0: every changed line is the old line with `std::as_const(...)` added |
| `git diff --stat` | 7 files, 23 insertions, 23 deletions |
| `git clang-format --style=file --diff HEAD` on the 7 changed files | One hunk, a blank line removed in `viewsToHideAreValid()` at `globalshortcuts.cpp:479`. That line is not one of the changed lines, so the hunk is not applied (`formatter.sh` was not run) |
| Full gate, container: `test-clazy.py --build-dir build/modernization/clang-debug --jobs 4` | **Exit 0**: 43 negative fixtures each rejected by their own gated check; 153 production sources with no finding from 43 gated checks |
| Host GCC Debug: `cmake --preset gcc-debug && cmake --build --preset gcc-debug -j4 && ctest --preset gcc-debug --output-on-failure`, on a fresh tree | Exit 0 for configure, build and ctest. Build: 0 `warning:` lines, 0 CMake warnings. CTest: **100% passed, 0 failed out of 58** |
| Host Clang Debug: `cmake --preset clang-debug && cmake --build --preset clang-debug -j4 && ctest --preset clang-debug --output-on-failure`, on a fresh tree | Exit 0 for configure, build and ctest. Build: 0 `warning:` lines, 0 CMake warnings. CTest: **100% passed, 0 failed out of 58** |

Toolchains. Container: image `latte-r2c-clazy:noble` (ID `8e9528e34f50`), Ubuntu
clang 18.1.3 (LLVM 19.1.1), clazy 1.17.1 (KDE Neon build), Qt 6.11.1 (Neon
`qt6-base-dev`). Host: GCC 16.2.1 (Gentoo, `/usr/bin/g++`), Clang 22.1.8 (`/usr/lib/llvm/22/bin/clang++`),
CMake 4.3.4, Qt 6.11.2.

Durable logs are outside the repository in `$HOME/r2c-build-batch-h/evidence/`:
`pre-fix-run.log`, `post-fix-run.log`, the host `host-gcc-debug-*.log`,
`host-clang-debug-*.log` and `host-verify.summary`, and the two inventory reports
`rld.json` (before) and `rld-post.json` (after). The wrap check and the wrap script
were scratch files and are not kept.

Existing autotests compile some of the changed files: `indicator/factory.cpp`,
`settings/detailsdialog/schemesmodel.cpp`, `wm/tasktools.cpp` and
`tools/commontools.cpp` (`autotests/CMakeLists.txt`). Global shortcuts, the window
tracker and `main.cpp` are compiled into no autotest target. The tests therefore
cover the compile and, for those four files, whatever their existing cases reach;
the reach of the wrapped loops was not checked.

## Handoff

- **Batch / slice and selected scope:** R2c fix batch 3, group H:
  `range-loop-detach`, 20 files, 65 findings in the inventory; 23 wrapped, 42
  reported as by-value ranges.
- **Status:** Pending validation. The container gate and both host builds with
  their tests passed; the desktop smoke and CI have not run.
- **Source baseline / start HEAD / implementation HEAD:** `95e342f72` / `95e342f72` /
  the commit that adds this report (one commit: the 7 sources and this report).
- **Branch / existing user edits preserved:** `claude/r2c-batch-h-065380` in its
  own worktree; clean at the start; no other edits.
- **Dependencies and evidence links:** [R2c](R2c.md), the reference slice
  [R2c-range-loop-reference](R2c-range-loop-reference.md) and
  [R2c-container-anti-pattern](R2c-container-anti-pattern.md) (batch C, the same
  kind of work for other checks). Other groups of this slice work on other files
  of the same check.
- **Files and externally observable behavior changed:** 7 production sources, 23
  loop headers: `app/shortcuts/globalshortcuts.cpp`, `app/main.cpp`,
  `app/wm/tracker/windowstracker.cpp`, `app/indicator/factory.cpp`,
  `app/settings/detailsdialog/schemesmodel.cpp`, `app/wm/tasktools.cpp`,
  `app/tools/commontools.cpp`. No observable change is intended: the same
  elements, in the same order, with no copy. This report. Not changed: the gate,
  the fixtures, `R2c.md`, the ledger, and the other 13 owned files.
- **State authority / invalidation / lifetime / fallback constraints:** each
  wrapped loop reads the container that its owner keeps; the table gives the writer
  or the reason none can run. A later edit that inserts into, removes from or
  assigns the container inside one of these loops must copy it first (constraint
  above). The `m_windows` loops of `windowstracker.cpp` assume that the first pass
  of `updateHints(View*)` writes nothing until `cleanupFaultyWindows()` runs after it;
  AGENTS.md rule 7 asks for a note beside that code. This slice adds none, because
  the brief allows only the wrap in the diff; the main session may add one.
- **Toolchain, import environment and artifact identity:** see Toolchains above.
- **Commands executed with exit status and durable log/artifact locations:** see
  Evidence (scratch logs listed there).
- **GCC/Clang Debug/Release warning/error results:** GCC 16.2.1 Debug: 0 warnings;
  Clang 22.1.8 Debug: 0 warnings; 0 CMake warnings on both; container Clang 18
  build: 0 warnings and 0 errors. Release presets were not built.
- **Tests registered / run / passed / skipped / failed and name deltas:** 58
  registered, 58 run, 58 passed on GCC and on Clang Debug, 0 skipped, 0 failed. No
  test added, changed or skipped; no gate name added. (The R2c reports of earlier
  trees count 57; this tree registers 58.)
- **Production behavior vs copied logic vs source-contract evidence:** production
  code is compiled and analysed; the claims about the bodies rest on reading the
  callees (table), not on a test.
- **QML diagnostics added/removed and environment compatibility:** none; no QML
  changed.
- **Negative fixtures or controlled mutation evidence:** none for this check yet:
  `autotests/clazy/range-loop-detach.cpp` is added by the main session with the
  gate entry.
- **Desktop user feedback / log review / clean-quit and coredump evidence:** not
  run. The main session runs the desktop smoke after merging: startup; a Meta key
  hold (`globalshortcuts.cpp`, `showViews`, `hideViewsTimerSlot`); a window moved or
  maximized (`windowstracker.cpp`, `updateHints`); the app launcher; and a dock
  with its context menu (`contextmenulayerquickitem.cpp`, which is not wrapped).
- **Performance:** not measured. The wrap adds no work. It removes the possible
  detach at `begin()` in each of the 23 loops; a detach happens only when the
  container is shared.
- **CI, distribution and Nix checks (passed / failed / unavailable):** CI not run;
  the Clang Debug job runs the gate after the merge. Debian 13 and Nix not run.
- **Unresolved failures, review assumptions and unavailable checks:** 42 by-value
  findings remain in these files (decision above). The wrapped bodies are checked
  by reading, not by a test. Desktop smoke and CI not run.
- **Next exact action:** the main session decides the 42 by-value loops (hoist
  into `const` locals, or a reviewed exclusion), then adds `range-loop-detach` to
  `GATED_CHECKS` with `autotests/clazy/range-loop-detach.cpp` once the tree-wide
  inventory is zero.
- **Commit approval status / push approval status:** committed on
  `claude/r2c-batch-h-065380` as requested; not pushed; `main` untouched.
