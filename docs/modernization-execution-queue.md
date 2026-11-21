# Modernization execution queue

Date: 2026-11-13. Planning source: `main` at `7c6c88845`.
Reconciled source: `796344495` after the user's appearance, vertical-gap,
preset-restore and translation commits; current validation is in T4a.
This queue decomposes the remaining work; it does not mark proposed checks as run.
The existing plan ledgers and batch reports remain the completion authority.

## Responsibility and integration

The coordinating agent owns scope, dependencies, integration and review of the
final diff. Worker agents use `gpt-6-luna` with `medium` reasoning. Workers do not
commit, push, switch branches or change desktop state independently. On `main`,
commit and push require the user's explicit authorization under AGENTS.md.

Assign disjoint files when workers share a checkout. Build and desktop jobs run
sequentially; read-only investigations can run concurrently. A worker returns
changed files, exact commands and results, limitations and its next action. The
coordinator reads the actual diff and checks the evidence before accepting it;
a worker summary alone is not acceptance. Preserve concurrent external changes.

## First wave

| Task | Deliverable and boundary | Dependency | Acceptance |
| --- | --- | --- | --- |
| Q0 | Synchronize stale next actions in the main plan, QML backlog and R2c report; preserve historical observations | None | Evidence links support each status; no new validation claims; whitespace and local links checked |
| A4-1 | Design an app-runtime test fixture that reaches the existing screen-mapping production methods | Existing A4/R8a/R7 evidence | Show how to construct and isolate the actual objects without copying their algorithm or changing mapping behavior; review fixture ownership and cost before implementation |
| A4-2 | Implement focused mapping behavior tests using the accepted fixture | A4-1 | Pending update precedence (including primary), connector-to-stable-ID lookup, missing mapping fallback and nested containment; relevant mutations fail; GCC/Clang builds and tests pass |
| A4-3 | Close remaining desktop mapping coverage | A4-2 | Record primary-not-zero and nested containment observations; reuse R7 hotplug evidence where applicable, and leave unavailable scenarios explicitly pending |
| T1 | Promote the first 15 cost-ranked Band A candidates, only after individual fixture proof | Current R8i gate | Every promoted check fires as an error under production flags in clang-tidy 18 and 22, has no current production finding, and is explicitly named; measure gate cost; no unrelated production fixes |
| Q1 | Integrate results and audit the combined diff | Each completed slice | Verify scope, test registration, failure handling, current evidence and status; record skipped checks and exact next action |

A4-2 is not a simple extension of `corecomponentstest`: that target currently
compiles ScreenPool, not Corona, GenericLayout or Synchronizer. A4-1 must settle
this integration boundary first. R8a covers detached mappings and primary
fallback; R7 covers two-output hotplug. Neither proves the entire mapping chain.
Keep A4 Pending validation until its remaining acceptance is satisfied.

T1 candidate names, in the recorded cost order:

1. `bugprone-empty-catch`
2. `misc-throw-by-value-catch-by-reference`
3. `performance-implicit-conversion-in-loop`
4. `performance-for-range-copy`
5. `bugprone-terminating-continue`
6. `performance-inefficient-vector-operation`
7. `bugprone-lambda-function-name`
8. `bugprone-too-small-loop-variable`
9. `performance-trivially-destructible`
10. `bugprone-suspicious-missing-comma`
11. `misc-misleading-bidirectional`
12. `bugprone-unique-ptr-array-mismatch`
13. `bugprone-shared-ptr-array-mismatch`
14. `bugprone-sizeof-container`
15. `bugprone-string-integer-assignment`

This selection comes from the [recorded candidate table](modernization-results/R8c-data/ci-gate-candidates.tsv),
not a fresh inventory. Fixture feasibility is proposed, not proven. Exclude a
candidate that cannot be demonstrated; do not weaken the runner to admit it.
Workers must check current tool/container availability before promising the CI
version run. Use `--jobs 4` for analysis and `--parallel 8` for builds.

## Execution status (2026-11-13)

- Q0: complete; stale ledger next actions reconciled with existing evidence.
- A4-1: fixture investigation reviewed. A second app-sized test target was
  rejected because its cost and duplicated build settings outweigh its limited
  branch coverage. See the [investigation](modernization-results/A4-validation.md).
