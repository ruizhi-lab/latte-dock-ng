#!/usr/bin/env python3
"""Simulate a KDE logout around the dock in a nested, headless KWin.

KWin's org.kde.KWin.Session.closeWaylandWindows() waits for every xdg_toplevel to
close but never sends close to a panel-role (dock) window. A dock that keeps such a
window mapped after a logout is confirmed holds the whole logout until KWin's "The
following applications did not close" notification. Session::ShutdownMonitor hides
the dock and suspends its helper windows when ksmserver confirms a logout, restores
them when the logout is cancelled and quits when plasma-shutdown leaves while
ksmserver still reports the logout. This tool checks that behavior end to end
without logging out of the desktop.

Every scenario runs on its own private session bus (dbus-run-session) with its own
`kwin_wayland --virtual`, a fake ksmserver and plasma-shutdown (this script in
--fake-session mode) and the dock. The bus and every process on it get the simulation
HOME and XDG tree, a private TMPDIR (the dock's instance lock lives in
QDir::tempPath()) and a private XDG_RUNTIME_DIR, so nothing can reach the desktop's
Wayland display or the user's data. The bus has no service directories: D-Bus
activation would start kactivitymanagerd, the portals or ksecretd with whatever
environment the bus has, and those processes outlive the bus. Each scenario's
processes carry a marker in their environment, and the orchestrator stops any that is
left when the scenario ends. The dock is never started with --replace, and the fake
services refuse to start on any bus but a scenario's private one.

Scenarios:
  confirm-cancel-complete  a confirmed logout hides every dock window and KWin's close
                           step does not wait for the dock; a cancel restores the
                           windows; a second confirmation followed by plasma-shutdown
                           leaving quits the dock cleanly.
  blocker                  an application window that refuses to close keeps KWin
                           waiting; KWin's report of the windows that did not close
                           must name only that window, and a cancel restores the dock.
  sigterm                  SIGTERM quits the dock cleanly.

The dock under test should come from a user-mode install into the simulation HOME so
that its QML and plugins match the binary: --install builds this checkout with
`install.sh --user Debug` into that HOME first. A system binary can be tested too, for
example an older release for an A/B comparison, with --latte and an empty --home.

Requirements: kwin_wayland, dbus-run-session, python3 with the dbus and gi modules,
and qml6 (or qml) for the blocker scenario.

Exit status: 0 when every check passed, 1 when a check failed, 2 when the simulation
could not be set up.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import signal
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SCENARIOS = ("confirm-cancel-complete", "blocker", "sigterm")

CONTROL_SERVICE = "org.latte.LogoutSimulation"
CONTROL_PATH = "/Control"
CONTROL_INTERFACE = "org.latte.LogoutSimulation"
# Set by the orchestrator for the processes it starts under dbus-run-session; the
# parent bus address lets them prove that they are not on the desktop bus.
PRIVATE_BUS_MARKER = "LATTE_LOGOUT_SIMULATION_PRIVATE_BUS"
PARENT_BUS_VARIABLE = "LATTE_LOGOUT_SIMULATION_PARENT_BUS"
# Unique per scenario run; every process started for the scenario inherits it.
RUN_MARKER = "LATTE_LOGOUT_SIMULATION_RUN"

SESSION_BUS_CONFIG = """<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-Bus Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<!-- Private bus of one logout simulation scenario. It has no service directories, so
     nothing is activated: every process on it is one that the simulation started. -->
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

WAYLAND_SOCKET = "wayland-lattesim"
BLOCKER_TITLE = "LatteSim blocker"
TEARDOWN_MARKERS = ("Latte Corona - deleted...", "QuickWindowSystem destructed")
COMPLETION_MARKER = "[shutdown] logout completed; quitting."
SIGTERM_MARKER = "KSignalHandler received signal 15"

# KWin starts its "applications did not close" report 10 s after closeWaylandWindows().
KWIN_CLOSE_REPORT_S = 10

