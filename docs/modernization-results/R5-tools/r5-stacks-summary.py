#!/usr/bin/env python3
"""R5 helper: summarize stack-sample.sh output (runs anywhere).

A sample is "idle" when its innermost frames sit in the event loop's poll (g_main_context_poll, poll,
epoll_wait, ppoll). For the busy samples the script counts, per frame name, in how many samples it appears,
which is the share of busy time under that function, and prints the innermost non-library frame of each
busy sample as a histogram. Names are the demangled symbols gdb printed; no source is involved.
usage: r5-stacks-summary.py SAMPLES.txt [--top 25]
"""

import argparse
import collections
import re
from pathlib import Path

IDLE = re.compile(r"\b(g_main_context_poll|g_poll|__poll|ppoll|epoll_wait|__GI_ppoll|__ppoll|poll\b|syscall)")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("samples", type=Path)
    parser.add_argument("--top", type=int, default=25)
    args = parser.parse_args()

    frame_re = re.compile(r"^#\d+\s+(?:0x[0-9a-f]+\s+(?:in\s+)?)?(.*)$")
    samples, current = [], None
    for line in args.samples.read_text(errors="replace").splitlines():
        if line.startswith("=== sample"):
            current = []
            samples.append(current)
        elif line.startswith("#") and current is not None:
            match = frame_re.match(line)
            if match:
                current.append(re.sub(r"\s+from\s+\S+$", "", match.group(1)).strip())
    busy, idle = [], 0
    for frames in samples:
        if not frames:
            continue
        if any(IDLE.search(f) for f in frames[:6]):
            idle += 1
        else:
            busy.append(frames)
    print(f"samples {len(samples)}, idle (in poll) {idle}, busy {len(busy)}")
    frame_counts = collections.Counter()
    innermost = collections.Counter()
    for frames in busy:
        seen = set()
        for frame in frames:
            name = re.sub(r"\s*\(.*", "", frame.strip())[:110]
            if name not in seen:
                frame_counts[name] += 1
                seen.add(name)
        innermost[re.sub(r"\s*\(.*", "", frames[0].strip())[:110]] += 1
    print("\nframes by number of busy samples that contain them:")
    for name, count in frame_counts.most_common(args.top):
        print(f"{count:4d}  {name}")
    print("\ninnermost frame of the busy samples:")
    for name, count in innermost.most_common(12):
        print(f"{count:4d}  {name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
