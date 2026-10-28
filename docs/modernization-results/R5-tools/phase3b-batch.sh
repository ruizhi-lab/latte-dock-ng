#!/bin/bash
# Why does the P0 launch method make the dock busy? (VM side, main build installed, AlwaysVisible layout)
# 1. watch the KSycoca rebuilds during a P0-style launch and during the session-environment launch;
# 2. bisect the environment: the session launcher with only XDG_CONFIG_DIRS (or LANG) changed;
# 3. the fix check: the P0-style launch with the KSycoca-relevant session variables imported.
raw="$HOME/latte-r5/raw"
revision="$(cat "$HOME/latte-r5/src-main-eba91a3e6/SOURCE_REVISION")"
run() {
    local scenario="$1"; shift
    python3 -I "$HOME/latte-r5/tools/r5-scenario.py" --scenario "$scenario" --revision "$revision" --visibility 0 --out "$raw" --warmup 30 --seconds 60 --watch-sycoca "$@"
}
SESSION_XDG="$(systemctl --user show-environment | sed -n 's/^XDG_CONFIG_DIRS=//p')"
run main-systemd-baseline-watch --starts 1
run main-p0ssh-dev-env-watch --starts 2 --launcher p0-ssh
run main-env-xdg-config-dirs-ssh-value --starts 2 --env "XDG_CONFIG_DIRS=$HOME/.config/kdedefaults:/etc/xdg"
run main-env-xdg-config-dirs-session-value --starts 1 --env "XDG_CONFIG_DIRS=$SESSION_XDG"
run main-env-lang-zh --starts 1 --env LANG=zh_CN.UTF-8 --env LANGUAGE=zh_CN
run main-p0ssh-dev-env-xdg-imported --starts 2 --launcher p0-ssh --env R5_IMPORT_XDG=1
