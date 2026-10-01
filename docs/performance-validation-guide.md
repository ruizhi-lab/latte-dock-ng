# Performance Measurement and Regression Acceptance

Use for P0 and every implemented P batch in the
[modernization plan](architecture-modernization-plan.md). It supplements the
[development testing guide](development-testing-guide.md); it does not replace
the user-mode Debug retest and clean-quit procedure in AGENTS.md.

The 2026-09-29 source assessment contains no live performance measurement.
Profile before choosing a fix; reduced source size, fewer timer declarations
or a smaller binary is not proof of reduced resource consumption.

## Comparable artifacts

1. A is the pre-change revision for the selected batch, initially the recorded
   main baseline. B is that revision plus only the selected slice. Later batches
   use the already validated preceding revision as A, not an unrelated old build.
2. Record full revisions/diff identity, compiler and flags, Qt/Plasma/compositor,
   kernel/GPU driver, screen geometry/scales/refresh rates, power profile and
   configuration fingerprint. Do not publish personal configuration contents.
3. Use matching Release or RelWithDebInfo modes for performance comparisons;
   matching Debug runs may characterize work but do not predict Release cost.
   Keep correctness retesting in the canonical user-mode Debug installation.
4. Keep each executable with its matching QML, project plugins and helpers.
   An old binary loading new loose QML is not an A comparison. Prefer separate
   staging prefixes and explicit import/library roots derived from the actual
   installation manifest. Document shared user-QML overrides and system modules.
   If isolation cannot be established, do not report A/B numbers as comparable.
5. Use an existing suitable checkout or the managed worktree workflow when
   isolated sources are needed. Do not silently switch the current main branch,
   reset it or copy half of a live installation. Preserve artifacts required
   for regression/coredump reproduction until verification finishes.
6. Save profile/instrumentation output separately from uninstrumented samples.
   Logging every frame or adding counters can change timing and allocations.
   Test seams/counters must be bounded and inactive in normal use.

## Scenario matrix

P0 selects representative scenarios; an implementation must run its relevant
rows and the common interaction checks. Keep window/task/applet counts fixed
across A/B and record operations explicitly rather than calling a run "normal".

| Scenario | Reproducible operation | What to observe |
| --- | --- | --- |
| Idle visible | Same screen/layout/windows; pointer outside dock; no pending animation; sample after warmup | CPU time, memory plateau, scheduling/timer activity |
| Idle hidden | Auto-hide settles; pointer stays away; wait for helper idle lease if applicable | Ongoing bindings/polls, helper lifetime, unexpected activation |
| Title-only updates | A controlled window changes title while its geometry/focus/desktop stay fixed | Metadata freshness, hint schedules/executions, CPU work |
| Hover/zoom | Repeat the same pointer sweep over the same tasks at fixed zoom/theme | Pixmap/texture work, frame duration/tail latency, visual fidelity |
| Edge relocation | Repeat bottom/right/top/left transitions, including rapid superseding changes | Layout passes, final geometry/readiness, task icon publication |
| Edit and drag | Enter/leave edit, reorder applets/tasks, close/cancel menus, then idle | Notification correctness, fallback polls, stale callbacks |
| Window state | Move/resize, activate, maximize, minimize, restore, desktop/activity changes | Hide/dodge behavior, task flags, latency, unrelated-view work |
| Resource cycles | Repeat affected open/close, create/remove or attach/detach operation at least 20 times | Live object/resource counts and retained-memory trend |
| Startup | Separate cold application-QML-cache and warm-cache runs with identical packages | Launch-to-usable latency and allocations; no global OS cache dropping |

Use a synthetic test window in an isolated desktop/VM when automated title,
focus or compositor operations would disturb the active user session. On the
live desktop coordinate the actual scenario with the user under the established
retest workflow. Do not repeatedly restart the user's dock for background
measurements without an implementation/retest task authorizing it.

## Metrics and limitations

| Metric | Suggested method | Required interpretation |
| --- | --- | --- |
| Process CPU | pidstat or deltas of /proc/<pid>/stat user/system ticks over elapsed time | State whether 100% means one core; do not compare lifetime ps %CPU values. |
| Memory | /proc/<pid>/smaps_rollup, RSS and private-memory fields | Prefer PSS/private memory for ownership; report RSS separately and note allocator/driver retention. |
| Scheduling | pidstat -w, process context-switch counters and a supported wakeup trace | Context switches and timer triggers are proxies, not exact CPU wakeups. Mark exact wakeups unavailable if not measured. |
| QML work | QML profiler, bounded counters or trace markers for selected production callbacks | Count binding/layout/pixmap work, not just elapsed time; separate profiler overhead. |
| Frames | QML/scene-graph profiling supported by the selected Qt version | Report median, p95/p99 and dropped/over-budget frames where available; refresh-rate budget varies. |
| GPU | Supported per-process/driver allocation counters and graphics profiling | Report method and compositor ownership; RSS/PSS do not measure all GPU allocations. |
| Startup | A repeatable usable-state marker with monotonic timestamps | Process creation time alone is not dock readiness; cold/warm application caches are distinct. |

