# R2c-overloaded-signal: Signals That Overload a Base-Class Signal

Status: Pending validation. Not run here: CI and the desktop smoke (the main session
runs the smoke after merging the batch).
Date: 2026-10-29. Branch: `claude/r2c-batch-e-fc8dc7`. Start HEAD: `95e342f72`.
Implementation commit: the commit that adds this report. Part of [R2c](R2c.md), the
first slice of fix batch 3 (batch E), before `R2c-overridden-signal` and
`R2c-qproperty-without-notify`.

## Defect

clazy's `overloaded-signal` reports a signal whose name equals a signal of a base
class, whatever its parameters. In the derived class the name hides the base
signal, so `&Derived::name` silently means the derived one, and a reader cannot tell
which signal a connect targets. The 11 findings (line numbers in this report are those of the pre-fix tree):

| Header:line | Signal | Base signal | Disposition |
| --- | --- | --- | --- |
| `app/settings/settingsdialog/layoutsmodel.h:139` | `Layouts::rowsInserted()` | `QAbstractItemModel::rowsInserted(QModelIndex, int, int)` | renamed |
| `app/settings/viewsdialog/viewsmodel.h:112-113` | `Views::rowsInserted()`, `Views::rowsRemoved()` | `QAbstractItemModel::rowsInserted`, `rowsRemoved` | renamed |
| `app/view/view.h:307`, `309`, `335`, `337`, `338` | `widthChanged`, `heightChanged`, `visibilityChanged`, `xChanged`, `yChanged` | `QWindow` signals of the same names | reviewed exception |
| `app/view/settings/primaryconfigview.h:106-107` | `xChanged`, `yChanged` | `QWindow::xChanged`, `QWindow::yChanged` | reviewed exception |
| `declarativeimports/core/iconitem.h:161` | `smoothChanged()` | `QQuickItem::smoothChanged(bool)` | reviewed exception |

The trap is real: `layoutscontroller.cpp:69` connects `&Model::Layouts::rowsRemoved`.
`Layouts` declares no `rowsRemoved`, so that name resolves to
`QAbstractItemModel::rowsRemoved`, not to a signal of the model, although the same
line pattern in `viewscontroller.cpp` names the model's own signal.

## Fix

- Renamed the three internal model signals that no `.qml`, `.js`, string lookup or
  other file uses: `Layouts::rowsInserted()` to `layoutsInserted()`,
  `Views::rowsInserted()` to `viewsInserted()` and `Views::rowsRemoved()` to
  `viewsRemoved()`. Every `Q_EMIT` stays where it was and every connect keeps its
  slot. Each renamed declaration has a `//!` line that says why the name is not the
  base name.
- Kept eight NOTIFY signals as reviewed exceptions, each marked
  `// clazy:exclude=overloaded-signal` on its line, with one `//!` reason above each
  group. Renaming them would change which signal the QML change handlers bind to, and
  `positioner.cpp:201` connects `Latte::View::visibilityChanged` by pointer, outside
  this slice's files.

The changed files are the owned production files of this slice, the one `GATED_CHECKS`
line, the new fixture `autotests/clazy/overloaded-signal.cpp` and this report.

## Sites

Line numbers are those of the pre-fix tree.

