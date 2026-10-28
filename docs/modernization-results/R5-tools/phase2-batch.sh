#!/bin/bash
# Which applet reacts to KSycoca changes? (VM side, main build installed, AlwaysVisible layout)
#  - one rebuild inside a 30 s window: the cost of a single change event;
#  - the storm with the app dashboard (kickerdash) removed from the layout, with only it left, and with all four
#    applets removed. The layout is restored byte for byte after every scenario.
raw="$HOME/latte-r5/raw"
revision="$(cat "$HOME/latte-r5/src-main-eba91a3e6/SOURCE_REVISION")"
run() {
    local scenario="$1"; shift
    python3 -I "$HOME/latte-r5/tools/r5-scenario.py" --scenario "$scenario" --revision "$revision" --visibility 0 --out "$raw" "$@"
}
TASKS=org.kde.latte.plasmoid; APPMENU=org.kde.plasma.appmenu; TRASH=org.kde.plasma.trash; KICKER=org.kde.plasma.kickerdash
run main-sycoca-once --starts 3 --warmup 30 --seconds 30 --stimulus "sycoca-once 33"
run main-sycoca-storm-no-kickerdash --starts 2 --warmup 30 --seconds 60 --stimulus "sycoca 0.5" --remove-applet $KICKER
run main-sycoca-storm-only-kickerdash --starts 2 --warmup 30 --seconds 60 --stimulus "sycoca 0.5" --remove-applet $TASKS --remove-applet $APPMENU --remove-applet $TRASH
run main-sycoca-storm-no-applets --starts 1 --warmup 30 --seconds 60 --stimulus "sycoca 0.5" --remove-applet $TASKS --remove-applet $APPMENU --remove-applet $TRASH --remove-applet $KICKER
for kind in sycoca sycoca-once; do "$HOME/latte-r5/tools/r5-stim.sh" stop "$kind"; done
