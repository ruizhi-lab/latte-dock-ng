## [Unreleased]

## [v1.2.46] - 2026-09-03

### Fixed
- Fix a dock crash (SIGABRT) when quitting during logout, shutdown or restart:
  the cascading-menu pointer tracker connected each tracked window's
  destroyed() signal to `View::onPointerWindowDestroyed` on Enter but never
  disconnected it, so a hovered-and-left window (including the dock's own
  window) kept a live connection that fired from `~QObject`'s destroyed()
  emission against the half-destroyed View. Connections are now dropped when
  the pointer leaves a window and for every still-tracked window in the View
  destructor.
- Preserve the user's XDG autostart entry across upgrades: pre-install
  cleanup passes the new `--preserve-autostart` flag, so reinstalling over an
  existing installation no longer removes the user's autostart preference.

### Tests
- Source-contract tests lock the pointer-window connection lifecycle on
  pointer Leave and in the View destructor.
- GCC and Clang autotest suites pass all 40 registered tests.

## [v1.2.45] - 2026-08-30

### Fixed
- Use the versioned `latte-dock-ng` icon name consistently in the application
  desktop entry, kicker action, and application window so updated shortcuts
  load the current artwork instead of a cached legacy icon.
- Fix the SVG icon wrappers to reference the PNG companion installed beside
  them, allowing KDE icon themes to render the new artwork correctly.

### Tests
- CMake build and the full registered autotest suite pass.

## [v1.2.44] - 2026-08-30

### Added
- Add opt-in per-window dots for grouped task indicators, showing up to four
  individual windows and an additional dot for remaining windows.
- Add the updated Latte application icon.

### Fixed
- Keep classic and modern indicator styles consistent when grouped-window
  options are enabled, including line width and active-window behavior.
- Restore the dock editor header settings context and keep indicator settings
  controls aligned with the desktop theme.
- Remove duplicate grouped-indicator properties from task items.

### Tests
- GCC and Clang autotest suites pass all 40 registered tests.
- QML lint passes without errors.

## [v1.2.43] - 2026-08-29

### Fixed
- Fix Qt 6 QML plugin loading on Unix installations by using the
  conventional `lib` prefix expected by the QML plugin loader for the core,
  containment, and tasks modules.
- Register Latte QML value types with valid lowercase names, preventing
  `Types` registration errors that could leave the dock invisible after a
  distribution package installation.

### Tests
- GCC and Clang builds complete successfully.
- GCC and Clang autotest suites pass all 40 registered tests.
- Runtime verification passes using the installed user build without
  development QML path overrides.
- NixOS package and flake metadata are kept in sync with the release version
  and supported Plasma baseline.
- The release workflow validates the NixOS flake with `nix flake check` and
  `nix build` before publishing release artifacts.

## [v1.2.42] - 2026-08-29

