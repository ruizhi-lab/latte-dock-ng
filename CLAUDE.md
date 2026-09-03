# Latte Dock NG — Project Memory

This file consolidates the project memory previously maintained by Claude Code
(the legacy Claude Code memory directory, 20 files).
pi loads `AGENTS.md` / `CLAUDE.md` as context files automatically.

Memory role split (kept consistent across the two AI docs): `AGENTS.md`
carries the always-on rules and the standard debug/re-test workflow and is
auto-loaded by pi, OpenAI Codex and other AGENTS.md-aware tools; this file
(`CLAUDE.md`) is the detailed reference for the release workflow, known
issues, and cross-distro compatibility notes. Runtime clean-quit/coredump
verification is documented in `docs/development-testing-guide.md` (Runtime
Retest Workflow). There is deliberately no separate CODEX.md — Codex reads
AGENTS.md, so one compact auto-loaded file keeps every agent on the same
rules. See `AGENTS.md` for the standard debug/re-test workflow (user-mode
install, kill, launch, log analysis, no auto-commit).

## Rules (explicit user feedback)

### No auto-commit / no auto-push
- Never commit or push git changes without **explicit** user approval.
- Commit and push are **two separate approvals**: after committing, ask "push?".
  "commit" alone never implies "push".
- Only use git read operations (diff, log, status) unless explicitly asked.

### English only
All codebase content must be in English: commit messages, release notes /
GitHub release descriptions, code comments, and documentation. Rewrite any
non-English text before committing or releasing.

### No AI attribution in commits
Commit messages must NOT include AI attribution lines (`Co-Authored-By`,
`Signed-off-by`, etc.). Keep messages clean: conventional-commit prefix +
description only.

### Zero warnings on both GCC and Clang
Every build — debug, development, pre-commit, release — must compile with
zero warnings and zero errors on both GCC and Clang. No exceptions.
Warning flags come from KDE's KDECompilerSettings module; no project-local
suppression file exists. If a suppression is ever needed, document why in
the commit message.

### No regressions when removing dead code
Before removing dead code: trace all consumers, keep working features intact
even if they share code, verify compilation AND runtime behavior afterwards,
and check the debug log for new errors/warnings. A prior cleanup removed 1463
lines of dead code while keeping schemeFile, icon, popUpMargin, isShownInMenu,
and textColor (edit mode) working.

### Release requires autotest (ctest)
Before every release, run `cd build && ctest --output-on-failure` — the full
autotest suite (40 registered targets incl. 170+ source-contract checks on GCC
and Clang) covers fragile special cases (digital clock, systray, volume,
appmenu, clipboard, separator/spacer, middle-click close, auto-pin on drag,
scroll minimize). Fix failures before version bump and tag.

## Reference: Release workflow (only on explicit request)

### 1. latte-dock-ng repo
1. Bump `set(VERSION X.Y.Z)` in `CMakeLists.txt` and `version = "X.Y.Z"` in
   `default.nix` so the native and NixOS packages use the same version.
2. Run both NixOS checks before tagging: `nix flake check --print-build-logs`
   and `nix build .#default --no-link --print-build-logs`.
3. Commit: `release: bump version to X.Y.Z` (include pending changes + CHANGELOG.md section).
4. Annotated tag with `v` prefix: `git tag -a vX.Y.Z -m "..."`.
5. `git push && git push --tags` — tag push triggers CI: build.yml runs the
   full ctest suite; release.yml builds all 4 packages and auto-creates the
   GitHub release with artifacts.
6. After CI, curate notes: `gh release edit vX.Y.Z --notes-file notes.md`
   (English only, list changes since previous release, changelog link
   `compare/vPREV...vX.Y.Z`). trixie deb uses `+deb13u1` revision marker
   (`latte-dock-ng_<ver>-1+deb13u1_amd64.deb`); testing/sid keeps plain `-1`.

### 2. Portage overlay
- Overlay repo: local checkout of `ruizhi-lab/gentoo-overlay`, branch `main`.
- Ebuild: `kde-misc/latte-dock-ng/` inside that overlay checkout.
1. `cp latte-dock-ng-X.Y.(Z-1).ebuild latte-dock-ng-X.Y.Z.ebuild`
2. SRC_URI must use `v${PV}` to match tag format.
3. Regenerate Manifest: `DISTDIR=/tmp/distfiles ebuild latte-dock-ng-X.Y.Z.ebuild manifest`
   (no sudo — the system DISTDIR may require the portage group; a `/tmp`
   DISTDIR bypasses it).
4. `git rm` the old ebuild, regenerate Manifest again to drop old checksums.
5. Commit and push.

### Release rules
- All text in English (notes, commits, ebuild metadata).
- Tags always use `v` prefix.
- Manifest only contains checksums for versions with ebuilds present.
- **Generate the Manifest AFTER the tag is final** — force-updated tags change
  GitHub tarballs; if the tag moves, re-download through the local proxy and
  regenerate, else emerge fails with
  "Filesize does not match recorded size" (v1.2.36 incident). Delete stale
  Manifest first so no baseline forces a refetch. Never use sudo.

## Reference: GitHub proxy
GitHub ops (push/ls-remote) may hang without a proxy. Try direct first; on a
hang, retry through the local HTTP proxy configured in the shell environment
(machine-local — the exact proxy address is not committed to the repo).

