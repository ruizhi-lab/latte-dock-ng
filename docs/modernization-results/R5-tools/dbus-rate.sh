#!/bin/bash
# Count what crosses the Plasma session bus for SECONDS (VM side): total messages, and the busiest
# senders / members, so the D-Bus traffic of an idle session can be compared with the dock's wakeups.
# Names only, no payload is kept. usage: dbus-rate.sh SECONDS OUTFILE
secs="${1:-30}"; out="$2"
tmp="$(mktemp)"
timeout "$secs" busctl --user monitor --no-pager --json=short > "$tmp" 2>/dev/null
python3 -I - "$tmp" "$secs" > "$out" <<'PY'
import collections, json, sys
msgs = [json.loads(l) for l in open(sys.argv[1]) if l.strip().startswith("{")]
seconds = float(sys.argv[2])
members = collections.Counter((m.get("type"), m.get("interface"), m.get("member")) for m in msgs)
senders = collections.Counter(m.get("sender") for m in msgs)
print(json.dumps({"seconds": seconds, "messages": len(msgs), "per_second": round(len(msgs) / seconds, 2),
                  "top_members": [[list(k), v] for k, v in members.most_common(12)],
                  "top_senders": senders.most_common(8)}, indent=1))
PY
rm -f "$tmp"