### Fixed
- XDG autostart is now the single source of truth (PR #50): the internal
  ensureAutostart flag is removed, so a normal dock start never changes the
  autostart state. Enabling autostart creates the desktop entry or sets
  Hidden=false, disabling sets Hidden=true; the Preferences dialog, CLI and
  DBus all use the same XDG state. Previously disabling could leave the active
  desktop entry untouched, while enabling could overwrite Hidden=true set by
  KDE.
- The configuration controls restored after the QML review regressions: the
  leftover maxlength debug ruler (Ruler/RulerMouseArea) and the stale qmllint
  backlog plan are removed, and the canvas configuration controls
  (MarginsArea, SettingsOverlay, HeaderSettings, CustomIndicatorButton) work
  again.

### Build & Toolchain
- CMake baseline modernized to 3.20 / C++20: the deprecated implicit capture
  of `this` via `[=]` is removed and `add_compile_options()` replaces the
  global CMAKE_CXX_FLAGS append.
- All three C++ QML plugin modules (org.kde.latte.core,
  org.kde.latte.private.containment, org.kde.latte.private.tasks) migrate from
  hand-written QQmlExtensionPlugin + qmldir to `qt_add_qml_module`: the
  generated qmldir and plugin are installed into both QML install roots, and
  the Types Q_GADGET / QObject types register declaratively via
  QML_NAMED_ELEMENT.
- plugins.qmltypes is installed next to each generated qmldir so qmllint and
  Qt Creator can resolve the latte types; a new type-resolution deep lint
  (scripts/qmllint-deep.sh) runs in CI against the freshly built modules.
- New scripts/qmllint.sh syntax gate lints every git-tracked QML file and runs
  as a dedicated `qml-lint` CI job; astyle is replaced by a clang-format
  configuration.
- The plugin-registration smoke tests no longer compile the removed plugin
  classes: they stage the generated qmldir + plugin .so into a temporary
  import path and resolve the types through a real QML engine, fixing the
  false-positive local pass (the engine silently fell back to the system
  latte-dock module).
- default.nix version is kept in lockstep with CMakeLists (was stuck at
  1.2.21).

### QML modernization
- Unversioned Qt module imports (QtQuick, QtQuick.Controls, Layouts, ...)
  across all 229 tracked QML files - strict 1:1 rewrite, no behavior change.
- Unversioned org.kde.* imports across 182 QML files (505 lines), dropping the
  Qt5-style version mix inherited from upstream.

### i18n
- Shared LatteComponents QML strings (CheckBox, TextField, BadgeText) are now
  extracted into latte-dock.pot (they were previously in no catalog), and the
  separator plasmoid strings into plasma_applet_org.kde.latte.separator.pot.

### Tests
- New direct tests for the shared XDG autostart implementation
  (autostartlogictest).
- New source-contract tests cover the qt_add_qml_module registration,
  singleton construction through the engine, and the unversioned abilities
  import.
- GCC and Clang autotest suites pass all 40 registered tests.

## [v1.2.41] - 2026-08-23

### Performance
- Reduce idle task hover work when window previews and window highlighting
  are disabled, while preserving tooltip and task auto-scroll behavior.
- Reduce repeated task, PulseAudio, layout-import, and headless desktop
  integration work across the dock's active and startup paths.

### Tests
- Add a source-contract autotest covering the inactive preview hover guard.
- GCC and Clang autotest suites pass all 39 registered tests.

## [v1.2.40] - 2026-08-21

### Fixed
- The separator at the tasks/widgets boundary no longer jitters or
  oscillates when the pointer rests or moves across it (macOS-style fixed
  divider).  At the boundary the top-most neighbour (e.g. an external widget
  drawing above the tasks) can briefly own the junction, zoom the adjacent
  task and then be pushed away by the resulting layout growth; its onExited
  nullifies the current item immediately, so the parabolic restore timer now
  keeps a 150 ms grace period that lets the task now covering the cursor
  re-enter and cancel the zoom clear instead of producing a visible blip.
- The separator line renders tight at its boundary slot: the content
  container now uses the separator's preferred length (falling back to
  `separatorLength` when the applet has not resolved `Layout.preferredWidth`
  yet, which is always the case for freshly added separators) instead of
  overflowing a full icon-sized slot into the neighbouring applets.  This
  also fixes a freshly added separator being invisible.
