# Modernization Source Baseline: 2026-09-29

This is an evidence record for the
[plan](architecture-modernization-plan.md), not a build or performance report.

## Revision and checks performed

- Local branch: main; HEAD `e3ef1ddf001aa032cffa62db21cfd97196174746`.
- Release version: 1.2.51 in CMakeLists.txt and default.nix.
- Remote refs/heads/main returned the same full hash using git ls-remote on
  2026-09-29. Initial connectivity failed; the repository's prescribed local
  GitHub proxy succeeded. No fetch, branch switch, commit or push was needed.
- Working tree was clean before this documentation refresh.
- Source and configuration were inspected with CodeGraph and focused reads.
- Existing CTest registrations were listed with --show-only=json-v1; tests
  were not executed. Existing compilation commands were sampled.
- No fresh configure/build, desktop restart, runtime profile, coredump test,
  remote CI inspection or Nix build was performed.

M0 must regenerate equivalent configurations. Nothing here claims the current
code compiles without warnings or that existing build artifacts match HEAD.

## M0 execution update: fresh local baseline

On 2026-09-29, branch `codex/modernization-m0-baseline` was created from the
recorded source revision above. Its starting HEAD was
`e3ef1ddf001aa032cffa62db21cfd97196174746`; it matched the recorded main
baseline. Fresh GCC and Clang Debug/Release configurations were generated in
`build-modernization-baseline/` with testing and compile-command export enabled.
See the [M0 handoff](modernization-results/M0.md) for exact commands, tool
versions and retained logs.

All four configurations built the application/plugin targets and
`latte-autotests`; each registered the same 43 tests (sorted-name SHA-256
`5a379c7ee7b6c098981b0a45acfd72792bd9e69e8f5e27374b1c9d4c0f703558`) and passed
43/43. The first in-sandbox GCC Debug run could not create an isolated D-Bus
socket; rerunning the same suite with the required host permission passed. This
is an execution-environment constraint, not a test assertion failure.

The syntax lint passed over 236 QML files. GCC Debug and Clang Debug deep lint
completed with matching category counts: unqualified 5572, missing-property
1268, Quick.anchor-combinations 18, incompatible-type 6, unused-imports 5,
unresolved-type 4, Quick.property-changes-parsed 4, stale-property-read 3,
import 2 and Quick.layout-positioning 2. Treat these as a measured existing
backlog, not zero-diagnostic acceptance. Compiler warning inventory found one
GCC Release `-Wsfinae-incomplete` at `app/view/view.h:69:7`; GCC Debug and both
Clang configurations had no C++ compiler warning lines. A separate QML type
metadata warning says `PlasmaQuick::Dialog` cannot be found as the base type in
`dialog.h:25`; it is tooling/metadata evidence and not a compiler warning.
These findings remain for M2a/M2b and M3; M0 made no production changes.

## Source and tooling observations

| Area | Evidence | Consequence |
| --- | --- | --- |
| Source floors | CMake 3.20; C++20; Qt 6.6; KF6 6.0; Plasma 6.3 | Preserve these declared source compatibility floors. |
| Oldest supported distro acceptance | Debian 13.7: CMake 3.31.6-2; Qt 6.8.2; KF6 6.13; Plasma 6.3.6 | Verify and record these packaged versions in the Debian 13 CI build; do not confuse them with source-declared floors. |
| Forward-compatibility distro acceptance | Gentoo, Arch and Fedora stable package stacks | Record versions from CI runs; rolling package versions are not pinned in the plan. |
| Application | app/CMakeLists.txt collects sources in latte-dock-ng; subdirectories append through PARENT_SCOPE | A directory is not an enforced library boundary. |
| Host symbols | latte-dock-ng enables exports and default C++ visibility for private.app | Hidden visibility or target extraction can break imports. |
| QML modules | C++ core/containment/tasks/app modules use generated registration/metadata; abilities/components install loose QML | Preserve module-specific metadata and both installation roots. |
| Compiler policy | KDECompilerSettings plus explicit -Wformat and -Werror=format-security | No general first-party warning gate was observed in source/CI. |
| CI | GCC/Clang Release builds, explicit latte-autotests builds, CTest, QML lint, distribution install/package checks | Existing gates must survive modernization; CI status was not checked. |
| Deep QML lint | Empty PROMOTED_ERROR_CATEGORIES; comments use a historical 7k+ count; referenced backlog file absent | Remeasure and add a comparable diagnostic baseline. |
| Developer configuration | .clangd points at build; no tracked CMakePresets.json or .clang-tidy | Preserve developer selection while making configurations repeatable. |
| Nix | Packages, overlay and NixOS module; no explicit checks or devShells | Flake evaluation alone is not autotest execution. |

Observed local tools: CMake 4.3.4; GCC 16.2.1_p20260926; Clang 22.1.8;
Qt Core and qmllint 6.11.2; ECM 6.29.0. qmllint was available at
/usr/lib64/qt6/bin/qmllint. These are local observations, not supported minimums
or a portable environment specification. Plasma runtime, compositor, GPU and
fresh build dependency versions were not established.

An existing build/compile_commands.json entry for the window tracker contained
-Wall, -Wextra and other ECM diagnostics, with specific -Werror options for
return-type, init-self, undef and format-security, but no general -Werror.
This is an artifact sample; M0/M2 must inspect fresh commands for all targets.

## Existing local registrations

