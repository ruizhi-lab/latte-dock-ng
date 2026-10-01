# Architecture, Quality and Performance Modernization Plan

Status: execution backlog; this refresh implements documentation only.
Assessment date: 2026-09-29.
Source baseline: `main` at `e3ef1ddf001aa032cffa62db21cfd97196174746` (1.2.51).
The local HEAD and remote `refs/heads/main` matched when checked on that date.

## Authority and how to use this plan

Preserve the current features, C++20, CMake 3.20, Qt 6.6, KF6 6.0, Plasma 6.3
and Wayland. This plan authorizes no implementation, release or remote mutation
by itself. The user's selected batch determines the implementation scope.

## Compatibility and validation environments

Keep the source-declared floors at CMake 3.20, Qt 6.6, KF6 6.0 and Plasma 6.3.
Validate the CMake preset minimum at 3.20 and separately test the oldest
supported Debian desktop stack using Debian 13.7 packages: CMake 3.31.6-2,
Qt 6.8.2, KDE Frameworks 6.13 and the Plasma 6.3.6 desktop release. Debian's
`libplasma-dev` package is currently 6.3.5-1 while `plasma-workspace-dev` is
4:6.3.6-2. The reproducible container checks both package floors; local desktop
retest records the installed Plasma runtime version. Gentoo, Arch and Fedora CI
checks represent forward-compatibility coverage against their current stable
component versions; capture those versions from each run rather than pinning a
moving “latest”.

Read [AGENTS.md](../AGENTS.md) and the relevant entries in the
[architecture map](architecture-overview.md). Then read this plan's selected
row, the common rules and that batch's section in the
[implementation guide](architecture-modernization-implementation.md).
The [baseline record](architecture-modernization-baseline.md) separates source
observations from checks actually executed. The
[performance protocol](performance-validation-guide.md) defines measurements
and desktop acceptance. These documents replace the assessment at `78813c598`;
they do not imply that its proposed M0-M7 packages were implemented.

For a smaller model, assign one slice such as M1a or P1b per turn. Read the
common rules plus the selected section rather than exploring every candidate.
Every slice must leave a durable handoff containing an exact next action.
Boundary review means an explicit review of the listed assumptions by the user
or a separately selected reviewer; it does not request automatic delegation.

## Current baseline and uncertainty

| Observation at the source baseline | Implication |
| --- | --- |
| Modern language/platform stack and API capability probes already exist | Keep dependency floors and cross-distribution fallbacks. |
| Most application sources still form one executable; private.app imports depend on host exports | Extract useful internal policy only; preserve host/plugin linking. |
| Geometry production helpers and direct backend tests already exist | Extend them instead of declaring all policy untested or re-extracting it. |
| Some visibility, storage and parabolic logic tests copy production algorithms | A passing copy cannot prove production behavior. Convert one bounded case first. |
| GCC/Clang CI, syntax/deep QML lint and distribution installation checks exist | Extend current gates. Their presence does not establish current CI health. |
| No tracked presets, clang-tidy config or referenced QML backlog document; promoted QML categories are empty | Reproducibility and diagnostic governance remain unfinished. |
| Existing local caches list 40-42 tests, depending on the directory | Fresh equivalent configurations and test-name comparison are mandatory. |
| Window hints already debounce, skip inactive views and aggregate layout hints | Preserve these optimizations; do not repeat the work. |
| updateWindowCache emits windowChanged; the tracker unconditionally schedules hint recalculation | A concrete event-routing optimization candidate exists. Start with title-only changes. |
| Icon geometry changes schedule pixmap loading and texture updates | Deduplication is a candidate, not a measured bottleneck. |
| Root-level edit polling and bounded edge-layout refreshes exist | Compatibility/timing workarounds need characterization before reduction. |
| Preview helper already starts lazily and has bounded IPC, watchdogs, stale-reply rejection and an idle lease | Retain isolation and fallback; do not claim process removal as an optimization. |
| Nix exposes packages/overlay/module but no explicit checks/devShells | Establish a test contract without changing the lock or release package scope. |

No fresh application build, full test execution, live desktop retest, remote CI
inspection or performance profile was performed for this documentation refresh.
No CPU, memory, GPU or startup improvement has been established.

## Delivery sequence

The M identifiers remain the quality/architecture track. P identifiers are the
new performance track. Each letter identifies an independently reviewable
slice; do not merge all letters into one change by default.

