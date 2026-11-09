# Architecture, Quality and Performance Modernization Plan

Status: execution backlog; this refresh implements documentation only.
Assessment date: 2026-09-29.
Source baseline: `main` at `e3ef1ddf001aa032cffa62db21cfd97196174746` (1.2.51).
The local HEAD and remote `refs/heads/main` matched when checked on that date.

## Authority and how to use this plan

Preserve the current features, C++20, CMake 3.20, Qt 6.7, KF6 6.10, Plasma 6.3
and Wayland. This plan authorizes no implementation, release or remote mutation
by itself. The user's selected batch determines the implementation scope.

## Compatibility and validation environments

Keep the source-declared floors at CMake 3.20, Qt 6.7.0, KF6 6.10.0 and
Plasma 6.3.0. R7g raised Qt and KF6 from 6.6 and 6.0 (`072cbcb76`): libplasma
6.3.0 itself requires Qt 6.7.0 and KF6 6.10.0, so the older pair could never
build with Plasma 6.3.
Validate the CMake preset minimum at 3.20 and separately test the oldest
supported Debian desktop stack using Debian 13.7 packages: CMake 3.31.6-2,
Qt 6.8.2, KDE Frameworks 6.13 and the Plasma 6.3.6 desktop release. Debian's
`libplasma-dev` package is currently 6.3.5-1 while `plasma-workspace-dev` is
4:6.3.6-2. The reproducible container checks both package floors; local desktop
retest records the installed Plasma runtime version. Gentoo, Arch and Fedora CI
checks represent forward-compatibility coverage against their current stable
component versions; capture those versions from each run rather than pinning a
moving “latest”. No job builds exactly the declared Qt 6.7.0 and KF6 6.10.0:
Debian 13 is the oldest stack that is built and tested (R7g audit).

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
| P3a | Deferred | [Evaluation](modernization-results/P3a.md): Debian 13 Plasma 6.3.6 and Fedora 44 Plasma 6.7.5 headers both expose only a `void` geometry-publication API with no completion/generation acknowledgement; Fedora VM Top/Bottom GUI transitions worked, but a cache-cleared QML probe emitted no refresh-pass markers and could not establish whether the handler ran | Add and test an authoritative generation-aware publisher before changing the bounded eight-pass fallback; independently verify probe loading/log routing before any future handler measurement |
| P3b | Complete | [Report](modernization-results/P3b.md): duplicate root edit-state poll removed; source-contract/QML tests, strict-warning builds and deep-QML lint pass in all GCC/Clang Debug/Release CI jobs in run 36741144541; user reports edit/hover interactions pass | Retain edit-state source contracts and the strict reviewed QML baseline |
| M6 | Deferred | [Evaluation](modernization-results/M6.md): the apparent shared `SchemeColors` source still depends on app-owned config/layout services; extracting it would add a target without reducing app dependencies | Revisit only if a measurable build or ownership benefit emerges |
| M7 | Complete | [Report](modernization-results/M7.md): local Nix verification passed 46/46 CTest, package build/install/uninstall, `.#default`, and GCC Debug preset; run 36668396118 passed all 20 jobs including NixOS install verification. Qt QML/Kirigami runtime paths select actual package outputs; mirror contracts pass. | Continue with the performance track after P0 and M5b runtime prerequisites are validated |
| P4 / P5 / P6 | Deferred | [Evaluation](modernization-results/P4-P6.md): P4 confirms two per-applet tracker models in one containment but no Release savings; P5 confirms the actual visible custom-background shadow path but has no effect cost metrics; P6 still has no module-specific Release cost evidence (Debian qmltime is instantiation-only and below resolution for the minimal probe) | Reopen only with P4 paired Release query/work plus CPU/PSS evidence, P5 supported effect-on/off allocation/frame metrics, or P6 module-specific cold/warm Release evidence and an override-safe build/install prototype |

Use Not started, In progress, Pending validation, Complete or Deferred.
Do not mark a parent package complete when one slice is still pending.
The handoff template and reusable task prompt are in the implementation guide.

## Assessment refresh (R track)

The [2026-10-25 assessment](architecture-assessment-2026-10.md) re-reviewed
architecture, performance and code modernity at `v1.2.62`. R slices use the
common gates above and the handoff template in the implementation guide. They
do not reopen completed M, P, A or S batches without a concrete regression.

