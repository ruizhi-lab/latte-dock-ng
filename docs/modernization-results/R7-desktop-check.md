# R7a–R7f: desktop validation (2026-11-10 and 2026-11-11)

Status: Complete. The checks that the R7a–R7f records left to the user ran on the user's
desktop. R7a, R7b, R7d, R7e and R7f pass within the limits listed below. The user decided
R7c: keep the current-activity marking. Its menu data was checked, but the submenu was not
seen on screen.

## Environment

- **Desktop:**
  - Gentoo, Plasma 6 on Wayland, zh_CN.
  - Two outputs: DP-2 (primary, 1920×1200) and HDMI-A-1 (1920×1080 at 1920,120).
  - Two running activities: 默认 (current) and Work.
  - Layout 我的布局 in single-layout mode: dock 1 on the bottom edge of DP-2 and dock 13 on the bottom edge of HDMI-A-1. Each has the Latte task manager, the application dashboard (kickerdash) and the trash.
- **Builds:**
  - Until 21:01 on 2026-11-10: the user-mode Debug install of `ed5d0b3c3`, build ID `36e61da3…`. Its code is the same as `main` from `04589b1e9` to `311c93ff5`.
  - From 21:01: the user-mode Debug install of `d2bc32e71`, build ID `e13072c7…`. Its code is that of `main` `d7fb24269` and `351761a4b`, which add R7h.
- **Launch:**
  - From 19:39 on 2026-11-10 every dock ran in its own transient user unit.
  - From 20:04 the units also had `PartOf=graphical-session.target`, `After=graphical-session.target` and `TimeoutStopSec=5s`, as AGENTS.md step 5 now prescribes.
  - Earlier docks were started with `setsid` from the agent shell; see Findings.
- **Evidence:**
  - The dock's `--debug` log of each run, the journal and D-Bus queries.
  - Configuration snapshots under `/data/tmp`, taken before every step that changed the configuration: 19:54, 21:28 and 22:08. These files are local and not committed.

## Results

| Slice | Check | Result |
|---|---|---|
| R7a | `dev-env.sh` with `XDG_CONFIG_DIRS` unset | In a subshell with the variable unset, sourcing it gave exactly the user manager's value, `/home/ruizhi/.config/kdedefaults:/etc/xdg`. In the real session, a dock started at 20:04 from a unit with `UnsetEnvironment=XDG_CONFIG_DIRS` got that value once, not duplicated. It used 0 CPU ticks in 10 s, and the journal had no KSycoca activity. |
| R7b | Hot-plug with a clone, two-output variant | A test dock (containment 12, added with `--add-dock`) was set to all screens, which put its clone on HDMI-A-1. HDMI-A-1 was switched off and on with `kscreen-doctor` in two cycles: off at 20:20:02, on at 20:21:25, off at 20:21:56, on at 20:22:15. All seven syncs logged since start-up had a START with a matching end. "view must consider its screen" named only originals: 1 and 13 at start-up, then 12. The clone was removed when its output went away and created again when it came back (views 15 and 16, `IsClonedFrom: 12`). No fatal marker, no coredump. |
| R7c | Move Dock To Layout marking | Multiple Layouts mode with Empty assigned to Work was set directly in the configuration (`memoryUsage=1`, `activities=` of Empty), because the editor's 活动 menu would not keep the tick (R7j). The D-Bus call `contextMenuData(12)` returned mode 1, loaded layouts `我的布局;;Empty`, current layouts `我的布局`, and the entries Empty and 我的布局. `LayoutMenuItemWidget` therefore draws 我的布局 bold and Empty plain. A screenshot of the Layouts Editor showed 我的布局 and Empty in bold and Default plain. The user's decision: keep the current-activity rule. The configuration was restored from the 22:08 snapshot on 2026-11-11 at 08:19. |
| R7d | Screen list after hot-plug | The HDMI-A-1 cycles above ran without a new warning or a coredump. The user did not report the optional check of whether the 屏幕 list drops the output and gets it back. |
| R7e | Start-up with several applets; task menu | After the restart at 20:04, the user saw every widget on both docks. The log has no warning from `createAppletItem` or `appletRetryTimerComponent`. The user also confirmed that the task context menu shows its items, places and separators as before, and that long entries are elided. |
| R7f | `--replace` | At 20:04:32 the new instance's SIGTERM ended the old dock. Its teardown finished in 125 ms, and the new dock had both views up within 2 s. |
| R7f | `--add-dock` | The forwarded `addView` reached the running dock. "Default Dock", the name that `--available-dock-templates` prints, added nothing in the zh_CN session (fixed by R7h). 默认停靠栏 added the test dock. |
| R7f | `--import-layout` | At 20:26:51 the dock imported a copy of 我的布局 as "R7f test" and switched to it. At 21:23 the user switched back and removed the copy. |
| R7f | `--import-full` | A full export taken at 21:28 was imported at 21:29:05. The old dock ended in 1.0 s, and the new dock had both views up in 1.3 s. The configuration afterwards was byte-identical to the snapshot taken just before the import, including the hidden multiple-layouts file, which the export does not contain. |
| R7f | Autostart toggles | Clearing 系统启动时自动启动 (21:35) added only `Hidden=true`, and ticking it again (21:37) changed it to `Hidden=false`. `Exec=/usr/bin/latte-dock-ng` was kept. KConfig also trimmed a trailing space in `GenericName[he]` when it rewrote the file. |
| R7f | Logout cancelled (L1) | On 2026-11-11 at 19:41:28 the logout prompt opened and was cancelled. No shutdown sequence started, and the dock kept running. |
| R7f | Real logout (L2) | The logout was confirmed at 19:41:43, and `org.kde.Shutdown` started at 19:41:45. The dock's shutdown monitor set the session-ending flag at 19:41:46. At 19:41:49.794 it logged "logout completed; quitting", and teardown was complete at .902. systemd stopped the session-bound unit at 19:41:49–50, after 11 h 22 min with a 190.1 MB memory peak. No coredump. After login, autostart started the packaged dock. |