- The Trash widget now keeps its original full-color icon by default in
  every dock configuration (issue #44): newly added trash applets are
  automatically added to `userBlocksColorizingApplets`, and older layouts
  get a one-time migration (`trashKeepOriginalColorsDefaulted`) that marks
  every present trash widget, after which explicit user toggles are never
  overwritten again.

### Tests
- New source-contract tests lock the separator content-container fallback
  (`separatorContentContainerStaysTightWithFallback`), the restore-timer
  grace period that prevents the boundary zoom blip
  (`restoreZoomTimerGracePeriodPreventsBoundaryBlip`), and the Trash
  keep-original-colors default + migration
  (`trashKeepOriginalColorsDefaultsToCheckedForAllConfigs`).

## [v1.2.39] - 2026-08-21

### Fixed
- The bounce-induced zoom freeze (issue #42) is fixed: parabolic scale
  broadcasts are now addressed through `effectiveIndex` (`index >= 0 ? index :
  taskItem.lastValidIndex`), so a delegate kept alive by `ListView.delayRemove`
  during a launcher->window conversion (index -1) keeps tracking the pointer
  instead of being frozen at its old zoom or pinned to zoom level 1 by the
  clear broadcast; outgoing broadcasts use the same effective index, removing
  out-of-bounds addresses like -1/-2.
- Icon scaling no longer behaves abnormally when the pointer moves into the
  dock and over other icons while the parabolic zoom animation is still
  running (issue #40): the zoom gate in `ParabolicEventsArea.onParabolicMove`
  was removed, so the parabolic effect keeps updating during the zoom-in
  animation and the `Behavior on zoom` in ParabolicItem animates smoothly
  toward the new target.
- Zoom restore is delegated to the `Behavior on zoom`: `slotClearZoom` only
  sets the target value (`parabolicItem.zoom = 1`) and the Behavior animates
  toward it, avoiding jarring jumps when prior animations have not finished.
- A window without launcher identity (an app that ships no .desktop file,
  e.g. a bare window with empty AppId) no longer renders as a phantom
  separator line at the right end of the dock: the separator placeholder
  is now applied only to launcher-type entries whose URL is still being
  resolved asynchronously, never to plain windows or startup items.
  Previously such a window stayed thin (separator-like) forever, and
  "Remove Right/Left Separator" could not get rid of it — the phantom had
  an empty launcher URL, so removing it through the launcher list was a
  silent no-op that still reported success.
- The launcher-local separator removal path now refuses entries with an
  empty launcher URL, so a task that merely looks like a separator can
  never be "removed" without actually changing the launcher list.
- The Trash widget now has a "Keep Original Icon Colors" option in its
  context menu (issue #44): when enabled, Latte strips the "-symbolic"
  suffix from the applet's icon so the full-color icon from the icon theme
  shows, while the empty/full trash icon keeps switching; disabling the
  toggle restores the original icon immediately. Scoped to the Trash widget
  because other applets (launchers) render monochrome through their own
  color masks that this cannot override.
- Translations are synced with the source after the Dock/Panel -> Dock
  migration: ~25 strings in latte-dock and the containment-actions context
  menu silently fell back to English in every language because the po files
  were never regenerated. Msgids are realigned and several user-facing
  strings wrongly marked `notr="true"` in the Qt Designer dialogs (Thickness
  margin influence, Export Template, New, Remove, Import..., Export...) are
  translatable again.

### Tests
- New source-contract tests lock the `effectiveIndex` fallback in
  `ParabolicEventsArea` and the Behavior-driven zoom restore path
  (`zoomRestoreDelegatedToBehaviorAnimation`).
- New source-contract tests lock the empty-URL guard in the separator
  removal path, the isSeparator window/startup exclusion, and the context
  menu routing through the guarded launcher-local removal.
- New source-contract tests lock the "Keep Original Icon Colors" menu wiring
  and the Trash-only icon override path.
- New source-contract tests lock the Qt Designer notr fixes, the absence of
  stale Dock/Panel msgids across all po domains, and that the translation
  Messages.sh scripts reference only existing directories.

## [v1.2.38] - 2026-08-10

### Fixed
- The Application Menu widget's menu bar (and its cascading submenus) no
  longer closes the moment the pointer reaches a submenu: the dock no longer
  reconfigures its window flags / plasma shell surface while the widget
  reports NeedsAttentionStatus (issue #36).  The first-level menu now stays
  open on pointer-leave — matching the official panel behaviour — and closes
  on click-outside.
- Cascading submenus (e.g. kicker) no longer stay stuck open after the
  pointer leaves them: the dock closes the submenu tree event-driven when
  the pointer leaves the menu windows (kicker's own close timer is broken by
  its submenu window stealing the keyboard focus, so the close is driven
  from the dock instead, using Enter/Leave events rather than the stale
  Wayland cursor position).
- A crash (segfault) while interacting with a cascading menu is fixed:
  destroyed popup/menu windows are removed from the pointer tracker so the
  deferred close check never dereferences a dangling pointer.

### Tests
- New source-contract tests lock the NeedsAttentionStatus window handling,
  the hideOnWindowDeactivate default, the event-driven submenu close, and
  the dangling-pointer guard.

## [v1.2.37] - 2026-08-05

### Fixed
- The animation speed setting (Effects > Animations, x1/x2/x3) is now
  actually perceptible: the containment maps x1/x2/x3 to 1x/2x/3x
  animation durations matching the plasmoid's standalone semantics, and
  the inverted EffectsConfig buttons were corrected (issue #39).
- Transparent icons (e.g. the Colloid theme) no longer show a ghost of
  the original-size icon while a widget zooms: the applet drop shadow
  now fades out smoothly during the zoom instead of rendering a
  duplicated copy behind the scaled applet (issue #38).
- Widgets whose zoom is disabled no longer participate in the parabolic
  wave, so the task icon at the widget boundary restores its size
  immediately when the mouse leaves.
- Release builds no longer emit an unused-variable warning in the debug
  message filter.

### Added
- New "Zoom widgets on hovering" option (Behavior > Items): when
  disabled, external widgets keep their original size on hover so their
  icons stay sharp, while task icons and launchers still zoom.
- The default layout templates no longer include the mail (Thunderbird)
  launcher.

### Changed
- The durationTime config default is now x1 (normal) instead of x2.

### Tests
- New source-contract tests cover the animation speed mapping, the
  EffectsConfig button values, the applet shadow fade, the widget-zoom
  option scope, and the default template launchers.

# Changelog

All notable changes to Latte Dock NG are documented in this file.

## [v1.2.36] - 2026-08-02

### Fixed
- Autostart no longer breaks silently when the XDG autostart desktop
  file disappears (e.g. removed by a package uninstall, depclean or a
  failed update): startup now restores it and logs a warning instead of
  leaving the dock unlaunched on every login.
- Autostart updates are staged: the replacement file is copied next to
  the working entry and swapped in only after a successful copy, so a
  failed copy can never delete the existing entry.
- Autostart failures (missing system desktop file, failed copy) now log
  warnings instead of failing silently.

### Tests
- New importer-logic and source-contract tests cover the autostart
  self-heal and staged-update behavior.

## [v1.2.35] - 2026-08-02

### Fixed
- Fixed logout, reboot and shutdown hanging on Plasma 6.7: KWin's
  session shutdown waits for every xdg_toplevel window to close, and the
  dock views ignored the compositor close request until the
  session-ending flag was set. The views are now unmapped as soon as the
  logout is announced and restored if the user cancels.
- Filtered cosmetic Plasma theme SVG warnings from the debug log.
- Mirrored system Kirigami template qmldir files in the KNS compat
  overrides.

### Build
- CI now builds and runs the full autotest suite (ctest) on every push,
  pull request and release tag.
- The Debian 13 (trixie) deb now carries the `+deb13u1` revision marker,
  per the Debian convention for stable-specific builds; the testing/sid
  build keeps the plain `-1` name.
- Documented the two deb variants in the README and installation docs.

## [v1.2.34] - 2026-08-01

### Fixed
- Lowered the minimum Plasma version from 6.5.0 to 6.3.0; Plasma 6.3
  (Debian 13 trixie) is now verified as the minimum supported version,
  development happens on Plasma 6.5+ / Qt 6.11.
- Fixed dock indicators disappearing on Plasma 6.3: the
  org.kde.plasma.plasmoid QML module is only registered lazily by
  PlasmaQuick on that version, so indicator components are now created on
  first access instead of at View construction.
- Fixed the latte_indicator package structure plugin shipping without
  embedded metadata on Qt 6.8 (moc macro expansion through the compat
  forwarding header), which made KPackage unable to resolve the
  "Latte/Indicator" structure.
- Fixed KNS dialog compatibility overrides never being created on
  Kirigami 6.12+ where org.kde.kirigami.controls was merged into the
  main module.
- Added infinite-loop and recursion guards across the codebase; made the
  guards regression-safe and repaired distro install contracts.
- Restored synchronous deletion in layout unload paths; added contracts
  keeping the behavior stable.
- Restored KDE compiler enforcement (-Wall, QT_NO_CAST_*, QT_NO_KEYWORDS)
  and fixed all norm violations; both GCC and Clang build warning-free.
- Fixed the latte-dock icon reference to the name installed by ECM.
- Removed a set-but-unused variable that triggered -Wunused-but-set-variable.

### Changed
- Concurrent parabolic zoom and launcher bounce animation.
- Bounce animation replaced with a parabolic bounce.

### Build
- The release .deb is now built on Debian 13 (trixie) with
  dpkg-shlibdeps versioned dependencies, so the package installs on
  Debian 13 stable, testing, sid and Ubuntu 26.04+.
- Linked Qt6::Quick in toolsunittest so tests build on distros with
  arch-triplet Qt header layouts.

### Test
- Added contract tests for the infinite-loop guard follow-ups and the
  layout unload deletion behavior; registered the plasmoid module stub
  for the smoke tests on Plasma 6.3; forced KConfigGui linkage in
  schemecolorsunittest for --as-needed linkers. Full ctest suite passes
  on Plasma 6.3 (Debian 13) and Plasma 6.5+ with both GCC and Clang.

## [v1.2.17] - 2026-06-28

### Fixed
- Fixed third-party clocks (e.g. Colorful Digital Clock) still overflowing
  after the v1.2.16 cap increase.  Clock detection now matches any plugin
  name containing "clock" (excluding "analogclock") instead of only
  "digitalclock".  The natural-width cap is further increased from 5× to
  8× maxIconSize for the widest clock representations.
- Added signal-driven slot width updates via onImplicitWidthChanged and
  onChildrenRectChanged so the slot resizes immediately when clock text
  content changes (no more 2 s polling delay).
- Added a height cap (3× maxIconSize) to guard against runaway compact
  representation heights (e.g. h=1352 from Colorful Digital Clock).
- Fixed "Unable to assign [undefined] to int" startup warning from
  MyView.qml:37 with a safeInt() helper.

### Test
- Added 15 boundary regression tests covering previously untested
  special-cased logic: indicator factory builtin exclusion, wayland
  window whitelist, context menu wiring, layout manager cleanup,
  separator plugin constants, export model applet list, latte package
  branching, indicator type remapping, message suppression, internal
  view splitter guards, fallback tracked windows, constraint hints,
  launcher/drag-drop detection, plasmoid wheel bypass, and compact
  applet fallback sizing.  Test suite: 61 → 76.

## [v1.2.16] - 2026-06-28

### Fixed
- Fixed digital clock widget overflowing past the dock edge and overlapping
  neighboring icons when using long date formats.  The natural-width cap was
  increased from 3× to 5× maxIconSize to accommodate formats like "Saturday,
  June 27, 2026 10:30 AM".
- Fixed "Unable to assign [undefined] to int" startup warning from
  MyView.qml:37.  Added a safeInt() helper that validates bridge-host
  property values before assignment, preventing undefined from reaching
  int-typed properties during initialization and bridge transitions.
- Guarded LayerShellQt::Window::setScreen with CMake feature detection to
  prevent build failures when the LayerShellQt version lacks the method.

### Test
- Added 60+ source contract regression tests covering widget-specific
  special handling: digital clock sizing, middle-click close active window,
  auto-pin on drag, scroll minimize/unmaximize, system tray guards, volume
  and application menu popup sizing, clipboard/digital-clock error
  suppression, applet insertion boundaries, separator/spacer detection,
  and MyView int property guard.

## [v1.2.15] - 2026-06-27

### Fixed
- Fixed systemsettings and other KDE applications crashing on startup due to KNS compat QML import paths leaking into child processes via environment variables. All QML and plugin import paths are now engine-scoped using `addImportPath()` and `addLibraryPath()` instead of `qputenv()`.
- Fixed `uninstall.sh` to clean up KNS compat QML overrides from both old (`~/.local/lib*/qt6/qml/`) and new private paths during uninstall.

## [v1.2.14] - 2026-06-26

### Fixed
- Fixed middle-click close active window not working on empty dock areas.
- Fixed scroll-down minimize not working for ScrollToggleMinimized action.
- Fixed auto-pin when dragging non-pinned tasks into launcher area.
- Fixed drag-and-drop icon reordering stability and visual feedback.

## [v1.2.13] - 2026-06-26

### Fixed
- Fixed KNS dialog compatibility QML overrides being written to Qt's global user QML path (`~/.local/lib*/qt6/qml/`), which could crash incompatible KDE applications like systemsettings on startup.
- Fixed `uninstall.sh` to clean up KNS compat overrides from both old (global QML) and new (private) paths during uninstall.

## [v1.2.12] - 2026-06-25

### Fixed
- Fixed widget hide/show synchronization across all screens during removal and undo.
- Fixed Plasma panel overlap for vertical docks on multi-screen Wayland setups.

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
