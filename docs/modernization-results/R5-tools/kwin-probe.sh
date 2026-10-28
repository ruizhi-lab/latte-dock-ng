#!/bin/bash
# Run the read-only KWin probe once and print what it reports. KWin's print() is not routed
# to the journal, so the script sends its report as a D-Bus call that busctl monitors.
name="r5probe$$"
out="$(mktemp)"
timeout 4 busctl --user monitor --no-pager --json=short >"$out" 2>/dev/null &
mon=$!
sleep 0.6
id="$(qdbus-qt6 org.kde.KWin /Scripting loadScript "$HOME/latte-r5/tools/kwin-probe.js" "$name")"
qdbus-qt6 org.kde.KWin "/Scripting/Script${id}" run >/dev/null
sleep 1
qdbus-qt6 org.kde.KWin /Scripting unloadScript "$name" >/dev/null
kill "$mon" 2>/dev/null; wait "$mon" 2>/dev/null
grep -a 'R5PROBE' "$out" | tail -1 | python3 -c '
import json,sys
for line in sys.stdin:
    try:
        m=json.loads(line)
        print(m["payload"]["data"][0])
    except Exception as e:
        print("unparsed:", line[:300])'
rm -f "$out"