| Directory | Cached mode/compiler | Registered tests |
| --- | --- | --- |
| build | Debug, /usr/bin/g++ | 42 |
| build-autotests-gcc | Debug, /usr/bin/c++ | 41 |
| build-autotests-clang | Debug, clang++ | 42 |
| build-release | Release, /usr/bin/c++ | 40 |
| build-user-ruizhi | Debug, /usr/bin/c++ | 42 |

These counts describe old local caches. They neither establish missing source
tests nor prove binaries exist. Compare names and optional dependencies after
fresh configuration; do not hard-code 42 as a universal acceptance condition.
The repository's required release/runtime checks still apply.

autotests/coverageestimate.py returned 79/152 = 52.0%. Its source-reference
heuristic is neither line/branch coverage nor proof of behavior coverage. In
particular, copied logic and source assertions must be identified separately.

## Production behavior coverage

- positionergeometrytest includes the production viewgeometryhelpers.h and
  checks screen/panel geometry. Preserve and extend this existing coverage.
- windowviewbackendtest compiles the production backend and uses an isolated
  D-Bus session. Preview and plugin tests provide additional existing coverage.
- visibilitylogictest defines its own BlockHidingEvents and other policy
  functions, with a comment saying they replicate production logic.
- parabolicmathtest replicates QML zoom calculations. storagelogictest defines
  copied storage helpers. Other tests were not comprehensively classified.
- M5 selects only the hiding-blocker case: empty/duplicate events do not change
  state; hidingIsBlockedChanged is emitted only on empty/nonempty transitions.
  VisibilityManager remains the authoritative per-view state owner.

## Performance candidates and current safeguards

| Candidate | Verified source behavior | Required caution |
| --- | --- | --- |
| Window metadata cascade | WaylandInterface::updateWindowCache emits AbstractWindowInterface::windowChanged; Windows::init refreshes requestInfo and calls updateAllHintsAfterTimer | Its cosmetic label also covers onAllDesktops, skipTaskbar and parentWindow. Do not classify the entire path as harmless metadata. |
| Window hints | A 300ms single-shot debounce, inactive-view filtering and aggregation from view hints already exist | Preserve active-window/removal immediacy and desktop/activity invalidation. |
| Icon regeneration | IconItem::geometryChange schedules pixmap work; loadPixmap marks texture changed; updatePaintNode uploads a new texture when needed | Preserve SVG repaint reentrancy guard, theme/source invalidation, overlays, color extraction, DPR and scene-graph thread rules. |
| Edit state | Tasks root runs a 200ms compatibility poll while tasksModel.count > 0; direct containment assignment is documented | It is already centralized, not one poll per task. Prove notifications before removing fallback. |
| Edge relocation | Task layout refresh has an immediate pass and at most eight 120ms delayed passes | It is bounded repair, not perpetual idle polling. Stable geometry alone may precede asynchronous delegate readiness. |
| Applet sorting | AppletItem's 16ms timer starts during drag and stops when drag ends | Do not claim this timer is idle overhead. |
| Per-applet queries | Eligible applets load TasksModel, VirtualDesktopInfo and ActivityInfo objects | Measure proxy/filter work; underlying library data sources may already be shared. |
| Hidden containment | LayoutsContainer uses opacity to prevent applet element resets; hidden parabolic areas forward neighbor messages | Unloading or setting the whole tree invisible can regress window activation and zoom propagation. |
| Effects | ExternalShadow uses a layer and MultiEffect | Allocation/visual benefit depends on actual usage and GPU measurements. |
| Preview | Separate process starts on demand; bounded IPC/watchdog/generations; helper idle lease is 30 seconds | Do not move capture into the dock or enable legacy previews to save memory. |
| QML startup | Pure-QML modules remain loose files; application clears disk cache on version/cache-revision changes | Build-time caching is an experiment; warm disk-cache behavior and overrides must be compared. |

Relevant source entry points:

- [Wayland adapter](../app/wm/waylandinterface.cpp),
  [window tracker](../app/wm/tracker/windowstracker.cpp),
  [last active window](../app/wm/tracker/lastactivewindow.cpp).
- [IconItem](../declarativeimports/core/iconitem.cpp),
  [tasks root](../plasmoid/package/contents/ui/main.qml),
  [AppletItem](../containment/package/contents/ui/applet/AppletItem.qml).
- [LayoutsContainer](../containment/package/contents/ui/layouts/LayoutsContainer.qml),
  [ExternalShadow](../declarativeimports/components/ExternalShadow.qml),
  [PreviewProcess](../plasmoid/plugin/previewprocess.cpp),
  [preview helper](../plasmoid/preview/main.cpp).

Large file lengths were unchanged from the earlier assessment: layoutmanager.cpp
2,872; containmentinterface.cpp 2,536; view.cpp 2,127; AppletItem.qml 1,975;
storage.cpp 1,911. Length is navigation context, not a refactoring objective.

## References checked for execution guidance

- [CMake 3.20 presets](https://cmake.org/cmake/help/v3.20/manual/cmake-presets.7.html):
  schema version 2 supports configure/build/test presets.
- [Compiler warning-as-error property](https://cmake.org/cmake/help/latest/prop_tgt/COMPILE_WARNING_AS_ERROR.html):
  available since CMake 3.24; use a compatible fallback at the retained floor.
- [Qt 6.8 performance guidance](https://doc.qt.io/qt-6.8/qtquick-performance.html):
  profile actual work and distinguish invisible objects from inactive ones.
- [Qt 6.8 QML module build integration](https://doc.qt.io/qt-6.8/qt-add-qml-module.html):
  QML_FILES enables build-time caching; generated metadata and paths remain
  integration contracts. Check used APIs against Qt 6.6 before implementation.
