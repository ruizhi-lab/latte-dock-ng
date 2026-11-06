#!/usr/bin/env python3
"""Run the dock in a nested, headless KWin session for the R8d idle memory study.

The isolation is the one scripts/test-logout-simulation.py uses (its docstring and the
"Logout Simulation" section of docs/development-testing-guide.md describe it). It is
copied here rather than imported, so this study does not depend on that script's
internals:

- a private session bus from dbus-run-session, with a configuration that has no
  service directories, so nothing is D-Bus activated onto the desktop's bus;
- a scratch HOME and XDG tree under the run root, a private TMPDIR, and a short
  private XDG_RUNTIME_DIR under /tmp (Wayland socket paths are limited to 108 bytes);
- `kwin_wayland --virtual` on its own socket, with no DISPLAY and no WAYLAND_DISPLAY
  from the desktop;
- the dock from the scratch HOME's user-mode install of this checkout (~/.local/bin),
  started without --debug and without --replace. dev-env.sh is not sourced: install.sh
  calls it a development-only file and says the binary finds its own QML path;
- up to three qml6 windows with distinct titles, so that the task list has entries.

XDG_CONFIG_DIRS and XDG_DATA_DIRS are kept from the desktop session, as the simulation
keeps them. A dock started with a shorter XDG_CONFIG_DIRS rewrites KSycoca in a loop
(see the comment in install.sh that generates dev-env.sh); every nested process gets
the same value, so they all read one KSycoca database.

Start it detached, and start memsample.py detached once dock.pid exists:

    setsid -f nohup python3 -I launch.py --root /tmp/r8d-<time> > /tmp/r8d-<time>/launch.log 2>&1

Files under the root: dock.pid, kwin.pid and qml-<A|B|C>.pid when the processes start,
stop (its appearance tears the session down; memsample.py creates it when it finishes),
session/ (kwin.log, dock.log, qml-*.log, versions.txt, session-bus.conf, exit.json) and
home/ (the scratch HOME, which holds the user-mode install).
The session also ends after --max-min minutes (default 170). Teardown sends SIGTERM to
the dock first, so it runs its normal quit path, and SIGKILL only after 20 seconds.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from datetime import datetime
from pathlib import Path

WAYLAND_SOCKET = "wayland-r8d"
RUN_MARKER = "R8D_RUN_MARKER"
LIBRARY_PATTERN = re.compile(r"libQt6|libKF6|libPlasma|libLayerShellQt|libKWin")
WINDOW_TITLES = {"A": "R8d idle window A", "B": "R8d idle window B", "C": "R8d idle window C"}
WINDOW_QML = """import QtQuick
import QtQuick.Window

// An idle top-level window, so that the dock's task list has entries during the run.
Window {
    visible: true
    width: 240
    height: 120
    title: "%s"
}
"""
SESSION_BUS_CONFIG = """<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-Bus Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<!-- Private bus of the R8d idle study. It has no service directories, so nothing is activated. -->
<busconfig>
  <type>session</type>
  <keep_umask/>
  <listen>unix:tmpdir=%(runtime_dir)s</listen>
  <auth>EXTERNAL</auth>
  <policy context="default">
    <allow send_destination="*" eavesdrop="true"/>
    <allow eavesdrop="true"/>
    <allow own="*"/>
  </policy>
