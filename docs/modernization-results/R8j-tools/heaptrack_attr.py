#!/usr/bin/env python3
"""Attribute heaptrack allocations to call sites, per cycle window (R8j step 3).

Input: an interpreted heaptrack file, as `heaptrack_interpret` writes it (zstd-compressed,
as heaptrack's wrapper writes it; a plain text file works too), the run directory of
session.py (events.jsonl gives the cycle windows, and session/maps-*.txt the dock's
memory map), and optionally the dock's executable. Output: attribution.json and
attribution.md in the output directory.

The interpreted format, as read from heaptrack 1.5.80 output:

- `s <len hex> <text>`: the next string of a 1-based table; the length is only a check.
- `i <ip hex> <module> [<function> <file> <line>]...`: an instruction record, 1-based. Each
  triple is one (possibly inlined) frame, innermost first; ids index the string table.
  heaptrack resolves function names only from debug information.
- `t <ip record> <parent trace>`: a stack-trace node, 1-based; parent 0 is the root.
- `a <size hex> <trace node>`: an allocation info, 0-based. Allocations of one site and size
  share one info, and the `+`/`-` lines refer to it.
- `+ <info>` / `- <info>`: one allocation / one free of that info, at the time of the last `c`.
- `c <time ms hex>`: the time since heaptrack started. Other lines are not used.

Frees are counted per info, not matched to single allocations. For each window the tool
therefore reports, exactly per info: allocated, freed and net bytes; the live peak of each
info inside the window and its live count at the window's end (the release); and the total
live bytes at the window's largest point minus the total at its end (the synchronized
release). The transient bytes, min(allocations, frees) x size, are an estimate reported
for information only.

Function names: a frame without debug information is named from the module's symbol table
(`nm`): the table of the file itself when it has one (exact), otherwise its exported symbols
(approximate, shown as `name+0xOFFSET`). The file address of an instruction is its address
minus the start of its mapping in /proc/<pid>/maps plus the mapping's file offset.

Standard library only (Python 3.14 reads zstd through `compression.zstd`).

    python3 -I heaptrack_attr.py --data <heaptrack.zst> --run <run dir> --out <dir> [--offset-ms 0]
"""

from __future__ import annotations

import argparse
import bisect
import json
import os
import subprocess
import sys
from pathlib import Path

MIB = 1024.0 * 1024.0
SHARE_CUTOFF = 0.5  # the attribution set covers at least this share of the window's release
REUSE_SHARE = 0.8  # windows 2 to 10 reuse the set when at least this share comes from it
SITE_FRAMES = 3  # a site is the first three named frames, innermost first
MAX_DEPTH = 512  # a guard against a corrupt trace loop
TOP_SITES = 15
NM_TIMEOUT_S = 300


class Model:
    """The parsed heaptrack file: strings, instruction records, trace nodes, infos and events."""

    def __init__(self) -> None:
        self.strings: list = []
        self.ips: list = [None]  # 1-based; each entry (ip address, module index, [(function, file, line), ...])
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

    def module_path(self, index: int) -> str:
        return self.string(index) if index else ""

    def module_name(self, index: int) -> str:
        return os.path.basename(self.module_path(index)) if index else "??"

    def frames(self, trace: int, resolver=None) -> list:
        """Frames of one trace node, innermost first, as (name, file, line, module) tuples.

        A frame without a function name from debug information is named by `resolver`, if
        given, as (name, approximate flag) from the module's symbol table.
        """
        out: list = []
        node = trace
        depth = 0
        while node and depth < MAX_DEPTH and node < len(self.traces):
            ip_index, parent = self.traces[node]
            address, module, triples = self.ips[ip_index] if 0 < ip_index < len(self.ips) else (0, 0, [])
            module_name = self.module_name(module)
            if not triples:
                name = resolver(self.module_path(module), address) if resolver else None
                out.append((name or "??", "", 0, module_name))
            for function, file_index, line in triples:
                name = self.string(function) if function else "??"
                if name == "??" and resolver is not None:
                    name = resolver(self.module_path(module), address) or "??"
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
                address = int(parts[1], 16)
                module = int(parts[2], 16) if len(parts) > 2 else 0
                triples = []
                fields = parts[3:]
                for position in range(0, len(fields) - 2, 3):
                    triples.append(
                        (int(fields[position], 16), int(fields[position + 1], 16), int(fields[position + 2], 16))
                    )
                model.ips.append((address, module, triples))
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


