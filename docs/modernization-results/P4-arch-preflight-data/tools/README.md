# P4 external TaskManager observer

`p4-taskmanager-observer.qml` observes one exact TaskManager window row in an
isolated nested KWin session. It requires a unique app ID and exact display
title, stable `WinIdList` identity, valid ActivityInfo and VirtualDesktopInfo
context, and a positive screen rectangle. It drives the row through
`requestActivate`, `requestToggleMinimized`, a second
`requestToggleMinimized`, and a final `requestActivate`, then validates every
role transition. Restoration and activation are separate controller calls,
and the observer verifies both reported roles afterward.

Run only from the private nested KWin session and provide a synthetic window
with an exact unique caption; do not point it at the real desktop. The outer
runner refuses to control windows unless `P4_PRIVATE_SESSION=1` is set. Example
after the test window has been created in that private session:

```bash
P4_PRIVATE_SESSION=1 python3 run_p4_taskmanager_observer.py \
  --app-id org.kde.dolphin --target-title 'P4 observer synthetic window' \
  --qml /usr/lib64/qt6/bin/qml \
  --timeout-ms 20000 --stage-timeout-ms 5000 \
  --log /tmp/p4-event-observer.jsonl
```

The QML timer is bounded, and the Python runner also enforces a monotonic hard
deadline while draining output without blocking. Only exit code 0 after the
complete ordered, same-window role sequence is acceptable for subsequent
sampling. Exit 20 means readiness timed out or became invalid; exit 21 means a
ready model/window/transition failed; other nonzero exits indicate runner or
QML failure. A failed observer run must not be used as performance data.

The offscreen readiness check intentionally has no KWin, display, or target
window. Run it in a private D-Bus and temporary HOME/XDG/TMPDIR environment;
expected result is exit 20 with `context-timeout`. It checks the TaskManager
QML imports and fail-closed readiness path, not desktop behavior:

```bash
p4_scratch="$(mktemp -d /tmp/p4-observer.XXXXXX)"
trap 'rm -rf "$p4_scratch"' EXIT
install -d -m 0700 "$p4_scratch/home" "$p4_scratch/config" \
  "$p4_scratch/cache" "$p4_scratch/data" "$p4_scratch/runtime" "$p4_scratch/tmp"
env -u DISPLAY -u WAYLAND_DISPLAY \
  HOME="$p4_scratch/home" XDG_CONFIG_HOME="$p4_scratch/config" \
  XDG_CACHE_HOME="$p4_scratch/cache" XDG_DATA_HOME="$p4_scratch/data" \
  XDG_RUNTIME_DIR="$p4_scratch/runtime" TMPDIR="$p4_scratch/tmp" \
  P4_PRIVATE_SESSION=1 QT_QPA_PLATFORM=offscreen \
dbus-run-session -- python3 run_p4_taskmanager_observer.py \
  --app-id org.kde.dolphin --target-title P4-offscreen-no-window \
  --timeout-ms 1000 --stage-timeout-ms 500 \
  --log /tmp/p4-observer-offscreen.jsonl
```

Run the focused support checks with:

```bash
python3 -m unittest discover -s . -p 'test_p4_taskmanager_observer.py'
```

The silent-child watchdog proof used a temporary `qml6` stub that slept without
writing output. The outer runner returned 70 after its 3-second monotonic
deadline and retained the timeout record in `silent-child-timeout.jsonl`.
