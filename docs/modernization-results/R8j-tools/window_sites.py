#!/usr/bin/env python3
"""Supplementary table for R8j step 3: the sites of windows 2 to 10 (not used by the rules).

The rules of heaptrack_attr.py speak of window 1's attribution set and of the new live growth
of windows 2 to 10. This script lists what those windows hold, from the same functions, so
that a reader can see which sites supply the later releases and the new live growth. It was
written after the formal run `WH`; it changes no rule and no result of heaptrack_attr.py.

    python3 -I window_sites.py <run dir> --data <heaptrack.zst> [--out <file.md>]
"""

from __future__ import annotations

import argparse
import collections
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import heaptrack_attr as ha  # noqa: E402

MIB = 1024 * 1024


def main(argv: list | None = None) -> int:
    parser = argparse.ArgumentParser(description="Supplementary sites of windows 2 to 10 (R8j step 3).")
    parser.add_argument("run", type=Path)
    parser.add_argument("--data", type=Path, required=True)
    parser.add_argument("--out", type=Path)
    args = parser.parse_args(sys.argv[1:] if argv is None else argv)
    model = ha.read_interpreted(args.data)
    maps = "".join(p.read_text(encoding="utf-8") + "\n" for p in (args.run / "session" / "maps-end.txt", args.run / "session" / "maps-warmup.txt") if p.exists())
    resolver = ha.Resolver(maps) if maps.strip() else None
    reports = ha.window_report(ha.account_windows(model, ha.load_windows(args.run, 0.0)), model, resolver)
    chosen = set(ha.attribution_set(reports[0]["sites"]))
    new_growth: collections.Counter = collections.Counter()
    releases: collections.Counter = collections.Counter()
    net_all = 0
    for window in reports[1:10]:
        for key, site in window["sites"].items():
            net_all += site["net_bytes"]
            releases[key] += site["share_bytes"]
            if key not in chosen and site["net_bytes"] > 0:
                new_growth[key] += site["net_bytes"]
    names = ha.pretty_names({name for key in list(new_growth) + list(releases) for name in ha.names_in([key])})
    lines = ["# R8j step 3: sites of windows 2 to 10 (supplementary)", ""]
    lines.append(f"Positive net outside the window-1 set, summed over windows 2 to 10: {sum(new_growth.values()) / MIB:.3f} MiB. "
                 f"Total net of windows 2 to 10 (all sites): {net_all / MIB:.3f} MiB.")
    lines.append("")
    lines.append("| site (innermost first) | positive net outside the set, MiB | share of the releases of windows 2 to 10, MiB |")
    lines.append("|---|---:|---:|")
    for key in sorted(set(new_growth) | {k for k, _ in releases.most_common(8)}, key=lambda k: -(new_growth[k] + max(0, releases[k])))[:12]:
        lines.append(f"| {ha.pretty_site(key, names)} | {new_growth[key] / MIB:.3f} | {releases[key] / MIB:.3f} |")
    lines.append("")
    text = "\n".join(lines) + "\n"
    if args.out:
        args.out.write_text(text, encoding="utf-8")
    sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
