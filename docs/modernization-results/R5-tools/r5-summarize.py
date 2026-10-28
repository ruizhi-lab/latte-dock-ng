#!/usr/bin/env python3
"""R5 helper: summarize the raw records into the tables of R5.md (runs on the developer host).

Reads, for each scenario given, <dir>/R5-<scenario>-family.jsonl (the raw
output of scripts/measure-process-family.py with one r5_start metadata line per
start) and <dir>/R5-<scenario>-threads.jsonl (r5-thread-sampler.py), and joins
every window with the host-side recorder output (host-monitor.py) by time.

Output is Markdown on stdout. Nothing is rewritten; the raw files stay
authoritative.
"""

from __future__ import annotations

import argparse
import datetime as dt
import json
from pathlib import Path
import statistics


def load(path: Path) -> list[dict]:
    records = []
    if path.exists():
        for line in path.read_text().splitlines():
            line = line.strip()
            if line:
                try:
                    records.append(json.loads(line))
                except json.JSONDecodeError:
                    records.append({"event": "unparsed", "text": line[:200]})
    return records


def parse_time(text: str) -> dt.datetime:
    return dt.datetime.fromisoformat(text)


def host_stats(samples: list[dict], start: dt.datetime, seconds: float) -> dict:
    end = start + dt.timedelta(seconds=seconds)
    inside = [s for s in samples if start <= parse_time(s["time"]) <= end and "host_busy_percent" in s]
    if not inside:
        return {}
    return {
        "host_busy_mean": statistics.mean(s["host_busy_percent"] for s in inside),
        "host_busy_min": min(s["host_busy_percent"] for s in inside),
        "host_load1_mean": statistics.mean(s["loadavg"][0] for s in inside),
        "host_steal_max": max(s["host_steal_percent"] for s in inside),
        "qemu_cpu_mean": statistics.mean(s["qemu_cpu_percent_of_one_core"] for s in inside),
        "samples": len(inside),
    }


def mib(kib: float) -> float:
    return kib / 1024


def fmt(value: float | None, digits: int = 3) -> str:
    return "-" if value is None else f"{value:.{digits}f}"