## Findings during the checks

- **Agent-shell environment.** The Debug docks started with `setsid` from the agent shell carried 70 variables more than plasmashell, among them session tokens and proxy settings. They also ran inside the agent application's cgroup, and Thunderbird, launched from such a dock, inherited the variables. AGENTS.md step 5 now starts agent-launched docks through a session-bound user unit (`b8e5a27db`, `311c93ff5`).
- **R7h** (fixed, `d7fb24269`): `--add-dock` and `addView()` did not accept the untranslated template name in a translated session.
- **Docks dialog abort** (Debug builds only). Removing the last docks in the Layouts Editor's docks dialog asserts in `qBound()` (`viewscontroller.cpp:371`). The dock aborted at 21:07:39, and nothing was lost. A fix is proposed as R7i.
- **R7j:** the Layouts Editor's 活动 menu did not keep a tick on this Wayland session. A fix is on branch `claude/sleepy-hugle-be343e` and waits for its desktop check. Ticks are by design discarded unless the menu's own 确定 is pressed.
- **Removing a dock with its settings window open.** At 20:19:04, removing docks in the Layouts Editor while one of them had its settings window open logged 41 warnings. They were TypeErrors from `TasksConfig.qml` and `AppearanceConfig.qml` reading a null object, and two `QQmlExpression` warnings for `VisibilityManager.qml:519` depending on a non-bindable property. Observed, not filed.
- **Removed docks restored.** During the R7b set-up, docks 1 and 13 were removed through the Layouts Editor (20:19). They were restored from the 19:54 backup at 20:25.
- **Benign warnings:**
  - The full-configuration export dialog logged 51 KDirModel "No node found for item that was just removed" warnings.
  - The volume widget's PulseAudio library logged a webcam-microphone warning.

## Not verified

- R7a: a VM session; the Mageia retry and dnf's reuse of downloaded packages, which only CI can show.
- R7b: the specified three-output scenario (this desktop has two outputs) and scenario B, where the original's output goes away.
- R7c: the submenu on screen. Only its data and the Layouts Editor were seen.
- R7d: the 屏幕 list observation.
- R7f: the Debug build starting at login. The session's autostart starts the packaged dock.
- Debian 13 verification: deferred by the user until the refactor is complete.
