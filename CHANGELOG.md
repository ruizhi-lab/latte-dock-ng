# Changelog

All notable changes to Latte Dock NG are documented in this file.

## [v1.2.11] - 2026-06-23

### Fixed
- Fixed all-screens dock synchronization for widget removal, widget add, drag-and-drop widget placement, applet ordering, and launcher/menu-backed applets.
- Fixed Wayland always-visible dock strut reservations so cloned docks reserve space on their own screen instead of affecting the primary screen.
- Refined session shutdown handling so Latte quits after committed shutdown blockers close while still surviving cancelled logout attempts.
- Fixed duplicate instance handling to exit cleanly with return 0 instead of calling qGuiApp->exit(), and moved SharedQmlEngine creation after the single-instance guard to avoid unnecessary teardown.

## [v1.2.10] - 2026-06-21

### Fixed
- Session shutdown handling now stays alive when logout is cancelled while still quitting cleanly after blocking windows close during committed shutdown.
- Modern dock background shadows now default to the same compact 6px effect as explicitly setting Appearance > Background > Shadow to 6px.

## [v1.2.9] - 2026-06-19

### Fixed
- Task icons now refresh immediately when the system icon theme changes, including switching back to the default Breeze icon theme.
- Audio stream badges now scale with task icon zoom while preserving their relative position.

## [v1.1.26] - 2026-06-14

### Fixed
- Analog clock widget no longer produces extra empty space on both sides when added to the dock. The clock was incorrectly classified as a text-heavy applet alongside the digital clock, causing an oversized slot allocation.

### Changed
- Wrap global-scope classes in namespace Latte to prevent symbol collisions
- Replace string-based SIGNAL()/SLOT() macros with type-safe &Class::method syntax
- Add override keyword to 46 virtual destructors for compiler-enforced signature checking
- Replace [&] lambda captures with [this] in connect callbacks to prevent dangling references
- Replace C-style casts with static_cast<> for type safety
- Centralize scattered plugin name strings into shared app/pluginids.h header
- Add required keyword to critical QML properties for clear runtime errors
- Create Constants.qml documenting shared visual-proportion values
- Replace const T return-by-value with T to enable move semantics in GenericTable
- Use concrete QML types (point, Instantiator) instead of var where applicable


## [v1.1.23] - 2026-06-13

### Fixed
- Volume widget and systray volume icon no longer show incorrect muted state when first added to a dock. PulseAudio output device subscription is primed at startup and a repeating safety timer forces plasma-pa's PreferredDevice to read the initial default sink state.
- Updating/reinstalling no longer silently deletes user custom dock configurations. The pre-clean step now preserves `~/.config/latte/` and saved layouts unless `--purge-user-data` is explicitly passed.

## [v1.1.22] - 2026-06-13

### Fixed
- Volume widget and systray volume icon no longer show incorrect muted state when first added after a cold system boot. PulseAudio output device (SinkModel) subscription is now primed at startup alongside the existing stream subscription.

## [v1.1.22] - 2026-06-13

### Fixed
- Updating/reinstalling no longer silently deletes user custom dock configurations. The pre-clean step now preserves `~/.config/latte/` and saved layouts unless `--purge-user-data` is explicitly passed.

## [v1.1.21] - 2026-06-13

### Added
- Automatic QML disk cache clearing on version change, preventing stale compiled QML from masking fixes after upgrades.

### Fixed
- Default background thickness in new docks now correctly defaults to 6% (was 10% due to stale template values).

### Changed
- Project license upgraded from GPL-2.0-or-later to GPL-3.0-or-later.

## [v1.1.20] - 2026-06-13

### Fixed
- Eliminated binding loop on `inNormalState` property in visibility controller.
- Prevented false muted icon when no audio stream exists.

## [v1.1.19] - 2026-06-13

### Changed
- Moved taskmanager fallback QML module from `org.kde.plasma.private.taskmanager` to `org.kde.latte.compat.taskmanager`, so latte no longer installs or removes files in Plasma's namespace.
- Removed dead `TaskManagerApplet` import from `TaskItem.qml`.

### Fixed
- Wheel events now pass through to all external applets, not just systray.
- Wayland no longer destroys applet popups on open.
- Widget explorer now uses single-click to add widgets instead of double-click.
- External C++ plasmoids that request `fillWidth` now render correctly.
- Widget explorer places new applets before systray/tasks, not at dock end.
- Systray drag-and-drop reorder works without breaking layout.
- Suppressed benign KDE framework property override warnings and KIO teardown errors.

## [v1.0.14] - 2026-05-15

### Added
- Added the modern dock style screenshot to README.
- Added modern/classic dock style switching support with preserved parabolic animation behavior.

### Fixed
- Fixed Justify alignment for both modern and classic dock styles by removing the legacy splitter-based layout path from dock-style views.
- Fixed widget/task spacing, separator placement, widget drag ordering, and style-specific indicator behavior across classic and modern dock styles.
- Cleaned temporary debug logs after validation so startup logs stay focused on actionable warnings/errors.

## [v1.0.8] - 2026-05-04

### Fixed
- Refined task icon highlight behavior to avoid stale clicked-highlight regression while preserving hover feedback behavior.
- Improved task-state indicator contrast logic against panel light/dark themes, while keeping audio mute/unmute corner badge color independent.
- Fixed task drag sorting policy:
  - dragging a non-pinned running app into pinned area now auto-pins it and reorders into the target position;
  - dragging a pinned launcher into non-pinned area remains blocked.

## [v1.0.6] - 2026-05-03

### Fixed
- Fixed mixed install/runtime import-path regression: system installs no longer force-load `~/.local` Qt6 QML paths by default.
- Prevented stale user-local QML trees from overriding packaged system modules when launching `/usr/bin/latte-dock-ng`.
- Added explicit env toggles for diagnostics:
  - `LATTE_FORCE_USER_LOCAL_QML_IMPORTS=1`
  - `LATTE_DISABLE_USER_LOCAL_QML_IMPORTS=1`

## [v1.0.5] - 2026-05-03

### Fixed
- Fixed logout/session shutdown blocking by adding a reliable Wayland session-end path:
  - detect KDE session shutdown via `org.kde.ksmserver.isShuttingDown()`
  - mark fast teardown state consistently
  - quit Latte promptly when shutdown is detected.
- Fixed indicator-record removal crash path during teardown (`removeAt(-1)` guard in `Indicator::Factory::removeIndicatorRecords`).

## [v1.0.4] - 2026-05-03

### Fixed
- Fixed context-menu callback lifecycle for "More Places" to avoid stale-object warnings in `ContextMenu.qml`.
- Fixed audio badge interaction so clicking the mute indicator no longer leaves a stuck selected/highlight state.

### Changed
- Aligned the audio badge input model with Plasma 6 task-manager behavior (`HoverHandler`/`TapHandler` for click/hover state, wheel handling isolated).

## [v1.0.3] - 2026-05-03

### Added
- Added fallback app-name hover tooltip for dock task items when thin-tooltip is unavailable.
- Replaced README screenshot with the latest Latte Dock NG screenshot asset.

### Changed
- Bumped runtime/application version to `1.0.3` to keep About dialog aligned with release tag.

## [v1.0.2] - 2026-05-03

### Notes
- Baseline public release tag.