class Symbols:
    """Function addresses of one file, for naming frames that have no debug information."""

    def __init__(self, path: str, exact: bool, addresses: list, names: list) -> None:
        self.path = path
        self.exact = exact
        self.addresses = addresses
        self.names = names

    def name_for(self, file_address: int):
        index = bisect.bisect_right(self.addresses, file_address) - 1
        if index < 0:
            return None
        offset = file_address - self.addresses[index]
        name = self.names[index]
        if self.exact:
            return name if offset == 0 else f"{name}+0x{offset:x}"
        return f"{name}+0x{offset:x}"


def load_mappings(text: str) -> list:
    """File mappings of a /proc/<pid>/maps text, as (start, end, file offset, path), sorted by start."""
    rows = []
    for line in text.splitlines():
        parts = line.split(None, 5)
        if len(parts) < 6 or "/" not in parts[5]:
            continue
        start_text, end_text = parts[0].split("-")
        rows.append((int(start_text, 16), int(end_text, 16), int(parts[2], 16), parts[5].strip()))
    rows.sort()
    return rows


def file_address(mappings: list, starts: list, address: int):
    """(path, file address) of an instruction address, or None when no file mapping covers it."""
    index = bisect.bisect_right(starts, address) - 1
    if index < 0:
        return None
    start, end, offset, path = mappings[index]
    if address >= end:
        return None
    return path, address - start + offset


def read_symbols(path: str) -> Symbols:
    """The function symbols of a file: its own table if it has one, else its exported ones."""
    for exact, args in ((True, ["nm", "-n", "--defined-only", "--", path]), (False, ["nm", "-D", "-n", "--defined-only", "--", path])):
        try:
            result = subprocess.run(args, capture_output=True, text=True, timeout=NM_TIMEOUT_S, check=False)
        except (OSError, subprocess.TimeoutExpired):
            continue
        addresses, names = [], []
        for line in result.stdout.splitlines():
            parts = line.split(maxsplit=2)
            if len(parts) != 3 or parts[1] not in ("T", "t", "W", "w"):
                continue
            try:
                addresses.append(int(parts[0], 16))
            except ValueError:
                continue
            names.append(parts[2])
        if addresses:
            return Symbols(path, exact, addresses, names)
    return Symbols(path, False, [], [])


class Resolver:
    """Names frames by the memory map of the dock, one symbol table per file."""

    def __init__(self, maps_text: str) -> None:
        self.mappings = load_mappings(maps_text)
        self.starts = [row[0] for row in self.mappings]
        self.symbols: dict = {}

    def __call__(self, path: str, address: int):
        if not path:
            return None
        located = file_address(self.mappings, self.starts, address)
        if located is None:
            return None
        # The loader names a library by its link (libQt6Core.so.6); the maps show the file (libQt6Core.so.6.11.3).
        real = os.path.realpath(path)
        if located[0] != real:
            return None
        if real not in self.symbols:
            self.symbols[real] = read_symbols(real)
        return self.symbols[real].name_for(located[1])


def site_key(frames: list) -> str:
    """The first SITE_FRAMES named frames, innermost first; file and line for those with them."""
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


class WindowAccount:
    """One window: its events in order, the counts per info, and the total live bytes at its start, peak and end."""

    def __init__(self, cycle: int, start: float, end: float) -> None:
        self.cycle = cycle
        self.start = start
        self.end = end
        self.entered = False
        self.total_start = 0
        self.total_max = 0
        self.total_end = 0
        self.peak_index = -1  # the index of the event that first reached total_max inside the window; -1 is its start
        self.events: list = []  # (index inside the window, info, +1 or -1)
        self.counts: dict = {}  # info -> [allocations, frees]


def account_windows(model: Model, windows: list) -> list:
    """One pass over the events: the window totals, and each window's events with the peak position."""
    accounts = [WindowAccount(cycle, start, end) for cycle, start, end in windows]
    total = 0  # total live bytes, from the start of the heaptrack data
    current = 0
    for time_ms, info, sign in model.events:
        while current < len(accounts) and time_ms > accounts[current].end:
            finish(accounts[current], total)
            current += 1
        size = model.infos[info][0]
        if current < len(accounts) and accounts[current].start <= time_ms:
            acc = accounts[current]
            if not acc.entered:
                acc.entered = True
                acc.total_start = total
                acc.total_max = total
            index = len(acc.events)
            acc.events.append((index, info, sign))
            acc.counts.setdefault(info, [0, 0])[0 if sign > 0 else 1] += 1
            total += sign * size
            if total > acc.total_max:
                acc.total_max = total
                acc.peak_index = index
        else:
            total += sign * size
    while current < len(accounts):
        finish(accounts[current], total)
        current += 1
    return accounts


