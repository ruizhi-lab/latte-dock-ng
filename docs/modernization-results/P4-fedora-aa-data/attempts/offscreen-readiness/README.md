# Fedora offscreen observer proof

The initial post-change wrapper invocation omitted the runner's required
`P4_PRIVATE_SESSION=1` guard and exited before starting the observer; its empty
[`observer.jsonl`](observer.jsonl) records that no observer events were
captured. The corrected proof ran the QML observer in a scratch environment
and private D-Bus session, with no `DISPLAY`, Wayland display, KWin, or Latte
process.

Environment: `QT_QPA_PLATFORM=offscreen`, `QT_LOGGING_RULES='qml.debug=true'`,
scratch HOME/config/cache/data/state/runtime/TMPDIR, and
`P4_PRIVATE_SESSION=1`. Command inside the private bus:

```sh
python3 /tmp/P4-fedora-20261116/tools/run_p4_taskmanager_observer.py \
  --qml /usr/lib64/qt6/bin/qml --app-id org.kde.dolphin \
  --target-title P4-offscreen-no-window --timeout-ms 1000 \
  --stage-timeout-ms 500 --log "$SCRATCH/observer.jsonl"
```

The runner returned exit 20 as expected. The captured records contain one
`started` event followed by one `result` event with verdict
`invalid-readiness` and reason `context-timeout`. The stdout capture also
contains `Failed to create wl_display`, expected with display variables unset.
This verifies the parser can capture warning-level QML event output and still
fail closed when no desktop context exists; it does not prove the live Fedora
window sequence or authorize an A/A retry.

The validated outputs are
[`offscreen-readiness-qt-stderr.jsonl`](offscreen-readiness-qt-stderr.jsonl)
and [`offscreen-readiness-qt-stderr.stdout`](offscreen-readiness-qt-stderr.stdout).
The observer QML hash is
`eba592aa81149cef187a52cf26240679a73726b1dfe6f9cf9c043cda61f23bb8`; the
runner hash is
`273ec09d4987bed2655402618fc68d0f1ae19219829c03226ffc416558db0aab`.