BLOCKER_QML = """import QtQuick
import QtQuick.Window

// An application window that refuses KWin's close request, like an editor that
// asks to save, so KWin's closeWaylandWindows() keeps waiting for it.
Window {
    visible: true
    width: 320
    height: 200
    title: "%s"
    onClosing: (close) => { close.accepted = false }
}
""" % BLOCKER_TITLE

# The caption goes last because it may contain spaces; JSON.stringify quotes it.
LIST_SCRIPT = """const windows = workspace.windowList();
for (let i = 0; i < windows.length; ++i) {
    const w = windows[i];
    print("LATTESIM %(tag)s class=" + w.resourceClass + " dock=" + w.dock + " pid=" + w.pid
          + " caption=" + JSON.stringify(w.caption));
}
print("LATTESIM %(tag)s end");
"""

WINDOW_LINE = re.compile(
    r"LATTESIM (?P<tag>\S+) class=(?P<cls>\S*) dock=(?P<dock>true|false) pid=(?P<pid>-?\d+) caption=(?P<caption>\".*\")\s*$"
)
NOT_CLOSED_LINE = re.compile(r"Not closed windows QList\((?P<items>.*)\)\s*$")
QUOTED_ITEM = re.compile(r'"((?:[^"\\]|\\.)*)"')


class SetupError(Exception):
    """The simulation could not be set up; no check result is meaningful."""


@dataclass(frozen=True)
class Window:
    resource_class: str
    dock: bool
    pid: int
    caption: str

    @property
    def is_latte(self) -> bool:
        return "latte" in self.resource_class.lower()


def parse_windows(log: str, tag: str) -> list[Window] | None:
    """Return the windows that the listing script printed under tag.

    None means that KWin has not printed the listing's end marker yet.
    """
    windows: list[Window] = []
    finished = False

    for line in log.splitlines():
        if line.rstrip().endswith(f"LATTESIM {tag} end"):
            finished = True
            continue

        match = WINDOW_LINE.search(line)
        if match and match["tag"] == tag:
            windows.append(Window(match["cls"], match["dock"] == "true", int(match["pid"]), json.loads(match["caption"])))

    return windows if finished else None


def parse_not_closed(log: str) -> list[list[str]]:
    """Return every list of applications that KWin reported as not closed."""
    reports: list[list[str]] = []

    for line in log.splitlines():
        match = NOT_CLOSED_LINE.search(line)
        if not match:
            continue

        names = []
        for item in QUOTED_ITEM.findall(match["items"]):
            name = item.replace('\\"', '"').replace("\\\\", "\\")
            names.append(name.removeprefix("•").strip())
        reports.append(names)

    return reports


def qml_runtime() -> str | None:
    """The Qt 6 QML runtime; a bare `qml` comes last because it may belong to Qt 5."""
    for name in ("qml6", "qml-qt6"):
        if path := shutil.which(name):
            return path

    for name in ("qtpaths6", "qtpaths-qt6"):
        if tool := shutil.which(name):
            bins = subprocess.run([tool, "--query", "QT_INSTALL_BINS"], capture_output=True, text=True).stdout.strip()
            candidate = Path(bins) / "qml"
            if bins and candidate.is_file() and os.access(candidate, os.X_OK):
                return str(candidate)

    return shutil.which("qml")


def default_work_dir() -> Path:
    cache = os.environ.get("XDG_CACHE_HOME") or str(Path.home() / ".cache")
    return Path(cache) / "latte-logout-simulation"


def simulation_env(home: Path) -> dict[str, str]:
    """Environment of every simulated process: the simulation HOME and its XDG tree."""
    env = dict(os.environ)
    env.update(
        HOME=str(home),
        XDG_CONFIG_HOME=str(home / ".config"),
        XDG_CACHE_HOME=str(home / ".cache"),
        XDG_DATA_HOME=str(home / ".local/share"),
        XDG_STATE_HOME=str(home / ".local/state"),
    )
    env.pop("DISPLAY", None)
    return env


