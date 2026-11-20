# R6-S5: measured, not implemented

Status: **Evaluation only, no source change.** The design study scheduled S5
(typed QML pushes for the drag path) behind a profile, and the profile does not
support it. This record keeps the measurement so the slice is not re-proposed
from the same static reading.

## What S5 proposed

[R6 design §7.5](R6-layoutmanager-design.md) planned typed `Q_PROPERTY` values
with `NOTIFY` on `LayoutManager` — `viewAlignment`, `formFactor`, the layout
spacings and the view object — bound in `containment/package/contents/ui/main.qml`,
to replace five name-based reads:

- `readConfigValue("alignment")`: `m_rootItem->property("myView")` then
  `myView->property("alignment")`;
- `insertAtLayoutCoordinates()`: `formFactor`, `rowSpacing`, `columnSpacing`.

The premise came from design §4.3, which marks `readConfigValue()` as
"**HOT when reached from `insertAtCoordinates`**".

## Premise checked against the code

| Claim from the design | Actual | Evidence |
| --- | --- | --- |
| `insertAtLayoutCoordinates()` reaches `readConfigValue()` | It does not | The whole function body contains zero `readConfigValue(` calls; it reads `formFactor`, `rowSpacing` and `columnSpacing` directly |
| `readConfigValue("alignment")` is on the drag path | Its only caller is `requestAppletsOrder()` | `layoutmanager.cpp:1729`; the other five callers are `restore()`/`saveOption()`/`restoreOption()` paths (`:903`, `:1131`, `:1303`, `:1412`, `:1413`) |
| An applet drag reaches `insertAtCoordinates()` | Only a drop *from the widget explorer* does | `AppletItem.qml` calls `fastLayoutManager.insertBefore()`/`insertAfter()` for drag sorting; `insertAtCoordinates()` is called from `DragDropArea.qml` (external drop) and `View::handlePlasmoidDrop()` |

## Measurement

A temporary `QElapsedTimer` probe wrapped the two access groups and the Debug
user installation was run with `--debug`. The user moved several widgets on the
main dock.

| Probe | Samples | Result |
| --- | --- | --- |
| `readConfigValue("alignment")` incl. both name-based reads | 6 | min 5.26 µs, median **5.71 µs**, max 8.82 µs |
| `insertAtLayoutCoordinates()` name reads (`formFactor`, `rowSpacing`, `columnSpacing`) | **0** | not reached by an applet drag at all |

The 5.71 µs figure is real but is paid on an order change, not per drag move.
Both probes were removed afterwards; `git diff containment/plugin/layoutmanager.cpp`
is empty and the user installation was uninstalled again.

## Verdict

Do not implement S5. By the design's own rule (§9: implement only when the
profile shows the name lookups among the top samples of the drag path), the
slice has no measured benefit to claim: the hot path it targets is not executed
by the interaction it was named after, and the one measured access group is
event-driven.

The behaviour S5 would have touched is unchanged. `readConfigValue()` keeps the
live-first order (comment at `layoutmanager.cpp` about KConfig holding the
previous alignment in the same event turn), which S5 would have had to preserve
anyway.

## What the profile did find

The actual drag path is a different family: `AppletItem.qml` sorting calls
`fastLayoutManager.insertBefore()`/`insertAfter()` by name per drag commit, and
`createAppletItem()`/`initAppletContainer()` are invoked by name from the
manager. Those belong to Family 3 of the design (QML functions called from C++,
plus C++ calling QML invokables), which is **S7**, not S5. S7 stays large
(QML root type and creation ownership) and is not started.

S6 (a typed layout-item base type) is independent of this measurement; it
addresses the `isInternalViewSplitter` / `isParabolicEdgeSpacer` /
`backendAppletRef` reads on layout children, which this profile did not cover.
