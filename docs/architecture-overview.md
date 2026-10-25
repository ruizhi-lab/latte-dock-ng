# Architecture Overview

This is a navigation map of the current implementation, checked on 2026-09-20.
It describes existing boundaries, not completion of the modernization backlog.
[AGENTS.md](../AGENTS.md) defines the architecture charter and coding rules;
local comments and tests define detailed behavior. Update this map when a
component boundary changes; keep workaround histories beside their implementation.

## Components and entry points

| Area | Responsibility | Start here |
| --- | --- | --- |
| Application composition | Startup, Plasma host, application services and shutdown coordination | [main.cpp](../app/main.cpp), [lattecorona.cpp](../app/lattecorona.cpp), [session/](../app/session) |
| Dock views | Per-view state, positioning, visibility, effects and containment bridge | [view.h](../app/view/view.h), [containmentinterface.cpp](../app/view/containmentinterface.cpp) |
| Layout instances | Individual layout objects and their views | [genericlayout.cpp](../app/layout/genericlayout.cpp), [centrallayout.cpp](../app/layout/centrallayout.cpp) |
| Layout coordination | Active layouts, synchronization, storage, import/export and shared launchers | [manager.cpp](../app/layouts/manager.cpp), [synchronizer.cpp](../app/layouts/synchronizer.cpp), [storage.cpp](../app/layouts/storage.cpp) |
| Containment | Dock applet presentation and arrangement inside a containment | [CMakeLists.txt](../containment/CMakeLists.txt), [layoutmanager.cpp](../containment/plugin/layoutmanager.cpp) |
| Tasks | Task presentation, actions, KWin integration and isolated preview control | [CMakeLists.txt](../plasmoid/CMakeLists.txt), [previewprocess.cpp](../plasmoid/plugin/previewprocess.cpp) |
| QML integration | Core reusable types, pure-QML components and application-facing adapters | [core/CMakeLists.txt](../declarativeimports/core/CMakeLists.txt), [app plugin configuration](../app/declarativeimports/CMakeLists.txt) |
| Platform integration | Window system, compositor protocols and platform-specific behavior | [app/wm](../app/wm), [compat](../compat), [cmake](../cmake) |
| Settings and verification | Settings models/views, production tests and runtime retest procedures | [app/settings](../app/settings), [autotests](../autotests), [testing guide](development-testing-guide.md) |

`Layouts::Manager`/`Synchronizer` coordinate layouts; the containment
`LayoutManager` arranges items within a dock. Their similar names do not imply
interchangeable state or ownership. `Layouts::Storage` handles persistence;
persisted settings and transient view state must not be treated as the same data.

## Build and runtime boundaries

Most application sources are collected into `latte-dock-ng` by
[app/CMakeLists.txt](../app/CMakeLists.txt). Functional directories are not all
separate libraries. The application uses both Qt Quick and Qt Widgets.

QML presentation consumes plugins and application objects through properties,
signals and adapters. Plugins also link directly to Qt/KDE/Plasma libraries;
they do not all sit above an independent application-runtime library. In
particular, `org.kde.latte.private.app` deliberately resolves application-facing
symbols from the exporting `latte-dock-ng` host. Preserve host exports and
plugin visibility/linker requirements when reorganizing targets. An import in
a standalone QML runner is not an equivalent test of that host-dependent module.

The separate `latte-dock-ng-preview` process contains window preview rendering.
`PreviewProcess` mediates its protocol and failure fallback; preserve serial
validation, bounded traffic and watchdog behavior described in its source.
The saved task hover action remains the feature authority, while helper failure
state may temporarily prevent rendering. `latte-dock-ng-add-launcher` is a
separate D-Bus client helper rather than another dock runtime.

## QML module map

| Import URI | Build/registration source | Important distinction |
| --- | --- | --- |
| `org.kde.latte.core` | `declarativeimports/core/CMakeLists.txt` | Generated plugin and type metadata |
| `org.kde.latte.private.containment` | `containment/CMakeLists.txt` | Generated containment helper plugin |
| `org.kde.latte.private.tasks` | `plasmoid/CMakeLists.txt` | Generated task backend plugin |
| `org.kde.latte.private.app` | `app/declarativeimports/CMakeLists.txt` | Generated plugin with host symbol dependencies; application types also have runtime registration paths |
| `org.kde.latte.abilities`, `org.kde.latte.components` | `declarativeimports/CMakeLists.txt` | Installed loose-file QML modules |

Registration also occurs in application code; inspect `Corona::qmlRegisterTypes`
and the relevant component's registration before moving a type. Generated plugin
metadata and runtime registration can coexist. The generated plugins use
target-specific `.qmltypes` filenames. Preserve both configured QML installation
roots when `LATTE_QT6_COMPAT_QMLDIR` is enabled, and check that a system/user
installation is not masking a missing build artifact during validation.

## Lifecycle and authoritative state

`Corona` constructs application services including layouts, screens, settings,
shortcuts and the window-system adapter. It creates the Wayland implementation
and coordinates startup with Activities and package loading. `View` exposes
per-dock state and composes focused helpers; `ContainmentInterface` bridges it
to Plasma applets. QObject parents describe ownership, but signal delivery,
QML references and explicit teardown paths also determine safe lifetimes.

Before changing a lifecycle boundary, identify the creator, owner, observers,
thread, teardown entry point and callbacks that can still arrive. Read the
actual shutdown implementation and nearby rationale before choosing an order;
this overview is not a universal destruction sequence. Follow the clean-quit
and coredump A/B procedure in the testing guide for crash fixes.

Session logout handling lives in `Session::ShutdownMonitor`, created in
`main()` after the `Corona` so that it is destroyed first. It samples ksmserver
only while plasma-shutdown owns `org.kde.Shutdown`. `~Corona()` deletes its
services in dependency order through `Session::deleteNow()`, which tolerates a
service already freed by `QCoreApplication::exec()` during a normal quit.

For any new shared state, document who changes it and how others observe it.
Derived geometry, hover state and cached window metadata need explicit
invalidation. Inspect the relevant TaskItem/AppletItem comments and tests before
changing tooltip, drag, popup or animation interactions; do not create a second
authority in C++ or QML to work around an unexplained synchronization failure.

## Compatibility and evolution

The runtime is Wayland-only. Supported dependency floors live in the root
`CMakeLists.txt`; use its feature probes for API differences and runtime checks
for compositor services. A compiled API does not establish compositor support.
Behavioral regressions may still need narrowly justified version checks.

Keep existing host/plugin boundaries, persistence formats, D-Bus interfaces and
QML names stable while extracting policy. Treat Qt/Plasma private integration as
a maintenance constraint with focused tests, not a general-purpose public API.
For popup geometry, follow the hard rule in AGENTS.md and the implementation in
[dialog.cpp](../declarativeimports/core/dialog.cpp).

The [modernization plan](architecture-modernization-plan.md) and
[implementation guide](architecture-modernization-implementation.md) describe
future improvements. Revalidate their dated baseline against this map and the
current source before implementing a package; neither plans nor this overview
override concrete ownership and compatibility comments beside the code.

## Qt reference semantics

- [QPointer](https://doc.qt.io/qt-6/qpointer.html): a guarded, non-owning QObject
  reference; use it when a retained object can disappear independently.
- [QObject](https://doc.qt.io/qt-6/qobject.html): parent ownership, thread affinity
  and context-bound connection behavior.
- [Exposing C++ attributes to QML](https://doc.qt.io/qt-6/qtqml-cppintegration-exposecppattributes.html):
  property notification and constant properties. Check API availability against
  the project's supported Qt floor when consulting newer documentation.
