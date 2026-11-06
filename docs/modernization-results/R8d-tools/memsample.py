#!/usr/bin/env python3
"""Sample the memory of running processes at a fixed interval (R8d idle study).

The sampler only reads /proc (stat, status, fd, smaps_rollup, smaps, comm, exe) and
runs `coredumpctl list`. It never signals, attaches to, traces or restarts a watched
process, so it can also watch the desktop's dock.

Every interval it appends one CSV row per watched process to <out>/samples.csv:
elapsed seconds since the first sample, wall-clock time, the series label, pid and
comm, the smaps_rollup sizes (Rss, Pss, Pss_Anon, Pss_File, Pss_Shmem, Private_Dirty,
Swap), the status sizes (VmRSS, RssAnon, RssFile, RssShmem, VmData), Threads, the
number of open file descriptors and CpuTicks (utime + stime, in clock ticks). Sizes
are KiB as the kernel reports them; an empty cell means the kernel did not report
that field.

At each snapshot time (default 0, 15 and 75 minutes, and the end) it writes
<out>/snapshots/<label>-<series>.json with the 15 mappings of largest Pss and the
smaps totals grouped by object (a file path, or anonymous mappings per permission
set). The per-mapping smaps of this kernel has no Pss_Anon, so mappings are compared
by Pss and Anonymous. The grouped totals keep a change attributable even when a
mapping leaves the top 15 between two snapshots.

Sampling ends when the duration is reached, when the stop file exists, or when a
watched process exits. Then it records the coredumps that coredumpctl lists for
latte-dock-ng since the start, writes <out>/summary.json, creates the --finish-touch
file if one was given (the launcher tears its session down when it sees that file)
and writes <out>/done last, so a waiter sees complete output.
"""

from __future__ import annotations

import argparse
import csv
import json
import os
import re
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path

ROLLUP_KEYS = ("Rss", "Pss", "Pss_Anon", "Pss_File", "Pss_Shmem", "Private_Dirty", "Swap")
STATUS_KIB_KEYS = ("VmRSS", "RssAnon", "RssFile", "RssShmem", "VmData")
# Per-mapping smaps fields. This kernel reports no Pss_Anon per mapping.
SMAPS_KEYS = ("Rss", "Pss", "Anonymous", "Private_Dirty", "Swap")
CSV_COLUMNS = (
    "elapsed_s",
    "wall_time",
    "series",
    "pid",
    "comm",
    *ROLLUP_KEYS,
    *STATUS_KIB_KEYS,
    "Threads",
    "Fds",
    "CpuTicks",
)
TOP_MAPPINGS = 15
COREDUMP_EXE = "latte-dock-ng"

KIB_FIELD = re.compile(r"^([A-Za-z_]+):\s+(\d+) kB\s*$")
SMAPS_HEADER = re.compile(r"^([0-9a-f]+)-([0-9a-f]+)\s+(\S+)\s+([0-9a-f]+)\s+(\S+)\s+(\d+)\s*(.*)$")


class ProcessGone(Exception):
    """The process exited, became a zombie, or its pid now belongs to another process."""


def now_stamp() -> str:
    return datetime.now().isoformat(timespec="seconds")


class Unreadable(Exception):
    """The file exists but this user may not read it (a non-dumpable process hides smaps, fd and exe)."""


def read_proc(path: str) -> str:
    try:
        with open(path, encoding="utf-8", errors="replace") as handle:
            return handle.read()
    except PermissionError as error:
        # Permission is not an exit: KWin is non-dumpable here and still shows stat and status.
        raise Unreadable(f"{path}: {error.strerror or error}") from error
    except OSError as error:
        raise ProcessGone(f"{path}: {error.strerror or error}") from error


