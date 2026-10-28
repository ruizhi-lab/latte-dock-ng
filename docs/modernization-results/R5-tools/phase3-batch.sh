#!/bin/bash
# Variant matrix on the installed main build (VM side), P0 protocol: 30 s warmup, 60 s window, pointer parked at
# the screen centre by the caller, independent starts. The layout is restored byte for byte after every scenario.
#   hidden (AutoHide), the VM's own DodgeActive layout, software rendering, Plasma's panel alone (dock stopped),
#   and the P0 launch method (bare SSH environment) in three flavours.
raw="$HOME/latte-r5/raw"
revision="$(cat "$HOME/latte-r5/src-main-eba91a3e6/SOURCE_REVISION")"
run() {
    local scenario="$1"; shift
    python3 -I "$HOME/latte-r5/tools/r5-scenario.py" --scenario "$scenario" --revision "$revision" --out "$raw" "$@"
}
run main-hidden-autohide --visibility 1 --starts 5 --warmup 30 --seconds 60 --strip-shot
run main-dodgeactive-vm-layout --visibility 2 --starts 3 --warmup 30 --seconds 60 --strip-shot
run main-software-rendering --visibility 0 --starts 3 --warmup 30 --seconds 60 --strip-shot --env QT_QUICK_BACKEND=software
run main-panel-alone --no-dock --starts 3 --warmup 30 --seconds 60
run main-p0ssh-dev-env --visibility 0 --starts 3 --warmup 30 --seconds 60 --launcher p0-ssh --prewarm 20 --strip-shot
run main-p0ssh-bare-env --visibility 0 --starts 3 --warmup 30 --seconds 60 --launcher p0-ssh --env R5_SKIP_DEV_ENV=1
run main-p0ssh-debug-log --visibility 0 --starts 3 --warmup 30 --seconds 60 --launcher p0-ssh --env "QT_LOGGING_RULES=latte*=true" --dock-arg=--debug --dock-arg=--log-file --dock-arg=/tmp/r5-dock-p0ssh.log
