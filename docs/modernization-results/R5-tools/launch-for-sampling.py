#!/usr/bin/env python3
"""R5 helper: start the dock the way r5-scenario.py would and leave it running (VM side).

usage: launch-for-sampling.py systemd|p0-ssh [NAME=VALUE ...] [--dock-arg=ARG ...]
Prints the dock's pid. The launcher and environment handling are the runner's own functions, so a process
sampled with gdb or watched by another tool runs in exactly the environment of the measured scenarios.
"""

import importlib.util
import os
import sys

spec = importlib.util.spec_from_file_location("r5", os.path.expanduser("~/latte-r5/tools/r5-scenario.py"))
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)
launcher = sys.argv[1]
dock_args = [a.removeprefix("--dock-arg=") for a in sys.argv[2:] if a.startswith("--dock-arg=")]
env = [a for a in sys.argv[2:] if not a.startswith("--dock-arg=")]
if not runner.stop_dock():
    sys.exit("dock did not stop")
pid = runner.start_dock(env, dock_args, launcher)
print(pid)