</busconfig>
"""


def now_text() -> str:
    return datetime.now().isoformat(timespec="seconds")


def say(message: str) -> None:
    print(f"{now_text()} {message}", flush=True)


def scratch_env(root: Path) -> dict:
    home = root / "home"
    env = dict(os.environ)
    env.update(
        HOME=str(home),
        XDG_CONFIG_HOME=str(home / ".config"),
        XDG_CACHE_HOME=str(home / ".cache"),
        XDG_DATA_HOME=str(home / ".local/share"),
        XDG_STATE_HOME=str(home / ".local/state"),
        TMPDIR=str(root / "tmp"),
        # Without this, libdbus falls back to the desktop's $XDG_RUNTIME_DIR/bus.
        XDG_RUNTIME_DIR=str(root / "runtime"),
    )
    # The desktop's display, bus and session manager must not reach the nested session.
    for name in ("DISPLAY", "WAYLAND_DISPLAY", "QT_QPA_PLATFORM", "DBUS_SESSION_BUS_ADDRESS", "SESSION_MANAGER"):
        env.pop(name, None)
    return env


def nested_env(root: Path, runtime: Path, run_id: str) -> dict:
    env = scratch_env(root)
    # dbus-run-session sets DBUS_SESSION_BUS_ADDRESS for its child and every process on it.
    env.update(XDG_RUNTIME_DIR=str(runtime), QT_FORCE_STDERR_LOGGING="1", **{RUN_MARKER: run_id})
    return env


def command_text(command: list, env: dict) -> str:
    try:
        result = subprocess.run(command, env=env, capture_output=True, text=True, timeout=90)
    except (OSError, subprocess.TimeoutExpired) as error:
        return f"unavailable ({error})"
    lines = (result.stdout or result.stderr).strip().splitlines()
    return lines[0] if lines else f"no output (status {result.returncode})"


def linked_libraries(binary: Path, env: dict) -> list:
    try:
        result = subprocess.run(["ldd", str(binary)], env=env, capture_output=True, text=True, timeout=60)
    except (OSError, subprocess.TimeoutExpired) as error:
        return [f"ldd unavailable ({error})"]
    found = []
    for line in result.stdout.splitlines():
        if "=>" not in line or not LIBRARY_PATTERN.search(line):
            continue
        name, _, rest = line.partition("=>")
        words = rest.split()
        if words and words[0].startswith("/"):
            found.append(f"{name.strip()} -> {os.path.realpath(words[0])}")
    return found


def preflight(root: Path, dock: Path) -> None:
    # --version creates a QApplication: keep it on the offscreen platform.
    offscreen = scratch_env(root)
    offscreen["QT_QPA_PLATFORM"] = "offscreen"
    lines = [
        f"kwin_wayland: {command_text(['kwin_wayland', '--version'], scratch_env(root))}",
        f"dock: {command_text([str(dock), '--version'], offscreen)}",
        f"qml6: {shutil.which('qml6') or 'missing'}",
    ]
    lines += [f"linked: {entry}" for entry in linked_libraries(dock, offscreen)]
    (root / "session" / "versions.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")
    for line in lines:
        say(line)


def stop_leftovers(run_id: str) -> int:
    """Stop every process that still carries this run's marker; return how many."""
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

    def alive(pid: int) -> bool:
        try:
            os.kill(pid, 0)
        except ProcessLookupError:
            return False
        return True

    for sig in (15, 9):  # SIGTERM, then SIGKILL
        for pid in pids:
            try:
                os.kill(pid, sig)
            except ProcessLookupError:
                pass
        deadline = time.monotonic() + 3
        while any(alive(pid) for pid in pids) and time.monotonic() < deadline:
            time.sleep(0.1)
        if not any(alive(pid) for pid in pids):
            break
    return len(pids)


def run_outer(args: argparse.Namespace) -> int:
    root = Path(args.root).resolve()
    session = root / "session"
    session.mkdir(parents=True, exist_ok=True)
    (root / "runtime").mkdir(exist_ok=True)
    (root / "runtime").chmod(0o700)
    dock = root / "home" / ".local" / "bin" / "latte-dock-ng"
    if not (dock.is_file() and os.access(dock, os.X_OK)):
        say(f"no executable dock at {dock}: install this checkout into {root / 'home'} with install.sh --user first")
        return 2
    # A stale stop file from an earlier run would tear the new session down at once.
    for name in ("stop", "dock.pid", "kwin.pid", "qml-A.pid", "qml-B.pid", "qml-C.pid"):
        (root / name).unlink(missing_ok=True)

    preflight(root, dock)
    runtime = Path(tempfile.mkdtemp(prefix="r8d-rt-", dir="/tmp"))
    runtime.chmod(0o700)
    config = session / "session-bus.conf"
    config.write_text(SESSION_BUS_CONFIG % {"runtime_dir": runtime}, encoding="utf-8")
    run_id = f"{os.getpid()}-{time.time_ns()}"
    command = [
        "dbus-run-session",
        f"--config-file={config}",
        "--",
        sys.executable,
        "-I",
        str(Path(__file__).resolve()),
        "--child",
        "--root",
        str(root),
        "--max-min",
        str(args.max_min),
    ]
    say(f"starting the nested session under {root}")
    try:
        status = subprocess.run(command, env=nested_env(root, runtime, run_id)).returncode
    finally:
        leftovers = stop_leftovers(run_id)
        shutil.rmtree(runtime, ignore_errors=True)
    if leftovers:
        say(f"stopped {leftovers} process(es) left from the session")
    say(f"session ended with status {status}")
    return status


def spawn(command: list, env: dict, log: Path) -> subprocess.Popen:
    with log.open("ab") as handle:
        # The child keeps its own copy of the descriptor, so closing ours is safe.
        return subprocess.Popen(command, env=env, stdout=handle, stderr=subprocess.STDOUT, start_new_session=True)


def write_pid(path: Path, process: subprocess.Popen) -> None:
    path.write_text(f"{process.pid}\n", encoding="utf-8")


