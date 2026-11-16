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