| Order | Batch | Dependencies | Deliverable and slice boundary | Execution level |
| --- | --- | --- | --- | --- |
| 1 | M0 | None | Fresh four-configuration quality baseline and test/diagnostic inventory | Routine evidence collection |
| 2 | M1a | M0 | CMake schema-v2 presets and local editor selection | Routine configuration |
| 3 | M1b | M1a | CI uses equivalent presets and eight build jobs | Routine configuration |
| 4 | M2a | M1a, M1b | Strict first-party compiler-warning gate with negative fixtures | Bounded build policy |
| 4a | M2b | M2a diagnostic evidence | One warning family per slice, only if required | Localized fixes; review behavior changes |
| 5 | M3a | M0 | Stable QML diagnostic identities and parser tests | Bounded tooling |
| 6 | M3b | M3a, M1b | Comparable baseline, CI comparison and backlog governance | Bounded tooling |
| 7 | M4a | M1a, M2a | Small targeted static-analysis configuration | Bounded tooling |
| 8 | M4b | M4a | Opt-in ASan/UBSan configuration and selected tests | Instrumentation review |
| 9 | M5a | M0, M1a, M2a | Production hiding-blocker value helper, behavior unchanged | Bounded C++ extraction |
| 10 | M5b | M5a, M4b | Replace copied blocker tests and verify production wiring | Lifecycle/binding review |
| 11 | P0 | M0, M1a | Repeatable A/B performance baseline and scenario evidence | Measurement; desktop coordination |
| 12 | P1a | P0, M3b, M5b | Title-event characterization and a reviewed routing specification | Window-state boundary review |
| 13 | P1b | P1a | Title-only fast path, conservative fallback and production tests | Bounded implementation after review |
| 14 | P2a | P0, M3b, M4b | Icon invalidation matrix and observed raster/texture activity | Render-state boundary review |
| 15 | P2b | P2a | One-item deduplication; no global/multi-size cache | Bounded implementation after review |
| 16 | P3a | P0, M3b | One bounded edge-layout convergence optimization | QML timing review |
| 17 | P3b | P0, M3b | Edit-state notification proof, then a scoped polling reduction | Plasma compatibility review |
| Deferred | M6 | M2a, M4b, M5b | Candidate review found no target that reduces an app dependency; see [M6 evaluation](modernization-results/M6.md) | Revisit only with measured build or ownership benefit |
| Independent | M7 | M1a, M2a | Explicit Nix development and test outputs | Nix sandbox review |
| Conditional | P4 | P0, M3b, M5b | Evaluate one shared per-view task query before implementation | Model ownership review |
| Conditional | P5 | P0, M3b | Evaluate one effect/layer allocation optimization | GPU/visual review |
| Conditional | P6 | M1a, M3b, P0 | Evaluate one pure-QML module's build-time caching | Import/install review |

M3 and M7 can proceed independently of runtime refactoring once their listed
dependencies are met. P1 and P2 are alternatives selected by measurement;
neither must wait for the other. P3a/P3b are separate fixes. M6 and P4-P6 need a
recorded benefit and approved narrow design before implementation. Do not add
M6 merely to increase the number of CMake targets. LTO/PGO and global QML
conversion remain deferred experiments, outside the default sequence.

Dependencies describe usable functionality and evidence. A predecessor with
all required local checks green but remote CI pending may support the next
local tooling slice; keep both remote acceptance and parent completion pending.
Unexplained local build/test failures block dependent runtime changes. A pending
desktop retest never counts as a validated runtime foundation for another fix.

## Invariants that every batch must preserve

- One authoritative source per state. Cached values have explicit invalidation;
  asynchronous work rejects stale results and stops at teardown.
- Corona coordinates application services, View composes a dock, QML owns
  presentation. Layout coordination and containment arrangement stay distinct.
- QML URIs/type names, configuration formats, D-Bus interfaces and both QML
  installation roots remain compatible. Preserve private.app host exports.
- QObject parent ownership does not replace dependency-aware teardown. Keep
  thread affinity, context-bound callbacks and the documented shutdown order.
- Preview hover choice remains authoritative; title tooltips, highlight,
  helper failure fallback and process isolation stay intact.
- Hidden applets can still forward parabolic messages. Opacity-based hiding is
  deliberate; changing the whole containment to visible:false or unloading it
  can reset applet state and reshow windows.
- Plasma popup positioning uses PlasmaQuick::Dialog::adjustGeometry(), never
  raw QWindow::setPosition(). Preserve icon/theme and fractional-DPR behavior.
- Do not remove compatibility timers based on interval alone. Bounded repair
  or drag-only timers are different from steady-state polling.
