# R2c-unused-non-trivial-variable: Remove Locals That Nothing Reads

Status: Pending validation. Merged into `main` in `181b7f871`; the checks on
`main` and the host desktop smoke passed ([R2c fix batch 2](R2c.md#fix-batch-2)). The CI run
waits for a push.
Date: 2026-10-28. Branch: `claude/r2c-batch-c-bca59f`. Start HEAD: `eba91a3e6`.
Implementation commit: the commit that adds this report. Part of [R2c](R2c.md);
the first slice of batch C (fix batch 2).

## Defect

14 local variables in 10 files had a non-trivial type (`QString`, `QStringList`,
`QHash`) and were never read: clazy reports "unused QString cid". Each one still
paid for its construction and destruction, and most of them for a getter call, a
conversion or a copy that produced the value. There is no functional defect and
no known symptom; the finding is a cost and hygiene finding. Most of the values
look like leftovers of older code (see "Lost intent").

## Fix

Every flagged declaration was deleted. A variable was deleted only after its
initializer was read and found free of side effects, so no call had to stay as a
statement. No later statement referenced any of the 14 variables: the tree
builds without them.

| Location before the fix | Variable | Initializer and why it has no side effect |
| --- | --- | --- |
| `app/layouts/storage.cpp:1055` (`Storage::hasDifferentAppletsWithSameId`) | `QString cid` | `QString::number(containment->id())`: a getter and a pure conversion |
| `app/layouts/storage.cpp:1155` (`Storage::hasAppletsAndContainmentsWithSameId`) | `QString cid` | Same expression |
| `app/layouts/synchronizer.cpp:800` (`Synchronizer::switchToLayoutInMultipleModeBasedOnActivities`) | `QStringList allactivities` | `activities()`, which is `Corona::activitiesConsumer()->activities()`. `KActivities::Consumer` is documented as "read-only information about activities" and `activities() const` is an accessor. The disassembly of `KActivities::Consumer::activities() const` in the host's `libPlasmaActivities.so.6.7.5` shows an allocation of the result list and a loop that appends each cached activity id; the helpers it calls contain only `QArrayData::allocate`, `memmove` and branches: no D-Bus call, no signal |
| `app/plasma/extended/screenpool.cpp:63` (`ScreenPool::load`) | `QHash<QString, int> idForConnector` | A copy of `m_idForConnector`. It only holds a reference count; the `m_idForConnector.clear()` a few lines below used to drop one reference instead of freeing the data, and now frees it |
| `app/settings/screensdialog/delegates/checkboxdelegate.cpp:82` (`CheckBox::paint`) | `QString screendisplay` | `index.data(Qt::DisplayRole).toString()`: `QSortFilterProxyModel::data()` forwards to `Screens::data() const`, whose display branch builds a string and has no other effect. `paint()` still queries the other roles |
| `app/settings/settingsdialog/layoutscontroller.cpp:917` (`Layouts::save`) | `QStringList knownActivities` | `synchronizer()->activities()`, the accessor above |
| `app/settings/settingsdialog/layoutscontroller.cpp:923` (`Layouts::save`) | `QString switchToLayout` | Default constructed |
| `app/settings/settingsdialog/layoutsmodel.cpp:638` (`Layouts::setId`) | `QString oldId` | `m_layoutsTable[row].id`. The non-const `GenericTable::operator[]` detaches the row list, and the next statement, `m_layoutsTable[row].id = newId`, does the same, so the list is detached exactly as before |
| `app/view/containmentinterface.cpp:2275` (`ContainmentInterface::onAppletAdded`) | `const QString pluginId` | `pluginIdFromMetaData(applet->pluginMetaData())`, a free function that reads the plugin metadata and the file name and has no other effect |
| `app/view/positioner.cpp:527` (`Positioner::immediateSyncGeometry`, vertical branch) | `QString layoutName` | `m_view->layout() ? m_view->layout()->name() : QString()`: two plain getters |
| `containment/plugin/layoutmanager.cpp:2916` (`LayoutManager::toStr`) | `QString str` | Default constructed |
| `containmentactions/contextmenu/menu.cpp:380` (`Menu::populateLayouts`) | `QString layoutText` | A copy of `layoutsmenulist[i].layoutName` |
| `containmentactions/contextmenu/menu.cpp:419` (`Menu::populateMoveToLayouts`) | `QStringList activeNames` | `m_data.value(...).split(...)`: a `QStringList` element and a pure split |
| `containmentactions/contextmenu/menu.cpp:420` (`Menu::populateMoveToLayouts`) | `QStringList currentNames` | Same |

The diff only deletes lines: the 14 declarations and 8 blank lines (the one that
separated a declaration from the next statement, and a doubled blank line in
`checkboxdelegate.cpp` that `git clang-format` asked to collapse once the
declaration below it was gone). `formatter.sh` itself rewrites a whole file (439
changed lines in `layoutsmodel.cpp` alone), so `git clang-format` ran over the
changed lines instead, with the same `.clang-format`; its one other suggestion, a
blank line before the closing namespace braces at the end of `layoutmanager.cpp`,
is outside the change and was left.

### Lost intent

Nothing reads these values, so the deletion keeps today's behavior; the notes
record what the code seems to have wanted, for a later decision. Git history
starts at the repository reset, so none of this can be dated.

- `Layouts::save()` (`layoutscontroller.cpp`): `knownActivities` and
  `switchToLayout` suggest an older save that filtered the layouts' activities by
  the known ones and switched to a chosen layout afterwards. Today the switch
  after saving uses `nextSingleLayoutName` in single-layout mode and
  `switchToLayout(QString(), MultipleLayouts)` otherwise, and the activities of a
  layout are stored as the model holds them by `central->setActivities(...)`.
- `Positioner::immediateSyncGeometry()`: `availableScreenRegionWithCriteria()` takes an
  activity id (`activityid`, computed on the line below), so `layoutName` looks
  like the former argument.
- `Layouts::setId()` (`layoutsmodel.cpp`): `oldId` suggests an old-to-new id
  notification; the model only emits `dataChanged`, and `Layouts::save()` diffs
  the original and the current table instead.
- `ScreenPool::load()`: `idForConnector` is the id-to-connector twin of the
  `connectorForId` snapshot, which alone feeds the `updated` decision. Nothing
  compares the hash.
- `ContainmentInterface::onAppletAdded()`: the `else` branch declares its own
  `const QString pluginId`, which hides the one at the top of the function.
- `Synchronizer::switchToLayoutInMultipleModeBasedOnActivities()`:
  `validActivities()` already compares the layout's activities with
  `activities()` a few lines above, so `allactivities` repeated that read.
- `Menu::populateMoveToLayouts()`: `activeNames` and `currentNames` were copied
  from `populateLayouts()`; the move-to menu only compares layout names with
  `viewLayoutName`. In `populateLayouts()` the action text uses
  `layoutsmenulist[i].layoutName` directly, which makes `layoutText` redundant.
- `Storage` id-conflict checks: the loops over applets do not need the
  containment id; the containment ids are collected in a separate loop.

The gate side is one gate entry and one fixture, as in every R2c slice: the name
in `GATED_CHECKS` (after `temporary-iterator`) and
`autotests/clazy/unused-non-trivial-variable.cpp`, which holds the smallest
violation (`QString text = QString::number(value);` in a function that never
reads it). The gate has 38 entries and 38 fixtures on this branch.

## Evidence

Container commands are those of [R2c](R2c.md#reproducing-the-environment) with
the checkout mounted read-only, a build tree of its own (`$HOME/r2c-build-batch-c`)
and `-j6`/`--jobs 6`, because an idle-CPU measurement runs in the Fedora VM and
a second session builds at the same time (load average 12 to 20 during the
runs, so the clazy times are longer than the 97 s of an idle host).

| Command | Result |
| --- | --- |
| Container `cmake --preset clang-debug`, `cmake --build build/modernization/clang-debug -j6` on `eba91a3e6` (Clang 18.1.3, Qt 6.11.1, strict warnings) | Exit 0 in 159 s, 181 objects, 0 compiler warnings |
| Baseline `test-clazy.py --build-dir build/modernization/clang-debug --jobs 6 --inventory unused-non-trivial-variable,detaching-temporary,container-anti-pattern --report ...` | Exit 0 in 146 s: `unused-non-trivial-variable` 14 findings in 10 files (the other two checks: 43 and 38) |
| Container `cmake --build ... -j6` after the fix | Exit 0 in 27 s, 10 objects, 0 compiler warnings |
| `test-clazy.py ... --inventory unused-non-trivial-variable` on the fixed tree | Exit 0 in 170 s: **0 findings** in 153 sources |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 6` | **Exit 0** in 207 s: 38 fixtures rejected by their own checks, 153 sources with no finding from 38 checks |
| Same, with the previous versions of the 10 files mounted over the fixed ones | **Exit 1** in 174 s: 14 `[unused-non-trivial-variable]` findings at the baseline lines, nothing else |
| Host `cmake --preset gcc-debug`, `cmake --build --preset gcc-debug -j6`, `ctest --preset gcc-debug` (GCC 16.2.1, fresh tree) | Exit 0 in 429 s, 425 objects, 0 compiler warnings, 57/57 passed |
| Host `cmake --preset clang-debug`, `cmake --build --preset clang-debug -j6`, `ctest --preset clang-debug` (Clang 22.1.8, fresh tree) | Exit 0 in 353 s, 425 objects, 0 compiler warnings, 57/57 passed |
| After the last edit, a formatting one (a doubled blank line in `checkboxdelegate.cpp`): both host presets again | Exit 0 in 60 s (GCC) and 63 s (Clang), 2 objects rebuilt, 0 compiler warnings, 57/57 passed each |
| Same edit: container build, `--inventory unused-non-trivial-variable` and the full gate again | Exit 0 each: build 25 s with 1 object and 0 compiler warnings, **0 findings** in 155 s, gate **exit 0** in 147 s with 38 fixtures rejected and 153 sources clean |

The build logs hold thousands of `Warning:` lines from the qmllint step; they
are the known QML warnings tracked by the baseline gate, not compiler output,
and this change touches no QML.

## Handoff

- **Batch / slice and selected scope:** R2c fix batch 2, batch C, slice 1 of 3:
  `unused-non-trivial-variable`, 14 locations in 10 files.
- **Status:** Pending validation.
- **Source baseline / start HEAD / implementation HEAD:** `eba91a3e6` /
  `eba91a3e6` / the commit that adds this report.
- **Branch / existing user edits preserved:** `claude/r2c-batch-c-bca59f` in its
  own worktree; the tree was clean at the start.
- **Dependencies and evidence links:** [R2c](R2c.md) (gate design, environment),
  [R2c-qfileinfo-exists](R2c-qfileinfo-exists.md) and
  [R2c-range-loop-reference](R2c-range-loop-reference.md) (report shape).
- **Files and behavior changed:** the 10 production sources of the table (14
  declarations), `scripts/test-clazy.py` (one name),
  `autotests/clazy/unused-non-trivial-variable.cpp`. No observable change.
- **State authority / lifetime constraints:** none. The deleted locals were
  copies or fresh values that nothing read; no shared state was touched by their
  initializers (table above).
- **Toolchain and artifact identity:** container image `latte-r2c-clazy:noble`
  (ID `8e9528e34f50`; clazy 1.17 on LLVM 19.1.1, Ubuntu clang 18.1.3, Qt 6.11.1,
  CMake 3.30.5, Python 3.12.3); host GCC 16.2.1, Clang 22.1.8, CMake 4.3.4, Qt
  6.11.2. Raw logs are in the session notes folder of the batch (outside the
  repository).
- **Commands executed:** the Evidence table.
- **GCC/Clang Debug warning/error results:** 0 compiler warnings and 0 errors on
  both; Release presets were not built (no change in code paths that differ).
- **Tests registered / run / passed:** 57 / 57 / 57 on GCC and Clang Debug, 0
  skipped, 0 failed; no test added or changed.
- **Production behavior vs copied logic vs source-contract evidence:** the gate
  runs clazy on the production sources; the fixture holds a copy of the pattern.
  The side-effect claims rest on reading the initializers (and, for
  `KActivities::Consumer::activities()`, its disassembly).
- **QML diagnostics added/removed:** none; no QML changed.
- **Negative fixtures / mutation evidence:** the fixture above and the run with
  the 10 pre-fix files mounted over the fixed ones (14 findings, exit 1).
- **Desktop feedback / log review / clean-quit evidence:** the host smoke on
  `main` passed with no warning from Latte's code and no core ([R2c fix batch
  2](R2c.md#fix-batch-2)). It reached the context menu's layout list, the Screens
  dialog list, the Docks dialog and startup with `ScreenPool::load`. Adding a
  widget passed by the user's report without a log entry. It did not reach "Move
  to Layout" (multiple-layouts mode), renaming a layout or a vertical dock. The
  paths listed before the smoke: the dock context menu "Switch Layout" and, in
  multiple-layouts mode, "Move to Layout" (`menu.cpp`); Preferences > Layouts >
  Apply and renaming a layout (`layoutscontroller.cpp`, `layoutsmodel.cpp`); the
  Screens dialog list (`checkboxdelegate.cpp`); adding a widget (`onAppletAdded`);
  a vertical dock's free-area computation (`positioner.cpp`); the applet order
  saved by `LayoutManager::toStr`; the layout error and warning rows of the Views
  dialog (`Storage` id-conflict checks); startup, which runs `ScreenPool::load`.
- **Performance:** not measured. The change removes a few string constructions
  and one list read; it cannot add work.
- **Combined validation:** on `main` at `382e55cd2`, GCC and Clang Debug and
  Release with 0 warnings and 57/57 tests each, the 43-check gate and the
  Debian 13 container build on Qt 6.8.2 passed; see [R2c fix batch 2](R2c.md#fix-batch-2).
- **CI, distribution and Nix checks:** not run (`main` is not pushed).
- **Unresolved failures, review assumptions and unavailable checks:** none
  failed. The only assumption is that the activity accessor has no side effect,
  shown by its documentation and disassembly on Plasma 6.7.5; the container's
  library (Plasma 6.8) was not disassembled.
- **Next exact action:** after a push is approved, confirm the CI run and mark
  this slice Complete.
- **Commit approval status / push approval status:** committed on the feature
  branch; merged into `main` in `181b7f871` with the user's approval; not
  pushed.
