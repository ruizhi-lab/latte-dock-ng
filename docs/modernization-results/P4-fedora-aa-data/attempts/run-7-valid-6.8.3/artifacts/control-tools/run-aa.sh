#!/usr/bin/env bash
# Execute one uninstrumented Release A/A run in a private nested desktop.
set -euo pipefail

BASE=/tmp/P4-fedora-20261116
PREFIX=/tmp/T4k-fedora-20261116/prefix
FIXTURE="$BASE/data/two-trash.layout.latte"
TOOLS="$BASE/tools"
RUN_ID=${1:?usage: run-aa.sh RUN_ID}
[[ "$RUN_ID" =~ ^[1-9][0-9]*$ ]] || {
  echo "RUN_ID must be a positive integer" >&2; exit 64;
}
RUN="$BASE/aa-run-$RUN_ID"
[[ ! -e "$RUN" ]] || { echo "refusing to overwrite retained run $RUN" >&2; exit 64; }
mkdir -p "$RUN/home/.config" "$RUN/home/.cache" "$RUN/home/.local/share" "$RUN/home/.local/state" "$RUN/tmp" "$RUN/runtime"
chmod 700 "$RUN/runtime"

EXE="$PREFIX/bin/latte-dock-ng"
EXPECTED_EXE_SHA=a86461b57ea69b4c2be145f4612a483e25fac329c11db6b3f978e44aaa675d60
EXPECTED_FIXTURE_SHA=90b974f266dbee2228c10315f0b86191cb42a7887da2ee7289979425aa07f506
EXPECTED_MANIFEST_SHA=5c91bfd3e032d69930f71c09720bae24146323f1fa30f17d6b178313d6e13ff9
EXPECTED_SOURCE_SHA=92607311df689f379ea27a56dcfcd9d7180db9131b80d2a97a797dc8c3665fc9
SOURCE_ARCHIVE=/tmp/T4k-fedora-20261116/source.tar.gz
[[ "$(sha256sum "$EXE" | cut -d' ' -f1)" == "$EXPECTED_EXE_SHA" ]]
[[ "$(sha256sum "$SOURCE_ARCHIVE" | cut -d' ' -f1)" == "$EXPECTED_SOURCE_SHA" ]]
[[ "$(sha256sum "$FIXTURE" | cut -d' ' -f1)" == "$EXPECTED_FIXTURE_SHA" ]]
MANIFEST="$BASE/data/install_manifest.txt"
[[ "$(sha256sum "$MANIFEST" | cut -d' ' -f1)" == "$EXPECTED_MANIFEST_SHA" ]]
mkdir -p "$RUN/artifacts"
mkdir -p "$RUN/artifacts/control-tools"
cp "$TOOLS/run-aa.sh" "$RUN/artifacts/control-tools/"
cp "$TOOLS/collect_aa.py" "$RUN/artifacts/control-tools/"
cp "$TOOLS/run_p4_taskmanager_observer.py" "$RUN/artifacts/control-tools/"
cp "$TOOLS/p4-taskmanager-observer.qml" "$RUN/artifacts/control-tools/"
HELPER_REFERENCE="$BASE/data/aa-reference-control-helper-sha256.txt"
sha256sum "$TOOLS/run-aa.sh" "$TOOLS/collect_aa.py" \
  "$TOOLS/run_p4_taskmanager_observer.py" "$TOOLS/p4-taskmanager-observer.qml" \
  > "$RUN/artifacts/control-helper-hashes.before"
if [[ ! -e "$HELPER_REFERENCE" ]]; then
  [[ "$RUN_ID" == 2 ]] || { echo "missing control-helper reference from run 2" >&2; exit 20; }
  cp "$RUN/artifacts/control-helper-hashes.before" "$HELPER_REFERENCE"
else
  cmp "$HELPER_REFERENCE" "$RUN/artifacts/control-helper-hashes.before"
fi
python3 - "$MANIFEST" "$PREFIX" "$RUN/artifacts/staged-qml-plugin-sha256.txt" <<'PY'
import hashlib, pathlib, sys
manifest, prefix, output = map(pathlib.Path, sys.argv[1:])
prefix_s = str(prefix)
rows = []
for raw in manifest.read_text().splitlines():
    path = pathlib.Path(raw)
    if not path.is_file() or not str(path).startswith(prefix_s + "/"):
        continue
    rel = path.relative_to(prefix)
    if rel.suffix in {".qml", ".so", ".qmltypes"} or path.name in {"qmldir", "plugins.qmltypes"}:
        rows.append((hashlib.sha256(path.read_bytes()).hexdigest(), str(rel)))
if not rows:
    raise SystemExit("no staged QML/plugin files found in install manifest")
