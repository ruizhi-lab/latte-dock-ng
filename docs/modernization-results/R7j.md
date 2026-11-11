# R7j: a click on a checkable activity item closes the Layouts menu (tick lost)

Status: the fix is committed on `claude/sleepy-hugle-be343e` as `7a4100b6a` (the
PersistentMenu fix and its test). It is not merged into `main` and not pushed. The
zh_CN desktop check is still to be done; the steps are at the end of this record.

## Summary

- **Symptom.** In the Layouts editor, the activity menu of a layout closes on the click
  that ticks Work. The editor then commits without OK, so the tick is lost and the cell stays
  empty (the desktop report of 2026-11-10; reproduced here as described in the evidence below).
- **Cause.** `PersistentMenu` (`app/settings/generic/persistentmenu.cpp`) kept the popup open by
  refusing one `setVisible(false)` after a release on an item. Since Qt 6.2 a native popup
  closes through `QWindow::close()`. Qt calls the widget's `setVisible(false)` (refused by
  Latte) and then hides the widget itself, once the window close is accepted. The refusal
  prevents nothing, so the popup closes in the same click.
- **Hypothesis check (from the request).** The second hide is confirmed: it is the window
  hide that follows the refused call. The focus step is confirmed too, but it follows the
  hide and does not start it. Nothing in the menu's `m_masterIndex` path is involved.
- **Fix.** `closeEvent()` ignores the close while `QMenu::mouseReleaseEvent()` handles a release
  on one of the items, and `setVisible(false)` is refused in the same window. The flag
  `m_keepOpenForRelease` lives only for that call. The old one-shot `m_blockHide` could also
  outlive a release that QMenu did not close and then swallow the next hide, the one of OK.
- **Test.** `autotests/persistentmenutest.cpp` (three cases). The click case fails on the
  previous code and passes on the fix.

## Environment

- Host: Gentoo, Qt 6.11.3, KWin and plasmashell 6.7.6, GCC 16.2.1, clang 22.1.8. The
  nested session runs `kwin_wayland --virtual` 6.7.6 with a scratch HOME and a private bus.
  Its environment sets KWin's own test switch `KWIN_WAYLAND_NO_PERMISSION_CHECKS=1`, which
  lets the pointer client use `org_kde_kwin_fake_input`; the switch applies to that KWin only.
- The user's desktop (zh_CN, Plasma 6 Wayland) was not touched. Its Qt version is not known;
  the desktop check asks for it.
- Dialog buttons in the harness show Qt's zh_CN strings (`确定(&O)`, `取消(&C)`).

## Reproduction

The harness [`R7j-tools/harness/repro.cpp`](R7j-tools/harness/repro.cpp) runs the production
`Settings::Layout::Delegate::Activities` editor on a `QTableView`. The model mirrors
`Model::Layouts` for one layout with nothing assigned, and two running activities: 默认
(current) and Work. The baseline is `PersistentMenu` from `351761a4b`; the copy with qInfo
probes in [`R7j-tools/variants/probes`](R7j-tools/variants/probes) has the same logic.

1. **Offscreen, clicks sent with QTest** (`--qtest`). The baseline closes the menu on the
   release on Work. The editor commits with `OKPRESSED=unset`.
2. **Nested KWin 6.7.6 Wayland, real pointer events**
   ([`R7j-tools/wl/run_nested.py`](R7j-tools/wl/run_nested.py)). The clicks go through KWin's
   `org_kde_kwin_fake_input` (the pointer enters the popup, then presses and releases on Work).
   The result is the same.

Baseline run, `R7j-data/wayland/baseline-none.harness.log` (times in ms, abridged):

```text
R7J-BUTTON 659 54
R7J mouseRelease pos= QPoint(149,-15) actionAt= (none) ... after QMenu::mouseReleaseEvent blockHide= false isVisible= true
R7J-WORK 659 208
R7J mouseRelease pos= QPoint(149,139) actionAt= Work activeAction= Work
R7J  3834 EVENT menu Close
R7J setVisible( false ) blockHide= true isVisible= true activePopup= true
R7J   -> blocked, blockHide cleared
R7J  3834 EVENT menu Hide
R7J  3835 SIGNAL delegate commitData OKPRESSED=unset
R7J  3835 SIGNAL action 'Work' toggled(1)
R7J  3835 SIGNAL editor destroyed
R7J  5334 STATE requested: Work checked=1 menuVisible=0 editorAlive=0 buttonText=''
```

The first release, on the editor button and outside the menu, leaves the menu open; the
user's report says the menu opens and stays. The screenshot after the click shows the cell
empty and no menu: `R7j-data/wayland/screenshots/baseline-window-after-work.png`.