- Preserve valuable source contracts. New tests exercise production logic;
  copied algorithms are not a replacement for behavioral coverage.
- Document changed local invariants beside their implementation, with trigger,
  state authority, lifetime/timing constraint and prevented failure.

## Acceptance and completion

Every batch has its specific checks in the implementation guide. Common gates:

1. Scope is confined to the selected slice, with no unrelated formatting,
   dependency bump, feature removal, release or configuration migration.
2. Affected GCC/Clang application, helper, plugin and test builds have zero
   warnings/errors. Strict-gate work includes Debug and Release negative checks.
3. Fresh configurations list the expected tests; all required tests execute.
   Missing binaries, unexplained skips and altered tests are not success.
4. QML syntax/module checks remain active; comparable diagnostic output has no
   unreviewed additions. A parser/tool failure is not an empty clean baseline.
5. Runtime changes pass the canonical Debug install, detached user-mode launch,
   user feedback and log review. Teardown/crash fixes additionally require the
   clean-quit/coredump and applicable pre-fix/fixed A/B checks.
6. Performance changes meet the [performance protocol](performance-validation-guide.md):
   reduced intended work, repeated representative A/B evidence, no functional
   regression and no unexplained latency/memory tradeoff. No invented target
   percentage or comparisons between Debug and Release.
7. Relevant host/plugin, distribution and Nix checks are recorded separately.
   Local CTest does not prove remote CI or a real compositor passed.
8. Durable evidence and an exact handoff exist. Required unavailable checks
   leave the batch pending validation; they are never reported as passed.

An evaluation with no measurable opportunity ends with a documented deferred
decision and no optimization patch. It is not completion of an implementation
batch. Quality-baseline failures must be explained before dependent changes.

## Progress ledger

Update individual slice rows as work starts. Add M2b rows per warning family
and separate rows for any later evaluation/implementation. The source snapshot
is complete; execution and runtime evidence below are not.