Measure the process family: dock, add-launcher when active, and preview helper
when enabled. Sum comparable PSS/private memory with process identities and
sample times; helper savings cannot be claimed by omitting it from B. An idle
helper that already exits after its lease is existing behavior, not new savings.

Example read-only capture after identifying the actual dock PID and verifying
its executable. Tool availability is optional; a missing tool is not a metric
value of zero. Prefer an argument-vector collector if adding reusable tooling.

```bash
latte_measurement_pid="$(pgrep -x latte-dock-ng)"
# Require exactly one PID and verify /proc/<pid>/exe before using these samples.
ps -p "$latte_measurement_pid" -o pid,comm,etime,rss
cat "/proc/$latte_measurement_pid/smaps_rollup"
pidstat -u -r -w -p "$latte_measurement_pid" 1 60
```

Collectors must handle process disappearance/restart and PID reuse. Verify
process start time/executable on each interval rather than silently measuring
a new process under the old label. No process termination, ptrace attachment,
privileged system-wide tracing or configuration changes are part of a
read-only collector. Use supported profiling under the chosen test workflow
when deeper traces are needed.

## Paired runs and decision rules

1. Predeclare the primary metric, relevant scenarios and expected work reduction
   before implementing. Examples: no new hint schedule for title-only events;
   no raster/texture recreation for an identical effective icon request.
2. Establish A/A repeatability with the same environment. Use a documented
   warmup (initially 30 seconds after startup/interaction settles) and a fixed
   sampling interval (initially 60 seconds for steady-state scenarios).
3. Perform at least five paired A/B runs. Alternate ordering or use ABBA to
   reduce warmup/load bias. Interactive scenarios use identical operation
   sequences; record deviations and retry invalid pairs. Never select only the
   best B or worst A result.
4. Preserve per-run data and show paired deltas, median, spread and outliers.
   For frames retain distribution/tail results, not merely average FPS.
   Thermal throttling, background loads and differing caches invalidate a pair
   if they materially differ; record exclusions with their reason.
5. Accept an optimization only when its intended production work decreases and
   representative measurements support the benefit without a regression beyond
   the established noise range in response/frame latency or retained resources.
   If work counts improve but CPU is below resolution, state that precisely;
   do not advertise a measured CPU saving. A material tradeoff needs an explicit
   user decision on the concrete results before accepting the batch.
6. Do not impose an invented universal CPU percentage, MB target or 60-FPS goal
   across hardware. Freeze any project-specific threshold in the batch record
   before results are collected; do not loosen it retrospectively.
7. After resource cycles, allow documented caches/helper leases to settle.
   Require no unexplained persistent growth in live objects/resources or memory
   slope. RSS need not return byte-for-byte because allocators can retain pages.
   Investigate a growth trend with object/allocation evidence, not RSS alone.

## Functional acceptance

Run the affected GCC/Clang tests and QML checks before desktop retest. Preserve
and exercise all applicable features; this matrix narrows measurement, not
feature support. Include the relevant combinations of:

- Four edges, horizontal/vertical orientation, center/justify alignment,
  multiple screens, hotplug and integer/fractional scale.
- Task title tooltips, hover highlight, grouped-window actions, middle-click
  close, scroll minimize, drag auto-pin and edit-mode interaction.
- Digital clock, systray, volume, appmenu, clipboard and separator/spacer
  sizing, popup positioning and interaction where the touched path affects them.
- Automatic hiding/dodge decisions, menus/submenus, overlapping hiding blockers,
  startup/reconfiguration, component removal and restart/clean quit.
- Preview enabled/disabled choices, helper failure fallback, stale messages,
  hover changes and idle exit when the preview path is touched. Do not enable
  the legacy preview Loader or change user feature choices for convenience.

Use the exact AGENTS.md Debug install and detached user-mode launch, wait for
user feedback, then inspect the fresh log for new warnings/errors. Teardown
changes require coredump baseline and expected teardown markers. Crash fixes
also require pre-fix/fixed reproduction, separately from performance comparison.
Do not drive actual logout/shutdown without the appropriate user coordination.

## Evidence and failure handling

Record artifacts/environment, per-run metrics, unavailable counters, operation
sequence, functional results, logs, clean-quit results and any tradeoff in the
batch handoff. Summaries/fixtures must be reproducible and scrub personal window
titles/paths before publishing evidence. Keep raw diagnostics locally when
they are unsuitable for the repository.

If a regression appears, stop dependent optimization, preserve the failing
scenario and add a focused production regression test when practical. Fix or
abandon only that slice while preserving user edits and the known-good artifact.
Unreproduced performance benefit or unavailable required desktop checks leaves
the implementation Pending validation; it does not justify deleting fallbacks.

[Qt's performance guidance](https://doc.qt.io/qt-6.8/qtquick-performance.html)
supports profiling actual bindings/work and controlling optional object
lifetimes. Apply those principles within the project's documented Plasma
visibility and ownership constraints, not as a blanket unloading rule.
