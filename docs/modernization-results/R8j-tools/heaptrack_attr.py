#!/usr/bin/env python3
"""Attribute heaptrack allocations to call sites, per cycle window (R8j step 3).

Input: an interpreted heaptrack file, as `heaptrack_interpret` writes it (zstd-compressed
by heaptrack's wrapper; a plain text file works too), and the run directory of session.py
(events.jsonl gives the cycle windows). Output: attribution.json and attribution.md.

The interpreted format, as read from heaptrack 1.5.80 output (see the record's step 3):

- `s <len hex> <text>`: the next string in a 1-based table (the length is a check only).
- `i <ip hex> <module> [<function> <file> <line>]...`: an instruction record, 1-based.
  Each triple is one (possibly inlined) frame, innermost first. Ids index the string table.
- `t <ip record> <parent trace>`: a stack-trace node, 1-based; parent 0 is the root.
- `a <size hex> <trace node>`: an allocation info, 0-based. Allocations of one site and size
  share one info; the `+`/`-` lines refer to it.
- `+ <info>` / `- <info>`: one allocation / one free of that info, at the time of the last `c`.
- `c <time ms hex>`: the time since heaptrack started. `R`, `v`, `X`, `I` and `#` lines are
  not used.

Frees are counted per info, not matched to a single pointer, so the transient bytes of a
window are estimated as min(allocations, frees) x size per info inside the window. Allocated,
freed and net bytes are exact per info and window.

Standard library only (Python 3.14 reads zstd through `compression.zstd`).

    python3 -I heaptrack_attr.py --data <heaptrack.zst> --run <run dir> --out <dir> [--offset-ms 0]
"""

from __future__ import annotations

import argparse
import json
import os
import sys
from bisect import bisect_right
from pathlib import Path

MIB = 1024.0 * 1024.0
SHARE_CUTOFF = 0.5  # the attribution set covers at least this share of a window's transient bytes
REUSE_SHARE = 0.8  # windows 2 to 10 reuse the set when at least this share comes from it
SITE_FRAMES = 3  # a site is the first three named frames, innermost first
MAX_DEPTH = 512  # a guard against a corrupt trace loop
TOP_SITES = 15


class Model:
    """The parsed heaptrack file: strings, instruction records, trace nodes, infos and events."""

    def __init__(self) -> None:
        self.strings: list = []
        self.ips: list = [None]  # 1-based; each entry (module index, [(function, file, line), ...])
        self.traces: list = [None]  # 1-based; each entry (ip record index, parent trace index)
        self.infos: list = []  # 0-based; each entry (size bytes, trace node index)
        self.events: list = []  # (time ms, info index, +1 for an allocation, -1 for a free)
        self.unknown_info_events = 0
        self.last_time_ms = 0
        self.unparsed_lines = 0

    def string(self, index: int) -> str:
        if 1 <= index <= len(self.strings):
            return self.strings[index - 1]
        return "??"

    def module_name(self, index: int) -> str:
        return os.path.basename(self.string(index)) if index else "??"

    def frames(self, trace: int) -> list:
        """Frames of one trace node, innermost first, as (name, file, line, module) tuples."""
        out: list = []
        node = trace
        depth = 0
        while node and depth < MAX_DEPTH and node < len(self.traces):
            ip_index, parent = self.traces[node]
            module, triples = self.ips[ip_index] if 0 < ip_index < len(self.ips) else (0, [])
            module_name = self.module_name(module)
            if not triples:
                out.append(("??", "", 0, module_name))
            for function, file_index, line in triples:
                name = self.string(function) if function else "??"
                file_name = self.string(file_index) if file_index else ""
                out.append((name, file_name, line, module_name))
            node = parent
            depth += 1
        return out


