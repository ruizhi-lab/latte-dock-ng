# R7d: UniversalSettings::screens NOTIFY

Status: Pending validation. The code change, the source contract and the GCC and Clang
Debug builds and ctest runs are done in this worktree. The real-desktop hot-plug check and
the Debian 13 verification are left for the user (see Handoff).
Date: 2026-11-06. Branch: `claude/r7d-screens-notify-b84309`. Start HEAD: `e18a0f9a7`
(release v1.3.0). Implementation commit: `1c72ce577`. This record: the commit that adds it.
Follow-up of [R2c-qproperty-without-notify](R2c-qproperty-without-notify.md), item 4 of
[R2c](R2c.md#follow-ups). The resolution note is appended to that record.

## Defect

`UniversalSettings::screens` is a `QQmlListProperty<QScreen>` built from
`qGuiApp->screens()` on every read, but its `Q_PROPERTY` had no `NOTIFY`. R2c kept it as a
reviewed exception with `clazy:exclude=qproperty-without-notify`. A property without NOTIFY
tells QML that it never changes, so a binding on it would not re-evaluate after a screen was
added, removed or made primary.

## Fix

- `app/settings/universalsettings.h`: `Q_PROPERTY(QQmlListProperty<QScreen> screens READ
  screens NOTIFY screensChanged)`. The clazy exclusion and the exception comment are removed.
  The new signal `void screensChanged();` is declared next to `screensCountChanged()`.
- `app/settings/universalsettings.cpp`: the constructor connects `screensChanged()` to
  `QGuiApplication::screenAdded`, `screenRemoved` and `primaryScreenChanged`. The two
  `screensCountChanged()` connections are unchanged. A `//!` comment states the timing rule
  that makes the NOTIFY correct.
- `autotests/sourcecontracttest.cpp`: `screensPropertyNotifiesOnScreenChanges()`, a source
  contract for the header line, the absence of the exclusion, the three new connections and
  the two existing `screensCountChanged()` connections.
- `docs/modernization-results/R2c-qproperty-without-notify.md`: a "Resolved in" section is
  appended. Nothing above it was changed.
- `docs/modernization-results/R7d-data/`: the Qt probe source and output, the contract
  mutation output and the old-header failure excerpt, so the evidence can be rerun.

## Design decisions

1. **A new `screensChanged()` signal, not `screensCountChanged()`.** The brief allowed reusing
   `screensCountChanged()` if its name fitted. It does not fit a list property, and a
   primary-screen change reorders the list without changing its count (see the probe).
   `screensCountChanged()` keeps its two connections and its one consumer,
   `BehaviorConfig.qml`.
2. **A third connection, `primaryScreenChanged`.** The brief named the two add and remove
   signals. The probe showed that `QWindowSystemInterface::handlePrimaryScreenChanged()` on a
   screen already in the list moves it to the front of `screens()`, and that only
   `primaryScreenChanged` reports this. Without the third connection a NOTIFY reader would
   keep the old order. The cost is one extra emission when a screen is added as primary
   (`screenAdded`, then `primaryScreenChanged`). To get the strict two-signal version, delete
   the `primaryScreenChanged` line in the constructor and its line in the contract; nothing
   else depends on it.
3. **Timing rule (checked, not documented by Qt).** The NOTIFY is correct only if Qt has
   already updated `qGuiApp->screens()` when the signals are emitted. The probe shows that it
   has for all three signals on Qt 6.11.2. The code comment says so and names the failure
   mode.

## Facts checked

### Consumers

- `git grep` finds five reads of `universalSettings.screens` in QML, all in
  `shell/package/contents/configuration/pages/BehaviorConfig.qml`, at lines 61, 77, 78, 88 and
  89, all in the body of `function updateScreens()`. No binding reads the property.
- `updateScreens()` is called from three places: `function onScreensCountChanged()` in a
  `Connections` on `page.universalSettings` (line 47), `function onShowSignal()` in a
  `Connections` on `page.viewConfig` (line 107) and `Component.onCompleted` (line 121). None of
  them is connected to the new signal.
- `screensCountChanged()` has one consumer, the `Connections` at line 47. No C++ code calls
  `UniversalSettings::screens()`, `countScreens()` or `atScreens()` apart from the property's
  own metadata. `plasmoid/`, `containment/` and the other QML files do not read `screens`.

### Qt 6.11.2 emission order and lifetime (probe)

Probe: `docs/modernization-results/R7d-data/screen-order-probe.cpp`; its output is in
`screen-order-probe.txt`. It runs on the offscreen platform with synchronous QPA window-system
events, which call the same `QWindowSystemInterface` entry points that platform plugins call on a hot-plug.
It was **not** run on a real output hot-plug.

| Step | Signal and whether the removed or new screen is in the list | `screens()` when the handler runs |
| --- | --- | --- |
| add a non-primary screen (x=1000) | `screenAdded`: new screen listed | `[0,1000]` |
| add a primary screen (x=2000) | `screenAdded` (listed), then `primaryScreenChanged` | `[2000,0,1000]` in both |
| remove x=1000 | `screenRemoved`: removed screen not listed | `[2000,0]` |
| remove the primary screen (x=2000) | `primaryScreenChanged`, then `screenRemoved` (not listed) | `[0]` in both |
| add a non-primary screen (x=3000) | `screenAdded` (listed) | `[0,3000]` |
| primary change to x=3000, no add or remove | `primaryScreenChanged` only | `[3000,0]` |

- The list is updated before all three signals are emitted: a new screen is listed in
  `screenAdded`, and a removed screen is not listed in `screenRemoved`.
- The removed `QScreen` is destroyed after the `screenRemoved` handler returns (the probe prints
  `removed QScreen destroyed` after the handler's line), so a handler may still use it during
  the emission.
- A primary change alone reorders the list. Only `primaryScreenChanged` reports it.
- Not checked: Qt versions other than 6.11.2 (the declared floor is 6.6). The code comment
  says "verified on Qt 6.11.2" for this reason.

## QML re-evaluation analysis

- **More re-evaluation today: none.** No binding reads `screens`, so no binding re-evaluates
  more often than before. The NOTIFY matters for future bindings, which now re-evaluate on each
  change: once for a plain add, remove or primary change, and twice when a screen is added as
  primary or removed while primary.
- **`updateScreens()` runs as before.** Its three callers are unchanged, and none of them is
  connected to `screensChanged()`. A primary change now emits `screensChanged()`, but
  it still does not run `updateScreens()`, which matches the behaviour before this change.
  A possible follow-up is to listen to `screensChanged()` in `BehaviorConfig.qml`, so the screen
  combo follows a primary change. That changes when `updateScreens()` runs, so it is not part of
  this change.
- **No binding loop.** The property is read-only and its getter writes nothing. Its reads in QML
  are inside `updateScreens()`, which is a function body, not a binding, so they create no
  dependency. The function writes `screensCount`, `screensModel` and `screenCmb.currentIndex`;
  since no binding reads `screens`, those writes cannot re-enter a `screens` binding.
- **Timing.** `screensChanged()` is emitted synchronously from the three QGuiApplication
  signals, after Qt has updated the list. Its emission runs the same synchronous path that
  `screensCountChanged()` already runs for `updateScreens()`.
- No QML file changed. The qmltypes metadata of `UniversalSettings` now carries
  `notify: "screensChanged"`. The qmllint comparison below checks whether that changed any
  diagnostic.

## Sites

| Site | Change | What was checked |
| --- | --- | --- |
| `app/settings/universalsettings.h` (`Q_PROPERTY`, `Q_SIGNALS`) | NOTIFY added, exception and exclusion removed, `screensChanged()` declared | moc: property `screens` has notify index 13, which is method 13 `screensChanged()` |
| `app/settings/universalsettings.cpp` (constructor) | Three connections to `screensChanged()`; the two `screensCountChanged()` connections unchanged | Probe; contract; mutants M2, M4 and M5 |
| `autotests/sourcecontracttest.cpp` | New test `screensPropertyNotifiesOnScreenChanges` (declaration and definition) | Fails on the old header; mutants M1 and M3 |
| `shell/package/contents/configuration/pages/BehaviorConfig.qml` | Unchanged | The five reads and the three callers above |
| `docs/modernization-results/R2c-qproperty-without-notify.md` | "Resolved in" section appended | Nothing above it changed |

## Evidence

### The contract fails on the old header

The contract was added and built before the production change. The run is in
`R7d-data/contract-old-header-ctest.txt` (log `build/modernization/logs/gcc-sourcecontract-old-header-ctest.log`):

```text
FAIL!  : SourceContractTest::screensPropertyNotifiesOnScreenChanges() 'headerSource.contains(QStringLiteral("Q_PROPERTY(QQmlListProperty<QScreen> screens READ screens NOTIFY screensChanged)"))' returned FALSE. ()
   Loc: [.../autotests/sourcecontracttest.cpp(2134)]
Totals: 225 passed, 1 failed, 0 skipped, 0 blacklisted, 32ms
```

Commands: `cmake --build --preset gcc-debug --target sourcecontracttest --parallel 6` (exit 0),
then `ctest --preset gcc-debug -R sourcecontract --output-on-failure` (exit 8, test failed).

### Mutation evidence

Each mutant changed one line of production source, ran
`ctest --preset gcc-debug -R '^sourcecontracttest$'` and was then restored from a backup taken
before the first mutant. The output is `R7d-data/contract-mutation-evidence.txt`. The driver
script was a scratch file outside the repository.

| Mutant | Contract check that failed | Contract line | ctest exit |
| --- | --- | --- | --- |
| M1: NOTIFY removed from the `Q_PROPERTY` | `Q_PROPERTY(... NOTIFY screensChanged)` present | 2134 | 8 |
| M2: `primaryScreenChanged` connection removed | constructor connects `primaryScreenChanged` | 2150 | 8 |
| M3: clazy exclusion added back to the `Q_PROPERTY` line | `clazy:exclude=qproperty-without-notify` absent | 2137 | 8 |
| M4: `screenRemoved` connection to `screensChanged()` removed | constructor connects `screenRemoved` | 2149 | 8 |
| M5: `screenAdded` connection to `screensCountChanged()` removed | constructor connects `screenAdded` to `screensCountChanged` | 2152 | 8 |

Before the mutants, the contract passed (exit 0). After the restore, both files were
byte-identical to the backup, and the contract passed again (exit 0).

### moc notify index

From `build/modernization/gcc-debug/app/latte-dock-ng_autogen/YPB6MEUM2W/moc_universalsettings.cpp`
(GCC Debug build):

- Property entry: `QtMocHelpers::PropertyData<QQmlListProperty<QScreen>>(51, 0x80000000 | 52,
  QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 13)`. The last argument is the notify signal
  index, 13.
- Method 13 is `screensChanged()`: `case 13: _t->screensChanged(); break;`, the
  `indexOfMethod(..., &UniversalSettings::screensChanged, 13)` check and the emitter
  `QMetaObject::activate(this, &staticMetaObject, 13, nullptr);`.
- Method 14 is `screensCountChanged()`, unchanged.

### Builds and tests

Toolchain: GCC 16.2.1 (`g++`), Clang 22.1.8 (`clang++`), Qt 6.11.2 on the host, CMake presets
`gcc-debug` and `clang-debug` (`LATTE_STRICT_WARNINGS=ON`), `--parallel 6`. Build trees are
`build/modernization/gcc-debug` and `build/modernization/clang-debug` in this worktree
(git-ignored); the logs are in `build/modernization/logs/`.

| Command (exit status) | Log | Result |
| --- | --- | --- |
| `cmake --preset gcc-debug` (0) | `gcc-configure.log` | 0 CMake warnings; the one matching line is the informational "Strict compiler warnings enabled for 63 first-party compile targets" |
| `cmake --build --preset gcc-debug --parallel 6` (0), before the production change | `gcc-build-baseline.log` | 223 targets built, 0 `warning:` lines, 0 compiler errors. The test source was edited while this build ran; `sourcecontracttest` was rebuilt afterwards (`gcc-sourcecontract-old-header.log`: 0 `warning:` lines) |
| `cmake --build --preset gcc-debug --parallel 6` (0), after the change, incremental | `gcc-build-after-change.log` | 20 translation units recompiled: the 19 sources that include `universalsettings.h` and the generated `mocs_compilation.cpp`; 0 `warning:` lines, 0 compiler errors |
| `ctest --preset gcc-debug --output-on-failure` (0), after the change | `gcc-ctest.log` | 61 of 61 passed |
| `cmake --preset clang-debug` (0) | `clang-configure.log` | 0 CMake warnings; the one matching line is the informational "Strict compiler warnings enabled for 63 first-party compile targets" |
| `cmake --build --preset clang-debug --parallel 6` (0), fresh tree, after the change | `clang-build.log` | 223 targets built, 0 `warning:` lines, 0 compiler errors |
| `ctest --preset clang-debug --output-on-failure` (0) | `clang-ctest.log` | 61 of 61 passed |
| `cmake --build --preset gcc-debug --parallel 6` (0), final tree | `gcc-build-final.log` | 20 translation units recompiled, the same set as the previous incremental build (the final comment edit of `universalsettings.h`); 0 `warning:` lines, 0 compiler errors, 223 targets built |
| `ctest --preset gcc-debug --output-on-failure` (0), final tree | `gcc-ctest-final.log` | 61 of 61 passed; `sourcecontracttest` alone: `Totals: 226 passed, 0 failed` (run directly with `QT_QPA_PLATFORM=offscreen`) |

The GCC incremental build (`gcc-build-after-change.log`) ran before the final comment edit of
`universalsettings.h`, a comment-only change that did not alter any code. The final GCC rebuild
recompiled the dependents of that header. The fresh Clang build was started after that edit,
so it compiled the final text.

### qmllint and metadata diagnostics

The build's qmllint step prints `Warning:` lines. The comparison uses the build logs, because
no `qmllint-deep.sh` before-state was kept.

- GCC baseline (fresh, before the change): 5491 `Warning:` lines. Clang (fresh, after the
  change): 5491. Sorted, the two sets are identical (`diff` exit 0).
- GCC incremental build after the change: 5490. The one missing line is
  `Warning: dialog.h:25: PlasmaQuick::Dialog is used as base type but cannot be found.`, printed
  by "Automatic QML type registration for target lattecoreplugin". That step ran in both fresh
  builds (GCC baseline and Clang) but not in the incremental build, because none of its declared
  inputs changed; no file under `declarativeimports/` includes `universalsettings.h`. The fresh
  Clang build prints the line again, so the two fresh builds match exactly.

## Handoff

- **Batch / slice and selected scope:** R7d, the NOTIFY follow-up of
  [R2c-qproperty-without-notify](R2c-qproperty-without-notify.md): one property, one signal,
  one source contract.
- **Status:** Pending validation. Automated checks complete on both compilers; the desktop
  hot-plug check and the Debian 13 verification are for the user.
- **Source baseline / actual start HEAD / implementation HEAD:** `e18a0f9a7` / `e18a0f9a7` /
  `1c72ce577` (the commit that adds the code and the contract).
- **Branch / existing user edits preserved:** `claude/r7d-screens-notify-b84309`. The worktree
  was clean at the start. Nothing was pushed, `main` was not touched, and the shared ledger
  `docs/architecture-modernization-plan.md` and `docs/qmllint-baseline.json` were not edited.
- **Dependencies and evidence links:** [R2c-qproperty-without-notify](R2c-qproperty-without-notify.md)
  (the reviewed exception this record replaces), [R2c](R2c.md#follow-ups) item 4.
- **Files and externally observable behavior changed:** `app/settings/universalsettings.h`,
  `app/settings/universalsettings.cpp`, `autotests/sourcecontracttest.cpp`,
  `docs/modernization-results/R2c-qproperty-without-notify.md` (appended), this record and
  `docs/modernization-results/R7d-data/`. Observable: the `screens` property has a NOTIFY, and
  the new `screensChanged()` signal is emitted on screen add or remove and on primary-screen
  change. Nothing in the code subscribes to it. The triggers of `screensCountChanged()` and of
  `updateScreens()` are unchanged.
- **State authority / invalidation / lifetime / fallback constraints:** the authoritative
  state is `qGuiApp->screens()`; the property keeps no copy. The NOTIFY is correct only while
  Qt updates the list before it emits (checked on Qt 6.11.2). The removed `QScreen` is still
  alive during `screenRemoved`. No fallback is needed because nothing caches the list.
- **Toolchain, import environment and artifact identity:** GCC 16.2.1, Clang 22.1.8, Qt 6.11.2
  on the host, presets as in the table. The probe used the same Qt headers and libraries.
- **Commands executed with exit status and durable log/artifact locations:** the tables above.
  Raw logs are in `build/modernization/logs/` (git-ignored). The durable copies of the probe,
  the mutation output and the old-header failure are in `R7d-data/`.
- **GCC/Clang Debug/Release warning/error results:** GCC Debug and Clang Debug: 0 compiler
  warnings and 0 compiler errors in the logs. The Release builds were not run.
- **Tests registered / run / passed / skipped / failed and name deltas:** 61 registered in
  CTest. GCC: 61 run, 61 passed; Clang: 61 run, 61 passed. One test function was added to
  `sourcecontracttest` (`screensPropertyNotifiesOnScreenChanges`); no CTest name was added or
  removed. The source-contract executable passes all of its functions with the change: 226
  passed, 0 failed (before the change, the new function failed and the other 225 passed).
- **Production behavior vs copied logic vs source-contract evidence:** production code. The
  signal wiring runs in the application; the list order and lifetime come from the probe, which
  calls Qt's own QPA code. The source contract only reads the source text, so it is an extra
  lock and does not prove that a QML binding re-evaluates.
- **QML diagnostics added/removed and environment compatibility:** no QML file changed. The
  qmllint `Warning:` set is identical between the fresh GCC baseline and the fresh Clang build
  after the change (see the section above). No diagnostic was added.
- **Negative fixtures or controlled mutation evidence:** the old-header failure and mutants
  M1 to M5, in the tables above.
- **Desktop user feedback / log review / clean-quit and coredump evidence:** not run. No dock
  was started against the user's session. The change adds signal wiring with no subscribers.
  A desktop check still worth doing: open Settings, then Behavior, with two screens, plug or
  unplug one, and confirm that the Screen combo updates as before. Make another screen the
  primary one and confirm that the dock behaves as before, since `updateScreens()` still does not
  run on a primary change.
- **Performance paired samples / variability / work counts / tradeoffs:** not applicable.
  Each screen event now also emits `screensChanged()`, which has no subscribers.
- **CI, distribution and Nix checks (passed / failed / unavailable):** not run; CI runs from the
  pushed branch. Debian 13 verification: deferred by the user until the refactor is complete.
  clazy is not installed here, so the gate's `qproperty-without-notify` rule was checked by hand:
  the property has a NOTIFY, the signal is declared, and the exclusion line is gone.
- **Unresolved failures, review assumptions and unavailable checks:** (1) the order of
  `screens()` relative to the signals was checked on Qt 6.11.2 only; (2) the third connection,
  `primaryScreenChanged`, goes beyond the two signals in the brief (see Design decisions);
  (3) no real hot-plug run; (4) clazy and the qmllint-deep baseline were not run here; (5) a
  primary change still does not refresh the Screen combo, which was already the case.
- **Next exact action:** the user runs the desktop check in the "Desktop user feedback" bullet
  above, or accepts the change without it. Then the main session reviews and merges the branch
  and updates the ledger from CI.
- **Commit approval status / push approval status:** committed on `claude/r7d-screens-notify-b84309`
  (`1c72ce577` for the code and contract, and the commit that adds this record). Not pushed.