- A4-2: the coordinator selected the bounded production resolver design.
  The [implementation](modernization-results/A4-resolver.md) preserves lazy
  authority lookup and adds direct policy tests plus adapter source guards.
  Full-chain QObject integration coverage is not claimed.
- A4-3: user-mode Debug desktop smoke passed by user report; subsequent log
  review found no emitted warning/error or new dock core. Full-chain
  primary-not-zero and nested-containment cases remain Pending validation.
- T1: thirteen candidates promoted and two exception-dependent checks excluded;
  fifteen total checks passed local gates on clang-tidy 22.1.8 and 18.1.3 across
  156 production sources. See the [slice record](modernization-results/T1.md)
  for the isolated LLVM 18 environment limits. The published A4/T1/T2 batch's
  CI run 43350616185 subsequently passed all 23 jobs.
- Q1: coordinator reviewed the combined production adapter/helper, tests,
  configuration and all new fixtures; independently verified five mutations.
  Four compiler/configuration builds have zero warnings and pass 64/64 tests
  each. This initial acceptance preceded authorized commit/push `803cce3d9`;
  its 23-job CI pass remains separate from later local work.
- T2: fourteen more candidates promoted, bringing the gate to twenty-nine;
  both local tool versions rejected all fixtures and found nothing in 156
  production sources. Published CI acceptance passed in run 43350616185;
  [record](modernization-results/T2.md).
- T3: the remaining sixteen Band A candidates plus the deferred string check
  are now proven and promoted. Both local full-source gates pass with forty-six
  fixtures and no findings in 156 production sources. The coordinator reviewed
  the actual diff; the expansion was published in `9e62bfaa9`.
  [Record](modernization-results/T3.md). Band A has forty-four promoted
  candidates and two exception-dependent exclusions; the original two checks
  bring the current total to forty-six.
- T4a: the existing BackgroundCache correction now has a
  `misc-redundant-expression` gate and negative fixture. All forty-seven checks
  pass on both local tool versions against `796344495`; all four application
  configurations have zero compiler warnings and pass 64/64 tests. This
  revalidates T3 against the newer source; both were published in `9e62bfaa9`.
  [Record](modernization-results/T4a.md).
- QML inventory refresh: the user's two new QML helpers expand coverage to 243
  files. The [reviewed CI baseline](modernization-results/QML-baseline-2026-11-13.md)
  preserves the 6.11.1 fingerprint and all visible warnings; three independent
  CI reports match. The previous 241-file baseline caused the current CI
  coverage failure. The refresh and T3/T4a were published in `9e62bfaa9`. Run
  [43404434373](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/43404434373)
  passed the deep QML and clang-tidy gates but failed on two pre-existing
  clazy loop findings in the user's preset feature. The
  [CI repair](modernization-results/CI-9e62bfaa9-clazy.md) preserves list snapshots.
- T4b: [move-constructor initialization](modernization-results/T4b-move-constructor.md)
  moves two window-ID byte arrays and expands the explicit gate to 48 checks.
  Remote acceptance passed in [run 43424231020](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/43424231020)
  on `64a7f1ae5` (23/23 successful jobs). Genuine Plasma 6.8 and 6.3 validation
  will use the user's Arch and Debian 13 VMs; no version macro is forced.

- T4c: [parent-virtual-call gate](modernization-results/T4c-parent-virtual-call.md)
  reaches 49 checks. The intentional ancestor call was reproduced on the real
  Arch Plasma 6.8 SDK before adding one line-specific exception; the normal
  immediate-base path remains unchanged. Both host full gates and four 64-test
  configurations pass. Published as `1d36b229b`; run 43429056818 passed all 23 jobs.
- Cross-version test API compatibility: [Qt test repairs](modernization-results/Qt-test-compatibility-2026-11-13.md)
  preserve the assertions on Qt 6.8 and 6.12. Arch passes both 64-test suites;
  Debian's accessibility test passes in its actual Wayland session. Debian's
  persistent-menu fixture now satisfies Qt 6.8's pointer-movement precondition,
  as verified against plain QMenu. Final per-platform results are recorded
  separately; the initial failures are not called passes.