def wait_for_socket(path: Path, process: subprocess.Popen, timeout_s: float) -> None:
    deadline = time.monotonic() + timeout_s
    while not path.exists():
        if process.poll() is not None:
            raise RuntimeError(f"kwin_wayland exited with status {process.returncode}; see session/kwin.log")
        if time.monotonic() > deadline:
            raise RuntimeError("kwin_wayland did not create its Wayland socket")
        time.sleep(0.2)


def monitor(procs: dict, stop_file: Path, started: float, max_s: float) -> str:
    while True:
        if stop_file.exists():
            return "stop file"
        for name in ("dock", "kwin"):
            code = procs[name].poll()
            if code is not None:
                return f"{name} exited with status {code}"
        if time.monotonic() - started >= max_s:
            return "max minutes reached"
        time.sleep(1)


def teardown(procs: dict) -> dict:
    # The dock goes first, while KWin still runs, so that it quits through its normal path.
    groups = [["dock"], [name for name in procs if name.startswith("qml-")], ["kwin"]]
    statuses = {}
    for group in groups:
        live = [name for name in group if name in procs]
        for name in live:
            if procs[name].poll() is None:
                procs[name].terminate()
        for name in live:
            try:
                procs[name].wait(timeout=20)
            except subprocess.TimeoutExpired:
                procs[name].kill()
                procs[name].wait()
            statuses[name] = procs[name].returncode
    return statuses


def child(root: Path, max_min: float) -> int:
    session = root / "session"
    runtime = Path(os.environ["XDG_RUNTIME_DIR"])
    dock = root / "home" / ".local" / "bin" / "latte-dock-ng"
    stop_file = root / "stop"
    procs: dict = {}
    result: dict = {"reason": None, "started": now_text()}
    status = 0
    try:
        kwin_env = dict(os.environ)
        kwin_env.pop("WAYLAND_DISPLAY", None)
        kwin_env.pop("QT_QPA_PLATFORM", None)
        procs["kwin"] = spawn(
            [
                "kwin_wayland",
                "--virtual",
                "--width",
                "1920",
                "--height",
                "1080",
                "--socket",
                WAYLAND_SOCKET,
                "--no-lockscreen",
                "--no-global-shortcuts",
            ],
            kwin_env,
            session / "kwin.log",
        )
        write_pid(root / "kwin.pid", procs["kwin"])
        wait_for_socket(runtime / WAYLAND_SOCKET, procs["kwin"], 90)
        # KWin still finishes its outputs and services after the socket appears.
        time.sleep(3)

        client_env = dict(os.environ)
        client_env.update(WAYLAND_DISPLAY=WAYLAND_SOCKET, QT_QPA_PLATFORM="wayland")
        qml = shutil.which("qml6")
        if qml is None:
            say("qml6 not found: the task list stays empty (limitation of this run)")
        else:
            for name, title in WINDOW_TITLES.items():
                qml_file = session / f"window-{name}.qml"
                qml_file.write_text(WINDOW_QML % title, encoding="utf-8")
                procs[f"qml-{name}"] = spawn([qml, str(qml_file)], client_env, session / f"qml-{name}.log")
                write_pid(root / f"qml-{name}.pid", procs[f"qml-{name}"])

        # No --debug and no --replace: a normal session start. The scratch HOME keeps the
        # desktop's instance lock and config out of reach.
        procs["dock"] = spawn([str(dock)], client_env, session / "dock.log")
        write_pid(root / "dock.pid", procs["dock"])
        say(f"dock pid {procs['dock'].pid}, nested KWin pid {procs['kwin'].pid}")
        result["reason"] = monitor(procs, stop_file, time.monotonic(), max_min * 60.0)
        say(f"tearing down: {result['reason']}")
    except Exception as error:  # the session is torn down below either way
        result["reason"] = f"error: {error}"
        status = 2
        say(result["reason"])
    finally:
        result["exit_status"] = teardown(procs)
        result["ended"] = now_text()
        (session / "exit.json").write_text(json.dumps(result, indent=1) + "\n", encoding="utf-8")
    return status


def main(argv: list | None = None) -> int:
    parser = argparse.ArgumentParser(description="Run the dock in a nested KWin session for an idle memory study.")
    parser.add_argument("--root", required=True, help="run directory (created)")
    parser.add_argument("--max-min", type=float, default=170.0, help="tear the session down after this many minutes")
    parser.add_argument("--child", action="store_true", help=argparse.SUPPRESS)
    args = parser.parse_args(sys.argv[1:] if argv is None else argv)
    if args.child:
        return child(Path(args.root), args.max_min)
    return run_outer(args)


if __name__ == "__main__":
    sys.exit(main())
