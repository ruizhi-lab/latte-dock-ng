# R1b: Quit Teardown Hang

Status: Complete.
Date: 2026-10-25. Branch: `main`. Implementation commit: `a2bb619ae`
(`fix: skip the view settings factory freed by a normal quit`), pushed to
`origin/main` with explicit user approval.

## Defect

Quit from the dock menu, the settings window or D-Bus (`quitApplication`)
could leave an invisible `latte-dock-ng` process blocked forever.
`Corona::onAboutToQuit()` calls `m_viewSettingsFactory->deleteLater()` from
inside `QCoreApplication::exit()`. `exec()` processes that deferred delete
during its cleanup, before `main()` destroys the `Corona`. `~Corona()` then
called `deleteLater()` on the dangling raw pointer and blocked in
`QCoreApplicationPrivate::lockThreadPostEventList()` on a garbage mutex.

Session logouts and signals take the fast path, which returns before that line.
The result was undefined: the hang reproduced on both the installed `v1.2.62`
binary and a Debug build, but one quit of `v1.2.62` exited normally.

## Fix

- `m_viewSettingsFactory` is a `QPointer` ([lattecorona.h](../../app/lattecorona.h)).
- `~Corona()` deletes services through `Session::deleteNow()`
  ([teardown.h](../../app/session/teardown.h)), which skips a service that is
  already gone and keeps the dependency order. The other services are deleted
  only in `~Corona()`.

## Evidence

| Check | Result |
| --- | --- |
| Backtrace of the hung process (Debug core) | `QBasicMutex::lockInternal` ← `lockThreadPostEventList` ← `QObject::deleteLater` ← `~Corona()` at `flushDelete(m_viewSettingsFactory)` |
| `teardowntest` | Reproduces the quit order with a real `QCoreApplication::exec()`; the service is freed before `exec()` returns and `deleteNow()` skips it |
| Source contract | The early `deleteLater()` requires the guarded member; `~Corona()` has no direct `deleteLater()` |
| GCC and Clang Debug/Release | 0 compiler warnings; 56/56 CTest each |
| D-Bus `quitApplication` on the fixed build | Process exited about 1.0 s after the call (0.8 s of that is the existing slide-out delay); teardown markers present; no core |
| Right-click Quit on the fixed build (user) | Teardown completed in 41 ms after the quit started; process gone; no core |
| SIGTERM | Fast path unchanged; clean teardown |

## Next action

None for R1b.
