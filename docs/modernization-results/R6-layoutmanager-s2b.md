# R6-S2b: migrate application calls through LayoutManagerApi

Status: **Complete.** This slice
migrates the application-side `LayoutManager` reads and calls through the
header-only `LayoutManagerApiDispatcher` added in S2a. The QML API, plugin
boundary, meta-property notification connections, and dynamic insertion-token
properties remain unchanged.

## Changes

`ContainmentInterface` now owns a dispatcher alongside its existing
`QPointer<QObject>`. `setLayoutManager()` updates both handles; the dispatcher
resolves `LayoutManagerApi` on each call and falls back to the original QObject
properties and invokable names for an older plugin without the IID. The QObject
handle remains available for QML exposure, notification connections, and
`_latte_pendingInsertionIndex`.

Application reads and commands in `ContainmentInterface`, drag and drop
handling in `View`, and context-menu actions now use the dispatcher. Existing
`QMetaProperty` notify connections remain dynamic. Context-menu callbacks keep
their guarded `QPointer<QObject>` captures and construct a short-lived
dispatcher at invocation time, so a destroyed or replaced plugin object is
never retained as a raw API pointer. `insertBefore` and `insertAfter` are only
used by QML and remain outside this application-side migration.

## Tests and verification

`pluginregistrationunittest` now loads the staged, real containment QML plugin
through `QQmlEngine`, creates its `LayoutManager`, checks the interface with
`qobject_cast`, requests an applet order through the production dispatcher,
reads the order back, and destroys the owning QML root. This verifies the IID
and meta-object across the plugin DSO boundary and checks that dispatcher reads
remain safe after QObject destruction. Existing S2a production tests cover the
typed and legacy dispatcher paths; `sourcecontracttest` now pins the migrated
root-item read.

- GCC Debug: the complete `latte-autotests` target and application/plugin
  targets built with no C++ compiler warnings or errors. Full CTest passed
  65/65 inside a transient service in the graphical-session user manager.
- Clang Debug: the same targets built with no C++ compiler warnings or errors;
  full CTest passed 65/65 in the same user-manager environment.
- Running tests from the agent shell initially stalled the offscreen
  `qmlsmoketest` and `importedcontainmentsunittest`, and sandbox restrictions
  blocked two tests from binding temporary D-Bus sockets. Running in the
  graphical-session user manager resolved these environment differences:
  `qmlsmoketest` passed 58/58 in 12.8 seconds, and
  `importedcontainmentsunittest` passed 5/5. Both D-Bus tests also passed on
  both compilers in the full session runs. Earlier bounded failures are
  retained in `/tmp/r6-s2b-*-ctest*.log`; they are not acceptance failures.
- `git diff --check` passed. No QML source changed. The install workflow emitted
  its existing QML tooling warnings, including unresolved Plasma type metadata;
  they are distinct from compiler warnings and no before/after lint baseline
  comparison was run.

## Runtime preparation

The user-mode Debug install completed after a sandboxed cleanup attempt failed
on a protected user-local QML file and the authorized retry succeeded. The
debug dock is running as PID 337105 in transient user unit
`latte-dock-ng-debug-20261117-s2b`. `/proc/337105/maps` confirms that it loaded
the user-local `liblattecontainmentplugin.so` and
`liblatteprivateappplugin.so`. Both existing containments initialized; the
log records DP-2 order `QList(43, 40, 41)` and HDMI-A-1 order
`QList(44, 46, 45)`.

The user confirmed the remaining §7.7 desktop scenarios passed, including
positioned drops, drag reordering, undo, context-menu actions, separator
add/remove, alignment and style changes, restart/session restart, clone
behavior, and modifier-held shortcut placement. The log records creation of ID 47
(`org.kde.plasma.konsoleprofiles`) and ID 48
(`org.kde.plasma.worldclock`), order changes while dragging, removal of IDs 48
and 47, then creation and removal of IDs 49 (`org.kde.plasma.systemmonitor.cpucore`),
50 (`org.kde.plasma.clipboard`), and 51 (`org.kde.plasma.volume`). The final
recorded DP-2 order is `QList(43, 40, 41)`. The user's confirmation did not
identify each context-menu action separately.

The fresh log contains 11 Warning entries and no Error, Fatal, or ASSERT:

- Two `QQmlExpression: Attempted to evaluate an expression in an invalid
  context` warnings at 22:58:03.768 and 22:58:48.405. The log does not identify
  their originating expression.
