# R2c-connect-3arg-lambda: Timers Without a Context Object

Status: Pending validation. The evidence section below lists what was run; the
desktop smoke and CI were not run here.
Date: 2026-10-29. Branch: `claude/r2c-batch-f-e6a6ee`. Start HEAD: `95e342f72`.
Implementation commit: the commit that adds this report. Part of [R2c](R2c.md),
fix batch 3 (batch F, the first of its two slices; the second is
`R2c-non-pod-global-static`).

## Defect

clazy `connect-3arg-lambda` reported eight `QTimer::singleShot(ms, lambda)` calls
with no context object, in five files. Such a timer runs its functor when it
fires, whatever has happened to the objects the functor uses. The overload with a
context object runs the functor in the context's thread and drops the call when
the context is destroyed first.

Four of the eight calls use an object that the timer does not own, so the call
could run on freed memory:

- `Manager::showInfoWindow()` deletes an `InfoView` that has no parent. Nothing
  else deletes it today, so the risk is latent.
- `WaylandInterface::setViewExtraFlags()` changes the role of a
  `PlasmaShellSurface` owned by its view. A view deleted within 50 ms would have
  made the call on freed memory.
- `Menu::addView()` and `Menu::moveToLayout()` call `containment()` on the
  `Menu`. A `Menu` destroyed within 400 ms would have made the call on freed
  memory.

The other calls use only process-wide objects or copied values. One of them is
in `Synchronizer::pauseLayout()`, which nothing calls (see below).

## Fix

Each call names the object whose lifetime bounds its callback. The four lambdas
that already captured `this` or the object keep their captures.

| Site | Context | Lifetime before the change | Lifetime after the change |
| --- | --- | --- | --- |
| [lattecorona.cpp](../../app/lattecorona.cpp):1208, quit 800 ms after the views start hiding | `qGuiApp` | Process-wide: the lambda uses only `qGuiApp`, which outlives the timer, so nothing dangled. The quit did not depend on the Corona, and still does not. | Same call. The application is the context, so destroying the Corona first does not cancel the quit (comment at the site). |
| [manager.cpp](../../app/layouts/manager.cpp):501, `deleteLater()` on the info view after `duration` | `infoView` | Not bounded: the view has no parent, so no owner tied the callback to it. A view destroyed earlier would have been called through a dangling pointer. Nothing destroys it earlier today. | The call is dropped if the view is destroyed first. Otherwise unchanged. |
| [waylandinterface.cpp](../../app/wm/waylandinterface.cpp):537, bottom view role change after 50 ms | `surface` | Not bounded: the surface is a child of its view (`createSurface(s, this)`), so a view deleted within 50 ms would have left a dangling pointer. | The role change is dropped if the surface, and so its view, is destroyed first. |
| [menu.cpp](../../containmentactions/contextmenu/menu.cpp):484, `addView` after 400 ms | `this` (`Menu`) | Not bounded: the callback calls `containment()` on the `Menu`. A `Menu` destroyed within 400 ms would have made the D-Bus call on freed memory. | The D-Bus call is dropped if the `Menu` is destroyed first. |
| [menu.cpp](../../containmentactions/contextmenu/menu.cpp):494, `moveToLayout` after 400 ms | `this` (`Menu`) | Same as line 484. | Same as line 484. |
| [menu.cpp](../../containmentactions/contextmenu/menu.cpp):505, open the settings window after 400 ms | `QCoreApplication::instance()` | The lambda captures nothing and calls the file-local `callLatte()`, so the call did not depend on the `Menu` and still runs when the `Menu` is gone. | Same call, same lifetime. The application is the context, so the call is not cancelled with the `Menu`. Comment at the site. |
| [menu.cpp](../../containmentactions/contextmenu/menu.cpp):510, switch to a layout after 400 ms | `QCoreApplication::instance()` | Captures a copy of the layout name only; the same as line 505. | Same as line 505. |
| [synchronizer.cpp](../../app/layouts/synchronizer.cpp):506 in `pauseLayout()` | none: the function is deleted | Dead code (see below). The timers ran an empty lambda. | Removed with the function. |