- T4d: [copy initialization](modernization-results/T4d-copy-initialization.md)
  removes three audited copies while retaining the independent Apply-time
  layout snapshot and original-ID lock mapping. The gate now has 50 checks;
  both full host tool versions reject all fixtures and report no findings in
  156 production sources. Four compiler configurations pass 64/64 tests each
  with zero compiler warnings; focused clazy passes all three changed sources.
  Published as `6f9757229`; [run 43432385377](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/43432385377)
  passed all 23 jobs on the 50-check source.

T4c was published as `1d36b229b`; [run 43429056818](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/43429056818)
passed all 23 jobs on that 49-check source. It does not validate the
subsequent T4d changes.

- T4e: [header include direction](modernization-results/T4e-header-cycle.md)
  removes the record/table include cycle and declares the table dependencies
  explicitly in its two embedding headers. The gate has 51 checks; LLVM18/22
  full gates and all four 64-test compiler configurations pass, with zero
  compiler warnings. Four affected data sources are clazy-clean. This slice
  was published as `bbc8f6782`; [run 43436056930](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/43436056930)
  passed all 23 jobs on the 51-check source.

- T4f: [forward declaration namespaces](modernization-results/T4f-forward-declarations.md)
  removes three unused, incorrectly scoped declarations without changing the
  actual dialog/layout types or QML registration. The gate has 52 checks;
  LLVM18/22 full gates and all four 64-test configurations pass with zero
  compiler warnings. Three consumer sources are clazy-clean. One external SDK
  diagnostic was audited and remains outside the unchanged production scope.
  Published as `d0ce8cf89`; run 43440085083 passed all 23 jobs.

- T4g: [automatic moves](modernization-results/T4g-automatic-move.md) removes
  `const` from five value-return locals covering six diagnostics. Both full
  LLVM gates pass with 53 checks; four compiler configurations pass 64/64
  tests with zero compiler warnings, and five affected sources are clazy-clean.
  Published as `d77eef902`; run 43474161451 passed all 23 jobs on its
  53-check source. An additional Qt 6.12-only finding was fixed in T4h.
- T4h: [explicit switch defaults](modernization-results/T4h-switch-defaults.md)
  preserves all eight existing fallbacks and adds the 54th check. Both local
  gates and four 64-test configurations pass with zero compiler warnings;
  focused clazy is clean on seven affected sources. The Arch Qt 6.12 follow-up
  also passes the real Plasma 6.8 gate and targeted core behavior tests.
  Published as `97c4be752`; CI run 43481441747 passed all 23 jobs.

- T4i: [integer-division gate](modernization-results/T4i-integer-division.md)
  preserves popup centering, adds the 55th check, and passes both native gates,
  four 64-test configurations, and the Arch Qt 6.12 gate and core test.
  Wayland popup placement was not visually re-observed. Published as
  `43bc885aa`; CI run 43530811418 passed all 23 jobs.

- T4j: [startup screen fallback and drawing](modernization-results/T4j-screen-fallback.md)
  retains the configured screen through temporary startup fallback and merges
  two equivalent drawing branches. Local validation and desktop retesting pass,
  including one live DP-3 disable, startup fallback and re-enable transition.
  Published as `e8ba4ad70`; CI run 43545267519 passed all 23 jobs. The
  branch-clone gate is not enabled.

- T5 evaluation: [unchecked optional access](modernization-results/T5-optional-evaluation.md)
  passed fixture and production scans on LLVM 18 and 22. Three local paired
  runs showed no large wall-time increase, but CI cost and its budget remain
  unestablished; the check has not been promoted.
- T5 promotion: [unchecked optional access](modernization-results/T5-optional-promotion.md)
  **Complete locally, CI-version acceptance pending the push.** The check is the
  fifty-sixth gated check, with a proven negative fixture and no finding in 157
  production sources on clang-tidy 22.1.8; three new order-alternated pairs put
  the 55-to-56 increment below measurement resolution (differences +0.042,
  -0.220, -0.299 s). All four presets build with zero compiler warnings and pass
  65/65 CTest each. LLVM 18.1.3 is unavailable here; the `clang-debug` clang-tidy
  step on the push is the first 18.1.3 gate over this source. The Band B cost
  budget governs promoting families, not this single proven check.