def parse(lines) -> Model:
    model = Model()
    now = 0
    for raw in lines:
        line = raw.rstrip("\r\n")
        if not line:
            continue
        kind = line[0]
        if kind in "#RvXI":
            continue
        parts = line.split()
        try:
            if kind == "s":
                text = line.split(" ", 2)[2] if line.count(" ") >= 2 else ""
                model.strings.append(text)
            elif kind == "i":
                module = int(parts[2], 16) if len(parts) > 2 else 0
                triples = []
                fields = parts[3:]
                for position in range(0, len(fields) - 2, 3):
                    triples.append(
                        (int(fields[position], 16), int(fields[position + 1], 16), int(fields[position + 2], 16))
                    )
                model.ips.append((module, triples))
            elif kind == "t":
                model.traces.append((int(parts[1], 16), int(parts[2], 16)))
            elif kind == "a":
                model.infos.append((int(parts[1], 16), int(parts[2], 16)))
            elif kind in "+-":
                info = int(parts[1], 16)
                if 0 <= info < len(model.infos):
                    model.events.append((now, info, 1 if kind == "+" else -1))
                else:
                    model.unknown_info_events += 1
            elif kind == "c":
                now = int(parts[1], 16)
                model.last_time_ms = now
            else:
                model.unparsed_lines += 1
        except (ValueError, IndexError):
            model.unparsed_lines += 1
    return model


def site_key(frames: list) -> str:
    """The first SITE_FRAMES named frames, innermost first; file and line for each of them."""
    parts = []
    for name, file_name, line, module in frames:
        if name == "??":
            parts.append(f"??@{module}")
        elif file_name:
            parts.append(f"{name} ({os.path.basename(file_name)}:{line})")
        else:
            parts.append(name)
        if len(parts) == SITE_FRAMES:
            break
    return " < ".join(parts) if parts else "??"


def load_windows(run: Path, offset_ms: float) -> list:
    """The cycle windows of a run as (cycle, start ms, end ms), from events.jsonl."""
    events = [json.loads(line) for line in (run / "events.jsonl").read_text(encoding="utf-8").splitlines() if line.strip()]
    starts = {e["cycle"]: e["elapsed_s"] for e in events if e.get("event") == "cycle-start"}
    ends = {e["cycle"]: e["elapsed_s"] for e in events if e.get("event") == "rest-end"}
    windows = []
    for cycle in sorted(set(starts) & set(ends)):
        windows.append((cycle, starts[cycle] * 1000.0 + offset_ms, ends[cycle] * 1000.0 + offset_ms))
    return windows


def attribute(model: Model, windows: list) -> dict:
    """Per window and per site: allocated, freed, transient (estimated) and net bytes."""
    window_starts = [start for _, start, _ in windows]
    counts: dict = {}  # (window index, info) -> [allocations, frees]
    for time_ms, info, sign in model.events:
        position = bisect_right(window_starts, time_ms) - 1
        if position < 0:
            continue
        _, start, end = windows[position]
        if not (start <= time_ms <= end):
            continue
        slot = counts.setdefault((position, info), [0, 0])
        slot[0 if sign > 0 else 1] += 1

    trace_frames: dict = {}
    per_window: list = []
    for position, (cycle, start, end) in enumerate(windows):
        sites: dict = {}
        for (window, info), (allocs, frees) in counts.items():
            if window != position:
                continue
            size, trace = model.infos[info]
            if trace not in trace_frames:
                trace_frames[trace] = model.frames(trace)
            key = site_key(trace_frames[trace])
            entry = sites.setdefault(key, {"allocations": 0, "frees": 0, "alloc_bytes": 0, "freed_bytes": 0, "transient_bytes": 0, "net_bytes": 0, "frames": trace_frames[trace]})
            entry["allocations"] += allocs
            entry["frees"] += frees
            entry["alloc_bytes"] += allocs * size
            entry["freed_bytes"] += frees * size
            entry["transient_bytes"] += min(allocs, frees) * size
            entry["net_bytes"] += (allocs - frees) * size
        totals = {key: sum(site[key] for site in sites.values()) for key in ("alloc_bytes", "freed_bytes", "transient_bytes", "net_bytes")}
        per_window.append({"cycle": cycle, "start_ms": start, "end_ms": end, "sites": sites, "totals": totals})
    return {"windows": per_window}