- Eight warnings at 22:58:36.124–22:58:36.259 as ID 48 was created. One says
  `plasma5support/dataengine/plasma_engine_geolocation` could not be found.
  Six name `/usr/share/plasma/plasmoids/org.kde.plasma.worldclock/contents/ui/main.qml`:
  undefined `IntervalAlignment` at line 27 (twice), undefined `marbleItem` at
  line 33 (twice), and undefined `units` at lines 47–48 (twice). Those exact
  QML expressions are in the installed system plasmoid, outside this patch.
  `QObject::disconnect: Unexpected nullptr parameter` is emitted amid Marble
  map-theme property setup and failed-theme diagnostics while the worldclock
  applet initializes. A separate Fedora Plasma 6.8.3 VM trace captured the
  call chain into the Marble QML plugin, identifying this warning as external
  to Latte. That VM trace attributes only the disconnect warning; it does not
  identify the two QQmlExpression warnings in the local interaction log.
- One warning at 22:59:33.677 says a desktop file with an empty `Exec` line
  declares URL protocols without `%u` or `%U`; the log does not identify the
  desktop file.

The original interaction log was preserved before restart as
`/tmp/latte-ng-s2b-interaction-log-20261117.log`. It records the 11 warnings
above; no additional warning, Error, Fatal, or ASSERT appears in the fresh
`/tmp/latte-ng.log` after restart. The fresh dock remains active as PID 343664
in transient user unit `latte-dock-ng-debug-20261117-s2b-restart` and loaded the
user-local containment plugin.

The user removed original containment ID 54 (`org.kde.plasma.volume`) before
the restart. The saved interaction log records original order
`QList(53, 43, 52, 40, 41)` and clone order `QList(44, 48, 47, 42, 45)` on
HDMI-A-1, with clone mapping `{53→44, 43→48, 52→47, 40→42, 41→45}`. It also
records the matching removal of clone ID 46, which mapped from original ID 54.
After restart, DP-2 restored the same original order
`QList(53, 43, 52, 40, 41)` with ID 54 absent. HDMI-A-1 is the clone in layout
`我的布局 - 2`; its regenerated IDs have mapping
`{53→45, 43→47, 52→44, 40→42, 41→46}` and order
`QList(45, 47, 44, 42, 46)`. Translating the regenerated clone IDs through
that map yields the same original order. The differing numeric clone IDs are
therefore consistent with clone reconstruction, not a persisted order change.

The S1 report recorded no runtime warnings for its different add/restart/remove
sequence. The S2a desktop check also used different applets and did not exercise
the S2b drag/context-menu call paths. Source search found no explicit
`QQmlExpression` in first-party production code. The two historical
`QQmlExpression: Attempted to evaluate an expression in an invalid context`
warnings at 22:58:03.768 and 22:58:48.405 have no captured expression or stack;
their source remains unknown and they are not claimed fixed or non-Latte.
The six worldclock QML warnings name the installed system plasmoid, and the
missing geolocation engine occurred during its initialization. The desktop-file
warning lacks a file path and remains unattributed.

On 2026-11-18, the user confirmed another dock interaction pass covering
applet drag sorting and add/remove. Diagnostic unit
`latte-dock-ng-qqlexpr-trace-20261118` (PID 349917) recorded order changes on
DP-2 and HDMI-A-1, creation/removal of brightness applets ID 55/54, and no
Warning, Error, Fatal, or ASSERT. Its targeted stack file
`/tmp/latte-warning-trace/interaction-stacks.log` remained empty, so the two
historical QQmlExpression warnings were not reproduced. Both containments
initialized and the user-local plugin loaded. The diagnostic log is
`/tmp/latte-warning-trace/latte-ng.log`; the previous ordinary debug log was
preserved at `/tmp/latte-ng-before-qqlexpr-trace-20261118.log`. No layout files
were edited by the diagnostic run.

After the diagnostic, the standard Debug launcher was restored without
`LD_PRELOAD`. Unit `latte-dock-ng-debug-20261118-after-trace` is active with PID
352015; the process has no `LD_PRELOAD` and loaded the user-local containment
plugin. Its fresh `/tmp/latte-ng.log` has no Warning, Error, Fatal, or ASSERT.
DP-2 restored `QList(43, 53, 52, 40, 41)`. HDMI-A-1 restored clone order
`QList(45, 47, 44, 46, 42)` with mapping `{43→45, 53→47, 52→44, 40→46, 41→42}`,
which maps back to the same original order.

GCC and Clang Debug full builds and 65/65 CTest pass; the user confirmed the
full §7.7 desktop matrix, and the fresh restarts restored the original order
and the mapped clone order. S2b is complete. The two historical
QQmlExpression warnings remain an unresolved observation: the diagnostic
interaction did not reproduce them, so no cause or fix is claimed. Release
builds and the Debian 13 dependency floor were not checked. The change has not
been committed; main-session review remains required.
