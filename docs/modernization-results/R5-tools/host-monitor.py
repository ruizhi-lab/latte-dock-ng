#!/usr/bin/env python3
"""R5 helper: host-side load and QEMU CPU recorder (read-only, runs on the developer host).

The Fedora guest shares the host with other work, so an idle-CPU window is only
representative when the host was quiet. This samples, every few seconds, the
host load average, the host's busy and steal percentages and the CPU time that
the guest's QEMU process (and each of its vCPU threads) used. The in-guest
collectors cannot see any of this. The QEMU numbers also give the cost of the
guest as the host sees it, which is the ground truth for "does the guest idle".
"""

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
from pathlib import Path
import time


def find_qemu_pid(domain: str) -> int | None:
    needle = f"guest={domain},".encode()
    for entry in os.scandir("/proc"):
        if not entry.name.isdecimal():
            continue
        try:
            cmdline = Path(f"/proc/{entry.name}/cmdline").read_bytes()
        except OSError:
            continue
        if needle in cmdline and b"qemu-system" in cmdline.split(b"\0", 1)[0]:
            return int(entry.name)
    return None


def host_cpu() -> tuple[int, int, int]:
    fields = [int(v) for v in Path("/proc/stat").read_text().splitlines()[0].split()[1:]]
    idle = fields[3] + fields[4]
    steal = fields[7] if len(fields) > 7 else 0
    return sum(fields), idle, steal


def thread_ticks(pid: int) -> dict[str, int]:
    ticks = {}
    for tid in os.listdir(f"/proc/{pid}/task"):
        try:
            stat = Path(f"/proc/{pid}/task/{tid}/stat").read_text()
        except OSError:
            continue
        name = stat[stat.index("(") + 1 : stat.rindex(")")]
        fields = stat[stat.rindex(")") + 2 :].split()
        ticks[f"{name}#{tid}"] = int(fields[11]) + int(fields[12])
    return ticks


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--domain", default="fedora")
    parser.add_argument("--interval", type=float, default=5.0)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()

    hz = os.sysconf(os.sysconf_names["SC_CLK_TCK"])
    previous = None
    with args.out.open("a") as out:
        while True:
            pid = find_qemu_pid(args.domain)
            total, idle, steal = host_cpu()
            ticks = thread_ticks(pid) if pid else {}
            monotonic = time.monotonic()
            record = {
                "time": dt.datetime.now().astimezone().isoformat(timespec="seconds"),
                "loadavg": [float(v) for v in Path("/proc/loadavg").read_text().split()[:3]],
                "qemu_pid": pid,
            }
            if previous and previous["pid"] == pid:
                elapsed = monotonic - previous["monotonic"]
                dtotal = total - previous["total"]
                if dtotal > 0:
                    record["host_busy_percent"] = round((dtotal - (idle - previous["idle"])) * 100 / dtotal, 2)
                    record["host_steal_percent"] = round((steal - previous["steal"]) * 100 / dtotal, 3)
                per_thread = {name: (count - previous["ticks"].get(name, count)) * 100 / hz / elapsed
                              for name, count in ticks.items()}
                record["qemu_cpu_percent_of_one_core"] = round(sum(per_thread.values()), 3)
                record["qemu_busy_threads"] = {name: round(value, 2) for name, value in per_thread.items() if value >= 0.5}
            out.write(json.dumps(record, sort_keys=True) + "\n")
            out.flush()
            previous = {"pid": pid, "monotonic": monotonic, "total": total, "idle": idle, "steal": steal, "ticks": ticks}
            time.sleep(args.interval)


if __name__ == "__main__":
    raise SystemExit(main())
