#!/usr/bin/env bash
set -euo pipefail

# Run only after reviewing this script. The source package and imported layout
# must belong to the same isolated build; nothing is copied into a live user's
# HOME or system installation.
: "${P3A_PREFIX:?set P3A_PREFIX to the isolated Fedora install prefix}"
: "${P3A_LAYOUT:?set P3A_LAYOUT to a disposable layout fixture with the Tasks plasmoid}"

ROOT="${P3A_ROOT:-/tmp}"
PACKAGE_ID=org.kde.latte.plasmoid
SOURCE_PACKAGE="$P3A_PREFIX/share/plasma/plasmoids/$PACKAGE_ID"
CONTAINMENT_PACKAGE="$P3A_PREFIX/share/plasma/plasmoids/org.kde.latte.containment"
DOCK="$P3A_PREFIX/bin/latte-dock-ng"
QML_MODULES="$P3A_PREFIX/lib64/qt6/qml:$P3A_PREFIX/lib64/qml"
QT_PLUGINS="$P3A_PREFIX/lib64/plugins:$P3A_PREFIX/lib64/qt6/plugins"

for required in "$SOURCE_PACKAGE/contents/ui/main.qml" "$CONTAINMENT_PACKAGE/contents/ui/main.qml" "$DOCK" "$P3A_LAYOUT"; do
    [[ -e "$required" ]] || { printf 'P3A_NO_GO missing=%s\n' "$required" >&2; exit 2; }
done
command -v kwin_wayland >/dev/null
command -v dbus-run-session >/dev/null
command -v systemd-run >/dev/null

BASE=$(mktemp -d "$ROOT/P3a-private-XXXXXX")
SESSION="$BASE/session"
HOME_DIR="$SESSION/home"
RUNTIME="$SESSION/runtime"
OVERLAY="$HOME_DIR/.local/share/plasma/plasmoids/$PACKAGE_ID"
CONTAINMENT_OVERLAY="$HOME_DIR/.local/share/plasma/plasmoids/org.kde.latte.containment"
mkdir -p "$HOME_DIR/.config" "$HOME_DIR/.cache" "$HOME_DIR/.local/share/plasma/plasmoids" \
    "$HOME_DIR/.local/state" "$SESSION/tmp" "$RUNTIME"
chmod 700 "$RUNTIME"
cp -a "$SOURCE_PACKAGE" "$OVERLAY"
cp -a "$CONTAINMENT_PACKAGE" "$CONTAINMENT_OVERLAY"
cp -- "$P3A_LAYOUT" "$SESSION/probe-layout.layout.latte"

SOURCE_SHA=$(sha256sum "$SOURCE_PACKAGE/contents/ui/main.qml" | cut -d' ' -f1)
CONTAINMENT_SHA=$(sha256sum "$CONTAINMENT_PACKAGE/contents/ui/main.qml" | cut -d' ' -f1)
python3 "$(dirname "$0")/instrument_p3a_qml.py" \
    "$OVERLAY/contents/ui/main.qml" "$CONTAINMENT_OVERLAY/contents/ui/main.qml"
OVERLAY_SHA=$(sha256sum "$OVERLAY/contents/ui/main.qml" | cut -d' ' -f1)
CONTAINMENT_OVERLAY_SHA=$(sha256sum "$CONTAINMENT_OVERLAY/contents/ui/main.qml" | cut -d' ' -f1)
printf 'P3A_STAGE task_source_sha256=%s task_overlay_sha256=%s containment_source_sha256=%s containment_overlay_sha256=%s\n' \
    "$SOURCE_SHA" "$OVERLAY_SHA" "$CONTAINMENT_SHA" "$CONTAINMENT_OVERLAY_SHA"
printf 'P3A_OVERLAYS tasks=%s containment=%s\n' "$OVERLAY/contents/ui/main.qml" "$CONTAINMENT_OVERLAY/contents/ui/main.qml"

cat >"$SESSION/bus.conf" <<EOF
<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-Bus Bus Configuration 1.0//EN" "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig><type>session</type><keep_umask/><listen>unix:tmpdir=$RUNTIME</listen><auth>EXTERNAL</auth><policy context="default"><allow send_destination="*" eavesdrop="true"/><allow eavesdrop="true"/><allow own="*"/></policy></busconfig>
EOF

cat >"$SESSION/run.sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
SESSION=${P3A_SESSION:?}
PREFIX=${P3A_PREFIX:?}
kwin_wayland --virtual --width 1280 --height 800 --socket wayland-p3a \
    --no-lockscreen --no-global-shortcuts >"$SESSION/kwin.log" 2>&1 & KWIN_PID=$!
DOCK_PID=
ACTIVITY_PID=
cleanup() {
    [[ -z "$DOCK_PID" ]] || { kill "$DOCK_PID" 2>/dev/null || true; wait "$DOCK_PID" 2>/dev/null || true; }
    [[ -z "$ACTIVITY_PID" ]] || { kill "$ACTIVITY_PID" 2>/dev/null || true; wait "$ACTIVITY_PID" 2>/dev/null || true; }
    kill "$KWIN_PID" 2>/dev/null || true
    wait "$KWIN_PID" 2>/dev/null || true
}
trap cleanup EXIT
for _ in $(seq 1 100); do [[ -S "$XDG_RUNTIME_DIR/wayland-p3a" ]] && break; sleep .1; done
[[ -S "$XDG_RUNTIME_DIR/wayland-p3a" ]] || { echo P3A_NO_GO kwin-socket-timeout; exit 3; }
export WAYLAND_DISPLAY=wayland-p3a
export QT_LOGGING_RULES='latte*=true;qml.debug=true'
export LANG=C.UTF-8
if [[ -x /usr/libexec/kactivitymanagerd ]]; then
    /usr/libexec/kactivitymanagerd >"$SESSION/activity.log" 2>&1 & ACTIVITY_PID=$!