## Where the second hide comes from (gdb, Qt 6.11.3)

`R7j-data/gdb/baseline-close-chain.txt` has the backtraces (the probe copy, `-g`, Qt
without debug symbols; the Qt frames are exported symbols or addresses). In short:

```text
QMenu::mouseReleaseEvent -> QMenuPrivate::activateAction -> hideUpToMenuBar -> hideMenu()
  -> QWidget::close()                                     (menu->close(), qmenu.cpp:621)
  -> QWindow::close() -> platformWindow->close()          (qwindow.cpp:2457)
  -> QGuiApplicationPrivate::processCloseEvent            (qguiapplication.cpp:2922)
  -> QWidgetWindow::closeEvent -> QWidgetPrivate::handleClose   (qwidgetwindow.cpp:890, qwidget.cpp:8604)
     -> PersistentMenu::setVisible(false)   refused: the one-shot flag is consumed here
  -> window close accepted -> QWidgetPrivate::setVisible(false) -> hide_helper -> Hide event
  -> editor FocusOut -> commitData(OKPRESSED unset) -> closeEditor -> menu aboutToHide
  -> QAction::toggled(Work): Qt triggers the item after closing the popup
```

Qt source checks (qt/qtbase, the tags named; read, not run):

- `v6.11.3`: `hideMenu()` closes with `menu->close()` (`qmenu.cpp:621`); `QWidgetPrivate::close()`
  routes a top-level native widget to `widgetWindow->close()` (`qwidget.cpp:8686`);
  `handleClose()` sends the close event and then calls `q->hide()` (`qwidget.cpp:8634`).
- `v6.2.4`: the same routing (`qwidget.cpp:8354`), the earliest release checked with it.
  `v6.5.3`, `v6.6.0`, `v6.7.0` and `v6.8.2` have the comment "Close native widgets via QWindow::close()".
- `v5.15.2`: `QWidget::close()` calls `close_helper()` (`qwidget.cpp:8309`), which sends the
  close event and then calls `q->hide()` (`qwidget.cpp:8247`). That virtual `hide()` was the
  only hide, so the refusal worked on Qt 5. `hideMenu()` already called `close()` there.

So the design held on Qt 5 and broke when the routing through the window came in with Qt 6.2.

## Hypothesis check

| Point in the request | Result |
|---|---|
| A second hide follows the click, and the menu closes | Confirmed. The first hide (refused) is the virtual `setVisible(false)` from the close; the second is the window hide after the close is accepted. |
| A popup dismissal follows the click | Not needed. The close is QMenu's own `hideMenu()` inside the release handling, and the traces show no outside dismissal. |
| `aboutToHide` clears the button's focus; the item view commits without OKPRESSED | Confirmed in the order of the logs: Hide, editor FocusOut, `commitData OKPRESSED=unset`, `closeEditor`, `aboutToHide`. The commit is a consequence of the hide. |
| `m_masterIndex` initialisation and `masterIndexChanged` | Checked, not involved. The index starts at -1. With "All activities" assigned, ticking Work unticks it through the handler (see the table below). |
| Stale flag (`m_blockHide`) | Real but separate: a release that QMenu does not close leaves the flag set, and the next hide (OK) is refused. The fix scopes the flag to the call. |

## Fix

- `app/settings/generic/persistentmenu.h`: `closeEvent()` override; the flag is
  `m_keepOpenForRelease` (replaces `m_blockHide`). The member comment says why it must not
  outlive the release.
- `app/settings/generic/persistentmenu.cpp`: `closeEvent()` ignores the close while the flag is
  set; `setVisible(false)` is refused in the same window; `mouseReleaseEvent()` sets the flag from
  `actionAt()` around the QMenu call and clears it afterwards. The comments state the Qt routing,
  the versions checked, the failure mode and the scope of the veto (rule 7 of AGENTS.md).
- `autotests/persistentmenutest.cpp` and its target in `autotests/CMakeLists.txt`
  (offscreen, `latte_add_offscreen_test`):
  - `checkableItemClickKeepsMenuOpen`: press and release on a checkable item, as a user does.
    The toggle fires once, the item is checked, the popup stays visible, and an explicit
    `hide()` closes it.
  - `explicitHideCloses`: `hide()` outside a release closes the popup.
  - `closeCloses`: `close()` outside a release closes the popup (the veto is scoped).

Red and green: on the previous code the first case fails with `'menu.isVisible()' returned
FALSE`; the other two pass. With the fix, all three pass.

## Behaviour with the fix (harness, offscreen and nested Wayland)

