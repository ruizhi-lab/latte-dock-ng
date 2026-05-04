# Changelog

All notable changes to Latte Dock NG are documented in this file.

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
