# R7e: Declared QML components replace Qt.createQmlObject

Status: Pending validation. Both Debug builds (GCC 16.2.1 and Clang 22.1.8) compile
with zero warnings and zero errors, both full ctest runs pass 61 of 61, and the deep
qmllint comparison adds no diagnostic identity. The desktop checks listed under
[Desktop checks for the user](#desktop-checks-for-the-user) have not been run here.

Date: 2026-11-06. Branch: `claude/r7e-createqmlobject-d6fee3`, started at
`e18a0f9a7b85c6fd03d15264fa59582468aaa89e` (main, release v1.3.0). Implementation
commit: `52b2c17d5` (`refactor: declare QML components instead of Qt.createQmlObject`).

## Defect

Three call sites built QML from a string at run time with `Qt.createQmlObject()`:

- `containment/package/contents/ui/main.qml`, `createAppletItem()`: the retry `Timer`
  (50 ms, repeating) that re-runs `initAppletContainer()` for up to 80 attempts while an
  applet's graphic object is not ready.
- `plasmoid/package/contents/ui/ContextMenu.qml`, `newMenuItem()` and `newSeparator()`:
  `PlasmaExtras.MenuItem` rows, the second with `separator: true`, used by the task
  context menu and its nested menus.
- `plasmoid/package/contents/ui/ContextMenu.qml`, `loadDynamicLaunchActions()`: a
  `TextMetrics` that elides long launcher-action, place and recent-document labels.

The QML inside the strings was invisible to qmllint, was not compiled with its file,
and a mistake in it would only appear at run time.

## Fix

**Retry timer (`main.qml`).** A declared template sits in the components block after
the `Upgrader` block, so the `appletItemComponent` slice that the existing source
contract reads is unchanged:

```qml
Component {
    id: appletRetryTimerComponent
    Timer {
        interval: 50
        repeat: true
    }
}
```

`createAppletItem()` now calls `appletRetryTimerComponent.createObject(appletContainer)`.
The rest of the retry path is unchanged: the callback still stops and destroys the timer
on success and at the 80th attempt, and the ceiling `retryCount >= 80` is untouched.

**Menu rows and metrics (`ContextMenu.qml`).** Three properties on the root
`PlasmaExtras.Menu`:

```qml
readonly property Component menuItemComponent: Component {
    PlasmaExtras.MenuItem {}
}
readonly property Component separatorComponent: Component {
    PlasmaExtras.MenuItem {
        separator: true
    }
}
readonly property Component textMetricsComponent: Component {
    TextMetrics {}
}
```

`newMenuItem(parent)` and `newSeparator(parent)` return `createObject(parent)` on the
first two. `loadDynamicLaunchActions()` creates its metrics with
`textMetricsComponent.createObject(menu)`.

The templates must be properties, not child objects. The default property of
`PlasmaExtras.Menu` accepts only `MenuItem`. A probe that declared
`Component { id: ...; PlasmaExtras.MenuItem {} }` as a child of the root menu failed with
`Cannot assign object of type "QQmlComponent" to list property "content"; expected "QMenuItem"`,
which stops the whole file from loading. A comment beside the properties records this.

The unversioned `org.kde.plasma.extras` import that the file already has resolves
`MenuItem` to the same type as the removed `2.0` string import. The installed plugin
exports only `org.kde.plasma.extras/MenuItem 2.0`
(`/usr/lib64/qt6/qml/org/kde/plasma/extras/plasmaextracomponentsplugin.qmltypes`). The
file's `QtQuick` import provides the same `TextMetrics` that the removed `QtQuick 2.4`
string imported.

**Behaviour.** No intended change. The created objects keep the same parent, properties
and destruction as before. The only difference is when the QML is compiled: once with
its file, instead of on every call.

## State authority and lifetime

- Retry timer: the authoritative state is the return value of `initAppletContainer()`.
  The timer's parent is `appletContainer`, and the callback destroys it explicitly on
  success and at the 80th attempt, as before. Destroying `appletContainer` also destroys
  the timer.
- Menu rows: the parent passed by the caller owns them. `createObject(parent)` makes that
  parent the owner, as `Qt.createQmlObject(qml, parent)` did, so they are destroyed with
  it. Callers pass the root menu or a nested menu (`virtualDesktopsMenu`,
  `activitiesDesktopsMenu`, `activitiesLaunchersMenu`), as before.
- Text metrics: parent `menu`. `onStatusChanged` destroys the menu on close
  (`plasmoid/package/contents/ui/ContextMenu.qml:63`), so the metrics go with it, as
  before. `loadDynamicLaunchActions()` runs once per open, so there is no growth beyond
  one menu's lifetime.

## Files

- `containment/package/contents/ui/main.qml`: retry timer template and its call site.
- `plasmoid/package/contents/ui/ContextMenu.qml`: three property templates,
  `newMenuItem()`, `newSeparator()`, the metrics line, and the explanatory comment.
- `autotests/sourcecontracttest.cpp`: two contract functions and their declarations. No
  existing assertion was edited.
- `docs/modernization-results/R7e.md`: this record.

Not changed, as instructed: `docs/architecture-modernization-plan.md` and
`docs/qmllint-baseline.json`.

## Evidence

### Commands and exit status

Run in the worktree. Every build used `--parallel 6`. Raw logs were written under
`/tmp/r7e-logs/` during the run; their results are summarised here.

| Step | Command | Exit |
| --- | --- | --- |
| 1 | `git grep -n 'Qt.createQmlObject'`, before edits | 0: three production hits (`main.qml`, two in `ContextMenu.qml`) and one prose hit in `docs/architecture-assessment-2026-10.md` |
| 2 | `cmake --preset gcc-debug` | 0 |
| 3 | `cmake --build --preset gcc-debug --parallel 6`, pristine tree | 0 |
| 4 | `bash scripts/qmllint-deep.sh build/modernization/gcc-debug`, pristine tree | 1 (fingerprint message, see qmllint below) |
| 5 | `cmake --build --preset gcc-debug --target sourcecontracttest --parallel 6`, after the test edit | 0, no warnings |
| 6 | `ctest --preset gcc-debug -R '^(sourcecontracttest\|qmlsmoketest)$'` | 0 (2/2) |
| 7 | `build/modernization/gcc-debug/bin/sourcecontracttest <new and related functions> -v1`, and mutation runs M1-M4 | 0 on the edited tree; 2 while each mutation was in place; 0 after restore |
| 8 | `bash scripts/qmllint-deep.sh build/modernization/gcc-debug`, edited tree | 1 (same fingerprint message) |
| 9 | `python3 scripts/qmllint-baseline.py compare --baseline <before> --current <after> --output <delta>` | 0 (added 0, removed 0) |
| 10 | `cmake --build --preset gcc-debug --parallel 6`, full | 0 |
| 11 | `ctest --preset gcc-debug` | 0 (61/61, 33.17 s) |
| 12 | `cmake --preset clang-debug` | 0 |
| 13 | `cmake --build --preset clang-debug --parallel 6`, full | 0 |
| 14 | `ctest --preset clang-debug` | 0 (61/61) |
| 15 | `git diff --check` | 0 |
| 16 | After a comment-only rewording in `ContextMenu.qml`, on the final tree: GCC build (step 10), GCC ctest (step 11), Clang build (step 13), Clang ctest (step 14), deep lint (step 8), comparison (step 9) | GCC build 0; GCC ctest 0 (61/61); Clang build 0; Clang ctest 0 (61/61); deep lint 1 (same fingerprint message); comparison 0 (added 0, removed 0) |
| 17 | `git grep -n 'Qt.createQmlObject'` on the final tree | 0. The only matches are the two `!src.contains` assertions in `autotests/sourcecontracttest.cpp` and one prose line in `docs/architecture-assessment-2026-10.md`. No production QML matches |

Reproducible qmllint evidence (git-ignored) is written to
`build/modernization/gcc-debug/qmllint-baseline/` by step 8.

### GCC and Clang warning results

- GCC Debug, full build: 0 compiler `warning:` lines, 0 `error:` lines, 0 make errors.
- Clang Debug, full build: 0 compiler `warning:` lines, 0 `error:` lines, 0 make errors.
- Release presets were not built, because the task asks for Debug.
- The build's `app/FakeTarget.cmake` PRE_BUILD step runs `qmllint` over the shell,
  containment and plasmoid sources and prints `Warning:` lines, about 5,490 per run.
  These are not compiler output. Two runs on the same pristine tree differ
  (`plasmoid/package/contents/ui/taskslayout/ScrollableList.qml` 60 vs 61, and
  `shell/package/contents/views/WidgetExplorer.qml` 52 vs 51), so that output is not a
  usable gate. `scripts/qmllint-deep.sh` is the gate.

### Tests

- Registered: 61 ctest tests for each compiler, with the same names.
  Run and passed: 61/61 for GCC and for Clang. Skipped: 0. Failed: 0.
- Name delta: none in ctest. `sourcecontracttest` gains two functions,
  `contextMenuFactoriesUseDeclaredComponentsNotStringCompilation` and
  `mainQmlRetryTimerUsesDeclaredComponentNotStringCompilation`.
- Existing checks that read the same files still pass unchanged:
  `mainQmlCreateAppletItemRetryCeilingAt80` (`retryCount >= 80`, `createAppletItem`),
  `contextMenuSeparatorRemovalRoutesThroughGuardedLauncherPath` (its three ids), and the
  `qmlsmoketest` functions `containmentWheelScrollsTaskViewport` and
  `editModeBlockerLeavesRightClicksToContextMenu`, which slice `main.qml` on markers this
  change does not touch.

### Source-contract checks and mutation evidence

Source contracts read text, so they lock the source shape; they are not proof of runtime
behaviour. The two new functions check:

- no `Qt.createQmlObject` in either file;
- the retry call text `var retryTimer = appletRetryTimerComponent.createObject(appletContainer);`,
  a template containing `interval: 50` and `repeat: true`, and `retryTimer.destroy();` exactly
  twice;
- the three `readonly property Component` declarations and their `createObject(...)`
  call sites, none of them in `id:` form;
- the separator template keeps `separator: true`.

Mutations, each applied to the worktree file, run through the test binary, then the file
restored from a saved copy (SHA-256 compared) and the test re-run:

| # | Mutation | Test that fails | Failing check |
| --- | --- | --- | --- |
| M1 | `newMenuItem` returns the string call again | `contextMenuFactoriesUseDeclaredComponentsNotStringCompilation` | `!src.contains("Qt.createQmlObject")` |
| M2 | Retry timer back to the string call | `mainQmlRetryTimerUsesDeclaredComponentNotStringCompilation` | `!src.contains("Qt.createQmlObject")` |
| M3 | Success-branch `retryTimer.destroy();` removed | `mainQmlRetryTimerUsesDeclaredComponentNotStringCompilation` | destroy count 1, expected 2 |
| M4 | Separator template `separator: false` | `contextMenuFactoriesUseDeclaredComponentsNotStringCompilation` | `separatorBlock.contains("separator: true")` |

M1 and M3 ran together, then M2 and M4 together. Each was caught by its own test, and
the other checks stayed green.

### Qt behaviour probes (outside the repo, not registered tests)

Scratch QML files under `/tmp/r7e-qmlcheck`, run with `QT_QPA_PLATFORM=offscreen qml6`,
Qt 6.11.2, with the installed `org.kde.plasma.extras`. They check the Qt behaviour the
change relies on. They are not production code and do not replace the desktop check.

- A declared child `Component` inside `PlasmaExtras.Menu`: the file fails to load
  (`expected "QMenuItem"`, quoted above).
- A `readonly property Component` inside `PlasmaExtras.Menu`: loads, and
  `createObject(menu)` of a `TextMetrics` returns an object.
- Ownership: destroying an owner `Item` destroys both a child from
  `Component.createObject(owner)` and a child from `Qt.createQmlObject(..., owner)`;
  `Component.onDestruction` fired for both.
- Retry timer: the component instance and the string-built timer both report
  `interval=50 repeat=true running=false`; `start()` sets `running=true`; `destroy()`
  on the component instance runs its destruction.
- Limitation: creating a `PlasmaExtras.MenuItem` segfaults (exit 139) under headless
  `qml6`. The legacy `Qt.createQmlObject` path crashes the same way, with the default
  style and with `QT_QUICK_CONTROLS_STYLE=Basic` and `Fusion`. The menu row path
  therefore cannot run in this sandbox, and the crash does not come from this change.

### qmllint, same host before and after

- Before (pristine tree): `scripts/qmllint-deep.sh build/modernization/gcc-debug` linted
  241 files and wrote 717 identities, 7,328 total count, with qmllint 6.11.2
  (`/usr/lib64/qt6/bin/qmllint`). The snapshot was copied to
  `/tmp/r7e-logs/qmllint-before.json`.
- After the edits: the same 241 files, 717 identities, 7,328 total count.
- `scripts/qmllint-baseline.py compare`: added 0, removed 0. On the two edited files,
  58 identities before and 58 after, with no count change.
- Final tree (after the comment-only rewording): 241 files, 717 identities, 7,328 total
  count, the same as before. The comparison gives added 0 and removed 0 (the snapshot is
  `/tmp/r7e-logs/qmllint-final.json`).
- The script exits 1 before and after with
  `qmllint-baseline: error: tool fingerprint changed; establish a reviewed comparable baseline`.
  That is the local-versus-CI fingerprint mismatch AGENTS.md describes, not a lint finding.
  `docs/qmllint-baseline.json` was not edited, as instructed.

### Toolchain and artifact identity

- GCC 16.2.1 (`/usr/bin/g++`) and Clang 22.1.8 (`/usr/lib/llvm/22/bin/clang++`), from the
  `gcc-debug` and `clang-debug` presets. Build trees are under `build/modernization/`
  (git-ignored).
- Qt 6.11.2 (`/usr/lib64/cmake/Qt6`), qmllint 6.11.2.
- The machine is shared with other sessions, so builds used six jobs.

## Desktop checks for the user

This session did not run the dock against a desktop session (its rules forbid it). Please run these on
a user-mode Debug build of this branch, and review the debug log (`/tmp/latte-ng.log`, or
the `--log-file` you choose):

1. Dock start with several applets. The retry path only runs when an applet's graphic
   object is not ready at start-up, so it may not be exercised. Check that every applet
   appears, and that the log has no new QML warnings or errors from `createAppletItem`
   or `appletRetryTimerComponent`.
2. Right-click a task. Task menu items and separators appear as before, the launcher
   actions and places appear, and long entries are elided as before (the elide width is
   `gridUnit * 22`).

Clean-quit and coredump checks were not run. This change does not touch quit or teardown
code. The Runtime Retest Workflow in `docs/development-testing-guide.md` is the reference
if you want it.

## Not verified here

- The desktop checks above.
- Debian 13 verification: deferred by the user until the refactor is complete.
- clazy: not installed here. The new test code has no container loops, lambdas or
  `connect()` calls.
- Release builds: not run, because the task asks for Debug.
- GitHub CI (the clazy gate and the CI qmllint fingerprint): the baseline refresh belongs
  to the main session, from CI evidence.

## Unresolved and review notes

- Headless MenuItem crash: a sandbox limitation (see the probes), not a regression.
- The build's PRE_BUILD qmllint output is non-deterministic (see Warning results).
- Merge note: sibling branches that edit `ContextMenu.qml` near `newMenuItem()` and
  `newSeparator()` (about lines 346-368), or the metrics line (about line 506), will
  conflict textually. Keep the property form when resolving.

## Next action

Run the two desktop checks above on a user-mode Debug build of
`claude/r7e-createqmlobject-d6fee3`, and report the log review. After that, the main
session refreshes `docs/qmllint-baseline.json` from CI evidence.

## Commit approval status

Committed on `claude/r7e-createqmlobject-d6fee3`, as the task requested. Not pushed; no push
was requested.
