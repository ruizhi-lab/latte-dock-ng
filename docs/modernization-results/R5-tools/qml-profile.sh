#!/bin/bash
# Record a QML profile of the user-installed dock with qmlprofiler in ATTACH mode (VM side). The installed dock must be a
# build with QML debugging enabled (a Debug build, or Release with -DQT_QML_DEBUG, the "mainqml" build of R5).
#
# Why attach: when qmlprofiler starts the program itself it adds `block`, which holds the QML engine until the first
# recording starts, so a "30 s idle trace" contains the whole application start (a 23 MB trace of compile, creation and
# binding events in R5's first attempt). Here the dock starts on its own with a local debug port, settles, and the
# profiler attaches to the running process and records only the window.
#
# The dock is launched with launch-for-sampling.py: R5_LAUNCHER=systemd (default) or p0-ssh. Recording starts when the dock's
# CPU has settled (below 40 % and stable for three 5 s checks, after at least R5_MIN_WAIT seconds, default 45).
# R5_STIMULUS="sycoca 0.5" starts that r5-stim.sh stimulus with the recording and stops it afterwards.
# usage: qml-profile.sh TRACE_FILE [SECONDS=30]
set -u
trace="$1"; seconds="${2:-30}"
log="${trace%.qtd}.profiler.log"; notes="${trace%.qtd}.notes.log"
tools="$HOME/latte-r5/tools"
: > "$notes"
pid="$(python3 -I "$tools/launch-for-sampling.py" "${R5_LAUNCHER:-systemd}" "--dock-arg=-qmljsdebugger=port:3768,host:127.0.0.1,services:CanvasFrameRate")"
echo "dock pid $pid launcher ${R5_LAUNCHER:-systemd} at $(date -Is)" >> "$notes"
start="$(date +%s)"; ticks_before=0; stable=0; hz="$(getconf CLK_TCK)"
while :; do
    sleep 5
    ticks="$(awk '{print $14 + $15}' "/proc/$pid/stat" 2>/dev/null || echo 0)"
    percent=$(( (ticks - ticks_before) * 100 / hz / 5 )); ticks_before="$ticks"
    elapsed=$(( $(date +%s) - start ))
    if (( elapsed >= ${R5_MIN_WAIT:-45} && percent < 40 )); then stable=$((stable + 1)); else stable=0; fi
    (( stable >= 3 )) && break
    (( elapsed > 240 )) && { echo "dock did not settle within 240 s" >> "$notes"; break; }
done
echo "settled after $(( $(date +%s) - start )) s at ${percent} % CPU" >> "$notes"
fifo="$(mktemp -u /tmp/r5-qmlprofiler-XXXXXX)"; mkfifo "$fifo"
# qmlprofiler asks "Overwrite (y/n)?" on stdin when the trace file exists and then spins on the closed fifo, so remove it.
rm -f "$trace"
/usr/lib64/qt6/bin/qmlprofiler --attach 127.0.0.1 --port 3768 --record off --interactive -o "$trace" < "$fifo" > "$log" 2>&1 &
profiler=$!
exec 3>"$fifo"
sleep 4
[[ -n "${R5_STIMULUS:-}" ]] && "$tools/r5-stim.sh" start $R5_STIMULUS
echo "record start $(date -Is)" >> "$notes"
echo r >&3
sleep "$seconds"
echo "record stop $(date -Is)" >> "$notes"
[[ -n "${R5_STIMULUS:-}" ]] && "$tools/r5-stim.sh" stop "${R5_STIMULUS%% *}"
echo "f $trace" >&3
sleep 10
echo q >&3
exec 3>&-
wait "$profiler"
rm -f "$fifo"
"$tools/stop-dock.sh"
