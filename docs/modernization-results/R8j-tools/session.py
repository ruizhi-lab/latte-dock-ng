#!/usr/bin/env python3
"""One nested-session run of one R8j scenario: idle control (I), window (W) or application (A).

The design, the cycles and the verdict rule are in docs/modernization-results/R8j.md.
The session is R8d's (`R8d-tools/launch.py`): a private session bus, a scratch HOME,
a private XDG_RUNTIME_DIR and a headless `kwin_wayland --virtual`. This file adds the
cycles, the allocator probe (`r8j_trim.c`, preloaded into the dock only) and the sampler
loop. The dock is a copy of the scratch HOME's user-mode install; the desktop dock is
never addressed (signals go to the Popen pid of the nested dock, checked against its
start time).

Usage (the formal runs use the defaults; the pilot shortens the timeline):

    python3 -I session.py --scenario W --out <run dir> --template <scratch HOME> \\
        --probe <r8j_trim.so> [--warmup-s 720] [--cycles 10] [--post-cycles 2] \\
        [--hold-s 20] [--rest-s 20] [--sample-s 5] [--shot-cycle K] [--shot-warm]

The run directory gets: samples.csv (every --sample-s seconds, the R8d column set),
events.jsonl (one JSON object per event; a signal's `index` is the probe's `seq`),
summary.json, and session/ (kwin.log, dock.log, probe.log, the application logs, the
window QML files, versions.txt, session-bus.conf, exit.json).

Exit status: 0 when the timeline completed, 2 when it stopped on an error.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
import shutil
import signal
import subprocess
import sys
import tempfile
import threading
import time
from datetime import datetime
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS_DIR.parent / "R8d-tools"))
import launch  # noqa: E402  (R8d's session helpers: scratch_env, spawn, wait_for_socket, ...)
import memsample  # noqa: E402  (R8d's read-only sampler: Watch, CSV_COLUMNS, coredump_report)

RUN_MARKER = "R8J_RUN_MARKER"
SCENARIOS = ("I", "W", "A")
# The application of scenario A. Its window and icon are what a user's application start shows.
APP_COMMAND = ["kcalc"]
TRIM_DELAY_S = 25.0  # the post-trim sample window is 15 to 25 s after the trim
SAMPLE_WINDOW_S = 15.0  # a level is the mean of the samples in the last 15 s of a window
SIGNALS = {"SIGUSR1": signal.SIGUSR1, "SIGUSR2": signal.SIGUSR2}
# Scenario I: this many seconds with nothing started, before the rest.
IDLE_HOLD_S = 20.0


def now_text() -> str:
    return datetime.now().isoformat(timespec="seconds")


def sha256_of(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


class Clock:
    """Seconds since the dock started (monotonic)."""

    def __init__(self) -> None:
        self.t0 = time.monotonic()

    def elapsed(self) -> float:
        return time.monotonic() - self.t0

    def wait(self, seconds: float, stop: threading.Event | None = None) -> None:
        deadline = time.monotonic() + seconds
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                return
            if stop is not None and stop.is_set():
                return
            time.sleep(min(remaining, 0.5))


class Journal:
    """Appends one JSON object per event, with the elapsed time and the wall clock."""

    def __init__(self, path: Path, clock: Clock) -> None:
        self.path = path
        self.clock = clock
        self.lock = threading.Lock()

    def write(self, event: str, **fields) -> dict:
        record = {"elapsed_s": round(self.clock.elapsed(), 2), "wall": now_text(), "event": event, **fields}
        with self.lock:
            with self.path.open("a", encoding="utf-8") as handle:
                handle.write(json.dumps(record, ensure_ascii=False) + "\n")
        return record


class Sampler(threading.Thread):
    """Samples the nested dock every interval, on the clock's schedule, until stopped."""

    def __init__(self, watch, csv_path: Path, clock: Clock, interval: float, journal: Journal) -> None:
        super().__init__(name="r8j-sampler", daemon=True)
        self.watch = watch
        self.csv_path = csv_path
        self.clock = clock
        self.interval = interval
        self.journal = journal
        self.stop_event = threading.Event()
        self.rows = 0
        self.gone = None

    def run(self) -> None:
        with self.csv_path.open("w", newline="", encoding="utf-8") as handle:
            writer = csv.DictWriter(handle, fieldnames=memsample.CSV_COLUMNS, lineterminator="\n")
            writer.writeheader()
            handle.flush()
            tick = 0
            while not self.stop_event.is_set():
                delay = tick * self.interval - self.clock.elapsed()
                if delay > 0 and self.stop_event.wait(delay):
                    break
                try:
                    row = self.watch.sample(self.clock.elapsed(), memsample.now_stamp())
                except memsample.ProcessGone as error:
                    self.gone = str(error)
                    self.journal.write("sampler-stopped", detail=self.gone)
                    break
                writer.writerow(row)
                handle.flush()
                self.rows += 1
                tick += 1

    def stop(self) -> None:
        self.stop_event.set()
        if self.is_alive():
            self.join(timeout=30)


