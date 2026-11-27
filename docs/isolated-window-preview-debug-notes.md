# Isolated Window Preview — Implementation & Debug Notes

This guide records the preview helper protocol and the root causes of runtime
failures. Use the [development testing guide](development-testing-guide.md) for
compiler presets, automated tests, and desktop retests.

## Architecture

```
latte-dock-ng (dock process)
    |  QProcess, bounded JSON-lines, no image data
    v
latte-dock-ng-preview (helper process)
    |  QQuickView + LayerShellQt overlay
    v
PipeWire / Plasma screencasting (asynchronous, per-window)
```

- The dock stays authoritative for task state, window identity and activation.
- The helper owns the preview surface, capture objects and interaction.
- The task hover-action setting is the only user-facing feature gate;
  `showPreviews` stays `false` in QML so the legacy in-process scene can never
  load. Selecting Preview Windows or Preview and Highlight Windows starts the
  isolated helper lazily on the first eligible hover.

## Protocol

One JSON object per line. The dock rejects malformed/oversized frames; the
helper ignores them and lets the dock watchdog decide.

Dock to helper:

- `show`: `serial`, `windows[]` (`uuid`, `appName`, `title`, `launcherUrl`,
  `appPid`, `minimized`), `x`, `y`, `width`, `height`, `edge`.
- `move`: `serial`, `x`, `y`, `width`, `height`, `edge` (geometry only).
- `hide`: `serial`.

Helper to dock:

- `heartbeat`: `serial`, `visible`, `hovered` (also acts as the watchdog ping).
- `activate`: `serial`, `uuid`.
- `close`: `serial`, `uuid`.
- `closed`: `serial` (announced on clean shutdown).

Bounds: 64 KiB frames, at most nine windows, UUIDs validated with `QUuid`,
geometry numbers bounded, `edge` restricted to Plasma dock locations. Stale
generations are ignored; activation is re-checked against the last window set.

## Lifecycle / fail-closed

- Helper starts lazily on the first hover; startup and heartbeats share a
  2.5 s watchdog.
- A single failure hides the preview for the current hover only. Three
  consecutive failures close the session gate to avoid a restart loop.
- An idle-lease exit (30 s) is a normal shutdown; the next hover starts a
  fresh helper.
- Fail-closed sequence: `failure/timeout -> hide -> title tooltip`.

## Runtime debug log (chronological)

### 1. No preview appeared — xdg_popup without transientParent

Symptom in the helper's forwarded stderr:

```
qt.qpa.wayland: Failed to create popup. Ensure popup PreviewDialog(...) has a transientParent set.
```

Cause: the helper used `PlasmaQuick::Dialog` with `Qt::ToolTip`. Qt Wayland then
creates an `xdg_popup`, which requires a `transientParent` on the same Wayland
connection. A separate process cannot reference the dock's `wl_surface`, so the
popup role is structurally impossible cross-process. `PlasmaQuick`'s
`adjustGeometry()` only positions such a surface because it is a popup; for a
plain toplevel the compositor ignores the requested position (verified with a
KWin script: the window was placed at 0,0).

Fix: the helper is a `QQuickView` configured as a **LayerShellQt** overlay
(`LayerOverlay`, `KeyboardInteractivityNone`, `exclusiveZone=-1`, anchors
`Top|Left`). Position is expressed as layer-shell margins, which KWin honours.
The helper is Wayland-only and must not reintroduce direct `QWindow` positioning:
KWin ignores it for the layer-shell surface even when Qt reports the requested
coordinates.

Verification (KWin window dump):

```
cap=latte-dock-ng-preview x=499 y=514 w=248 h=178 layer=9
```

for an anchor of `(600,700,48,48)` on the bottom edge: correct size and
position.

### 2. Preview appeared late and did not follow icon switching

Cause A: the show delay reused `plasmoid.configuration.previewsDelay`, whose
default is 650 ms (tuned for the legacy in-process scene).
Cause B: every task change called `hide()` and re-created the surface, so
switching paid the full delay plus a remap.

Fix:
- Clamp the isolated-preview show delay to 150-250 ms.
- When a preview is already visible, switch the target task immediately and
  update in place (`show` with the new selection) instead of hiding.
- `onIsolatedPreviewTaskChanged` only hides when the selection becomes empty.
- Removed the per-message pulse so unchanged requests are not re-sent.

### 3. Preview did not follow the parabolic icon animation

Cause: geometry was refreshed only by the 100 ms poll.

Fix: a `FrameAnimation` calls a new `move()` on the dock side every rendered
frame; `move` sends a geometry-only message that reuses the current serial and
does not re-serialize the window list. The helper applies it to the existing
selection.

### 4. Move messages were accepted but the surface stayed put (root cause)

`WAYLAND_DEBUG=1` trace of a `show` followed by a `move`:

```
-> zwlr_layer_surface_v1#53.set_margin(514, 0, 0, 499)
-> wl_surface#48.commit()                       (show: repaint damages the surface)
-> zwlr_layer_surface_v1#53.set_margin(214, 0, 0, 1099)
                                                (no commit for ~2.4 s)
```

