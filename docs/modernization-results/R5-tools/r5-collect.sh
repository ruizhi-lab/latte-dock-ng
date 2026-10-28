#!/bin/bash
# Gather the raw records of R5 from the Fedora VM into docs/modernization-results/R5-data (runs on the developer host).
# Large text logs are compressed, qmlprofiler traces are replaced by their summary plus the compressed trace when it is
# small, the host monitor is trimmed to the samples around the recorded windows, and a SHA-256 manifest is written.
# usage: r5-collect.sh [VM] [HOST_MONITOR.jsonl]
set -eu
vm="${1:-fedora@192.168.122.128}"
monitor="${2:-/tmp/r5-host-monitor.jsonl}"
here="$(cd "$(dirname "$0")" && pwd)"
out="$here/../R5-data"
rm -rf "$out"; mkdir -p "$out/logs" "$out/qml"
scp -q "$vm:latte-r5/raw/R5-*.jsonl" "$vm:latte-r5/raw/R5-*.json" "$vm:latte-r5/raw/R5-*.txt" "$out/" 2>/dev/null || true
scp -q "$vm:latte-r5/R5-environment-part1.txt" "$vm:latte-r5/R5-environment-part2.txt" "$out/"
# strips: keep the first start of every scenario
mkdir -p "$out/strips"
for s in $(ssh "$vm" 'ls ~/latte-r5/raw/ | grep -E "start1-strip.png$"'); do scp -q "$vm:latte-r5/raw/$s" "$out/strips/"; done
# logs
for f in $(ssh "$vm" 'ls ~/latte-r5/logs/*.log'); do ssh "$vm" "cat $f" | gzip -9 > "$out/logs/$(basename "$f").gz"; done
for f in $(ssh "$vm" 'ls /tmp/r5-wl-*.log /tmp/r5-dock-*.log 2>/dev/null'); do ssh "$vm" "cat $f" | gzip -9 > "$out/logs/$(basename "$f").gz"; done
# qmlprofiler traces: summary json always, the trace itself (compressed) when it is below 3 MB
for t in $(ssh "$vm" 'ls ~/latte-r5/raw/R5-qmlprofiler-*.qtd 2>/dev/null'); do
    name="$(basename "$t")"
    ssh "$vm" "cat $t" > "/tmp/$name"
    python3 -I "$here/r5-qtd-summary.py" "/tmp/$name" --json > "$out/qml/${name%.qtd}.summary.json"
    python3 -I "$here/r5-qtd-summary.py" "/tmp/$name" --top 25 > "$out/qml/${name%.qtd}.summary.txt"
    if [[ "$(stat -c %s "/tmp/$name")" -lt 3000000 ]]; then gzip -9 < "/tmp/$name" > "$out/qml/$name.gz"; fi
    rm -f "/tmp/$name"
done
for f in $(ssh "$vm" 'ls ~/latte-r5/raw/R5-qmlprofiler-*.notes.log 2>/dev/null'); do scp -q "$vm:$f" "$out/qml/"; done
# host monitor, trimmed
python3 -I "$here/r5-trim-host.py" "$monitor" "$out/R5-host-monitor-windows.jsonl" "$out"/R5-*-family.jsonl
( cd "$out" && find . -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SHA256SUMS )
du -sh "$out"; ls "$out" | wc -l
