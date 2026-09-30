# Modernization final desktop retest

Run this checklist after all implementation commits are installed. The Fedora
44 VM is running the user-mode Debug build from commit `c03ffa7ec` with its
Plasma Wayland environment restored. Debian 13.7 / Plasma 6.3.6 is reachable
and is running the user-mode Debug build. The user confirmed that
`PreviewWindows` now shows a preview on Debian; the other hover modes remain
untested there.

## Task icon hover choices

Set each task's hover action separately and test with a grouped application
that has at least two open windows. Wait through the configured preview delay
and move the pointer across the icon, its zoomed area and the preview surface.

| Setting | Expected result | Check that must remain off |
| --- | --- | --- |
| `PreviewWindows` | Window previews appear and follow the hovered task/icon. | Window highlighting must not activate. |
| `HighlightWindows` | Matching windows are highlighted by the compositor. | Window previews must not appear. |
| `PreviewAndHighlightWindows` | Window previews and matching-window highlighting both activate and clear on exit. | Neither effect may remain stuck after moving away or changing tasks. |

Repeat the three choices with a single-window task and a grouped task. Confirm
title tooltips still work, preview-helper failure preserves the documented
fallback, and the saved hover choice is read after relaunch. Debian's confirmed
preview result covers only the `PreviewWindows` path; the other two choices
remain pending.

## Task interactions and edit mode

- Confirm running-task dots/lines, attention indicators, launcher/window
  transitions and grouped-window counts render for the active test layout.
- Check right-click task menus, launcher places/recent actions, middle-click
  close, configured modifier clicks, drag/reorder, wheel actions and tooltips.
- Enter and exit Plasma/Latte edit mode. Repeat with the Latte bridge absent at
  startup and after it reattaches. Add and remove a task while editing, then
  verify clicks, menus, dragging and wheel actions follow the current edit
  state without a delay or stuck state.
- Exercise hide/dodge modes and overlapping hide blockers (context menu,
  preview, edit mode); verify the dock returns to the expected visibility.
- Close the preview surface, move between tasks and leave the dock; no preview
  or highlight may remain visible.

## Icon rendering and geometry

- Move the dock through top, bottom, left and right edges; use horizontal and
  vertical layouts, center and justify alignment, and the task counts available
  in the test layout.
- At integer and fractional display scale, compare task icon sharpness during
  parabolic zoom. Check for blur, clipping, stale textures, flicker or
  incorrect destination rectangles while the dock resizes and relocates.
- Where two outputs are available, move the dock between outputs with
  different DPRs and hotplug/reconnect one output. This remains unavailable on
  the single-output Fedora VM unless a second display becomes available.
- Change the desktop icon theme while a named icon is visible. Verify its
  pixels and derived colors refresh, and check overlays, disabled state and
  active state remain correct.

## Window and shutdown behavior

- Change a test window title and verify its task label/tooltip updates while
  other window geometry, activation, minimized state and task eligibility still
  update normally.
- Check edge relocation with an empty task list, then add/remove tasks during a
  rapid sequence of edge changes. Verify the latest edge and published window
  geometries win.
- Record the current `coredumpctl` baseline, quit/restart the dock cleanly, and
  confirm no new Latte core, Fatal, ASSERT or unexpected teardown warning.

## Record results

For each OS, record the Plasma/Qt/KF versions, selected hover mode, pass/fail,
any reproduction steps, and the final log scan. Useful Fedora paths are
`/home/fedora/.local/bin/latte-dock-ng` and `/tmp/latte-ng.log`; the final
Debug process already uses the captured Wayland session environment and
`~/.config/latte-dock-ng/dev-env.sh`. Do not mark cross-screen checks passed
unless that environment was actually available. Debian startup, helper
installation and the `PreviewWindows` interaction are recorded in
[`debian13-runtime.md`](debian13-runtime.md); retain the separate highlight
checks above as pending.