| Site | Change | What was checked |
| --- | --- | --- |
| `layoutsmodel.h:139` | `rowsInserted()` renamed `layoutsInserted()` | `git grep` over the tree (QML, JS, C++, autotests) for `rowsInserted`, `rowsRemoved`, `layoutsInserted`, `viewsInserted` and `viewsRemoved` finds only the sites in this table. No `SIGNAL()` or `invokeMethod` use |
| `layoutsmodel.cpp:184` (`appendLayout`) | `Q_EMIT layoutsInserted()` | Same place: after `endInsertRows()` |
| `layoutsmodel.cpp:859` (`setOriginalData`) | `Q_EMIT layoutsInserted()` | Same place: after `endInsertRows()` |
| `layoutscontroller.cpp:68` | connect to `&Model::Layouts::layoutsInserted` | Target slot `Layouts::dataChanged` unchanged |
| `layoutscontroller.cpp:69` | **unchanged** | `Model::Layouts` declares no `rowsRemoved`, so the name is `QAbstractItemModel::rowsRemoved`, emitted by `endRemoveRows()`. The task said to keep it; behavior is the same before and after |
| `viewsmodel.h:112-113` | renamed `viewsInserted()`, `viewsRemoved()` | Same grep; the only users are the three emits and the two connects |
| `viewsmodel.cpp:345` (`appendTemporaryView`) | `Q_EMIT viewsInserted()` | Same place: after `endInsertRows()` |
| `viewsmodel.cpp:354` (`removeView`) | `Q_EMIT viewsRemoved()` | Same place: after `removeRows()`, which calls `endRemoveRows()` before it returns (`viewsmodel.cpp:372`) |
| `viewsmodel.cpp:579` (`setOriginalData`) | `Q_EMIT viewsInserted()` | Same place: after `endInsertRows()` |
| `viewscontroller.cpp:57-58` | connects to `viewsInserted` and `viewsRemoved` | Same target slot `Views::dataChanged` |
| `view.h:307`, `309`, `335`, `337`, `338` | exclusion on `widthChanged`, `heightChanged`, `visibilityChanged`, `xChanged`, `yChanged` | QML: `Connections` with `target: latteView` in `VisibilityManager.qml:61-68` (`onXChanged`, `onYChanged`, `onWidthChanged`, `onHeightChanged`), in `abilities/AutoSize.qml:87-100` (`onWidthChanged`, `onHeightChanged`) and in `main.qml:637-653` (`onVisibilityChanged`). C++ by pointer: `positioner.cpp:201` connects `&Latte::View::visibilityChanged`. Other C++ connects of these properties name the `QWindow`/`QQuickWindow` signals (`subwindow.cpp:51-54`, `effects.cpp:57-58`, `view.cpp:381-383`, `positioner.cpp:156-159`); none names `View::xChanged` and so on. `View` emits `visibilityChanged()` at `view.cpp:238` |
| `primaryconfigview.h:106-107` | exclusion on `xChanged`, `yChanged` | `primaryconfigview.cpp:48-49` forwards the `QWindow` signals to these, in the constructor. QML reads `viewConfig.x` and `viewConfig.y` only inside the `onPressed` and `onPositionChanged` mouse handlers (`shell/package/contents/controls/DragCorner.qml:49-77`), so no binding depends on the NOTIFY. No `onXChanged` handler in the configuration QML |
| `iconitem.h:161` | exclusion on `smoothChanged` | No `onSmoothChanged` handler and no read of `IconItem.smooth` in the tree. The one `smooth:` read, `plasmoid/package/contents/ui/task/TaskIcon.qml:206`, reads a `Kirigami.Icon`. `setSmooth()` (`iconitem.cpp:356-364`) does not emit the signal |

## Observations for the main session (not changed here)

- `View` declares `xChanged`, `yChanged`, `widthChanged` and `heightChanged` as NOTIFY
  signals, but no `Q_EMIT` of these names exists in the tree (`git grep`). Which signal
  the QML handlers in `VisibilityManager.qml` and `AutoSize.qml` bind to depends on
  QML's name lookup, and this slice did not test whether they run. That is one reason
  to keep the names unchanged. A runtime check would settle it; this slice does not
  run one.
- `IconItem::smoothChanged()` is never emitted, because `setSmooth()` only calls
  `update()`. Bindings on `IconItem.smooth` are not re-evaluated on change. No QML in
  the tree depends on it today.
- `layoutscontroller.cpp:69` names the base signal (see Sites). Spelling it
  `&QAbstractItemModel::rowsRemoved` would not change behavior; this was left out
  because the task asked to keep the line.

## Evidence

