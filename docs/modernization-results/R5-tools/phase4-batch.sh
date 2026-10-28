#!/bin/bash
# Diagnostics on the main code (VM side), unattended; the caller parks the pointer at the screen centre first.
#  1. session-bus traffic of the idle session with the dock running;
#  2. instrumented idle frame count (Wayland protocol log, Qt scene-graph log);
#  3. qmlprofiler traces of the idle dock and of the dock under a KSycoca storm (a Release build with QML debugging);
#  4. a gdb-sampled profile of the dock's main thread under the storm;
#  5. back to the plain main build.
raw="$HOME/latte-r5/raw"
tools="$HOME/latte-r5/tools"
layout="$HOME/.config/latte/我的布局.layout.latte"
pristine="$HOME/latte-r5/layout-pristine/我的布局.layout.latte"
always_visible() { sed 's/^visibility=.*/visibility=0/' "$pristine" > "$layout"; }
restore_layout() { cp -p "$pristine" "$layout"; sha256sum "$layout"; }
start_dock() { systemd-run --user --unit=latte-r5-dock --collect --quiet "$tools/launch-dock.sh" "$@"; }

echo "== 1. D-Bus traffic $(date -Is)"
"$tools/stop-dock.sh"; always_visible; start_dock; sleep 40
"$tools/dbus-rate.sh" 30 "$raw/R5-dbus-session-idle-dock-running.json"
"$tools/stop-dock.sh"; "$tools/dbus-rate.sh" 30 "$raw/R5-dbus-session-idle-dock-stopped.json"; restore_layout

echo "== 2. instrumented idle frames $(date -Is)"
"$tools/phase4-frames.sh" main-frames-idle

echo "== 3. qmlprofiler traces $(date -Is)"
"$tools/swap-install.sh" mainqml-eba91a3e6 || echo "swap to the QML debug build failed"
always_visible
systemd-run --user --unit=latte-r5-qmlprof --collect --quiet --wait "$tools/qml-profile.sh" "$raw/R5-qmlprofiler-idle.qtd" 30 30
systemd-run --user --unit=latte-r5-qmlprof --collect --quiet --wait --setenv=R5_STIMULUS="sycoca 0.5" "$tools/qml-profile.sh" "$raw/R5-qmlprofiler-sycoca-storm.qtd" 30 30
"$tools/r5-stim.sh" stop sycoca; "$tools/stop-dock.sh"; restore_layout

echo "== 4. gdb-sampled profiles of the dock main thread $(date -Is)"
# 4a. launched like P0 (bare SSH environment + dev-env.sh): the busy case
always_visible
pid="$(python3 -I "$tools/launch-for-sampling.py" p0-ssh)"; sleep 40
env -u DEBUGINFOD_URLS R5_GDB_OUT="$raw/R5-main-p0ssh-gdb-samples.txt" R5_GDB_SECONDS=150 R5_GDB_THREAD=1 \
    gdb -p "$pid" -batch -x "$tools/gdb-sampler.py" > "$raw/R5-main-p0ssh-gdb.log" 2>&1
"$tools/stop-dock.sh"; restore_layout
# 4b. session launcher plus the artificial KSycoca storm
always_visible; start_dock; sleep 35
"$tools/r5-stim.sh" start sycoca 0.5; sleep 12
pid="$(pgrep -x latte-dock-ng)"
env -u DEBUGINFOD_URLS R5_GDB_OUT="$raw/R5-main-sycoca-storm-gdb-samples.txt" R5_GDB_SECONDS=150 R5_GDB_THREAD=1 \
    gdb -p "$pid" -batch -x "$tools/gdb-sampler.py" > "$raw/R5-main-sycoca-storm-gdb.log" 2>&1
"$tools/r5-stim.sh" stop sycoca; "$tools/stop-dock.sh"; restore_layout

echo "== 5. back to the plain main build $(date -Is)"
"$tools/swap-install.sh" main-eba91a3e6 || echo "swap back failed"
echo "== done $(date -Is)"
