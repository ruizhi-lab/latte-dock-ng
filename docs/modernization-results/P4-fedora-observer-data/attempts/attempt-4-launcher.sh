#!/usr/bin/env bash
set -euo pipefail
BASE=/tmp/P4-fedora-20261116
PREFIX=/tmp/T4k-fedora-20261116/prefix
SCRATCH="$BASE/session"
rm -rf "$SCRATCH"
mkdir -p "$SCRATCH/home/.config" "$SCRATCH/home/.cache" "$SCRATCH/home/.local/share/plasma/plasmoids" "$SCRATCH/home/.local/state" "$SCRATCH/tmp" "$SCRATCH/runtime"
chmod 700 "$SCRATCH/runtime"
PLASMOID="$PREFIX/share/plasma/plasmoids/org.kde.latte.containment"
cp -a "$PLASMOID" "$SCRATCH/home/.local/share/plasma/plasmoids/"
python3 - "$SCRATCH/home/.local/share/plasma/plasmoids/org.kde.latte.containment/contents/ui/applet/AppletItem.qml" <<'PY'
import pathlib, sys
p = pathlib.Path(sys.argv[1])
s = p.read_text()
s = s.replace("applet.id", "appletItem.backendAppletId")
needle = '''    Component.onCompleted: {\n        checkIndex();\n'''
insert = '''    Component.onCompleted: {\n        console.warn("P4_APPLET_ROOT id=" + applet.id + " tracked=" + JSON.stringify(trackedWindowAppIds) + " loaderActive=" + trackedWindowsModelLoader.active + " indexer=" + indexerIsSupported);\n        checkIndex();\n'''
if s.count(needle) != 1:
    raise SystemExit("expected unique applet-root insertion point")
loader_needle = '''        active: !appletItem.indexerIsSupported\n                && appletItem.trackedWindowAppIds.length > 0\n                && !!latteView\n'''
loader_insert = '''        active: !appletItem.indexerIsSupported\n                && appletItem.trackedWindowAppIds.length > 0\n                && !!latteView\n        onActiveChanged: console.warn("P4_TRACKER_LOADER id=" + appletItem.applet.id + " active=" + active + " indexer=" + appletItem.indexerIsSupported + " appIds=" + JSON.stringify(appletItem.trackedWindowAppIds))\n'''
row_needle = '''            if (trackedWindowAppIds.indexOf(appId) < 0) {\n                continue;\n            }\n'''
row_insert = '''            if (trackedWindowAppIds.indexOf(appId) < 0) {\n                continue;\n            }\n            console.log("P4_TRACKER_ROW applet=" + applet.id + " appId=" + appId + " title=" + model.data(modelIndex, Qt.DisplayRole) + " winIdList=" + JSON.stringify(model.data(modelIndex, roles.WinIdList)) + " isWindow=" + model.data(modelIndex, roles.IsWindow) + " isStartup=" + model.data(modelIndex, roles.IsStartup) + " isActive=" + model.data(modelIndex, roles.IsActive) + " isMinimized=" + model.data(modelIndex, roles.IsMinimized));\n'''
if s.count(loader_needle) != 1 or s.count(row_needle) != 1:
    raise SystemExit("expected unique tracker insertion points")
p.write_text(s.replace(needle, insert).replace(loader_needle, loader_insert).replace(row_needle, row_insert))
PY
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
cleanup() { kill "$DOCK_PID" "$DOLPHIN_PID" "$ACT_PID" "$KWIN_PID" 2>/dev/null || true; wait "$DOCK_PID" "$DOLPHIN_PID" "$ACT_PID" "$KWIN_PID" 2>/dev/null || true; }
trap cleanup EXIT
for n in $(seq 1 100); do test -S "$XDG_RUNTIME_DIR/wayland-p4" && break; sleep .1; done
test -S "$XDG_RUNTIME_DIR/wayland-p4" || { echo P4_NO_GO kwin-socket-timeout; exit 2; }
export WAYLAND_DISPLAY=wayland-p4 QT_LOGGING_RULES='latte*=true;org.kde.taskmanager*=true' LANG=C.UTF-8
/usr/libexec/kactivitymanagerd >"$BASE/session/activity.log" 2>&1 & ACT_PID=$!
sleep 2
"$PREFIX/bin/latte-dock-ng" --debug --log-file "$BASE/session/latte.log" --import-layout "$BASE/data/two-trash.layout.latte" --suggested-layout-name 'P4 Fedora Fixture' >"$BASE/session/latte.stdout" 2>&1 & DOCK_PID=$!
sleep 8
mkdir -p "$BASE/session/window-title"
dolphin --new-window "$BASE/session/window-title" >"$BASE/session/dolphin.log" 2>&1 & DOLPHIN_PID=$!
sleep 12
printf 'P4_PREFLIGHT pids kwin=%s activity=%s dock=%s dolphin=%s\n' "$KWIN_PID" "$ACT_PID" "$DOCK_PID" "$DOLPHIN_PID"
grep -hE 'P4_TRACKER|P4_APPLET|AppletItem.qml|Failed|error|Error|WARNING|Warning' "$BASE/session/latte.stdout" "$BASE/session/latte.log" "$BASE/session/activity.log" "$BASE/session/kwin.log" "$BASE/session/dolphin.log" || true
if ! grep -q 'P4_TRACKER_ROW applet=3 ' "$BASE/session/latte.stdout" || ! grep -q 'P4_TRACKER_ROW applet=4 ' "$BASE/session/latte.stdout"; then
  echo P4_NO_GO internal-tracker-rows-not-observed
  exit 20
fi
set +e
P4_PRIVATE_SESSION=1 python3 "$BASE/tools/run_p4_taskmanager_observer.py" --qml /usr/lib64/qt6/bin/qml --app-id org.kde.dolphin --target-title 'window-title - Dolphin' --timeout-ms 12000 --stage-timeout-ms 3000 --log "$BASE/session/observer.jsonl"
STATUS=$?
printf 'P4_PREFLIGHT observer_exit=%s\n' "$STATUS"
grep -hE 'P4_EVENT|P4_TRACKER' "$BASE/session/observer.jsonl" "$BASE/session/latte.stdout" || true
exit "$STATUS"
EOF2
chmod +x "$SCRATCH/session.sh"
systemd-run --user --collect --wait --pipe --unit=p4-fedora-preflight-20261116-r2 \
  -p PartOf=graphical-session.target -p After=graphical-session.target -p TimeoutStopSec=8s \
  /usr/bin/env -u DISPLAY -u WAYLAND_DISPLAY -u QT_QPA_PLATFORM HOME="$SCRATCH/home" XDG_CONFIG_HOME="$SCRATCH/home/.config" \
  XDG_CACHE_HOME="$SCRATCH/home/.cache" XDG_DATA_HOME="$SCRATCH/home/.local/share" \
  XDG_STATE_HOME="$SCRATCH/home/.local/state" XDG_DATA_DIRS="$PREFIX/share:/usr/local/share:/usr/share" \
  XDG_RUNTIME_DIR="$SCRATCH/runtime" TMPDIR="$SCRATCH/tmp" \
  QML_IMPORT_PATH="$PREFIX/lib64/qt6/qml:$PREFIX/lib64/qml" QT_PLUGIN_PATH="$PREFIX/lib64/plugins:$PREFIX/lib64/qt6/plugins" \
  QT_FORCE_STDERR_LOGGING=1 /usr/bin/dbus-run-session --config-file="$SCRATCH/bus.conf" \
  -- /bin/bash "$SCRATCH/session.sh"
