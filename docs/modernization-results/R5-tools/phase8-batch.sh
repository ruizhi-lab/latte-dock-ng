#!/bin/bash
# Stack samples with eu-stack (a short stop per sample) of the busy dock, plus the plain sampler over the same minute
# (VM side, main build installed, AlwaysVisible layout). gdb's attach froze the dock for about 2 s per sample and its
# P0-style run saw 150 of 150 samples in ppoll, so the churn did not survive it.
#  A. the dock launched the P0 way;  B. the session launcher plus the artificial KSycoca storm.
raw="$HOME/latte-r5/raw"
tools="$HOME/latte-r5/tools"
layout="$HOME/.config/latte/我的布局.layout.latte"
pristine="$HOME/latte-r5/layout-pristine/我的布局.layout.latte"
always_visible() { sed 's/^visibility=.*/visibility=0/' "$pristine" > "$layout"; }
restore_layout() { cp -p "$pristine" "$layout"; sha256sum "$layout"; }
dock="$HOME/.local/bin/latte-dock-ng"

echo "== A. P0-style launch $(date -Is)"
always_visible
pid="$(python3 -I "$tools/launch-for-sampling.py" p0-ssh)"; sleep 40
python3 -I "$tools/r5-thread-sampler.py" --label p0ssh-eustack --process "dock=$dock" --process plasmashell=/usr/bin/plasmashell --process kwin=comm:kwin_wayland --seconds 60 --windows 1 > "$raw/R5-main-p0ssh-eustack-threads.jsonl" &
"$tools/eu-stack-sample.sh" "$pid" 400 0.03 "$raw/R5-main-p0ssh-eustack.txt"
wait
"$tools/stop-dock.sh"; restore_layout

echo "== B. session launcher + storm $(date -Is)"
always_visible
systemd-run --user --unit=latte-r5-dock --collect --quiet "$tools/launch-dock.sh"; sleep 35
"$tools/r5-stim.sh" start sycoca 0.5; sleep 12
pid="$(pgrep -x latte-dock-ng)"
python3 -I "$tools/r5-thread-sampler.py" --label storm-eustack --process "dock=$dock" --process plasmashell=/usr/bin/plasmashell --process kwin=comm:kwin_wayland --seconds 60 --windows 1 > "$raw/R5-main-sycoca-storm-eustack-threads.jsonl" &
"$tools/eu-stack-sample.sh" "$pid" 400 0.03 "$raw/R5-main-sycoca-storm-eustack.txt"
wait
"$tools/r5-stim.sh" stop sycoca; "$tools/stop-dock.sh"; restore_layout
echo "== done $(date -Is)"