| Batch | Status | Evidence | Next action |
| --- | --- | --- | --- |
| Source assessment | Complete | [2026-09-29 record](architecture-modernization-baseline.md) | Start M0 when implementation is requested |
| M0 | Complete | [Fresh local baseline and handoff](modernization-results/M0.md) | M1a: add CMake schema-v2 presets and validate all four configurations |
| M1a | Complete | [CMake 3.20.6 Debian configure/build pass with zero warnings; 43/43 CTest](modernization-results/M1a.md) | M2a: add and verify warning-as-error policy |
| M1b | Complete | [Run 36623951814: all four preset build/test/lint jobs and distro checks pass](modernization-results/M1b.md) | Continue with M3b application-host smoke while strict-gate CI runs |
| M2a | Complete | [Run 36633737377: GCC/Clang Debug/Release, strict fixtures, CTest and distro matrix pass](modernization-results/M2a.md) | Continue M4b sanitizer coverage |
| M2b-GCC16 | Complete | [Qt QMetaType incomplete-SFINAE diagnostic scoped to generated moc aggregate](modernization-results/M2b-gcc16.md) | Recheck the suppression after GCC or Qt changes this probe |
| M3a | Complete | [Stable parser, fail-closed comparisons and fixtures](modernization-results/M3a.md) | M3b: collect a reviewed baseline in a controlled environment and gate CI |
| M3b | Complete | [Run 36633737377: all four baseline comparisons and private.app host smoke pass](modernization-results/M3b.md) | Continue runtime/performance track after prerequisites |
| M4a | Complete | [Run 36633737377: Clang Debug clang-tidy production and negative checks pass](modernization-results/M4a.md) | Continue M4b sanitizer coverage |
| M4b | Complete | Run 36642361102: GCC sanitizer job passed with leak detection enabled; ASan/UBSan fixtures and selected tests passed | Keep sanitizer checks in CI |
| M5a | Complete | [Report](modernization-results/M5a.md): production helper and focused tests pass GCC/Clang; run 36642361102 passed the full GCC/Clang matrix and sanitizer job | Record desktop interaction retest after implementation batches |
| M5b | Complete | [Report](modernization-results/M5b.md): user reports functional blocker/hover checks passed; GCC Debug lifecycle test passes 9/9; Debian Dock PID 14073 exited cleanly with live helper PID 14215, no new coredump or teardown warnings, then restarted as PID 14298 | Retain the focused lifecycle regression test and bounded helper reap behavior |
| P0 | Complete | [Report](modernization-results/P0.md): Fedora baseline, idle/hidden-idle, multi-window, continuous-hover and live four-edge transition evidence retained; Debian 13.7 refreshed from USTC mirrors, rebuilt from the checked branch with GCC 14.2/CMake 3.31.6/Qt 6.8.2/KF 6.13/Plasma 6.3.6, passed all 46 CTest targets and received the final runtime pass; both saved test layouts restored byte-for-byte. User confirms all three hover modes and selected widgets work on both Fedora and Debian, resolving the earlier Debian combined-mode screenshot discrepancy. Two-output behavior remains accepted by code review, not runtime-tested. Measurements characterize tested scenarios and do not establish a CPU or memory saving | Retain the two-output code-review assumption; revisit performance only with comparable A/B or additional instrumentation |
| P1a | Complete | [Report](modernization-results/P1a.md): instrumented Fedora Wayland title-only changes; five runs delivered 10 schedules each (execution median 4, with all outliers retained); no production behavior change | P1b adds a title-only tracker route with zero hint schedules as its predeclared work target; keep all non-title events on the full path |
| P1b | Deferred | [Report](modernization-results/P1b.md): GCC/Clang builds and focused tests pass; five Fedora Wayland Debug trials show 0/10 hint schedules; five uninstrumented Release A/B pairs retain a CPU/PSS outlier; user reports functional checks passed; event-correlated latency is unavailable | Keep as long-term observation; do not claim a measured CPU or memory saving unless controlled A/B results later resolve the noise and latency gap |
| P2a | Complete | [Report](modernization-results/P2a.md): Debug raster/texture traces and invalidation tests pass; user reports Debian theme/icon checks passed; cross-screen/hotplug accepted by code review per user request, not physically tested | Keep the physical multi-display limitation visible in future validation summaries |
| P2b | Complete | [Report](modernization-results/P2b.md): equivalent smaller-edge geometry avoids raster/texture reload at DPR 1/1.5; changed request reloads; GCC/Clang Debug/Release tests pass; user reports visual checks pass; cross-screen path accepted by code review only | Retain per-window texture ownership; no whole-app A/B memory claim is made |
| P3a | Deferred | [Evaluation](modernization-results/P3a.md): Debian 13 Plasma 6.3.6 and Fedora 44 Plasma 6.7.5 headers both expose only a `void` geometry-publication API with no completion/generation acknowledgement; virtualized delegates prevent proving latest-generation readiness | Add and test an authoritative generation-aware publisher before changing the bounded eight-pass fallback |
| P3b | Complete | [Report](modernization-results/P3b.md): duplicate root edit-state poll removed; source-contract/QML tests, strict-warning builds and deep-QML lint pass in all GCC/Clang Debug/Release CI jobs in run 36741144541; user reports edit/hover interactions pass | Retain edit-state source contracts and the strict reviewed QML baseline |
| M6 | Deferred | [Evaluation](modernization-results/M6.md): the apparent shared `SchemeColors` source still depends on app-owned config/layout services; extracting it would add a target without reducing app dependencies | Revisit only if a measurable build or ownership benefit emerges |
| M7 | Complete | [Report](modernization-results/M7.md): local Nix verification passed 46/46 CTest, package build/install/uninstall, `.#default`, and GCC Debug preset; run 36668396118 passed all 20 jobs including NixOS install verification. Qt QML/Kirigami runtime paths select actual package outputs; mirror contracts pass. | Continue with the performance track after P0 and M5b runtime prerequisites are validated |
| P4 / P5 / P6 | Deferred | [Evaluation](modernization-results/P4-P6.md): P4 confirms two per-applet tracker models in one containment but no Release savings; P5 confirms the actual visible custom-background shadow path but has no effect cost metrics; P6 has no module-specific Release cost evidence and the installed QML-time probe is unusable | Reopen only with P4 paired Release query/work plus CPU/PSS evidence, P5 supported effect-on/off allocation/frame metrics, or P6 module-specific cold/warm Release evidence and an override-safe build/install prototype |

Use Not started, In progress, Pending validation, Complete or Deferred.
Do not mark a parent package complete when one slice is still pending.
The handoff template and reusable task prompt are in the implementation guide.

## Git and rollback

On main, editing/testing is allowed within the requested batch; committing and
pushing require separate explicit user approvals. Never silently switch
branches. Follow AGENTS.md for authorized work on other branches. This plan is
not commit, push, tag or release authorization.

If a slice fails a gate, preserve its evidence and fix that slice. Restore only
its own uncommitted edits when abandoning it; preserve unrelated user work.
Do not automatically reset, rewrite published history or revert commits.