| ID | Dependencies | Deliverable and slice boundary | Acceptance |
| --- | --- | --- | --- |
| R1a | None | Event-driven logout monitor, bounded ksmserver queries, dead Wayland retry loop removed, close-on-exec self-pipe, `QPointer` for QML-owned items, event-driven original-icon fallback | Behavior tests on a private bus, zero-delta QML lint, idle wakeup A/B, logout confirm and cancel retest |
| R1b | None | Quit teardown use-after-free of the view settings factory | Backtrace, reproduction test, D-Bus and menu quit retest |
| R2a | R1a | `ecm_set_disabled_deprecation_versions()` at the declared Qt and KF6 floors (Qt 6.6 / KF 6.0 when R2a landed, Qt 6.7.0 / KF6 6.10.0 since R7g) | Zero warnings on GCC/Clang Debug/Release locally and on the Debian 13 floor in CI; any replaced API explained beside the change |
| R2b | R1a | Give the context-less lambda connections a context object with the right lifetime, then define `QT_NO_CONTEXTLESS_CONNECT` (enforced from Qt 6.7) | Per-site lifetime note in the handoff, builds and CTest, desktop smoke of the touched paths |
| R2c | M4a | clazy inventory on production code; gate only checks with zero findings, or one fixed family per slice, with a negative fixture | Requires clazy in the CI image or on a review host |
| R3 | M4a | Rule of Zero for `app/data`: default the copy/move members that only copy fields, keep custom equality, lock with clang-tidy scoped to `app/data` | Field-by-field equivalence review, `dataunittest` and dependent tests, tidy negative check |
| R4 | R2b | Settings views: replace `setContextProperty()` with initial or required properties where project QML owns the consumer | Settings, indicator and widget explorer desktop retest; no new QML diagnostics |
| R5 | P0 | Explain the idle CPU measured in the Fedora VM: idle frame rate, render timing, QML profile | Fedora VM session; no optimization patch without a measured cause |
| R6 | R2–R5 evidence | Strategic candidates, one at a time with recorded benefit: executable-backed `private.app` module; typed interfaces or a model for containment `LayoutManager`; per-component QML typing, then compiled QML (verify `plasma_add_applet()` at Plasma 6.3); parabolic scale computation in C++; behavioral tests replacing source contracts; split `main()` startup and shutdown; `target_sources()`, a shared static library instead of copied sources, and removal of compat shims below the floors | Separate design review per candidate |
| R7a | R1a, R4, R5 | Closing items: the R1a and R4 records corrected, `dev-env.sh` takes an unset `XDG_CONFIG_DIRS` from the user manager instead of inventing one, the Mageia `dnf install` retried inside the image build | Install-script test and packaging contract red/green; the retry loop run against a fake `dnf` |
| R7b | R2c | Snapshot iteration in `GenericLayout::syncLatteViewsToScreens()` (R2c follow-up 1) | Source contract red/green on GCC and Clang; hot-plug retest with clones |
| R7c | R2c | Current layouts drawn bold again in the Move To Layout menu (R2c follow-up 7, finding F5) | History shows the removal was not a decision; unit tests red/green; Multiple Layouts desktop comparison |
| R7d | R2c | `NOTIFY` for `UniversalSettings::screens`, replacing its reviewed clazy exception (R2c follow-up 4) | Source contract red/green; moc notify index; Qt emission-order probe; QML consumer analysis |
| R7e | R2b | Declared `Component`s instead of the four `Qt.createQmlObject()` calls | Contracts red/green; qmllint delta 0; desktop checks of the start-up retry and the task context menu |
| R7f | R2c | `main()` split into named functions in `app/main.cpp`, behavior unchanged | ctest; logout simulation; a contract that keeps `main()` short |
| R7g | R2a | Audit of the declared Qt, KF and Plasma floors against what Plasma 6.3 itself requires | Report only: changing the floors is the user's policy decision |
| R6 designs | R2–R5 | Design studies without code: `R6-layoutmanager-design`, `R6-corona-design`, `R6-privateapp-design` | Review by the user before any slice starts |
| R8a | None | Inventory of `autotests/sourcecontracttest.cpp` by class (keep, behavioural candidate, duplicate, stale) and at most five conversions into behavioural tests that call production code; no production change | Every contract function classified; each conversion fails under a production mutation; the contract count stays above 170; GCC/Clang Debug with 0 warnings and CTest passing |
| R8b | R4 | QML typing of one component, starting with `containment/package/contents/ui/debugger/DebugWindow.qml` (201 unqualified diagnostics), behavior unchanged | Deep qmllint before and after on the same host: no added identities, fewer for the file; desktop check of the debug window |
| R8c | R2c | clang-tidy check inventory on production code, report only and no gate change (CI runs clang-tidy 18; the local `latte-r2c-clazy:noble` image has the same 18.1.3, see the record) | Report with counts per check and a proposed gate order |
| R8d | R1a | Long-run idle memory growth: a 150-minute nested-session run and a read-only desktop series, verdict rule fixed in advance; tools and data committed, no source change | Record with data, verdict and limits; the analysis reproduces from the committed data |
| R8e | R7g | CMake structure: no `configure_file()` copies of production sources, the app sources through `target_sources()`, and a list-only inventory of `compat/` | Executable translation units, flags and exported symbols identical to a frozen baseline; GCC/Clang Debug with 0 warnings and CTest passing; Release presets after the merge, CI gates on the push |
| R8f | R8a | The 14 contracts that the R8a inventory classed D, each checked against its twin with a scratch mutation of the production code: a contract part is removed only when its twin runs that code and fails; the others are kept and reclassified | Every removed part has a failing mutation and every kept row a recorded reason; the contract count stays above 170; GCC/Clang Debug with 0 warnings and CTest passing |
| R8g | R8f | Behavioural case for `Package::pathChanged()` (R8a row 107): a foreign shell package gets the Latte shell as fallback and Latte's own package none; the contract that only checks the shell id string is removed; tests only | Scratch mutations of the gate, the fallback id and both fallback writes fail the case; the contract count stays above 170; GCC/Clang Debug with 0 warnings and CTest passing |
| R8h | R8f | `ContainmentInterface` uses `Latte::PluginId` for the separator and splitter ids instead of its own copies (R8a row 105); the contract that read the copies becomes a no-copy contract; behaviour unchanged | The contract fails under scratch reintroductions of a copy; GCC/Clang Debug and Release with 0 warnings and CTest passing; the clazy gate; a desktop check of the boundary separators that the task menu adds and removes |