## Architecture & compatibility notes

- The application is one large executable assembled by `app/CMakeLists.txt`.
  Large runtime sources include `layoutmanager.cpp`, `containmentinterface.cpp`,
  `view.cpp`, `storage.cpp`, and `AppletItem.qml`.
- Use `-j8` for project builds unless a command has a specific resource limit.
- User configuration intentionally disables window previews and keeps only the
  normal tooltip; treat preview-window rendering as an inactive path unless the
  user explicitly enables it for a regression check.
- The application icon uses the versioned `latte-dock-ng` name; do not fall
  back to the legacy `latte-dock` name, which third-party icon themes (e.g.
  Breeze) shadow with the old artwork.
- Debian Plasma 6.3 lacks a filesystem `org.kde.plasma.plasmoid` QML module:
  register it lazily and never use an attached-type stub that breaks Behavior
  resolution (Qt 6.8 QML compiler).
- On Fedora, Wayland runs natively even with a minimal environment; capture
  latte's own logs with `--log-file` plus `QT_LOGGING_RULES='latte*=true'`.

## Known issues & fixes (diagnosis memory)

### Appmenu shows empty dock slot (NOT a regression)
Issue #36: official appmenu (org.kde.plasma.appmenu) shows an empty slot when no
menu-bearing window is active. Cause: appmenu QML sets `Plasmoid.status =
HiddenStatus`; the official panel hides HiddenStatus applets
(`containments/panel/AppletContainer.qml`), but latte deliberately keeps external
applets visible (`AppletItem.qml` `keepVisibleOnHiddenStatus`) → empty slot.
"Works on 6.6, blank on 6.7" was windows-open vs windows-closed, not a regression.
Reply to such issues: widget is in no-menu HiddenStatus; panel comparison is the
reference behavior.

### knscompat Badge qmldir bug (FIXED)
`kTemplatesQmldir` in `app/knscompat.cpp` was a hardcoded static qmldir missing
`Badge 2.0 Badge.qml` → `KT.Badge is not a type` → kicker/kickoff/kickerdash fail
to load → EMPTY SLOTS on newer Kirigami (verified on Kirigami 6.28).
FIXED in b0b0a63c3: `templatesQmldirForSystem()` generates the compat templates
qmldir from the system qmldir (stripping `prefer`/plugin lines); static list
(now including Badge) is fallback; compat stamp bumped 8→9. Test:
`systemTemplatesQmldirMirrorsNewTypes`.

### Blur ghosting regression (FIXED)
f183f7356 (v1.2.27) reported themeExtendedBackground.maxOpacity as
effectiveBackgroundOpacity (was 1.0), bypassing the ghosting gate
(`effectiveBackgroundOpacity < 0.95f`) → blur on every default translucent panel.
Fix (2026-07-17): blurEnabled default true→false; effects.cpp gate uses
`m_backgroundOpacity` with explicit `skipForGhosting` bool (default -1 = theme-
controlled, never skipped; user values >= 0.95 skipped); BindingsExternal.qml
effectiveBackgroundOpacity restored to 1.0. Tests: effectslogictest, sourcecontracttest.

### Bounce-end zoom micro-stutter (KNOWN, PR #43 residual)
After the PR #43 fix (issue #42), when the pointer sits STATIONARY on a neighbour
icon while a launcher's bounce finishes, the just-opened icon still stutters for
a frame as the dying launcher delegate is replaced by the window delegate.
Mechanism: the parabolic zoom chain is only re-evaluated on pointer movement
(`ParabolicEventsArea.onParabolicMove` -> `calculateParabolicScales`). When
`RealRemovalAnimation` finishes (zoomLength->0 collapse, then
`removeWaitingLauncher` -> `slotWaitingLauncherRemoved` sets the new window
delegate visible at `zoom = 1`), the ListView reflows and neighbour positions
shift while the pointer is idle -> zoom values are stale for one frame and the
incoming delegate pops in at 1x. No cheap fix: re-broadcasting on delegate
visibility change would re-enter the relay during removal animations
(oscillation risk, see `relayDepth` guard in
definition/ParabolicEffect.qml). Accepted as known; revisit only if a safe
"tasks layout changed" hook can re-trigger the current item's parabolicMove.

### Hover preview stutter (open research)
Separate Wayland platform window (`LatteCore.Dialog`, QQuickWindow) for the
preview popup: `QWindow::setVisible(true)` maps a Wayland surface and blocks the
Qt Quick render pipeline → stutter. Plasma 6 avoids this by rendering tooltips
INLINE in the panel window. `ParabolicEventsArea.qml` parabolicMouseArea (z:10000)
intercepts all hover events; TaskMouseArea (z:0) never gets onEntered; any
cross-component JS call to `abilityItem.showPreviewWindow()` stutters.
Next approach: C++ signal (`currentParabolicItemChanged`) → QML-invokable slot;
or investigate `PlasmaCore.Dialog` vs `LatteCore.Dialog`. Key files:
`plasmoid/package/contents/ui/main.qml:438` (windowsPreviewDlg),
`app/view/parabolic.h/cpp`, `TaskItem.qml:531` (showPreviewWindow).