def parse_stat(text: str) -> dict:
    # The command name sits in parentheses and may contain spaces and parentheses, so
    # split at the last ")" and index the fields after it (state is field 3, index 0).
    head, _, tail = text.rpartition(")")
    fields = tail.split()
    return {
        "comm": head[head.index("(") + 1 :],
        "state": fields[0],
        "utime": int(fields[11]),
        "stime": int(fields[12]),
        "starttime": int(fields[19]),
    }


def parse_rollup(text: str) -> dict:
    values = {}
    for line in text.splitlines():
        match = KIB_FIELD.match(line)
        if match and match.group(1) in ROLLUP_KEYS:
            values[match.group(1)] = int(match.group(2))
    return values


def parse_status(text: str) -> dict:
    values = {}
    for line in text.splitlines():
        key, _, rest = line.partition(":")
        value = rest.strip()
        if key in STATUS_KIB_KEYS:
            values[key] = int(value.split()[0])
        elif key == "Threads":
            values[key] = int(value)
    return values


def parse_smaps(text: str) -> list:
    mappings: list = []
    current = None
    for line in text.splitlines():
        header = SMAPS_HEADER.match(line)
        if header:
            start, end, perms, offset, dev, inode, path = header.groups()
            current = {
                "start": start,
                "end": end,
                "perms": perms,
                "offset": offset,
                "dev": dev,
                "inode": int(inode),
                "path": path.strip(),
            }
            mappings.append(current)
            continue
        match = KIB_FIELD.match(line)
        if current is not None and match and match.group(1) in SMAPS_KEYS:
            current[match.group(1)] = int(match.group(2))
    return mappings


def summarize_smaps(mappings: list) -> dict:
    ranked = sorted(mappings, key=lambda m: (-m.get("Pss", 0), m["start"]))
    grouped: dict = {}
    for mapping in mappings:
        key = mapping["path"] or f"[anon {mapping['perms']}]"
        entry = grouped.setdefault(key, {"mappings": 0, **{name: 0 for name in SMAPS_KEYS}})
        entry["mappings"] += 1
        for name in SMAPS_KEYS:
            entry[name] += mapping.get(name, 0)
    return {
        "mappings_total": len(mappings),
        "top": ranked[:TOP_MAPPINGS],
        "by_object": {key: grouped[key] for key in sorted(grouped)},
    }


class Watch:
    """One watched process. Its start time detects a pid that was reused by another process."""

    def __init__(self, series: str, pid: int) -> None:
        self.series = series
        self.pid = pid
        stat = parse_stat(read_proc(f"/proc/{pid}/stat"))
        if stat["state"] == "Z":
            raise ProcessGone(f"pid {pid} is a zombie")
        self.comm = stat["comm"]
        self.starttime = stat["starttime"]
        # Fields this user may not read; their CSV cells stay empty and the sampler says so once.
        self.unreadable: set = set()
        self.reported = False
        try:
            self.exe = os.readlink(f"/proc/{pid}/exe")
        except OSError:
            self.exe = "unknown"

    def check_alive(self) -> dict:
        stat = parse_stat(read_proc(f"/proc/{self.pid}/stat"))
        if stat["state"] == "Z" or stat["starttime"] != self.starttime:
            raise ProcessGone(f"{self.series} pid {self.pid} exited or its pid was reused")
        return stat

    def optional(self, name: str, path: str):
        """Text of a file this user may read, or None when it is hidden (recorded in self.unreadable)."""
        try:
            return read_proc(path)
        except Unreadable:
            self.unreadable.add(name)
            return None

    def sample(self, elapsed: float, stamp: str) -> dict:
        stat = self.check_alive()
        rollup_text = self.optional("smaps_rollup", f"/proc/{self.pid}/smaps_rollup")
        status_text = self.optional("status", f"/proc/{self.pid}/status")
        rollup = parse_rollup(rollup_text) if rollup_text is not None else {}
        status = parse_status(status_text) if status_text is not None else {}
        fds: object = ""
        try:
            fds = len(os.listdir(f"/proc/{self.pid}/fd"))
        except PermissionError:
            self.unreadable.add("fd")
        except OSError as error:
            raise ProcessGone(f"{self.series} pid {self.pid}: {error.strerror or error}") from error
        row = {
            "elapsed_s": f"{elapsed:.1f}",
            "wall_time": stamp,
            "series": self.series,
            "pid": self.pid,
            "comm": self.comm,
            "Threads": status.get("Threads", ""),
            "Fds": fds,
            "CpuTicks": stat["utime"] + stat["stime"],
        }
        row.update({key: rollup.get(key, "") for key in ROLLUP_KEYS})
        row.update({key: status.get(key, "") for key in STATUS_KIB_KEYS})
        return row

    def snapshot(self, out: Path, label: str, elapsed: float, stamp: str, gpu_mib) -> None:
        self.check_alive()
        data = {
            "label": label,
            "series": self.series,
            "pid": self.pid,
            "comm": self.comm,
            "exe": self.exe,
            "elapsed_s": round(elapsed, 1),
            "wall_time": stamp,
            # nvidia-smi's GPU memory of the pid: None when nvidia-smi cannot run, 0 when not listed.
            "gpu_mib": gpu_mib,
        }
        text = self.optional("smaps", f"/proc/{self.pid}/smaps")
        if text is None:
            # No mapping table for this series: the analysis skips the mapping comparison.
            data["unreadable"] = "smaps is not readable by this user (non-dumpable process)"
        else:
            data.update(summarize_smaps(parse_smaps(text)))
        path = out / "snapshots" / f"{label}-{self.series}.json"
        path.write_text(json.dumps(data, indent=1, sort_keys=True) + "\n", encoding="utf-8")


