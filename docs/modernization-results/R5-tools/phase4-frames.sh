#!/bin/bash
# Instrumented single-start runs on the installed main build (VM side). The instrumentation changes timing, so
# these runs only count frames and record the renderer; their CPU figures are not compared with the plain series.
#  - Wayland protocol log (WAYLAND_DEBUG=client, to a file: libwayland prints it outside Qt's logging);
#  - the dock's own debug log with Qt Quick scene-graph categories (QSG_INFO, QSG_RENDER_TIMING and the
#    qt.scenegraph.* logging rules; the first is what the plan names, the second is what Qt 6 documents).
# usage: phase4-frames.sh SCENARIO [stimulus argument string]
scenario="$1"; stimulus="${2:-}"
raw="$HOME/latte-r5/raw"
revision="$(cat "$HOME/latte-r5/src-main-eba91a3e6/SOURCE_REVISION")"
rm -f "/tmp/r5-wl-$scenario.log" "/tmp/r5-dock-$scenario.log"
args=(--scenario "$scenario" --revision "$revision" --visibility 0 --starts 1 --warmup 30 --seconds 60 --out "$raw"
      --env WAYLAND_DEBUG=client --env "R5_STDERR_FILE=/tmp/r5-wl-$scenario.log"
      --env QSG_INFO=1 --env QSG_RENDER_TIMING=1
      --env "QT_LOGGING_RULES=latte*=true;qt.scenegraph.general=true;qt.scenegraph.time.renderloop=true;qt.scenegraph.time.renderer=true;qt.scenegraph.time.texture=true;qt.scenegraph.time.glyph=true;qt.rhi.general=true"
      --dock-arg=--debug --dock-arg=--log-file "--dock-arg=/tmp/r5-dock-$scenario.log")
[[ -n "$stimulus" ]] && args+=(--stimulus "$stimulus")
python3 -I "$HOME/latte-r5/tools/r5-scenario.py" "${args[@]}"
