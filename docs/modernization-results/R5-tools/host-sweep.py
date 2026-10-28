#!/usr/bin/env python3
"""R5 helper: sweep the Fedora guest's pointer back and forth along a horizontal line (runs on the host).

Used as a stimulus that makes the dock animate continuously (parabolic zoom under the
pointer), so that the CPU cost of one rendered frame in the VM can be measured next to the
idle numbers. Positions go through the QEMU usb-tablet over one long-lived virsh session
(absolute axes 0..32767 for the 1920x1080 guest screen); the rate is bounded by --period.

usage: host-sweep.py SECONDS Y X_MIN X_MAX [--period 0.05] [--sweep-seconds 2.0]
Afterwards the pointer is parked at --park-x/--park-y (default: the screen centre).
"""

import argparse
import subprocess
import time


def to_abs(x: int, y: int) -> str:
    ax, ay = x * 32767 // 1919, y * 32767 // 1079
    return ('qemu-monitor-command fedora \'{"execute":"input-send-event","arguments":{"events":'
            f'[{{"type":"abs","data":{{"axis":"x","value":{ax}}}}},{{"type":"abs","data":{{"axis":"y","value":{ay}}}}}]}}}}\'\n')


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("seconds", type=float)
    parser.add_argument("y", type=int)
    parser.add_argument("x_min", type=int)
    parser.add_argument("x_max", type=int)
    parser.add_argument("--period", type=float, default=0.05)
    parser.add_argument("--sweep-seconds", type=float, default=2.0, help="Time for one pass from x_min to x_max.")
    parser.add_argument("--park-x", type=int, default=960)
    parser.add_argument("--park-y", type=int, default=540)
    args = parser.parse_args()

    shell = subprocess.Popen(["virsh", "-c", "qemu:///system"], stdin=subprocess.PIPE, stdout=subprocess.DEVNULL,
                             stderr=subprocess.DEVNULL, text=True)
    sent = 0
    start = time.monotonic()
    try:
        while True:
            elapsed = time.monotonic() - start
            if elapsed >= args.seconds:
                break
            phase = (elapsed / args.sweep_seconds) % 2.0
            fraction = phase if phase <= 1.0 else 2.0 - phase
            x = round(args.x_min + (args.x_max - args.x_min) * fraction)
            shell.stdin.write(to_abs(x, args.y))
            shell.stdin.flush()
            sent += 1
            time.sleep(args.period)
        # park: nudge first because the tablet only reports a changed position
        shell.stdin.write(to_abs(args.park_x + 3, args.park_y + 3))
        shell.stdin.flush()
        time.sleep(0.3)
        shell.stdin.write(to_abs(args.park_x, args.park_y))
        shell.stdin.flush()
        time.sleep(0.3)
    finally:
        shell.stdin.close()
        shell.wait(timeout=10)
    print(f"sent {sent} positions in {time.monotonic() - start:.1f} s")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