def coredump_report() -> dict:
    try:
        result = subprocess.run(
            ["coredumpctl", "list", "--no-pager", "--no-legend", COREDUMP_EXE],
            capture_output=True,
            text=True,
            timeout=120,
        )
    except (OSError, subprocess.TimeoutExpired) as error:
        return {"lines": [], "error": str(error)}
    lines = [line.strip() for line in result.stdout.splitlines() if COREDUMP_EXE in line]
    # coredumpctl exits 1 when nothing matches; any other failure is reported.
    error = None if result.returncode in (0, 1) else (result.stderr.strip() or f"status {result.returncode}")
    return {"lines": lines, "error": error}


GPU_PROCESS_LINE = re.compile(r"^\|\s+\d+\s+\S+\s+\S+\s+(\d+)\s+\S+\s+.*?\s(\d+)MiB\s+\|\s*$")


def gpu_memory_mib():
    """GPU memory per pid from nvidia-smi's process table; None when nvidia-smi cannot run.

    The nested KWin and dock render on the host's NVIDIA GPU, so their video memory is
    not part of their RSS. It is read at the snapshot times only, not at every sample.
    """
    try:
        result = subprocess.run(["nvidia-smi"], capture_output=True, text=True, timeout=60)
    except (OSError, subprocess.TimeoutExpired):
        return None
    if result.returncode != 0:
        return None
    usage: dict = {}
    for line in result.stdout.splitlines():
        match = GPU_PROCESS_LINE.match(line)
        if match:
            pid = int(match.group(1))
            usage[pid] = usage.get(pid, 0) + int(match.group(2))
    return usage


def parse_watch_args(values: list, is_file: bool) -> list:
    specs = []
    for value in values:
        series, sep, target = value.partition("=")
        if not sep or not series or not target:
            raise SystemExit(f"expected LABEL=PID or LABEL=PIDFILE, got {value!r}")
        if is_file:
            specs.append((series, None, target))
        elif target.isdigit():
            specs.append((series, int(target), None))
        else:
            raise SystemExit(f"PID must be a number: {value!r}")
    return specs