def finish(acc: WindowAccount, total_now: int) -> None:
    """Close a window whose end has passed. No event of the window comes after this one, so the total now is its end."""
    if not acc.entered:
        acc.total_start = total_now
        acc.total_max = total_now
    acc.total_end = total_now


def window_report(accounts: list, model: Model, resolver) -> list:
    """Per window: the totals, and per site the share of the synchronized release, transient and net bytes.

    The share of an info is its live count at the window's peak minus its live count at the
    window's end, times its size. Summed over the infos this is exactly the peak total minus the
    end total, so the sites' shares divide the window's synchronized release.
    """
    reports = []
    trace_frames: dict = {}
    for acc in accounts:
        delta_end: dict = {}
        delta_peak: dict = {}
        for index, info, sign in acc.events:
            delta_end[info] = delta_end.get(info, 0) + sign
            if index <= acc.peak_index:
                delta_peak[info] = delta_peak.get(info, 0) + sign
        sites: dict = {}
        allocated = freed = 0
        for info in set(acc.counts) | set(delta_end):
            size, trace = model.infos[info]
            allocations, frees = acc.counts.get(info, [0, 0])
            allocated += allocations * size
            freed += frees * size
            if trace not in trace_frames:
                trace_frames[trace] = model.frames(trace, resolver)
            key = site_key(trace_frames[trace])
            entry = sites.setdefault(
                key,
                {"allocations": 0, "frees": 0, "alloc_bytes": 0, "freed_bytes": 0, "transient_bytes": 0,
                 "net_bytes": 0, "share_bytes": 0, "frames": trace_frames[trace]},
            )
            entry["allocations"] += allocations
            entry["frees"] += frees
            entry["alloc_bytes"] += allocations * size
            entry["freed_bytes"] += frees * size
            entry["transient_bytes"] += min(allocations, frees) * size
            entry["net_bytes"] += delta_end.get(info, 0) * size
            entry["share_bytes"] += (delta_peak.get(info, 0) - delta_end.get(info, 0)) * size
        shares_sum = sum(site["share_bytes"] for site in sites.values())
        reports.append(
            {
                "cycle": acc.cycle,
                "start_ms": acc.start,
                "end_ms": acc.end,
                "entered": acc.entered,
                "totals": {
                    "alloc_bytes": allocated,
                    "freed_bytes": freed,
                    "transient_bytes": sum(site["transient_bytes"] for site in sites.values()),
                    "net_bytes": acc.total_end - acc.total_start,
                    "release_bytes_synchronized": acc.total_max - acc.total_end,
                    "share_sum_check": shares_sum,
                },
                "sites": sites,
            }
        )
    return reports


def top_sites(sites: dict, field: str) -> list:
    return sorted(sites.items(), key=lambda item: (-item[1][field], item[0]))


def attribution_set(sites: dict, field: str = "share_bytes") -> list:
    """The smallest prefix of the sites, by field, that covers SHARE_CUTOFF of the positive total."""
    total = sum(max(0, site[field]) for site in sites.values())
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


def reuse_check(reports: list) -> dict:
    """Windows 2 to 10 against the window-1 attribution set: the share of their releases it covers."""
    if not reports:
        return {}
    chosen = set(attribution_set(reports[0]["sites"]))
    rows = {}
    for window in reports[1:10]:
        total = sum(max(0, site["share_bytes"]) for site in window["sites"].values())
        from_set = sum(max(0, site["share_bytes"]) for key, site in window["sites"].items() if key in chosen)
        new_net = sum(site["net_bytes"] for key, site in window["sites"].items() if key not in chosen and site["net_bytes"] > 0)
        rows[str(window["cycle"])] = {
            "release_bytes": total,
            "share_from_window_1_set": (from_set / total) if total else None,
            "net_bytes_from_new_sites": new_net,
        }
    return {"window_1_set": sorted(chosen), "windows": rows}


