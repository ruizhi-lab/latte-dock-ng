# R8c-fix: the two defects of the R8c inventory

Status: Pending desktop check. The code, the tests, the four host presets, the clazy
gate and the clang-tidy re-check are verified (see Verification). The user-mode
install, the desktop check of the Docks table and the merge wait for the end of the
R8d memory series (about 23:59 on 2026-11-09).

Batch / slice and selected scope: R8c-fix, fix the two real defects that the
[R8c record](R8c.md) lists under "Findings worth a look". Acceptance: each check no
longer reports its defect, a behavioural test where the code can be reached,
0 compiler warnings on GCC and Clang in the four presets, CTest passing, the clazy gate
in `latte-r2c-clazy:noble`, and a desktop check of the Docks table.

Source baseline and start HEAD: `86de58ed6` (`main`). Branch
`claude/strange-pare-756530`, worktree `.claude/worktrees/strange-pare-756530`.

## The defects and the fixes

| Check (R8c inventory, clang-tidy 18.1.3) | Location | Change |
|---|---|---|
| `misc-redundant-expression` (1) | `app/plasma/extended/backgroundcache.cpp:269` | `inBounds` now tests `bright1 <= 255`; before it tested `bright2 <= 255` twice |
| `bugprone-parent-virtual-call` (2) | `app/settings/viewsdialog/viewstableview.cpp:35` | `ViewsTableView::selectionChanged()` calls `QTableView::selectionChanged()` |
| `bugprone-parent-virtual-call` (2) | `app/view/view.cpp:1800` | Unchanged: the documented libplasma 6.8 workaround is an intended call |

### `BackgroundCache::areaIsBusy()`: the bound

- The defect is latent. `colorBrightness()` weights its channels 299, 587 and 114,
  which sum to 1000, so the brightness of a tile is always in 0..255. The public
  `busyFor()` path therefore never reaches the out-of-range branch that the wrong bound
  decides. The fix changes results only for values that path does not produce.
- Test: `CoreComponentsTest::areaIsBusyTreatsOutOfRangeAsBusy` in
  `autotests/corecomponentstest.cpp` calls `areaIsBusy()` directly. `BackgroundCache`
  declares `friend class ::CoreComponentsTest;`, the same test access as
  `DeclarativeCoreUnitTest` in `declarativeimports/core/iconitem.h`. Without it the
  method is private and the branch cannot be reached from a test target.
- Mutation: with the old bound, the test fails at `areaIsBusy(300, 200)`: the call
  returns 0 where `true` is expected. With the fix the test passes.

### `ViewsTableView::selectionChanged()`: the base call

- `QAbstractItemView::selectionChanged()` skips `QTableView::selectionChanged()`, and
  the table view override sends the accessibility events for the selection.
- Test: `SettingsViewUnitTest::viewsTableEmitsSelectionAccessibilityEvents` in
  `autotests/settingsviewunittest.cpp` shows the table, sets accessibility active and
  checks for `QAccessible::SelectionAdd` after `selectRow(1)` and for
  `QAccessible::SelectionRemove` and `SelectionAdd` after `selectRow(2)`.
- Accessibility events are generated only while accessibility is active. The first
  probe, which did not set it, saw no selection events at all, so the test sets it
  explicitly.
- Mutation: with the grandparent call, the test fails with no `SelectionAdd` event;
  with the fix it passes. The unit test checks only these events; the visible
  behaviour (selection, highlight, the buttons that act on the selected rows) is what
  the desktop check covers.

## Verification

Targeted runs, local GCC Debug tree (`settingsviewunittest`, `corecomponentstest`):

| Run | Result |
|---|---|
| Fix in place | `settingsviewunittest` 5/5 pass, `corecomponentstest` 13/13 pass, 0 warnings |
| Old bound restored (mutation) | `areaIsBusyTreatsOutOfRangeAsBusy` fails at `areaIsBusy(300, 200)` |
| Grandparent call restored (mutation) | `viewsTableEmitsSelectionAccessibilityEvents` fails: no `SelectionAdd` |

Matrix on the branch sources. Fresh build trees, run from 21:40 to 22:08 on
2026-11-09; the source files were last changed before the first build.

