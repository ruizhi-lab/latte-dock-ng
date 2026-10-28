#!/usr/bin/env python3
"""R5 helper: read-only per-thread CPU and scheduling sampler.

Companion to scripts/measure-process-family.py. That collector reports one CPU
and context-switch figure per process family; this one breaks the same window
down per thread so a cost can be attributed to a named thread (main thread,
QSGRenderThread, llvmpipe rasterizer workers, QQmlThread, D-Bus thread, ...).

It reads only /proc (no ptrace, no perf, no root) and writes one JSON object
per window on stdout. Per thread it uses /proc/<pid>/task/<tid>/schedstat, which
has nanosecond run time and the number of times the thread was put on a CPU
("timeslices"), a better wakeup proxy than the 10 ms utime/stime ticks. The
whole machine is sampled from /proc/stat so that the cost outside the requested
processes stays visible.
"""

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
from pathlib import Path
import statistics
import time


def now() -> str:
    return dt.datetime.now().astimezone().isoformat(timespec="seconds")


def find_processes(requested: dict[str, str]) -> dict[int, tuple[str, int]]:
    """Map pid -> (label, start_ticks) for every process whose exe matches."""
    found = {}
    for entry in os.scandir("/proc"):
        if not entry.name.isdecimal():
            continue
        pid = int(entry.name)
        try:
            stat = Path(f"/proc/{pid}/stat").read_text()
            start_ticks = int(stat[stat.rfind(")") + 2 :].split()[19])
            comm = Path(f"/proc/{pid}/comm").read_text().strip()
        except OSError:
            continue
        try:
            exe = os.readlink(f"/proc/{pid}/exe").removesuffix(" (deleted)")
        except OSError:
            exe = None  # capability-carrying processes such as kwin_wayland hide /proc/<pid>/exe
        key = exe if exe in requested else f"comm:{comm}" if f"comm:{comm}" in requested else None
        if key is not None:
            found[pid] = (requested[key], start_ticks)
    return found


def read_threads(pid: int) -> dict[int, dict]:
    """Per-thread snapshot; vanished threads are skipped silently."""
    threads = {}
    try:
        tids = [int(name) for name in os.listdir(f"/proc/{pid}/task") if name.isdecimal()]
    except OSError:
        return threads
    for tid in tids:
        base = f"/proc/{pid}/task/{tid}"
        try:
            comm = Path(f"{base}/comm").read_text().strip()
            run_ns, wait_ns, slices = (int(v) for v in Path(f"{base}/schedstat").read_text().split())
            vol = nonvol = 0
            for line in Path(f"{base}/status").read_text().splitlines():
                if line.startswith("voluntary_ctxt_switches:"):
                    vol = int(line.split(":", 1)[1])
                elif line.startswith("nonvoluntary_ctxt_switches:"):
                    nonvol = int(line.split(":", 1)[1])
        except (OSError, ValueError):
            continue
        threads[tid] = {"comm": comm, "run_ns": run_ns, "wait_ns": wait_ns, "slices": slices,
                        "vol": vol, "nonvol": nonvol}
    return threads


def read_system() -> dict:
    """Machine-wide CPU jiffies plus the interrupt and context-switch totals."""
    cpu = [int(v) for v in Path("/proc/stat").read_text().splitlines()[0].split()[1:]]
    names = ("user", "nice", "system", "idle", "iowait", "irq", "softirq", "steal")
    values = dict(zip(names, cpu))
    ctxt = intr = forks = 0
    for line in Path("/proc/stat").read_text().splitlines():
        if line.startswith("ctxt "):
            ctxt = int(line.split()[1])
        elif line.startswith("intr "):
            intr = int(line.split()[1])
        elif line.startswith("processes "):
            forks = int(line.split()[1])
    values["ctxt"] = ctxt
    values["intr_total"] = intr
    values["forks"] = forks
    # Per-source totals that matter in a VM: local timer, rescheduling IPIs,
    # function-call IPIs. Summed over CPUs; absent lines count as zero.
    wanted = {"LOC": "irq_local_timer", "RES": "irq_rescheduling", "CAL": "irq_function_call"}
    for key in wanted.values():
        values[key] = 0
    for line in Path("/proc/interrupts").read_text().splitlines()[1:]:
        label, _, rest = line.partition(":")
        label = label.strip()
        if label in wanted:
            values[wanted[label]] = sum(int(tok) for tok in rest.split() if tok.isdecimal())
    return values


