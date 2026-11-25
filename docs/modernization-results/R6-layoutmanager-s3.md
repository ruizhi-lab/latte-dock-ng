# R6 LayoutManager S3: retire legacy Justify splitter branches

Status: **Complete.** This slice follows the consumer trace in
`R6-layoutmanager-design.md` and preserves the live Justify migration entry
points and persisted layout compatibility.

## Scope and preserved contracts

The S3 work removes the branches that created or positioned legacy Justify
splitters while `usesLegacyJustifySplitters()` is always false. It simplifies
`updateOrder()`, `restore()`, `save()`, `dndSpacerIndex()`,
`insertAtCoordinates()`, `requestAppletsOrder()` and indexed `addAppletItem()`;
removes the no-op splitter reorder helpers and private first/last splitter
lookup functions; and turns `addJustifySplittersInMainLayout()` and
`moveAppletsBasedOnJustifyAlignment()` into compatibility entry points that
join the three layouts into main.

The work keeps these behaviors and interfaces:

- `requestAppletsOrder()` still joins start/end layouts and saves when alignment
  is Justify. `AppletItem.qml` still calls
  `moveAppletsBasedOnJustifyAlignment()` after classic Justify drag sorting;
  that invokable now forwards to the same join path.
- `addJustifySplittersInMainLayout()` and the invokable
  `usesLegacyJustifySplitters()` remain available for possible separately
  installed QML/plugin clients. The former retains its false-path join.
- `joinLayoutsToMainLayout()` and `destroyJustifySplitters()` remain active for
  style/alignment changes and cleanup of already-created legacy items.
  `isJustifySplitter()` remains used when inserting at layout edges so a stale
  legacy item does not displace applets or parabolic spacers during migration.
- Existing `appletOrder` values with `-1` splitter markers still restore in
  applet order, then the normal save path removes those transient markers.
  `splitterPosition` and `splitterPosition2` remain in the persisted format and
  are carried through by `save()` rather than recomputed from the single flow.
- The C++ root contract now diagnoses only the two functions still required by
  production: `createAppletItem(QVariant)` and
  `initAppletContainer(QVariant,QVariant)`. The QML
  `createJustifySplitter()` factory and its count helper remain temporarily for
  mixed upgrades where an older, separately installed containment plugin may
  still call that root function by name. The current C++ plugin no longer
  resolves or invokes it.
- `isInternalViewSplitter` handling stays in QML/C++ migration and item sizing;
  S3 does not remove separator, spacer, or old transient-item cleanup logic.

## Slice history

The already committed slices on this branch are:

| Commit | Slice |
| --- | --- |
| `f341516ceadf1f4dafc18a10235a1d56eb068a7b` | Remove unreachable splitter insertion from `updateOrder()`. |
| `64c4b8ed9519c5ab7efd865de6360ce5fe0230bc` | Remove the two no-op start/end splitter reorder helpers and their calls. |
| `e6dda41890ba7b9e9ceb3b1ef21fcb8df80aae3e` | Remove legacy splitter creation/repartition from restore while retaining old `-1` markers and splitter config keys. |
| `3324e560ca521bc6c907d08f62010e7254ba5136` | Remove legacy splitter position recomputation from save; keep persisted positions authoritative. |
| `f2cfde99ef2a70f57d6039f6ea7389b8ae362cf9` | Remove dead side-layout branches from DND spacer index and coordinate insertion while preserving main-layout geometry and distance tie behavior. |
| `ce5e17d13d832419ce3d383c551441d39522b540` | Remove dead splitter handling from order requests/indexed applet insertion while retaining Justify join+save. |

The final C++/QML contract cleanup in the current working-tree diff removes
`m_createJustifySplitterMethod`, its root lookup/diagnostic and the unreachable
splitter implementations. It keeps the public QML entry points above and the
QML factory for older-plugin compatibility. It also updates the S1 production
test expectations so missing-root diagnostics cover exactly the two currently
required functions.

## Verification

- GCC Debug: built `lattecontainmentplugin`, `containmentlayoutunittest` and
  `sourcecontracttest`; CTest targets `containmentlayoutunittest` and
  `sourcecontracttest` passed 2/2.
- Clang Release: built the same targets; those two CTest targets passed 2/2.
- `bash scripts/qmllint.sh`: passed all 244 QML files.
- `git diff --check`: passed.
- The Clang target generation emitted the existing unrelated QML type warning
  `dialog.h:25: PlasmaQuick::Dialog is used as base type but cannot be found`;
  it did not fail the build and is not caused by this slice.
- The final user-mode Debug build was installed and restarted through the active
  graphical-session user manager as transient unit
  `latte-dock-ng-debug-20261118-204548.service`. At startup both containments
  initialized: DP-2 order `QList(53,43,61,59,56,58,57,40,41)` and HDMI-A-1 order
  `QList(44,50,46,49,47,45,42,48,51)`. The fresh `/tmp/latte-ng.log` had no
  Warning, Error, Fatal, `QQmlExpression`, `QObject::disconnect`, missing
  required root method, or failed root invocation matches. The order and IDs
  differ from earlier session snapshots; the agent cannot determine whether
  those changes match the user's expected visual order without manual review.

## Desktop retest

The user confirmed that reordering synchronized across both screens. The
Debug log recorded three paired order updates on DP-2 and HDMI-A-1 without
runtime warnings or errors. The next slice is S4, the typed one-shot insertion
protocol.

## Justify order-request reentrancy fix and final acceptance

The final Justify desktop matrix passed alignment and cross-area dragging, but
separator removal crashed PID 352199 at 20:30:58 on 2026-11-25. Its native
backtrace repeats `ContainmentInterface::updateAppletsOrder()` through
`setAppletsOrder()`, `LayoutManager::requestAppletsOrder()`, `save()` and
`updateOrder()`. QML then reported `Maximum call stack size exceeded` in
`AppletItem.qml` and containment `main.qml`.

`requestAppletsOrder()` now rejects synchronous reentry with a scoped guard.
The outer request still arranges the layout and saves it. A production-path
regression reproduces the requested/visual order oscillation behind the crash:
before the guard, a bounded callback published ten nested orders instead of
the expected two. After the guard, the containment suite passes 37/37 and the
source-contract suite passes 235/235. All four GCC/Clang Debug/Release presets
pass 68/68 CTest targets without new C/C++ compiler warnings.

For the desktop A/B, the same separator add/remove operation that crashed the
pre-fix binary passed every step on the fixed Debug build, as did another
cross-area drag and restart. No new core appeared. The final Justify layout
restored eight mapped applet pairs, including the boundary separator, with no
recursive-order errors. Two startup warnings came from Plasma's external
brightness `PopupDialog.qml`, not this change. A3's saved Justify, separator
and drag acceptance is complete.