| Step | Configure | Build | Compiler warnings | CTest |
|---|---|---|---|---|
| `gcc-debug` | 0 | 0 | 0 | 61/61 passed |
| `clang-debug` | 0 | 0 | 0 | 61/61 passed |
| `gcc-release` | 0 | 0 | 0 | 61/61 passed |
| `clang-release` | 0 | 0 | 0 | 61/61 passed |
| `clang-debug` in `latte-r2c-clazy:noble` (Clang 18.1.3) | 0 | 0 | 0 | not run |

"Compiler warnings" counts the `warning:` lines that GCC and Clang print. The configure
logs have no `CMake Warning`. Each tree compiled 429 C++ objects.

Clazy gate, in the container: exit 0. Its output is "155 production sources: no
findings from 49 gated clazy checks", and each of the 49 negative fixtures was rejected
by its own check.

Clang-tidy re-check, CI version (clang-tidy 18.1.3). The R8c inventory tool ran with
only the two checks over the 155 production sources; no source failed.

| Check | R8c inventory | Now |
|---|---:|---:|
| `misc-redundant-expression` | 1, at `backgroundcache.cpp:269` | 0 |
| `bugprone-parent-virtual-call` | 2, at `viewstableview.cpp:35` and `view.cpp:1800` | 1, at `view.cpp:1800` (the documented workaround) |

Qt QML tool output, not compiler warnings. The Debug builds print 5769 lines, qmllint
messages such as `Warning: ... [missing-property]` and `You first have to give the
element an id`. The container build prints 7110. The Release builds print one line:
`Warning: dialog.h:25: PlasmaQuick::Dialog is used as base type but cannot be found.`
None of them is in the GCC and Clang form (`warning:`). The project classes that
`dialog.h:25` message as tooling evidence, not a compiler warning
([architecture baseline](../architecture-modernization-baseline.md)), and
[M0](M0.md), [P2a](P2a.md) and [R7d](R7d.md) already record it. The CI deep QML lint
fails only on QML errors and broken generated modules, and it summarizes the legacy
warnings. This change touches no QML file and no class registered to QML. The deep QML
lint itself was not run here, because `qmllint` is not on this host's PATH.

## Desktop check (zh_CN, to run after the R8d series ends)

Run on the user-mode Debug build of this branch (the install and the restart follow the
Development Debug workflow in `AGENTS.md` and must not start before 23:59 on
2026-11-09). The labels are the zh_CN strings from `po/zh_CN/latte-dock.po`; the English
msgid is in brackets. The menu entry in step 1 comes from the dock context-menu data in
`app/data/contextmenudata.h`, not from a run, so confirm its wording on screen.

Precondition: the layout to test has at least two docks, so that the multi-select step
has two rows to select.

1. Right-click a Latte dock, choose 配置 Latte... [Configure Latte...]. The Latte settings window opens.
2. Click the tab 布局编辑器 [Layouts Editor].
3. Click a layout row in the layouts table to select it. Click the button 停靠栏(P)... [&Docks...] (shortcut Ctrl+P). The dialog titled 停靠栏 [Docks] opens with that layout's docks.
4. Click one dock row. The row shows the selection highlight, and the buttons 复制(D) [&Duplicate], 移除 [Remove] and 导出(E) [&Export] become enabled.
5. Ctrl+click a second dock row. Both rows show the highlight and the three buttons stay enabled.
6. Click the empty area below the rows. The highlight clears and the three buttons are disabled again.
7. Click one dock row, then 复制(D). A copy of that dock appears. Select the copy and 移除. The row leaves the list at once; no confirmation prompt appears. These are staged changes, not applied yet.
8. Select one dock row, open the 导出(E) menu, and choose 导出备份(E)... [&Export For Backup...] or 导出为模板(T)... [Export As &Template...]. The first opens a save-file dialog for that dock; the second opens the template export dialog. Cancel whichever opens, so no file is written.
9. Close the 停靠栏 dialog with 取消 [Cancel], which discards the staged changes. Do not press 立即应用 [Apply Now].

Expected: every step behaves as above. The Debug log (`/tmp/latte-ng.log`) has no new
warning or error from the dialog or the table. Report each step as passed or failed,
with what you saw.

## Limits

- Not run here: the deep QML lint (see above), the Debian 13 container build, the
  ASan/UBSan preset and the install-script tests. They are left to CI.
- The desktop check is not run yet. The user-mode install and any dock restart wait for
  the end of the R8d series (about 23:59 on 2026-11-09).