A layer-shell margin change does not damage the Qt Quick scene graph, so Qt
never schedules a frame and never commits the `wl_surface`. The compositor
therefore keeps the previous position until some unrelated event (window
resize, close) forces a commit. `QWindow::requestUpdate()` was not enough; it
schedules a frame but Qt still skips the render because nothing is dirty.

Fix: after `LayerShellQt::Window::setMargins()`, call `QQuickView::update()` to
force a repaint. Trace after the fix:

```
-> zwlr_layer_surface_v1#53.set_margin(214, 0, 0, 1099)
-> wl_surface#48.commit()                       (0.4 ms later)
```

This also explains why task switches (window-list change -> repaint) moved the
surface while in-icon zoom moves did not.

### 5. Match the Plasma 6 task-manager preview layout

The initial helper UI was only a diagnostic card: its thumbnail was above the
title, every card drew a separate rounded rectangle and the transparent helper
surface had no Plasma dialog background. It did not resemble Plasma 6's task
preview even after positioning worked.

The helper now follows Plasma 6.7's `ToolTipInstance.qml` geometry and visual
structure: a `16 * gridUnit` card, application name and two-line window title
above an `8 * gridUnit` thumbnail, a close button, themed hover highlight and
thumbnail shadow. Group members flow horizontally for horizontal docks and
vertically for vertical docks. Because the layer-shell helper is not a
`PlasmaQuick::Dialog`, it explicitly draws the Plasma `widgets/background`
frame around the shared preview surface. Close requests cross the same bounded
protocol and are revalidated by UUID against the current task model.

The standalone helper also needs its own hidden
`org.kde.latte-dock.preview.desktop` entry and must set that desktop identity
before constructing `QApplication`. KWin 6.7 authorizes privileged Wayland
interfaces by resolving the client PID's executable path and finding a desktop
entry whose `Exec` resolves to that exact path. Reusing the main Latte desktop
identity is insufficient because its `Exec` points at `latte-dock-ng`, not
`latte-dock-ng-preview`. Without the matching entry KWin withholds
`zkde_screencast_unstable_v1`; the helper surface and placeholder work, but
every live capture request fails.

Loading PlasmaCore in a bare `QQuickView` also initialized its KI18n QML
context from the QML type-loader thread, producing
`QObject::installEventFilter(): Cannot filter events for objects in a different
thread.` The helper creates and installs `KLocalizedQmlContext` on the engine
from the GUI thread before importing Plasma QML. This keeps startup warning-free
while preserving translated Plasma components.

A bare helper also starts with the normal application palette, while the
`widgets/background` SVG follows the Plasma theme. With a dark Plasma theme
this produced a correct dark popup frame but nearly black title text and
buttons, making the entire header appear absent. Applying
`Plasma::Theme::palette()` to `QApplication` is both insufficient and changes
the colorized SVG variant, which can turn the already-correct dark popup light.
The helper leaves the application palette alone, injects only
`Plasma::Theme::TextColor` into the root and refreshes it on theme changes;
labels and symbolic button icons use that color explicitly, matching the
foreground normally propagated by `PlasmaQuick::Dialog`.

The window descriptor also carries the launcher's URL and application PID. A
helper-local `Mpris2Model` uses the same `playerForLauncherUrl()` lookup as the
Plasma 6 task manager, and shows track/artist plus previous, play/pause and next
controls below the thumbnail for the first window in the group. MPRIS commands
go directly to the session bus; window activation and closing remain dock-owned
and cross the validated helper protocol.

## Diagnostics that helped

- KWin script logging `workspace.windowList()` geometry, loaded through
  `org.kde.KWin /Scripting`; useful to observe real compositor placement.
- `WAYLAND_DEBUG=1` on the helper filtered for `set_margin` and `commit`.
- Temporary `console.log`/`qWarning` of the computed anchor and move serials
  while FrameAnimation ran.

## Verification checklist

- Build the dock, preview helper and tasks plugin with GCC and Clang. Run the
  preview process test and source contracts before the full CTest suites.
- Confirm that the default hover action leaves previews disabled. Enable a
  preview action and check single-window, grouped and minimized tasks, rapid
  hover transitions, window closure and dock exit with the preview visible.
  Check that helper termination or capture failure restores the title tooltip
  and leaves the dock responsive.
- `sourcecontracttest` protects: hover-setting gating, protocol types, bounded frames,
  LayerShellQt margins, FrameAnimation, and `moveIsolatedPreview`.
- `previewprocessunittest` drives the real asynchronous `QProcess` transport
  against a fake helper and covers the disabled gate, UUID validation, close,
  activation, and geometry-only heartbeat state updates.
- Standalone helper protocol test: heartbeats, show/hide visibility, malformed
  input tolerance, unknown-UUID rejection, clean exit on stdin close.
- KWin placement: `show` and `move` reposition the layer surface.

## Still open

- Interactive retest of the four dock edges, auto-hide, multi-monitor and rapid
  transitions remains manual (no synthetic pointer input available in this
  environment).
- PipeWire capture authorization failure paths are exercised only through the
  placeholder path so far.
