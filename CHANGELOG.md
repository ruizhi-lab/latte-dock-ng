# Changelog

All notable changes to Latte Dock NG are documented in this file.

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
