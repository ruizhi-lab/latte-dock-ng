# A/A run 7 — valid in the 6.8.3 environment group

Run 7 completed with observer exit 0 and an 11-event `valid-sequence` for the
controlled Dolphin window. The workload is **same-fixture idle after the
controlled Dolphin activation/minimize/restore sequence**; dock visibility
and auto-hide mode were not verified.

The collector recorded 61 sample boundaries spanning 59.991 seconds. The
KSycoca fingerprint and descendant-family identity set remained stable, and
per-sample executable/start-time checks passed. Plasma, KWin, Qt Declarative,
Qt Base and running-kernel versions matched the 6.8.3 group before and after
the run. Source/executable/fixture/manifest, staged QML/plugin/type data and
all four control-helper hashes matched their frozen pre/post references. Latte
exited after SIGTERM before its supporting processes stopped.

This is the fourth valid run in the 6.8.3 group. Raw CPU ticks, RSS/PSS,
private memory, load, timestamps, observer records, logs, artifact hashes and
helper copies are retained here. CPU tick delta was zero whole ticks at the
sampling resolution, which does not mean zero CPU use. No CPU or memory
comparison is claimed. The compact local copy has no separate
`launcher-output.txt` for run 7; its collector data and
`dock-exited-after-SIGTERM` teardown marker are retained.
