#!/bin/bash
# Poor-man's profiler for the dock's main thread when perf is not installed (VM side, no root needed:
# ptrace_scope is 0 and the dock is the same user's process). Attaches with gdb, prints thread 1's stack,
# detaches, repeats. DEBUGINFOD_URLS is cleared so no sample waits for the network.
# usage: stack-sample.sh PID COUNT INTERVAL_SECONDS OUTFILE
pid="$1"; count="$2"; interval="${3:-0.3}"; out="$4"
: > "$out"
for i in $(seq 1 "$count"); do
    kill -0 "$pid" 2>/dev/null || { echo "process $pid is gone" >> "$out"; exit 1; }
    echo "=== sample $i $(date +%s.%N)" >> "$out"
    env -u DEBUGINFOD_URLS timeout 30 gdb -p "$pid" -batch -ex "set pagination off" -ex "thread 1" -ex "bt 40" -ex "detach" 2>&1 \
        | grep -E '^#[0-9]+ ' >> "$out"
    sleep "$interval"
done
