# Fedora P4 A/A raw data

This directory records same-build Release A/A baseline runs in separate
environment groups. Run 1 is retained as invalid, runs 2–3 are historical
valid runs from Plasma/KWin 6.8.2, and runs 4–8 are five valid runs in the
separate Plasma/KWin 6.8.3 group. See the
[environment review](environment-review-2026-11-17/README.md). After any
additional invalid attempt, use the next unused positive run ID.
See [`attempts`](attempts/README.md). Failed attempts must be retained with
their logs and marked invalid; do not replace or omit them when reporting the
five valid runs.

The runner and process-family collector are in [`tools`](tools/README.md).
They use the immutable Fedora prefix `/tmp/T4k-fedora-20261116/prefix`, the
two-Trash fixture, a private nested KWin and D-Bus session, and a scratch HOME,
XDG tree and `TMPDIR`. The full user-manager graphical environment is inherited
by the transient unit; only the active display variables are unset before the
private session is created. The user's real desktop and saved configuration
are not targets.

## Frozen build and fixture

- Source: `0a6974f5cd1b13116764a2320e3d882674f5fed3` plus the four recorded
  CMake-only changes preserved in [the frozen source patch](source-t4k.patch); archive SHA-256
  `92607311df689f379ea27a56dcfcd9d7180db9131b80d2a97a797dc8c3665fc9`
  (guest file `/tmp/T4k-fedora-20261116/source.tar.gz`, 7,010,937 bytes).
- Release executable SHA-256:
  `a86461b57ea69b4c2be145f4612a483e25fac329c11db6b3f978e44aaa675d60`.
- Install manifest SHA-256:
  `5c91bfd3e032d69930f71c09720bae24146323f1fa30f17d6b178313d6e13ff9`.
- Fixture SHA-256:
  `90b974f266dbee2228c10315f0b86191cb42a7887da2ee7289979425aa07f506`.
- Exact synthetic title observed in the accepted attempt-7 observer records:
  `window-title — Dolphin`.

Each run retains the executable, install-manifest, fixture, and staged QML,
plugin and type metadata hashes before and after sampling. The staged tree must
match at both ends. Source identity remains frozen by the archive digest above;
no build or source modification is allowed during a run.

## Frozen protocol

The five accepted runs (4–8) use the same staged build, layout, private 1280×800 virtual output,
synthetic Dolphin operation, observer sequence, and normal uninstrumented
Release process. The external observer alone enables `qml.debug=true` for its
own process. The dock receives
`QT_LOGGING_RULES='latte*.debug=false;qml.debug=false'`, no `--debug` flag, and
no QML instrumentation overlay. This keeps opt-in trace argument evaluation
disabled while the observer still emits its JSON event sequence.

For each run, the observer must exit 0 and report the exact target sequence
before a 30-second settle period. The collector then records 61 sample
boundaries at one-second intervals, spanning 60 seconds. It writes monotonic
timestamps, PID and `/proc` start-time
identity, the complete dock descendant family, CPU user/system ticks and
`SC_CLK_TCK`, summed RSS/PSS/private memory, load average, and per-sample
private KSycoca fingerprint. PID reuse, executable mismatch, disappearance,
memory-read failure or a cache fingerprint change rejects the run. CPU 100%
means one full core; 1-second sample scheduling drift is visible in timestamps.
The per-sample KSycoca fingerprint hashes sorted cache names, sizes and
modification times; this avoids rereading the cache contents every second.
The dock descendant set must remain constant across the window; a child/helper
appearing or disappearing invalidates the run instead of silently changing the
measured family. The first accepted 6.8.3 run (run 4) freezes the staged
QML/plugin/type payload hash list; every later run and each run's post-sample
payload must match it.
The collector does not measure exact wakeups, GPU memory, or frame timing.

The nested compositor, Activity Manager, Dolphin and dock remain alive through
the dock's SIGTERM and wait. Only after Latte exits are supporting processes
stopped. `RuntimeMaxSec=180s` bounds a stuck run. Each run gets a new directory;
the launcher refuses to overwrite one. Runs are not A/B comparisons and cannot
support a resource-savings claim.

## Completed 6.8.3 group

Runs 4–8 each contain 61 sample rows spanning 59.980–59.994 seconds, with
stable process-family and KSycoca fingerprints and successful observer verdicts.
Each run's aggregate CPU tick delta was zero whole ticks at `SC_CLK_TCK=100`;
this is below counter resolution and does not establish zero CPU consumption.
Across 305 samples, PSS ranged from 247,209 to 249,090 KiB. The median of the
per-run PSS medians was 247,803 KiB (range 247,209–249,089 KiB). A simple
inclusive-quartile 1.5×IQR screen flags run 4 as a high PSS outlier; it remains
included. No run was excluded or selected by its measured values. These are
descriptive baseline observations only. Dock visibility and auto-hide state
were not manually verified; no A/B or resource-savings claim is made.
