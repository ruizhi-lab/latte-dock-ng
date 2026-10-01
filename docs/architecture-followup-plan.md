# Architecture and Legacy-Code Follow-Up Plan

Status: planning only. Reviewed on 2026-10-01 from `main` at `39610fd89`.
This document does not mark any implementation or runtime verification complete.

## Scope and relationship to existing work

This plan follows the source review of architecture, modern C++/Qt practices,
and Qt 5, Plasma 5 and X11 remnants. The completed quality and performance
packages in [the modernization plan](architecture-modernization-plan.md) remain
the baseline; do not repeat or reopen them without a concrete regression.
Read [AGENTS.md](../AGENTS.md), [the architecture map](architecture-overview.md),
and the relevant source comments before each implementation slice.

The active build requires C++20, Qt 6.6+, KF6, Plasma 6.3+ and Wayland. No
Qt5/KF5 link target, X11 backend or XCB build dependency was found in the
reviewed active paths. Search matches are not by themselves removal candidates:
`qt5compat` is a distribution package name, XWayland may be a runtime
dependency of the desktop stack, persisted legacy identifiers are compatibility
data, and the old primary-output protocol remains a supported fallback.

## Execution order

Use one row per implementation turn and one branch per independent row or a
short linear sequence. Keep each diff reviewable. The discovery rows must
produce evidence before any deletion or behavioral refactor.

| ID | Dependency | Bounded work | Acceptance |
| --- | --- | --- | --- |
| A0 | None | Record a fresh inventory of active vs historical Qt5/KF5/X11 references, large ownership boundaries, QML diagnostic counts, current HEAD and build presets. Classify every candidate below as executable behavior, persisted compatibility, package naming, comment or dead code. | A dated evidence table with file/line, consumer, authority and disposition; no production edit. |
| A1 | A0 | Remove stale *comments only* in `declarativeimports/core/extras.h`, `containment/package/contents/ui/colorizer/CustomBackground.qml` and similar proven historical text. Check whether `ENABLE_MAKE_UNIQUE` is ever enabled before touching its guarded implementation. | Comments explain the current Qt6 constraint; no behavior change; link/whitespace review. If the macro can be enabled, move its code to a separate reviewed slice. |
| A2 | A0 | Audit `app/main.cpp::filterDebugMessageOutput` as a table of exact message, emitter, reason, supported versions and removal condition. Split the broad substring matches from exact known third-party diagnostics; change one match family per patch only after a captured log proves its source. | A controlled injected diagnostic remains visible; known dependency warning is filtered; real warning/error still reaches the log. GCC/Clang and relevant startup checks pass. Do not blanket-remove filtering. |
| A3 | A0 | Trace `LayoutManager::usesLegacyJustifySplitters()` and every caller, config migration and QML counterpart. Decide whether the constant-false branches are unreachable for supported saved layouts. | Consumer map and saved-layout fixtures first; if safe, remove only the unreachable branch family and verify dock alignment, separator/spacer and drag behavior on a real desktop. |
| A4 | A0 | Trace `Corona::screenForContainment`, `Synchronizer::screenForContainment`, `ScreenPool` connector IDs and `lastScreen()` fallback. Characterize primary-not-zero, hotplug and nested containment behavior before changing anything. | Focused production tests for mapping/fallback plus physical two-output retest or an explicit pending-validation record. No index-to-ID conversion based on the old FIXME alone. |
| A5 | A2-A4 evidence | Choose **one** decision-heavy policy in `View`, `ContainmentInterface` or containment `LayoutManager` with a stable input/output contract. Extract only that policy into a small production helper; retain QObject, QML and plugin ownership at the existing boundary. | Tests call the production helper and cover invalidation/fallback; host-dependent QML plugin loads; GCC/Clang builds, CTest, QML checks and affected runtime behavior pass. Stop if extraction creates a second state authority. |
| A6 | A5 | Review the next candidate using actual change cost and failure evidence. Update the component map only if an ownership boundary changed. | Written continue/defer decision with measured maintenance or validation benefit; no automatic second extraction. |

