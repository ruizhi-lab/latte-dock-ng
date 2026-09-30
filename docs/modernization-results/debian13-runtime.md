# Debian 13.7 runtime staging

Environment: Debian 13.7, KDE Plasma 6.3.6, Qt 6.8.2, CMake 3.31.6,
GCC 14.2, Wayland, one `Virtual-1` output at 1920x1080.

## Build and startup

- Updated the VM through its configured USTC Debian mirror. The initial
  user-mode Debug install identified missing `qt6-declarative-private-dev`;
  installed that package and reran `bash install.sh --user Debug`
  successfully.
- Started `/home/debian/.local/bin/latte-dock-ng --replace --debug
  --log-file /tmp/latte-ng.log` with the live Plasma Wayland session variables
  and `~/.config/latte-dock-ng/dev-env.sh` sourced.
- `/home/debian/.local/bin/latte-dock-ng-preview` is installed and executable;
  `ldd` reports no missing shared libraries.
- KWin exposes both `org.kde.KWin.HighlightWindow.highlightWindows(as)` and
  `org.kde.KWin.Effect.WindowView1.activate(as)` on the session bus.
- The final startup log scan found no Warning, Error, Fatal, ASSERT or missing
  icon entries.

## Compatibility fix found during staging

Debian's Plasma Trash applet returned a bundled icon path in `:/icons/...`
form. Passing that string directly to a QML `Image` made QML resolve it as a
relative file URL and log `Cannot open` warnings. `LayoutManager::appletIconPath`
now converts only Qt resource paths to the equivalent `qrc:/...` URL; ordinary
icon paths retain their prior value. The source-contract test protects this
production conversion.

Validation performed:

- GCC Debug and Release and Clang Debug and Release builds of
  `lattecontainmentplugin` and `sourcecontracttest` passed with zero compiler
  warnings.
- `sourcecontracttest` passed in all four configurations.
- Debian `bash install.sh --user Debug` succeeded after the compatibility fix.
- Restarted the installed Debug process and confirmed the resource-path
  warnings disappeared from `/tmp/latte-ng.log`.

## Hover interaction status

The active `我的布局` saves `hoverAction=PreviewWindows`. The user manually
confirmed that a task hover displays the window preview after the final Debug
restart. This validates the preview path only. `HighlightWindows` and
`PreviewAndHighlightWindows` still require separate manual checks, including
confirming that each mode leaves the other effect off as documented in the
[manual retest checklist](manual-retest-checklist.md).
