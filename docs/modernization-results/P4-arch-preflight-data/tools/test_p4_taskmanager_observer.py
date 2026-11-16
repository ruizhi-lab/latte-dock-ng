#!/usr/bin/env python3
"""Focused checks for the P4 observer's fail-closed event gate."""

from __future__ import annotations

import subprocess
import sys
import time
import unittest

from run_p4_taskmanager_observer import consume_process, validate_sequence


class ObserverRunnerTest(unittest.TestCase):
    def test_silent_child_is_killed_by_monotonic_deadline(self):
        child = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(10)"],
                                 stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        return_code, timed_out = consume_process(child, time.monotonic() + 0.2, lambda _: None)
        self.assertTrue(timed_out)
        self.assertNotEqual(return_code, 0)

    def test_only_complete_same_window_sequence_is_accepted(self):
        context = {"event": "readiness", "activityReady": True,
                   "desktopReady": True, "screenReady": True}
        window = {"appId": "org.kde.dolphin", "title": "P4 unique title",
                  "windowId": "91", "isWindow": True, "isStartup": False}
        events = [context, {"event": "baseline-row", **window, "isActive": False,
                            "isMinimized": False},
                  {"event": "controller-activate", "windowId": "91"},
                  {"event": "observed-active", **window, "isActive": True,
                   "isMinimized": False},
                  {"event": "controller-minimize", "windowId": "91"},
                  {"event": "observed-minimized", **window, "isActive": False,
                   "isMinimized": True},
                  {"event": "controller-restore", "windowId": "91"},
                  {"event": "controller-reactivate", "windowId": "91"},
                  {"event": "observed-restored-active", **window, "isActive": True,
                   "isMinimized": False},
                  {"event": "result", "verdict": "valid-sequence", "windowId": "91"}]
        self.assertEqual(validate_sequence(events, "org.kde.dolphin", "P4 unique title"),
                         (True, "valid-sequence"))
        self.assertFalse(validate_sequence(events, "org.kde.dolphin", "other title")[0])
        events[5] = {**events[5], "windowId": "92"}
        self.assertFalse(validate_sequence(events, "org.kde.dolphin", "P4 unique title")[0])


if __name__ == "__main__":
    unittest.main()