def scenario_env(home: Path, tmp_dir: Path, runtime_dir: Path, run_id: str) -> dict[str, str]:
    """Environment of a scenario's bus and of every process started on it.

    The private XDG_RUNTIME_DIR alone keeps clients off the desktop's Wayland socket;
    WAYLAND_DISPLAY names the nested KWin's socket inside it.
    """
    env = simulation_env(home)
    env.update(
        TMPDIR=str(tmp_dir),
        XDG_RUNTIME_DIR=str(runtime_dir),
        WAYLAND_DISPLAY=WAYLAND_SOCKET,
        # Qt built with journald support (Fedora) logs to the journal unless stderr
        # is a terminal; the checks read KWin's and the dock's output from files.
        QT_FORCE_STDERR_LOGGING="1",
        **{
            PRIVATE_BUS_MARKER: "1",
            PARENT_BUS_VARIABLE: os.environ.get("DBUS_SESSION_BUS_ADDRESS", ""),
            RUN_MARKER: run_id,
        },
    )
    env.pop("QT_QPA_PLATFORM", None)
    return env


def stop_leftovers(run_id: str) -> int:
    """Stop every process that still carries the scenario's run marker; return how many."""
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

    for sig in (signal.SIGTERM, signal.SIGKILL):
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


def require_private_bus() -> None:
    """Refuse to claim org.kde.ksmserver or org.kde.Shutdown on the desktop bus."""
    address = os.environ.get("DBUS_SESSION_BUS_ADDRESS", "")
    parent = os.environ.get(PARENT_BUS_VARIABLE, "")
    if os.environ.get(PRIVATE_BUS_MARKER) != "1" or not address or address == parent:
        raise SetupError("not on the private bus of a scenario; refusing to claim ksmserver or plasma-shutdown names")


# --- fake ksmserver and plasma-shutdown -----------------------------------------------


def run_fake_session() -> int:
    require_private_bus()

    import dbus
    import dbus.service
    from dbus.mainloop.glib import DBusGMainLoop
    from gi.repository import GLib

    class KsmServer(dbus.service.Object):
        def __init__(self, bus):
            super().__init__(bus, "/KSMServer")
            self.shutting_down = False

        @dbus.service.method("org.kde.KSMServerInterface", in_signature="", out_signature="b")
        def isShuttingDown(self):
            return self.shutting_down

    class Control(dbus.service.Object):
        def __init__(self, bus, ksmserver):
            super().__init__(bus, CONTROL_PATH)
            self.bus = bus
            self.ksmserver = ksmserver
            self.shutdown_name = None

        @dbus.service.method(CONTROL_INTERFACE, in_signature="b", out_signature="")
        def SetShuttingDown(self, value):
            self.ksmserver.shutting_down = bool(value)
            print(f"isShuttingDown() = {self.ksmserver.shutting_down}", flush=True)

        @dbus.service.method(CONTROL_INTERFACE, in_signature="", out_signature="")
        def OwnShutdown(self):
            if self.shutdown_name is None:
                self.shutdown_name = dbus.service.BusName("org.kde.Shutdown", self.bus)
                print("org.kde.Shutdown owned", flush=True)

        @dbus.service.method(CONTROL_INTERFACE, in_signature="", out_signature="")
        def ReleaseShutdown(self):
            if self.shutdown_name is not None:
                # Dropping the last reference releases the name, as plasma-shutdown exiting does.
                self.shutdown_name = None
                print("org.kde.Shutdown released", flush=True)

    DBusGMainLoop(set_as_default=True)
    bus = dbus.SessionBus()
    names = [dbus.service.BusName("org.kde.ksmserver", bus), dbus.service.BusName(CONTROL_SERVICE, bus)]
    control = Control(bus, KsmServer(bus))
    print("fake session ready", flush=True)
    GLib.MainLoop().run()
    del names, control
    return 0


# --- one scenario on a private bus ------------------------------------------------------