def stop_process(process: subprocess.Popen, timeout_s: float = 10.0) -> int | None:
    """SIGTERM to the process group (the application was started in its own session), then SIGKILL."""
    if process.poll() is None:
        try:
            os.killpg(process.pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
        try:
            process.wait(timeout=timeout_s)
        except subprocess.TimeoutExpired:
            try:
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            process.wait()
    return process.returncode


def start_group(command: list, env: dict, log: Path) -> subprocess.Popen:
    """Start an application in its own session, so that its whole process group can be stopped."""
    with log.open("ab") as handle:
        # The child keeps its own copy of the descriptor, so closing ours is safe.
        return subprocess.Popen(command, env=env, stdout=handle, stderr=subprocess.STDOUT, start_new_session=True)


def count_lines(path: Path) -> int:
    if not path.exists():
        return 0
    with path.open(encoding="utf-8", errors="replace") as handle:
        return sum(1 for _ in handle)


class Timeline:
    """The cycles of one scenario, in the order fixed by R8j.md."""

    def __init__(self, args: argparse.Namespace, clock: Clock, journal: Journal, dock: subprocess.Popen,
                 watch, env: dict, session: Path) -> None:
        self.args = args
        self.clock = clock
        self.journal = journal
        self.dock = dock
        self.watch = watch
        self.env = env
        self.session = session
        self.index = 0  # the number of signals sent; the probe's seq follows it
        self.live: list = []  # application or window processes still running

    def check_dock(self) -> None:
        # Raises memsample.ProcessGone when the nested dock has exited or its pid was reused.
        self.watch.check_alive()

    def send(self, name: str, label: str) -> None:
        self.check_dock()
        os.kill(self.dock.pid, SIGNALS[name])
        self.index += 1
        self.journal.write("signal", name=name, index=self.index, label=label)

    def window_command(self, cycle: int) -> tuple:
        qml = self.session / f"window-{cycle}.qml"
        qml.write_text(launch.WINDOW_QML % f"R8j window {cycle}", encoding="utf-8")
        qml_bin = shutil.which("qml6")
        if qml_bin is None:
            raise RuntimeError("qml6 not found")
        return [qml_bin, str(qml)], "window"

    def app_command(self, cycle: int) -> tuple:
        binary = shutil.which(APP_COMMAND[0])
        if binary is None:
            raise RuntimeError(f"{APP_COMMAND[0]} not found")
        return [binary], "application"

    def start_action(self, scenario: str, cycle: int):
        if scenario == "I":
            return None
        command, kind = self.window_command(cycle) if scenario == "W" else self.app_command(cycle)
        log = self.session / f"{kind}-{cycle}.log"
        process = start_group(command, self.env, log)
        self.live.append(process)
        self.journal.write("app-start", cycle=cycle, kind=kind, pid=process.pid, command=command[0])
        return process

    def stop_action(self, process, cycle: int) -> None:
        if process is None:
            return
        code = stop_process(process)
        if process in self.live:
            self.live.remove(process)
        self.journal.write("app-stop", cycle=cycle, pid=process.pid, status=code)

    def screenshot(self, name: str) -> None:
        path = self.session / f"{name}.png"
        try:
            result = subprocess.run(
                ["spectacle", "-b", "-n", "-f", "-o", str(path)],
                env=self.env,
                capture_output=True,
                text=True,
                timeout=60,
            )
            status = result.returncode
            detail = (result.stderr or result.stdout).strip()[:200]
        except (OSError, subprocess.TimeoutExpired) as error:
            status, detail = -1, str(error)
        self.journal.write("screenshot", name=name, status=status, detail=detail)

    def cycle(self, scenario: str, cycle: int) -> None:
        self.journal.write("cycle-start", cycle=cycle)
        process = self.start_action(scenario, cycle)
        hold = IDLE_HOLD_S if scenario == "I" else self.args.hold_s
        if self.args.shot_cycle == cycle:
            self.clock.wait(hold / 2)
            self.screenshot(f"shot-cycle-{cycle}")
            self.clock.wait(hold / 2)
        else:
            self.clock.wait(hold)
        self.stop_action(process, cycle)
        self.clock.wait(self.args.rest_s)
        self.journal.write("rest-end", cycle=cycle)
        self.send("SIGUSR2", f"dump after cycle {cycle}")

    def run(self, scenario: str) -> None:
        args = self.args
        self.clock.wait(args.warmup_s)
        self.journal.write("warmup-end")
        if args.shot_warm:
            self.screenshot("shot-warmup-end")
        self.send("SIGUSR2", "dump at warm-up end (before the trim)")
        self.send("SIGUSR1", "trim at warm-up end: baseline")
        self.clock.wait(TRIM_DELAY_S)
        self.journal.write("baseline-ready")
        for cycle in range(1, args.cycles + 1):
            self.cycle(scenario, cycle)
        # The trim after cycle 10 is part of the design whatever the number of post-trim cycles.
        self.send("SIGUSR1", "trim after the cycles")
        self.clock.wait(TRIM_DELAY_S)
        self.journal.write("post-trim-ready")
        for cycle in range(args.cycles + 1, args.cycles + args.post_cycles + 1):
            self.cycle(scenario, cycle)
        self.journal.write("timeline-end")

    def stop_all(self) -> None:
        for process in list(self.live):
            self.stop_action(process, cycle=0)


def child(args: argparse.Namespace) -> int:
    out = Path(args.out)
    session = out / "session"
    runtime = Path(os.environ["XDG_RUNTIME_DIR"])
    dock_path = out / "home" / ".local" / "bin" / "latte-dock-ng"
    procs: dict = {}
    timeline = None
    sampler = None
    clock = Clock()
    journal = Journal(out / "events.jsonl", clock)
    summary: dict = {"scenario": args.scenario, "reason": None, "dock_status": None}
    status = 0
    try:
        kwin_env = dict(os.environ)
        kwin_env.pop("WAYLAND_DISPLAY", None)
        kwin_env.pop("QT_QPA_PLATFORM", None)
        procs["kwin"] = launch.spawn(
            [
                "kwin_wayland",
                "--virtual",
                "--width",
                "1920",
                "--height",
                "1080",
                "--socket",
                launch.WAYLAND_SOCKET,
                "--no-lockscreen",
                "--no-global-shortcuts",
            ],
            kwin_env,
            session / "kwin.log",
        )
        launch.wait_for_socket(runtime / launch.WAYLAND_SOCKET, procs["kwin"], 90)
        time.sleep(3)  # KWin still finishes its outputs and services after the socket appears

        client_env = dict(os.environ)
        client_env.update(WAYLAND_DISPLAY=launch.WAYLAND_SOCKET, QT_QPA_PLATFORM="wayland")
        dock_env = dict(client_env)
        dock_env.update(
            LD_PRELOAD=str(Path(args.probe).resolve()),
            R8J_LOG=str(session / "probe.log"),
            QT_FORCE_STDERR_LOGGING="1",
        )
        coredumps_before = memsample.coredump_report()
        clock = Clock()  # the warm-up counts from here: the dock's start
        journal = Journal(out / "events.jsonl", clock)
        # No --debug and no --replace in the formal runs: the same normal start as R8d's nested dock.
        # Without --debug the dock discards its debug messages (app/main.cpp), so a pilot that
        # needs them passes --dock-arg --debug.
        procs["dock"] = launch.spawn([str(dock_path), *args.dock_arg], dock_env, session / "dock.log")
        (out / "dock.pid").write_text(f"{procs['dock'].pid}\n", encoding="utf-8")
        watch = memsample.Watch("nested-dock", procs["dock"].pid)
        journal.write("dock-start", pid=procs["dock"].pid, comm=watch.comm, exe=watch.exe)
        sampler = Sampler(watch, out / "samples.csv", clock, args.sample_s, journal)
        sampler.start()
        # Applications and windows get the client environment, not the probe's variables.
        timeline = Timeline(args, clock, journal, procs["dock"], watch, client_env, session)
        timeline.run(args.scenario)
        summary["reason"] = "completed"
    except Exception as error:  # the session is torn down below either way
        summary["reason"] = f"error: {error}"
        status = 2
        journal.write("error", detail=str(error))
    finally:
        if sampler is not None:
            sampler.stop()  # the last sample is taken before the dock is signalled to quit
        if timeline is not None:
            timeline.stop_all()
        if "dock" in procs:
            dock = procs["dock"]
            if dock.poll() is None:
                dock.terminate()  # SIGTERM: the dock's normal quit path, as in R8d
            try:
                dock.wait(timeout=30)
            except subprocess.TimeoutExpired:
                dock.kill()
                dock.wait()
            summary["dock_status"] = dock.returncode
            journal.write("dock-exit", status=dock.returncode)
        if "kwin" in procs:
            kwin = procs["kwin"]
            if kwin.poll() is None:
                kwin.terminate()
            try:
                kwin.wait(timeout=20)
            except subprocess.TimeoutExpired:
                kwin.kill()
                kwin.wait()
            summary["kwin_status"] = kwin.returncode
        coredumps_after = memsample.coredump_report()
        new = [line for line in coredumps_after["lines"] if line not in coredumps_before["lines"]]
        summary.update(
            {
                "coredumps_new": new,
                "coredump_errors": [e for e in (coredumps_before["error"], coredumps_after["error"]) if e],
                "signals_sent": timeline.index if timeline is not None else 0,
                "samples": sampler.rows if sampler is not None else 0,
                "sampler_stopped": sampler.gone if sampler is not None else None,
                "dock_binary": {"path": str(dock_path), "sha256": sha256_of(dock_path)} if dock_path.exists() else None,
                "probe": {"path": str(Path(args.probe).resolve()), "sha256": sha256_of(Path(args.probe))},
                "probe_log_lines": count_lines(session / "probe.log"),
                "dock_log": dock_log_counts(session / "dock.log"),
                "options": {k: getattr(args, k) for k in ("warmup_s", "cycles", "post_cycles", "hold_s", "rest_s", "sample_s", "dock_arg")},
                "ended": now_text(),
            }
        )
        (out / "summary.json").write_text(json.dumps(summary, indent=1, ensure_ascii=False) + "\n", encoding="utf-8")
    return status


def dock_log_counts(path: Path) -> dict:
    counts = {"lines": 0, "warning": 0, "critical_or_error": 0, "fatal": 0, "assert": 0}
    if not path.exists():
        return counts
    with path.open(encoding="utf-8", errors="replace") as handle:
        for line in handle:
            counts["lines"] += 1
            lowered = line.lower()
            if "warning" in lowered:
                counts["warning"] += 1
            if "critical" in lowered or "error" in lowered:
                counts["critical_or_error"] += 1
            if "fatal" in lowered:
                counts["fatal"] += 1
            if "assert" in lowered:
                counts["assert"] += 1
    return counts


def localize_desktop_files(home: Path) -> None:
    """Point the copied desktop files at this run's binaries.

    KWin grants the dock its privileged Wayland interfaces (the window-management protocol
    that the task list needs) to the executable named by the first token of a desktop
    file's Exec, as install.sh's comment says. The template's files name the template's
    binary, so the copy's dock would not match them and would get no task list. The copy's
    desktop files are rewritten here, and the copied KSycoca cache, which records the
    template's paths, is removed by rebuild_ksycoca().
    """
    for path in (home / ".local" / "share" / "applications").glob("org.kde.latte-dock*.desktop"):
        lines = []
        for line in path.read_text(encoding="utf-8").splitlines(keepends=True):
            if line.startswith("Exec="):
                # The value keeps no line ending here: a newline would stop the name from matching.
                value = line[len("Exec="):].rstrip("\r\n")
                first, _, rest = value.partition(" ")
                name = Path(first).name
                if name in ("latte-dock-ng", "latte-dock-ng-preview"):
                    binary = home / ".local" / "bin" / name
                    line = f"Exec={binary}" + (f" {rest}" if rest else "") + "\n"
            lines.append(line)
        path.write_text("".join(lines), encoding="utf-8")


def rebuild_ksycoca(out: Path) -> None:
    """Rebuild the copied KSycoca database once, so that it names this run's desktop files."""
    cache = out / "home" / ".cache"
    for stale in cache.glob("ksycoca6_*"):
        stale.unlink()
    env = launch.scratch_env(out)
    env["QT_QPA_PLATFORM"] = "offscreen"
    try:
        result = subprocess.run(["kbuildsycoca6", "--noincremental"], env=env, capture_output=True, text=True, timeout=300)
        detail = f"status {result.returncode}"
    except (OSError, subprocess.TimeoutExpired) as error:
        detail = f"error: {error}"
    (out / "session" / "ksycoca.txt").write_text(f"kbuildsycoca6 --noincremental: {detail}\n", encoding="utf-8")


def write_versions(out: Path, dock: Path, probe: Path) -> None:
    session = out / "session"
    offscreen = launch.scratch_env(out)
    offscreen["QT_QPA_PLATFORM"] = "offscreen"
    lines = [
        f"kwin_wayland: {launch.command_text(['kwin_wayland', '--version'], launch.scratch_env(out))}",
        f"dock: {launch.command_text([str(dock), '--version'], offscreen)}",
        f"qml6: {shutil.which('qml6') or 'missing'}",
        f"application: {shutil.which(APP_COMMAND[0]) or 'missing'}",
        f"probe: {probe} sha256 {sha256_of(probe)}",
    ]
    lines += [f"linked: {entry}" for entry in launch.linked_libraries(dock, offscreen)]
    (session / "versions.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")


def outer(args: argparse.Namespace) -> int:
    out = Path(args.out).resolve()
    if out.exists() and any(out.iterdir()):
        raise SystemExit(f"run directory is not empty: {out}")
    (out / "session").mkdir(parents=True)
    template = Path(args.template).resolve()
    dock_template = template / ".local" / "bin" / "latte-dock-ng"
    if not (dock_template.is_file() and os.access(dock_template, os.X_OK)):
        raise SystemExit(f"no executable dock at {dock_template}: install this checkout into {template} first")
    shutil.copytree(template, out / "home", symlinks=True)  # the template itself is never run or changed
    (out / "tmp").mkdir()
    localize_desktop_files(out / "home")
    rebuild_ksycoca(out)
    runtime = Path(tempfile.mkdtemp(prefix="r8j-rt-", dir="/data/tmp"))
    runtime.chmod(0o700)
    config = out / "session" / "session-bus.conf"
    config.write_text(launch.SESSION_BUS_CONFIG % {"runtime_dir": runtime}, encoding="utf-8")
    run_id = f"{os.getpid()}-{time.time_ns()}"
    env = launch.scratch_env(out)
    env.update(XDG_RUNTIME_DIR=str(runtime), QT_FORCE_STDERR_LOGGING="1", **{RUN_MARKER: run_id})
    write_versions(out, out / "home" / ".local" / "bin" / "latte-dock-ng", Path(args.probe).resolve())
    command = [
        "dbus-run-session",
        f"--config-file={config}",
        "--",
        sys.executable,
        "-I",
        str(Path(__file__).resolve()),
        "--child",
        "--out",
        str(out),
        "--scenario",
        args.scenario,
        "--probe",
        str(Path(args.probe).resolve()),
        "--warmup-s",
        str(args.warmup_s),
        "--cycles",
        str(args.cycles),
        "--post-cycles",
        str(args.post_cycles),
        "--hold-s",
        str(args.hold_s),
        "--rest-s",
        str(args.rest_s),
        "--sample-s",
        str(args.sample_s),
    ]
    for dock_arg in args.dock_arg:
        # The = form: a value that starts with "--" would otherwise be read as an option.
        command += [f"--dock-arg={dock_arg}"]
    if args.shot_cycle is not None:
        command += ["--shot-cycle", str(args.shot_cycle)]
    if args.shot_warm:
        command += ["--shot-warm"]
    print(f"{now_text()} starting scenario {args.scenario} under {out}", flush=True)
    try:
        status = subprocess.run(command, env=env).returncode
    finally:
        leftovers = stop_leftovers(run_id)
        shutil.rmtree(runtime, ignore_errors=True)
    if leftovers:
        print(f"{now_text()} stopped {leftovers} process(es) left from the session", flush=True)
    print(f"{now_text()} scenario {args.scenario} ended with status {status}", flush=True)
    return status


def stop_leftovers(run_id: str) -> int:
    """Stop the processes that carry this run's marker in their environment (never the desktop's)."""
    marker = f"{RUN_MARKER}={run_id}".encode()
    pids = []
    for entry in Path("/proc").iterdir():
        if not entry.name.isdigit() or int(entry.name) == os.getpid():
            continue
        try:
            environ = (entry / "environ").read_bytes()
        except OSError:
            continue
        if marker in environ.split(b"\0"):
            pids.append(int(entry.name))
    for sig in (signal.SIGTERM, signal.SIGKILL):
        for pid in pids:
            try:
                os.kill(pid, sig)
            except ProcessLookupError:
                pass
        deadline = time.monotonic() + 3
        while time.monotonic() < deadline and any(_alive(pid) for pid in pids):
            time.sleep(0.1)
        if not any(_alive(pid) for pid in pids):
            break
    return len(pids)


def _alive(pid: int) -> bool:
    try:
        os.kill(pid, 0)
    except ProcessLookupError:
        return False
    except PermissionError:
        return True
    return True


def parse_args(argv: list) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run one R8j scenario in a nested KWin session.")
    parser.add_argument("--child", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--scenario", required=True, choices=SCENARIOS)
    parser.add_argument("--out", required=True, help="run directory (created; must not exist or be empty)")
    parser.add_argument("--template", help="a scratch HOME with the installed dock (copied for the run)")
    parser.add_argument("--probe", required=True, help="path of r8j_trim.so")
    parser.add_argument("--warmup-s", type=float, default=720.0)
    parser.add_argument("--cycles", type=int, default=10)
    parser.add_argument("--post-cycles", type=int, default=2)
    parser.add_argument("--hold-s", type=float, default=20.0)
    parser.add_argument("--rest-s", type=float, default=20.0)
    parser.add_argument("--sample-s", type=float, default=5.0)
    parser.add_argument("--dock-arg", action="append", default=[], help="pilot only: extra argument for the dock (for example --debug)")
    parser.add_argument("--shot-cycle", type=int, default=None, help="pilot only: screenshot in this cycle")
    parser.add_argument("--shot-warm", action="store_true", help="pilot only: screenshot at the warm-up end")
    args = parser.parse_args(argv)
    if not args.child and not args.template:
        parser.error("--template is required")
    return args


def main(argv: list | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    if args.child:
        return child(args)
    return outer(args)


if __name__ == "__main__":
    sys.exit(main())
