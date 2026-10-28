#!/usr/bin/env python3
"""R5 helper: condensed Markdown tables for R5.md from the raw records (runs on the developer host).

One row per scenario. A window is valid when both collectors marked it valid; a window counts as taken
on a busy host when the mean host CPU during it (host-monitor.py) reached --busy-threshold percent.
Statistics use every valid window and list how many of them overlapped a busy host, so a reader can see
whether the conclusion depends on those windows.

usage: r5-tables.py --dir R5-data --host-monitor host.jsonl LABEL=scenario[,scenario...] ...
Each LABEL=scenario[,scenario...] argument becomes one row (several scenarios are pooled).
"""

from __future__ import annotations

import argparse
import datetime as dt
import json
from pathlib import Path
import statistics


def load(path: Path) -> list[dict]:
    if not path.exists():
        return []
    return [json.loads(line) for line in path.read_text().splitlines() if line.strip()]


def host_mean_busy(samples: list[dict], start: str, seconds: float) -> float | None:
    begin = dt.datetime.fromisoformat(start)
    end = begin + dt.timedelta(seconds=seconds)
    inside = [s["host_busy_percent"] for s in samples if "host_busy_percent" in s and begin <= dt.datetime.fromisoformat(s["time"]) <= end]
    return statistics.mean(inside) if inside else None


def windows(scenario: str, directory: Path, host_samples: list[dict], busy_threshold: float) -> list[dict]:
    family = load(directory / f"R5-{scenario}-family.jsonl")
    threads = load(directory / f"R5-{scenario}-threads.jsonl")
    fam_windows = [r for r in family if r.get("event") == "window"]
    thr_windows = [r for r in threads if r.get("event") == "r5_window"]
    result = []
    for fam, thr in zip(fam_windows, thr_windows):
        procs = {p["label"]: p for p in thr["processes"]}
        primary = procs.get("dock") or procs.get("plasmashell")
        busy = host_mean_busy(host_samples, fam["start"], fam["elapsed_seconds"])
        render_slices = sum(t["timeslices_per_second"] for t in primary["threads"] if t["comm"] == "QSGRenderThread") if primary else 0.0
        llvmpipe = sum(t["cpu_percent_one_core"] for t in primary["threads"] if t["comm"].startswith("llvmpipe")) if primary else 0.0
        main_thread = next((t for t in primary["threads"] if t["tid"] == primary["pid"]), None) if primary else None
        result.append({
            "valid": fam["valid"] and thr["valid"],
            "cpu": fam["cpu_percent_one_core"],
            "ctx": fam["context_switches_per_second"],
            "slices": primary["timeslices_per_second"] if primary else None,
            "main_cpu": main_thread["cpu_percent_one_core"] if main_thread else None,
            "render_slices": render_slices,
            "llvmpipe_cpu": llvmpipe,
            "kwin": procs["kwin"]["cpu_percent_one_core"] if "kwin" in procs else None,
            "plasmashell": procs["plasmashell"]["cpu_percent_one_core"] if "plasmashell" in procs else None,
            "pss": fam["memory_mean_kib"]["Pss"] / 1024,
            "steal": thr["system"].get("steal_percent_of_machine"),
            "host_busy": busy,
            "host_busy_flag": busy is not None and busy >= busy_threshold,
        })
    return result


def rng(values: list[float], digits: int) -> str:
    if not values:
        return "-"
    values = sorted(values)
    median = statistics.median(values)
    if len(values) == 1 or values[0] == values[-1]:
        return f"{median:.{digits}f}"
    return f"{median:.{digits}f} ({values[0]:.{digits}f}–{values[-1]:.{digits}f})"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rows", nargs="+", metavar="LABEL=scenario[,scenario]")
    parser.add_argument("--dir", type=Path, default=Path("."))
    parser.add_argument("--host-monitor", type=Path)
    parser.add_argument("--busy-threshold", type=float, default=50.0)
    parser.add_argument("--kind", choices=("idle", "stimulus"), default="idle",
                        help="idle: CPU, wakeups, render thread, PSS; stimulus: adds kwin and plasmashell CPU and the llvmpipe share.")
    args = parser.parse_args()
    host_samples = load(args.host_monitor) if args.host_monitor else []

    if args.kind == "idle":
        print("| Scenario | Windows (valid / busy host) | Dock CPU, % of one core | Main-thread ctx/s | All-thread timeslices/s | Render-thread timeslices/s | PSS MiB |")
        print("| --- | :---: | ---: | ---: | ---: | ---: | ---: |")
    else:
        print("| Scenario | Windows (valid / busy host) | Dock CPU, % of one core | Dock main thread, % | Main-thread ctx/s | Dock llvmpipe workers, % | kwin_wayland, % | plasmashell, % |")
        print("| --- | :---: | ---: | ---: | ---: | ---: | ---: | ---: |")
    for spec in args.rows:
        label, _, scenarios = spec.rpartition("=")
        pooled = []
        for scenario in scenarios.split(","):
            pooled += windows(scenario, args.dir, host_samples, args.busy_threshold)
        valid = [w for w in pooled if w["valid"]]
        busy = sum(1 for w in valid if w["host_busy_flag"])
        count = f"{len(valid)} / {busy}"
        if args.kind == "idle":
            print(f"| {label} | {count} | {rng([w['cpu'] for w in valid], 3)} | {rng([w['ctx'] for w in valid], 1)} | "
                  f"{rng([w['slices'] for w in valid if w['slices'] is not None], 1)} | {rng([w['render_slices'] for w in valid], 1)} | {rng([w['pss'] for w in valid], 1)} |")
        else:
            print(f"| {label} | {count} | {rng([w['cpu'] for w in valid], 2)} | {rng([w['main_cpu'] for w in valid if w['main_cpu'] is not None], 2)} | "
                  f"{rng([w['ctx'] for w in valid], 1)} | {rng([w['llvmpipe_cpu'] for w in valid], 2)} | "
                  f"{rng([w['kwin'] for w in valid if w['kwin'] is not None], 1)} | {rng([w['plasmashell'] for w in valid if w['plasmashell'] is not None], 1)} |")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