| Batch | Status | Evidence | Next action |
| --- | --- | --- | --- |
| R1a | Complete | [Report](modernization-results/R1a.md): commit `db966ccd6` | None |
| R1b | Complete | [Report](modernization-results/R1b.md): commit `a2bb619ae` | None |
| R2a | Complete | [Report](modernization-results/R2a.md): commit `a087f3511`; GCC 16.2.1 and Clang 22.1.8 Debug/Release with 0 warnings and 56/56 CTest each; CI run 40180566388 passed the Release jobs and the Debian 13 build | None |
| R2b | Complete | [Report](modernization-results/R2b.md): commits `484a8d59b` and `56c34148c` (alternatives dialog callbacks); 24 connections given a context object, 0 remain; macro enforced on Qt 6.11.2 locally and on Qt 6.8.2 in the Debian 13 CI build. Three desktop rounds on 2026-10-25 and 2026-10-26 passed every touched path, including the alternatives dialog, the edit-mode menu and the indicator removal confirmation, after merges `f634322d4` and `12f063d93` | None |
| R2c | Complete | [Report](modernization-results/R2c.md): commit `1feb770dc`, merged in `8d1fe1568`; the level0/level1 inventory in a container that mirrors the CI build found 570 findings from 19 checks; the Clang Debug job gates the zero-finding checks (31 at the inventory, 43 since fix batches 1 and 2, 49 since fix batch 3), each proven by a fixture; CI runs 40345972905 at `09b030e61`, 40683601580 at `14d705a5d` and 41452548625 at `6b51b43a6` ran the clazy step and passed | All fix slices are Complete (batches 1 to 3); the follow-ups the slices and their validation found are listed in [R2c](modernization-results/R2c.md#follow-ups) |
| R2c-auto-unexpected-qstringbuilder | Complete | [Report](modernization-results/R2c-auto-unexpected-qstringbuilder.md): commit `eb83f088c`; a `const auto` holding a `QStringBuilder` of temporaries in `BackgroundCache` fixed and the check gated; CI run 40345972905 passed | None |
| R2c-qfileinfo-exists | Complete | [Report](modernization-results/R2c-qfileinfo-exists.md): commit `4a08e472e`, merged in `1dd485bc1`; 35 `QFileInfo(path).exists()` temporaries replaced by the static `QFileInfo::exists()`; the check is gated. Builds, tests, the gate and the desktop smoke passed on `main` ([batch 1](modernization-results/R2c.md#fix-batch-1)); CI run 40683601580 at `14d705a5d` passed all 23 jobs, including the 43-check clazy gate ([CI validation](modernization-results/R2c.md#ci-validation)) | None |
| R2c-range-loop-reference | Complete | [Report](modernization-results/R2c-range-loop-reference.md): commit `e9bf4acf5`, merged in `1dd485bc1`; 15 range loops bind their element by `const &` instead of copying it; the check is gated. Builds, tests, the gate and the desktop smoke passed on `main` ([batch 1](modernization-results/R2c.md#fix-batch-1)); CI run 40683601580 at `14d705a5d` passed all 23 jobs, including the 43-check clazy gate ([CI validation](modernization-results/R2c.md#ci-validation)) | None |
| R2c-no-module-include | Complete | [Report](modernization-results/R2c-no-module-include.md): commit `1c87da632`, merged in `1dd485bc1`; 14 files include the Qt headers they use instead of `<QtQml>` or `<QtDBus>`; also built on the Debian 13 floor, Qt 6.8.2; the check is gated. Builds, tests, the gate and the desktop smoke passed on `main` ([batch 1](modernization-results/R2c.md#fix-batch-1)); CI run 40683601580 at `14d705a5d` passed all 23 jobs, including the 43-check clazy gate ([CI validation](modernization-results/R2c.md#ci-validation)) | None |
| R2c-empty-qstringliteral | Complete | [Report](modernization-results/R2c-empty-qstringliteral.md): commit `cd966836e`, merged in `fc8b803d0`; the actions editor separator row takes `QIcon()` instead of a theme lookup of an empty name; the check is gated. Builds, tests, the gate and the desktop smoke passed on `main` ([batch 1](modernization-results/R2c.md#fix-batch-1)); CI run 40683601580 at `14d705a5d` passed all 23 jobs, including the 43-check clazy gate ([CI validation](modernization-results/R2c.md#ci-validation)) | None |
| R2c-use-static-qregularexpression | Complete | [Report](modernization-results/R2c-use-static-qregularexpression.md): commit `4bafbe6d9`, merged in `fc8b803d0`; six constant patterns in `knscompat.cpp` and `tasktools.cpp` compiled once; the check is gated. Builds, tests, the gate and the desktop smoke passed on `main` ([batch 1](modernization-results/R2c.md#fix-batch-1)); CI run 40683601580 at `14d705a5d` passed all 23 jobs, including the 43-check clazy gate ([CI validation](modernization-results/R2c.md#ci-validation)) | None |
| R2c-connect-not-normalized | Complete | [Report](modernization-results/R2c-connect-not-normalized.md): commit `a4cfaa80c`, merged in `fc8b803d0`; ten `Q_ARG` types spelled in normalized form, each proven equal to `QMetaObject::normalizedSignature()` of the old one; the check is gated. Builds, tests, the gate and the desktop smoke passed on `main` ([batch 1](modernization-results/R2c.md#fix-batch-1)); CI run 40683601580 at `14d705a5d` passed all 23 jobs, including the 43-check clazy gate ([CI validation](modernization-results/R2c.md#ci-validation)) | None |
| R2c-unused-non-trivial-variable | Complete | [Report](modernization-results/R2c-unused-non-trivial-variable.md): commit `cec126a31`, merged in `181b7f871`; 14 locals that nothing read removed after checking that their initializers have no side effect; lost intent recorded; the check is gated. Builds, tests, the gate, the Debian 13 build and the host smoke passed on `main` ([batch 2](modernization-results/R2c.md#fix-batch-2)); CI run 40683601580 at `14d705a5d` passed all 23 jobs, including the 43-check clazy gate ([CI validation](modernization-results/R2c.md#ci-validation)) | None |
| R2c-detaching-temporary | Complete | [Report](modernization-results/R2c-detaching-temporary.md): commit `0ca056cc7`, merged in `181b7f871`; 43 `operator[]` and `first()` calls on returned lists became `at()` and `constFirst()`; the check is gated. Builds, tests, the gate, the Debian 13 build and the host smoke passed on `main` ([batch 2](modernization-results/R2c.md#fix-batch-2)); CI run 40683601580 at `14d705a5d` passed all 23 jobs, including the 43-check clazy gate ([CI validation](modernization-results/R2c.md#ci-validation)) | None |
| R2c-container-anti-pattern | Complete | [Report](modernization-results/R2c-container-anti-pattern.md): commit `17248e996`, merged in `181b7f871`; 38 temporary `keys()`/`values()` containers replaced by direct iteration or lookups, with a documented snapshot where a loop changes its container; the check is gated. Builds, tests, the gate, the Debian 13 build and the host smoke passed on `main` ([batch 2](modernization-results/R2c.md#fix-batch-2)); CI run 40683601580 at `14d705a5d` passed all 23 jobs, including the 43-check clazy gate ([CI validation](modernization-results/R2c.md#ci-validation)) | None |
| R2c-const-signal-or-slot | Complete | [Report](modernization-results/R2c-const-signal-or-slot.md): commit `a315abf65`, merged in `382e55cd2`; ten const getters declared `Q_INVOKABLE` instead of as slots, keeping their access; the check is gated. Builds, tests, the gate, the Debian 13 build and the host smoke passed on `main` ([batch 2](modernization-results/R2c.md#fix-batch-2)); CI run 40683601580 at `14d705a5d` passed all 23 jobs, including the 43-check clazy gate ([CI validation](modernization-results/R2c.md#ci-validation)) | None |
| R2c-incorrect-emit | Complete | [Report](modernization-results/R2c-incorrect-emit.md): commit `ae9fe447b`, merged in `382e55cd2`; three constructor emits that nothing could receive removed and a missing `Q_EMIT` added; the check is gated. Builds, tests, the gate, the Debian 13 build and the host smoke passed on `main` ([batch 2](modernization-results/R2c.md#fix-batch-2)); CI run 40683601580 at `14d705a5d` passed all 23 jobs, including the 43-check clazy gate ([CI validation](modernization-results/R2c.md#ci-validation)) | None |
| R2c-fully-qualified-moc-types | Complete | [Report](modernization-results/R2c-fully-qualified-moc-types.md): commit `a50614ff2`, merged in `382e55cd2`; 21 signal and slot parameter types spelled fully qualified for moc, with every name-based consumer checked; the check is gated. Builds, tests, the gate, the Debian 13 build and the host smoke passed on `main` ([batch 2](modernization-results/R2c.md#fix-batch-2)); CI run 40683601580 at `14d705a5d` passed all 23 jobs, including the 43-check clazy gate ([CI validation](modernization-results/R2c.md#ci-validation)) | None |
| R2c-overloaded-signal | Complete | [Report](modernization-results/R2c-overloaded-signal.md): commit `b30ae731a`, merged in `4e403af80`; three model signals renamed so they no longer overload `QAbstractItemModel` signals; eight QML-facing NOTIFY signals keep their names under documented `clazy:exclude` exceptions; the check is gated. Builds, tests, the 49-check gate and the Debian 13 build passed on the integration branch and the host smoke passed on `6b51b43a6` ([batch 3](modernization-results/R2c.md#fix-batch-3)); CI run 41452548625 at `6b51b43a6` passed all 23 jobs, including the 49-check clazy gate ([CI validation](modernization-results/R2c.md#ci-validation)) | None |
| R2c-overridden-signal | Complete | [Report](modernization-results/R2c-overridden-signal.md): commit `67180cbda`, merged in `4e403af80`; the `IconItem` enabled slot renamed `onEnabledChanged()`; `View::screenGeometryChanged` kept under a documented exception; the check is gated. Builds, tests, the 49-check gate and the Debian 13 build passed on the integration branch and the host smoke passed on `6b51b43a6` ([batch 3](modernization-results/R2c.md#fix-batch-3)); CI run 41452548625 at `6b51b43a6` passed all 23 jobs, including the 49-check clazy gate ([CI validation](modernization-results/R2c.md#ci-validation)) | None |
| R2c-qproperty-without-notify | Complete | [Report](modernization-results/R2c-qproperty-without-notify.md): commit `b3ac2960c`, merged in `4e403af80`; `UniversalSettings::screens` kept without NOTIFY under a documented exception (screen hotplug is a follow-up); the check is gated. Builds, tests, the 49-check gate and the Debian 13 build passed on the integration branch and the host smoke passed on `6b51b43a6` ([batch 3](modernization-results/R2c.md#fix-batch-3)); CI run 41452548625 at `6b51b43a6` passed all 23 jobs, including the 49-check clazy gate ([CI validation](modernization-results/R2c.md#ci-validation)) | None |
| R2c-connect-3arg-lambda | Complete | [Report](modernization-results/R2c-connect-3arg-lambda.md): commit `085fbd190`, merged in `3a180839b`; seven `QTimer::singleShot` callbacks got a context object that owns their lifetime and the unused `Synchronizer::pauseLayout()` was deleted; the check is gated. Builds, tests, the 49-check gate and the Debian 13 build passed on the integration branch and the host smoke passed on `6b51b43a6` ([batch 3](modernization-results/R2c.md#fix-batch-3)); CI run 41452548625 at `6b51b43a6` passed all 23 jobs, including the 49-check clazy gate ([CI validation](modernization-results/R2c.md#ci-validation)) | None |
| R2c-non-pod-global-static | Complete | [Report](modernization-results/R2c-non-pod-global-static.md): commit `50ba3074b`, merged in `3a180839b`; 31 namespace-scope `QString` and `QStringList` constants became function-local-static accessors; the check is gated. Builds, tests, the 49-check gate and the Debian 13 build passed on the integration branch and the host smoke passed on `6b51b43a6` ([batch 3](modernization-results/R2c.md#fix-batch-3)); CI run 41452548625 at `6b51b43a6` passed all 23 jobs, including the 49-check clazy gate ([CI validation](modernization-results/R2c.md#ci-validation)) | None |
| R2c-range-loop-detach | Complete | [Report](modernization-results/R2c-range-loop-detach.md): commits `a18a905bc`, `d8b07b428`, `6f42e5ed2`, `8a998f1b8`, `4116888bb`, merged in `c0f64b9ee`, `e62767af2`, `98b00c657`, `b7fc199db`; 107 loops iterate a `std::as_const` view, 164 loops over a by-value result hold it in a const local, one loop uses `removeIf`, two keep their live iteration under documented exceptions; the check is gated. Builds, tests, the 49-check gate and the Debian 13 build passed on the integration branch and the host smoke passed on `6b51b43a6` ([batch 3](modernization-results/R2c.md#fix-batch-3)); CI run 41452548625 at `6b51b43a6` passed all 23 jobs, including the 49-check clazy gate ([CI validation](modernization-results/R2c.md#ci-validation)) | None |
| R3 | Complete | [Report](modernization-results/R3.md): refactor commit `478ba01ee`, verified field by field; commit `3e0757397` strengthens the source contract, which rejects every re-added copy or move member shape in the red/green check on GCC and Clang. A source contract replaces the planned tidy lock; the report records why | None |
| R4 | Complete | [Report](modernization-results/R4.md): commit `1cef62616`, merged in `5bac5f723`; the settings windows publish their inputs as root properties, and only the context properties that third-party QML reads remain; desktop retest passed | None |
| R5 | Complete | [Report](modernization-results/R5.md): the VM's 20 % was a launch artifact. Release main idles at 0.000–0.017 % of one core in 13 windows (also hidden, software rendering, the VM's layout) with zero frames by three counters; P0's own baseline built today idles at 0.55 % with the 16 + 8 wakeups per second that R1a removed (0.52–0.55 % → 0.000 % in the VM). An SSH launch that sources `dev-env.sh` gives the dock a shorter `XDG_CONFIG_DIRS` than the session and starts a KSycoca rewrite loop: same binary 18.8 %, 117–128 wakeups per second, kwin_wayland 26 %, plasmashell 22 %; the session launcher, or the session value of that one variable, gives 0.00 %. Under such churn the hosted `kickerdash` applet is 80–88 % of the dock's cost (about 60 ms per change event) and the dock's own code idles. No dock patch justified | None for R5. Candidates only with a measured benefit: a `dev-env.sh` that leaves `XDG_CONFIG_DIRS` alone when unset; bounding hosted-applet refresh under KSycoca churn |
| R6 | Deferred | — | Reopen one candidate with a measured benefit |
| R7b | Pending validation | [Report](modernization-results/R7b.md): merge `2d07ff019` (fix `1df198b38`, contract `72b624e28`). The merged `main` (the four merges together) built with GCC 16.2.1 and Clang 22.1.8 in the Debug and Release presets with 0 compiler warnings and passed 61/61 CTest in each. The defect is established from the code; no log or report shows a failure | User: the hot-plug retest with clones (report, section Desktop retest) |
| R7c | Pending validation | [Report](modernization-results/R7c.md): merge `32de04beb` (fix `21f715767`). The history shows that commit `69ec3d937` dropped the bold marking as a side effect of the widget conversion; running layouts on the current activity are bold again, the `[Current]` suffix is not restored. The merged `main` (the four merges together) built with GCC 16.2.1 and Clang 22.1.8 in the Debug and Release presets with 0 compiler warnings and passed 61/61 CTest in each | User: compare the bold items in Move Dock To Layout with Preferences > Layouts in Multiple Layouts mode, and keep or revert the marking |
| R7d | Pending validation | [Report](modernization-results/R7d.md): merge `5c0d56c71` (fix `1c72ce577`). `screensChanged()` is the NOTIFY and also follows `primaryScreenChanged`; the clazy exclusion is gone; the emission order was probed on Qt 6.11.2 only. The merged `main` (the four merges together) built with GCC 16.2.1 and Clang 22.1.8 in the Debug and Release presets with 0 compiler warnings and passed 61/61 CTest in each | User (optional): Preferences > Behavior with two screens, plug and unplug one; the CI clazy gate runs on the push |
| R7e | Pending validation | [Report](modernization-results/R7e.md): merge `c90a7838d` (refactor `52b2c17d5`). The deep qmllint delta is 0 added and 0 removed on the author's host; the CI QML lint job runs on the push. The merged `main` (the four merges together) built with GCC 16.2.1 and Clang 22.1.8 in the Debug and Release presets with 0 compiler warnings and passed 61/61 CTest in each | User: dock start with several applets (the retry path only runs when an applet is not ready), and a right-click on a task: items, separators and elided long names as before |
| R7a | Pending validation | [Report](modernization-results/R7a.md): merge `72d186d64` (R1a note `637e1b318`, R4 update `0453ba35e`, `dev-env.sh` `d2d6cfe78`, Mageia retry `82a8bf58d`). The install-script test and the packaging contract fail on the old text; the retry loop was run against a fake `dnf` by the author and again by the main session (first try, third try, always failing); `autotests/installscriptbehavior.py` passes on the merged tree. The merged `main` built with GCC 16.2.1 and Clang 22.1.8 in the Debug and Release presets with 0 compiler warnings and passed 61/61 CTest in each | `dev-env.sh` was not launched in a VM session and the new rule has not been measured there; the Mageia retry only shows when a Cauldron mirror misses a package (CI); whether dnf reuses downloaded packages between attempts is unverified |
| R7f | Pending validation | [Report](modernization-results/R7f.md): merge `97af590a1` (twelve commits). `main()` went from 631 to 187 lines with 18 named helpers defined after it, and a source contract keeps it at 200 lines or fewer. The author ran the logout simulation (nested KWin) and five mutation checks; the main session compared `--help`, `-v` and the three `--available-*` listings with the base binary: identical, `--help` differs only in the program path. The merged `main` built with GCC 16.2.1 and Clang 22.1.8 in the Debug and Release presets with 0 compiler warnings and passed 61/61 CTest in each, including `privateapphostsmoketest` | User: start-up in the real session, `--replace`, `--import-full`, the autostart toggles and a real logout (only simulated) |
| R7g | Complete | [Report](modernization-results/R7g.md): audit merge `d4d18b85a`; option A applied in `072cbcb76` (merge `23cc38eb4`). Qt 6.6 with KF6 6.0 could not be built with Plasma 6.3: libplasma 6.3.0 needs Qt 6.7.0 and KF6 6.10.0, so the declared floors are now Qt 6.7.0, KF6 6.10.0 and Plasma 6.3.0. KWayland is looked up against `PLASMA_MIN_VERSION` (it is released with Plasma), PlasmaWaylandProtocols moved from 1.6 to 1.10.0 (`kde-output-order-v1.xml`), and the KF6 6.8 need of the preview helper (`KF6::I18nQml`) is covered. The R2a deprecation gate follows the raised floors. CI run 42142374314 for `23cc38eb4` passed all 23 jobs, and a local build of that merge with GCC 16.2.1 and Clang 22.1.8 in the Debug and Release presets had 0 compiler warnings and passed 61/61 CTest in each. Only Debian 13 (Qt 6.8.2, KF 6.13) is built at a distribution floor; no job builds exactly Qt 6.7.0 and KF6 6.10.0 | None |
| R6 designs | Complete (documents) | Merges `479157e44`, `ca1e56563` and `31207fa35`; no code. [LayoutManager](modernization-results/R6-layoutmanager-design.md): keep `LayoutManager` in the plugin behind a typed `LayoutManagerApi`; first slice makes the name contracts fail loudly. [Corona](modernization-results/R6-corona-design.md): decompose by consumer family; first slice is the available-screen-area reduction as a pure header. [private.app](modernization-results/R6-privateapp-design.md): conditional go as one coordinated slice; four mandatory changes were found on a scratch prototype that is not committed. The counts in these records are `git grep` inventories; two of them mention raw lists or a patch kept outside the repository | User: pick at most one candidate; R6 stays Deferred until one has an accepted design |
| R8a | Complete | [Report](modernization-results/R8a.md) and [inventory](modernization-results/R8a-inventory.md): merge `2094761b5` (eight commits, tests and documents only, no production change). All 229 functions of `sourcecontracttest` are classified: K 113, B1 6, B2 45, B3 45, D 14, S 6. Five B1 cases (`servicesFromCmdLine`, three `ScreenPool` cases and the indicator `Factory`) now call production code, each proven by a scratch mutation that fails the new test; the main session re-ran the `screenForId` mutation independently and it failed as recorded. The five matching contracts are removed, so the file has 224 functions, still above the 170 that AGENTS.md rule 6 expects. The merged `main` built with GCC 16.2.1 and Clang 22.1.8 in the Debug and Release presets with 0 compiler warnings and passed 61/61 CTest in each; the incremental rebuild recompiled only the five touched test units | None for the D rows: [R8f](modernization-results/R8f.md) settled them. The attached-screen branches of `ScreenPool` and a named primary-screen sentinel still need a live desktop |
| R8b | Complete | [Report](modernization-results/R8b.md): merge `76ae1247b` (two commits, QML and documents). `DebugWindow.qml` takes ten required inputs from `main.qml` (eight typed with their ability and item types, two `var` with the reason beside them) instead of thirteen context names; `main.qml` sets them with `Loader.setSource()` because the file is not `ComponentBehavior: Bound`. Deep qmllint on the author's host (6.11.3, 241 files, complete): 7328 diagnostics before, 7127 after, no identity added, the file's 201 unqualified accesses removed, `main.qml` unchanged; the reviewed baseline drops that entry. Syntax lint and the user-mode install pass. The user checked the windows on the desktop (`--with-window`): every row live, "Contains Mouse" and "Edit Mode" follow the dock. The complete log has no error and one warning, from KIO, about a variable that the launching shell (not the Plasma session) put in the dock's environment; the dock quit cleanly and no core dump appeared. The merged `main` has the linted QML unchanged and no C++ change (nothing recompiled, clazy not affected); it passed 61/61 CTest in the four presets with 0 compiler warnings. Build run 42557674581 on `4741deeab` passed all 23 jobs, among them the deep QML lint against the reviewed 6.11.1 baseline in the four presets | None |
| R8c | Complete | [Report](modernization-results/R8c.md): merge `54974f121` (three commits, documents, tools and data only; no gate, CI or source change). `R8c-tools/tidy_inventory.py` counts clang-tidy findings per check over the clazy gate's 155 production sources, bypassing `.clang-tidy`; its ten tests pass with clang-tidy 18 and 22. CI's clang-tidy 18.1.3 (run in the local CI container): 410 checks, 12317 findings from 91 checks, 319 checks without findings; clang-tidy 22.1.8 on this host: 470 checks, 14358 findings from 110. The gate's two checks find nothing in any production source. Three CI-version cost runs at 4 jobs price the candidates: the two checks over all sources 213 s, 46 more zero-finding checks about 36 s more, all non-analyzer checks 1842 s, the analyzer 279 s. The record proposes the gate order and reviews the defect-family findings: a copy-paste defect in `BackgroundCache::areaIsBusy()` and a skipped `QTableView::selectionChanged()` in `ViewsTableView` are real, the analyzer's leak and pure-virtual reports are not | None for R8c. The gate order's first step (the two checks over all production sources) and the two defects are candidates for their own slices |
| R8c-fix | Complete | [Report](modernization-results/R8c-fixes.md): branch `claude/strange-pare-756530`, merged into `main` after the desktop check. Fixes the two R8c defects: the `inBounds` bound in `backgroundcache.cpp` (`misc-redundant-expression`) and the base call in `viewstableview.cpp` (`bugprone-parent-virtual-call`); `view.cpp:1800` stays as documented. One behavioural test each, each failing under its mutation. Matrix (four presets: 0 compiler warnings, CTest 61/61), clazy gate and clang-tidy re-check passed; merged tree re-run passed (four presets, CTest 61/61, R8i gate); desktop check passed on 2026-11-10 (nine steps, user report; Debug log clean) | None. The `view.cpp:1800` call stays as documented; a gate on `bugprone-parent-virtual-call` would need a suppression with its reason beside that call |
| R8d | Complete (measurement) | [Report](modernization-results/R8d.md): merge `6f948ad3b` (six commits: tools, data and record, no production change). The analysis, its 14 unit tests and an independent slope computation by the main session reproduce the record. Nested Release dock: no growth over 150 minutes, but it idled at zero CPU with 71 MiB anonymous memory against 417 MiB on the desktop, so it is weak evidence. Desktop dock (read-only, live session): +2.45 MiB in three steps after warm-up, slope 0.69 MiB/h (95 % CI 0.45 to 0.95): "no growth" by the rule fixed in advance, but its second half is above 1 MiB/h, so it is a watch item. The session also started a 12-hour read-only desktop series (ends about 00:50 on 2026-11-07) whose data will be committed to its branch | The first 12-hour series was written to `/tmp` and lost when the host rebooted on 2026-11-09, before its data were committed. It runs again since 11:58 on 2026-11-09 in `/data/tmp/r8d-desktop-12h` (transient user unit `r8d-desktop-12h`, the same tool and 30 s interval) against the packaged dock (1.3.0, pid 74810) that its autostart unit started at 11:57, and ends about 23:59. A dock restart, logout or reboot before then ends it early, so no desktop check until then. Then analyze it with the record's command and rule and commit the data; if the steps keep recurring, run the next tool named in the record (heaptrack or massif) |
| R8e | Complete | [Report](modernization-results/R8e.md): merge `60ca9429c` (five commits). The contextmenu plugin compiles `app/settings/generic/generictools.cpp` directly, the 29 `PARENT_SCOPE` lists became `target_sources()`, and `compat/` is inventoried: of 26 headers 19 are pure forwarders, 3 replacement implementations and 4 unused, nothing removed. Against the frozen baseline the executable's 142 translation units (also in order), its 5628 exported symbols and the plugin's 6 are identical; the main session reproduced that independently. The merged `main` built with GCC 16.2.1 and Clang 22.1.8 in the Debug and Release presets with 0 compiler warnings and passed 61/61 CTest in each; the incremental rebuild recompiled only the four contextmenu plugin units. Follow-up merge `d8d76944b` (fix `a45d5233b`, see the record): reused build trees kept the `configure_file()` copies that Part A stopped writing, and `CMAKE_INCLUDE_CURRENT_DIR` put them first on the plugin's include path, so trees configured before 50ba3074b, including the default `install.sh --user` tree, failed to compile `menu.cpp`; CMake now removes the copies. Red/green on one tree; on the merged `main` the configure removed them from the four preset trees, which then built with 0 compiler warnings and passed 61/61 CTest each. After the merge, Build run 42545506041 on `e92a534a2` passed all 23 jobs, among them the clazy and clang-tidy gates, the deep QML lint, Debian 13 and the Nix check; in the R8b desktop check the user right-clicked a dock that had loaded this plugin (user-mode install, same build ID as its fresh build tree) and the context menu opened, with no error in the log and no new core dump | None |
| R8f | Complete | [Report](modernization-results/R8f.md): merge `a11a345c4` (three commits, tests and documents only). Of the 14 D rows, one contract is removed, four are trimmed to the parts no test covers and nine are kept and reclassified, because their twin builds no production source or tests a helper instead of the code the contract reads. Twelve scratch mutations behaved as predicted: every removed part's twin fails, and two kept rows' twins pass under a mutation of their code. The shutdown monitor test now checks that the ending flag is set before the views hide. Contracts 224 to 223, assertion lines 1922 to 1906. The merged `main` (with the R8e follow-up) built in the four presets with 0 compiler warnings and passed 61/61 CTest in each | None. The report's two candidates became R8g (a behavioural case for `Package::pathChanged()`) and R8h (`ContainmentInterface` using `Latte::PluginId`) |
| R8g | Complete | [Report](modernization-results/R8g.md): merge `4612c6a0d` (two commits, tests and documents only). `packageunittest` now checks the fallback that `Package::pathChanged()` sets: a foreign shell package gets `org.kde.latte.shell` from the source tree (the XDG data directories are redirected, so an installed Latte cannot answer), and pointing the package back at Latte's shell clears it; the case skips where libplasma's `Plasma/Shell` structure is not on the plugin path (the Nix check phase). Five scratch mutations of the gate, the fallback id and both fallback writes fail it, R8f's M106 among them. The contract that only checked the shell id string is removed: 223 to 222 contracts. GCC/Clang Debug on the branch, then the merged `main` in the four presets, with 0 compiler warnings and 61/61 CTest in each. Build run 42561813597 on `705a7b94c` passed all 23 jobs | None. Candidate in the report: the indicator package structure's embedded plugin Id |
| R8h | In progress | Branch `claude/r8h-containment-plugin-ids` (`0979a2a72`), not merged; its record is on the branch. `ContainmentInterface` takes its 18 separator and splitter uses from `Latte::PluginId` instead of copies left over from before `pluginids.h`; the values and the helper are identical. The contract that required the copies is now a no-copy contract and fails under four scratch reintroductions of a copy. GCC Debug with 0 compiler warnings and 61/61 CTest; the clazy gate in the local container: 49 fixtures rejected, no finding in 155 production sources | User: the boundary separator check in the record, after the R8d series ends (about 23:59 on 2026-11-09); then log review, merge, the four presets and the push |
| R8i | Pending validation | [Report](modernization-results/R8i.md): branch `claude/distracted-chatterjee-722eed` (`3baa2252c`, `85f34625b` and the record commit), merged into `main`. The clang-tidy gate analyzes the clazy gate's 155 production sources with its two named checks, each proven by a fixture under the production flags (R8c's gate-order step 1). CI's clang-tidy 18.1.3 in the local CI image: 0 findings, 200 s at `--jobs 4` (R8c's two-check run: 213 s); clang-tidy 22.1.8: 0 findings. GCC and Clang Debug: 0 compiler warnings; CTest 61/61 on the host, 60/60 in the container (no `appstreamcli` there). Three scratch edits make the gate fail as predicted | CI: the Build run on the merge commit; its Clang Debug job's clang-tidy step moves the slice to Complete |

## Git and rollback

On main, editing/testing is allowed within the requested batch; committing and
pushing require separate explicit user approvals. Never silently switch
branches. Follow AGENTS.md for authorized work on other branches. This plan is
not commit, push, tag or release authorization.

If a slice fails a gate, preserve its evidence and fix that slice. Restore only
its own uncommitted edits when abandoning it; preserve unrelated user work.
Do not automatically reset, rewrite published history or revert commits.
