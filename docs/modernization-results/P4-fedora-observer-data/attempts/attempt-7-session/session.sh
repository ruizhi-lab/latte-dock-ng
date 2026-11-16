#!/usr/bin/env bash
set -eu
BASE=/tmp/P4-fedora-20261116
PREFIX=/tmp/T4k-fedora-20261116/prefix
kwin_wayland --virtual --width 1280 --height 800 --socket wayland-p4 --no-lockscreen --no-global-shortcuts >"$BASE/session/kwin.log" 2>&1 & KWIN_PID=$!
ACT_PID= DOCK_PID= DOLPHIN_PID=
# Keep KWin and the activity service alive until Latte has completed its own
# teardown; stopping all four together can close the Wayland pipe mid-shutdown.
cleanup() {
  kill "$DOCK_PID" 2>/dev/null || true
  wait "$DOCK_PID" 2>/dev/null || true
  kill "$DOLPHIN_PID" "$ACT_PID" "$KWIN_PID" 2>/dev/null || true
  wait "$DOLPHIN_PID" "$ACT_PID" "$KWIN_PID" 2>/dev/null || true
}
trap cleanup EXIT
for n in $(seq 1 100); do test -S "$XDG_RUNTIME_DIR/wayland-p4" && break; sleep .1; done
test -S "$XDG_RUNTIME_DIR/wayland-p4" || { echo P4_NO_GO kwin-socket-timeout; exit 2; }
export WAYLAND_DISPLAY=wayland-p4 QT_LOGGING_RULES='latte*=true;org.kde.taskmanager*=true;qml.debug=true' LANG=C.UTF-8
/usr/libexec/kactivitymanagerd >"$BASE/session/activity.log" 2>&1 & ACT_PID=$!
sleep 2
"$PREFIX/bin/latte-dock-ng" --debug --log-file "$BASE/session/latte.log" --import-layout "$BASE/data/two-trash.layout.latte" --suggested-layout-name 'P4 Fedora Fixture' >"$BASE/session/latte.stdout" 2>&1 & DOCK_PID=$!
sleep 8
mkdir -p "$BASE/session/window-title"
dolphin --new-window "$BASE/session/window-title" >"$BASE/session/dolphin.log" 2>&1 & DOLPHIN_PID=$!
sleep 12
printf 'P4_PREFLIGHT pids kwin=%s activity=%s dock=%s dolphin=%s\n' "$KWIN_PID" "$ACT_PID" "$DOCK_PID" "$DOLPHIN_PID"
grep -hE 'P4_TRACKER|P4_APPLET|AppletItem.qml|Failed|error|Error|WARNING|Warning' "$BASE/session/latte.stdout" "$BASE/session/latte.log" "$BASE/session/activity.log" "$BASE/session/kwin.log" "$BASE/session/dolphin.log" || true
if ! grep -hEq 'P4_TRACKER_ROW.*"appletId":3' "$BASE/session/latte.stdout" "$BASE/session/latte.log" || ! grep -hEq 'P4_TRACKER_ROW.*"appletId":4' "$BASE/session/latte.stdout" "$BASE/session/latte.log"; then
  echo P4_NO_GO internal-tracker-rows-not-observed
  exit 20
