#!/usr/bin/env python3
"""R7j runner: the activity menu harness on a nested KWin Wayland session, with real pointer input.

Scratch tooling for the R7j investigation (docs/modernization-results/R7j.md). Structure follows
docs/modernization-results/R8d-tools/launch.py: a private session bus from dbus-run-session
(a configuration without service directories), a scratch HOME and XDG tree, and
`kwin_wayland --virtual` on its own socket. It touches neither the user's desktop session, its
D-Bus session bus, its HOME, nor a desktop dock.

Build the two programs first: harness/build.py (the harness) and wl/build_fakeinput.sh (the
pointer client). Then:

    python3 -I run_nested.py --root <new empty run dir> --harness <harness binary> \\
        --fakeinput <fakeinput binary> --scenario none

Scenarios (after the first click on the Work item): none, ok, cancel, outside, work-again.
Set R7J_ASSIGNED=all in the environment to start with the "All activities" item assigned.

KWIN_WAYLAND_NO_PERMISSION_CHECKS=1 is set for the nested KWin only. KWin hides
org_kde_kwin_fake_input from clients whose executable is not listed in a desktop file
(src/wayland_server.cpp, interfacesBlackList); this is KWin's own test switch.
"""

from __future__ import annotations

import argparse
import os
import queue
import re
import shutil
import subprocess
import sys
import threading
import time
from pathlib import Path

SOCKET = "r7j-wl"
SESSION_BUS_CONFIG = """<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-Bus Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<!-- Private bus of the R7j scratch runs. It has no service directories, so nothing is activated. -->
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


def say(message: str) -> None:
    print(f"{time.strftime('%H:%M:%S')} {message}", flush=True)


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
        XDG_RUNTIME_DIR=str(root / "runtime"),
    )
    for name in ("DISPLAY", "WAYLAND_DISPLAY", "QT_QPA_PLATFORM", "SESSION_MANAGER", "DBUS_SESSION_BUS_ADDRESS"):
        env.pop(name, None)
    return env


class Reader:
    """Reads a child's stdout line by line into a queue and into a log file."""

    def __init__(self, stream, log_path: Path) -> None:
        self.lines: "queue.Queue[str]" = queue.Queue()
        self.history: list[str] = []
        self._log = log_path.open("a", encoding="utf-8", buffering=1)
        self._thread = threading.Thread(target=self._run, args=(stream,), daemon=True)
        self._thread.start()

    def _run(self, stream) -> None:
        for raw in iter(stream.readline, ""):
            line = raw.rstrip("\n")
            self.history.append(line)
            self.lines.put(line)
            self._log.write(line + "\n")
        self._log.close()

    def wait_for(self, pattern: str, timeout: float) -> str:
        regex = re.compile(pattern)
        deadline = time.monotonic() + timeout
        for line in self.history:
            if regex.search(line):
                return line
        while time.monotonic() < deadline:
            try:
                line = self.lines.get(timeout=0.2)
            except queue.Empty:
                continue
            if regex.search(line):
                return line
        raise TimeoutError(f"no line matching {pattern!r} within {timeout} s")


def send(process: subprocess.Popen, command: str) -> None:
    process.stdin.write(command + "\n")
    process.stdin.flush()


def coordinates(line: str) -> tuple[int, int]:
    parts = line.split()
    return int(parts[-2]), int(parts[-1])


