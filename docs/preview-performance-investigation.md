# Preview Popup Performance Investigation

## Conclusion

**The window preview popup in Latte Dock causes rendering stutter on Wayland/KWin, and this is a fundamental platform limitation rather than a code bug.**

### Root Cause

The standard Plasma task manager (`plasmashell`) runs inside the plasma shell process. KWin grants its `PlasmaCore.ToolTipArea` popups special compositor scheduling priority via the plasma-shell protocol. 

Latte Dock is an independent process. Even when using the identical `PlasmaCore.ToolTipArea` component, KWin treats its popup as a standard xdg-popup surface. When both the dock window and popup window are simultaneously visible, KWin's GPU scheduling causes frame drops in the dock — the continuous "stuttering" observed.

### What Was Tried (all failed to eliminate stutter)

| Approach | Result |
|----------|--------|
| Disable MouseArea events during parabolic animation | ✅ Smooth animation, but stutter when dialog appears |
| Wait for animation to finish before showing dialog | ❌ Same stutter (dialog visible = stutter) |
| Dialog always visible (opacity toggle only) | ❌ Same stutter |
| Inline Item inside dock window (no separate popup) | ❌ Clipped by compositor (plasma-shell surface bounds) |
| Inline Item expanding dock window height | ❌ Same stutter (window resize = re-render) |
| `PlasmaCore.ToolTipArea` (matching plasma-desktop approach) | ❌ Same stutter (separate process) |
| Async Loader for content (`asynchronous: true`) | ❌ Same stutter |
| Minimal content (red rectangle, 300×200) | ❌ Same stutter (inline test inconclusive — implementation issues) |

### Only Effective Mitigation

`hoverEnabled: !inAnimation && !inBlockingAnimation` in `TaskMouseArea.qml`

This prevents the preview trigger from firing during the parabolic zoom animation period. The animation itself plays smoothly. The dialog appears afterward, at which point there is no animation to stutter.

Applied in: `plasmoid/package/contents/ui/task/TaskMouseArea.qml`

### Reference

Standard task manager source: `/data/projects/plasma-desktop/applets/taskmanager/qml/`
- Uses `PlasmaCore.ToolTipArea` (C++ component in plasma-framework)
- Content loaded via `Loader` with `asynchronous: true`
- Runs within plasmashell process — gets special compositor treatment
