#!/bin/bash
# Stimulus matrix (VM side): for each label, swap the install, then record two starts per stimulus
# in the AlwaysVisible layout. The attention scenario is short because the dock stops its attention
# animation after 8.5 s (attentionTimer in plasmoid/package/contents/ui/main.qml).
# usage: stim-batch.sh label ...
raw="$HOME/latte-r5/raw"
run() {   # run <scenario> <extra runner arguments...>
    local scenario="$1"; shift
    python3 -I "$HOME/latte-r5/tools/r5-scenario.py" --scenario "$scenario" --revision "$revision" --visibility 0 --out "$raw" "$@"
}
for label in "$@"; do
    revision="$(cat "$HOME/latte-r5/src-$label/SOURCE_REVISION")"
    "$HOME/latte-r5/tools/swap-install.sh" "$label" || { echo "swap failed for $label"; continue; }
    run "$label-stim-ksmserver-stall" --starts 2 --warmup 30 --seconds 60 --prewarm 20 --stimulus ksmserver-stall
    run "$label-stim-sycoca" --starts 2 --warmup 30 --seconds 60 --stimulus "sycoca 0.5"
    run "$label-stim-zenity5" --starts 2 --warmup 30 --seconds 60 --stimulus "zenity 5"
    run "$label-stim-title50ms" --starts 2 --warmup 30 --seconds 60 --stimulus "title-churn 50"
    run "$label-stim-attention" --starts 2 --warmup 4 --seconds 20 --stimulus attention
done
for kind in ksmserver-stall sycoca zenity title-churn attention; do "$HOME/latte-r5/tools/r5-stim.sh" stop "$kind"; done