Do not combine A2, A3, A4 or A5 in one patch. A1 may be done independently
after A0. A3 and A4 are high-risk because saved layouts and screen identity
cross process restarts; a static search cannot establish runtime safety.

## Shared implementation gates

1. Start from a clean or fully accounted-for worktree. Record branch, HEAD and
   existing user edits. Use CodeGraph first when `.codegraph/` exists.
2. No feature, interaction, persisted setting, fallback or supported compositor
   path may regress. Keep behavior unchanged for cleanup/refactoring slices;
   behavior changes require a focused test of the production path and relevant
   desktop retest before the batch can be Complete. If equivalence cannot be
   established, retain the implementation and mark that candidate Deferred.
3. Preserve QML URIs, D-Bus interfaces, config keys, plugin installation roots,
   host-exported `private.app` symbols and Wayland compositor capability checks.
4. Keep the authoritative state and stale-result/cancellation rule explicit
   beside changed code. Add a focused behavioral test when practical; source
   contracts alone do not prove desktop behavior.
5. Format only touched C++ files. Run affected GCC and Clang builds with zero
   warnings, relevant CTest targets and QML syntax/deep lint. Use the existing
   presets and strict-warning gate; record exact commands and failures.
6. For runtime changes, use the user-mode Debug install and detached launch in
   AGENTS.md, obtain user retest feedback, then inspect `/tmp/latte-ng.log`.
   Crash/quit changes additionally use the documented coredump A/B workflow.
7. Do not claim physical multi-display, release-build performance, remote CI
   or compositor behavior passed when it was not exercised. Mark the slice
   Pending validation with its next exact action.

## Branch and handoff protocol

The planning document can remain on `main` for review. When implementation is
requested, create or select a `codex/` branch explicitly for the selected row;
record the base commit and do not silently switch an existing working branch.
On `main`, commit and push need separate user approvals. On a non-main branch,
follow the AGENTS.md commit/push policy after checks pass. Never rewrite a
published branch as part of a cleanup slice.

For each row, write `docs/modernization-results/Ax.md` with: actual start HEAD,
branch, changed files, consumer map, source of truth, exact checks and results,
desktop feedback, unavailable gates, and one exact next action. Update this
plan's ledger. Large logs may be stored as build artifacts; a `/tmp` path alone
is not a durable handoff.

| Row | Status | Evidence | Next action |
| --- | --- | --- | --- |
| A0 | Complete | [Inventory and candidate classification](modernization-results/A0.md) | A1: remove only the verified stale compatibility comments. |
| A1 | Complete | [Comment-only cleanup](modernization-results/A1.md) | A2: classify each log filter against captured emitters before changing behavior. |
| A2 | Deferred | [No current log evidence](modernization-results/A2.md) | Capture a fresh user-mode Debug log that identifies each suppressed message before changing filters. |
| A3 | Deferred | [Legacy splitter branch analysis](modernization-results/A3.md) | Reopen only with fixture coverage and user retest for Justify alignment, separators and drag ordering. |
| A4 | Pending validation | [Mapping trace and comment correction](modernization-results/A4.md) | Add production mapping tests and retest primary-not-zero, nested applets and display hotplug before changing mapping logic. |
| A5 | Not started | None | Wait for selected A2-A4 evidence. |
| A6 | Not started | None | Wait for A5. |

## Prompt for a smaller implementation model

Copy this prompt and replace `A0` with exactly one row ID:

> Execute only row A0 of `docs/architecture-followup-plan.md`. Read AGENTS.md,
> docs/architecture-overview.md, that row and its dependencies/handoff. Verify
> actual branch, HEAD and user edits; use CodeGraph first. Do not infer that the
> dated assessment is still current. Keep the patch within the selected scope,
> preserve the listed compatibility contracts, run its acceptance gates and
> record exact results in `docs/modernization-results/A0.md`. Update the ledger
> and leave one exact next action. If a prerequisite or runtime check is
> unavailable, mark Pending validation rather than claiming completion. Follow
> the current branch's commit/push policy in AGENTS.md.
