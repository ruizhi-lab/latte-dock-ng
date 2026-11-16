# P4 Fedora same-build A/A baseline

Status: Five valid same-build A/A baseline runs (4–8) are complete in the
Plasma/KWin 6.8.3 environment group. Runs 2–3 remain a separate historical
valid group from Plasma/KWin 6.8.2. Run 1 remains invalid because its observer
runner returned 72 without a pipe-captured verdict, so it produced no sample.
Root found the complete event sequence in run 1's unit journal. There is no A/B
comparison or savings claim.

## Gate evidence and limits

The bounded attempt-7 observer records show the two internal tracker rows with
the same controlled Dolphin window, matching Activity/desktop/screen context,
and an ordered external activation/minimize/restore sequence for the same
window. Root review independently accepted that scoped sequence and context
evidence. The attempt-7 launcher itself exited 1 because its snapshot selector
failed; the runner overwrote the original Latte log. The evidence review
therefore relies on retained runner rows and observer events. This is not a
claim that attempt 7 or the overall Fedora preflight completed successfully.
The production QML and staged prefix remained unmodified. This scoped
preflight evidence is distinct from the completed A/A measurements below.

## Frozen artifact and workload

The same-build Release artifact is source `0a6974f5cd1b13116764a2320e3d882674f5fed3`
plus the four CMake-only edits preserved in [the frozen source patch](P4-fedora-aa-data/source-t4k.patch), source archive SHA-256
`92607311df689f379ea27a56dcfcd9d7180db9131b80d2a97a797dc8c3665fc9`, GCC
16.2.1, system CMake 4.3.0, Release mode with `BUILD_TESTING=OFF`, and prefix
`/tmp/T4k-fedora-20261116/prefix`. The executable SHA-256 is
`a86461b57ea69b4c2be145f4612a483e25fac329c11db6b3f978e44aaa675d60`; the
install-manifest SHA-256 is
`5c91bfd3e032d69930f71c09720bae24146323f1fa30f17d6b178313d6e13ff9`. The
fixture is `two-trash.layout.latte`, SHA-256
`90b974f266dbee2228c10315f0b86191cb42a7887da2ee7289979425aa07f506`, with
two Trash applets IDs 3 and 4 in one containment. The staged files and build
details are recorded in [T4k Fedora artifact notes](T4k-data/fedora/README.md).

The controlled Dolphin title was observed as `window-title — Dolphin` in the
retained attempt-7 row and observer records. The A/A runner verifies the exact
title by requiring the external observer's complete same-window sequence.
Any observer failure, missing sample, PID identity change, unexpected cache
change, or artifact mismatch invalidates that run. Preserve invalid runs and
reasons alongside valid raw data.

## Protocol

Each same-build A/A run used a fresh private nested KWin,
private session bus, Activity Manager, scratch HOME/XDG/TMPDIR and transient
user unit. The full user-manager graphical/config environment is inherited;
the private run only unsets the outer display identifiers and points display,
configuration, install import paths and caches to its isolated session/stage.
It does not restart or reconfigure the user's live dock.

Each run starts the staged Release binary with production QML and project
plugins, no debug flag, no profiler, and no instrumentation overlay. Its own
`QT_LOGGING_RULES='latte*.debug=false;qml.debug=false'` disables opt-in trace
categories and argument evaluation. The separate observer process enables
`qml.debug=true` to capture the required ordered events and exits before the
warmup. The observed valid activation/minimize/restore operation is followed
by 30 seconds of settle and 61 one-second sample boundaries spanning 60 seconds.
The workload is described as same-fixture idle after that controlled Dolphin
sequence; dock visibility and auto-hide state are unverified.
The override guard checks that the private `XDG_DATA_HOME` has no Latte
plasmoid overlay; it does not inspect or depend on the real home directory.
The first run freezes a hash list for all staged QML, plugin and type metadata;
each subsequent run must match that list, and every run checks it again after
sampling. The source archive, executable, install manifest and fixture hashes
are also compared before and after each run.

The process-family collector writes raw CSV rows for monotonic time, PID,
`/proc` start time, executable, descendant identities, user/system CPU ticks
and `SC_CLK_TCK`, summed RSS/PSS/private memory, load average and private
KSycoca cache fingerprint. PID reuse, process loss, executable change,
incomplete memory data, cache change, observer failure or staged artifact hash
mismatch rejects the run. A reported 100% CPU equals one core. Load, cache,
sample timing and all invalid runs remain visible in retained outputs. Exact
wakeups, GPU memory and frame timing are not collected.

Each run keeps KWin, Activity Manager and Dolphin alive until the dock receives
SIGTERM and has exited; then support processes are stopped. `RuntimeMaxSec=180s`
bounds a stuck unit. These are baseline characterization runs only. They are
not A/B measurements and do not establish optimization benefit.

## Tools and local verification

The tools are documented in [A/A data](P4-fedora-aa-data/README.md). The
observer pipe-capture fix passed a [private offscreen readiness check](P4-fedora-aa-data/attempts/offscreen-readiness/README.md).
The first authorized live run remains invalid, with its [failure evidence](P4-fedora-aa-data/attempts/run-1-invalid/summary.md)
preserved. Runs 2 and 3 passed all observer, sample, identity, cache, artifact
and teardown gates in their historical group. Runs 4–8 passed the same gates in
the separate 6.8.3 group. Each run retains its raw CSV, observer events, logs,
identity, cache and artifact records; see the [attempt index](P4-fedora-aa-data/attempts/README.md).

## 6.8.3 baseline summary

All five valid runs recorded 61 sample boundaries spanning 59.980–59.994 s.
Their start/end CPU counters changed by zero whole `SC_CLK_TCK` ticks in each
window. At 100 ticks/s, this is below the counter's one-tick resolution; it is
not evidence of zero CPU consumption. CPU usage therefore cannot be summarized
as zero or used for a savings claim from these samples.

Across all 305 samples, PSS ranged from 247,209 to 249,090 KiB. The median of
the five per-run sample medians was 247,803 KiB; per-run medians ranged from
247,209 to 249,089 KiB. An inclusive-quartile 1.5×IQR screen flags run 4's PSS
median as a high outlier; it remains included, with no exclusion or
metric-based selection. This small-sample flag is descriptive only. RSS and
private-memory samples are also retained in each run's CSV. These are
descriptive baseline values for this one build,
workload and environment only. The workload was the same fixture after the
controlled Dolphin activation/minimize/restore sequence; whether the dock was
visibly shown or auto-hidden was not manually verified. The observer's ordered
window events were independently validated from its log. No optimization
benefit, A/B difference or general idle behavior is inferred.

## Continuation status (2026-11-17)

The checkout remains on `main` at `0a6974f5cd1b13116764a2320e3d882674f5fed3`;
the existing T4k and modernization worktree edits remain untouched. The Fedora
package update from Plasma/KWin 6.8.2 to 6.8.3 occurred after historical runs 2
and 3. The separate [environment review](P4-fedora-aa-data/environment-review-2026-11-17/README.md)
records DNF/RPM evidence, run windows, current package/runtime identities and
the non-Latte `plasmashell --version` core. Runs 2 and 3 remain a historical
6.8.2 group; runs 4–8 form the completed five-run 6.8.3 group. The source,
executable, fixture, install manifest, QML/plugin/type payload and all four
controller helper hashes match across the five runs. Run 1 remains invalid and
preserved. The guest has been released for the separately authorized T4k work.
No savings claim is made.

Raw generated logs and CSV files retain their original locale and line endings
for evidence integrity; authored reports and tooling are in English.
