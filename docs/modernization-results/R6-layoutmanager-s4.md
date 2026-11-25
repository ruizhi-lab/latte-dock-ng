# R6 LayoutManager S4: typed one-shot insertion protocol

Status: **Complete.** This
slice replaces the two dynamic properties that carried the pending insertion
index and the parabolic-spacer suppression with typed members of
`LayoutManagerApi`. It closes the stale-token hazard recorded in
[R6 §5 family 4](R6-layoutmanager-design.md) and is the S4 slice the ledger's
next action names.

## Problem

`_latte_pendingInsertionIndex` was a one-shot token written before a
synchronous `createApplet()` and consumed by the next `addAppletItem()`.
Nothing owned the token's lifetime: `app/view/view.cpp` set it, but the only
clears were the consumer and the caller of a *different* code path
(`addInternalSeparatorBeforeApplet`, `onAppletAdded`, `QML onAppletAdded`).
A creation that reported no applet left the index behind, and the next applet
added anywhere was placed at that stale slot. `_latte_skipSpacerReorder` had
the same shape in `ContainmentInterface::addInternalSeparatorBeforeApplet`.

Both were also invisible to the type system: C++ wrote them with
`QObject::setProperty()` and QML read and cleared them by name.

## Change

- `LayoutManagerApi` (IID now `.../1.1`) declares three members:
  `expectAppletInsertionAt(int)`, `takeAppletInsertionIndex()` and
  `expectSkipSpacerReorderOnce()`. `LayoutManager` keeps them in private state
  (`m_pendingInsertionIndex`, `m_skipSpacerReorderOnce`) instead of dynamic
  properties. A negative `expectAppletInsertionAt()` is the documented
  "drop the request" form.
- `addAppletItem()` (both overloads) and `save()` consume the token exactly
  once; `save()` clears the spacer suppression after the first call, so an
  unrelated later save still reorders the parabolic spacers.
- `LayoutManagerApiDispatcher` is the only writer. For a plugin without the
  current IID it keeps the previous dynamic-property behaviour, so an older
  containment plugin from another installation root still receives requests.
  The dispatcher already resolves the interface per call, so a still-installed
  `1.0` plugin falls into that fallback instead of binding to changed vtable
  slots.
- `ContainmentInterface`, `View::handlePlasmoidDrop()` and
  `containment/package/contents/ui/main.qml` now use the typed calls. Both
  creation sites drop an unconsumed request after `createApplet()` returns.
- Comments beside the code record the ownership rule and the failure mode the
  token prevents, per AGENTS.md.

Unchanged: the QML module URI, the registered `LayoutManager` type, every QML
property and existing invokable, persisted configuration keys, the retry
timers, the layout merge behaviour and the 30 ms/300 ms separator reschedules.
`ContainmentInterface::setLayoutManager()` keeps its `QMetaProperty` notify
connections, because the interface cannot declare signals.

Known limitation: QML has no dynamic-property fallback any more. A dropped
Widget Explorer entry on top of a pre-S4 containment plugin skips the
position override inside `addAppletItem()` (the applet is still created by the
same call chain, at the default slot). Mixed installations of application and
plugin are the upgrade path this costs; a full reinstall restores the typed
path.

## Tests

`containmentlayoutunittest` gained three production-path cases (29 total):

- `insertionRequestIsConsumedOnce`: a take clears the request, and a second
  expect replaces the first instead of queueing.
- `droppedInsertionRequestDoesNotDisplaceNextApplet`: after a drop that
  reported no applet, the next applet lands at the default index; a live
  request still wins over it.
- `skipSpacerReorderRequestIsConsumedByOneSave`: the first save after the
  request leaves the parabolic spacers in place, the next one reorders them.

The legacy fixture now emulates the pre-S4 dynamic properties, so the
dispatcher's compatibility path is asserted rather than assumed.
`sourcecontracttest::insertionRequestIsATypedOneShotProtocol` pins both ends of
the boundary: the manager no longer touches the old names, the application
callers reach the protocol only through the dispatcher, and the QML handler
consumes it with the typed invokable.

Mutation checks, each rebuilt and run: a manager that ignores the spacer
request fails the save test; a manager that ignores the negative expect fails
the stale-index test; a dispatcher that always returns `-1` on the legacy
path fails the dispatcher test.

## Verification

- GCC Debug and Clang Debug: full builds of the application, both QML plugins
  and the autotests with zero compiler warnings and zero errors. The only
  `console.warn` matches in the build log come from QML sources echoed by the
  autogen step, not from a compiler.
- Full CTest: 65/65 on both configurations.
- `scripts/qmllint.sh`: 244 files, gate OK (errors-only gate; no errors).
- `git diff --check`: passed.
- User-mode Debug install completed; the dock was started as transient unit
  `latte-dock-ng-debug-s4-20261120-183807.service` bound to
  `graphical-session.target`. Both containments initialized with the expected
  orders (DP-1 `QList(4, 2, 3)`, DP-3 `QList(42, 41, 40)`), and the fresh
  `/tmp/latte-ng.log` had no Warning, Error, Fatal, ASSERT, "No such method" or
  missing-root-function entries.

## Separator sync on a mirrored dock is a pre-existing defect

During the desktop retest the user reported that a separator added between the
task manager and the widgets does not appear on the mirrored dock. This slice
does not cause it. The system package `kde-misc/latte-dock-ng-1.3.4`
(`/usr/bin/latte-dock-ng` with `/usr/lib64`'s containment plugin, no user-mode
overlay) reproduces the same sequence without any S4 code:

```
appletCreated containment 1   id 72  separator
appletCreated suppressed containment 12   id 73  separator
removeApplet  containment 12  id 73          <- the mirrored divider is destroyed at once
```

The containment cleanup also persists clone IDs that no longer exist (a
`73` written into `appletOrder` with no matching applet section). Both belong to
the clone synchronisation and persistence paths, not to the typed insertion
protocol. An attempt to fix them was reverted; it is not part of this slice.

At that stage the desktop retest item "separators follow on both screens"
could not pass. Commit `80cf37c20` later propagated insertion through the
mapped neighboring applet; the final acceptance below supersedes this interim
result.

## Final desktop acceptance

On 2026-11-25 the user passed Widget Explorer creation and positioned drops,
Left/Center/Right reordering, delete/undo on both screens, applet context-menu
actions, and launcher/file drops. An initial boundary-separator mismatch was
not reproduced after the requested dock restart. The subsequent log records
synchronized deletion of separator IDs 68/42 and creation of IDs 78/79 on the
two screens. The final saved orders place both separators at the same logical
index.

The restart restored all six logical applet pairs without duplicates. The full
post-restart interaction log contains no warning, error, fatal or assertion
entries. Current-tree clazy and clang-tidy acceptance is recorded in the main
plan and execution queue.

## Remaining limits

- The pre-S4-plugin QML path described above was not exercised on a real mixed
  installation.
- A full session logout/login was not separately tested in this acceptance;
  the user requested and completed a dock restart instead.
