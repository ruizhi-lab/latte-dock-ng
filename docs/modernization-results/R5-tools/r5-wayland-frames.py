#!/usr/bin/env python3
"""R5 helper: count the frames a Wayland client really sent, from a WAYLAND_DEBUG=client log.

libwayland prints every request and event of the client when WAYLAND_DEBUG=1 (or
client) is set and writes it to stderr, outside Qt's logging, so the count does
not depend on the dock's message filter and works for Release builds. A frame
that reached the compositor is a wl_surface.commit() that follows an attach();
a compositor-paced frame is answered by a wl_callback.done event.

The script reports, over the requested time span (relative to the first
timestamp in the file, or to --from/--to in seconds of the log's own clock):

* commits, commits with a new buffer, frame requests and frame callbacks, in
  total and per second, and the busiest and quietest second;
* the same per wl_surface id, with the mean damaged area of a frame, so the
  dock's view surface can be told from the edge ghost window and from popups;
* the input events (pointer motion/enter/leave/button/axis) received, because
  the dock can animate only in answer to input or to its own timers.

Output is one JSON object on stdout.
"""

from __future__ import annotations

import argparse
import collections
import json
from pathlib import Path
import re
import statistics

# libwayland prints either "[ms.fraction]" (monotonic milliseconds, older versions) or "[HH:MM:SS.microseconds]"
# (1.26 on the Fedora VM); both are converted to seconds.
LINE = re.compile(r"^\[\s*(?P<ts>\d+(?::\d+:\d+)?\.\d+)\]\s+(?:\{[^}]*\}\s+)?(?P<dir>->)?\s*(?P<obj>\w+)[@#](?P<id>\d+)\.(?P<msg>\w+)\((?P<args>.*)\)\s*$")


def seconds(stamp: str) -> float:
    if ":" in stamp:
        hours, minutes, rest = stamp.split(":")
        return int(hours) * 3600 + int(minutes) * 60 + float(rest)
    return float(stamp) / 1000.0


def parse(path: Path):
    for raw in path.read_text(errors="replace").splitlines():
        match = LINE.match(raw)
        if not match:
            continue
        yield (seconds(match["ts"]), match["dir"] == "->", match["obj"], int(match["id"]), match["msg"], match["args"])


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("--from", dest="start", type=float, help="Start second, relative to the first line of the log.")
    parser.add_argument("--to", dest="end", type=float, help="End second, relative to the first line of the log.")
    args = parser.parse_args()

    events = list(parse(args.log))
    if not events:
        print(json.dumps({"error": "no wayland debug lines found", "log": str(args.log)}))
        return 1
    origin = events[0][0]
    start = origin + (args.start or 0.0)
    end = origin + args.end if args.end is not None else events[-1][0]
    span = max(end - start, 1e-9)

    totals = collections.Counter()
    per_second: dict[int, collections.Counter] = collections.defaultdict(collections.Counter)
    surfaces: dict[int, collections.Counter] = collections.defaultdict(collections.Counter)
    damage_area: dict[int, list[int]] = collections.defaultdict(list)
    pending_damage: dict[int, int] = collections.defaultdict(int)
    pending_attach: dict[int, bool] = collections.defaultdict(bool)
    input_events = collections.Counter()

    for ts, is_request, obj, ident, msg, margs in events:
        if ts < start or ts > end:
            continue
        second = int(ts - start)
        if obj == "wl_surface" and is_request:
            if msg == "attach":
                pending_attach[ident] = "nil" not in margs and not margs.startswith("null")
            elif msg in ("damage", "damage_buffer"):
                numbers = [int(v) for v in re.findall(r"-?\d+", margs)]
                if len(numbers) >= 4:
                    if abs(numbers[2]) > 100000 or abs(numbers[3]) > 100000:
                        pending_damage[ident] = -1          # "damage everything" (INT_MAX sizes)
                    elif pending_damage[ident] >= 0:
                        pending_damage[ident] += abs(numbers[2] * numbers[3])
            elif msg == "frame":
                totals["frame_requests"] += 1
                per_second[second]["frame_requests"] += 1
                surfaces[ident]["frame_requests"] += 1
            elif msg == "commit":
                totals["commits"] += 1
                per_second[second]["commits"] += 1
                surfaces[ident]["commits"] += 1
                if pending_attach[ident]:
                    totals["commits_with_buffer"] += 1
                    per_second[second]["commits_with_buffer"] += 1
                    surfaces[ident]["commits_with_buffer"] += 1
                    if pending_damage[ident] < 0:
                        surfaces[ident]["full_surface_damage_frames"] += 1
                    else:
                        damage_area[ident].append(pending_damage[ident])
                pending_attach[ident] = False
                pending_damage[ident] = 0
        elif obj == "wl_callback" and not is_request and msg == "done":
            totals["frame_callbacks_done"] += 1
            per_second[second]["frame_callbacks_done"] += 1
        elif obj == "wl_pointer" and not is_request:
            input_events[msg] += 1
        elif obj == "wl_touch" and not is_request:
            input_events[f"touch_{msg}"] += 1

    seconds = int(span)
    commit_series = [per_second[s]["commits_with_buffer"] for s in range(max(seconds, 1))]
    result = {
        "log": str(args.log),
        "span_seconds": round(span, 3),
        "totals": dict(totals),
        "per_second_mean": {key: round(value / span, 3) for key, value in totals.items()},
        "frames_with_buffer_per_second": {
            "mean": round(statistics.mean(commit_series), 3) if commit_series else 0,
            "max": max(commit_series) if commit_series else 0,
            "seconds_without_any_frame": sum(1 for v in commit_series if v == 0),
        },
        "surfaces": {
            str(ident): {
                **dict(counter),
                "mean_damage_pixels": round(statistics.mean(damage_area[ident]), 1) if damage_area[ident] else None,
            }
            for ident, counter in sorted(surfaces.items(), key=lambda item: -item[1]["commits"])
        },
        "input_events_received": dict(input_events),
    }
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
