#!/bin/bash
# qmlprofiler traces in attach mode on the QML-debug Release build (VM side); see qml-profile.sh for why attach.
# Three 30 s traces of the settled dock: session launcher idle, the P0-style launch, and the session launcher under the
# artificial KSycoca storm. The plain main build is installed again at the end.
raw="$HOME/latte-r5/raw"
tools="$HOME/latte-r5/tools"
layout="$HOME/.config/latte/我的布局.layout.latte"
pristine="$HOME/latte-r5/layout-pristine/我的布局.layout.latte"
always_visible() { sed 's/^visibility=.*/visibility=0/' "$pristine" > "$layout"; }
restore_layout() { cp -p "$pristine" "$layout"; sha256sum "$layout"; }
echo "== swap to the QML debug build $(date -Is)"
"$tools/stop-dock.sh"
"$tools/swap-install.sh" mainqml-eba91a3e6 || echo "swap to the QML debug build failed"
always_visible
systemd-run --user --unit=latte-r5-dock --collect --quiet "$tools/launch-dock.sh"; sleep 60; "$tools/stop-dock.sh"   # fill the QML cache
echo "== traces $(date -Is)"
systemd-run --user --unit=latte-r5-qmlprof --collect --quiet --wait "$tools/qml-profile.sh" "$raw/R5-qmlprofiler-idle.qtd" 30
systemd-run --user --unit=latte-r5-qmlprof --collect --quiet --wait --setenv=R5_LAUNCHER=p0-ssh "$tools/qml-profile.sh" "$raw/R5-qmlprofiler-p0style.qtd" 30
systemd-run --user --unit=latte-r5-qmlprof --collect --quiet --wait --setenv=R5_STIMULUS="sycoca 0.5" "$tools/qml-profile.sh" "$raw/R5-qmlprofiler-sycoca-storm.qtd" 30
"$tools/r5-stim.sh" stop sycoca; "$tools/stop-dock.sh"; restore_layout
echo "== back to the plain main build $(date -Is)"
"$tools/swap-install.sh" main-eba91a3e6 || echo "swap back failed"
echo "== done $(date -Is)"