class Simulation:
    def __init__(self, scenario: str, home: Path, latte: Path, out: Path):
        import dbus

        self.dbus = dbus
        self.scenario = scenario
        self.home = home
        self.latte_binary = latte
        self.out = out
        self.out.mkdir(parents=True, exist_ok=True)
        # The orchestrator created both for this scenario and passed them to the bus.
        self.runtime_dir = Path(os.environ["XDG_RUNTIME_DIR"])
        self.tmp_dir = Path(os.environ["TMPDIR"])
        self.kwin_log = out / "kwin.log"
        self.latte_log = out / "latte.log"
        self.processes: dict[str, subprocess.Popen] = {}
        self.log_files = []
        self.checks: list[dict] = []
        self.list_count = 0
        self.bus = None

    # Environment and processes

    def env(self, client: bool) -> dict[str, str]:
        # The scenario's environment is already isolated; see scenario_env().
        env = dict(os.environ)

        if client:
            env.update(WAYLAND_DISPLAY=WAYLAND_SOCKET, QT_QPA_PLATFORM="wayland")
        else:
            env.pop("WAYLAND_DISPLAY", None)
            env.pop("QT_QPA_PLATFORM", None)

        return env

    def spawn(self, name: str, command: list[str], env: dict[str, str], log: Path) -> subprocess.Popen:
        handle = log.open("ab")
        self.log_files.append(handle)
        process = subprocess.Popen(command, env=env, stdout=handle, stderr=subprocess.STDOUT, start_new_session=True)
        self.processes[name] = process
        return process

    def stop_all(self) -> None:
        for process in self.processes.values():
            if process.poll() is None:
                process.terminate()

        deadline = time.monotonic() + 5
        for process in self.processes.values():
            try:
                process.wait(timeout=max(0.1, deadline - time.monotonic()))
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()

        for handle in self.log_files:
            handle.close()

    def wait_for(self, predicate, timeout: float, interval: float = 0.2):
        deadline = time.monotonic() + timeout
        while True:
            value = predicate()
            if value or time.monotonic() >= deadline:
                return value
            time.sleep(interval)

    def start_kwin(self) -> None:
        env = self.env(client=False)
        # "Not closed windows" is a kwin_core debug message and print() in a KWin script
        # is a "js" debug message. Fedora's qtlogging.ini turns every debug category off,
        # and QT_LOGGING_RULES takes precedence over it.
        env["QT_LOGGING_RULES"] = "kwin_core.debug=true;kwin_scripting.debug=true;js.debug=true"
        self.spawn(
            "kwin",
            ["kwin_wayland", "--virtual", "--width", "1920", "--height", "1080", "--socket", WAYLAND_SOCKET, "--no-lockscreen", "--no-global-shortcuts"],
            env,
            self.kwin_log,
        )
        self.bus = self.dbus.SessionBus()
        socket = self.runtime_dir / WAYLAND_SOCKET

        if not self.wait_for(lambda: socket.exists() and self.bus.name_has_owner("org.kde.KWin"), 30):
            raise SetupError(f"kwin_wayland did not start; see {self.kwin_log}")

    def start_fake_session(self) -> None:
        self.spawn("fake-session", [sys.executable, "-I", str(Path(__file__).resolve()), "--fake-session"], self.env(client=False), self.out / "fake-session.log")

        if not self.wait_for(lambda: self.bus.name_has_owner(CONTROL_SERVICE) and self.bus.name_has_owner("org.kde.ksmserver"), 15):
            raise SetupError(f"the fake ksmserver did not start; see {self.out / 'fake-session.log'}")

    def start_latte(self) -> None:
        env = self.env(client=True)
        env["QT_LOGGING_RULES"] = "latte*=true"
        env_script = self.home / ".config/latte-dock-ng/dev-env.sh"
        # dev-env.sh points a user-mode install at its own QML modules.
        command = ["bash", "-c", 'if [ -f "$1" ]; then source "$1"; fi; exec "$2" --debug', "latte-dock-simulation", str(env_script), str(self.latte_binary)]
        self.spawn("latte", command, env, self.latte_log)

    def start_blocker(self) -> None:
        tool = qml_runtime()
        if tool is None:
            raise SetupError("the blocker scenario needs qml6 (Qt 6 QML runtime)")

        qml = self.out / "blocker.qml"
        qml.write_text(BLOCKER_QML, encoding="utf-8")
        self.spawn("blocker", [tool, str(qml)], self.env(client=True), self.out / "blocker.log")

    def latte_running(self) -> bool:
        return self.processes["latte"].poll() is None

    def wait_latte_exit(self, timeout: float) -> tuple[int | None, float]:
        start = time.monotonic()
        try:
            status = self.processes["latte"].wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            status = None
        return status, time.monotonic() - start

    # D-Bus

    def interface(self, service: str, path: str, name: str):
        return self.dbus.Interface(self.bus.get_object(service, path, introspect=False), name)

    def control(self, method: str, *args) -> None:
        getattr(self.interface(CONTROL_SERVICE, CONTROL_PATH, CONTROL_INTERFACE), method)(*args)

    def confirm(self) -> None:
        """plasma-shutdown starts and ksmserver reports the confirmed logout."""
        self.control("OwnShutdown")
        self.control("SetShuttingDown", True)
        print("  logout confirmed", flush=True)

    def cancel(self) -> None:
        """ksmserver resets the logout, then plasma-shutdown exits."""
        self.control("SetShuttingDown", False)
        self.control("ReleaseShutdown")
        print("  logout cancelled", flush=True)

    def complete(self) -> None:
        """plasma-shutdown exits while ksmserver still reports the logout."""
        self.control("ReleaseShutdown")
        print("  logout completed", flush=True)

    def windows(self) -> list[Window]:
        self.list_count += 1
        tag = f"list{self.list_count}"
        script = self.out / f"{tag}.js"
        script.write_text(LIST_SCRIPT % {"tag": tag}, encoding="utf-8")
        scripting = self.interface("org.kde.KWin", "/Scripting", "org.kde.kwin.Scripting")
        plugin = f"lattesim_{tag}"
        script_id = int(scripting.loadScript(str(script), plugin))

        if script_id < 0:
            raise SetupError("KWin refused the window listing script")

        self.interface("org.kde.KWin", f"/Scripting/Script{script_id}", "org.kde.kwin.Script").run()
        # An empty listing is a valid result; only a missing end marker is an error.
        deadline = time.monotonic() + 5
        listed = parse_windows(self.kwin_log.read_text(encoding="utf-8", errors="replace"), tag)
        while listed is None and time.monotonic() < deadline:
            time.sleep(0.1)
            listed = parse_windows(self.kwin_log.read_text(encoding="utf-8", errors="replace"), tag)
        scripting.unloadScript(plugin)

        if listed is None:
            raise SetupError("KWin did not run the window listing script")

        return listed

    def dock_windows(self) -> list[Window]:
        return [window for window in self.windows() if window.is_latte]

    def wait_dock_windows(self, wanted, timeout: float) -> list[Window]:
        """Poll the dock's windows until wanted(windows) holds or the timeout passes."""
        deadline = time.monotonic() + timeout
        while True:
            windows = self.dock_windows()
            if wanted(windows) or time.monotonic() >= deadline:
                return windows
            time.sleep(0.3)

    def close_windows(self, timeout: float) -> tuple[str, float]:
        session = self.interface("org.kde.KWin", "/Session", "org.kde.KWin.Session")
        start = time.monotonic()
        try:
            outcome = "true" if bool(session.closeWaylandWindows(timeout=timeout)) else "false"
        except self.dbus.exceptions.DBusException as error:
            name = error.get_dbus_name() or "error"
            outcome = "no reply" if name.endswith("NoReply") else name
        return outcome, time.monotonic() - start

    # Checks

    def check(self, name: str, ok: bool, detail: str = "") -> None:
        self.checks.append({"name": name, "ok": bool(ok), "detail": detail})
        print(f"  {'PASS' if ok else 'FAIL'}  {name}" + (f": {detail}" if detail else ""), flush=True)

    def latte_log_text(self) -> str:
        return self.latte_log.read_text(encoding="utf-8", errors="replace")

    def start_dock(self) -> list[Window]:
        self.start_latte()
        # A view's helper windows can map before or after the view itself, so wait for a
        # view and then for two listings in a row with the same windows.
        windows = self.wait_dock_windows(lambda listed: any(window.caption.startswith("#view#") for window in listed), 90)
        if not any(window.caption.startswith("#view#") for window in windows):
            raise SetupError(f"the dock mapped no view within 90 s; see {self.latte_log}")

        for _ in range(10):
            time.sleep(1)
            settled = self.dock_windows()
            if sorted(settled, key=str) == sorted(windows, key=str):
                break
            windows = settled

        print("  dock windows: " + ", ".join(window.caption for window in windows), flush=True)
        return windows

    def check_teardown(self, status: int | None, seconds: float, marker: str, quit_name: str) -> None:
        log = self.latte_log_text()
        self.check(quit_name, status == 0, f"exit status {status} after {seconds:.2f} s")
        self.check("the dock logged why it quit", marker in log, marker)
        missing = [item for item in TEARDOWN_MARKERS if item not in log]
        self.check("the teardown ran to the end", not missing, "missing: " + ", ".join(missing) if missing else "")

    # Scenarios

    def run_confirm_cancel_complete(self) -> None:
        startup = self.start_dock()
        self.confirm()
        hidden = self.wait_dock_windows(lambda windows: not windows, 5)
        self.check("a confirmed logout hides every dock window", not hidden, ", ".join(window.caption for window in hidden))
        self.check("the dock keeps running after the confirmation", self.latte_running())

        outcome, seconds = self.close_windows(timeout=KWIN_CLOSE_REPORT_S + 5)
        self.check("KWin's close step does not wait for the dock", outcome == "true" and seconds < 5, f"closeWaylandWindows -> {outcome} after {seconds:.2f} s")

        self.cancel()
        restored = self.wait_dock_windows(lambda windows: len(windows) == len(startup), 10)
        self.check("a cancel restores every dock window", len(restored) == len(startup), f"{len(restored)} of {len(startup)}")
        self.check("the dock keeps running after the cancel", self.latte_running())

        self.confirm()
        self.wait_dock_windows(lambda windows: not windows, 5)
        self.complete()
        status, seconds = self.wait_latte_exit(15)
        self.check_teardown(status, seconds, COMPLETION_MARKER, "a completed logout quits the dock")

    def run_blocker(self) -> None:
        self.start_blocker()
        startup = self.start_dock()
        blocker = self.wait_for(lambda: [window for window in self.windows() if window.caption == BLOCKER_TITLE], 30)
        if not blocker:
            raise SetupError(f"the blocker window did not map; see {self.out / 'blocker.log'}")

        self.confirm()
        hidden = self.wait_dock_windows(lambda windows: not windows, 5)
        self.check("a confirmed logout hides every dock window", not hidden, ", ".join(window.caption for window in hidden))

        outcome, seconds = self.close_windows(timeout=KWIN_CLOSE_REPORT_S + 4)
        print(f"  closeWaylandWindows -> {outcome} after {seconds:.2f} s", flush=True)
        reports = parse_not_closed(self.kwin_log.read_text(encoding="utf-8", errors="replace"))
        reported = reports[-1] if reports else None
        self.check("KWin reports the blocker as not closed", bool(reported) and BLOCKER_TITLE in reported, f"not closed: {reported}")
        others = [name for name in (reported or []) if name != BLOCKER_TITLE]
        self.check("KWin does not wait for the dock", reported is not None and not others, f"also not closed: {others}" if others else "")
        self.check("the dock keeps running while KWin waits", self.latte_running())

        self.cancel()
        restored = self.wait_dock_windows(lambda windows: len(windows) == len(startup), 10)
        self.check("a cancel restores every dock window", len(restored) == len(startup), f"{len(restored)} of {len(startup)}")
        self.check("the dock keeps running after the cancel", self.latte_running())

    def run_sigterm(self) -> None:
        self.start_dock()
        self.processes["latte"].send_signal(signal.SIGTERM)
        status, seconds = self.wait_latte_exit(15)
        self.check_teardown(status, seconds, SIGTERM_MARKER, "SIGTERM quits the dock")

    def run(self) -> None:
        self.start_kwin()
        self.start_fake_session()
        getattr(self, "run_" + self.scenario.replace("-", "_"))()


