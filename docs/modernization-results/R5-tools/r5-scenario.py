#!/usr/bin/env python3
"""R5 scenario runner; runs inside the Fedora VM, never on the developer host.

One scenario is N independent dock starts. Each start stops any running dock,
launches the user-mode dock through `systemd-run --user` (so it has the Plasma
session environment), waits for the warmup and records one idle window with
two read-only collectors that run side by side over the same seconds:

* scripts/measure-process-family.py, P0's collector, for the headline CPU,
  context-switch and memory figures that are comparable with P0.md;
* r5-thread-sampler.py, for the per-thread breakdown of the dock, plasmashell
  and kwin_wayland and for the machine-wide totals.

The raw output lines of both collectors are appended unchanged to
<out>-family.jsonl and <out>-threads.jsonl, preceded by one "r5_start" metadata
line per start. The KWin probe runs before the window opens and reports the
pointer position and the window geometry by class (no window titles).

With --no-dock the scenario measures the rest of the desktop (Plasma's panel
alone) and samples plasmashell as the primary process.
"""

from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time

HOME = Path.home()
R5 = HOME / "latte-r5"
TOOLS = R5 / "tools"
DOCK = HOME / ".local/bin/latte-dock-ng"
HELPERS = [HOME / ".local/bin/latte-dock-ng-preview", HOME / ".local/bin/latte-dock-ng-add-launcher"]
PLASMASHELL = Path("/usr/bin/plasmashell")
UNIT = "latte-r5-dock"
LAYOUT = HOME / ".config/latte/我的布局.layout.latte"


def now() -> str:
    return dt.datetime.now().astimezone().isoformat(timespec="seconds")


def run(cmd: list[str], **kwargs) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, capture_output=True, text=True, **kwargs)


def sha256(path: Path) -> str | None:
    try:
        return hashlib.sha256(path.read_bytes()).hexdigest()
    except OSError:
        return None


def dock_pids() -> list[int]:
    pids = []
    for entry in os.scandir("/proc"):
        if entry.name.isdecimal():
            try:
                if os.readlink(f"/proc/{entry.name}/exe").removesuffix(" (deleted)") == str(DOCK):
                    pids.append(int(entry.name))
            except OSError:
                pass
    return pids


def stop_dock() -> bool:
    run(["systemctl", "--user", "stop", f"{UNIT}.service"])
    # A dock started outside this runner (the session's own unit) must go too, but only by its unit.
    for unit in run(["systemctl", "--user", "list-units", "--no-legend", "--plain", "latte-dock-ng-dev-*.service"]).stdout.split():
        if unit.endswith(".service"):
            run(["systemctl", "--user", "stop", unit])
    deadline = time.monotonic() + 25
    while dock_pids() and time.monotonic() < deadline:
        time.sleep(0.5)
    if dock_pids():
        # Started by the p0-ssh launcher, so there is no unit: SIGTERM by exact process name (never `pkill -f`).
        run(["pkill", "-x", "latte-dock-ng"])
        deadline = time.monotonic() + 25
        while dock_pids() and time.monotonic() < deadline:
            time.sleep(0.5)
    return not dock_pids()


def ssh_like_environment(extra: list[str]) -> dict[str, str]:
    """What an SSH command receives on the Fedora VM (captured with `ssh vm env`), plus the scenario's variables."""
    uid = os.getuid()
    base = {
        "HOME": str(HOME), "USER": HOME.name, "LOGNAME": HOME.name, "SHELL": "/bin/bash",
        "PATH": f"{HOME}/.local/bin:{HOME}/bin:/usr/local/bin:/usr/bin", "LANG": "zh_CN.UTF-8", "KDEDIRS": "/usr",
        "XDG_DATA_DIRS": f"{HOME}/.local/share/flatpak/exports/share:/var/lib/flatpak/exports/share:/usr/local/share:/usr/share",
        "XDG_RUNTIME_DIR": f"/run/user/{uid}", "DBUS_SESSION_BUS_ADDRESS": f"unix:path=/run/user/{uid}/bus",
        "XDG_SESSION_TYPE": "tty", "XDG_SESSION_CLASS": "user", "SSH_CONNECTION": "192.168.122.1 40332 192.168.122.128 22",
    }
    for item in extra:
        name, _, value = item.partition("=")
        base[name] = value
    return base


