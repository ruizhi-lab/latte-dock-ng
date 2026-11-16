# A/A run 4 — execution valid, first run in the 6.8.3 environment group

Run 4 completed with observer exit 0 and an 11-event `valid-sequence` for the
controlled Dolphin window. It is labeled **same-fixture idle after the
controlled Dolphin activation/minimize/restore sequence**; dock visibility
and auto-hide mode were not verified.

The collector recorded 61 sample boundaries spanning 59.980 seconds. The
KSycoca fingerprint and descendant-family identity set remained stable, and
per-sample executable/start-time checks passed. Source, executable, fixture,
install manifest, staged QML/plugin/type data and all four control-helper
hashes matched their frozen pre/post references. Latte exited after SIGTERM
before its supporting processes stopped.

Run 4's wall-clock interval was 2026-11-17 07:24:43–07:25:43 CST. The host
CMake download/unpack ended by 07:23:34.507 CST, before this run's unit began
at 07:23:49; the download did not overlap the sample window. The Plasma/KWin
package update from 6.8.2 to 6.8.3 had already occurred. Run 4 is the first
execution-valid baseline in the separate 6.8.3 environment group and is not
pooled with historical runs 2–3. See the
[environment review](../../environment-review-2026-11-17/README.md).

This is the first of five valid runs in the completed 6.8.3 group. Raw CPU
ticks, RSS/PSS, private memory, load, timestamps, events, logs and hashes are
retained here. No CPU or memory comparison is claimed.

The `systemd-run --wait` wrapper completed successfully after 1 minute
53.940 seconds. See [`launcher-output.txt`](launcher-output.txt) for its exact
completion record.