def attach(specs: list, timeout_s: float) -> tuple:
    """Return ({series: Watch}, pending specs). A pid file may appear after the sampler starts."""
    deadline = time.monotonic() + timeout_s
    watches: dict = {}
    pending = list(specs)
    while True:
        still = []
        for series, pid, pidfile in pending:
            if pid is None:
                try:
                    pid = int(Path(pidfile).read_text(encoding="utf-8").strip())
                except (OSError, ValueError):
                    still.append((series, None, pidfile))
                    continue
            try:
                watches[series] = Watch(series, pid)
            except ProcessGone:
                # A pid file may still name the previous process: read it again next time.
                still.append((series, None if pidfile else pid, pidfile))
        pending = still
        if not pending or time.monotonic() >= deadline:
            return watches, pending
        time.sleep(0.5)


def label_for(seconds: float) -> str:
    return f"{seconds / 60:g}min"


class Sampler:
    def __init__(self, args: argparse.Namespace) -> None:
        self.args = args
        self.out = Path(args.out)
        self.started = time.monotonic()
        self.t0 = self.started
        self.watches: list = []
        self.snapshots: list = []
        self.ticks = 0
        self.last_elapsed = 0.0
        self.reason = None
        self.gone = None
        self.baseline = {"lines": [], "error": None}
        self.wall_start = now_stamp()

    def note(self, message: str) -> None:
        # Journal lines count from the sampler's start; CSV elapsed_s counts from the first sample.
        line = f"{now_stamp()} +{time.monotonic() - self.started:.1f}s {message}"
        print(line, flush=True)
        with (self.out / "events.log").open("a", encoding="utf-8") as handle:
            handle.write(line + "\n")

    def snapshot(self, label: str) -> None:
        elapsed = time.monotonic() - self.t0
        usage = gpu_memory_mib()  # one nvidia-smi call per snapshot time, not per sample
        for watch in self.watches:
            gpu_mib = None if usage is None else usage.get(watch.pid, 0)
            try:
                watch.snapshot(self.out, label, elapsed, now_stamp(), gpu_mib)
                self.snapshots.append(f"{label}-{watch.series}.json")
                self.note(f"snapshot {label} written for {watch.series} (pid {watch.pid})")
            except ProcessGone as error:
                self.note(f"snapshot {label} skipped: {error}")

    def run(self) -> int:
        args = self.args
        self.out.mkdir(parents=True, exist_ok=True)
        (self.out / "snapshots").mkdir(exist_ok=True)
        specs = parse_watch_args(args.watch, is_file=False) + parse_watch_args(args.watch_file, is_file=True)
        if not specs:
            raise SystemExit("nothing to watch: pass --watch or --watch-file")
        attached, pending = attach(specs, args.attach_timeout_s)
        self.watches = [attached[series] for series, _, _ in specs if series in attached]
        if pending:
            self.reason = "not attached: " + ", ".join(series for series, _, _ in pending)
            self.note(self.reason)
            return 2  # main() writes the summary and done file once

        self.t0 = time.monotonic()
        self.baseline = coredump_report()
        for watch in self.watches:
            self.note(f"watching {watch.series} pid {watch.pid} comm {watch.comm} exe {watch.exe}")

        interval = float(args.interval_s)
        duration = float(args.duration_min) * 60.0
        pending_snapshots = sorted({float(item) * 60.0 for item in args.snapshot_min.split(",") if item.strip()})
        stop_file = Path(args.stop_file) if args.stop_file else None
        csv_path = self.out / "samples.csv"
        write_header = not csv_path.exists() or csv_path.stat().st_size == 0
        with csv_path.open("a", newline="", encoding="utf-8") as handle:
            writer = csv.DictWriter(handle, fieldnames=CSV_COLUMNS, lineterminator="\n")
            if write_header:
                writer.writeheader()
                handle.flush()
            tick = 0
            while True:
                due = min(tick * interval, duration)
                delay = self.t0 + due - time.monotonic()
                if delay > 0:
                    time.sleep(delay)
                elapsed = time.monotonic() - self.t0
                if stop_file is not None and stop_file.exists():
                    self.reason = "stop file"
                    break
                stamp = now_stamp()
                rows = []
                for watch in self.watches:
                    try:
                        rows.append(watch.sample(elapsed, stamp))
                    except ProcessGone as error:
                        self.gone = str(error)
                        break
                if self.gone is not None:
                    self.reason = f"process exited: {self.gone}"
                    self.note(self.reason)
                    break
                for row in rows:
                    writer.writerow(row)
                handle.flush()
                self.ticks += 1
                for watch in self.watches:
                    if watch.unreadable and not watch.reported:
                        watch.reported = True
                        self.note(f"{watch.series}: not readable by this user {sorted(watch.unreadable)}; those cells stay empty")
                self.last_elapsed = elapsed
                while pending_snapshots and pending_snapshots[0] <= due + 1e-6:
                    self.snapshot(label_for(pending_snapshots.pop(0)))
                if due >= duration:
                    self.reason = "duration reached"
                    break
                tick += 1

        if self.reason in ("duration reached", "stop file"):
            self.snapshot("end")
        return 0

    def finish(self) -> None:
        coredumps = coredump_report()
        new = [line for line in coredumps["lines"] if line not in self.baseline["lines"]]
        self.note(f"coredumps listed for {COREDUMP_EXE}: {len(new)} new since the start")
        summary = {
            "reason": self.reason or "error",
            "wall_start": self.wall_start,
            "wall_end": now_stamp(),
            "interval_s": float(self.args.interval_s),
            "duration_s_requested": float(self.args.duration_min) * 60.0,
            "ticks": self.ticks,
            "last_elapsed_s": round(self.last_elapsed, 1),
            "clk_tck": os.sysconf("SC_CLK_TCK"),
            "watched": [
                {"series": w.series, "pid": w.pid, "comm": w.comm, "exe": w.exe, "starttime_ticks": w.starttime}
                for w in self.watches
            ],
            "snapshots": self.snapshots,
            "unreadable": {watch.series: sorted(watch.unreadable) for watch in self.watches},
            "coredumps": {
                "before": self.baseline["lines"],
                "error_before": self.baseline["error"],
                "after": coredumps["lines"],
                "error_after": coredumps["error"],
                "new": new,
            },
        }
        (self.out / "summary.json").write_text(json.dumps(summary, indent=1, ensure_ascii=False) + "\n", encoding="utf-8")
        if self.args.finish_touch:
            Path(self.args.finish_touch).touch()
        (self.out / "done").write_text(summary["reason"] + "\n", encoding="utf-8")

    def main(self) -> int:
        try:
            status = self.run()
        except BaseException as error:
            self.reason = self.reason or f"error: {error}"
            self.note(self.reason)
            self.finish()
            raise
        self.finish()
        return status


def parse_args(argv: list) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Sample process memory into CSV and smaps snapshots (read-only).")
    parser.add_argument("--out", required=True, help="output directory (created)")
    parser.add_argument("--watch", action="append", default=[], metavar="LABEL=PID", help="watch a fixed pid")
    parser.add_argument("--watch-file", action="append", default=[], metavar="LABEL=PIDFILE", help="watch the pid written to a file")
    parser.add_argument("--interval-s", type=float, default=30.0)
    parser.add_argument("--duration-min", type=float, default=150.0)
    parser.add_argument("--snapshot-min", default="0,15,75", help="comma-separated minutes; the end is always taken")
    parser.add_argument("--stop-file", help="sampling ends when this file exists")
    parser.add_argument("--finish-touch", help="create this file when sampling ends")
    parser.add_argument("--attach-timeout-s", type=float, default=120.0)
    return parser.parse_args(argv)


def main(argv: list | None = None) -> int:
    return Sampler(parse_args(sys.argv[1:] if argv is None else argv)).main()


if __name__ == "__main__":
    sys.exit(main())
