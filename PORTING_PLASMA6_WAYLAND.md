# Plasma 6.6+ Wayland Port Status

This branch is a **Wayland-only** fork of [KDE Latte Dock](https://github.com/KDE/latte-dock) targeting **KDE Plasma 6.6+**.

## Completed

- CMake migration to Qt6/KF6 targets:
  - `Qt5::` → `Qt6::`
  - `KF5::` → `KF6::`
  - `qt5_add_dbus_adaptor` → `qt_add_dbus_adaptor`
- Removed all build-time X11/XCB dependency chain from top-level CMake.
- Switched runtime window-system backend to Wayland-only:
  - `XWindowInterface` source files removed from the tree.
  - `Corona` always instantiates `WaylandInterface`.
- Enforced Wayland at process startup (`QT_QPA_PLATFORM=wayland`, hard exit when not running on Wayland).
- Removed all `isPlatformX11()` runtime branches from active app code (window tracking, masks, frame extents, config views, positioner, visibility manager).
- Removed all `#include <NETWM>` and `QtX11Extras` includes from Wayland-only code paths.
- Renamed `byPassX11WM` parameter to `byPassWM` across `View`, `OriginalView`, `ClonedView`.
- Added explanatory comments to `VisibilityManager::hide()` / `show()` no-op stubs.
- Updated package install path to `KDE_INSTALL_KSERVICETYPESDIR`.
- Updated README/INSTALLATION baseline requirements to Qt6/KF6 and Wayland-only.
- Removed `QuickWindowSystem::isPlatformX11` QML API surface.
- Converted legacy KWin reconfigure helpers (`kwin_forwardMetaToLatte`, `kwin_setDisabledMaximizedBorders`, synchronizer border updates) to explicit Wayland no-op paths.
- Disabled unsupported KWin-dependent preferences in Settings UI for Wayland sessions.
- QML controls migration:
  - Removed all `QtQuick.Controls 1.x` and `QtQuick.Controls.Styles*` imports.
  - Removed `QtQuick.Dialogs 1.x` usage.
  - Removed `QtGraphicalEffects` dependencies from active code paths.
  - Dropped legacy `ExclusiveGroup`-based button grouping.
  - Ported shared controls (`CheckBox`, `Switch`, `SpinBox`) to `QtQuick.Controls 2`.

## Remaining work

- Visual parity audit: controls migrated from QQC1 to QQC2 defaults may need styling adjustments to match prior Plasma 5 appearance.
- Runtime QA on real Plasma 6.6+ session: window previews, context menu interactions, indicator behaviors, and settings pages.
- Build verification on a live Plasma 6.6+ environment with full KF6/Qt6 dev packages.

## Note on X11

This fork can no longer target X11. All X11 implementation code has been removed. If you need X11 support, use the upstream KDE repository at https://invent.kde.org/plasma/latte-dock.