output.write_text("".join(f"{digest}  {rel}\n" for digest, rel in sorted(rows)))
PY
sha256sum "$RUN/artifacts/staged-qml-plugin-sha256.txt" > "$RUN/artifacts/staged-qml-plugin-manifest.sha256.before"
QML_REFERENCE="$BASE/data/aa-reference-qml-plugin-sha256.txt"
if [[ ! -e "$QML_REFERENCE" ]]; then
  [[ "$RUN_ID" == 1 ]] || { echo "missing first-run staged payload reference" >&2; exit 20; }
  cp "$RUN/artifacts/staged-qml-plugin-sha256.txt" "$QML_REFERENCE"
else
  cmp "$QML_REFERENCE" "$RUN/artifacts/staged-qml-plugin-sha256.txt"
fi
cp "$FIXTURE" "$RUN/artifacts/two-trash.layout.latte"
sha256sum "$SOURCE_ARCHIVE" "$EXE" "$MANIFEST" "$FIXTURE" > "$RUN/artifacts/frozen-hashes.txt"
cat > "$RUN/bus.conf" <<EOF
<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-Bus Bus Configuration 1.0//EN" "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig><type>session</type><keep_umask/><listen>unix:tmpdir=$RUN/runtime</listen><auth>EXTERNAL</auth><policy context="default"><allow send_destination="*" eavesdrop="true"/><allow eavesdrop="true"/><allow own="*"/></policy></busconfig>
EOF
cat > "$RUN/session.sh" <<'SESSION'
#!/usr/bin/env bash
set -euo pipefail
BASE=/tmp/P4-fedora-20261116
PREFIX=/tmp/T4k-fedora-20261116/prefix
RUN="$BASE/aa-run-$P4_RUN_ID"
TOOLS="$BASE/tools"
EXE="$PREFIX/bin/latte-dock-ng"
QML_REFERENCE="$BASE/data/aa-reference-qml-plugin-sha256.txt"
HELPER_REFERENCE="$BASE/data/aa-reference-control-helper-sha256.txt"
KWIN_PID= ACT_PID= DOCK_PID= DOLPHIN_PID=
cleanup() {
  local status=$?
  trap - EXIT
  if [[ -n "$DOCK_PID" ]]; then
    kill -TERM "$DOCK_PID" 2>/dev/null || true
    for _ in $(seq 1 80); do
      DOCK_STATE=$(python3 - "$DOCK_PID" <<'PY'
from pathlib import Path
import sys
try:
    raw=Path('/proc', sys.argv[1], 'stat').read_text()
    print(raw[raw.rfind(')')+2:].split()[0])
except OSError:
    print('X')
PY
)
      [[ "$DOCK_STATE" == Z || "$DOCK_STATE" == X ]] && break
      sleep .1
    done
    if [[ "$DOCK_STATE" != Z && "$DOCK_STATE" != X ]]; then
      echo 'AA_INVALID_RUN: dock did not exit within 8 seconds of SIGTERM' | tee "$RUN/teardown-status.txt"
      kill -KILL "$DOCK_PID" 2>/dev/null || true
      status=1
    else
      echo 'dock-exited-after-SIGTERM' > "$RUN/teardown-status.txt"
    fi
    wait "$DOCK_PID" 2>/dev/null || true
  fi
  [[ -z "$DOLPHIN_PID" ]] || { kill "$DOLPHIN_PID" 2>/dev/null || true; wait "$DOLPHIN_PID" 2>/dev/null || true; }
  [[ -z "$ACT_PID" ]] || { kill "$ACT_PID" 2>/dev/null || true; wait "$ACT_PID" 2>/dev/null || true; }
  [[ -z "$KWIN_PID" ]] || { kill "$KWIN_PID" 2>/dev/null || true; wait "$KWIN_PID" 2>/dev/null || true; }
  exit "$status"
}
trap cleanup EXIT
kwin_wayland --virtual --width 1280 --height 800 --socket wayland-p4-aa --no-lockscreen --no-global-shortcuts >"$RUN/kwin.log" 2>&1 & KWIN_PID=$!
for _ in $(seq 1 100); do test -S "$XDG_RUNTIME_DIR/wayland-p4-aa" && break; sleep .1; done
test -S "$XDG_RUNTIME_DIR/wayland-p4-aa" || { echo no-go:kwin-socket-timeout; exit 20; }
export WAYLAND_DISPLAY=wayland-p4-aa LANG=C.UTF-8
/usr/libexec/kactivitymanagerd >"$RUN/activity.log" 2>&1 & ACT_PID=$!
sleep 2
# This uses only the staged installation. No user-QML overlay, debug flag,
# profiler, or opt-in Latte trace category is enabled during samples.
QT_LOGGING_RULES='latte*.debug=false;qml.debug=false' "$EXE" --import-layout "$RUN/artifacts/two-trash.layout.latte" --suggested-layout-name 'P4 Fedora Fixture' >"$RUN/latte.stdout" 2>"$RUN/latte.stderr" & DOCK_PID=$!
sleep 8
mkdir -p "$RUN/window-title"
dolphin --new-window "$RUN/window-title" >"$RUN/dolphin.log" 2>&1 & DOLPHIN_PID=$!
sleep 12
DOCK_START=$(python3 - "$DOCK_PID" <<'PY'
from pathlib import Path
import sys
s=Path('/proc', sys.argv[1], 'stat').read_text(); f=s[s.rfind(')')+2:].split(); print(f[19])
PY
)
# The private XDG_DATA_HOME starts empty and no plasmoid overlay is copied into
# it; the immutable staged prefix is the only Latte data/import root.
if [[ -e "$XDG_DATA_HOME/plasma/plasmoids/org.kde.latte.containment" ]]; then
  echo no-go:scratch-qml-override-visible; exit 20
