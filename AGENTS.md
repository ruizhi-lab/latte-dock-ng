# Latte Dock NG — Universal AI Instructions & Knowledge Base

This is the repository's single source of truth for AI coding rules,
architecture principles and workflows. It is tool-neutral and intended for
pi, Claude Code, OpenAI Codex, Cursor and every other AGENTS.md-aware assistant.
Keep cross-component rules here and local design knowledge beside its
implementation; do not create tool-specific `CLAUDE.md` or `CODEX.md` files.

Shared testing/release procedures live in `docs/`: `development-testing-guide.md`
documents the autotest suite and the Runtime Retest Workflow (clean-quit and
coredump A/B verification after runtime fixes).

The current component map and integration boundaries are documented in
`docs/architecture-overview.md`. Read the relevant entries before changing
ownership, QML registration, layout coordination or process boundaries.

The staged architecture and tooling improvement backlog is documented in
`docs/architecture-modernization-plan.md`; execution and handoff requirements
are in `docs/architecture-modernization-implementation.md`. These documents
describe planned work, not completed capabilities or permission to commit/push.

The rules below are always in effect. Consult the later knowledge-base sections
when working on releases, compatibility problems or known runtime behavior.

Local container builds and verification runs should prefer China-hosted Docker
image mirrors and China-hosted package/Nix cache mirrors when available. Keep
these as local Compose build arguments or runtime environment overrides, with
upstream defaults in Dockerfiles so GitHub workflows continue using their
configured upstream sources. Do not change GitHub workflow mirror settings as
part of local mirror maintenance.

## User Rules (always apply)

1. **Main branch commit/push protection** — On `main`, never commit or push
   without explicit user approval. Commit and push are two SEPARATE approvals:
   after committing, ask "push?". "commit" alone never implies "push". On
   every non-`main` branch, an explicitly requested implementation may be
   committed and pushed automatically after the required checks pass. Do not
   silently switch branches, rewrite published history, or push a different
   branch than the one being worked on. A user request to commit or push on
   `main` authorizes only that requested operation; release tags and other
   remote mutations still require their own explicit authorization.
2. **English only** — All codebase content in English: commit messages,
   release notes/GitHub descriptions, code comments, documentation.
3. **No AI attribution** — Commit messages must NOT include `Co-Authored-By`,
   `Signed-off-by`, or similar AI attribution lines.
4. **Zero warnings on GCC and Clang** — Every build (debug, development,
   pre-commit, release) must compile with zero warnings and zero errors on
   BOTH compilers. Warning flags come from KDE's KDECompilerSettings module;
   when adding any warning suppression, document why.
5. **No regressions when removing dead code** — Trace all consumers before
   removal; keep working features intact even if they share code; verify
   compilation AND runtime behavior afterwards; check the debug log for new
   errors/warnings.
6. **Release requires autotest** — Before every release run
   `cd build && ctest --output-on-failure` (42 registered autotest targets
   incl. 170+ source-contract checks on GCC and Clang; fragile areas: digital
   clock, systray, volume, appmenu, clipboard, separator/spacer, middle-click
   close, auto-pin on drag, scroll minimize).
7. **Document non-obvious design constraints** — Add concise English code
   comments for runtime workarounds and subtle ownership, lifecycle, timing or
   cross-component state logic. Explain why the code exists, identify the
   authoritative state source and record the failure mode that would return if
   the constraint were removed. This context is required for both human and AI
   maintainers; do not narrate self-evident code.
8. **Keep local design knowledge beside the implementation** — Skills, plans
   and review notes may provide process or broad design guidance, but they are
   not a substitute for durable component knowledge. When a feature, workaround
   or invariant is discovered or changed, put its concise rationale next to the
   owning C++/QML/Python/Shell implementation and add a focused test or contract
   when practical. The comment should state the trigger, the authoritative
   state, the ownership/lifecycle or timing constraint, and the failure mode
   prevented. Do not paste a whole skill or plan into source comments, and do
   not leave essential behavior explainable only by a skill file or chat history.
   Keep AGENTS.md as the cross-component architecture and workflow charter;
   keep component-specific details beside the code they govern. Update the
   nearby comment and its test when behavior, supported versions or the
   authoritative state changes.

