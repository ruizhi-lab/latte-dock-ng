#!/usr/bin/env python3
"""R5 helper: who rebuilds the KSycoca database during a window, and in which environment? (VM side, read-only)

Polls /proc every 20 ms for kbuildsycoca6 processes and records, once per process, its pid, parent, start
time and the environment variables that decide which database a KDE process works against (XDG_CONFIG_DIRS,
XDG_DATA_DIRS, XDG_CURRENT_DESKTOP, XDG_MENU_PREFIX, LANG, LANGUAGE). It also polls the mtime of
~/.cache/ksycoca6_* every 50 ms. Environment values are KDE search paths and locale names, nothing private.
Output: one JSON object on stdout after --seconds (default 60), preceded by an optional --warmup sleep.
"""

import argparse
import glob
import json
import os
import time
from pathlib import Path

KEYS = ("XDG_CONFIG_DIRS", "XDG_DATA_DIRS", "XDG_CURRENT_DESKTOP", "XDG_MENU_PREFIX", "LANG", "LANGUAGE", "LC_MESSAGES")


def environ(pid: int) -> dict:
    try:
        raw = Path(f"/proc/{pid}/environ").read_bytes().split(b"\0")
    except OSError:
        return {}
    env = {}
    for item in raw:
        name, _, value = item.decode(errors="replace").partition("=")
        if name in KEYS:
            env[name] = value
    return env


def comm(pid: int) -> str:
    try:
        return Path(f"/proc/{pid}/comm").read_text().strip()
    except OSError:
        return "?"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--seconds", type=float, default=60)
    parser.add_argument("--warmup", type=float, default=0)
    args = parser.parse_args()
    time.sleep(args.warmup)
    seen: dict[int, dict] = {}
    mtimes: dict[str, set[int]] = {}
    start = time.monotonic()
    next_stat = 0.0
    while time.monotonic() - start < args.seconds:
        for entry in os.scandir("/proc"):
            if not entry.name.isdecimal():
                continue
            pid = int(entry.name)
            if pid in seen or comm(pid) != "kbuildsycoca6":
                continue
            try:
                stat = Path(f"/proc/{pid}/stat").read_text()
                ppid = int(stat[stat.rindex(")") + 2 :].split()[1])
            except OSError:
                ppid = -1
            seen[pid] = {"pid": pid, "ppid": ppid, "parent": comm(ppid), "t": round(time.monotonic() - start, 2), "env": environ(pid),
                         "cmdline": Path(f"/proc/{pid}/cmdline").read_bytes().replace(b"\0", b" ").decode(errors="replace").strip() if os.path.exists(f"/proc/{pid}") else ""}
        now = time.monotonic() - start
        if now >= next_stat:
            for path in glob.glob(os.path.expanduser("~/.cache/ksycoca6_*")):
                if path.endswith(".lock"):
                    continue
                try:
                    mtimes.setdefault(os.path.basename(path), set()).add(os.stat(path).st_mtime_ns)
                except OSError:
                    pass
            next_stat = now + 0.05
        time.sleep(0.02)
    runs = sorted(seen.values(), key=lambda r: r["t"])
    summary: dict[str, dict] = {}
    for run in runs:
        key = json.dumps({"parent": run["parent"], **run["env"]}, sort_keys=True)
        summary.setdefault(key, {"count": 0})["count"] += 1
    print(json.dumps({
        "seconds": args.seconds,
        "kbuildsycoca6_processes": len(runs),
        "per_second": round(len(runs) / args.seconds, 3),
        "by_parent_and_environment": [{"count": v["count"], **json.loads(k)} for k, v in summary.items()],
        "first_runs": runs[:6],
        "sycoca_file_versions_seen": {name: len(values) for name, values in mtimes.items()},
    }, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
