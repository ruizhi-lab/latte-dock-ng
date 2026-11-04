#!/usr/bin/env python3
"""Check the parsers and safety guards of scripts/test-logout-simulation.py.

The simulation itself needs kwin_wayland, a private session bus and a dock build, so
ctest cannot run it. These cases cover what it decides from and what keeps it safe:
the window listing that its KWin script prints, KWin's "Not closed windows" report,
the dock window filter, the guards that keep its fake ksmserver off the desktop bus,
and the refusal to run without a dock binary. Nothing here needs D-Bus, KWin or the
Python dbus module.
"""

import importlib.util
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.dont_write_bytecode = True

SCRIPT = Path(__file__).resolve().parents[1] / "scripts/test-logout-simulation.py"


def load_script():
    spec = importlib.util.spec_from_file_location("logout_simulation", SCRIPT)
    module = importlib.util.module_from_spec(spec)
    # dataclasses look the defining module up in sys.modules.
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


simulation = load_script()

# Lines as kwin_wayland prints them: print() from a KWin script comes out as "js: ...",
# and the close report is a kwin_core debug message quoting each application.
KWIN_LOG = r"""kwin_core: Successfully loaded scripted effect:  "fade"
js: LATTESIM list1 end
js: LATTESIM list2 class=org.qt-project.qml dock=false pid=101 caption="LatteSim blocker"
js: LATTESIM list2 class=org.kde.latte-dock dock=true pid=202 caption="#view#1 — Latte Dock NG"
js: LATTESIM list2 class=org.kde.latte-dock dock=true pid=202 caption="#subwindow#1 — Latte Dock NG"
js: LATTESIM list2 end
js: LATTESIM list3 class=org.kde.latte-dock dock=true pid=202 caption="#view#1 — Latte Dock NG"
js: LATTESIM list4 class=org.kde.kate dock=false pid=303 caption="say \"hi\" \\ again"
js: LATTESIM list4 end
kwin_core: Not closed windows QList("• LatteSim blocker", "• Latte 停靠栏")
kwin_core: Not closed windows QList("• LatteSim blocker")
kwin_core: Not closed windows QList("• An \"editor\" window")
"""


def run_script(*args, env=None):
    return subprocess.run([sys.executable, "-I", str(SCRIPT), *args], capture_output=True, text=True, timeout=30, env=env)


class WindowListingTest(unittest.TestCase):
    def test_finished_listing_returns_every_window(self):
        windows = simulation.parse_windows(KWIN_LOG, "list2")
        self.assertEqual(
            windows,
            [
                simulation.Window("org.qt-project.qml", False, 101, "LatteSim blocker"),
                simulation.Window("org.kde.latte-dock", True, 202, "#view#1 — Latte Dock NG"),
                simulation.Window("org.kde.latte-dock", True, 202, "#subwindow#1 — Latte Dock NG"),
            ],
        )

    def test_empty_listing_is_a_result(self):
        self.assertEqual(simulation.parse_windows(KWIN_LOG, "list1"), [])

    def test_listing_without_end_marker_is_not_finished(self):
        self.assertIsNone(simulation.parse_windows(KWIN_LOG, "list3"))
        self.assertIsNone(simulation.parse_windows(KWIN_LOG, "list9"))

    def test_caption_is_json_decoded(self):
        self.assertEqual(simulation.parse_windows(KWIN_LOG, "list4")[0].caption, 'say "hi" \\ again')

    def test_dock_windows_are_recognized_by_class(self):
        self.assertEqual([window.is_latte for window in simulation.parse_windows(KWIN_LOG, "list2")], [False, True, True])


class NotClosedReportTest(unittest.TestCase):
    def test_every_report_is_returned_in_order_without_bullets(self):
        self.assertEqual(
            simulation.parse_not_closed(KWIN_LOG),
            [["LatteSim blocker", "Latte 停靠栏"], ["LatteSim blocker"], ['An "editor" window']],
        )

    def test_log_without_report(self):
        self.assertEqual(simulation.parse_not_closed("kwin_core: nothing to see\n"), [])