Container commands are those of [R2c](R2c.md#reproducing-the-environment) with a
private build directory (`~/r2c-build-batch-e`) and `--jobs 4`. Versions in the
container: Ubuntu 24.04, clang 18.1.3 (the CMake compiler), Qt 6.11.1 (KDE neon
packages), clazy 1.17.1 (`clazy version 1.17`, reported as "Ubuntu LLVM version
19.1.1"). The inventory compile database holds 153 production sources; 55 autotest
sources, 134 duplicate compile commands and 87 generated sources are not analyzed.

| Command | Result |
| --- | --- |
| `cmake --preset clang-debug -DVERSION=...`, `cmake --build --preset clang-debug -j4` on the pre-fix tree `95e342f72` (fresh build) | Exit 0. 0 `warning:` lines and 0 compiler error lines in the configure and build logs |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 4 --inventory overloaded-signal,overridden-signal,qproperty-without-notify --report build/inventory.json` before the fix | Exit 0: `overloaded-signal` 11 findings in 5 files, `overridden-signal` 2 in 2 files, `qproperty-without-notify` 1 in 1 file; 14 first-party findings |
| Same tree after the fix (incremental build `build/build-slice1.log`, then the same inventory with `--report build/inventory-slice1.json`) | Build exit 0, 0 `warning:` lines, 0 clang error lines; the qmllint `Warning:` count is 6832 before and after. Inventory exit 0: `overloaded-signal` **0 findings in 0 files**; `overridden-signal` 2 and `qproperty-without-notify` 1 remain for later slices |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 4` (full gate, 44 gated checks) | **Exit 0**: "44 negative fixtures: each is rejected by its own gated check", then "153 production sources: no findings from 44 gated clazy checks" |
| Mutation: `git show 95e342f72:app/view/view.h` mounted read-only over `app/view/view.h` in the container (no rebuild), then the full gate | **Exit 1**: "44 negative fixtures" still rejected, then "5 clazy finding(s) in gated checks": `view.h:307`, `309`, `335`, `337` and `338`, all `[overloaded-signal]`. The same file without its exclusion comments fails the gate, so the exclusions are what keeps these five out |
| Host `cmake --preset gcc-debug`, `cmake --build --preset gcc-debug -j4`, `ctest --preset gcc-debug` (GCC 16.2.1, fresh build tree) | **Exit 0** for all three. 0 `warning:` lines, 0 error lines and 0 CMake warnings in the logs; `ctest`: 100% passed, 0 failed out of 58 tests |
| Host `cmake --preset clang-debug`, `cmake --build --preset clang-debug -j4`, `ctest --preset clang-debug` (Clang 22.1.8, fresh build tree) | **Exit 0** for all three. 0 `warning:` lines, 0 error lines and 0 CMake warnings; `ctest`: 100% passed, 0 failed out of 58 tests |
| `git clang-format --style=file --diff HEAD -- <changed C++ files>` | Three hunks, none applied: `private Q_SLOTS:` in `layoutsmodel.h` and `protected:` in `primaryconfigview.h` are lines this slice did not change; the three new `Q_SIGNALS` lines in `viewsmodel.h` would be indented by two spaces, but the members of that block use four (see Handoff) |

## Handoff

- **Batch / slice and selected scope:** R2c fix batch 3, batch E, slice 1
  `R2c-overloaded-signal` (11 findings in 5 files).
- **Status:** Pending validation (see the evidence table).
- **Source baseline / start HEAD / implementation HEAD:** `95e342f72` (`main`, the
  same as the branch start) / `95e342f72` / the commit that adds this report.
- **Branch / existing user edits preserved:** `claude/r2c-batch-e-fc8dc7`; the
  worktree was clean at the start. Nothing was pushed and `main` was not touched.
- **Dependencies and evidence links:** [R2c](R2c.md), in particular its "Gate design"
  and "Next actions" sections; [R2c-const-signal-or-slot](R2c-const-signal-or-slot.md)
  for the report shape.
- **Files and externally observable behavior changed:** `app/settings/settingsdialog/layoutsmodel.h`,
  `layoutsmodel.cpp`, `layoutscontroller.cpp`; `app/settings/viewsdialog/viewsmodel.h`,
  `viewsmodel.cpp`, `viewscontroller.cpp`; `app/view/view.h`;
  `app/view/settings/primaryconfigview.h`; `declarativeimports/core/iconitem.h`;
  `scripts/test-clazy.py` (one name); `autotests/clazy/overloaded-signal.cpp`.
  Externally observable behavior does not change: the renamed signals are internal, the
  exceptions add comments only, and each signal is emitted at the same point.
- **State authority / invalidation / lifetime / fallback constraints:** none new. The
  emit points are the same as before: after `endInsertRows()` and after `removeRows()`
  (which calls `endRemoveRows()`), so views of the models see the base signal first.
- **Toolchain, import environment and artifact identity:** as in the Evidence section.
- **Commands executed with exit status and durable log/artifact locations:** the table
  above. Raw logs were in a scratch directory that is deleted at the end; the inventory
  JSON files stay in `~/r2c-build-batch-e/` (`inventory.json`, `inventory-slice1.json`).
- **GCC/Clang Debug/Release warning/error results:** GCC Debug and Clang Debug: 0 compiler
  warnings, 0 errors, 0 CMake warnings (see the table). Release builds were not run.
- **Tests registered / run / passed / skipped / failed and name deltas:** 58 registered,
  58 run, 58 passed on GCC 16.2.1 and on Clang 22.1.8 (host Debug); none skipped or
  failed. No test was added, removed or renamed.
- **Formatting:** `formatter.sh` was not run. `git clang-format --style=file --diff HEAD`
  would indent the new `Q_SIGNALS` lines of `viewsmodel.h` by two spaces; the members
  of that block use four, so the file's style was kept (the same choice as
  [R2c-const-signal-or-slot](R2c-const-signal-or-slot.md#handoff)). Nothing else applied.
- **Production behavior vs copied logic vs source-contract evidence:** production
  code. No source-contract test names the renamed or excluded declarations (`git grep`
  in `autotests/`).
- **QML diagnostics added/removed and environment compatibility:** none added. The qmllint
  warning count in the container build is unchanged (6832).
- **Negative fixtures or controlled mutation evidence:** `autotests/clazy/overloaded-signal.cpp`
  is rejected by its check (the gate's fixture step) and the mutation above.
- **Desktop user feedback / log review / clean-quit and coredump evidence:** not run.
  The main session runs the desktop smoke after the merge.
- **Performance paired samples / variability / work counts / tradeoffs:** not applicable.
- **CI, distribution and Nix checks (passed / failed / unavailable):** unavailable here.
- **Unresolved failures, review assumptions and unavailable checks:** the IDE shows
  clang diagnostics for the edited headers and sources because it has no project
  include flags; the container build is the evidence, and it has no errors. The
  observations above are open questions, not fixes.
- **Next exact action:** the main session merges this slice with the others and runs
  the combined gate and the desktop smoke.
- **Commit approval status / push approval status:** committed on the feature branch
  `claude/r2c-batch-e-fc8dc7`; not pushed.
