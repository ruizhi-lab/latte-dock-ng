# A/A run 2 — valid

Run 2 completed with observer exit 0 and a complete `valid-sequence` verdict
for one stable Dolphin window. This run is labeled **same-fixture idle after
the controlled Dolphin activation/minimize/restore sequence**. Dock visibility
and auto-hide mode were not verified. The raw observer log contains 11 parsed events.
The collector recorded 61 samples spanning 59.993 seconds, with one unchanged
KSycoca fingerprint and one unchanged descendant-family identity set. The
collector's per-sample executable/start-time checks completed without error.

The source, executable, install manifest and fixture pre/post hashes matched.
The staged QML/plugin/type hash list matched its frozen reference and its
pre/post copy. All four control helper copies and their pre/post SHA-256 list
matched the run-2 helper reference. Latte exited after SIGTERM before KWin,
Activity Manager or Dolphin were stopped.

This is one valid run in the historical Plasma/KWin 6.8.2 environment group.
The later package update separates it from the 6.8.3 series. Raw CPU ticks, RSS/PSS,
private memory, load, timestamps, events, logs, hash lists and helper copies
remain in this directory. No CPU or memory comparison is claimed here.

The `systemd-run --wait` wrapper completed successfully after 1 minute
53.318 seconds. See [`launcher-output.txt`](launcher-output.txt) for its exact
completion record.