def top_sites(sites: dict, field: str) -> list:
    return sorted(sites.items(), key=lambda item: (-item[1][field], item[0]))


def attribution_set(sites: dict, field: str = "transient_bytes") -> list:
    """The smallest prefix of the sites, by field, that covers SHARE_CUTOFF of its total."""
    total = sum(site[field] for site in sites.values())
    if total <= 0:
        return []
    chosen, covered = [], 0
    for key, site in top_sites(sites, field):
        if site[field] <= 0:
            break
        chosen.append(key)
        covered += site[field]
        if covered >= SHARE_CUTOFF * total:
            break
    return chosen


def reuse_check(result: dict) -> dict:
    """Windows 2 to 10 against the set of window 1: the share of their transient bytes it covers."""
    windows = result["windows"]
    if not windows:
        return {}
    first = windows[0]["sites"]
    chosen = set(attribution_set(first))
    rows = {}
    for window in windows[1:10]:
        total = window["totals"]["transient_bytes"]
        from_set = sum(site["transient_bytes"] for key, site in window["sites"].items() if key in chosen)
        new_net = sum(site["net_bytes"] for key, site in window["sites"].items() if key not in chosen and site["net_bytes"] > 0)
        rows[str(window["cycle"])] = {
            "transient_bytes": total,
            "share_from_window_1_set": (from_set / total) if total else None,
            "net_bytes_from_new_sites": new_net,
        }
    return {"window_1_set": sorted(chosen), "windows": rows}


def describe(result: dict, model: Model, data_path: Path, run: Path) -> dict:
    windows = result["windows"]
    report = {
        "data": str(data_path),
        "run": str(run),
        "strings": len(model.strings),
        "instruction_records": len(model.ips) - 1,
        "trace_nodes": len(model.traces) - 1,
        "infos": len(model.infos),
        "events": len(model.events),
        "unknown_info_events": model.unknown_info_events,
        "unparsed_lines": model.unparsed_lines,
        "last_heaptrack_time_ms": model.last_time_ms,
        "shares": {"cutoff": SHARE_CUTOFF, "reuse": REUSE_SHARE},
        "window_totals": [{"cycle": w["cycle"], **w["totals"]} for w in windows],
        "window_1": None,
        "reuse": reuse_check(result),
    }
    if windows:
        first = windows[0]
        chosen = attribution_set(first["sites"])
        ranked = top_sites(first["sites"], "transient_bytes")
        covered = sum(first["sites"][key]["transient_bytes"] for key in chosen)
        total = first["totals"]["transient_bytes"]
        report["window_1"] = {
            "cycle": first["cycle"],
            "transient_top": [
                {"site": key, **{k: v for k, v in site.items() if k != "frames"}, "frames": [f"{n} ({os.path.basename(f) if f else ''}:{l})" if f else n for n, f, l, _ in site["frames"][:8]]}
                for key, site in ranked[:TOP_SITES]
            ],
            "attribution_set": chosen,
            "attribution_share": (covered / total) if total else None,
            "net_top": [
                {"site": key, **{k: v for k, v in site.items() if k != "frames"}}
                for key, site in top_sites(first["sites"], "net_bytes")[:TOP_SITES]
                if site["net_bytes"] > 0
            ],
        }
    return report


