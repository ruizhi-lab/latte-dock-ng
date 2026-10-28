#!/usr/bin/env python3
"""R5 helper: keep only the host-monitor samples that overlap recorded windows (runs on the developer host).

host-monitor.py runs for hours; the repository keeps the samples from 10 s before the start to 10 s after the
end of every window found in the given family files, so every window's host context stays checkable.
usage: r5-trim-host.py HOST_MONITOR.jsonl OUT.jsonl FAMILY.jsonl [FAMILY.jsonl ...]
"""

import datetime as dt
import json
import sys
from pathlib import Path


def main() -> int:
    monitor, out, *families = sys.argv[1:]
    spans = []
    for family in families:
        for line in Path(family).read_text().splitlines():
            rec = json.loads(line)
            if rec.get("event") == "window":
                start = dt.datetime.fromisoformat(rec["start"])
                spans.append((start - dt.timedelta(seconds=10), start + dt.timedelta(seconds=rec["elapsed_seconds"] + 10)))
    kept = 0
    with Path(out).open("w") as handle:
        for line in Path(monitor).read_text().splitlines():
            rec = json.loads(line)
            when = dt.datetime.fromisoformat(rec["time"])
            if any(a <= when <= b for a, b in spans):
                handle.write(line + "\n")
                kept += 1
    print(f"kept {kept} samples for {len(spans)} windows")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
