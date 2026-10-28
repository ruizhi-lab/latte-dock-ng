#!/usr/bin/env python3
"""R5 helper: summarize a qmlprofiler .qtd trace (XML, version 1.02) without a GUI (runs anywhere).

For every event type the script prints how many ranges the trace holds and the time they cover, per second of
the recording as a coarse histogram, and the busiest events by total time with their file:line. Frames are
the "SceneGraph" ranges (render loop stages) and the "Painting"/"Animation" types of the profiler.
usage: r5-qtd-summary.py TRACE.qtd [--top 15] [--json]
"""

import argparse
import collections
import json
import xml.etree.ElementTree as ET


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("trace")
    parser.add_argument("--top", type=int, default=15)
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()

    root = ET.parse(args.trace).getroot()
    start, end = int(root.get("traceStart")), int(root.get("traceEnd"))
    events = {}
    for event in root.iter("event"):
        events[int(event.get("index"))] = {
            "type": (event.findtext("type") or "?"),
            "name": event.findtext("displayname") or "?",
            "file": (event.findtext("filename") or "").rsplit("/", 1)[-1],
            "line": event.findtext("line") or "",
            "details": (event.findtext("details") or "")[:70],
        }
    per_type = collections.Counter()
    time_per_type = collections.Counter()
    per_event = collections.Counter()
    time_per_event = collections.Counter()
    buckets: dict[str, collections.Counter] = collections.defaultdict(collections.Counter)
    for rng in root.iter("range"):
        index = int(rng.get("eventIndex"))
        event = events.get(index, {"type": "?"})
        kind = event["type"]
        begin = int(rng.get("startTime"))
        duration = int(rng.get("duration", "0"))
        per_type[kind] += 1
        time_per_type[kind] += duration
        per_event[index] += 1
        time_per_event[index] += duration
        buckets[kind][int((begin - start) / 1e9)] += 1
    seconds = max(1, round((end - start) / 1e9))
    result = {"trace_seconds": round((end - start) / 1e9, 2),
              "types": {k: {"ranges": per_type[k], "total_ms": round(time_per_type[k] / 1e6, 1),
                            "seconds_with_any": len(buckets[k])} for k in per_type}}
    if args.json:
        print(json.dumps(result, sort_keys=True))
        return 0
    print(f"trace span {result['trace_seconds']} s")
    print(f"{'type':22s} {'ranges':>8s} {'total ms':>10s} {'seconds with any':>17s}")
    for kind, count in per_type.most_common():
        print(f"{kind:22s} {count:8d} {time_per_type[kind] / 1e6:10.1f} {len(buckets[kind]):17d}")
    print("\nbusiest events by total time (ms), count, type, name, file:line, details")
    for index, total in time_per_event.most_common(args.top):
        e = events.get(index, {})
        print(f"{total / 1e6:9.1f} {per_event[index]:6d} {e.get('type', '?'):14s} {e.get('name', '?')[:40]:40s} {e.get('file', '')}:{e.get('line', '')} {e.get('details', '')}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
