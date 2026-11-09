#!/usr/bin/env bash
# R8j formal runs: scenarios I (idle control), W (window) and A (application), in turn.
# Each scenario is its own nested session (session.py), and analyze_cycles.py gives its
# verdict into the scenario's run directory. The scenarios run one after another so that
# they see the same load. Status lines go to <run root>/runs/status.txt.
#
#   bash run_scenarios.sh <run root> <probe .so> <scratch HOME with the installed dock>
#
# Example (detached):
#   setsid -f nohup bash run_scenarios.sh /data/tmp/r8j-20261110 \
#       /data/tmp/r8j-20261110/tools/r8j_trim.so /data/tmp/r8j-20261110/home
set -u

if [ "$#" -ne 3 ]; then
    echo "usage: bash run_scenarios.sh <run root> <probe .so> <scratch HOME>" >&2
    exit 2
fi
root=$1
probe=$2
template=$3
tools=$(cd "$(dirname "$0")" && pwd)
runs="$root/runs"
mkdir -p "$runs"
: > "$runs/status.txt"

for scenario in I W A; do
    out="$runs/$scenario"
    python3 -I "$tools/session.py" --scenario "$scenario" --out "$out" \
        --template "$template" --probe "$probe" > "$runs/$scenario.stdout" 2>&1
    echo "session $scenario exit=$? at $(date -Iseconds)" >> "$runs/status.txt"
    python3 -I "$tools/analyze_cycles.py" "$out" > "$runs/$scenario.analysis.stdout" 2>&1
    echo "analysis $scenario exit=$? at $(date -Iseconds)" >> "$runs/status.txt"
done
echo "done at $(date -Iseconds)" >> "$runs/status.txt"