def run_scenario(args: argparse.Namespace) -> int:
    out: Path = args.out
    out.mkdir(parents=True, exist_ok=True)
    result: dict = {"scenario": args.run_scenario, "latte": str(args.latte), "checks": [], "error": None}
    simulation = None

    try:
        require_private_bus()
        simulation = Simulation(args.run_scenario, args.home, args.latte, out)
        simulation.run()
    except SetupError as error:
        result["error"] = str(error)
        print(f"  ERROR {error}", flush=True)
    finally:
        if simulation is not None:
            result["checks"] = simulation.checks
            simulation.stop_all()
        (out / "result.json").write_text(json.dumps(result, indent=2, ensure_ascii=False), encoding="utf-8")

    if result["error"]:
        return 2

    return 0 if all(check["ok"] for check in result["checks"]) else 1


# --- orchestration ----------------------------------------------------------------------


def seed_kdeglobals(home: Path) -> None:
    """Give the simulation HOME the kdeglobals that every Plasma session has.

    With the default Plasma theme, which ships no colors file, the dock takes its
    color scheme from kdeglobals. Without the file it creates no scheme, and
    PlasmaExtended::Theme then crashes on quit (SIGSEGV in ~Theme, seen with Plasma
    6.8.2 on Fedora 45); that is a dock bug, not part of the logout under test. An
    existing file is left alone.
    """
    kdeglobals = home / ".config/kdeglobals"
    if not kdeglobals.exists():
        kdeglobals.parent.mkdir(parents=True, exist_ok=True)
        kdeglobals.write_text("[General]\nColorScheme=BreezeLight\n", encoding="utf-8")