def summarize(scenario: str, directory: Path, host_samples: list[dict], busy_threshold: float, top: int) -> None:
    family = load(directory / f"R5-{scenario}-family.jsonl")
    threads = load(directory / f"R5-{scenario}-threads.jsonl")

    # Pair every r5_start line with the window that follows it in each file.
    def windows_by_start(records: list[dict], kind: str) -> dict[int, dict]:
        result: dict[int, dict] = {}
        current = None
        for rec in records:
            if rec.get("event") == "r5_start":
                current = rec["start"]
                result[current] = {"meta": rec}
            elif rec.get("event") == kind and current is not None:
                result[current]["window"] = rec
            elif rec.get("event") == "invalid" and current is not None:
                result[current]["invalid"] = rec
        return result

    fam = windows_by_start(family, "window")
    thr = windows_by_start(threads, "r5_window")

    print(f"### {scenario}\n")
    print("| Start | Valid | Family CPU % | Family ctx/s | PSS MiB | Private MiB | RSS MiB | Sampler CPU % | Sampler slices/s | VM busy % of machine | VM steal % | Host busy % | Host load1 | Pointer |")
    print("| ---: | :---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |")
    cpu_values, ctx_values, pss_values, sampler_values = [], [], [], []
    busy_hosts = []
    detail = []
    for start in sorted(set(fam) | set(thr)):
        f = fam.get(start, {})
        t = thr.get(start, {})
        meta = f.get("meta") or t.get("meta") or {}
        window = f.get("window")
        tw = t.get("window")
        primary = None
        if tw:
            labels = {p["label"]: p for p in tw["processes"]}
            primary = labels.get("dock") or labels.get("plasmashell")
        if window is None and tw is None:
            print(f"| {start} | no window ({meta.get('error', f.get('invalid', {}).get('reason', 'missing'))}) |" + " |" * 12)
            continue
        valid = (window or {}).get("valid", False) and (tw or {}).get("valid", False)
        host = {}
        if window:
            host = host_stats(host_samples, parse_time(window["start"]), window["elapsed_seconds"])
        busy = bool(host) and host["host_busy_mean"] >= busy_threshold
        if busy:
            busy_hosts.append(start)
        mem = (window or {}).get("memory_mean_kib", {})
        sysinfo = (tw or {}).get("system", {})
        row = [
            str(start),
            "yes" if valid else "NO",
            fmt((window or {}).get("cpu_percent_one_core")),
            fmt((window or {}).get("context_switches_per_second"), 1),
            fmt(mib(mem["Pss"]), 1) if mem else "-",
            fmt(mib(mem["Private"]), 1) if mem else "-",
            fmt(mib(mem["Rss"]), 1) if mem else "-",
            fmt(primary["cpu_percent_one_core"]) if primary else "-",
            fmt(primary["timeslices_per_second"], 1) if primary else "-",
            fmt(sysinfo.get("busy_percent_of_machine"), 2),
            fmt(sysinfo.get("steal_percent_of_machine"), 2),
            (fmt(host.get("host_busy_mean"), 1) + (" (busy)" if busy else "")) if host else "n/a",
            fmt(host.get("host_load1_mean"), 2) if host else "n/a",
            meta.get("pointer", "?"),
        ]
        print("| " + " | ".join(row) + " |")
        if valid and not busy and window:
            cpu_values.append(window["cpu_percent_one_core"])
            ctx_values.append(window["context_switches_per_second"])
            pss_values.append(mib(mem["Pss"]))
            if primary:
                sampler_values.append(primary["cpu_percent_one_core"])
        if tw:
            detail.append((start, tw, busy))

    def stat_row(name: str, values: list[float], digits: int = 3) -> str:
        if not values:
            return f"| {name} | - | - | - |"
        return f"| {name} | {statistics.median(values):.{digits}f} | {statistics.mean(values):.{digits}f} | {min(values):.{digits}f}-{max(values):.{digits}f} |"

    print()
    if busy_hosts:
        print(f"Windows taken while the host was busy (mean host CPU >= {busy_threshold:.0f} %): starts {busy_hosts}; they are excluded from the statistics below.\n")
    print("| Metric (valid windows, quiet host) | Median | Mean | Range |")
    print("| --- | ---: | ---: | ---: |")
    print(stat_row("Family CPU, percent of one core", cpu_values))
    print(stat_row("Family context switches per second", ctx_values, 1))
    print(stat_row("PSS, MiB", pss_values, 1))
    print()
    if detail:
        print(f"Per-thread view of the primary process (top {top} threads per start, percent of one core / timeslices per second):\n")
        for start, tw, busy in detail:
            labels = {p["label"]: p for p in tw["processes"]}
            primary = labels.get("dock") or labels.get("plasmashell")
            if primary is None:
                continue
            parts = [f"{th['comm']}#{th['tid']} {th['cpu_percent_one_core']:.3f}/{th['timeslices_per_second']:.1f}" for th in primary["threads"][:top]]
            others = [f"{p['label']} {p['cpu_percent_one_core']:.3f}%" for p in tw["processes"] if p is not primary]
            print(f"- start {start}{' (host busy)' if busy else ''}: {primary['label']} {primary['cpu_percent_one_core']:.3f}% [{'; '.join(parts)}]; others: {', '.join(others)}")
        print()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("scenarios", nargs="+")
    parser.add_argument("--dir", type=Path, default=Path("."))
    parser.add_argument("--host-monitor", type=Path)
    parser.add_argument("--busy-threshold", type=float, default=50.0,
                        help="Mean host CPU percent from which a window counts as taken on a busy host.")
    parser.add_argument("--top", type=int, default=4)
    args = parser.parse_args()
    host_samples = load(args.host_monitor) if args.host_monitor else []
    for scenario in args.scenarios:
        summarize(scenario, args.dir, host_samples, args.busy_threshold, args.top)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
