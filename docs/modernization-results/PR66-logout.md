# PR #66: Event-Driven Logout End and the Dock's Helper Windows

Status: Merged with follow-up fixes; the desktop logout check by the user is
open (see [Desktop check](#desktop-check)).
Date: 2026-11-04. Branch: `claude/pr66-logout`, merged into `main` with a
fast-forward. Start HEAD: `1e8a1d03b`. Commits: `f36e66ffe` merges
[PR #66](https://github.com/ruizhi-lab/latte-dock-ng/pull/66) (`0dc44bdf5`,
VerrPower), `6d42a9d71` applies the
[review](https://github.com/ruizhi-lab/latte-dock-ng/pull/66#pullrequestreview-5683067808).

## Scope and behavior

- **Merge.** The pull request replaces KWin's window list with session events
  in the quit decision: plasma-shutdown leaving `org.kde.Shutdown` while
  ksmserver still reports the logout is the completion. Main had added the Meta
  hold preference next to the pull request's new option; the merge keeps both
  and moves the option's checkbox to the end of the Preferences grid because
  main uses row 4 for the Meta hold checkbox.
- **Root cause of the stall.** KWin's `closeWaylandWindows()` waits for every
  `xdg_toplevel` but never sends close to a panel-role (dock) window. A
  confirmed logout hid the dock views, but the screen-edge ghost window of
  AutoHide, Dodge and WindowsCanCover docks and the floating gap window stayed
  mapped, so KWin waited for them until its "The following applications did not
  close" notification. The pull request worked around this by quitting the dock
  at the confirmation.
- **Follow-up fix** (`6d42a9d71`):
  - The confirmed-logout hook also suspends the helper windows
    (`VisibilityManager::setHelperWindowsSuspended()` through
    `Layouts::Synchronizer`): they are deleted and none is created until the
    logout is resolved. A cancel shows the views and recreates the helper
    windows.
  - The "Quit immediately on logout" option and the 8 s quit fallback are
    removed. Both quit while KWin can still offer "Cancel Logout". The dock now
    stays until plasma-shutdown leaves: a cancel restores it, a completion
    quits it.
  - `Session::ksmServerShutdownState()` reports a ksmserver query without an
    answer as no value, and `ShutdownMonitor::evaluate()` keeps its state for
    it instead of reading a cancel.
  - The session-end `::_Exit()` is removed. It also ran for every SIGTERM,
    SIGHUP and SIGINT quit (`--replace` sends SIGTERM).
  - `AbstractWindowInterface::hasSessionBlockingWindows()` and its contract
    checks are removed; nothing calls it since the window list left the
    decision.
  - The help pages lose the option's row; the architecture overview describes
    the logout handling.

## Evidence

| Check | Result |
| --- | --- |
| Merge commit `f36e66ffe` alone, GCC Debug preset | 0 compiler warnings; `sourcecontracttest`, `shutdownmonitortest`, `handbookdocumenttest` passed |
| GCC Debug, `ctest --preset gcc-debug` at `6d42a9d71` | 0 compiler warnings; 59/59 passed |
| Clang Debug, `ctest --preset clang-debug` | 0 compiler warnings; 59/59 passed |
| GCC Release, `ctest --preset gcc-release` | 0 compiler warnings; 59/59 passed |
| Clang Release, `ctest --preset clang-release` | 0 compiler warnings; 59/59 passed |
| `shutdownmonitortest` | 12 cases: idle, service start and stop, confirm, a confirmed logout that waits while plasma-shutdown runs, cancel, completion, unanswered queries (no logout started, no cancel, no completion, completion once answered), signal-only flag, ownership watcher, pre-existing owner, ksmserver states including no owner, bounded unanswered query; 4.7 s instead of 10.5 s |
| clazy gate in `latte-r2c-clazy:noble` (clazy 1.17, LLVM 19.1.1) | Container Clang Debug build: 0 compiler warnings. 49 negative fixtures rejected; 155 production sources, no findings from 49 gated checks |
| Debian 13 container, `verify-install.sh debian13`: CMake 3.31.6, GCC 14.2.0, Qt 6.8.2, KF 6.13.0, libplasma 6.3.5, plasma-workspace 6.3.6 | Exit 0 in 375 s: system and user build, install and uninstall; 0 compiler warnings |
| Nested KWin logout simulation, below | The dock no longer holds KWin's close step; cancel and completion work |
| `coredumpctl` after the simulation | No new `latte-dock-ng` entry |

### Nested KWin logout simulation

KWin 6.7.5 (`kwin_wayland --virtual`) ran on a private bus from
`dbus-run-session`, with its own Wayland socket. A Python service owned
`org.kde.ksmserver` (`isShuttingDown()` on request) and took and released
`org.kde.Shutdown` as plasma-shutdown does. The dock ran with a scratch `HOME`,
XDG tree and `TMPDIR` (its instance lock lives in `QDir::tempPath()`), never with
`--replace`, from a user-mode Debug install into that scratch `HOME`. Its default
layout has one DodgeActive dock with KWin edges, so it maps two panel-role
windows: `#view#1` and the ghost window `#subwindow#1`. A KWin script listed the
windows; `org.kde.KWin.Session.closeWaylandWindows` was called the way
plasma-shutdown calls it. The blocker is a QML window that refuses close
requests, like an editor with unsaved text.

| Scenario | Binary | Result |
| --- | --- | --- |
| Confirm, close step, cancel, confirm, completion | `6d42a9d71` | Confirm: dock windows 2 → 0, the dock keeps running. `closeWaylandWindows` returned `true` after 0.01 s. Cancel: both windows back. Completion: "logout completed; quitting.", exit status 0 after 0.2 s, teardown markers `Latte Corona - deleted...` and `QuickWindowSystem destructed`. Repeated twice more with the same result |
| Blocker, confirm, close step, cancel | `6d42a9d71` | Confirm: dock windows 2 → 0. KWin after 10 s: `Not closed windows QList("• LatteSim blocker")`. The dock kept running; cancel brought both windows back |
| Blocker, confirm, close step, cancel | `/usr/bin/latte-dock-ng` 1.2.62 (before PR #66) | Confirm: dock windows 2 → 1, `#subwindow#1` stays mapped. KWin after 10 s: `Not closed windows QList("• LatteSim blocker", "• Latte 停靠栏")`. Cancel brought the view back |
| SIGTERM quit, three runs | `6d42a9d71` | `KSignalHandler received signal 15`, exit status 0 after 0.2 s, both teardown markers, no core |

The six session-ending quits (three completions, three SIGTERMs) ran the full Qt
platform teardown without the `wl_list_remove` fault that the pull request's
`::_Exit()` skipped. The fault was reported on Fedora 44 with Qt 6.11.2; it did
not reproduce on this host.

### Simulation script and an isolation fault

The simulation is now `scripts/test-logout-simulation.py`
([Logout Simulation](../development-testing-guide.md#logout-simulation)).

The hand-run simulations above started `dbus-run-session` with the desktop
environment and gave the scratch `HOME` only to KWin, the fake services and the
dock. D-Bus activation on those private buses therefore started
kactivitymanagerd, xdg-desktop-portal-kde and ksecretd with the real `HOME` and
the desktop's `wayland-0`, and the processes outlived their buses. On
2026-11-04, 33 of them were found by the private bus address in their
environment and stopped; the session's own instances were not touched. The
activity database passed a read-only `PRAGMA integrity_check`, and
`kactivitymanagerdrc`, `kdewallet.kwl` and the flatpak permission store were not
modified. kactivitymanagerd refreshed its own `resources/test-backup` copy, as
it does at every start. The script gives every scenario a bus without service
directories, passes the isolated environment to the bus itself and stops each
process that still carries the scenario's environment marker.

| Run of the script | Dock | Result |
| --- | --- | --- |
| All scenarios, host KWin 6.7.5 | User-mode install of `6d42a9d71` | 3/3 passed: confirm hides 2 → 0 windows, close step `true` after 0.00 s, cancel restores 2 of 2, completion and SIGTERM exit 0 with the teardown markers; `blocker` reports only the blocker. No process left on a private bus |
| `blocker` and `confirm-cancel-complete`, host KWin 6.7.5 | `/usr/bin/latte-dock-ng` 1.2.62 | Both FAILED as expected. `blocker`: `#subwindow#1` stays mapped and KWin also lists 「Latte 停靠栏」. `confirm-cancel-complete`: without another application the old dock quits at the confirmation, so the cancel has nothing to restore |
| All scenarios, Fedora 45 VM: KWin 6.8.2, Plasma 6.8.2, Qt 6.11.2, GCC 16.2.1 | User-mode install built natively by `--install` (`install.sh --user Debug`, 0 compiler warnings) | 3/3 passed with the seeded `kdeglobals`: same results as on the host, close step `true` after 0.00 s, completion and SIGTERM exit 0 with the teardown markers. No new core |
| Same VM, simulation HOME without `kdeglobals` | Same | The logout checks passed, but every quit crashed (four cores): SIGSEGV in `QObject::deleteLater()` called from `PlasmaExtended::Theme::~Theme()` in `~Corona()`. With the default Plasma theme, which has no `colors` file, and no `kdeglobals`, `SchemeColors::possibleSchemeFile()` finds no scheme, `Theme::setOriginalSchemeFile()` returns early, and the destructor calls `deleteLater()` on the null scheme pointers. A real Plasma session has a `kdeglobals`. Fixed by `874f05540`, see [the follow-up](#follow-up-crash-on-quit-without-a-color-scheme) |

### Follow-up: crash on quit without a color scheme

`874f05540` fixes the crash that the Fedora 45 run without `kdeglobals` found.
`~Theme()` no longer calls `deleteLater()` on the default and reversed schemes:
they are children of the Theme and are deleted with it, and they stay null
while no scheme file was found. `SchemeColors::possibleSchemeFile()` falls back
to `BreezeLight`, Plasma's default scheme, when `kdeglobals` does not exist, as
it already did when `kdeglobals` names no scheme. The simulation gained
`--no-kdeglobals` to run a logout for such a new account.

| Check | Result |
| --- | --- |
| `schemecolorsunittest` | 10/10: the `kdeglobals` `ColorScheme` entry, no scheme file at all (empty result), and the `BreezeLight` fallback without `kdeglobals`. The test now points `XDG_CONFIG_HOME` and `XDG_CONFIG_DIRS` at empty directories: KConfig merged the developer's own `kdeglobals` into the test's file, so its `[WM]` colors turned the case into an auto-accent one |
| `sourcecontracttest` | `plasmaThemeDestructorToleratesMissingSchemes` passes; the destructor on `deedbd382` fails it |
| Fedora 45 VM (KWin 6.8.2), native `install.sh --user Debug` of `874f05540`, `--no-kdeglobals` | 0 compiler warnings. The dock resolved `/usr/share/color-schemes/BreezeLight.colors`; 3/3 scenarios passed, completion and SIGTERM exit 0 with the teardown markers, `latte-dock-ng` core count 6 before and 6 after. Before the fix the same HOME gave a SIGSEGV on every quit |
| GCC and Clang, Debug and Release presets at `874f05540` | 0 compiler warnings; 60/60 `ctest` each |
| clazy gate in `latte-r2c-clazy:noble` (clazy 1.17, LLVM 19.1.1) | Container build 0 warnings; 49 negative fixtures rejected; no findings in 155 production sources |
| Debian 13 container, `verify-install.sh debian13` | Exit 0 in 276 s: system and user build, install and uninstall; 0 compiler warnings |
| Debian 13 VM (KWin 6.3.6, Plasma 6.3.6): user-mode Debug install of `874f05540` built in the `latte-verify-debian13` container under the VM's HOME path (0 compiler warnings) and copied over; the VM has no build tools | Without `kdeglobals` the dock resolved `/usr/share/color-schemes/BreezeLight.colors`; `confirm-cancel-complete` and `sigterm` passed (11 checks, 0 failed), and again with the seeded `kdeglobals`. No `latte-dock-ng` core. `blocker` was left out: the VM has no Qt 6 `qml` runtime |

## Not run

- A real KDE logout on the desktop: the user's [desktop check](#desktop-check).
  The simulation has no plasmashell, no ksmserver session saving and no real
  plasma-shutdown.
- Floating docks (`FloatingGapWindow`), the multiple-layouts mode, reboot and
  shutdown confirmations, and Plasma versions other than 6.7.5 and 6.8.2 (the
  Fedora 45 VM ran the simulation only, not a real logout).
- CI for the pushed `main`.

## Desktop check

The dock on the desktop is the packaged 1.2.62. The layout `我的布局` has two
AutoHide docks with KWin edges, so both map a ghost window: no setting needs to
change. Install the new build for the current user (`bash install.sh --user
Debug`), stop the packaged dock (`pkill -x latte-dock-ng`) and start the
user-mode binary as in the Development Debug & Retest Workflow of `AGENTS.md`.

1. Open KWrite and type a few characters without saving. Log out from the
   application launcher (开始菜单 → 注销) and confirm 「注销」. KWrite asks to
   save; after about 10 s KWin shows 「以下应用程序没有关闭：…仍将在 2 分钟内注销。」.
   Expected: the list names KWrite only, not 「Latte 停靠栏」. Click
   「取消注销」: the session stays, both docks come back and appear when the
   pointer touches their screen edge.
2. Repeat step 1 and click 「仍要注销」: the session ends. After the next login
   there is no crash notification, and `coredumpctl list latte-dock-ng` has no
   new entry.
3. Close KWrite and log out with no unsaved document (a minimized tray
   application may stay open): the session ends without the 10 s wait and
   without the notification.

The debug log `/tmp/latte-ng.log` of steps 1 and 2 shows `isShuttingDown() =
true`, then `logout cancelled` or `logout completed; quitting.`.