fi
"$PREFIX/bin/latte-dock-ng" --debug --log-file "$SESSION/latte.log" \
    --import-layout "$SESSION/probe-layout.layout.latte" --suggested-layout-name 'P3a Private Probe' \
    >"$SESSION/latte.stdout" 2>&1 & DOCK_PID=$!
sleep "${P3A_OBSERVE_SECONDS:-40}"
printf 'P3A_PROBE_PIDS kwin=%s activity=%s dock=%s\n' "$KWIN_PID" "${ACTIVITY_PID:-none}" "$DOCK_PID"
grep -hE 'P3A_SOURCE_LOADED|P3A_EDGE_SIGNAL|P3A_REFRESH_PASS|SOURCE:' \
    "$SESSION/latte.log" "$SESSION/latte.stdout" || true
if ! grep -hFq 'P3A_SOURCE_LOADED' "$SESSION/latte.log" "$SESSION/latte.stdout"; then
    echo P3A_NO_GO overlay-source-marker-not-observed
    exit 20
fi
if ! grep -hFq "$HOME/.local/share/plasma/plasmoids/org.kde.latte.plasmoid/contents/ui/main.qml" \
        "$SESSION/latte.log" "$SESSION/latte.stdout"; then
    echo P3A_NO_GO loaded-url-did-not-identify-private-overlay
    exit 21
fi
python3 - "$SESSION/latte.log" <<'PY'
import pathlib
import sys

lines = pathlib.Path(sys.argv[1]).read_text(errors="replace").splitlines()
for edge, value in (("TopEdge", "3"), ("BottomEdge", "4")):
    request = f"P3A_DRIVER_REQUEST edge={edge} value={value}"
    request_at = next((i for i, line in enumerate(lines) if request in line), None)
    if request_at is None:
        print(f"P3A_EDGE_RESULT edge={edge} driver=NOT_OBSERVED signal=NOT_OBSERVED refresh=NOT_OBSERVED")
        continue
    signal_at = next((i for i in range(request_at + 1, len(lines))
                      if f"P3A_EDGE_SIGNAL locationChanged value={value}" in lines[i]), None)
    if signal_at is None:
        print(f"P3A_EDGE_RESULT edge={edge} driver=OBSERVED signal=NOT_OBSERVED refresh=NOT_OBSERVED")
        continue
    refresh_at = next((i for i in range(signal_at + 1, len(lines))
                       if "P3A_REFRESH_PASS reason=locationChanged pass=0" in lines[i]), None)
    if refresh_at is None:
        print(f"P3A_EDGE_RESULT edge={edge} driver=OBSERVED signal=OBSERVED refresh=NOT_OBSERVED")
    else:
        print(f"P3A_EDGE_RESULT edge={edge} value={value} driver=OBSERVED signal=OBSERVED refresh=OBSERVED")
PY
EOF
chmod +x "$SESSION/run.sh"

printf 'P3A_PRIVATE_SESSION artifacts=%s\n' "$SESSION"
systemd-run --user --collect --wait --pipe --unit="p3a-private-probe-$(date +%s)" \
    -p PartOf=graphical-session.target -p After=graphical-session.target \
    -p TimeoutStopSec=8s -p RuntimeMaxSec=90s \
    /usr/bin/env -u DISPLAY -u WAYLAND_DISPLAY -u QT_QPA_PLATFORM \
    HOME="$HOME_DIR" XDG_CONFIG_HOME="$HOME_DIR/.config" XDG_CACHE_HOME="$HOME_DIR/.cache" \
    XDG_DATA_HOME="$HOME_DIR/.local/share" XDG_STATE_HOME="$HOME_DIR/.local/state" \
    XDG_DATA_DIRS="$P3A_PREFIX/share:/usr/local/share:/usr/share" \
    XDG_SESSION_TYPE=wayland \
    XDG_RUNTIME_DIR="$RUNTIME" TMPDIR="$SESSION/tmp" \
    QML_IMPORT_PATH="$QML_MODULES" QT_PLUGIN_PATH="$QT_PLUGINS" \
    P3A_SESSION="$SESSION" P3A_PREFIX="$P3A_PREFIX" \
    QT_FORCE_STDERR_LOGGING=1 /usr/bin/dbus-run-session --config-file="$SESSION/bus.conf" \
    -- /bin/bash "$SESSION/run.sh"

printf 'P3A_ARTIFACTS=%s\n' "$SESSION"
printf 'P3A_FINAL_HASHES task_source=%s task_overlay=%s containment_source=%s containment_overlay=%s\n' \
    "$(sha256sum "$SOURCE_PACKAGE/contents/ui/main.qml" | cut -d' ' -f1)" \
    "$(sha256sum "$OVERLAY/contents/ui/main.qml" | cut -d' ' -f1)" \
    "$(sha256sum "$CONTAINMENT_PACKAGE/contents/ui/main.qml" | cut -d' ' -f1)" \
    "$(sha256sum "$CONTAINMENT_OVERLAY/contents/ui/main.qml" | cut -d' ' -f1)"
