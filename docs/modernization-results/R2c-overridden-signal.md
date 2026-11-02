# R2c-overridden-signal: Declarations That Override a Base-Class Signal

Status: Complete. CI run 41452548625 passed all 23 jobs and the host desktop smoke passed
([R2c fix batch 3](R2c.md#fix-batch-3)).
Date: 2026-10-29. Branch: `claude/r2c-batch-e-fc8dc7`. Start HEAD: `b30ae731a` (the
`R2c-overloaded-signal` commit; the batch starts at `95e342f72`).
Implementation commit: the commit that adds this report. Part of [R2c](R2c.md), the
second slice of fix batch 3 (batch E), after [R2c-overloaded-signal](R2c-overloaded-signal.md).

## Defect

clazy's `overridden-signal` reports a declaration in a derived class that has the name
of a signal of its base class. The two findings (line numbers of the batch start,
`95e342f72`):

| Header:line | Declaration | Base declaration | clazy message | Disposition |
| --- | --- | --- | --- | --- |
| `app/view/view.h:331` | `View::screenGeometryChanged()`, a signal | `PlasmaQuick::View::screenGeometryChanged()`, `app/plasma/quick/view.h:112` | Overriding signal with signal | reviewed exception |
| `declarativeimports/core/iconitem.h:168` | `IconItem::enabledChanged()`, a private slot | `QQuickItem::enabledChanged()`, a signal | Overriding signal with non-signal | renamed to `onEnabledChanged()` |

For the slot, the shared name is the risk: `connect(this, &QQuickItem::enabledChanged,
this, &IconItem::enabledChanged)` reads like a signal connected to itself, and the
name means one thing in the base class and another in the derived one.

## Fix

- **`IconItem::enabledChanged()` renamed `onEnabledChanged()`** (private slot). Changed
  sites: `iconitem.h:170-171` (a `//!` line that says why the name is not the base name,
  then the declaration), `iconitem.cpp:110-111` (the connect keeps `QQuickItem::enabledChanged`
  as its signal) and `iconitem.cpp:477` (the definition; the body is unchanged).
- **`View::screenGeometryChanged()` kept** as a reviewed exception: the declaration at
  `view.h:339` (start HEAD `331`) has `// clazy:exclude=overridden-signal` and a `//!`
  block with the reason below.

No QML, no other `.cpp` file and no test changed. The changed files are the owned
production files of this slice, one `GATED_CHECKS` line, the new fixture
`autotests/clazy/overridden-signal.cpp` and this report.

## Sites

Line numbers are those of the batch start unless the header says otherwise.

| Site | Change | What was checked |
| --- | --- | --- |
| `iconitem.h:168` (now `171`) | private slot renamed `onEnabledChanged()` | `git grep` for `enabledChanged` finds no other reference to the `IconItem` slot. The QML `function onEnabledChanged()` at `shell/package/contents/configuration/LatteDockConfiguration.qml:617` is inside a `Connections` whose target is `actionsComboBtn.comboBox` (lines 597-598), so it is not an `IconItem`. Private slots are not visible to QML (the probe in [R2c-const-signal-or-slot](R2c-const-signal-or-slot.md#proof-that-only-the-kind-of-ten-methods-changes-in-the-meta-object) showed the QML engine does not expose private meta-methods) |
| `iconitem.cpp:110-111` | connect target renamed to `&IconItem::onEnabledChanged` | The signal stays `QQuickItem::enabledChanged`, so the slot runs on the same emission as before |
| `iconitem.cpp:477` | definition renamed | Body unchanged (`schedulePixmapUpdate()`) |
| `view.h:331` (now `339`) | exclusion and `//!` block, declaration kept | See the facts below |

### Facts behind the `View::screenGeometryChanged` exception

- **Declarations.** `PlasmaQuick::View` declares the signal (`app/plasma/quick/view.h:112`)
  and the NOTIFY of `Q_PROPERTY(QRectF screenGeometry ...)` (`view.h:35`). `View`
  redeclares the signal (`view.h:339`) and its own `Q_PROPERTY(QRect screenGeometry ...
  NOTIFY screenGeometryChanged)` (`view.h:127`).
- **Emitters.** The base signal is never emitted in the tree: `git grep` finds no
  emit of it, and `app/plasma/quick/` only declares it. The `View` signal has no
  `Q_EMIT` either. It is emitted by the forward `connect(m_positioner,
  &ViewPart::Positioner::screenGeometryChanged, this, &View::screenGeometryChanged)` in
  `View::init()` (`app/view/view.cpp:468`). The Positioner signal is emitted by
  `Positioner::setScreenToFollow()` (`positioner.cpp:400`) and by the
  `QScreen::geometryChanged` connection (`positioner.cpp:395`).
- **Consumers of the `View` signal.** C++: `app/view/helpers/subwindow.cpp:59` and
  `app/wm/tracker/windowstracker.cpp:176` connect `&Latte::View::screenGeometryChanged`.
  QML: bindings that read `latteView.screenGeometry` or `view.screenGeometry`, for example
  `containment/package/contents/ui/abilities/MyView.qml:42`,
  `containment/package/contents/ui/layouts/loaders/Tasks.qml:19`,
  `containment/package/contents/ui/applet/AppletItem.qml:1683` and
  `declarativeimports/abilities/client/MyView.qml:43`. No QML handler is named
  `onScreenGeometryChanged` (`git grep`).
- **Effect of removing the redeclaration.** `&View::screenGeometryChanged` in `view.cpp:468`
  would then name the inherited base signal, so the Positioner forward would emit the
  base class's signal in place of the `View` one. The task asked for "too". The precise
  statement is "instead": there would be no `View` signal to emit in addition. The
  pointer-to-member connects in `subwindow.cpp` and `windowstracker.cpp` and the NOTIFY
  of `screenGeometry` would also name the base signal. The `View` redeclaration is
  therefore a design choice, not a compile requirement: a probe in the container
  (`moc` of Qt 6.11.1 on a base signal named by a derived `Q_PROPERTY ... NOTIFY`) exited 0.

## Observations for the main session (not changed here)

- The QML `onEnabledChanged` handler in `LatteDockConfiguration.qml:617` is on a
  `ComboBox`, not on an `IconItem`. Renaming the private slot leaves it alone.
- The start HEAD `View` and `PlasmaQuick::View` both declare `screenGeometry` properties
  with different types (`QRect` and `QRectF`); this slice does not change either.

## Evidence

Container commands are those of [R2c](R2c.md#reproducing-the-environment) with the
private build directory `~/r2c-build-batch-e`, `--jobs 4` and the same versions as
[R2c-overloaded-signal](R2c-overloaded-signal.md#evidence): clang 18.1.3, Qt 6.11.1,
clazy 1.17.1.

| Command | Result |
| --- | --- |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 4 --inventory overloaded-signal,overridden-signal,qproperty-without-notify --report build/inventory-slice1.json` on the slice-1 commit `b30ae731a` (before this slice) | `overloaded-signal` 0, `overridden-signal` 2 in 2 files (slice-1 numbering: `view.h:335`, `iconitem.h:170`), `qproperty-without-notify` 1 |
| `cmake --build --preset clang-debug -j4` after this slice (incremental, `build/build-slice2.log`) | Exit 0; 0 `warning:` lines, 0 clang error lines; the qmllint `Warning:` count is 6832, as before |
| Same inventory after this slice (`build/inventory-slice2.json`) | Exit 0: `overridden-signal` **0 findings in 0 files**; `overloaded-signal` 0; `qproperty-without-notify` 1 remains for the last slice |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 4` (full gate, 45 gated checks) | **Exit 0**: "45 negative fixtures: each is rejected by its own gated check", then "153 production sources: no findings from 45 gated clazy checks" |
| Mutation: the slice-1 versions of `view.h`, `iconitem.h` and `iconitem.cpp` (`git show b30ae731a:<path>`) mounted read-only over the fixed files, then the gate | **Exit 1**: "45 negative fixtures" still rejected, then "2 clazy finding(s) in gated checks" (slice-1 numbering): `app/view/view.h:335:5: [overridden-signal] Overriding signal with signal: Latte::View::screenGeometryChanged` and `declarativeimports/core/iconitem.h:170:5: [overridden-signal] Overriding signal with non-signal: Latte::IconItem::enabledChanged` |
| Host `cmake --preset gcc-debug`, `cmake --build --preset gcc-debug -j4`, `ctest --preset gcc-debug` (incremental from the slice-1 build tree) | **Exit 0** for all three; 0 `warning:` lines, 0 error lines, 0 CMake warnings; `ctest`: 100% passed, 58 of 58 |
| Host `cmake --preset clang-debug`, `cmake --build --preset clang-debug -j4`, `ctest --preset clang-debug` (incremental) | **Exit 0** for all three; 0 `warning:` lines, 0 error lines, 0 CMake warnings; `ctest`: 100% passed, 58 of 58 |
| `git clang-format --style=file --diff HEAD` on the changed C++ files | Three hunks, none applied. Two are in `iconitem.cpp`: the two-line `connect` is joined onto one line, and the return type of the renamed definition is split onto its own line. The start version of `iconitem.cpp` gets the same two changes from clang-format (checked on `95e342f72`), so the file's layout is kept. The third is `private:` in `iconitem.h`, a line this slice did not change |

## Handoff

- **Batch / slice and selected scope:** R2c fix batch 3, batch E, slice 2
  `R2c-overridden-signal` (2 findings in 2 files).
- **Status:** Complete; CI run 41452548625 passed and the host desktop smoke passed
  ([R2c](R2c.md#fix-batch-3)).
- **Source baseline / start HEAD / implementation HEAD:** `95e342f72` (`main`) / `b30ae731a`
  / the commit that adds this report.
- **Branch / existing user edits preserved:** `claude/r2c-batch-e-fc8dc7`; the worktree
  was clean before the edits. Nothing was pushed and `main` was not touched while the slice
  was written.
- **Dependencies and evidence links:** [R2c](R2c.md) (`Gate design`, `Next actions`),
  [R2c-overloaded-signal](R2c-overloaded-signal.md) (the previous slice on this branch).
- **Files and externally observable behavior changed:** `declarativeimports/core/iconitem.h`,
  `declarativeimports/core/iconitem.cpp`, `app/view/view.h`, `scripts/test-clazy.py` (one
  name), `autotests/clazy/overridden-signal.cpp`. No behavior changes: the slot is renamed
  and connected to the same signal, and the `View` signal is kept as it was.
- **State authority / invalidation / lifetime / fallback constraints:** none new. The
  slot runs on `QQuickItem::enabledChanged` as before and only calls
  `schedulePixmapUpdate()`.
- **Toolchain, import environment and artifact identity:** as in the Evidence section.
- **Commands executed with exit status and durable log/artifact locations:** the table
  above. Raw logs were in a scratch directory that is deleted at the end; the inventory
  JSON files stay in `~/r2c-build-batch-e/`.
- **GCC/Clang Debug/Release warning/error results:** GCC Debug and Clang Debug: 0 compiler
  warnings, 0 errors, 0 CMake warnings. Release builds were not run.
- **Tests registered / run / passed / skipped / failed and name deltas:** 58 registered,
  58 run and passed on each compiler; none skipped or failed. No test added, removed or
  renamed.
- **Production behavior vs copied logic vs source-contract evidence:** production code. No
  source-contract test names `enabledChanged` of `IconItem` or `screenGeometryChanged`
  (`git grep` in `autotests/`).
- **QML diagnostics added/removed and environment compatibility:** none added. The qmllint
  `Warning:` count in the container build is unchanged (6832).
- **Negative fixtures or controlled mutation evidence:** `autotests/clazy/overridden-signal.cpp`
  is rejected by its check, and the mutation above removes the two exceptions.
- **Formatting:** `formatter.sh` was not run; the `clang-format` hunks are described in the
  evidence table.
- **Desktop user feedback / log review / clean-quit and coredump evidence:** the host smoke on
  `6b51b43a6` passed with no warning from Latte's code and no core ([R2c fix batch
  3](R2c.md#fix-batch-3)). The `View` exception depends on QML handlers that resolve the
  signals by name. Both docks were placed and sized as before at startup (check 1) and after
  the layout switch (check 6), and the dock settings window opened in edit mode (21:39:08);
  none of those bindings broke.
- **Performance paired samples / variability / work counts / tradeoffs:** not applicable.
- **CI, distribution and Nix checks (passed / failed / unavailable):** run 41452548625 at
  `6b51b43a6` passed all 23 jobs after one rerun of `Install verify (mageia)`, which had failed
  on the Mageia Cauldron mirror. `Build (Clang Debug)` ran the 49-check gate and 57 tests, and
  the Debian 13 (Qt 6.8.2) install job and the Nix check passed. See
  [CI validation](R2c.md#ci-validation).
- **Unresolved failures, review assumptions and unavailable checks:** the IDE reports
  clang errors for the edited files because it has no project include flags; the container
  build is the evidence and it has no errors. The `View` exception rests on the facts above,
  not on a runtime check of the QML bindings.
- **Next exact action:** none.
- **Commit approval status / push approval status:** committed on the feature branch
  `claude/r2c-batch-e-fc8dc7`; merged into the integration branch in `4e403af80` by the main
  session. `main` was fast-forwarded to `6b51b43a6` and pushed on 2026-11-02 with the user's
  approval.