def describe(reports: list, model: Model, data_path: Path, run: Path, maps_files: list) -> dict:
    report = {
        "data": str(data_path),
        "run": str(run),
        "maps": maps_files,
        "strings": len(model.strings),
        "instruction_records": len(model.ips) - 1,
        "trace_nodes": len(model.traces) - 1,
        "infos": len(model.infos),
        "events": len(model.events),
        "unknown_info_events": model.unknown_info_events,
        "unparsed_lines": model.unparsed_lines,
        "last_heaptrack_time_ms": model.last_time_ms,
        "shares": {"cutoff": SHARE_CUTOFF, "reuse": REUSE_SHARE},
        "window_totals": [{"cycle": w["cycle"], "entered": w["entered"], **w["totals"]} for w in reports],
        "window_1": None,
        "reuse": reuse_check(reports),
    }
    if reports:
        first = reports[0]
        chosen = attribution_set(first["sites"])
        total = sum(max(0, site["share_bytes"]) for site in first["sites"].values())
        covered = sum(first["sites"][key]["share_bytes"] for key in chosen)
        report["window_1"] = {
            "cycle": first["cycle"],
            "share_top": [
                {"site": key, **{k: v for k, v in site.items() if k != "frames"}, "dock_frame": first_dock_frame(site["frames"]), "frames": [format_frame(f) for f in site["frames"][:8]]}
                for key, site in top_sites(first["sites"], "share_bytes")[:TOP_SITES]
                if site["share_bytes"] > 0
            ],
            "attribution_set": chosen,
            "attribution_share": (covered / total) if total else None,
            "transient_top": [
                {"site": key, **{k: v for k, v in site.items() if k != "frames"}}
                for key, site in top_sites(first["sites"], "transient_bytes")[:TOP_SITES]
            ],
            "net_top": [
                {"site": key, **{k: v for k, v in site.items() if k != "frames"}}
                for key, site in top_sites(first["sites"], "net_bytes")[:TOP_SITES]
                if site["net_bytes"] > 0
            ],
        }
    return report



def first_dock_frame(frames: list) -> str:
    """The innermost frame that belongs to the dock or its plugins (latte in the module name), or '-'."""
    for name, file_name, line, module in frames:
        if "latte" in module:
            return format_frame((name, file_name, line, module))
    return "-"


def pretty_names(names: set) -> dict:
    """C++ names demangled with c++filt (one call); a name that cannot be demangled stays as it is."""
    import shutil

    mapping = {name: name for name in names}
    if not names or shutil.which("c++filt") is None:
        return mapping
    ordered = sorted(names)
    try:
        result = subprocess.run(["c++filt"], input="\n".join(ordered) + "\n", capture_output=True, text=True, timeout=120, check=False)
    except (OSError, subprocess.TimeoutExpired):
        return mapping
    demangled = result.stdout.splitlines()
    if len(demangled) == len(ordered):
        mapping.update(dict(zip(ordered, demangled)))
    return mapping


def pretty_site(key: str, mapping: dict) -> str:
    """A site key with each frame's name demangled and its library version tag (@@...) dropped."""
    parts = []
    for part in key.split(" < "):
        head, sep, tail = part.partition(" (")
        name, plus, offset = head.partition("+0x")
        name = mapping.get(name.split("@@")[0], name.split("@@")[0]) if name else name
        parts.append(name + (plus + offset if plus else "") + (sep + tail if sep else ""))
    return " < ".join(parts)


def names_in(keys: list) -> set:
    names = set()
    for key in keys:
        for part in key.split(" < "):
            head = part.partition(" (")[0]
            name = head.partition("+0x")[0]
            if name and not name.startswith("??"):
                names.add(name.split("@@")[0])
    return names

def format_frame(frame: tuple) -> str:
    name, file_name, line, _ = frame
    return f"{name} ({os.path.basename(file_name)}:{line})" if file_name else name