def render(report: dict) -> str:
    lines = ["# R8j step 3: heaptrack attribution per cycle window", ""]
    lines.append(f"Data: `{report['data']}`. Run: `{report['run']}`.")
    lines.append(
        f"Records: {report['strings']} strings, {report['instruction_records']} instruction records, "
        f"{report['trace_nodes']} trace nodes, {report['infos']} allocation infos, {report['events']} allocation and free events; "
        f"unknown infos {report['unknown_info_events']}, unparsed lines {report['unparsed_lines']}. "
        f"Last heaptrack time {report['last_heaptrack_time_ms']} ms."
    )
    lines.append("")
    lines.append("## Totals per cycle window (MiB)")
    lines.append("")
    lines.append("| cycle | allocated | freed | transient (estimate) | net |")
    lines.append("|---:|---:|---:|---:|---:|")
    for row in report["window_totals"]:
        lines.append(
            f"| {row['cycle']} | {row['alloc_bytes'] / MIB:,.2f} | {row['freed_bytes'] / MIB:,.2f} | "
            f"{row['transient_bytes'] / MIB:,.2f} | {row['net_bytes'] / MIB:,.2f} |"
        )
    lines.append("")
    first = report.get("window_1")
    if first:
        lines.append(f"## Cycle {first['cycle']}: sites by transient bytes (top {len(first['transient_top'])})")
        lines.append("")
        lines.append(
            f"Attribution set (covers {SHARE_CUTOFF:.0%} of the transient bytes): {len(first['attribution_set'])} sites; "
            f"share of the window's transient bytes: {first['attribution_share']:.1%}."
            if first["attribution_share"] is not None
            else "No transient bytes in this window."
        )
        lines.append("")
        lines.append("| site (innermost first) | allocations | frees | transient MiB | freed MiB | net MiB |")
        lines.append("|---|---:|---:|---:|---:|---:|")
        for row in first["transient_top"]:
            lines.append(
                f"| {row['site']} | {row['allocations']} | {row['frees']} | {row['transient_bytes'] / MIB:,.3f} | "
                f"{row['freed_bytes'] / MIB:,.3f} | {row['net_bytes'] / MIB:,.3f} |"
            )
        lines.append("")
        lines.append(f"## Cycle {first['cycle']}: sites with net live growth (top {len(first['net_top'])})")
        lines.append("")
        lines.append("| site | allocations | frees | net MiB |")
        lines.append("|---|---:|---:|---:|")
        for row in first["net_top"]:
            lines.append(f"| {row['site']} | {row['allocations']} | {row['frees']} | {row['net_bytes'] / MIB:,.3f} |")
        lines.append("")
    reuse = report.get("reuse") or {}
    if reuse.get("windows"):
        lines.append("## Reuse check: windows 2 to 10 against the window-1 set")
        lines.append("")
        lines.append("| cycle | transient MiB | share from the window-1 set | net MiB from new sites (positive) |")
        lines.append("|---:|---:|---:|---:|")
        for cycle, row in reuse["windows"].items():
            share = "-" if row["share_from_window_1_set"] is None else f"{row['share_from_window_1_set']:.1%}"
            lines.append(
                f"| {cycle} | {row['transient_bytes'] / MIB:,.3f} | {share} | {row['net_bytes_from_new_sites'] / MIB:,.3f} |"
            )
        lines.append("")
    return "\n".join(lines)


def run_cli(argv: list | None = None) -> int:
    parser = argparse.ArgumentParser(description="Attribute heaptrack allocations to cycle windows (R8j step 3).")
    parser.add_argument("--data", type=Path, required=True, help="interpreted heaptrack file (.zst or text)")
    parser.add_argument("--run", type=Path, required=True, help="session.py run directory (events.jsonl)")
    parser.add_argument("--out", type=Path, required=True, help="directory for attribution.json and attribution.md")
    parser.add_argument("--offset-ms", type=float, default=0.0, help="heaptrack time minus harness elapsed time, in ms")
    args = parser.parse_args(sys.argv[1:] if argv is None else argv)
    if str(args.data).endswith(".zst"):
        from compression import zstd

        with zstd.open(args.data, "rt", encoding="utf-8", errors="replace") as handle:
            model = parse(handle)
    else:
        with args.data.open(encoding="utf-8", errors="replace") as handle:
            model = parse(handle)
    windows = load_windows(args.run, args.offset_ms)
    result = attribute(model, windows)
    report = describe(result, model, args.data, args.run)
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / "attribution.json").write_text(json.dumps(report, indent=1, sort_keys=True, ensure_ascii=False) + "\n", encoding="utf-8")
    (args.out / "attribution.md").write_text(render(report) + "\n", encoding="utf-8")
    print(f"windows {len(windows)}; events {len(model.events)}; infos {len(model.infos)}")
    return 0


if __name__ == "__main__":
    sys.exit(run_cli())
