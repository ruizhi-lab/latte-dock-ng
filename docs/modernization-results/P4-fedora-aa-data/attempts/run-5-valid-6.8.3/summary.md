# A/A run 5 — valid in the 6.8.3 environment group

Run 5 completed with observer exit 0 and an 11-event `valid-sequence` for the
controlled Dolphin window. It is labeled **same-fixture idle after the
controlled Dolphin activation/minimize/restore sequence**; dock visibility
and auto-hide mode were not verified.

The collector recorded 61 sample boundaries spanning 59.994 seconds. The
KSycoca fingerprint and descendant-family identity set remained stable, and
per-sample executable/start-time checks passed. Plasma, KWin, Qt Declarative,
Qt Base and running-kernel versions matched the run-4 6.8.3 group before and
after the run. Source/executable/fixture/manifest, staged QML/plugin/type data
and all four control-helper hashes matched their frozen pre/post references.
The dock exited after SIGTERM before its supporting services stopped.

This is the second valid run in the 6.8.3 environment group. Raw CPU ticks,
RSS/PSS, private memory, load, timestamps, observer records, logs, artifact
hashes and helper copies are retained here. No CPU or memory comparison is
claimed.

The `systemd-run --wait` wrapper completed successfully after 1 minute
53.505 seconds. See [`launcher-output.txt`](launcher-output.txt) for its exact
completion record.
