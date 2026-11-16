#!/usr/bin/env bash
set -euo pipefail
BASE=/tmp/P4-fedora-20261116
PREFIX=/tmp/T4k-fedora-20261116/prefix
SCRATCH="$BASE/session"
if [[ -e "$SCRATCH" ]]; then
  ATTEMPT=1
  while [[ -e "$BASE/attempts/attempt-$ATTEMPT-session" ]]; do ((ATTEMPT += 1)); done
  mkdir -p "$BASE/attempts"
  mv "$SCRATCH" "$BASE/attempts/attempt-$ATTEMPT-session"
fi
mkdir -p "$SCRATCH/home/.config" "$SCRATCH/home/.cache" "$SCRATCH/home/.local/share/plasma/plasmoids" "$SCRATCH/home/.local/state" "$SCRATCH/tmp" "$SCRATCH/runtime"
chmod 700 "$SCRATCH/runtime"
PLASMOID="$PREFIX/share/plasma/plasmoids/org.kde.latte.containment"
cp -a "$PLASMOID" "$SCRATCH/home/.local/share/plasma/plasmoids/"
python3 "$(dirname "$0")/generate_p4_diagnostics.py" "$SCRATCH/home/.local/share/plasma/plasmoids/org.kde.latte.containment/contents/ui/applet/AppletItem.qml"
ORIGINAL=$(sha256sum "$PLASMOID/contents/ui/applet/AppletItem.qml" | cut -d' ' -f1)
PATCHED=$(sha256sum "$SCRATCH/home/.local/share/plasma/plasmoids/org.kde.latte.containment/contents/ui/applet/AppletItem.qml" | cut -d' ' -f1)
printf 'P4_PREFLIGHT original_AppletItem_sha256=%s diagnostic_AppletItem_sha256=%s\n' "$ORIGINAL" "$PATCHED"
cat > "$SCRATCH/bus.conf" <<EOF2
<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-Bus Bus Configuration 1.0//EN" "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig><type>session</type><keep_umask/><listen>unix:tmpdir=$SCRATCH/runtime</listen><auth>EXTERNAL</auth><policy context="default"><allow send_destination="*" eavesdrop="true"/><allow eavesdrop="true"/><allow own="*"/></policy></busconfig>
EOF2
cat > "$SCRATCH/session.sh" <<'EOF2'
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
TITLE=$(python3 "$BASE/tools/select_p4_internal_row.py" "$BASE/session/latte.log" "$BASE/session/internal-parity.json")
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
EOF2
chmod +x "$SCRATCH/session.sh"
systemd-run --user --collect --wait --pipe --unit=p4-fedora-preflight-20261116-r2 \
  -p PartOf=graphical-session.target -p After=graphical-session.target -p TimeoutStopSec=8s \
  -p RuntimeMaxSec=90s \
  /usr/bin/env -u DISPLAY -u WAYLAND_DISPLAY -u QT_QPA_PLATFORM HOME="$SCRATCH/home" XDG_CONFIG_HOME="$SCRATCH/home/.config" \
  XDG_CACHE_HOME="$SCRATCH/home/.cache" XDG_DATA_HOME="$SCRATCH/home/.local/share" \
  XDG_STATE_HOME="$SCRATCH/home/.local/state" XDG_DATA_DIRS="$PREFIX/share:/usr/local/share:/usr/share" \
  XDG_RUNTIME_DIR="$SCRATCH/runtime" TMPDIR="$SCRATCH/tmp" \
  QML_IMPORT_PATH="$PREFIX/lib64/qt6/qml:$PREFIX/lib64/qml" QT_PLUGIN_PATH="$PREFIX/lib64/plugins:$PREFIX/lib64/qt6/plugins" \
  QT_FORCE_STDERR_LOGGING=1 /usr/bin/dbus-run-session --config-file="$SCRATCH/bus.conf" \
  -- /bin/bash "$SCRATCH/session.sh"