## Development Debug & Retest Workflow

When testing changes to latte-dock-ng, follow this exact workflow:

1. **User-mode install modified code**
   ```bash
   cd /data/projects/latte-dock-ng && bash install.sh --user Debug >/tmp/latte-install-user.log 2>&1; tail -n 80 /tmp/latte-install-user.log
   ```

2. **Kill old latte-dock-ng process** (CAUTION: never use `pkill -f` in a
   command line that also contains "latte-dock-ng" elsewhere — it matches the
   shell's own command line and kills the shell; prefer `pkill -x latte-dock-ng`)
   ```bash
   pkill -x latte-dock-ng || true
   ```

3. **Remove old log file**
   ```bash
   rm -f /tmp/latte-ng.log
   ```

4. **Source user-mode environment variables**
   ```bash
   source ~/.config/latte-dock-ng/dev-env.sh
   ```

5. **Launch the USER-MODE Debug binary (~/.local/bin), NOT the system
   /usr/bin binary** (user explicitly corrected this; the dev-env.sh sourcing
   enables locally-built QML module overrides). Must survive shell timeout —
   use script + nohup in a separate session. The explicit `setsid` prevents
   command-runner session cleanup from terminating an otherwise nohup-protected
   process:
   ```bash
   cat > /tmp/launch-latte.sh << 'SCRIPT'
   #!/bin/bash
   source ~/.config/latte-dock-ng/dev-env.sh
   exec ~/.local/bin/latte-dock-ng --replace --debug > /tmp/latte-ng.log 2>&1
   SCRIPT
   chmod +x /tmp/launch-latte.sh
   setsid -f nohup /tmp/launch-latte.sh > /dev/null 2>&1
   sleep 5
   ps aux | grep latte-dock-ng | grep -v grep || echo "DOCK FAILED TO START"
   ```

   Note: `bash install.sh --user Debug` already installs all plasmoid QML to
   ~/.local/share/plasma/plasmoids/ — a manual `cp` overlay of QML files is
   usually not needed. (If DESTDIR causes prefix duplication like
   ~/.local/home/user/.local/..., install directly with
   `cmake --install build --prefix ~/.local`.)

6. **Wait for user retest feedback**, then automatically analyze debug log for warnings/errors

7. **Analyze debug log** (`/tmp/latte-ng.log`) for warning/error entries. If found, record them as issues that need fixing.

8. **Verify clean-quit scenarios (logout/shutdown/restart fixes)** — follow
   the Runtime Retest Workflow in `docs/development-testing-guide.md`: record a
   `coredumpctl` baseline, drive the quit scenario, then confirm no new
   latte-dock-ng core appears, the log shows the expected teardown markers and
   no Fatal/ASSERT, and run an A/B check against the pre-fix binary for crash
   fixes.

9. **Commit/push according to branch policy** — On `main`, stop after
   validation and request the separate commit and push approvals described
   above. On another branch, commit and push the requested implementation
   after validation without an additional approval, unless the user explicitly
   asks for a draft-only result or the action would rewrite published history.

## Quick references

- **Debug logging**: latte discards ALL output unless launched with `-d`
  (`--debug`). In minimal or VM test environments, also pass
  `--log-file /tmp/latte-ng.log` plus `QT_LOGGING_RULES='latte*=true'` for
  latte's own qCDebug.
- **Runtime retest & clean-quit/coredump verification**: see
  `docs/development-testing-guide.md` (Runtime Retest Workflow) — canonical
  steps for crash fixes are kept there, not duplicated here.
- **GitHub proxy**: if git push/ls-remote hangs, retry through the local HTTP
  proxy configured in the shell environment (machine-local; exact address is
  not committed to the repo).
- **Wayland popup positioning (hard rule)**: never position a
  `LatteCore.Dialog` / plasma popup with a raw `QWindow::setPosition()`. For a
  Plasma shell surface the compositor ignores it and keeps the position the
  popup was first mapped at, while `x()` still reports the requested value.
  Always route through `PlasmaQuick::Dialog::adjustGeometry()`
  (`declarativeimports/core/dialog.cpp`). A stale `x()` in a diagnostic means
  the compositor ignored the request — not that the position math is wrong.

## Release Workflow (only on explicit request)

Follow [`docs/release-workflow.md`](docs/release-workflow.md) for release
preparation, package publication, repository availability and current
validation status. Do not duplicate release-specific platform status here.
The main-branch commit/push rules above still apply; release tags require their
own explicit authorization and are not implied by approval to commit or push.

### Gentoo overlay

- Work in the local checkout of `ruizhi-lab/gentoo-overlay`, branch `main`, at
  `kde-misc/latte-dock-ng/`.
- Copy the previous ebuild, ensure `SRC_URI` uses `v${PV}`, remove the obsolete
  ebuild, and regenerate the Manifest with a temporary writable `DISTDIR`.
- Generate the Manifest only after the release tag is final. Moving a tag
  changes GitHub tarballs; delete the stale Manifest and regenerate it or
  emerge will report a filesize mismatch. Never use sudo for this workflow.

## Architecture & Compatibility Notes

- **Architecture charter**: `Corona` coordinates application services; `View`
  composes per-dock behavior; QML composes presentation and interaction.
  Layout coordination (`app/layouts/`) and containment item arrangement
  (`containment/plugin/layoutmanager.cpp`) are different responsibilities.
  Keep deterministic policy separate from UI/platform adapters where practical.
  Directory names do not imply independent libraries or a strict dependency
  chain: consult `docs/architecture-overview.md` and actual CMake targets.
- **State and lifetime**: designate one authoritative source for each state;
  derived caches need explicit invalidation. Make ownership and asynchronous
  cancellation/stale-result rules visible at component boundaries. Preserve
  QObject thread affinity and the established shutdown order; do not infer
  destruction safety from parent ownership alone.
- **Integration contracts**: QML URIs/type names, D-Bus interfaces, persisted
  configuration and plugin installation paths are compatibility surfaces.
  Trace consumers before changing them. Prefer API capability probes for build
  compatibility and runtime capability checks for compositor services; use
  version checks for behavior-specific workarounds with documented evidence.
  Preserve the preview helper's process isolation and failure fallback.
- The application is one large executable assembled by `app/CMakeLists.txt`.
  Large runtime sources include `layoutmanager.cpp`, `containmentinterface.cpp`,
  `view.cpp`, `storage.cpp`, and `AppletItem.qml`.
- Use `-j8` for project builds unless a command has a specific resource limit.
- User configuration normally disables window previews and retains only title
  tooltips; the task hover-action setting is the authoritative preview gate.
- The application icon is `latte-dock-ng`; never fall back to the legacy
  `latte-dock` name because third-party themes may supply old artwork for it.
- Debian Plasma 6.3 lacks a filesystem `org.kde.plasma.plasmoid` QML module.
  Register it lazily; an attached-type stub breaks Qt 6.8 Behavior resolution.
- On minimal Fedora Wayland systems, use `--log-file` and
  `QT_LOGGING_RULES='latte*=true'` to capture Latte's own logs.
- Keep detailed component invariants next to their implementation. In
  particular, `TaskItem.qml` documents the task-tooltip ownership, hover and
  enable-state contract; its source-contract autotests protect that design.
- Treat `AGENTS.md` as the high-level index, architecture charter and policy
  document. Put localized feature contracts, failure modes and workaround
  rationale in concise English comments beside the relevant implementation;
  future maintainers and AI tools must be able to understand the behavior from
  the code and its nearby documentation without recovering hidden skill or chat
  context.

## Coding Standards

Apply these rules to new and changed code. Existing patterns are context, not
permission to extend a known defect; keep unrelated cleanup out of the patch.

### C++ and Qt

- Use C++20 and the dependency floors in `CMakeLists.txt`. Follow nearby naming
  and `.clang-format`; run `formatter.sh` only on touched C++ files and inspect
  the diff. Its `Standard: Latest` setting does not raise the language standard.
- Prefer const-correct interfaces, initialized members, `override`, and scoped
  resource ownership. Use `auto` when it preserves readability, explicit types
  when units/conversions matter, and `std::as_const` for read-only iteration of
  mutable Qt containers when appropriate. Use `QStringLiteral` for fixed QString
  values and KDE translation APIs for user-visible text.
- Use QObject parent ownership or explicit RAII ownership without competing
  owners. Guard retained non-owning QObject references with `QPointer` when the
  referenced object can disappear independently; ordinary pointers/references
  are valid when lifetime is guaranteed. `QPointer` is not thread synchronization
  and does not prove an object is safe during partially completed destruction.
- Prefer typed signal/slot connections and context-bound lambdas. Do not capture
  short-lived locals by reference in deferred callbacks; guard independently
  owned captures and reject stale results after state changes. Disconnecting a
  producer alone is not a substitute for validating already queued work.
- Keep UI objects on their owning thread. Use bounded asynchronous work for
  potentially expensive I/O or computation; do not move QObjects to workers
  without an explicit ownership/thread design. Avoid blocking waits or nested
  event loops in interactive paths. Destructors must not depend on starting new
  asynchronous work that requires a shutting-down event loop to finish.
- For mutable C++ properties used in QML bindings, provide correct change
  notification (`NOTIFY` or supported bindable semantics); use `CONSTANT` only
  for values invariant over the object's lifetime. Notify on actual changes.
- Validate external input, report actionable failures through the appropriate
  logging category, and preserve documented fallbacks. New IPC needs bounded
  input, failure handling and timeout/cancellation policy appropriate to its
  transport; do not add silent success paths for failed operations.

### QML

- Declare typed properties and explicit component inputs/signals where the
  interface is known. Qualify cross-object access with an id or explicit input;
  avoid adding hidden context dependencies or mutating another component's
  private state. Preserve necessary dynamic Plasma interfaces with local rationale.
- Keep visual state and animation in QML; centralize shared decisions in a
  clearly owned component or C++ policy object. Do not relocate every small
  presentation decision to C++ merely for uniformity.
- Prefer bindings for ongoing state synchronization. Imperative writes must
  account for binding removal. `Component.onCompleted` initializes a component;
  it does not replace subscriptions to later state changes. Give timers,
  animations and deferred handlers a defined deactivation/teardown path.
- Preserve module URI, type registration and import/install contracts. Verify
  the generated `qmldir`, plugin and referenced `.qmltypes` metadata; filenames
  differ by target and are not universally `plugins.qmltypes`. Test host-dependent
  plugins in their intended host as well as checking tooling metadata.

### Build, scripts and validation

- For new targets use target-scoped sources, includes, definitions and link
  dependencies. Avoid expanding global flags or include paths. Declare generated
  output dependencies and preserve PIC, AUTOMOC and host symbol requirements.
  Prefer linking shared production logic into tests when a suitable target
  exists; direct source compilation remains valid for focused isolated tests.
- Use capability probes and small adapters for compatibility. Document any
  version-specific exception and its removal condition next to implementation.
  When adding dependencies, inspect and update affected CI, Docker, Nix and
  packaging definitions; distinguish build dependencies from runtime imports.
- Quote shell paths and variables, propagate failures (including pipelines),
  and keep temporary/test data isolated from user configuration. Document
  intentionally tolerated failures. Do not hard-code developer-specific paths
  in reusable tooling.
- Test production behavior rather than duplicating its algorithm. Cover the
  relevant boundary, failure and destruction cases for risky changes. Use
  isolated D-Bus sessions and bounded event-driven waits. Source contracts are
  useful regression locks but do not prove live QML/Wayland behavior.
- Run affected GCC/Clang builds, tests and QML checks for code/build changes;
  follow the canonical desktop retest for runtime changes. Documentation-only
  edits need accuracy/link/whitespace review, not a rebuild or desktop restart.
  Report skipped or unavailable checks explicitly; do not call them passed.
