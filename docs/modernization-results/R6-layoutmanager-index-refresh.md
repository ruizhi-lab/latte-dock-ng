# R6 containment applet index refresh

The [factory profile](R6-layoutmanager-factory-profile.md) attributed most of
the measured factory time to attaching the Plasma item and setting its
anchors. A follow-up candidate created the Latte wrapper directly in the
main grid during startup, removing its intermediate parent. The stock and
candidate factories used the same instrumented GCC Release containment plugin,
binary and two-dock layout, with six applets per dock in isolated KWin. The
debug pilot restored both expected orders. Twelve alternating pairs after a
warmup pair passed the applet count, expected plugin, clean exit and KSycoca
checks. The paired creation and attachment delta was +0.34 ms (95% bootstrap
interval −4.67 to +11.77 ms), and process launch to both restores was +5.80 ms
(−43.15 to +21.35 ms). The candidate remains an experiment.

## Index update

Each `AppletItem` calculates its index from siblings in its actual visual
parent. Items outside the three dock grids have index −1. The lookup still
excludes parabolic spacers and legacy internal splitters and retains the
grid's start, main or end index range. The method publishes the computed
value once. The old method scanned all three grids and published −1 before
reassigning a valid value, briefly invalidating consumers even when the
position was unchanged.

Three before/after launch pairs used the same nested two-output fixture with
debug logging and counted actual `indexChanged` notifications. Each stock
launch emitted 36 notifications, including 12 transient invalidations.
Each changed launch emitted 12 notifications, all for the initial valid
indices. Both applet orders matched on every run. This measures notification
work separately from wall time.

The focused QML test checks all three grids, spacer and splitter exclusion,
movement between grids, stacking order within a grid, and temporary removal
from the indexed grids. It calls the production index method and observes
its change signal. The old method fails the stable refresh case by emitting
two additional notifications. The changed method passes.

Twelve separate Release pairs with the index candidate, using the same
parent-only scan before the final QML type-resolution adjustment, measured
a median paired change of −3.35 ms from process launch to both restores
(95% bootstrap interval −13.91 to +7.11 ms). This interval does not establish
an overall startup improvement. The final implementation also exercised the
real LayoutManager in the isolated session. Moving two neighboring applets
swapped indices 100 and 101, a second refresh preserved both values, and
saving reported the expected new order.

## Validation

GCC and Clang Debug and Release each built with zero compiler diagnostics
and passed 68/68 CTest targets. Syntax lint passed all 244 QML files. Deep
lint against the previous local Qt 6.11.3 report shows no added diagnostics
and nine fewer. The checked-in Qt 6.11.1 comparison still requires a matching
tool fingerprint.

The user-mode Debug dock restarted in the graphical session with the same
primary order and no warning or error in the startup log. The clone
published a different order, consistent with the instability already
[reproduced on the previous factory](R6-layoutmanager-factory-profile.md#desktop-and-clone-order-follow-up).

The user confirmed normal widget reorder and hover scaling on the primary
Dock. The subsequent Debug log had no warning or error.
