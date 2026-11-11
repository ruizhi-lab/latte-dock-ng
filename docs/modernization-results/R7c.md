# R7c: Move To Layout marks the current layouts again

Status: Complete. The GCC and Clang Debug builds and the full CTest suite pass on this
branch. On 2026-11-11 the user kept the current-activity marking. The menu data was checked on
the desktop, but the submenu itself was not seen (see [Desktop validation](#desktop-validation)). Debian 13 verification: deferred by the
user until the refactor is complete.

Scope: R2c follow-up 7, finding F5 of
[R2c-lost-intent-review.md](R2c-lost-intent-review.md#f5---the-move-to-layout-menu-no-longer-marks-running-layouts-low-impact-decision-needed).
Branch: `claude/r7c-move-menu-mark-ea9cdd`. Start HEAD: `e18a0f9a7` (`main`, release v1.3.0).
Commits on the branch: the implementation `21f715767` (`fix:`) and this record (`docs:`).
Nothing is pushed, and `main` is untouched.

## Step 1: was the removal deliberate?

No. The marking was lost in a conversion of the menu items, not dropped by a decision. Read-only
history, from `git log --all -S'[Current]' --oneline` and `git show` of each commit. HEAD's own
history starts at the reset root `8350a6b1c`, so the upstream lineage was read through the `--all`
refs, not through HEAD:

| Commit | Date | Effect on the marking |
| --- | --- | --- |
| `41fe38c19` | 2021-02-28 | Adds the `[Current]` suffix and bold for current layouts to the switch menu. |
| `631f4a9d6` | 2021-03-01 | Adds Move To Layout over the running layouts (`activeNames`). Current layouts are bold through the `QAction` font; a `[Current]` suffix is added while a running layout is not current. |
| `88f72bcc3` | 2021-05-15 | Switch menu items become radio buttons. Bold and suffix are replaced by the radio state (`setChecked(isCurrent)`). Move To Layout is not touched. Message: "change also layout actions to radiobuttons". |
| `69ec3d937` | 2021-05-15 | Move To Layout items become `LayoutMenuItemWidget`. Bold and suffix are removed, and the radio now marks only the view's own layout. `isCurrent` is still computed but never read. The message is "provide icons for moveToLayout options" and does not mention the marking. |
| `4674a4e2f` | 2026-07-31 | Removes the unused `isCurrent` local, with the `activeNames` and `currentNames` locals of the same function, during the warning cleanup. This was the last trace of the marking. |

Why this is a side effect: a custom-painted widget ignores the `QAction` font, so the bold went away
with the `QAction` code path. The replacement that the switch menu received (the radio state)
cannot carry over, because the Move To Layout radio already marks the view's own layout. No commit
message or replacement feature records a decision to drop the marker, so the branch restores it.

`69ec3d937` also changed the list source. The old menu iterated `activeNames` (load order); the new
one iterates the menu layout list, which `Synchronizer::menuLayouts()` sorts case-insensitively. The
set is the same, because in Multiple Layouts mode both are the central layouts. This branch keeps
the new order.

## Corrections to the brief

- `LayoutMenuItemWidget` did not draw bold at HEAD. Its paint code sets no weight, and
  `Latte::drawFormattedText()` renders the name through a `QTextDocument` that never receives the item
  font. The bold therefore has to be markup inside the widget. This branch adds it, and `sizeHint()`
  measures the same markup.
- The `[Current]` suffix is not restored. It appeared only while a running layout was not current,
  it is translated text, and the brief asked for the least intrusive restoration. Adding it back is
  a small change if the user wants it.

## Which items are bold, and what the desktop check should expect

- The Move To Layout items are built only in Multiple Layouts mode. Their list holds only the
  central (running) layouts, because `Corona::contextMenuData()` filters the menu layouts by
  `centralLayout()` in that mode.
- Bold is decided by `CURRENTLAYOUTSINDEX`, which `Corona::contextMenuData()` fills from
  `Synchronizer::currentLayoutsNames()`. In Multiple Layouts mode that is the central layouts that
  are on all activities or applied to the current activity. In that mode the switch menu uses the
  same list for its radio buttons.
- Preferences > Layouts bolds a different set. `LayoutName::paint()` bolds `ISACTIVEROLE`, which
  `Layouts::updateActiveStates()` sets for every loaded (central) layout.
- Every item in Move To Layout is already running, so bolding all of them would mark nothing. The
  marking singles out the running layouts on the current activity, which is what the code did
  before `69ec3d937`.

Expected desktop result: the bold items in Move Dock To Layout equal the bold items in
Preferences > Layouts when every running layout applies to the current activity (or to all
activities). A running layout that applies only to another activity is bold in Preferences and
plain in the menu. That is the current-activity rule the old code used. The user should accept it
or ask for the other rule, which would bold every item.

## Files and behaviour changed

- [menu.h](../../containmentactions/contextmenu/menu.h) and
  [menu.cpp](../../containmentactions/contextmenu/menu.cpp): `isCurrentLayoutName()` is the pure
  decision (non-empty name contained in `CURRENTLAYOUTSINDEX`). `populateMoveToLayouts()` parses the
  list the same way `populateLayouts()` does and calls `setCurrent()` on each item.
- [layoutmenuitemwidget.h](../../containmentactions/contextmenu/layoutmenuitemwidget.h) and
  [layoutmenuitemwidget.cpp](../../containmentactions/contextmenu/layoutmenuitemwidget.cpp):
  `setCurrent()` and `isCurrent()`. `displayedText()` adds the `<b>` markup, and both `paintEvent()`
  and `sizeHint()` use it.
- Tests: [contextmenuunittest.cpp](../../autotests/contextmenuunittest.cpp),
  [containmentactionmenuunittest.cpp](../../autotests/containmentactionmenuunittest.cpp) and
  [sourcecontracttest.cpp](../../autotests/sourcecontracttest.cpp). No new test target was added.

Behaviour: in Multiple Layouts mode the items of current layouts are drawn in bold. Check marks,
`setData()` and the click handling are unchanged, so a click on the view's own layout is still a
no-op. The switch menu is unchanged: `LayoutMenuItemWidget` defaults to not current, so its paint
and size do not change. No CMake target, QML, translation or help file changed.

## State authority, lifetime and failure modes

- Authority: `CURRENTLAYOUTSINDEX` from `Corona::contextMenuData()`, which reads
  `Synchronizer::currentLayoutsNames()`. `Menu::contextualActions()` copies it into `m_data` when
  the menu opens, and `populateMoveToLayouts()` reads that copy on `aboutToShow`. No other cache
  exists. The widget keeps only the flag. The flag is set before the item joins the menu, and a
  later change calls `updateGeometry()` and `update()`.
- Failure modes guarded:
  - A bold name clipped on the right: `sizeHint()` measures the same markup as `paintEvent()`. The
    width assertion in `currentLayoutItemIsDrawnBoldAndWider` covers it.
  - An empty name marked: an empty list splits into one empty entry, and an incomplete DBus reply
    can yield an empty name. `isCurrentLayoutName()` rejects empty names.
  - The marking silently dropped again: `contextMenuMarksCurrentLayoutsInMoveMenu` fails if the
    `setCurrent()` call is removed.

## Commands and exit status

Run in this worktree, with `--parallel 6`, GCC first and Clang after it. Logs are in
`build/modernization/` (ignored by git).

| Command | Exit | Log |
| --- | --- | --- |
| `git log -S'[Current]' --oneline -- containmentactions` | 0, no output on HEAD | terminal |
| `git log --all -S'[Current]' --oneline` | 0 | terminal |
| `git show 41fe38c19`, `631f4a9d6`, `88f72bcc3`, `69ec3d937`, `4674a4e2f` (menu and widget diffs) | 0 | terminal |
| `cmake --preset gcc-debug` | 0 | `gcc-configure.log` |
| `cmake --build --preset gcc-debug --parallel 6`, first build (started at HEAD; the edits began while it ran) | 0 | `gcc-build.log` |
| `cmake --build` and `ctest --preset gcc-debug` after the first edits (every edited file touched first) | 0 and 0, 61/61 | `gcc-build-2.log`, `gcc-ctest.log` |
| `cmake --build` and `ctest`, suite with the first same-size pixel test | 0 and 8 (`contextmenuunittest` failed; see Tests) | `gcc-build-final.log`, `gcc-final-ctest.log` |
| `cmake --build` and `ctest`, threshold pixel test, before the comment fix | 0 and 0, 61/61 | `gcc-build-final2.log`, `gcc-final2-ctest.log` |
| `cmake --build --preset gcc-debug --parallel 6`, final tree (recompiles `sourcecontracttest.cpp` only) | 0 | `gcc-build-final3.log` |
| `ctest --preset gcc-debug`, final tree | 0, 61/61 | `gcc-final3-ctest.log` |
| `cmake --preset clang-debug` | 0 | `clang-configure.log` |
| `cmake --build --preset clang-debug --parallel 6`, full build from scratch (429 objects) | 0 | `clang-build.log` |
| `ctest --preset clang-debug` | 0, 61/61 | `clang-ctest.log` |

Mutation runs used `cmake --build --preset gcc-debug --target <test>` with the script
`/tmp/r7c-mutations/mutate.py`, which lives outside the repository.

## Warnings

- GCC: 0 C++ compiler warnings or errors in each build round that compiled the changed files
  (`gcc-build.log`, `gcc-build-2.log`, `gcc-build-final.log`, `gcc-build-final2.log`,
  `gcc-build-final3.log`), counted with `grep -cE` on the `file:line:col: warning:` and `error:` pattern.
- Clang: 0 C++ compiler warnings or errors in `clang-build.log`, a full build of 429 objects from
  scratch that includes every changed file.
- The first full GCC build log (`gcc-build.log`, taken before the final edits) contains 5491 lines
  `Warning: ...qml:... [missing-property]` and `[unqualified]` from the build's existing qmllint
  step. They are not compiler diagnostics. Incremental builds do not rerun that step, so the later
  logs have none. This branch changes no QML, so the qmllint baseline comparison in the brief does
  not apply and was not run.

## Tests

No new executable was registered. Four cases were added to three existing executables:

- `contextmenuunittest`: `currentLayoutItemIsDrawnBoldAndWider`.
- `containmentactionmenuunittest`: `onlyListedCurrentLayoutsAreMarked`, `emptyCurrentListMarksNothing`.
- `sourcecontracttest`: `contextMenuMarksCurrentLayoutsInMoveMenu` (an extra lock on the wiring).

Results: GCC CTest 61/61 passed; Clang CTest 61/61 passed. The logout simulation
(`logoutsimulationtest`) is part of each run and passed in both.

The bold test renders the regular and the bold item at the same size and state, and counts the
pixels that differ between the two renderings (1023 in the last measurement). It requires more than
100. The mutation run below shows that removing the markup from the paint path fails it.

Three versions of the pixel check were tried; the record keeps them for the reviewer:

- The first counted pixels that differ from the top-left pixel of each rendering, each item at its
  own size. It passed on the correct code and also with the markup removed. The highlight background
  is not uniform, so almost every pixel differs and the count does not follow the glyph weight.
- The second rendered both items at one size but kept that count. It failed the full suite on the
  correct code (`gcc-final-ctest.log`, `contextmenuunittest`).
- The final version compares the two renderings pixel by pixel. That is the check described above.

Against HEAD, the new cases cannot compile: `setCurrent` and `isCurrentLayoutName` are absent from
HEAD's headers (`git show HEAD:<header> | grep -c` gives 0 for both).

## Mutation evidence

Each production line was changed temporarily, the affected test target was rebuilt, and the test
binary was run. The original file was then restored, and `cmp` confirmed it byte for byte.

| # | Temporary production change | Expected failing case | Observed |
| --- | --- | --- | --- |
| 1 | `isCurrentLayoutName` inverted | `onlyListedCurrentLayoutsAreMarked`, `emptyCurrentListMarksNothing` | 2 FAIL |
| 2 | empty-name guard removed | `emptyCurrentListMarksNothing` | 1 FAIL |
| 3 | `paintEvent()` ignores the bold markup | `currentLayoutItemIsDrawnBoldAndWider` (pixel check) | FAIL |
| 4 | `sizeHint()` ignores the bold markup | `currentLayoutItemIsDrawnBoldAndWider` (width check) | FAIL |
| 5 | `setCurrent()` call removed from `populateMoveToLayouts()` | `contextMenuMarksCurrentLayoutsInMoveMenu` | FAIL |

A first attempt at mutation 1 (`return false`) did not compile under the strict flags, because it
left the parameters unused. The inverted decision replaced it.

## Not verified

- The desktop check in "Next action". No dock was started, as the brief requires; only CTest
  runs, including the logout simulation, executed here.
- Multi-activity behaviour on a live desktop. The unit tests do not model activities.
- The bold and pixel assumptions depend on a font with a bold face. A system without one would make
  the pixel test fail, which is a visible signal rather than a silent pass.
- clazy is not installed here. Its rules were followed by hand, and CI runs it.
- Debian 13 verification: deferred by the user until the refactor is complete.

## Next action

Start Latte in Multiple Layouts mode, right-click a dock, open Move Dock To Layout, and compare its
bold items with the bold layouts in Preferences > Layouts. Then accept or discard
`claude/r7c-move-menu-mark-ea9cdd` as a whole.

## Commit status

Committed locally on `claude/r7c-move-menu-mark-ea9cdd`: the implementation `21f715767`, then this
record. Not pushed, and `main` was not touched. The user accepts or discards the branch as a whole.

## Desktop validation

Checked on 2026-11-10 and 2026-11-11 on the user's desktop. Multiple Layouts mode with Empty on Work was set in the configuration, because the editor's 活动 menu did not keep the tick (R7j). Among the entries Empty and 我的布局, `contextMenuData()` returned 我的布局 as the only current layout, so 我的布局 is drawn bold and Empty plain. The Layouts Editor showed both loaded layouts in bold. The user kept the current-activity rule. The submenu was not seen on screen. Full record: [R7-desktop-check](R7-desktop-check.md).
