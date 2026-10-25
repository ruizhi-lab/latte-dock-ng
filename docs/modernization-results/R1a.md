# R1a: Idle Wakeup Removal and Shutdown Hardening

Status: Complete.
Date: 2026-10-25. Branch: `main`. Start HEAD:
`cb94d71c62e37f35e6b6a4a7dafd16a380142efb` (`v1.2.62`). Implementation commit:
`db966ccd6` (`perf: stop idle polling in shutdown and icon paths`), pushed to
`origin/main` with explicit user approval.

## Scope and behavior

- `Session::ShutdownMonitor` ([shutdownmonitor.cpp](../../app/session/shutdownmonitor.cpp))
  replaces the session-long 500 ms poller in `main()`. It watches
  `org.kde.Shutdown`, which plasma-shutdown owns only after a logout is
  confirmed, and samples ksmserver and KWin only while that name is owned or a
  sampled logout is unresolved. The hide, restore-on-cancel and quit decisions
  and `shouldQuitForCommittedShutdown()` are unchanged. A session-ending flag
  set by a signal never starts sampling.
- ksmserver queries, including the one in `Corona::onAboutToQuit()`, use
  `Session::isKsmServerShuttingDown()` with a 1 s timeout.
- The startup Wayland retry loop is removed: `KWindowSystem` caches the QPA
  platform chosen at `QApplication` construction, so retrying only delayed the
  failure.
- The SIGINT self-pipe is created with `pipe2(O_CLOEXEC | O_NONBLOCK)`.
- QML-owned items held by `View`, containment `LayoutManager`, `Effects`,
  `IndicatorUiManager`, `ContextMenuLayerQuickItem` and `Interfaces::m_plasmoid`
  are `QPointer`s. `ContextMenuLayerQuickItem` falls back to geometric hit
  testing when its cached item is gone. The other `Interfaces` references
  already had `destroyed` handlers and were left unchanged.
- The original-colors icon fallback in `ItemWrapper.qml` refreshes on the
  backend `Plasma::Applet` `iconChanged` signal (resolved through
  `statusSource`) instead of a 100 ms timer.

## Evidence

| Check | Result |
| --- | --- |
| GCC and Clang Debug/Release (`LATTE_STRICT_WARNINGS=ON`) | 0 compiler warnings; 55/55 CTest each |
| Standalone build of the commit in a detached worktree | GCC and Clang Debug: 0 warnings, 55/55 |
| `shutdownmonitortest` (private D-Bus) | 10 cases: idle, service start/stop, confirm, cancel, commit, signal-only flag, ownership watcher, pre-existing owner, ksmserver reply, unanswered ksmserver bounded at about 190 ms for a 200 ms timeout |
| Deep qmllint, before vs after on the same host | 0 added, 0 removed diagnostics; CI QML lint job passed for `a2bb619ae` |
| Project clang-tidy checks on changed sources | Pass |
| Idle wakeups (30 s, hardware) | Main thread 16.0 → 0.16/s; D-Bus thread 8.0 → 0/s; CPU 0.27 % → below resolution |
| Self-pipe flags at runtime | `02004000` / `02004001` (`O_CLOEXEC`, `O_NONBLOCK`) |
| SIGINT and SIGTERM quit | Clean teardown markers, no core |
| Committed logout (user) | `org.kde.Shutdown` registered → `isShuttingDown()` 0.47 s later → views hidden → quit when ordinary windows closed; session ended in the same second |
| Logout cancelled after confirmation (user) | Registered → hidden at the first sample → `unregistered` → `logout cancelled` → views restored |
| Trash applet with original colors (user) | Icon follows the trash state |

The only `latte-dock-ng` core recorded during this work was a deliberate
SIGABRT used to capture the pre-existing quit hang fixed in R1b.

## Limitations

- Two-output and VM behavior were not retested for this slice.
- Remote CI for the pushed commits was still running when this record was
  written; its QML lint job had passed.

## Next action

None for R1a. Continue with R2a in the
[R track ledger](../architecture-modernization-plan.md#assessment-refresh-r-track).