The only behavior difference is on the path where a `this` or view object is
destroyed while its timer is pending. That path would previously have called into
freed memory; now the call is dropped. The calls on process-wide objects or copies
run as before.

## Dead code: `Synchronizer::pauseLayout()`

`pauseLayout(QString layoutName)` was a plain public method of `Synchronizer`
(no slot, no `Q_INVOKABLE`, declared in the `public:` section of
[synchronizer.h](../../app/layouts/synchronizer.h)). Its body created one timer
per applied activity, and each timer ran an empty lambda; the comment said
`stopActivity` had been removed in KF6. Its removal is the rule 5 dead-code case
in `AGENTS.md`: every consumer was traced before deletion.

| Search | Result |
| --- | --- |
| `git grep -n pauseLayout` over the whole tracked tree (C++, headers, QML, JS, CMake, string-based uses) before the change | 2 hits: the definition in `synchronizer.cpp` and the declaration in `synchronizer.h` |
| The same search after the change | 0 hits |
| Callers through `QMetaObject::invokeMethod`, `SLOT()`, `connect()` or QML | None: the name appears in no string, QML or JS file |

The change removes the definition, with the blank line after it, from
`synchronizer.cpp`, and the declaration from `synchronizer.h`. Nothing else in
those two files changed. Lines after the removed block move up by 27 lines in
`synchronizer.cpp`; the batches that fix its loops must re-read that file.

## Formatting

