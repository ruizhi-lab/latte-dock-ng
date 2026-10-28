#!/bin/bash
# Second diagnostics round (VM side), unattended; the caller parks the pointer at the screen centre first.
#  1. qmlprofiler traces on the QML-debug Release build: settled idle dock (session launcher), the dock launched
#     the P0 way (bare SSH environment + dev-env.sh), and the session launcher under an artificial KSycoca storm;
#  2. back to the plain main build;
#  3. controls: the session launcher with dev-env.sh sourced (the Debug flow of AGENTS.md) and the P0-style launch
#     with debug logging switched on (--debug --log-file, latte*=true).
raw="$HOME/latte-r5/raw"
tools="$HOME/latte-r5/tools"
layout="$HOME/.config/latte/我的布局.layout.latte"
pristine="$HOME/latte-r5/layout-pristine/我的布局.layout.latte"
always_visible() { sed 's/^visibility=.*/visibility=0/' "$pristine" > "$layout"; }
restore_layout() { cp -p "$pristine" "$layout"; sha256sum "$layout"; }
revision="$(cat "$HOME/latte-r5/src-main-eba91a3e6/SOURCE_REVISION")"

echo "== 1. qmlprofiler traces $(date -Is)"
"$tools/stop-dock.sh"
"$tools/swap-install.sh" mainqml-eba91a3e6 || echo "swap to the QML debug build failed"
always_visible
# a first start only fills the QML cache that the uninstall removed
systemd-run --user --unit=latte-r5-dock --collect --quiet "$tools/launch-dock.sh"; sleep 60; "$tools/stop-dock.sh"
systemd-run --user --unit=latte-r5-qmlprof --collect --quiet --wait "$tools/qml-profile.sh" "$raw/R5-qmlprofiler-idle.qtd" 30
systemd-run --user --unit=latte-r5-qmlprof --collect --quiet --wait --setenv=R5_P0STYLE=1 "$tools/qml-profile.sh" "$raw/R5-qmlprofiler-p0style.qtd" 30
systemd-run --user --unit=latte-r5-qmlprof --collect --quiet --wait --setenv=R5_STIMULUS="sycoca 0.5" "$tools/qml-profile.sh" "$raw/R5-qmlprofiler-sycoca-storm.qtd" 30
"$tools/r5-stim.sh" stop sycoca; "$tools/stop-dock.sh"; restore_layout

echo "== 2. back to the plain main build $(date -Is)"
"$tools/swap-install.sh" main-eba91a3e6 || echo "swap back failed"

echo "== 3. controls $(date -Is)"
python3 -I "$tools/r5-scenario.py" --scenario main-systemd-dev-env --revision "$revision" --visibility 0 --out "$raw" --starts 2 --warmup 30 --seconds 60 --prewarm 20 --env R5_SOURCE_DEV_ENV=1
python3 -I "$tools/r5-scenario.py" --scenario main-p0ssh-debug-log --revision "$revision" --visibility 0 --out "$raw" --starts 2 --warmup 30 --seconds 60 --launcher p0-ssh \
    --env "QT_LOGGING_RULES=latte*=true" --dock-arg=--debug --dock-arg=--log-file --dock-arg=/tmp/r5-dock-p0ssh.log
echo "== done $(date -Is)"
