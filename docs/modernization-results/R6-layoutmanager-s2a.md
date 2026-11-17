# R6 LayoutManager S2a: typed API with legacy QObject fallback

Status: **Complete.** The implementation, focused tests, host plugin load and user desktop confirmation passed. This preparatory slice establishes the typed interface and a compatibility dispatcher. It does not migrate application call sites; that work is reserved for S2b.

## Change

`LayoutManagerApi` is a header-only C++ contract in the containment plugin. `LayoutManager` implements it with `Q_INTERFACES` and `override` methods. The API covers the getters and commands needed by application-side containment, drag/drop and applet-menu code. The existing QML module URI, registered type name, properties, invokables and installation path are unchanged.

`LayoutManagerApiDispatcher` holds the manager as a `QPointer<QObject>` and resolves the interface on each operation. It calls the typed virtual method when the IID is present. If a QML plugin is loaded from another installation root and implements the older QObject property/invokable contract without the IID, it dispatches through those existing names instead. The `QPointer` remains the lifetime authority: after destruction getters return the same empty defaults used for absent QObject properties, commands become no-ops as before, and replacing the target redirects later calls to the new object.

The IID currently identifies this source-level interface version; any future incompatible method/signature change must coordinate an IID version change with the plugin and application. The unit test exercises qobject_cast and virtual dispatch in one test executable. The desktop run confirms the real QML plugin loads, but the application does not yet call through the new interface. Cross-DSO typed dispatch therefore remains unverified and must be checked in S2b after production call sites use the dispatcher.

The dispatcher is not consumed by production call sites in this slice. That is deliberate: S2a validates the boundary and compatibility behavior independently; S2b will migrate call sites through this dispatcher. No QML files or production application call sites changed.

## Production tests

`containmentlayoutunittest` exercises the real `LayoutManager` through the IID path and a `LegacyLayoutManagerObject` fixture that deliberately does not implement the interface. The legacy fixture verifies all exposed property families and command families: order and option updates, applet add/remove/hide/show, drag coordinates and spacer index, zoom/coloring lists, and scheduled-destruction state. The same test deletes the old target, checks safe defaults, installs a replacement target and confirms subsequent reads come from it.

The test does not instantiate `ContainmentInterface`; its constructor requires a live `View`. No cached interface pointer is introduced here, so this slice does not claim to test ContainmentInterface lifecycle behavior.

`insertBefore()` and `insertAfter()` remain QML invokables and are not part of this application-side interface slice: there are no C++ consumers under `app/`; the current consumers are in `main.qml` and `AppletItem.qml`.

## Verification

- GCC Debug: built `lattecontainmentplugin` and `containmentlayoutunittest`; the focused CTest target passed.
- Clang Debug: built the same targets; the focused CTest target passed.
- The final build logs contain no C++ compiler warnings or errors.
- `git diff --check` passed.
- Full CTest, Release builds and Debian 13 floor verification were not run. The user-mode install build ran its integrated QML lint and emitted 5,370 `Warning:` lines across QML files; no QML changed in this slice, and no before/after lint baseline comparison was run. The final GCC/Clang focused build logs had no C++ compiler warnings or errors. S2b's application call-site migration owns the broader S2 desktop retest list.

## Host plugin load

The user-mode Debug install completed at `/home/ruizhi/.local`. The previous
Debug dock was stopped with `pkill -x`, the old log was cleared, and the new
binary was launched in transient user unit
`latte-dock-ng-debug-20261117-s2a` with `PartOf=` and `After=` bound to
`graphical-session.target`. The unit was active with PID 317980 when checked;
`/proc/317980/exe` resolves to `/home/ruizhi/.local/bin/latte-dock-ng` and its
memory map contains
`/home/ruizhi/.local/lib64/qt6/qml/org/kde/latte/private/containment/liblattecontainmentplugin.so`.
This confirms the running host loaded the newly installed QML plugin from the
same user prefix. Both containment screens reached `initializationCompleted`.
The fresh `/tmp/latte-ng.log` had no runtime Warning, Error, Fatal, ASSERT,
missing-method or QML-load failure entries at inspection time. The user
confirmed that the dock was visible and interactive, and added and removed an
applet. The log records applet creation for ID 47
(`org.kde.plasma.konsoleprofiles`) and ID 48
(`org.kde.plasma.systemmonitor.cpucore`), followed by removal of ID 47. The
final DP-2 order is `QList(43, 48, 40, 41)`; the other containment remained at
`QList(44, 46, 45)`. The unit remained active at the final check.

This desktop interaction validates normal QML manager startup and add/remove
behavior, but does not exercise the new dispatcher, which S2b has not
connected to production call sites. A post-interaction restart was not
performed in this S2a check.

No commit or push was made in this handoff because the main session requested review first. The user authorized automatic commit and push after each phase passes and review is complete.
