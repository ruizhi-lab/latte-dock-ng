#!/bin/bash
# Stack samples of the dock's main thread (VM side), main build installed, AlwaysVisible layout:
#  A. the dock launched the P0 way (the busy case), B. the session launcher plus the artificial KSycoca storm.
# gdb attaches for every sample (the scripted single-attach sampler cannot run in gdb's batch mode), so the samples
# are slow but complete; the busy fraction of the main thread decides how many of them are useful.
# Also times the instrumented attention animation to turn the CPU of the animation into a per-frame cost.
raw="$HOME/latte-r5/raw"
tools="$HOME/latte-r5/tools"
layout="$HOME/.config/latte/我的布局.layout.latte"
pristine="$HOME/latte-r5/layout-pristine/我的布局.layout.latte"
always_visible() { sed 's/^visibility=.*/visibility=0/' "$pristine" > "$layout"; }
restore_layout() { cp -p "$pristine" "$layout"; sha256sum "$layout"; }

echo "== A. P0-style launch $(date -Is)"
always_visible
pid="$(python3 -I "$tools/launch-for-sampling.py" p0-ssh)"; sleep 40
echo "dock pid $pid; eu-stack probe:"; env -u DEBUGINFOD_URLS eu-stack -p "$pid" -1 -n 25 2>&1 | head -30
"$tools/stack-sample.sh" "$pid" 150 0.2 "$raw/R5-main-p0ssh-stacks.txt"
"$tools/stop-dock.sh"; restore_layout

echo "== B. session launcher + storm $(date -Is)"
always_visible
systemd-run --user --unit=latte-r5-dock --collect --quiet "$tools/launch-dock.sh"; sleep 35
"$tools/r5-stim.sh" start sycoca 0.5; sleep 12
pid="$(pgrep -x latte-dock-ng)"
"$tools/stack-sample.sh" "$pid" 150 0.2 "$raw/R5-main-sycoca-storm-stacks.txt"
"$tools/r5-stim.sh" stop sycoca; "$tools/stop-dock.sh"; restore_layout

echo "== C. instrumented attention animation $(date -Is)"
rm -f /tmp/r5-wl-main-frames-attention.log /tmp/r5-dock-main-frames-attention.log
revision="$(cat "$HOME/latte-r5/src-main-eba91a3e6/SOURCE_REVISION")"
python3 -I "$tools/r5-scenario.py" --scenario main-frames-attention --revision "$revision" --visibility 0 --starts 1 --warmup 4 --seconds 20 --out "$raw" \
    --env WAYLAND_DEBUG=client --env "R5_STDERR_FILE=/tmp/r5-wl-main-frames-attention.log" --env QSG_INFO=1 --env QSG_RENDER_TIMING=1 \
    --env "QT_LOGGING_RULES=latte*=true;qt.scenegraph.general=true;qt.scenegraph.time.renderloop=true;qt.scenegraph.time.renderer=true" \
    --dock-arg=--debug --dock-arg=--log-file --dock-arg=/tmp/r5-dock-main-frames-attention.log --stimulus attention
echo "== done $(date -Is)"