`git clang-format --style=file --diff HEAD` proposes joining each changed
`QTimer::singleShot` lambda onto one line (the project's `ColumnLimit` is 200).
The hunks also join the unchanged body lines of the same calls. The changed
calls keep the multi-line lambda form that the neighbouring calls in the same
functions use (`lattecorona.cpp`, `menu.cpp`), so the diff stays limited to the
lines whose context changed. `formatter.sh` was not run: it rewrites whole files.

## Evidence

Container: Ubuntu clang 18.1.3, clazy 1.17 on LLVM 19.1.1, Qt 6.11.1 (KDE neon
`noble` build). Host: GCC 16.2.1, Clang 22.1.8, Qt 6.11.2. The container runs
used the commands of [R2c](R2c.md#reproducing-the-environment) with a private
build tree (`~/r2c-build-batch-f`) and `--jobs 4`; the worktree is mounted
read-only, and its `build/` directory is git-ignored and serves as the mount
point. The host builds are fresh trees in `build/modernization/` of the worktree.

| Command | Result |
| --- | --- |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 4 --inventory connect-3arg-lambda,non-pod-global-static --report build/inventory.json`, at `95e342f72` | Exit 0: `connect-3arg-lambda` 8 findings in 5 files; `non-pod-global-static` 31 findings in 6 files; 18 in the generated copy of `contextmenudata.h`, not counted |
| Same, after the slice-1 edits | Exit 0: `connect-3arg-lambda` **0 findings in 0 files**; `non-pod-global-static` 31 in 6 files (slice 2, unchanged) |
| `cmake --build --preset clang-debug -j4` of the slice-1 tree in the container (incremental) | Exit 0; 0 compiler errors, 0 `warning:` lines; the log's other `Warning:` lines come from qmllint, not from the compiler |
| Full gate: `test-clazy.py --build-dir build/modernization/clang-debug --jobs 4` | **Exit 0**: 44 negative fixtures, each rejected by its own gated check; 153 production sources with no finding from 44 gated checks |
| Mutation: the pre-fix `containmentactions/contextmenu/menu.cpp` mounted read-only over the fixed file, then the full gate | **Exit 1**: 4 findings, `menu.cpp:484:5`, `494:5`, `505:9` and `510:9`, all `[connect-3arg-lambda]` |
| `git grep -n pauseLayout` before and after the change | 2 hits, then 0 |
| `git clang-format --style=file --diff HEAD` on the changed production files | `synchronizer.cpp` and `synchronizer.h`: no hunk. `menu.cpp`, `lattecorona.cpp`, `manager.cpp`, `waylandinterface.cpp`: only the joined-lambda hunks described under Formatting |
| `git diff --check` | Clean |
| Host `cmake --preset gcc-debug`, `cmake --build --preset gcc-debug -j4`, `ctest --preset gcc-debug` | Configure 0, 0 CMake warnings; build 0 with **0 compiler warnings**; **58/58 tests passed** (28 s) |
| Host `cmake --preset clang-debug`, `cmake --build --preset clang-debug -j4`, `ctest --preset clang-debug` | Configure 0, 0 CMake warnings; build 0 with **0 compiler warnings**; **58/58 tests passed** (23 s) |

No runtime test was added. The negative fixture guards the check, not the
behavior. `git grep` finds no test that calls `addView`, `moveToLayout`,
`switchToLayout`, `showInfoWindow`, `setViewExtraFlags` or `quitApplication`, so
the timer paths are covered only by the build and by the desktop smoke listed in
the Handoff.

## Handoff

- **Files and behavior changed:** `app/lattecorona.cpp` (1 call),
  `app/layouts/manager.cpp` (1), `app/wm/waylandinterface.cpp` (1),
  `containmentactions/contextmenu/menu.cpp` (4 calls and the
  `QCoreApplication` include), `app/layouts/synchronizer.cpp` and
  `synchronizer.h` (the dead `pauseLayout()`), `scripts/test-clazy.py` (one
  name), `autotests/clazy/connect-3arg-lambda.cpp`, this report. Behavior:
  unchanged, except on the destroyed-first path described under Fix.
- **State authority / lifetime constraints:** the context object is the
  authority for each callback's lifetime (table under Fix). The application is
  the context for the two D-Bus calls and for the quit, so they still run when
  the `Menu` or the Corona is gone.
- **Tests:** no test added. Host CTest 58/58 on GCC 16.2.1 and on Clang 22.1.8.
  The fixture rejects the old code in the gate.
- **Smoke plan (for the integration):** quit Latte from the dock context menu
  (`Menu::quitApplication` → the `quitApplication` D-Bus method →
  `Corona::quitApplication`, the 800 ms quit), and check that the process exits
  with no new `coredumpctl` entry and no Fatal or ASSERT line in the log; the
  settings dialog's quit and a full configuration import take the same path. Switch
  layouts and open the settings window from the dock context menu (the
  `switchToLayout` calls); add a view and move a view to another layout from the
  context menu (`addView`, `moveToLayout`); activate a layout with the information
  window enabled (`showInfoWindow`); with a dock in "windows can cover" or "windows
  always cover" mode, click a task and check that it still minimizes (the
  `atBottom` tooltip role). Look for `No such method` or `QDBusError` lines in the
  log.
- **Not run:** CI, the desktop smoke, a Qt 6.6 or 6.8 build, the Release
  presets. No test calls the changed paths (see Evidence).
- **Formatting:** `formatter.sh` was not run; the clang-format joins were not
  applied (see Formatting).
- **Decision for later:** the `this`-context calls in `menu.cpp` drop the D-Bus
  call when the `Menu` is destroyed within 400 ms. If that call must still reach
  Latte, read `containment()->id()` when the action fires and use the application
  as the context, as the two calls that use no `this` already do. The current
  code reads the id 400 ms later, so this was not changed here.
- **Line numbers in `synchronizer.cpp`:** lines after 514 of the original
  file moved up by 27. The batches that fix its loops must re-read the file.
- **Next exact action:** none in this slice; the second slice,
  `R2c-non-pod-global-static`, follows in this batch.
- **Commit approval status / push approval status:** the slice is committed on
  `claude/r2c-batch-f-e6a6ee` (a non-`main` branch) and not pushed.
