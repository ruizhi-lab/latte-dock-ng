#!/bin/bash
# Faster poor-man's profiler than stack-sample.sh: elfutils eu-stack reads thread 1's stack without loading gdb (about
# 0.15 s per sample on the VM). DEBUGINFOD_URLS is cleared so no sample waits for the network.
# usage: eu-stack-sample.sh PID COUNT INTERVAL_SECONDS OUTFILE
pid="$1"; count="$2"; interval="${3:-0.1}"; out="$4"
: > "$out"
for i in $(seq 1 "$count"); do
    kill -0 "$pid" 2>/dev/null || { echo "process $pid is gone" >> "$out"; exit 1; }
    echo "=== sample $i $(date +%s.%N)" >> "$out"
    env -u DEBUGINFOD_URLS timeout 10 eu-stack -p "$pid" -1 -n 40 2>/dev/null | grep -E '^#[0-9]+ ' >> "$out"
    sleep "$interval"
done
