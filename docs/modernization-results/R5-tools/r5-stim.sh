#!/bin/bash
# R5 stimuli for the Fedora VM; every one runs as a transient user unit so the Plasma session
# environment applies and `stop` removes exactly what `start` created.
#   r5-stim.sh start zenity [N]          N synthetic Zenity windows (P0's multi-window scenario), default 5
#   r5-stim.sh start title-churn [MS]    a Konsole window whose title changes every MS milliseconds (default 50)
#   r5-stim.sh start sycoca [PAUSE]      endless `kbuildsycoca6 --noincremental` with PAUSE seconds in between (default 0.5)
#   r5-stim.sh start sycoca-once         one `kbuildsycoca6 --noincremental` DELAY (default 2) seconds after start
#   r5-stim.sh start attention           mark every Dolphin window as demanding attention (KWin script)
#   r5-stim.sh start ksmserver-stall     SIGSTOP the session's ksmserver, so its D-Bus replies stop (stop = SIGCONT)
#   r5-stim.sh stop  <kind>              undo it
set -u
action="$1"; kind="$2"; arg="${3:-}"
unit_prefix="latte-r5-stim-$kind"

kwin_script() {   # kwin_script <name> <javascript text>; no output needed, the effect is the window state
    local js="/tmp/r5-stim-$1.js"
    printf '%s\n' "$2" > "$js"
    local id; id="$(qdbus-qt6 org.kde.KWin /Scripting loadScript "$js" "r5stim$1")"
    qdbus-qt6 org.kde.KWin "/Scripting/Script${id}" run >/dev/null
    sleep 0.5
    qdbus-qt6 org.kde.KWin /Scripting unloadScript "r5stim$1" >/dev/null
    rm -f "$js"
}

case "$action:$kind" in
start:zenity)
    for i in $(seq 1 "${arg:-5}"); do
        systemd-run --user --quiet --unit="${unit_prefix}-$i" --collect zenity --info --title="R5 window $i" --text="R5 synthetic window $i" --no-wrap
        sleep 0.4
    done ;;
stop:zenity)
    for unit in $(systemctl --user list-units --no-legend --plain "${unit_prefix}-*.service" | awk '{print $1}'); do systemctl --user stop "$unit"; done ;;
start:title-churn)
    ms="${arg:-50}"
    systemd-run --user --quiet --unit="$unit_prefix" --collect konsole -e bash -c \
      "i=0; while :; do printf '\\033]0;R5 title %d\\007' \$i; i=\$((i+1)); sleep 0.$(printf '%03d' "$ms"); done" ;;
stop:title-churn)
    systemctl --user stop "$unit_prefix.service" ;;
start:sycoca)
    systemd-run --user --quiet --unit="$unit_prefix" --collect bash -c "while :; do kbuildsycoca6 --noincremental >/dev/null 2>&1; sleep ${arg:-0.5}; done" ;;
stop:sycoca)
    systemctl --user stop "$unit_prefix.service"; kbuildsycoca6 --noincremental >/dev/null 2>&1 ;;
start:ksmserver-stall)
    # A stopped ksmserver keeps its bus name but never answers. The pre-R1a session poller asked it
    # isShuttingDown() with a blocking call every 500 ms. SIGCONT in `stop` (and never leave it stopped).
    # A transient timer sends SIGCONT after 5 minutes even if nobody calls `stop`.
    systemd-run --user --quiet --unit=latte-r5-ksm-deadman --collect --on-active=300 \
        systemctl --user kill --signal=SIGCONT --kill-whom=main plasma-ksmserver.service
    systemctl --user kill --signal=SIGSTOP --kill-whom=main plasma-ksmserver.service ;;
stop:ksmserver-stall)
    systemctl --user kill --signal=SIGCONT --kill-whom=main plasma-ksmserver.service
    systemctl --user stop latte-r5-ksm-deadman.timer 2>/dev/null; true ;;
start:sycoca-once)
    # One rebuild, DELAY seconds (default 2) after `start` returns, so a sampling window contains exactly one KSycoca change.
    systemd-run --user --quiet --unit="$unit_prefix" --collect bash -c "sleep ${arg:-2}; kbuildsycoca6 --noincremental >/dev/null 2>&1" ;;
stop:sycoca-once)
    systemctl --user stop "$unit_prefix.service" 2>/dev/null; true ;;
start:attention)
    kwin_script attention-on 'var w = workspace.windowList(); for (var i = 0; i < w.length; i++) { if (w[i].resourceClass == "org.kde.dolphin") { w[i].demandsAttention = true; } }' ;;
stop:attention)
    kwin_script attention-off 'var w = workspace.windowList(); for (var i = 0; i < w.length; i++) { if (w[i].resourceClass == "org.kde.dolphin") { w[i].demandsAttention = false; } }' ;;
*)
    echo "usage: $0 start|stop zenity|title-churn|sycoca|sycoca-once|attention|ksmserver-stall [arg]" >&2; exit 2 ;;
esac