def run_child(root: Path, harness: Path, fakeinput: Path, scenario: str) -> int:
    session = root / "session"
    shots = root / "shots"
    shots.mkdir(exist_ok=True)
    env = scratch_env(root)
    # KWin hides org_kde_kwin_fake_input from clients whose executable is not listed in a desktop
    # file with X-KDE-Wayland-Interfaces (src/wayland_server.cpp, interfacesBlackList). This
    # switch is KWin's own test setting; it only changes this nested KWin, which has a scratch HOME
    # and a private bus, and nothing on the user's desktop.
    env["KWIN_WAYLAND_NO_PERMISSION_CHECKS"] = "1"
    kwin_log = (session / "kwin.log").open("ab")
    kwin = subprocess.Popen(
        [
            "kwin_wayland",
            "--virtual",
            "--width",
            "1920",
            "--height",
            "1080",
            "--socket",
            SOCKET,
            "--no-lockscreen",
            "--no-global-shortcuts",
        ],
        env=env,
        stdout=kwin_log,
        stderr=subprocess.STDOUT,
        start_new_session=True,
    )
    (root / "kwin.pid").write_text(f"{kwin.pid}\n", encoding="utf-8")
    socket_path = Path(env["XDG_RUNTIME_DIR"]) / SOCKET
    deadline = time.monotonic() + 90
    while not socket_path.exists():
        if kwin.poll() is not None or time.monotonic() > deadline:
            say(f"kwin did not create {socket_path} (exit {kwin.poll()})")
            return 2
        time.sleep(0.2)
    time.sleep(3)
    say(f"nested KWin pid {kwin.pid} on {SOCKET}")

    client_env = dict(env)
    client_env.update(WAYLAND_DISPLAY=SOCKET, QT_QPA_PLATFORM="wayland", QT_FORCE_STDERR_LOGGING="1", R7J_OUT=str(shots))
    harness_proc = subprocess.Popen(
        [str(harness)],
        env=client_env,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
        start_new_session=True,
    )
    harness_out = Reader(harness_proc.stdout, session / "harness.log")
    driver_proc = subprocess.Popen(
        [str(fakeinput)],
        env=client_env,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
        start_new_session=True,
    )
    driver_out = Reader(driver_proc.stdout, session / "fakeinput.log")
    result = 0
    try:
        driver_out.wait_for(r"^FI authenticate sent|^FI org_kde|is not offered|cannot connect", 30)
        if not any("authenticate sent" in line for line in driver_out.history):
            say("fake input unavailable: " + " | ".join(driver_out.history[-3:]))
            return 3

        button = coordinates(harness_out.wait_for(r"R7J-BUTTON ", 60))
        say(f"editor button at {button}")
        time.sleep(1.0)

        def click(point: tuple[int, int], label: str) -> None:
            say(f"{label}: move to {point}")
            send(driver_proc, f"move {point[0]} {point[1]}")
            driver_out.wait_for(rf"^FI move {point[0]} {point[1]} ok", 10)
            time.sleep(0.4)
            send(driver_proc, "press")
            driver_out.wait_for(r"^FI press ok", 10)
            time.sleep(0.12)
            send(driver_proc, "release")
            driver_out.wait_for(r"^FI release ok", 10)

        click(button, "press the editor button")
        work_line = harness_out.wait_for(r"R7J-WORK ", 20)
        work = coordinates(work_line)
        ok_point = coordinates(harness_out.wait_for(r"R7J-OK ", 5))
        cancel_point = coordinates(harness_out.wait_for(r"R7J-CANCEL ", 5))
        say(f"Work item at {work}, OK at {ok_point}, Cancel at {cancel_point}")
        time.sleep(0.8)

        click(work, "click Work")
        time.sleep(1.5)
        send(harness_proc, "state")
        time.sleep(0.6)

        if scenario == "ok":
            click(ok_point, "click OK")
        elif scenario == "cancel":
            click(cancel_point, "click Cancel")
        elif scenario == "outside":
            click((1500, 900), "click outside the menu")
        elif scenario == "work-again":
            click(work, "click Work again")
        if scenario != "none":
            time.sleep(1.5)
            send(harness_proc, "state")
            time.sleep(0.6)
        say("scenario done")
    except (TimeoutError, BrokenPipeError, OSError) as error:
        say(f"scenario error: {error}")
        result = 4
    finally:
        try:
            send(harness_proc, "quit")
            harness_proc.wait(timeout=15)
        except (subprocess.TimeoutExpired, BrokenPipeError, OSError):
            harness_proc.kill()
        try:
            send(driver_proc, "quit")
            driver_proc.wait(timeout=10)
        except (subprocess.TimeoutExpired, BrokenPipeError, OSError):
            driver_proc.kill()
        kwin.terminate()
        try:
            kwin.wait(timeout=20)
        except subprocess.TimeoutExpired:
            kwin.kill()
            kwin.wait()
        kwin_log.close()
        say(f"harness exit {harness_proc.returncode}, kwin exit {kwin.returncode}")
    return result


def run_outer(args: argparse.Namespace) -> int:
    root = Path(args.root).resolve()
    session = root / "session"
    session.mkdir(parents=True, exist_ok=True)
    (root / "runtime").mkdir(exist_ok=True)
    (root / "runtime").chmod(0o700)
    (root / "tmp").mkdir(exist_ok=True)
    (root / "home").mkdir(exist_ok=True)
    config = session / "session-bus.conf"
    config.write_text(SESSION_BUS_CONFIG % {"runtime_dir": root / "runtime"}, encoding="utf-8")
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
        "--harness",
        str(Path(args.harness).resolve()),
        "--fakeinput",
        str(Path(args.fakeinput).resolve()),
        "--scenario",
        args.scenario,
    ]
    say(f"run root {root}, scenario {args.scenario}")
    status = subprocess.run(command, env=dict(os.environ, R7J_RUN=str(root))).returncode
    say(f"session ended with status {status}")
    return status


def main(argv: list | None = None) -> int:
    parser = argparse.ArgumentParser(description="Run the activity menu harness on a nested KWin Wayland session.")
    parser.add_argument("--root", required=True, help="run directory (created; must not exist yet)")
    parser.add_argument("--harness", required=True, help="harness binary from harness/build.py")
    parser.add_argument("--fakeinput", required=True, help="pointer client from wl/build_fakeinput.sh")
    parser.add_argument("--scenario", default="none", choices=["none", "ok", "cancel", "outside", "work-again"])
    parser.add_argument("--child", action="store_true", help=argparse.SUPPRESS)
    args = parser.parse_args(sys.argv[1:] if argv is None else argv)
    if args.child:
        return run_child(Path(args.root), Path(args.harness), Path(args.fakeinput), args.scenario)
    return run_outer(args)


if __name__ == "__main__":
    sys.exit(main())