def install_user_mode(home: Path, work: Path, jobs: int) -> None:
    env = simulation_env(home)
    command = ["bash", str(ROOT / "install.sh"), "--user", "--jobs", str(jobs), "--build-dir", str(work / "build-user"), "Debug"]
    print(f"Installing this checkout into {home} (install.sh --user Debug)", flush=True)
    log = work / "install.log"
    with log.open("wb") as handle:
        status = subprocess.run(command, env=env, cwd=ROOT, stdout=handle, stderr=subprocess.STDOUT).returncode
    if status != 0:
        raise SetupError(f"install.sh failed with status {status}; see {log}")


def missing_tools(scenarios: list[str]) -> list[str]:
    missing = [tool for tool in ("kwin_wayland", "dbus-run-session", "bash") if not shutil.which(tool)]

    for module in ("dbus", "gi"):
        probe = subprocess.run([sys.executable, "-I", "-c", f"import {module}"], capture_output=True)
        if probe.returncode != 0:
            missing.append(f"python3 module {module}")

    if "blocker" in scenarios and qml_runtime() is None:
        missing.append("qml6 (for the blocker scenario)")

    return missing


def version_of(command: list[str], env: dict[str, str] | None = None) -> str:
    try:
        output = subprocess.run(command, capture_output=True, text=True, timeout=20, env=env).stdout.strip()
    except (OSError, subprocess.TimeoutExpired):
        return "unknown"
    return output.splitlines()[-1] if output else "unknown"