fi
TITLE=$(python3 - "$BASE/session/latte.log" <<'PY'
import json, pathlib, sys
latest = {}
for line in pathlib.Path(sys.argv[1]).read_text().splitlines():
    marker = "P4_TRACKER_ROW "
    if marker not in line:
        continue
    try:
        row = json.loads(line.split(marker, 1)[1])
    except json.JSONDecodeError:
        continue
    if row.get("appId") == "org.kde.dolphin" and row.get("appletId") in (3, 4):
        latest[row["appletId"]] = row
if set(latest) != {3, 4}:
    raise SystemExit(f"expected matching-row snapshots for applets 3 and 4, found {sorted(latest)}")
left, right = latest[3], latest[4]
identity = (left.get("appId"), left.get("title"), left.get("winIdList"))
if not identity[1] or not identity[2] or identity != (right.get("appId"), right.get("title"), right.get("winIdList")):
    raise SystemExit("applets 3 and 4 do not show the same unique controlled Dolphin row")
if left.get("screen") != right.get("screen"):
    raise SystemExit("applets 3 and 4 do not report the same screen geometry")
for applet_id, row in latest.items():
    if row.get("isWindow") is not True or row.get("isStartup") is not False or row.get("isActive") is not True or row.get("isMinimized") is not False:
        raise SystemExit(f"applet {applet_id} controlled row is not an active, shown window")
    activity = row.get("activity")
    activity_hex = str(activity).replace("-", "").lower() if activity is not None else ""
    if activity != left.get("activity") or activity in (None, "") or (activity_hex and not activity_hex.strip("0")):
        raise SystemExit("controlled rows do not share a valid activity")
    desktop = row.get("desktop")
    desktop_hex = str(desktop).replace("-", "").lower() if desktop is not None else ""
    if desktop != left.get("desktop") or desktop in (None, "") or (desktop_hex and not desktop_hex.strip("0")):
        raise SystemExit("controlled rows do not share a valid desktop")
    screen = row.get("screen")
    if not screen or screen.get("width", 0) <= 0 or screen.get("height", 0) <= 0:
        raise SystemExit(f"applet {applet_id} has invalid screen geometry")
print(left["title"])
pathlib.Path(sys.argv[1].replace("latte.stdout", "internal-parity.json")).write_text(json.dumps({
    "appId": identity[0], "title": identity[1], "winIdList": identity[2],
    "activity": left["activity"], "desktop": left["desktop"], "screen": left["screen"]
}) + "\n")
PY
)
set +e
P4_PRIVATE_SESSION=1 python3 "$BASE/tools/run_p4_taskmanager_observer.py" --qml /usr/lib64/qt6/bin/qml --app-id org.kde.dolphin --target-title "$TITLE" --timeout-ms 12000 --stage-timeout-ms 3000 --log "$BASE/session/observer.jsonl"
STATUS=$?
printf 'P4_PREFLIGHT observer_exit=%s\n' "$STATUS"
grep -hE 'P4_EVENT|P4_TRACKER' "$BASE/session/observer.jsonl" "$BASE/session/latte.stdout" || true
if [[ "$STATUS" -eq 0 ]]; then
  python3 - "$BASE/session/internal-parity.json" "$BASE/session/observer.jsonl" <<'PY' || STATUS=$?
import json, pathlib, sys
internal = json.loads(pathlib.Path(sys.argv[1]).read_text())
events = []
for line in pathlib.Path(sys.argv[2]).read_text().splitlines():
    try:
        record = json.loads(line)
    except json.JSONDecodeError:
        continue
    if isinstance(record.get("qml"), dict):
        events.append(record["qml"])
readiness = [event for event in events if event.get("event") == "readiness"]
baseline = [event for event in events if event.get("event") == "baseline-row"]
results = [event for event in events if event.get("event") == "result"]
if len(readiness) != 1 or len(baseline) != 1 or len(results) != 1:
    raise SystemExit("observer log does not contain exactly one readiness, baseline, and result event")
ready, row, result = readiness[0], baseline[0], results[0]
expected_screen = [internal["screen"][key] for key in ("x", "y", "width", "height")]
if ready.get("activityReady") is not True or ready.get("desktopReady") is not True or ready.get("screenReady") is not True:
    raise SystemExit("observer readiness event did not report ready context")
if ready.get("activityId") != internal["activity"] or ready.get("desktopId") != internal["desktop"] or ready.get("screen") != expected_screen:
    raise SystemExit("observer readiness context differs from the internal tracker snapshot")
window_ids = internal.get("winIdList")
expected_window = str(window_ids[0]) if isinstance(window_ids, list) and window_ids else ""
if (not expected_window or str(row.get("windowId", "")) != expected_window
        or str(result.get("windowId", "")) != expected_window
        or row.get("appId") != internal["appId"] or row.get("title") != internal["title"]
        or result.get("verdict") != "valid-sequence"):
    raise SystemExit("observer window identity or final verdict differs from the internal tracker snapshot")
print("P4_PREFLIGHT parity=PASS activity desktop screen and window identity matched")
PY
fi
exit "$STATUS"
