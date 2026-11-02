# R2c-qproperty-without-notify: A Property Without NOTIFY or CONSTANT

Status: Complete. CI run 41452548625 passed all 23 jobs and the host desktop smoke passed
([R2c fix batch 3](R2c.md#fix-batch-3)).
Date: 2026-10-29. Branch: `claude/r2c-batch-e-fc8dc7`. Start HEAD: `67180cbda` (the
`R2c-overridden-signal` commit; the batch starts at `95e342f72`).
Implementation commit: the commit that adds this report. Part of [R2c](R2c.md), the
third and last slice of fix batch 3 (batch E).

## Defect

clazy's `qproperty-without-notify` reports a `Q_PROPERTY` with neither `NOTIFY` nor
`CONSTANT`. The one finding is `UniversalSettings::screens` at
`app/settings/universalsettings.h:61`:

```cpp
Q_PROPERTY(QQmlListProperty<QScreen> screens READ screens)
```

The value is not constant. `screens()` returns a `QQmlListProperty` whose
`countScreens()` and `atScreens()` call `qGuiApp->screens()` at each access
(`app/settings/universalsettings.cpp:475-489`). A property without NOTIFY tells QML
that it never changes, so a binding on it would not re-evaluate when a screen is added
or removed.

## Fix

No code change. The property is kept as a reviewed exception: the `Q_PROPERTY` line
has `// clazy:exclude=qproperty-without-notify`, and a `//!` block above it gives the
reason. The facts behind the reason were checked as follows.

- **QML readers.** The only QML reads of `universalSettings.screens` are in
  `shell/package/contents/configuration/pages/BehaviorConfig.qml` (lines 61, 77-78 and
  88-89). All of them are inside `function updateScreens()`. That function is called by
  the `Connections` on `page.universalSettings` with `function onScreensCountChanged()`
  (lines 45-48), by the `Connections` on `page.viewConfig` with `function onShowSignal()`
  (lines 105-108) and by `Component.onCompleted` (line 121). So no QML binding reads
  `screens`; each read happens when a handler runs.
- **C++ readers.** None. `git grep` finds no call of `UniversalSettings::screens()`
  outside the property's own getter. The other `qGuiApp->screens()` calls are in other
  classes.
- **Hotplug notification.** `UniversalSettings` connects `QGuiApplication::screenAdded`
  and `screenRemoved` to its `screensCountChanged()` signal
  (`universalsettings.cpp:57-58`). That signal is the one `BehaviorConfig.qml` listens
  to. It is not the NOTIFY of any property.
- **Effect of a NOTIFY (follow-up, not this slice).** Adding `NOTIFY` would change when
  bindings on `screens` re-evaluate: they would run on each emitted change instead of
  never. No binding exists today, so the imperative `updateScreens()` reads would not
  change. Which signal to use is a design decision. `screensCountChanged()` is emitted on
  screen add and remove, but its order relative to the update of the screen list has not
  been checked, so it must be checked before it is used as a NOTIFY.

## Sites

| Site | Change | What was checked |
| --- | --- | --- |
| `app/settings/universalsettings.h:61` | `//!` block and `// clazy:exclude=qproperty-without-notify` on the `Q_PROPERTY` line | QML and C++ readers, as above |
| `app/settings/universalsettings.cpp:475-489` | unchanged | `screens()`, `countScreens()` and `atScreens()` read `qGuiApp->screens()` on each call |
| `shell/package/contents/configuration/pages/BehaviorConfig.qml` | unchanged | The `updateScreens()` reads and the three callers listed above |
| `autotests/clazy/qproperty-without-notify.cpp` | new fixture: a `Q_PROPERTY(int value READ value)` without NOTIFY or CONSTANT | Rejected by the check under the production flags (the gate's fixture step) |

## Evidence

Container commands are those of [R2c](R2c.md#reproducing-the-environment) with the
private build directory `~/r2c-build-batch-e`, `--jobs 4` and the versions of
[R2c-overloaded-signal](R2c-overloaded-signal.md#evidence): clang 18.1.3, Qt 6.11.1,
clazy 1.17.1.

| Command | Result |
| --- | --- |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 4 --inventory overloaded-signal,overridden-signal,qproperty-without-notify --report build/inventory-slice2.json` on the slice-2 commit `67180cbda` (before this slice) | `overloaded-signal` 0, `overridden-signal` 0, `qproperty-without-notify` 1 in 1 file (`universalsettings.h:61`) |
| `cmake --build --preset clang-debug -j4` after this slice (incremental, `build/build-slice3.log`) | Exit 0; 0 `warning:` lines, 0 clang error lines; the qmllint `Warning:` count is 6832, as before |
| Same inventory after this slice (`build/inventory-slice3.json`) | Exit 0: `qproperty-without-notify` **0 findings in 0 files**; `overloaded-signal` 0 and `overridden-signal` 0 |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 4` (full gate, 46 gated checks) | **Exit 0**: "46 negative fixtures: each is rejected by its own gated check", then "153 production sources: no findings from 46 gated clazy checks" |
| Mutation: the slice-2 version of `universalsettings.h` (`git show 67180cbda:app/settings/universalsettings.h`, the same as `95e342f72`) mounted read-only over the fixed file, then the gate | **Exit 1**: "46 negative fixtures" still rejected, then "1 clazy finding(s) in gated checks": `app/settings/universalsettings.h:61:5: [qproperty-without-notify] Q_PROPERTY should have either NOTIFY or CONSTANT` |
| Host `cmake --preset gcc-debug`, `cmake --build --preset gcc-debug -j4`, `ctest --preset gcc-debug` (incremental from the slice-2 build tree) | **Exit 0** for all three; 0 `warning:` lines, 0 error lines, 0 CMake warnings; `ctest`: 100% passed, 58 of 58 |
| Host `cmake --preset clang-debug`, `cmake --build --preset clang-debug -j4`, `ctest --preset clang-debug` (incremental) | **Exit 0** for all three; 0 `warning:` lines, 0 error lines, 0 CMake warnings; `ctest`: 100% passed, 58 of 58 |
| `git clang-format --style=file --diff HEAD -- app/settings/universalsettings.h autotests/clazy/qproperty-without-notify.cpp` | One hunk, not applied: `public:` in `universalsettings.h`, a line this slice did not change. The changed lines are clean |

## Handoff

- **Batch / slice and selected scope:** R2c fix batch 3, batch E, slice 3
  `R2c-qproperty-without-notify` (1 finding in 1 file).
- **Status:** Complete; CI run 41452548625 passed and the host desktop smoke passed
  ([R2c](R2c.md#fix-batch-3)).
- **Source baseline / start HEAD / implementation HEAD:** `95e342f72` (`main`) /
  `67180cbda` / the commit that adds this report.
- **Branch / existing user edits preserved:** `claude/r2c-batch-e-fc8dc7`; the worktree was
  clean before the edits. Nothing was pushed and `main` was not touched while the slice was
  written.
- **Dependencies and evidence links:** [R2c](R2c.md) (`Gate design`, `Next actions`),
  [R2c-overridden-signal](R2c-overridden-signal.md) (the previous slice on this branch).
- **Files and externally observable behavior changed:** `app/settings/universalsettings.h`
  (a comment and a trailing exclusion comment), `scripts/test-clazy.py` (one name),
  `autotests/clazy/qproperty-without-notify.cpp`. No behavior changes.
- **State authority / invalidation / lifetime / fallback constraints:** the list is read from
  `qGuiApp->screens()` at each access, so it is always current. That is the reason no NOTIFY
  is added in this slice, and the follow-up above is the place to change it.
- **Toolchain, import environment and artifact identity:** as in the Evidence section.
- **Commands executed with exit status and durable log/artifact locations:** the table above.
  Raw logs were in a scratch directory that is deleted at the end; the inventory JSON files
  stay in `~/r2c-build-batch-e/`.
- **GCC/Clang Debug/Release warning/error results:** GCC Debug and Clang Debug: 0 compiler
  warnings, 0 errors, 0 CMake warnings. Release builds were not run.
- **Tests registered / run / passed / skipped / failed and name deltas:** 58 registered, 58 run
  and passed on each compiler; none skipped or failed. No test added, removed or renamed.
- **Production behavior vs copied logic vs source-contract evidence:** production code. No
  source-contract test names `screens` or `UniversalSettings::screens`; the source contracts
  that read `universalsettings.h` check `autostart` only.
- **QML diagnostics added/removed and environment compatibility:** none added. The qmllint
  `Warning:` count in the container build is unchanged (6832).
- **Negative fixtures or controlled mutation evidence:** `autotests/clazy/qproperty-without-notify.cpp`
  is rejected by its check, and the mutation above restores the property without its exclusion.
- **Formatting:** `formatter.sh` was not run; the `clang-format` hunk is described in the
  evidence table.
- **Desktop user feedback / log review / clean-quit and coredump evidence:** the host smoke on
  `6b51b43a6` passed with no warning from Latte's code and no core ([R2c fix batch
  3](R2c.md#fix-batch-3)). The Screens dialog opened from the File menu (check 7). No screen
  was attached or detached, so the refresh of the screen list was not exercised; this slice
  does not change it.
- **Performance paired samples / variability / work counts / tradeoffs:** not applicable.
- **CI, distribution and Nix checks (passed / failed / unavailable):** run 41452548625 at
  `6b51b43a6` passed all 23 jobs after one rerun of `Install verify (mageia)`, which had failed
  on the Mageia Cauldron mirror. `Build (Clang Debug)` ran the 49-check gate and 57 tests, and
  the Debian 13 (Qt 6.8.2) install job and the Nix check passed. See
  [CI validation](R2c.md#ci-validation).
- **Unresolved failures, review assumptions and unavailable checks:** the NOTIFY follow-up is
  open; it needs a decision on the signal and its timing (see Fix). The IDE reports clang errors
  for the edited files because it has no project include flags; the container build is the
  evidence and it has no errors.
- **Next exact action:** none in this slice. Whether `screens` gets a NOTIFY is a separate
  decision (item 4 of the [R2c follow-ups](R2c.md#follow-ups)).
- **Commit approval status / push approval status:** committed on the feature branch
  `claude/r2c-batch-e-fc8dc7`; merged into the integration branch in `4e403af80` by the main
  session. `main` was fast-forwarded to `6b51b43a6` and pushed on 2026-11-02 with the user's
  approval.