def parse_args(argv: list[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0], formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--scenario", action="append", choices=SCENARIOS, help="scenario to run; repeat for several (default: all)")
    parser.add_argument("--home", type=Path, help="simulation HOME (default: <work dir>/home)")
    parser.add_argument("--latte", type=Path, help="dock binary (default: <home>/.local/bin/latte-dock-ng)")
    parser.add_argument("--install", action="store_true", help="first install this checkout into the simulation HOME with install.sh --user Debug")
    parser.add_argument("--jobs", type=int, default=8, help="build jobs for --install (default: 8)")
    parser.add_argument("--work-dir", type=Path, default=default_work_dir(), help="build tree, logs and default HOME (default: %(default)s)")
    parser.add_argument("--out", type=Path, help="log directory (default: <work dir>/runs/<time>)")
    parser.add_argument("--timeout", type=int, default=240, help="seconds allowed for each scenario (default: %(default)s)")
    # Internal modes, started by the orchestrator on a scenario's private bus.
    parser.add_argument("--fake-session", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--run-scenario", choices=SCENARIOS, help=argparse.SUPPRESS)
    return parser.parse_args(argv)


def orchestrate(args: argparse.Namespace) -> int:
    work = args.work_dir.expanduser().resolve()
    home = (args.home or work / "home").expanduser().resolve()
    scenarios = args.scenario or list(SCENARIOS)
    home.mkdir(parents=True, exist_ok=True)

    latte = (args.latte or home / ".local/bin/latte-dock-ng").expanduser().resolve()

    def require_dock() -> None:
        if not (latte.is_file() and os.access(latte, os.X_OK)):
            raise SetupError(f"no dock binary at {latte}; pass --latte or run with --install")

    if not args.install:
        require_dock()

    missing = missing_tools(scenarios)
    if missing:
        raise SetupError("missing: " + ", ".join(missing))

    if args.install:
        install_user_mode(home, work, args.jobs)
        require_dock()

    seed_kdeglobals(home)
    out_root = (args.out or work / "runs" / time.strftime("%Y%m%d-%H%M%S")).expanduser().resolve()
    # --version still creates the QApplication: keep it off the desktop's display and HOME.
    version_env = simulation_env(home)
    version_env.pop("WAYLAND_DISPLAY", None)
    version_env["QT_QPA_PLATFORM"] = "offscreen"
    print(f"Dock: {latte} ({version_of([str(latte), '--version'], version_env)})", flush=True)
    print(f"KWin: {version_of(['kwin_wayland', '--version'])}", flush=True)
    print(f"Simulation HOME: {home}\nLogs: {out_root}\n", flush=True)

    summary = []

    for scenario in scenarios:
        print(f"[{scenario}]", flush=True)
        out = out_root / scenario
        (out / "tmp").mkdir(parents=True, exist_ok=True)
        # Wayland socket paths are limited to 108 bytes, so the runtime directory gets a
        # short path of its own instead of one inside the output directory.
        runtime_dir = Path(tempfile.mkdtemp(prefix="lattesim-rt-"))
        runtime_dir.chmod(0o700)
        config = out / "session-bus.conf"
        config.write_text(SESSION_BUS_CONFIG % {"runtime_dir": runtime_dir}, encoding="utf-8")
        run_id = f"{os.getpid()}-{scenario}-{time.time_ns()}"
        command = ["dbus-run-session", f"--config-file={config}", "--", sys.executable, "-I", str(Path(__file__).resolve()),
                   "--run-scenario", scenario, "--home", str(home), "--latte", str(latte), "--out", str(out)]
        try:
            status = subprocess.run(command, env=scenario_env(home, out / "tmp", runtime_dir, run_id), timeout=args.timeout).returncode
        except subprocess.TimeoutExpired:
            status = 2
            print(f"  ERROR the scenario did not finish within {args.timeout} s", flush=True)
        finally:
            leftovers = stop_leftovers(run_id)
            shutil.rmtree(runtime_dir, ignore_errors=True)

        if leftovers:
            print(f"  stopped {leftovers} process(es) left from the scenario", flush=True)
        summary.append((scenario, status))

    print("\nSummary:", flush=True)
    for scenario, status in summary:
        print(f"  {scenario}: {({0: 'passed', 1: 'FAILED'}).get(status, 'NOT RUN (setup error)')}", flush=True)

    statuses = {status for _, status in summary}
    if statuses == {0}:
        return 0
    return 1 if 1 in statuses else 2


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)

    try:
        if args.fake_session:
            return run_fake_session()
        if args.run_scenario:
            if not (args.home and args.latte and args.out):
                raise SetupError("--run-scenario needs --home, --latte and --out")
            return run_scenario(args)
        return orchestrate(args)
    except SetupError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
