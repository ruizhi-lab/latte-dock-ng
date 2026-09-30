# P3b: Remove the duplicate task edit-state poll

Status: Pending validation. Task gesture guards now consume the task root's
combined, notifying `inEditMode` property, and its always-running 200 ms mirror
poll has been removed. The outer containment's compatibility poll remains in
place to detect Plasma's missed `userConfiguring` exit notification. Local
source-contract/QML smoke tests and syntax lint pass, and the user reports the
desktop edit/task interactions passed. Full deep-QML diagnostic comparison
remains pending a matching qmllint version.

Recorded main baseline: `e3ef1ddf001aa032cffa62db21cfd97196174746`.
Implementation branch: `codex/modernization-m0-baseline`.

## Change

The task root's `inEditMode` is the authoritative combined state of the Latte
bridge, the task plasmoid's `userConfiguring` property and the propagated
`containmentEditing` value. The outer containment initializes and updates that
value directly in `onEditModeChanged`; its own 200 ms poll remains active while
in edit mode because Plasma may fail to notify when `userConfiguring` becomes
false. Changes to the task root's declared QML property notify normally, so a
second root timer that merely copied it into `containmentEditingPolled` was
redundant. `TaskMouseArea` now reads `root.inEditMode`, aligning menu, click and
wheel guards with the same state used by `TaskItem` and preview/highlight gates.

With at least one task, this removes one timer callback every 200 ms while the
task plasmoid is idle (five callbacks per second). This is a bounded reduction
in scheduled work; no process CPU or memory saving is claimed without a valid
runtime A/B measurement.

## Validation

- For `gcc-debug`, `clang-debug`, `gcc-release` and `clang-release`:
  `cmake --build build/modernization/<config> --target sourcecontracttest qmlsmoketest -j8` — passed without compiler warnings or errors.
- For all four configurations:
  `ctest --test-dir build/modernization/<config> -R '^(sourcecontracttest|qmlsmoketest)$' --output-on-failure` — passed (2/2).
- `bash scripts/qmllint.sh plasmoid/package/contents/ui/main.qml plasmoid/package/contents/ui/task/TaskMouseArea.qml` — passed.
- `bash scripts/qmllint-deep.sh build/modernization/gcc-debug` and the corresponding Clang Debug command — did not pass the baseline gate because the installed qmllint is 6.11.2 while the reviewed baseline fingerprints 6.11.1. The current run produced 724 diagnostic identities; a direct identity/count comparison of the two changed QML files found no additions or removals (26 identities in each baseline/current pair). The full-tree result remains pending a comparable qmllint environment.
- `git diff --check` — passed.
- Fedora 44 final staging: the committed branch snapshot at `c03ffa7ec` was built and installed with `bash install.sh --user Debug`; the user-mode process is running from `/home/fedora/.local/bin/latte-dock-ng` with the captured Plasma session environment and developer QML overrides.
- Initial Fedora startup log scan: no Warning, Error, Fatal or ASSERT entries. The log contains informational shutdown/startup messages, a missing KActivities service notice and the existing containment-action metadata notice. The final Fedora/Debian Dock shutdown and fresh startup logs on 2026-09-30 also contained no Warning, Error, Fatal or ASSERT entries.
- Debian 13.7 / Plasma 6.3.6 build/install completed after installing the missing `qt6-declarative-private-dev` build dependency from the configured USTC mirror. The user-mode Debug dock is running with the session environment restored. Startup scan has no Warning, Error, Fatal, ASSERT or missing-icon entries; the Debian Trash icon resource URL compatibility fix is recorded in [debian13-runtime.md](debian13-runtime.md).
- On 2026-09-30, the user reported that the remaining task hover, edit-state and interaction checks passed. Per-OS/mode detail was not supplied in that aggregate result.
- The user accepts two-output behavior by code review because a second display is unavailable; it is not recorded as a runtime test.

## Pending runtime acceptance

Test edit entry/exit, startup with the Latte bridge absent, bridge reattachment,
late task arrival and removal, then verify right-click menus, click/modifier
actions, dragging, wheel behavior and tooltips obey edit mode. In the same final
task-icon pass, set each hover choice independently and verify its own behavior:

- `PreviewWindows`: show window previews without enabling window highlighting.
- `HighlightWindows`: highlight matching windows without showing previews.
- `PreviewAndHighlightWindows`: show previews and highlight matching windows.

Also verify tooltips, grouped-window actions, hide/dodge behavior and preview-
helper failure fallback in each applicable mode. The user reports this matrix
passed; per-OS and per-mode details were not supplied.

## Handoff

The user reports the edit-state and three-hover-choice matrix passed. See the
aggregate [manual retest checklist](manual-retest-checklist.md). The comparable
deep-QML diagnostic gate remains unavailable because the installed qmllint
version differs from the reviewed baseline; retain that limitation above.
