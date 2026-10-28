#!/bin/bash
# Idle A/B over several built revisions (VM side): for each label, swap the install, then record
# STARTS independent starts in the AlwaysVisible layout with the pointer parked by the caller.
# usage: ab-batch.sh STARTS label[:revision] ...   (labels as in build-only.sh)
starts="$1"; shift
for spec in "$@"; do
    label="${spec%%:*}"
    revision="$(cat "$HOME/latte-r5/src-$label/SOURCE_REVISION")"
    "$HOME/latte-r5/tools/swap-install.sh" "$label" || { echo "swap failed for $label"; continue; }
    python3 -I "$HOME/latte-r5/tools/r5-scenario.py" --scenario "$label-idle-visible" --revision "$revision" \
        --starts "$starts" --warmup 30 --seconds 60 --visibility 0 --prewarm 25 --out "$HOME/latte-r5/raw"
done