- P1b measurement foundation: [KSycoca validity rule](modernization-results/P1b-measurement-foundation.md)
  **Complete locally.** `scripts/measure-process-family.py` now marks a window that
  overlaps a `~/.cache/ksycoca6_*` rewrite as invalid (the un-diagnosable part of
  P1b's outlier), pinned by `autotests/test_measure_process_family.py`
  (`measurefamilyrulestest`, 6 cases, two mutations caught). Four presets build with
  zero compiler warnings and pass 66/66 CTest each. A **live positive case** was
  produced on a user-provided Arch Plasma 6.8.3 VM: a forced rebuild inside a window
  was reported as `"valid": false` with the exact file and second. This supplies the
  launch-environment half only; P1b still needs a clean-environment rerun and an
  event-latency seam.

- VM validation (2026-11-21): [Debian 13.7 and Arch Plasma 6.8 results](modernization-results/VM-validation-2026-11-21.md).
  Debian: clang-tidy 18.1.8 passes the 56-check gate with 0 compiler warnings and
  CTest 65/66 (the one failure is an image accessibility-bridge gap proven by a
  standalone probe). Arch: the Clang Debug build is clean, and the known LLVM 23
  deprecated-check transition reproduces with an exact message. T5 was published in
  `564199ec1`; Build run 44863409259 passed the Clang Debug clang-tidy step (CI image
  clang-tidy 18.1.3), closing T5. Its GCC Debug/Release jobs failed before
  `actions/checkout` on the KDE Neon repository and dependency install, which is
  infrastructure and unrelated to the source.

- Tooling follow-up: CMake 4.5 emits four author warnings about QML typeinfo
  install-preparation dependencies. On Arch a verified clean `install_prep/all`
  and staged install generated and installed all four typeinfo/plugin pairs in
  both QML roots. The warnings remain; investigate explicit producer dependencies
  without suppressing them or raising the existing CMake floor.
  [Evidence](modernization-results/T4h-data/arch/cmake45/install-prep-qmltypes.md).

## Continuation (2026-11-16–17)

The continuation started from `main` at `0a6974f5cd1b13116764a2320e3d882674f5fed3`
with a clean working tree. Workers use `gpt-6-luna` with `medium` reasoning;
the coordinator owns integration and review. On 2026-11-17 the user explicitly
authorized commit and push after each completed task unless manual validation
is required; this does not authorize a release or tag.

- P4 A/A baseline: **Complete as a measurement slice**, published in
  `ebb736602`. Five same-build Fedora Plasma/KWin 6.8.3 runs passed the
  observer, identity, cache and artifact gates. The earlier 6.8.2 runs remain
  separate after an intervening package upgrade. The [baseline](modernization-results/P4-fedora-aa-baseline.md)
  records raw data, limitations and the resolution of zero whole CPU ticks;
  it establishes no saving and does not complete the deferred P4 optimization.
  Build CI for `ebb736602` is tracked separately.
- T4k: [QML metadata producer dependencies](modernization-results/T4k-qmltypes-install-dependencies.md)
  are **Complete**: clean genuine CMake 4.5/GCC on Fedora and CMake 4.5/Clang
  on the host passed Release install preparation and both-root staged metadata
  checks, independently reviewed by the coordinator. GCC/Clang Debug builds
  on CMake 4.3.4 each passed 64/64 tests with zero compiler diagnostics. Host
  QML tooling diagnostics remain recorded; no Release CTest or desktop retest
  is claimed for this build/install-only slice.

Post-publication CI repair: the P4 evidence commit introduced 11 archived
`.qml` copies under `docs/modernization-results/`, which expanded both lint
inventories and caused Deep QML lint to reject its 244-file baseline. The
[repair record](modernization-results/P4-qmllint-ci-inventory.md) documents a
shared inventory helper that excludes only that archive subtree, keeps the
baseline unchanged, and adds an isolated Git inventory test. Host GCC and Clang
Debug each pass 65/65 tests; the Nix check and CI-version deep lint require the
post-push Build workflow. No desktop behavior changed.

## Later waves

| Task group | Separate slices | Entry condition |
| --- | --- | --- |
| T2-T3 | Remaining Band A checks, approximately 15 per slice | T1 accepted; each slice repeats fixture and current-production proof |
| T4 | Fix and then gate one R8c check at a time | Review every finding; preserve documented intentional calls |
| T5 | Band B remaining checks, standalone analyzer, then Band C/unpriced candidates | For each further promotion: individual fixture proof and a measured cost argument; the first Band B check (unchecked optional access) is promoted |
| A2 | Bounded pre-filter message capture, then one filter family | Established emitter/version evidence; remove temporary instrumentation |
| A3 | Saved-layout fixtures, then one legacy splitter branch family | Consumer trace and Justify/separator/drag desktop coverage |
| P1b | Resolve Release A/B noise and event-correlated latency | Comparable repeated runs; distinguish reduced work from CPU/PSS savings |
| P3a | Generation-aware geometry publication design and probe verification | Authoritative completion signal before reducing eight-pass fallback |
| P4 | Shared task-query evaluation, then a bounded implementation if justified | Paired Release work and CPU/PSS evidence, accepted ownership design |
| P5 | Effect allocation/frame measurement, then a bounded implementation | Supported on/off measurement and visual acceptance |
| P6 | Module cold/warm measurement and override-safe build prototype | Demonstrated module-specific Release cost |
| M6/A5/R6 | Select one useful dependency or ownership boundary | Measured benefit and accepted narrow design; documents alone are not implementation. For R6 the remaining LayoutManager slices are S5 (needs the drag profile), S6 and S7; S1–S4 are implemented |

R8j remains closed at the user's decision. Its unrun 60-window experiment is
not a scheduled task. A new memory investigation requires its own design and
pre-registered verdict rule.

A3 has a preliminary [legacy Justify restore/save fixture](modernization-results/A3-legacy-justify-fixture.md).
It exercises production layout logic through the configuration property map;
disk-backed KConfig loading and live desktop interactions remain unverified.

P4's [Arch synthetic preflight](modernization-results/P4-arch-synthetic-preflight.md)
reached a private TaskManager session and activated two per-applet trackers in
both Folder/Trash and two-Trash synthetic layouts. These are not the historical
Fedora layout; runtime environment warnings remain, and the runs do not yet
justify resource measurement or a shared-query implementation.

P6's [scratch Release build preflight](modernization-results/P6-scratch-build-baseline.md)
establishes a local build baseline only; it does not measure module-attributable
runtime import cost or justify a build-system change.

## R6 LayoutManager continuation (2026-11-20)

S3 was already accepted when the execution queue was written; the next named
action was S4.

- R6-S4: [typed one-shot insertion protocol](modernization-results/R6-layoutmanager-s4.md)
  replaces `_latte_pendingInsertionIndex` and `_latte_skipSpacerReorder` with
  typed `LayoutManagerApi` members (IID `.../1.1`) owned by the manager and
  written only through `LayoutManagerApiDispatcher`; the dispatcher keeps the
  dynamic-property fallback for a pre-S4 plugin, and both creation sites drop
  an unconsumed request, closing the stale-index hazard in the
  [design](modernization-results/R6-layoutmanager-design.md#-family-4-name-channels-in-both-directions-dynamic-properties-and-qml-binding--property-).
  QML, registered names and persisted keys are unchanged. **Complete except**
  the user's desktop interaction retest (design §7.4) and the clazy and
  clang-tidy gates: `clazy-standalone` is not installed on this host, so
  `scripts/test-clazy.py` aborts and neither gate ran here. GCC and Clang Debug
  full builds have zero compiler warnings and pass 65/65 CTest each; the
  qmllint errors-only gate is clean on 244 files; a user-mode Debug startup
  logged both containment orders with no warning or error entries.
- R6-S5: [measured, not implemented](modernization-results/R6-layoutmanager-s5-evaluation.md).
  The slice was gated on a drag profile. The profile disproves its premise:
  `insertAtLayoutCoordinates()` contains no `readConfigValue()` call, and
  `readConfigValue("alignment")` is reached only from `requestAppletsOrder()`,
  which an applet drag never runs (`AppletItem.qml` sorts with
  `insertBefore()`/`insertAfter()`). A temporary probe collected six
  event-driven alignment-read samples (median 5.71 µs) and **zero** samples of
  the insertion-path reads. Probe removed, user installation uninstalled, no
  source change: **S5 is closed as unjustified**.
- Next action: the user runs the S4 desktop retest; the published CI run then
  owns the clazy/clang-tidy gates. The per-drag name calls the profile did find
  (`insertBefore`/`insertAfter`, `createAppletItem`, `initAppletContainer`)
  belong to S7, not S5.
- R6-S6: [evaluated, not recommended as written](modernization-results/R6-layoutmanager-s6-evaluation.md).
  The sixteen name-based layout-child reads all sit in structural functions, so
  the slice has no performance case either; its value is type safety. It costs a
  new registered QML type, both root types (`AppletItem.qml` at 2077 lines and
  `layouts/ParabolicEdgeSpacer.qml`) and all 60 qmllint baseline entries for
  `AppletItem.qml`. The same defect class — a renamed property silently reading
  as false — can be closed with missing-property diagnostics in the manager, the
  shape S1 already used for the root functions, without changing any QML.
- R6-S6 (implemented as diagnostics): `LayoutManager::noteLayoutChildRoles()`
  reports once per role a layout child stops declaring, using the same shape S1
  used for the root functions. A first draft demanded every role of every child
  and produced two false warnings on a real desktop, because a parabolic edge
  spacer never has `backendAppletRef`; the role list is now per class and
  `edgeSpacerNeedsNoBackendAppletRole` pins it. No QML and no qmllint baseline
  changed. GCC/Clang Debug: zero compiler warnings, 65/65 CTest each; qmllint
  gate OK; a user-mode Debug run logged 0 role warnings. The typed base class
  itself is not started.
- R6-S7: [evaluated, not started](modernization-results/R6-layoutmanager-s7-evaluation.md).
  Moving the creation functions to C++ does not remove the QML-engine boundary:
  their bodies own a `Qt.binding()` over `status`, `isHidden` and
  `keepVisibleOnHiddenStatus`, where `status` is itself resolved dynamically, and
  a per-container retry timer whose path no autotest covers. The opacity-based
  visibility is deliberate. The functions are also a compatibility surface for
  separately installed plugins.
- R6-S8: [measured, not implemented](modernization-results/R6-layoutmanager-s8-evaluation.md).
  This is the repository's only per-pointer-move named call, so it was worth
  measuring: over 7831 hover samples `indexOfMethod()` costs 1.11 µs and
  `method()` 0.055 µs against a 196 µs invocation (0.59 %). Caching the method
  would save ~1.2 µs of a 16.7 ms frame, below noise. The remaining 196 µs is the
  QML parabolic computation, a separate candidate.
- Parabolic computation (candidate, now measured): 196 µs p50 on Debug versus
  184 µs p50 on Release across 958 samples each. A 6 % gap means the cost is QML
  interpretation and JavaScript allocation, not compiled arithmetic, so a C++
  port of `applyParabolicEffect()` would not reach it and would cross the same
  engine boundary S7 rejected. Closed as unjustified; a future attempt must
  reduce per-move work instead. During this measurement a Release binary was run
  against the Debug `dev-env.sh` QML overlay, which broke window previews until
  the packaged dock was restored; the record documents that and the leftover
  state it created.
- Next action: this round of R6 is closed on evidence — S1–S4 implemented, S6
  shipped as diagnostics, S5/S7/S8 recorded as measured or assessed negatives.
  Nothing in this queue is blocked on the user. A future slice needs its own
  accepted design and, for the parabolic computation, a Release profile.

## References

- [Main plan and ledgers](architecture-modernization-plan.md)
- [Implementation rules and verification](architecture-modernization-implementation.md)
- [A-track ledger](architecture-followup-plan.md)
- [clang-tidy gate order](modernization-results/R8c.md#proposed-gate-order)
- [Existing production-wide gate](modernization-results/R8i.md)
- [Desktop evidence](modernization-results/R7-desktop-check.md)
- [Memory investigation closure](modernization-results/R8j.md#closure)
