# Fedora A/A tools

- [`run-aa.sh`](run-aa.sh) creates one private nested-session run and retains
  its logs and artifacts. Use distinct positive integer IDs; run `1` is retained
  as an invalid observer attempt, and the next five valid runs can use IDs `2`
  through `6`. Use the next unused ID after any further invalid attempt. It
  refuses to overwrite an existing run directory.
- [`collect_aa.py`](collect_aa.py) records fixed-interval process-family CPU
  ticks, PSS/RSS/private memory, load and KSycoca fingerprints. It rejects
  process identity or family changes and KSycoca updates during the window.
- [`test_collect_aa.py`](test_collect_aa.py) checks process identity, PID reuse,
  executable verification and the 60-second inclusive sample schedule.

The runner invokes the shared, reviewed
[TaskManager observer](../../P4-arch-preflight-data/tools/README.md),
`run_p4_taskmanager_observer.py` and `p4-taskmanager-observer.qml`, from the
same remote `tools/` directory. The external observer alone enables
`qml.debug=true` and forces its QML child messages to stderr for pipe capture;
the dock keeps `latte*.debug` and `qml.debug` disabled.
Do not add the diagnostic QML generator or overlay to an A/A run.

Each run also retains its four controller helpers and checks their SHA-256
identities before and after execution. Run `2` freezes the helper hash reference
for runs `2`–`6`; invalid run `1` used the prior `console.log` observer and is
not part of accepted baseline evidence.

On Fedora, stage these scripts and the shared observer under
`/tmp/P4-fedora-20261116/tools/`, with `two-trash.layout.latte` and
`install_manifest.txt` under its `data/` directory. Run the collector tests
and shell syntax check locally before upload. Each A/A run itself is launched
through the user manager with a fresh private bus, KWin, Activity Manager,
HOME, XDG tree and `TMPDIR`.
