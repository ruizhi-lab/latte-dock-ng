#!/usr/bin/env python3
"""Collect fixed-interval process-family metrics for one Fedora P4 A/A run."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import time


def proc_stat(path: Path) -> dict:
    raw = path.read_text()
    close = raw.rfind(")")
    if close < 0:
        raise ValueError(f"malformed proc stat: {path}")
    fields = raw[close + 2 :].split()
    return {
        "state": fields[0],
        "ppid": int(fields[1]),
        "utime": int(fields[11]),
        "stime": int(fields[12]),
        "starttime": int(fields[19]),
        "minflt": int(fields[7]),
        "majflt": int(fields[9]),
        "vsize": int(fields[20]),
        "rss_pages": int(fields[21]),
    }


def identity(proc_root: Path, pid: int, expected_start: int | None = None) -> dict:
    base = proc_root / str(pid)
    stat = proc_stat(base / "stat")
    executable = os.readlink(base / "exe")
    if expected_start is not None and stat["starttime"] != expected_start:
        raise ValueError(f"PID {pid} was reused (start time changed)")
    return {**stat, "pid": pid, "exe": executable}


def descendants(proc_root: Path, root_pid: int) -> list[int]:
    parents = {}
    for entry in proc_root.iterdir():
        if not entry.name.isdecimal():
            continue
        try:
            parents[int(entry.name)] = proc_stat(entry / "stat")["ppid"]
        except (OSError, ValueError, IndexError):
            continue
    found = {root_pid}
    changed = True
    while changed:
        changed = False
        for pid, parent in parents.items():
            if parent in found and pid not in found:
                found.add(pid)
                changed = True
    return sorted(found)


def staged_latte_processes(proc_root: Path, staged_bin: Path) -> set[int]:
    found = set()
    for entry in proc_root.iterdir():
        if not entry.name.isdecimal():
            continue
        try:
            executable = Path(os.readlink(entry / "exe"))
        except OSError:
            continue
        if executable.parent == staged_bin and executable.name.startswith("latte-dock-ng"):
            found.add(int(entry.name))
    return found


def rollup(path: Path) -> dict:
    values = {}
    for line in path.read_text().splitlines():
        key, _, value = line.partition(":")
        if key in {"Rss", "Pss", "Private_Clean", "Private_Dirty", "Private_Hugetlb"}:
            values[key] = int(value.strip().split()[0])
    required = {"Rss", "Pss", "Private_Clean", "Private_Dirty"}
    if not required.issubset(values):
        raise ValueError(f"missing smaps_rollup fields in {path}")
    values["Private_kB"] = values["Private_Clean"] + values["Private_Dirty"] + values.get("Private_Hugetlb", 0)
    return values


def cache_fingerprint(cache_dir: Path) -> str:
    digest = hashlib.sha256()
    if cache_dir.exists():
        for path in sorted(cache_dir.glob("ksycoca6_*")):
            if not path.is_file():
                continue
            stat = path.stat()
            digest.update(f"{path.name}\0{stat.st_size}\0{stat.st_mtime_ns}\n".encode())
    return digest.hexdigest()


def sample(proc_root: Path, pid: int, expected_start: int, expected_exe: str,
           seen_starts: dict[int, int], cache_dir: Path,
           expected_family: set[int] | None) -> tuple[dict, set[int]]:
    dock = identity(proc_root, pid, expected_start)
    if dock["exe"] != expected_exe:
        raise ValueError(f"dock executable changed: {dock['exe']}")
    pids = descendants(proc_root, pid)
    family_ids = set(pids)
    if expected_family is not None and family_ids != expected_family:
        raise ValueError(f"process family changed: {sorted(expected_family)} -> {sorted(family_ids)}")
    staged_family = staged_latte_processes(proc_root, Path(expected_exe).parent)
    if not staged_family.issubset(set(pids)):
        raise ValueError(f"unaccounted staged Latte process(es): {sorted(staged_family - set(pids))}")
    rows = []
    for child_pid in pids:
        row = identity(proc_root, child_pid, seen_starts.get(child_pid))
        seen_starts[child_pid] = row["starttime"]
        rows.append(row)
    memory = [rollup(proc_root / str(row["pid"]) / "smaps_rollup") for row in rows]
    # Recheck after memory reads: a PID can exit and be reused while a sample
    # is being assembled, which would otherwise pair one identity with another
    # process's smaps_rollup.
    for row in rows:
        after = identity(proc_root, row["pid"], row["starttime"])
        if after["exe"] != row["exe"]:
            raise ValueError(f"PID {row['pid']} executable changed while sampling")
    identity(proc_root, pid, expected_start)
    if set(descendants(proc_root, pid)) != family_ids:
        raise ValueError("process family changed while sampling")
    try:
        load = (proc_root / "loadavg").read_text().split()[:3]
    except OSError:
        load = Path("/proc/loadavg").read_text().split()[:3]
    cache_state = cache_fingerprint(cache_dir)
    return {
        "monotonic_ns": time.monotonic_ns(),
        "wall_time": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
        "pid": pid,
        "starttime_ticks": expected_start,
        "executable": expected_exe,
        "family_count": len(rows),
        "family": json.dumps([{"pid": r["pid"], "starttime_ticks": r["starttime"], "exe": r["exe"]} for r in rows], sort_keys=True),
        "cpu_user_ticks": sum(r["utime"] for r in rows),
        "cpu_system_ticks": sum(r["stime"] for r in rows),
        "rss_kB": sum(m["Rss"] for m in memory),
        "pss_kB": sum(m["Pss"] for m in memory),
        "private_kB": sum(m["Private_kB"] for m in memory),
        "load_1m": load[0], "load_5m": load[1], "load_15m": load[2],
        "ksycoca_sha256": cache_state,
    }, family_ids


def sample_deadlines(start: float, duration: int, interval: float) -> list[float]:
    """Return inclusive sample boundaries so duration seconds spans duration+1 rows."""
    return [start + index * interval for index in range(duration + 1)]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pid", type=int, required=True)
    parser.add_argument("--starttime-ticks", type=int, required=True)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--duration", type=int, default=60)
    parser.add_argument("--interval", type=float, default=1.0)
    parser.add_argument("--cache-dir", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--proc-root", type=Path, default=Path("/proc"), help=argparse.SUPPRESS)
    parser.add_argument("--identity-json", action="store_true", help="print validated process identity and exit")
    args = parser.parse_args()
    if args.identity_json:
        current = identity(args.proc_root, args.pid, args.starttime_ticks)
        if current["exe"] != args.exe:
            raise ValueError(f"dock executable changed: {current['exe']}")
        print(json.dumps(current, sort_keys=True))
        return 0
    if args.cache_dir is None or args.output is None:
        parser.error("--cache-dir and --output are required for sample collection")
    if args.duration != 60 or args.interval != 1.0:
        parser.error("the frozen baseline window is 60 seconds with 1-second intervals")
    if not args.output.parent.is_dir():
        parser.error("output directory must already exist")
    hz = os.sysconf("SC_CLK_TCK")
    fields = ["monotonic_ns", "wall_time", "pid", "starttime_ticks", "executable", "family_count", "family",
              "cpu_user_ticks", "cpu_system_ticks", "rss_kB", "pss_kB", "private_kB",
              "load_1m", "load_5m", "load_15m", "ksycoca_sha256", "cpu_hz"]
    seen_starts: dict[int, int] = {args.pid: args.starttime_ticks}
    deadlines = sample_deadlines(time.monotonic(), args.duration, args.interval)
    initial_cache = None
    expected_family = None
    with args.output.open("x", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        for index, deadline in enumerate(deadlines):
            if index:
                time.sleep(max(0, deadline - time.monotonic()))
            row, current_family = sample(args.proc_root, args.pid, args.starttime_ticks, args.exe,
                                         seen_starts, args.cache_dir, expected_family)
            expected_family = current_family
            if initial_cache is None:
                initial_cache = row["ksycoca_sha256"]
            elif row["ksycoca_sha256"] != initial_cache:
                raise ValueError("KSycoca cache changed during sampling window")
            row["cpu_hz"] = hz
            writer.writerow(row)
            stream.flush()
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, IndexError) as error:
        raise SystemExit(f"AA_INVALID_SAMPLE: {error}")
