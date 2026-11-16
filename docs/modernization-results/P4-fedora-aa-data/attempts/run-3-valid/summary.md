# A/A run 3 — valid

Run 3 completed with observer exit 0 and an 11-event `valid-sequence` for the
controlled Dolphin window. This run is labeled **same-fixture idle after the
controlled Dolphin activation/minimize/restore sequence**. Dock visibility and
auto-hide mode were not verified.

The collector recorded 61 sample boundaries spanning 59.994 seconds. The
KSycoca fingerprint and descendant-family identity set were each stable across
the window; per-sample executable and start-time checks passed. Source,
executable, install-manifest, fixture, QML/plugin/type payload and all four
control-helper hashes matched their required pre/post and frozen references.
The dock exited after SIGTERM before its supporting services stopped.

This is the second valid run in the historical Plasma/KWin 6.8.2 environment
group. The later package update separates it from the 6.8.3 series. Raw CPU ticks, RSS/PSS,
private memory, load, timestamps, observer records, logs, artifact hashes and
control-helper copies are retained here. No CPU or memory comparison is claimed.

The `systemd-run --wait` wrapper completed successfully after 1 minute
53.302 seconds. See [`launcher-output.txt`](launcher-output.txt) for its
completion record.