def read_all_process_ticks() -> dict[int, tuple[int, str, int]]:
    """pid -> (utime+stime ticks, comm, start_ticks) for every process; /proc/<pid>/stat is world readable."""
    result = {}
    for entry in os.scandir("/proc"):
        if not entry.name.isdecimal():
            continue
        try:
            stat = Path(f"/proc/{entry.name}/stat").read_text()
        except OSError:
            continue
        comm = stat[stat.index("(") + 1 : stat.rindex(")")]
        fields = stat[stat.rindex(")") + 2 :].split()
        result[int(entry.name)] = (int(fields[11]) + int(fields[12]), comm, int(fields[19]))
    return result


def summarize_series(values: list[float]) -> dict:
    if not values:
        return {}
    ordered = sorted(values)
    def pct(p: float) -> float:
        return ordered[min(len(ordered) - 1, int(round(p * (len(ordered) - 1))))]
    return {
        "median": round(statistics.median(values), 3),
        "mean": round(statistics.mean(values), 3),
        "p90": round(pct(0.90), 3),
        "max": round(max(values), 3),
        "seconds_over_5_percent": sum(1 for v in values if v > 5.0),
        "seconds_total": len(values),
    }


def sample_window(requested: dict[str, str], seconds: int, label: str) -> dict:
    start_family = find_processes(requested)
    if not start_family:
        return {"event": "invalid", "label": label, "reason": "no requested process is running", "time": now()}

    snapshots = []  # one entry per second: {pid: {tid: threadinfo}}
    system_snapshots = []
    start_wall = now()
    all_before = read_all_process_ticks()
    start = time.monotonic()

    def take() -> None:
        snapshots.append({pid: read_threads(pid) for pid in start_family})
        system_snapshots.append(read_system())

    take()
    for second in range(1, seconds + 1):
        deadline = start + second
        remaining = deadline - time.monotonic()
        if remaining > 0:
            time.sleep(remaining)
        take()
    elapsed = time.monotonic() - start
    all_after = read_all_process_ticks()

    end_family = find_processes(requested)
    valid = set(end_family.items()) == set(start_family.items())

    processes = []
    for pid, (proc_label, start_ticks) in sorted(start_family.items()):
        totals: dict[int, dict] = {}
        per_second_cpu = []
        for first, second_snapshot in zip(snapshots, snapshots[1:]):
            before, after = first[pid], second_snapshot[pid]
            second_ns = 0
            for tid, info in after.items():
                if tid not in before:
                    continue  # born inside this second: counted from the next one
                prev = before[tid]
                delta_ns = max(0, info["run_ns"] - prev["run_ns"])
                second_ns += delta_ns
                entry = totals.setdefault(tid, {"comm": info["comm"], "run_ns": 0, "wait_ns": 0, "slices": 0,
                                                "vol": 0, "nonvol": 0, "seconds_seen": 0})
                entry["run_ns"] += delta_ns
                entry["wait_ns"] += max(0, info["wait_ns"] - prev["wait_ns"])
                entry["slices"] += max(0, info["slices"] - prev["slices"])
                entry["vol"] += max(0, info["vol"] - prev["vol"])
                entry["nonvol"] += max(0, info["nonvol"] - prev["nonvol"])
                entry["seconds_seen"] += 1
            per_second_cpu.append(second_ns / 1e7)  # percent of one core in a 1 s interval

        threads = []
        for tid, entry in totals.items():
            threads.append({
                "tid": tid,
                "comm": entry["comm"],
                "cpu_percent_one_core": round(entry["run_ns"] / 1e9 / elapsed * 100, 4),
                "timeslices_per_second": round(entry["slices"] / elapsed, 3),
                "voluntary_ctx_per_second": round(entry["vol"] / elapsed, 3),
                "nonvoluntary_ctx_per_second": round(entry["nonvol"] / elapsed, 3),
                "run_delay_ms_per_second": round(entry["wait_ns"] / 1e6 / elapsed, 3),
            })
        threads.sort(key=lambda item: item["cpu_percent_one_core"], reverse=True)

        processes.append({
            "label": proc_label,
            "pid": pid,
            "start_ticks": start_ticks,
            "cpu_percent_one_core": round(sum(t["cpu_percent_one_core"] for t in threads), 4),
            "timeslices_per_second": round(sum(t["timeslices_per_second"] for t in threads), 3),
            "voluntary_ctx_per_second": round(sum(t["voluntary_ctx_per_second"] for t in threads), 3),
            "nonvoluntary_ctx_per_second": round(sum(t["nonvoluntary_ctx_per_second"] for t in threads), 3),
            "per_second_cpu_percent_one_core": summarize_series(per_second_cpu),
            "thread_count_end": len(snapshots[-1][pid]),
            "threads": threads,
        })

    first_sys, last_sys = system_snapshots[0], system_snapshots[-1]
    delta = {key: last_sys[key] - first_sys[key] for key in first_sys}
    jiffies = sum(delta[key] for key in ("user", "nice", "system", "idle", "iowait", "irq", "softirq", "steal"))
    system = {"cpus": os.cpu_count(), "jiffies_total": jiffies}
    if jiffies > 0:
        for key in ("user", "nice", "system", "idle", "iowait", "irq", "softirq", "steal"):
            system[f"{key}_percent_of_machine"] = round(delta[key] * 100 / jiffies, 3)
        system["busy_percent_of_machine"] = round((jiffies - delta["idle"] - delta["iowait"]) * 100 / jiffies, 3)
        system["busy_percent_of_one_core"] = round((jiffies - delta["idle"] - delta["iowait"]) * 100 / jiffies * (os.cpu_count() or 1), 3)
    hz = os.sysconf(os.sysconf_names["SC_CLK_TCK"])
    present_both = {pid: (all_after[pid][0] - all_before[pid][0], all_after[pid][1]) for pid in all_after
                    if pid in all_before and all_before[pid][2] == all_after[pid][2]}
    others = sorted(((ticks, comm, pid) for pid, (ticks, comm) in present_both.items() if ticks > 0), reverse=True)
    system["top_processes_percent_of_one_core"] = [
        {"comm": comm, "pid": pid, "cpu_percent_one_core": round(ticks * 100 / hz / elapsed, 3)} for ticks, comm, pid in others[:10]]
    accounted_ticks = sum(ticks for ticks, _comm, _pid in others)
    busy_ticks = jiffies - delta["idle"] - delta["iowait"]
    system["cpu_not_in_surviving_processes_percent_of_one_core"] = round(max(0, busy_ticks - accounted_ticks) * 100 / hz / elapsed, 3)
    system["forks_per_second"] = round(delta["forks"] / elapsed, 3)
    system["context_switches_per_second"] = round(delta["ctxt"] / elapsed, 1)
    system["interrupts_per_second"] = round(delta["intr_total"] / elapsed, 1)
    for key in ("irq_local_timer", "irq_rescheduling", "irq_function_call"):
        system[f"{key}_per_second"] = round(delta[key] / elapsed, 1)

    return {
        "event": "r5_window",
        "label": label,
        "valid": valid,
        "start": start_wall,
        "elapsed_seconds": round(elapsed, 3),
        "load_average": list(os.getloadavg()),
        "processes": processes,
        "system": system,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--process", action="append", required=True, metavar="LABEL=EXE",
                        help="Process label and exact executable path, for example dock=/home/u/.local/bin/latte-dock-ng, or comm:NAME for processes whose /proc/<pid>/exe is hidden; repeat for more.")
    parser.add_argument("--label", default="", help="Free-form scenario label copied into every record.")
    parser.add_argument("--warmup", type=int, default=0)
    parser.add_argument("--seconds", type=int, default=60)
    parser.add_argument("--windows", type=int, default=1)
    args = parser.parse_args()

    requested = {}
    for spec in args.process:
        proc_label, _, exe = spec.partition("=")
        if not exe:
            parser.error("--process needs LABEL=EXE or LABEL=comm:NAME")
        requested[exe if exe.startswith("comm:") else str(Path(exe).resolve())] = proc_label
    if args.seconds < 2 or args.windows < 1 or args.warmup < 0:
        parser.error("seconds must be at least 2, windows at least 1, warmup nonnegative")

    if args.warmup:
        print(json.dumps({"event": "warmup", "label": args.label, "seconds": args.warmup, "time": now()}), flush=True)
        time.sleep(args.warmup)
    for index in range(1, args.windows + 1):
        record = sample_window(requested, args.seconds, args.label)
        record["index"] = index
        print(json.dumps(record, sort_keys=True), flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
