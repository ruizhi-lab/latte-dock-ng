# Latte Dock NG — Codex Project Memory

Compact durable memory for Codex sessions, aligned with the other AI-facing
docs:

- `AGENTS.md` — the always-on rules and the standard debug/retest workflow.
- `CLAUDE.md` — detailed reference: release workflow, test-VM quirks, known
  issues and fixes.
- `docs/development-testing-guide.md` — autotest suite and the Runtime Retest
  Workflow (user-mode Debug retest incl. clean-quit and coredump A/B
  verification).

Update this file when durable project knowledge changes and keep it consistent
with `AGENTS.md` and `CLAUDE.md`.

## Working rules

- Keep all repository text in English, including comments and commit messages.
- No auto-commit / no auto-push: every commit and every push needs explicit
  user approval (two separate approvals). Only git read operations are allowed
  without approval.
- Do not add AI attribution to commits.
- GCC and Clang builds must complete with zero warnings and zero errors.
- Before removing code, trace all consumers and verify build plus runtime
  behavior; check the debug log for new errors/warnings afterwards.
- Release work requires the full autotest suite before versioning:
  `cd build && ctest --output-on-failure` (40 registered targets incl. 170+
  source-contract checks on GCC and Clang).
- Release versioning must update both `CMakeLists.txt` (`VERSION`) and
  `default.nix` (`version`) so NixOS packages stay synchronized. When Nix is
  available on the host, run `nix flake check --print-build-logs` and
  `nix build .#default --no-link --print-build-logs` before tagging; without
  Nix, rely on the autotest suite. Full release steps (tag, GitHub release,
  Gentoo overlay ebuild + Manifest rules) are in `CLAUDE.md`.
- Runtime retesting after risky changes follows the Runtime Retest Workflow in
  `docs/development-testing-guide.md` (user-mode Debug install, drive the
  targeted scenario, verify clean quit via `coredumpctl` baseline and teardown
  log markers, A/B against the pre-fix binary for crash fixes). Never use
  `pkill -f` in a command line that also contains "latte-dock-ng" — it matches
  the shell itself; use `pkill -x latte-dock-ng`.

## Architecture notes

- Project: Latte Dock NG, Qt 6 / KDE Frameworks 6 / Plasma 6, C++17.
- The application is still one large executable assembled by `app/CMakeLists.txt`.
- Large runtime sources include `layoutmanager.cpp`, `containmentinterface.cpp`,
  `view.cpp`, `storage.cpp`, and `AppletItem.qml`.
- User configuration intentionally disables window previews and keeps only the
  normal tooltip. Treat preview-window rendering as an inactive path unless the
  user explicitly enables it for a regression check.
- Use `-j8` for project builds unless a command has a specific resource limit.

## Known behavior and compatibility notes

- Empty appmenu slots with no menu-bearing window are expected HiddenStatus
  behavior, not a regression.
- The KNS compat `Badge` qmldir generation fix is already present; preserve
  compatibility with both old and merged Kirigami module layouts.
- Blur ghosting defaults and gates are intentionally conservative; retain the
  existing source-contract tests.
- The application icon uses the versioned `latte-dock-ng` name; do not fall
  back to the legacy `latte-dock` name, which third-party icon themes (e.g.
  Breeze) shadow with the old artwork.
- Debian Plasma 6.3 lacks a filesystem `org.kde.plasma.plasmoid` QML module.
  Register the module lazily and never use an attached-type stub that breaks
  Behavior resolution.
- Fedora Wayland runs natively even with a minimal environment. Use
  `--log-file` and `QT_LOGGING_RULES='latte*=true'` for useful logs.

## Optimization run archive (perf/memory-cpu-optimization-v2, completed 2026-09)

The user granted one-off auto-commit/auto-push permission **only during that
dedicated run**; the exception has expired and the normal approval rules above
apply again. Commit history of the run lives on the `perf/memory-cpu-optimization-v2`
branch; inspect `git log` there for the exact commit list.

### Baseline measured during the run

- A host baseline of the system release binary measured approximately 553 MiB
  RSS, 373 MiB PSS, 281 MiB anonymous memory, seven threads, and 67 file
  descriptors. The largest mapping was an approximately 175 MiB anonymous
  region. Re-measure after installing a debug build before claiming runtime
  memory improvements.

### Completed steps

- Deferred callback lambdas now use QObject context objects; applet hit
  testing guards null quick items.
- Indicator package import/removal is asynchronous and non-blocking.
- CI covers GCC and Clang and applies a 120-second autotest timeout.
- PulseAudio PID matches expire, idle cleanup timers stop, and stream lookups
  use indexes for exact PID/application keys.
- Headless QML smoke tests avoid synchronous icon-loader and PlasmaCore
  initialization; fake PlasmaCore components cover the required smoke paths.
- Offscreen/minimal environments defer KIconLoader, KDirWatch, KWindowShadow,
  KApplicationTrader, and SVG desktop integration. QML tests use writable
  temporary XDG directories and skip PlasmaQuick dialogs that require a real
  desktop platform.
- High-resolution wheel task activation reuses the task index list for all
  increments in one wheel event.
- Task edit-mode polling is one root timer instead of one repeating timer per
  task delegate.
- Idle task mouse moves return before drag coordinate mapping and reorder work.
- Disabled preview/highlight configurations skip inactive preview-state checks
  during task hover while retaining tooltip and auto-scroll behavior.
- Layout ID allocation builds a `QSet` once instead of repeatedly scanning two
  `QStringList` instances.
- The task preview delegate is now loaded only when `showPreviews` is enabled;
  null guards preserve the disabled-preview path. This still needs real Wayland
  validation for preview toggling and hover behavior.

### Remaining optimization queue

Requires manual Wayland retest:

1. Replace the separate preview `QQuickWindow`/`LatteCore.Dialog` with an
   inline panel-window preview. This is the largest known UI stutter source:
   mapping a Wayland surface can block the Qt Quick render pipeline. Current
   thumbnail preview behavior is intentionally suppressed while Plasma 6
   thumbnail support is unstable. Candidate files are
   `plasmoid/package/contents/ui/main.qml`, `TaskItem.qml`,
   `app/view/parabolic.cpp`, and `app/view/parabolic.h`.
   The preview delegate itself is now lazy-loaded. This remains deferred while
   the user's normal configuration keeps previews disabled.
2. Revisit the bounce-end zoom micro-stutter. A safe fix needs a layout-change
   hook that re-triggers the existing parabolic calculation without re-entering
   the removal-animation relay.
3. Consider lazy creation of the hidden group-dialog compatibility object and
   preview delegate only after measuring startup RSS and verifying task-manager
   backend behavior.

Requires profiling before code changes:

1. Split the monolithic application target into focused libraries or object
   libraries; this is primarily a build/architecture optimization and must be
   benchmarked for incremental build and link costs.
2. Evaluate Release-only LTO/IPO after recording binary size, startup time,
   RSS, and package build time on GCC and Clang.
3. Add model-level caching only after profiling task-model updates; avoid
   caching grouped task indexes without reliable invalidation for rowsInserted,
   rowsRemoved, modelReset, and layoutChanged.
