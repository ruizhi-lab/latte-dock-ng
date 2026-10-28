# R2c-container-anti-pattern: Ask the Container, Do Not Copy It Into a List First

Status: Pending validation. Merged into `main` in `181b7f871`; the checks on
`main` and the host desktop smoke passed ([R2c fix batch 2](R2c.md#fix-batch-2)). The CI run
waits for a push.
Date: 2026-10-28. Branch: `claude/r2c-batch-c-bca59f`. Start HEAD: `0ca056cc7`
(the `R2c-detaching-temporary` commit on top of `cec126a31` and `eba91a3e6`).
Implementation commit: the commit that adds this report. Part of [R2c](R2c.md);
the third slice of batch C (fix batch 2).

## Defect

38 locations in 13 files built a temporary list with `keys()` or `values()` of a
`QHash` or `QMap` and used it for one thing the container can answer itself: 29
looped over it, 6 asked `keys().contains(x)`, 2 asked `values().contains(x)` and
1 took `keys()[0]`. Each call allocates a list and copies every key or value
(`QString` keys for most of them), and clazy reports "allocating an unneeded
temporary container". There is no functional defect and no known symptom; the
finding is a cost finding, small because the containers hold a handful of
entries, in code that runs on window, layout and applet changes.

## Fix

Each site now asks the container directly. Line numbers are those of the
inventory at `eba91a3e6`; the fixed tree has moved a few of them.

| Form | Replacement | Sites |
| --- | --- | ---: |
| `keys().contains(k)` | `contains(k)` | 6 |
| `values().contains(v)` | `std::find(c.cbegin(), c.cend(), v) != c.cend()`: the hash is keyed by something else, so the question is about its values | 2 |
| `keys()[0]` | `constBegin().key()`, guarded by the same `count() > 0` test as before | 1 |
| loop over `keys()` whose body also reads the value of each key | `for (const auto &[key, value] : std::as_const(c).asKeyValueRange())` (Qt 6.4 and later) | 13 |
| loop whose body only needs the values | `for (const auto &value : c)` over the container, `std::as_const` for a non-const member | 9 |
| loop whose body only needs the keys | key iterators, `for (auto it = c.keyBegin(); it != c.keyEnd(); ++it)` | 2 |
| loop that needs a mutable value, because the callee takes `Data::View &` | mutable iterators, `it.key()` and `it.value()` | 1 |
| loop whose body changes or may change the container | an explicit named snapshot of the keys with a comment | 4 |

The forms add up to the 38 reported locations. Two more tests of the same shape
that clazy 1.17 does not report, `m_hintsCache[imageFile].keys().contains(location)`
in `brightnessForFile` and `busyForFile`, got the same `contains()` fix as their
neighbours. `git clang-format` over the changed lines suggested joining three
multi-line conditions into single lines (one in `modifiertracker.cpp`, two in
`windowstracker.cpp`); they keep the layout they had, because the suggestion only
reflects the difference between `.clang-format` and the older layout. Per site:

| Site (`eba91a3e6`) | Was | Now and why it is safe |
| --- | --- | --- |
| `genericlayout.cpp:357` `onLastConfigViewChangedFrom` | `m_latteViews.values().contains(view)` | `std::find` over the values of `m_latteViews`; the hash maps containments to views and the question is about a view |
| `genericlayout.cpp:1161` `hasLatteView` | `m_latteViews.keys().contains(containment)` | `m_latteViews.contains(containment)`; the argument converts to the `const Containment *` key as before |
| `genericlayout.cpp:1235-1236` `mapContainsId` | nested loops over `keys()` of the map and of each screen's hash, reading `(*map)[scr][edge]` | nested loops over the values; the keys were only used to look the lists up again. Read-only, the map is `const` |
| `genericlayout.cpp:1248-1249` `mapScreenName` | same nesting | `asKeyValueRange()` on the map for the screen name, values of the inner hash; returns the first match in the same order |
| `layouts/importer.cpp:806` `checkRepairMultipleLayoutsLinkedFile` | `linkedLayoutContainmentGroups.keys()` with `[layoutName]` lookups | `asKeyValueRange()` over the local hash; the body reads the container ids from the binding and never touches the hash. `updatedLayouts` keeps its order |
| `layouts/storage.cpp:972` `exportTemplate` | `subInfo.keys()` | key iterators over the local hash; the body does not touch it |
| `layouts/synchronizer.cpp:97` `isAssigned` | `keys()` then `m_assignedLayouts[activityid].contains(...)` | loop over the values; const method, any match returns `true` |
| `layouts/synchronizer.cpp:139` `freeActivities` | `m_assignedLayouts.keys()` | key iterators; the body removes from the local `frees` |
| `plasma/extended/backgroundcache.cpp:191` `reload` | `updates.keys()` then `updates[activity]` | `asKeyValueRange()` over the local `updates`; emitting `backgroundChanged` cannot reach a local |
| `backgroundcache.cpp:399` `updateImageCalculations` | `m_hintsCache.keys().contains(imageFile)` | `contains(imageFile)` |
| `backgroundcache.cpp:416, 429` `brightnessForFile` and `438, 451` `busyForFile` | same | `contains(...)`. The two inner `m_hintsCache[imageFile].keys().contains(location)` tests at `:417` and `:439`, which clazy 1.17 does not report, got the same fix |
| `plasma/extended/screenpool.cpp:93` `load` | `m_connectorForId.keys()` for a debug print | `asKeyValueRange()`; the loop only prints |
| `settings/settingsdialog/layoutscontroller.cpp:1014` `save` | `activeLayoutsToRename.keys()` with two `[newLayoutName]` lookups | `asKeyValueRange()` over the local hash; `renameLayout()` cannot reach a local |
| `layoutscontroller.cpp:1048` `save` | `activeLayoutsToRename.keys()[0]` | `activeLayoutsToRename.constBegin().key()`; the ternary evaluates it only when `count() > 0` |
| `settings/universalsettings.cpp:506` `saveScalesConfig` | `m_screenScales.keys()` with two lookups | `asKeyValueRange()`; the body writes to a `KConfigGroup`, not to the hash |
| `settings/viewsdialog/viewscontroller.cpp:940` `save` | `cuttedpastedviews.keys()` with four lookups | loop over the values of the local hash; the key was only used for lookups |
| `viewscontroller.cpp:965` `save` | `cuttedpastedactiveviews.keys()` | loop over the values, copying each into `pastedactiveview` as before; the body writes `newviewsresponses`, another hash |
| `viewscontroller.cpp:1002` `save` | `newviewsresponses.keys()` | mutable iterators: `Model::Views::setOriginalView()` takes a non-const `Data::View &` (it only copies it), so a const range would not compile and a header change is outside this slice |
| `shortcuts/modifiertracker.cpp:90` `noModifierPressed` | `m_pressed.keys()` then `m_pressed[modifier]` | loop over the values; returns on the first pressed one |
| `modifiertracker.cpp:119` `singleModifierPressed` | same | `asKeyValueRange()` |
| `view/containmentinterface.cpp:2012` `appletIsExpanded` | `m_expandedAppletIds.values().contains(id)` | `std::find` over the values: the hash maps expandable items to applet ids and the question is about an applet id |
| `containmentinterface.cpp:2172` `updateAppletDelayedConfiguration` | `m_appletData.keys()` | **snapshot kept**: named `ids` with a comment. The body changes the entries it visits, emits `appletConfigurationReady()` and adds a task, and it looks every entry up again by id |
| `wm/tracker/windowstracker.cpp:103` `init` (lambda) | `m_views.keys()` then `m_views[view]` | loop over the values of `m_views`; the body reads the tracked info and writes `m_windows`, another container |
| `windowstracker.cpp:787` `cleanupFaultyWindows` | `m_windows.keys()` | **snapshot kept**: the body removes entries from `m_windows` |
| `windowstracker.cpp:801` `updateScreenGeometries` | `m_views.keys()` with three lookups | `asKeyValueRange()` over `m_views`, using the bound info |
| `windowstracker.cpp:857` `updateAllHints` | `m_views.keys()` with two lookups | `asKeyValueRange()` |
| `windowstracker.cpp:877, 887, 913` `updateLayoutHintsFromViews` | `m_layouts.keys()` and two inner `m_views.keys()` loops | `asKeyValueRange()` over `m_layouts` and `m_views`, using the bound infos |
| `windowstracker.cpp:929, 937` `updateExtraViewHints` | `m_views.keys()` in the outer loop and again in each pass of the inner one | **snapshots kept**: `horViews` and, per outer pass, `verViews`. Both loop bodies re-check `m_views.contains()` for every key, so they are written to tolerate a view disappearing while the hints change |
| `containmentactions/contextmenu/menu.cpp:320` `updateVisibleActions` | `m_actions.keys()` with two `[actionName]` lookups | `asKeyValueRange()`; `setVisible()` does not change `m_actions` |
| `menu.cpp:348` `updateVisibleActions` | `m_actions.keys()` | loop over the `QAction *` values |

## Iteration order and what the loops may change

- **Order.** In Qt 6 `QHash::keys()` and `values()` walk the hash from `begin()`
  to `end()`, so a direct walk visits the same keys and values in the same order,
  and `constBegin().key()` is `keys().first()`. `QMap` is sorted either way.
  Every container involved is a `QHash` or a `QMap`; none is a `QMultiHash` or
  `QMultiMap`, whose `keys()` repeats a key for each value, so no key is visited
  once more or less than before. A probe confirms it (below).
- **The container does not change during a converted loop.** Of the 25 converted
  loops, 7 walk a local hash that no callee can name (`importer.cpp`,
  `storage.cpp`, `BackgroundCache::reload`, `Layouts::save` and the three in
  `Views::save`). 12 walk a member or a parameter whose body calls nothing that
  writes to it: `genericlayout.cpp` (4, read only), `m_assignedLayouts` (2, a
  `QList::contains` and a `removeAll` on a local), `m_pressed` (2, a boolean test),
  `m_connectorForId` (a debug print), `m_screenScales` (a `KConfigGroup` write) and
  `m_actions` (2, `QAction::setVisible()` and `setEnabled()`). The mutators of those
  members are elsewhere: `m_actions` is filled in `restore()` and emptied in the
  destructor, `m_pressed` is written by key events, `m_screenScales` by
  `loadScalesConfig()` and `setScreenScales()`.
- **The windows tracker.** The other 6 walk `m_views` or `m_layouts` and call
  `updateHints()`, `initLayoutHints()` or the `set*()` functions, which emit
  signals. Inside the class `Windows`, the only functions that insert into or
  remove from those two hashes are `addView`, `removeView`, `addRelevantLayout`,
  `updateRelevantLayouts` and `setEnabled`; none of the converted bodies calls
  them, and from outside they run when a `Latte::View` is created or destroyed or
  when the per-view tracker is switched on or off. The signals the bodies emit
  (`existsWindowActiveChanged` and its siblings) are received by the per-view
  trackers and the visibility manager, which re-emit them or change visibility,
  and by QML bindings that read the hints; I found no receiver that creates or
  destroys a view or switches a tracker. This is a reading of the code, not a
  test, and it is the assumption to check in the desktop smoke.
- **Where that cannot be shown** the snapshot stays, with a comment: the four
  loops of the table. A later change that makes a converted body re-entrant must
  copy the keys first, as these four do.
- **`std::as_const`.** A non-const range over a member would detach a shared
  hash. The converted loops over members use `std::as_const` (or a const method),
  so none of them is a `range-loop-detach` finding. That check is not gated yet
  (slice `R2c-range-loop-detach`); its inventory is compared below.

## Behavior check

A probe program, not committed, builds random `QHash<QString, int>`,
`QMap<int, QString>`, `QHash<uint, QString>` and `QHash<int *, int>` of 0 to 299
inserted entries with removals in between, and compares each direct form with the
list it replaces: `asKeyValueRange()`, key iterators, the range-for over values and
`constBegin().key()` against `keys()` and `values()`, `contains()` against
`keys().contains()`, and `std::find` over the values against `values().contains()`
for present and absent integers and for null and non-null pointers.

| Qt | Checks | Failures |
| --- | ---: | ---: |
| 6.11.2 (host) | 128894 | 0 |
| 6.4.2 (Ubuntu 24.04 container, `qt6-base-dev 6.4.2+dfsg-21.1build5`) | 128695 | 0 |

## Evidence

Container commands are those of [R2c](R2c.md#reproducing-the-environment) with
the checkout mounted read-only, a build tree of its own (`$HOME/r2c-build-batch-c`)
and `-j6`/`--jobs 6`, because an idle-CPU measurement runs in the Fedora VM and
a second session builds at the same time.

| Command | Result |
| --- | --- |
| Baseline `test-clazy.py --build-dir build/modernization/clang-debug --jobs 6 --inventory unused-non-trivial-variable,detaching-temporary,container-anti-pattern --report ...` on `eba91a3e6` | Exit 0 in 146 s: `container-anti-pattern` 38 findings in 13 files (the other two checks: 14 and 43) |
| Container `cmake --build ... -j6` after the fix (Clang 18.1.3, Qt 6.11.1, strict warnings) | Exit 0 in 32 s, 13 objects, 0 compiler warnings |
| `test-clazy.py ... --inventory container-anti-pattern` on the fixed tree | Exit 0 in 160 s: **0 findings** in 153 sources |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 6` | **Exit 0** in 198 s: 40 fixtures rejected by their own checks, 153 sources with no finding from 40 checks |
| Same, with the previous versions of the 13 files mounted over the fixed ones | **Exit 1** in 178 s: 38 `[container-anti-pattern]` findings at the baseline positions (6, 1, 1, 2, 6, 1, 2, 1, 3, 2, 2, 9 and 2 in the 13 files), nothing else |
| `test-clazy.py ... --inventory range-loop-detach` on an export of `eba91a3e6` and on the fixed tree | Exit 0 in 90 s and 154 s: **305 findings in 47 files** before, **274 in 45** after. Compared line by line (the flagged source text), 0 findings are new and 31 are gone: loops over a `keys()` temporary, over `updates[activity]` and over `linkedLayoutContainmentGroups[layoutName]` were detach findings too, in 13 files (`windowstracker.cpp` 13 to 4, `viewscontroller.cpp` 4 to 1, `genericlayout.cpp` 28 to 24, `modifiertracker.cpp` 2 to 0, `menu.cpp` 2 to 0, and the others by 1 or 2) |
| Host `cmake --preset gcc-debug`, `cmake --build --preset gcc-debug -j6`, `ctest --preset gcc-debug` (GCC 16.2.1, incremental) | Exit 0 in 64 s, 17 objects rebuilt (13 sources and the 4 test targets that compile `backgroundcache.cpp`, `screenpool.cpp`, `modifiertracker.cpp` and `menu.cpp`), 0 compiler warnings, 57/57 passed |
| Host `cmake --preset clang-debug`, `cmake --build --preset clang-debug -j6`, `ctest --preset clang-debug` (Clang 22.1.8, incremental) | Exit 0 in 79 s, 17 objects rebuilt, 0 compiler warnings, 57/57 passed |
| After the last edit, a comment (the note above `updateScreenGeometries()`): both host presets, the container build and the full gate again | Exit 0 each: 39 s (GCC) and 38 s (Clang) with `windowstracker.cpp` rebuilt, 0 compiler warnings, 57/57 passed; container build 15 s, 1 object, 0 compiler warnings; gate **exit 0** in 115 s with 40 fixtures rejected and 153 sources clean |

Before the edits reached the worktree, the 13 files were compiled with
`-fsyntax-only` and the flags of the real compile database (strict warnings
included) on GCC 16.2.1 and Clang 22.1.8; that run found the one const problem
of `viewscontroller.cpp` described in the table.

The build logs hold thousands of `Warning:` lines from the qmllint step; they
are the known QML warnings tracked by the baseline gate, not compiler output,
and this change touches no QML.

Existing tests run changed production code: `shortcutunittest` calls
`noModifierPressed()` and `singleModifierPressed()` of the real
`ModifierTracker`, and `corecomponentstest` reaches `brightnessForFile()`,
`busyForFile()` and `updateImageCalculations()` of the real `BackgroundCache`
through `brightnessFor()` and `busyFor()`. The windows tracker, the layout
synchronizer, the settings controllers and the context menu have no test that
reaches the changed loops.

## Handoff

- **Batch / slice and selected scope:** R2c fix batch 2, batch C, slice 3 of 3:
  `container-anti-pattern`, 38 locations in 13 files.
- **Status:** Pending validation.
- **Source baseline / start HEAD / implementation HEAD:** `eba91a3e6` /
  `0ca056cc7` / the commit that adds this report.
- **Branch / existing user edits preserved:** `claude/r2c-batch-c-bca59f` in its
  own worktree; the tree was clean at the start of the slice.
- **Dependencies and evidence links:** [R2c](R2c.md),
  [R2c-detaching-temporary](R2c-detaching-temporary.md) and
  [R2c-unused-non-trivial-variable](R2c-unused-non-trivial-variable.md) (the
  slices before it on the branch).
- **Files and behavior changed:** the 13 production sources of the table (38
  locations and two look-alikes), `scripts/test-clazy.py` (one name),
  `autotests/clazy/container-anti-pattern.cpp`. No observable change. One side
  effect on a later slice: the `range-loop-detach` inventory of
  [R2c](R2c.md#findings-per-check-and-per-file) falls from 305 findings in 47
  files to 274 in 45, because 31 of its loops ran over the same `keys()`
  temporaries (table above); none was added, and the 274 are the baseline minus
  those 31.
- **State authority / lifetime constraints:** the converted loops now read the
  live container instead of a copy of its keys, so a body that inserts into or
  removes from the container it walks, directly or through a signal handler,
  would invalidate the iteration. The bodies were read for that (above); the four
  loops where it could not be excluded keep a snapshot with a comment, and the
  invariant of the windows tracker loops is written above `updateScreenGeometries()`
  in `windowstracker.cpp`.
- **Toolchain and artifact identity:** container image `latte-r2c-clazy:noble`
  (ID `8e9528e34f50`; clazy 1.17 on LLVM 19.1.1, Ubuntu clang 18.1.3, Qt 6.11.1,
  CMake 3.30.5); host GCC 16.2.1, Clang 22.1.8, CMake 4.3.4, Qt 6.11.2. Raw logs
  and the probes are in the session notes folder of the batch (outside the
  repository).
- **Commands executed:** the Evidence table.
- **GCC/Clang Debug warning/error results:** 0 compiler warnings and 0 errors on
  both; Release presets were not built.
- **Tests registered / run / passed:** 57 / 57 / 57 on GCC and Clang Debug; no
  test added or changed.
- **Production behavior vs copied logic vs source-contract evidence:** the gate
  runs clazy on the production sources; the fixture holds a copy of the pattern;
  the probe reproduces the iteration forms on Qt containers and not the
  production functions; two existing tests run changed production code.
- **QML diagnostics added/removed:** none; no QML changed.
- **Negative fixtures / mutation evidence:** the fixture above and the run with
  the 13 pre-fix files mounted over the fixed ones.
- **Desktop feedback / log review / clean-quit evidence:** the host smoke on
  `main` passed with no warning from Latte's code and no core ([R2c fix batch
  2](R2c.md#fix-batch-2)). It reached startup with two docks, the window tracker
  through a day of window changes (AutoHide, AlwaysVisible and a DodgeActive
  dock), the context menu, the app launcher popup and the Docks dialog. It did not
  reach a vertical and a horizontal dock touching, a wallpaper change, renaming an
  active layout, cut and paste of docks, a layout import or a template export. The
  paths listed before the smoke: startup with one and with two docks and a window
  moved, maximized and closed next to a dock (`windowstracker.cpp`: the hint
  updates run on every window change); a dock with Dodge or Dynamic Background
  settings; a vertical and a horizontal dock touching on one screen
  (`updateExtraViewHints`); the dock context menu, opened in normal and in Edit
  mode (`updateVisibleActions`); a wallpaper change or a dock on a screen with a
  wallpaper (`BackgroundCache`); Preferences > Layouts > Apply with a renamed
  active layout; the Views dialog with a cut and pasted view (`Views::save`,
  multiple-layouts mode); a layout import of the old linked file format
  (`checkRepairMultipleLayoutsLinkedFile`); a template export of a view with a
  sub-containment (`exportTemplate`); expanding and collapsing a widget popup
  (`appletIsExpanded`).
- **Performance:** not measured. 34 of the 38 locations no longer allocate a
  list; the four snapshots still do, as before. The change cannot add work.
- **Combined validation:** on `main` at `382e55cd2`, GCC and Clang Debug and
  Release with 0 warnings and 57/57 tests each, the 43-check gate and the
  Debian 13 container build on Qt 6.8.2 passed; see [R2c fix batch 2](R2c.md#fix-batch-2).
- **CI, distribution and Nix checks:** not run (`main` is not pushed).
- **Unresolved failures, review assumptions and unavailable checks:** none
  failed. The assumption that the converted loop bodies do not change the
  container they walk, directly or through a signal handler, rests on reading the
  code, not on a test; the windows tracker, the synchronizer, the settings
  controllers and the context menu are not covered by one. Qt 6.6, the declared
  floor, was not built: the forms used (`asKeyValueRange()` since 6.4, `keyBegin()`,
  `std::as_const`, `std::find`) exist there, and the probe ran on Qt 6.4.2.
  `qDeleteAll(m_actions.values())` in the destructor of `Menu` has the same shape
  and is reported by neither `qdeleteall` nor this check in clazy 1.17, so it was
  left alone.
- **Next exact action:** after a push is approved, confirm the CI run and mark
  this slice Complete.
- **Commit approval status / push approval status:** committed on the feature
  branch; merged into `main` in `181b7f871` with the user's approval; not
  pushed.