| Step | Baseline (351761a4b logic) | With the fix |
|---|---|---|
| Click Work | Menu closes; the editor commits without OK; the cell stays empty | Menu stays open; Work ticked; the editor button shows "Work" (`R7j-data/wayland/screenshots/fixed-menu-after-work.png`) |
| 确定 | Not reachable (the menu is gone) | `MODEL setData(UserRole)` with the Work id; the editor commits with `OKPRESSED=1` |
| 取消 | Not reachable | Commit without OK, nothing saved (by design) |
| Click outside the menu | Not reachable | Menu closes; commit without OK, nothing saved (by design) |
| Second click on Work | Not reachable | Work unticked; menu stays open |
| "All activities" assigned, click Work | Menu closes; both changes lost | All activities unticked by the master handler; Work ticked; menu stays open |

The logs are in `R7j-data/offscreen/` and `R7j-data/wayland/`. The Wayland runs are the
harness with the production sources (`fixed-*`) and the probe copy (`baseline-*`).

## Verification of the commit (`7a4100b6a`)

| Check | Where | Result |
| --- | --- | --- |
| Configure, build (`all` and `latte-autotests`), CTest | The four host presets: `gcc-debug`, `clang-debug`, `gcc-release`, `clang-release` (GCC 16.2.1, clang 22.1.8) | Exit 0 in each; no `warning:` line in any build log; CTest 63/63 in each, including `persistentmenutest` |
| clang-tidy gate | Host, `scripts/test-clang-tidy.py --build-dir build/modernization/clang-debug --jobs 4` | Exit 0: 156 production sources, no finding from the 2 gated checks |
| CI image | `latte-r2c-clazy:noble` (clang and clang-tidy 18.1.3), a `git archive` of `7a4100b6a` with an empty `build/` as the mount point, `clang-debug` preset | Configure and build: no `warning:` line. CTest 62/62 (the image has no `appstreamcli`, so one test less than the host). clazy gate: 49 negative fixtures rejected, no finding in 156 production sources. clang-tidy gate: 2 negative fixtures rejected, no finding in 156 production sources |

The first container attempt stopped before the build, because the export had no `build/` mount
point. The rerun used the same commit. The tools in `R7j-tools/` were then rebuilt from the
committed copies: the offscreen `ok` case and a nested Wayland `ok` run gave the results shown
above.

## Desktop check (zh_CN, Plasma 6 Wayland): not done yet

This check needs the desktop handed over by the R7 session (the desktop dock is in use there), and
an install of this branch with `install.sh --user Debug` (AGENTS.md step 1 and the launch rules in
step 5). Steps on the user's desktop:

1. Preconditions: activities 默认 and Work both running; a layout whose 活动 cell is empty, in
   the 布局 tab's 基于活动的多个布局 mode.
2. Right-click a dock → 编辑布局(&L)… → 布局 tab → 基于活动的多个布局 → click the 活动 cell of the
   layout. The menu lists 全部活动, 可用活动, the current activity, 默认 and Work.
3. Click Work. Expected: the menu stays open and Work is ticked.
4. Click 确定. Expected: the menu closes and the cell shows Work.
5. Click 应用. Expected: the layout is assigned to Work (the cell still shows Work after the
   apply; the Work dock shows this layout).
6. Optional: tick Work again and press 取消; expected: the cell is unchanged.

Also send the Qt version of the desktop: `qmake6 -query QT_VERSION` (or the version of
`libQt6Core` that `plasmashell` loads).

## Not verified

- The user's desktop: not run here. Qt versions other than 6.11.3 were not run, only read
  (5.15.2 and 6.2.4 to 6.8.2 in the sources above).
- The 应用 step of the Layouts editor and the layout switch on the real dock: the harness stops
  at the model's `setData`.
- Keyboard navigation inside the menu (Up, Down, Escape) is unchanged in the code; it was not run.
- The CI Build run for the merge: none, because nothing is pushed yet.

## Tools and data

- `R7j-tools/harness/` — `repro.cpp` (the harness) and `build.py` (builds it against the
  toolsunittest objects of a configured tree; `--variant` selects the PersistentMenu sources).
- `R7j-tools/variants/probes/` — the probe copy of PersistentMenu used for the baseline traces.
- `R7j-tools/wl/` — `run_nested.py` (nested KWin session, scenarios `none`, `ok`, `cancel`,
  `outside`, `work-again`, `R7J_ASSIGNED=all`), `fakeinput.c` and `build_fakeinput.sh`.
- `R7j-tools/gdb/` — the two trace scripts behind the backtraces.
- `R7j-data/` — the logs above, the gdb excerpt and two screenshots.

Baseline sources: `git show 351761a4b:app/settings/generic/persistentmenu.h` (and `.cpp`) put in a
directory of their own, passed to `build.py --variant`. Scratch outputs go under a directory of
your choice, never under the repository.