fi
python3 "$TOOLS/collect_aa.py" --pid "$DOCK_PID" --starttime-ticks "$DOCK_START" --exe "$EXE" --identity-json >"$RUN/process-identity.json"
QT_LOGGING_RULES='qml.debug=true' P4_PRIVATE_SESSION=1 python3 "$TOOLS/run_p4_taskmanager_observer.py" \
  --qml /usr/lib64/qt6/bin/qml --app-id org.kde.dolphin \
  --target-title 'window-title — Dolphin' --timeout-ms 12000 --stage-timeout-ms 3000 \
  --log "$RUN/observer.jsonl"
sleep 30
python3 "$TOOLS/collect_aa.py" --pid "$DOCK_PID" --starttime-ticks "$DOCK_START" --exe "$EXE" \
  --duration 60 --interval 1 --cache-dir "$HOME/.cache" --output "$RUN/samples.csv"
sha256sum "$SOURCE_ARCHIVE" "$EXE" "$BASE/data/install_manifest.txt" "$BASE/data/two-trash.layout.latte" > "$RUN/artifacts/frozen-hashes.txt.after"
cmp "$RUN/artifacts/frozen-hashes.txt" "$RUN/artifacts/frozen-hashes.txt.after"
sha256sum "$TOOLS/run-aa.sh" "$TOOLS/collect_aa.py" \
  "$TOOLS/run_p4_taskmanager_observer.py" "$TOOLS/p4-taskmanager-observer.qml" \
  > "$RUN/artifacts/control-helper-hashes.after"
cmp "$RUN/artifacts/control-helper-hashes.before" "$RUN/artifacts/control-helper-hashes.after"
cmp "$HELPER_REFERENCE" "$RUN/artifacts/control-helper-hashes.after"
python3 - "$BASE/data/install_manifest.txt" "$PREFIX" "$RUN/artifacts/staged-qml-plugin-sha256.txt.after" <<'PY'
import hashlib, pathlib, sys
manifest, prefix, output = map(pathlib.Path, sys.argv[1:])
prefix_s = str(prefix)
rows = []
for raw in manifest.read_text().splitlines():
    path = pathlib.Path(raw)
    if path.is_file() and str(path).startswith(prefix_s + "/"):
        rel = path.relative_to(prefix)
        if rel.suffix in {".qml", ".so", ".qmltypes"} or path.name in {"qmldir", "plugins.qmltypes"}:
            rows.append((hashlib.sha256(path.read_bytes()).hexdigest(), str(rel)))
output.write_text("".join(f"{digest}  {rel}\n" for digest, rel in sorted(rows)))
PY
sha256sum "$RUN/artifacts/staged-qml-plugin-sha256.txt.after" > "$RUN/artifacts/staged-qml-plugin-manifest.sha256.after"
cmp "$RUN/artifacts/staged-qml-plugin-sha256.txt" "$RUN/artifacts/staged-qml-plugin-sha256.txt.after"
cmp "$QML_REFERENCE" "$RUN/artifacts/staged-qml-plugin-sha256.txt.after"
SESSION
chmod +x "$RUN/session.sh"

systemd-run --user --collect --wait --pipe --unit="p4-fedora-aa-$RUN_ID-20261116" \
  -p PartOf=graphical-session.target -p After=graphical-session.target \
  -p TimeoutStopSec=8s -p RuntimeMaxSec=180s \
  /usr/bin/env -u DISPLAY -u WAYLAND_DISPLAY -u QT_QPA_PLATFORM \
  HOME="$RUN/home" XDG_CONFIG_HOME="$RUN/home/.config" XDG_CACHE_HOME="$RUN/home/.cache" \
  XDG_DATA_HOME="$RUN/home/.local/share" XDG_STATE_HOME="$RUN/home/.local/state" \
  XDG_DATA_DIRS="$PREFIX/share:/usr/local/share:/usr/share" XDG_RUNTIME_DIR="$RUN/runtime" TMPDIR="$RUN/tmp" \
  QML_IMPORT_PATH="$PREFIX/lib64/qt6/qml:$PREFIX/lib64/qml" \
  QT_PLUGIN_PATH="$PREFIX/lib64/plugins:$PREFIX/lib64/qt6/plugins" \
  QT_LOGGING_RULES='latte*.debug=false;qml.debug=false' SOURCE_ARCHIVE="$SOURCE_ARCHIVE" \
  P4_RUN_ID="$RUN_ID" /usr/bin/dbus-run-session --config-file="$RUN/bus.conf" -- /bin/bash "$RUN/session.sh"