class SimulationHomeTest(unittest.TestCase):
    def test_missing_kdeglobals_is_seeded(self):
        with tempfile.TemporaryDirectory() as home:
            simulation.seed_kdeglobals(Path(home))
            self.assertIn("ColorScheme=BreezeLight", Path(home, ".config/kdeglobals").read_text(encoding="utf-8"))

    def test_existing_kdeglobals_is_kept(self):
        with tempfile.TemporaryDirectory() as home:
            kdeglobals = Path(home, ".config/kdeglobals")
            kdeglobals.parent.mkdir(parents=True)
            kdeglobals.write_text("[General]\nColorScheme=BreezeDark\n", encoding="utf-8")
            simulation.seed_kdeglobals(Path(home))
            self.assertEqual(kdeglobals.read_text(encoding="utf-8"), "[General]\nColorScheme=BreezeDark\n")

    def test_scenario_environment_is_isolated(self):
        env = simulation.scenario_env(Path("/sim/home"), Path("/sim/tmp"), Path("/sim/run"), "run-1")
        self.assertEqual(env["HOME"], "/sim/home")
        self.assertEqual(env["XDG_CONFIG_HOME"], "/sim/home/.config")
        self.assertEqual(env["XDG_DATA_HOME"], "/sim/home/.local/share")
        self.assertEqual(env["XDG_RUNTIME_DIR"], "/sim/run")
        self.assertEqual(env["TMPDIR"], "/sim/tmp")
        self.assertEqual(env["WAYLAND_DISPLAY"], simulation.WAYLAND_SOCKET)
        self.assertEqual(env[simulation.RUN_MARKER], "run-1")
        self.assertEqual(env["QT_FORCE_STDERR_LOGGING"], "1")
        self.assertNotIn("DISPLAY", env)


class SafetyGuardTest(unittest.TestCase):
    def guard_env(self, **overrides):
        env = dict(os.environ)
        for name in (simulation.PRIVATE_BUS_MARKER, simulation.PARENT_BUS_VARIABLE, simulation.RUN_MARKER):
            env.pop(name, None)
        env.update(overrides)
        return env

    def test_fake_session_refuses_a_bus_without_the_marker(self):
        result = run_script("--fake-session", env=self.guard_env(DBUS_SESSION_BUS_ADDRESS="unix:path=/nonexistent/bus"))
        self.assertEqual(result.returncode, 2, result.stderr)
        self.assertIn("refusing", result.stderr)

    def test_fake_session_refuses_the_parent_bus(self):
        env = self.guard_env(
            DBUS_SESSION_BUS_ADDRESS="unix:path=/nonexistent/bus",
            **{simulation.PRIVATE_BUS_MARKER: "1", simulation.PARENT_BUS_VARIABLE: "unix:path=/nonexistent/bus"},
        )
        result = run_script("--fake-session", env=env)
        self.assertEqual(result.returncode, 2, result.stderr)
        self.assertIn("refusing", result.stderr)

    def test_scenario_refuses_a_bus_without_the_marker(self):
        with tempfile.TemporaryDirectory() as work:
            out = Path(work, "out")
            result = run_script(
                "--run-scenario", "sigterm", "--home", work, "--latte", "/bin/true", "--out", str(out),
                env=self.guard_env(DBUS_SESSION_BUS_ADDRESS="unix:path=/nonexistent/bus"),
            )
            self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
            self.assertIn("refusing", (out / "result.json").read_text(encoding="utf-8"))

    def test_orchestrator_needs_a_dock_binary(self):
        with tempfile.TemporaryDirectory() as work:
            result = run_script("--work-dir", work, "--scenario", "sigterm", env=self.guard_env())
            self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
            self.assertIn("no dock binary", result.stderr)
            self.assertFalse(Path(work, "runs").exists())

    def test_help_lists_the_scenarios(self):
        result = run_script("--help")
        self.assertEqual(result.returncode, 0, result.stderr)
        for scenario in simulation.SCENARIOS:
            self.assertIn(scenario, result.stdout)


if __name__ == "__main__":
    unittest.main()
