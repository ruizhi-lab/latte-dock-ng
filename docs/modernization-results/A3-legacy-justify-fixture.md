# A3: legacy Justify configuration fixture

The existing offscreen `containmentlayoutunittest` now supplies a legacy-style
Justify configuration through the plasmoid configuration property map. The
production layout manager restores the semicolon-separated applet order and
splitter positions, does not create legacy splitter items, then persists an
explicit reordered applet sequence through its production save path. A second
test calls the production join entry point and verifies that start, main and
end items are merged in order without creating splitters.

The fixture tests the property-map interface and production layout logic. It
does not read a saved layout from disk through Plasma containment and KConfig,
and it does not establish the behavior of all historical layouts. A trial
using a real Plasma containment hung under a private D-Bus session and was
removed; the stable test does not activate that service. No splitter branch or
other production code was changed. Disk-backed coverage, consumer tracing and
desktop Justify, separator and drag checks remain prerequisites for any legacy
branch removal.

The affected target built under GCC and Clang Debug and Release; the full
CTest suite passed 64/64 in each of the four configurations. The focused
offscreen test also passed without a session bus. Remote CI is pending.

## Desktop completion

The saved-layout gap is now covered by the real dual-screen Justify layout on
2026-11-25. Cross-area dragging and separator creation/removal synchronize on
both screens, and restart restores the complete mapped order. Separator removal
initially exposed the synchronous Justify order-request recursion documented in
[R6-S3](R6-layoutmanager-s3.md#justify-order-request-reentrancy-fix-and-final-acceptance).
The scoped reentry guard fixed that path; the user then passed the same
separator operation, another drag and restart with no new core. All four build
presets pass 68/68 CTest targets, and A3 is complete.
