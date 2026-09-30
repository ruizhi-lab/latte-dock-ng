#!/usr/bin/env python3
"""Collect read-only CPU, memory and context-switch samples from named processes."""

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
from pathlib import Path
import statistics
import time


def process_record(pid: int, requested: set[str]) -> dict | None:
    proc = Path("/proc") / str(pid)
    try:
        comm = (proc / "comm").read_text().strip()
        if comm not in {Path(path).name[:15] for path in requested}:
            return None
        exe = os.readlink(proc / "exe").removesuffix(" (deleted)")
        if exe not in requested:
            return None
        stat = (proc / "stat").read_text()
        fields = stat[stat.rfind(")") + 2 :].split()
        status = (proc / "status").read_text().splitlines()
    except (FileNotFoundError, PermissionError, ProcessLookupError, OSError):
        return None

    counters = {}
    for line in status:
        if line.startswith(("voluntary_ctxt_switches:", "nonvoluntary_ctxt_switches:")):
            key, value = line.split(":", 1)
            counters[key] = int(value.strip())

    return {
        "pid": pid,
        "exe": exe,
        "start_ticks": int(fields[19]),
        "cpu_ticks": int(fields[11]) + int(fields[12]),
        "voluntary_context_switches": counters.get("voluntary_ctxt_switches", 0),
        "nonvoluntary_context_switches": counters.get("nonvoluntary_ctxt_switches", 0),
    }


def process_family(requested: set[str]) -> dict[tuple[int, int, str], dict]:
    family = {}
    for entry in os.scandir("/proc"):
        if not entry.name.isdecimal():
            continue
        record = process_record(int(entry.name), requested)
        if record is not None:
            key = (record["pid"], record["start_ticks"], record["exe"])
            family[key] = record
    return family


def memory_kib(record: dict) -> dict[str, int]:
    values = {}
    try:
        for line in (Path("/proc") / str(record["pid"]) / "smaps_rollup").read_text().splitlines():
            key, _, value = line.partition(":")
            if key in {"Pss", "Rss", "Private_Clean", "Private_Dirty", "Private_Hugetlb"}:
                values[key] = int(value.strip().split()[0])
    except (FileNotFoundError, PermissionError, ProcessLookupError, OSError):
        return {}

    if "Pss" not in values or "Rss" not in values:
        return {}
    values["Private"] = sum(values.get(key, 0) for key in ("Private_Clean", "Private_Dirty", "Private_Hugetlb"))
    return values


def family_memory(family: dict) -> dict[str, int] | None:
    total = {"Pss": 0, "Rss": 0, "Private": 0}
    for record in family.values():
        values = memory_kib(record)
        if not values:
            return None
        for key in total:
            total[key] += values[key]
    return total


def load_average() -> tuple[float, float, float]:
    return tuple(os.getloadavg())


def now() -> str:
    return dt.datetime.now().astimezone().isoformat(timespec="seconds")


def sleep_until(deadline: float) -> None:
    remaining = deadline - time.monotonic()
    if remaining > 0:
        time.sleep(remaining)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--process", action="append", required=True, help="Exact executable path; repeat for helpers.")
    parser.add_argument("--warmup", type=int, default=30)
    parser.add_argument("--interval", type=int, default=60)
    parser.add_argument("--windows", type=int, default=5)
    parser.add_argument("--memory-period", type=int, default=5)
    args = parser.parse_args()

    requested = {str(Path(path).resolve()) for path in args.process}
    primary = str(Path(args.process[0]).resolve())
    if args.warmup < 0 or args.interval < 1 or args.windows < 1 or args.memory_period < 1:
        parser.error("warmup must be nonnegative; interval, windows and memory-period must be positive")

    family = process_family(requested)
    if not family or not any(item["exe"] == primary for item in family.values()):
        parser.error("the primary process is not running at the requested executable path")

    print(json.dumps({"event": "warmup_start", "time": now(), "seconds": args.warmup,
                      "processes": [{"pid": item["pid"], "exe": item["exe"]} for item in family.values()]},
                     sort_keys=True), flush=True)
    warmup_end = time.monotonic() + args.warmup
    while time.monotonic() < warmup_end:
        sleep_until(min(warmup_end, time.monotonic() + 1))
        if set(process_family(requested)) != set(family):
            print(json.dumps({"event": "invalid", "reason": "process family changed during warmup", "time": now()}), flush=True)
            return 2

    print(json.dumps({"event": "warmup_complete", "time": now()}, sort_keys=True), flush=True)
    hz = os.sysconf(os.sysconf_names["SC_CLK_TCK"])
    for index in range(1, args.windows + 1):
        baseline = process_family(requested)
        identity = set(baseline)
        if not identity:
            print(json.dumps({"event": "invalid", "reason": "no process family at sample start", "time": now()}), flush=True)
            return 2

        initial_memory = family_memory(baseline)
        if initial_memory is None:
            print(json.dumps({"event": "invalid", "reason": "smaps_rollup unavailable at sample start", "time": now()}), flush=True)
            return 2

        start_time = time.monotonic()
        wall_start = now()
        load_start = load_average()
        memory_samples = [initial_memory]
        invalid_reasons = []
        next_sample = start_time + 1
        for second in range(1, args.interval + 1):
            sleep_until(next_sample)
            current = process_family(requested)
            if set(current) != identity:
                invalid_reasons.append(f"process family changed at second {second}")
            if second % args.memory_period == 0 or second == args.interval:
                values = family_memory(current)
                if values is None:
                    invalid_reasons.append(f"smaps_rollup unavailable at second {second}")
                else:
                    memory_samples.append(values)
            next_sample = start_time + second + 1

        end = process_family(requested)
        elapsed = time.monotonic() - start_time
        if set(end) != identity:
            invalid_reasons.append("process family changed at sample end")
        final_memory = family_memory(end)
        if final_memory is None:
            invalid_reasons.append("smaps_rollup unavailable at sample end")
        else:
            memory_samples.append(final_memory)

        cpu_delta = sum(end[key]["cpu_ticks"] - baseline[key]["cpu_ticks"] for key in identity if key in end and key in baseline)
        context_delta = sum(
            (end[key]["voluntary_context_switches"] - baseline[key]["voluntary_context_switches"])
            + (end[key]["nonvoluntary_context_switches"] - baseline[key]["nonvoluntary_context_switches"])
            for key in identity if key in end and key in baseline
        )
        memory_means = {
            name: round(statistics.mean(sample[name] for sample in memory_samples))
            for name in ("Pss", "Private", "Rss")
        } if memory_samples else {}
        result = {
            "event": "window",
            "index": index,
            "valid": not invalid_reasons,
            "invalid_reasons": invalid_reasons,
            "start": wall_start,
            "elapsed_seconds": round(elapsed, 3),
            "processes": [{"pid": item["pid"], "start_ticks": item["start_ticks"], "exe": item["exe"]}
                          for item in baseline.values()],
            "cpu_percent_one_core": round(cpu_delta * 100 / hz / elapsed, 4),
            "context_switches_per_second": round(context_delta / elapsed, 3),
            "context_switches_total": context_delta,
            "memory_samples": len(memory_samples),
            "memory_mean_kib": memory_means,
            "load_average_start": load_start,
            "load_average_end": load_average(),
        }
        print(json.dumps(result, sort_keys=True), flush=True)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