def render(report: dict) -> str:
    lines = ["# R8j step 3: heaptrack attribution per cycle window", ""]
    lines.append(f"Data: `{report['data']}`. Run: `{report['run']}`. Memory maps: {', '.join(report['maps']) or 'none'}.")
    lines.append(
        f"Records: {report['strings']} strings, {report['instruction_records']} instruction records, "
        f"{report['trace_nodes']} trace nodes, {report['infos']} allocation infos, {report['events']} allocation and free events; "
        f"unknown infos {report['unknown_info_events']}, unparsed lines {report['unparsed_lines']}. "
        f"Last heaptrack time {report['last_heaptrack_time_ms']} ms."
    )
    lines.append("")
    lines.append("## Totals per cycle window (MiB)")
    lines.append("")
    lines.append("| cycle | allocated | freed | net | synchronized release | transient (estimate) |")
    lines.append("|---:|---:|---:|---:|---:|---:|")
    for row in report["window_totals"]:
        lines.append(
            f"| {row['cycle']} | {row['alloc_bytes'] / MIB:,.2f} | {row['freed_bytes'] / MIB:,.2f} | "
            f"{row['net_bytes'] / MIB:,.2f} | {row['release_bytes_synchronized'] / MIB:,.2f} | {row['transient_bytes'] / MIB:,.2f} |"
        )
    lines.append("")
    first = report.get("window_1")
    if first:
        share = first["attribution_share"]
        lines.append(f"## Cycle {first['cycle']}: sites by their share of the synchronized release (top {len(first['share_top'])})")
        lines.append("")
        lines.append(
            f"Attribution set (covers {SHARE_CUTOFF:.0%} of the positive shares): {len(first['attribution_set'])} sites; "
            f"share of the positive shares: {share:.1%}."
            if share is not None
            else "No positive share in this window."
        )
        lines.append("")
        mapping = pretty_names(names_in([row["site"] for row in first["share_top"] + first["net_top"]]))
        lines.append("| site (innermost first) | share MiB | net MiB | allocations | frees | first dock frame |")
        lines.append("|---|---:|---:|---:|---:|---|")
        for row in first["share_top"]:
            lines.append(
                f"| {pretty_site(row['site'], mapping)} | {row['share_bytes'] / MIB:,.3f} | {row['net_bytes'] / MIB:,.3f} | "
                f"{row['allocations']} | {row['frees']} | {row['dock_frame']} |"
            )
        lines.append("")
        lines.append(f"## Cycle {first['cycle']}: sites with net live growth (top {len(first['net_top'])})")
        lines.append("")
        lines.append("| site | net MiB | allocations | frees |")
        lines.append("|---|---:|---:|---:|")
        for row in first["net_top"]:
            lines.append(f"| {pretty_site(row['site'], mapping)} | {row['net_bytes'] / MIB:,.3f} | {row['allocations']} | {row['frees']} |")
        lines.append("")
    reuse = report.get("reuse") or {}
    if reuse.get("windows"):
        lines.append("## Reuse check: windows 2 to 10 against the window-1 attribution set")
        lines.append("")
        lines.append("| cycle | release MiB | share from the window-1 set | net MiB from new sites (positive) |")
        lines.append("|---:|---:|---:|---:|")
        for cycle, row in reuse["windows"].items():
            share_text = "-" if row["share_from_window_1_set"] is None else f"{row['share_from_window_1_set']:.1%}"
            lines.append(f"| {cycle} | {row['release_bytes'] / MIB:,.3f} | {share_text} | {row['net_bytes_from_new_sites'] / MIB:,.3f} |")
        lines.append("")
    return "\n".join(lines)


def read_interpreted(path: Path) -> Model:
    if str(path).endswith(".zst"):
        from compression import zstd

        with zstd.open(path, "rt", encoding="utf-8", errors="replace") as handle:
            return parse(handle)
    with path.open(encoding="utf-8", errors="replace") as handle:
        return parse(handle)


def run_cli(argv: list | None = None) -> int:
    parser = argparse.ArgumentParser(description="Attribute heaptrack allocations to cycle windows (R8j step 3).")
    parser.add_argument("--data", type=Path, required=True, help="interpreted heaptrack file (.zst or text)")
    parser.add_argument("--run", type=Path, required=True, help="session.py run directory (events.jsonl, session/maps-*.txt)")
    parser.add_argument("--out", type=Path, required=True, help="directory for attribution.json and attribution.md")
    parser.add_argument("--offset-ms", type=float, default=0.0, help="heaptrack time minus harness elapsed time, in ms")
    parser.add_argument("--maps", type=Path, action="append", default=[], help="a /proc/<pid>/maps text (default: the run's maps-end.txt, then maps-warmup.txt)")
    args = parser.parse_args(sys.argv[1:] if argv is None else argv)
    model = read_interpreted(args.data)
    maps = args.maps or [p for p in (args.run / "session" / "maps-end.txt", args.run / "session" / "maps-warmup.txt") if p.exists()]
    maps_text = "".join(p.read_text(encoding="utf-8") + "\n" for p in maps)
    resolver = Resolver(maps_text) if maps_text.strip() else None
    windows = load_windows(args.run, args.offset_ms)
    reports = window_report(account_windows(model, windows), model, resolver)
    report = describe(reports, model, args.data, args.run, [str(p) for p in maps])
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / "attribution.json").write_text(json.dumps(report, indent=1, sort_keys=True, ensure_ascii=False) + "\n", encoding="utf-8")
    (args.out / "attribution.md").write_text(render(report) + "\n", encoding="utf-8")
    print(f"windows {len(windows)}; events {len(model.events)}; infos {len(model.infos)}")
    return 0


if __name__ == "__main__":
    sys.exit(run_cli())