def start_dock(env: list[str], dock_args: list[str], launcher: str = "systemd") -> int | None:
    if launcher == "p0-ssh":
        # Detached like `ssh vm 'setsid -f nohup launcher'`: no session environment except what the launcher imports.
        subprocess.Popen(["setsid", "-f", "nohup", str(TOOLS / "launch-p0style.sh"), *dock_args], env=ssh_like_environment(env),
                         stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    else:
        cmd = ["systemd-run", "--user", f"--unit={UNIT}", "--collect", "--quiet"]
        cmd += [f"--setenv={item}" for item in env]
        cmd += [str(TOOLS / "launch-dock.sh"), *dock_args]
        result = run(cmd)
        if result.returncode != 0:
            print(json.dumps({"event": "error", "reason": "systemd-run failed", "stderr": result.stderr[-400:]}), flush=True)
            return None
    deadline = time.monotonic() + 40
    while time.monotonic() < deadline:
        pids = dock_pids()
        if len(pids) == 1:
            return pids[0]
        time.sleep(0.5)
    return None


def kwin_probe() -> str:
    result = run([str(TOOLS / "kwin-probe.sh")], timeout=30)
    return result.stdout.strip()


def remove_applets(text: str, plugins: list[str]) -> str:
    """Drop every applet group of the given plugins (and its sub-groups) from the layout ini text and take its id
    out of appletOrder. Used only on the temporary copy a scenario runs with; the pristine bytes are restored."""
    groups: list[tuple[str, list[str]]] = [("", [])]
    for line in text.splitlines(keepends=True):
        if line.startswith("["):
            groups.append((line.strip(), []))
        groups[-1][1].append(line)
    doomed = []
    for header, body in groups:
        match = re.fullmatch(r"\[Containments\]\[(\d+)\]\[Applets\]\[(\d+)\]", header)
        if match and any(line.strip() == f"plugin={plugin}" for line in body for plugin in plugins):
            doomed.append((header, match.group(2)))
    kept = []
    for header, body in groups:
        if any(header == prefix or header.startswith(prefix + "[") for prefix, _ in doomed):
            continue
        if re.fullmatch(r"\[Containments\]\[\d+\]\[General\]", header):
            ids = {applet_id for _, applet_id in doomed}
            body = [("appletOrder=" + ";".join(tok for tok in line.strip().split("=", 1)[1].split(";") if tok not in ids) + "\n")
                    if line.startswith("appletOrder=") else line for line in body]
        kept.extend(body)
    return "".join(kept)


def modified_since(marker: Path) -> list[str]:
    """Files the desktop wrote after the marker (names only): config, cache, data and runtime dirs."""
    roots = [str(HOME / ".config"), str(HOME / ".cache"), str(HOME / ".local/share"), "/tmp", f"/run/user/{os.getuid()}"]
    result = run(["find", *roots, "-xdev", "-type", "f", "-newer", str(marker)], timeout=60)
    ignored = (str(R5), "/tmp/r5-", "/tmp/systemd-private")
    paths = sorted({line.replace(str(HOME), "~") for line in result.stdout.splitlines() if not line.startswith(ignored)})
    return paths[:60]


def pointer_from_probe(line: str) -> str:
    for token in line.split():
        if token.startswith("cursor="):
            return token.removeprefix("cursor=")
    return ""


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--scenario", required=True, help="Name used for the output files and the metadata.")
    parser.add_argument("--revision", required=True, help="Source revision of the installed build (recorded, not checked).")
    parser.add_argument("--starts", type=int, default=5)
    parser.add_argument("--warmup", type=int, default=30)
    parser.add_argument("--seconds", type=int, default=60)
    parser.add_argument("--no-dock", action="store_true", help="Measure with the dock stopped; plasmashell is the primary process.")
    parser.add_argument("--env", action="append", default=[], metavar="NAME=VALUE", help="Environment for the dock unit; repeatable.")
    parser.add_argument("--dock-arg", action="append", default=[], help="Extra latte-dock-ng argument; repeatable.")
    parser.add_argument("--visibility", type=int, help="Temporarily set the layout 'visibility=' value (0 always visible, 1 auto hide, 2 dodge active).")
    parser.add_argument("--prewarm", type=int, default=0, metavar="SECONDS",
                        help="Start the dock once for this long before the first measured start and discard it, so the QML and shader caches of a freshly installed build are warm.")
    parser.add_argument("--launcher", choices=("systemd", "p0-ssh"), default="systemd",
                        help="systemd: transient user unit with the Plasma session environment. p0-ssh: the bare SSH launch P0 used.")
    parser.add_argument("--remove-applet", action="append", default=[], metavar="PLUGIN",
                        help="Run with the layout minus every applet of this plugin (for example org.kde.plasma.kickerdash); repeatable.")
    parser.add_argument("--watch-sycoca", action="store_true",
                        help="Also record every kbuildsycoca6 process during the window, with its parent and environment (r5-sycoca-watch.py).")
    parser.add_argument("--stimulus", help="r5-stim.sh kind and argument, for example 'zenity 5'; started after the dock, stopped after the window.")
    parser.add_argument("--strip-shot", action="store_true",
                        help="Save the bottom 120 pixel rows of the screen, cropped at once so no window content outside the dock area is kept, taken before the window opens.")
    parser.add_argument("--leave-running", action="store_true", help="Leave the last dock running instead of stopping it.")
    parser.add_argument("--out", type=Path, default=R5 / "raw")
    args = parser.parse_args()

    args.out.mkdir(parents=True, exist_ok=True)
    family_path = args.out / f"R5-{args.scenario}-family.jsonl"
    threads_path = args.out / f"R5-{args.scenario}-threads.jsonl"
    family_file = family_path.open("a")
    threads_file = threads_path.open("a")
    sycoca_file = (args.out / f"R5-{args.scenario}-sycoca.jsonl").open("a") if args.watch_sycoca else None

    layout_edited = False
    layout_backup = LAYOUT.read_bytes() if LAYOUT.exists() else None
    layout_hash_before = sha256(LAYOUT)
    try:
        if not stop_dock():
            print(json.dumps({"event": "error", "reason": "dock did not stop"}), flush=True)
            return 2
        if (args.visibility is not None or args.remove_applet) and layout_backup is not None:
            text = layout_backup.decode()
            if args.visibility is not None:
                lines = [f"visibility={args.visibility}" if line.startswith("visibility=") else line for line in text.splitlines()]
                text = "\n".join(lines) + ("\n" if text.endswith("\n") else "")
            if args.remove_applet:
                text = remove_applets(text, args.remove_applet)
            LAYOUT.write_text(text)
            layout_edited = True

        if args.prewarm and not args.no_dock:
            start_dock(args.env, args.dock_arg, args.launcher)
            time.sleep(args.prewarm)
            stop_dock()

        for index in range(1, args.starts + 1):
            if not stop_dock():
                print(json.dumps({"event": "error", "reason": "dock did not stop", "start": index}), flush=True)
                return 2
            time.sleep(3)  # let KWin and the session bus drop the old dock's surfaces and names
            meta = {
                "event": "r5_start", "scenario": args.scenario, "start": index, "time": now(),
                "revision": args.revision, "dock": not args.no_dock, "env": args.env, "dock_args": args.dock_arg, "launcher": args.launcher,
                "visibility_override": args.visibility, "removed_applets": args.remove_applet, "layout_sha256": sha256(LAYOUT),
                "binary_sha256": sha256(DOCK), "kernel": os.uname().release,
            }
            dock_pid = None
            if not args.no_dock:
                dock_pid = start_dock(args.env, args.dock_arg, args.launcher)
                if dock_pid is None:
                    meta["error"] = "dock did not start"
                    for handle in (family_file, threads_file):
                        handle.write(json.dumps(meta, sort_keys=True) + "\n")
                    continue
                meta["dock_pid"] = dock_pid
                meta["dock_cmdline"] = Path(f"/proc/{dock_pid}/cmdline").read_text().replace("\0", " ").strip()

            stimulus = args.stimulus.split() if args.stimulus else None
            if stimulus:
                started = run([str(TOOLS / "r5-stim.sh"), "start", *stimulus], timeout=120)
                meta["stimulus"] = {"kind": stimulus, "start_rc": started.returncode, "stderr": started.stderr[-200:]}

            primary = PLASMASHELL if args.no_dock else DOCK
            family_cmd = [sys.executable, "-I", str(TOOLS / "measure-process-family.py"), "--process", str(primary)]
            if not args.no_dock:
                family_cmd += [item for helper in HELPERS for item in ("--process", str(helper))]
            family_cmd += ["--warmup", str(args.warmup), "--interval", str(args.seconds), "--windows", "1"]
            threads_cmd = [sys.executable, "-I", str(TOOLS / "r5-thread-sampler.py"), "--label", args.scenario,
                           "--process", f"plasmashell={PLASMASHELL}", "--process", "kwin=comm:kwin_wayland",
                           "--warmup", str(args.warmup), "--seconds", str(args.seconds), "--windows", "1"]
            if not args.no_dock:
                threads_cmd += ["--process", f"dock={DOCK}", "--process", f"preview={HELPERS[0]}"]

            watch_proc = (subprocess.Popen([sys.executable, "-I", str(TOOLS / "r5-sycoca-watch.py"), "--warmup", str(args.warmup),
                                            "--seconds", str(args.seconds)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
                          if args.watch_sycoca else None)
            family_proc = subprocess.Popen(family_cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            threads_proc = subprocess.Popen(threads_cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            time.sleep(max(0, args.warmup - 6))
            probe = kwin_probe()
            meta["kwin_probe"] = probe
            meta["pointer"] = pointer_from_probe(probe)
            if args.strip_shot:
                full = Path(f"/tmp/r5-full-{os.getpid()}.png")
                run(["spectacle", "--background", "--nonotify", "--fullscreen", "--output", str(full)], timeout=60)
                strip = args.out / f"R5-{args.scenario}-start{index}-strip.png"
                run(["convert", str(full), "-crop", "1920x120+0+960", "+repage", str(strip)], timeout=30)
                full.unlink(missing_ok=True)
                meta["strip_shot"] = strip.name
            marker = Path(f"/tmp/r5-marker-{os.getpid()}")
            marker.touch()
            family_out = family_proc.communicate()[0]
            threads_out = threads_proc.communicate()[0]
            if watch_proc is not None:
                sycoca_file.write(json.dumps({"event": "r5_start", "scenario": args.scenario, "start": index}, sort_keys=True) + "\n")
                sycoca_file.write(watch_proc.communicate()[0])
                sycoca_file.flush()
            meta["files_modified_during_window"] = modified_since(marker)
            marker.unlink(missing_ok=True)
            if stimulus:
                run([str(TOOLS / "r5-stim.sh"), "stop", stimulus[0]], timeout=120)

            for handle, out in ((family_file, family_out), (threads_file, threads_out)):
                handle.write(json.dumps(meta, sort_keys=True) + "\n")
                handle.write(out if out.endswith("\n") or not out else out + "\n")
                handle.flush()
            print(json.dumps({"event": "r5_start_done", "scenario": args.scenario, "start": index,
                              "dock_pid": dock_pid, "pointer": meta["pointer"], "time": now(),
                              "family_rc": family_proc.returncode, "threads_rc": threads_proc.returncode}), flush=True)
    finally:
        if args.stimulus:
            # Idempotent safety net: a stimulus must never outlive its scenario (a stopped ksmserver above all).
            run([str(TOOLS / "r5-stim.sh"), "stop", args.stimulus.split()[0]], timeout=120)
        if not args.leave_running:
            stop_dock()
        if layout_edited and layout_backup is not None:
            LAYOUT.write_bytes(layout_backup)
            print(json.dumps({"event": "layout_restored", "sha256_before": layout_hash_before, "sha256_after": sha256(LAYOUT)}), flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
