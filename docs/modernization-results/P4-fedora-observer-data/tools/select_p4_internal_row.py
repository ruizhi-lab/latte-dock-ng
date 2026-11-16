#!/usr/bin/env python3
"""Select and persist the unique controlled row from P4 tracker diagnostics."""

from __future__ import annotations

import json
from pathlib import Path
import sys


def select_internal_row(log_path: Path, output_path: Path) -> str:
    if log_path.resolve() == output_path.resolve():
        raise SystemExit("diagnostic input and parity output paths must be different")

    latest: dict[int, dict] = {}
    for line in log_path.read_text().splitlines():
        marker = "P4_TRACKER_ROW "
        if marker not in line:
            continue
        try:
            row = json.loads(line.split(marker, 1)[1])
        except json.JSONDecodeError:
            continue
        if row.get("appId") == "org.kde.dolphin" and row.get("appletId") in (3, 4):
            latest[row["appletId"]] = row

    if set(latest) != {3, 4}:
        raise SystemExit(f"expected matching-row snapshots for applets 3 and 4, found {sorted(latest)}")
    left, right = latest[3], latest[4]
    identity = (left.get("appId"), left.get("title"), left.get("winIdList"))
    if (not identity[1] or not identity[2]
            or identity != (right.get("appId"), right.get("title"), right.get("winIdList"))):
        raise SystemExit("applets 3 and 4 do not show the same unique controlled Dolphin row")
    if left.get("screen") != right.get("screen"):
        raise SystemExit("applets 3 and 4 do not report the same screen geometry")

    for applet_id, row in latest.items():
        if (row.get("isWindow") is not True or row.get("isStartup") is not False
                or row.get("isActive") is not True or row.get("isMinimized") is not False):
            raise SystemExit(f"applet {applet_id} controlled row is not an active, shown window")
        activity = row.get("activity")
        activity_hex = str(activity).replace("-", "").lower() if activity is not None else ""
        if (activity != left.get("activity") or activity in (None, "")
                or (activity_hex and not activity_hex.strip("0"))):
            raise SystemExit("controlled rows do not share a valid activity")
        desktop = row.get("desktop")
        desktop_hex = str(desktop).replace("-", "").lower() if desktop is not None else ""
        if (desktop != left.get("desktop") or desktop in (None, "")
                or (desktop_hex and not desktop_hex.strip("0"))):
            raise SystemExit("controlled rows do not share a valid desktop")
        screen = row.get("screen")
        if not screen or screen.get("width", 0) <= 0 or screen.get("height", 0) <= 0:
            raise SystemExit(f"applet {applet_id} has invalid screen geometry")

    output_path.write_text(json.dumps({
        "appId": identity[0], "title": identity[1], "winIdList": identity[2],
        "activity": left["activity"], "desktop": left["desktop"], "screen": left["screen"],
    }) + "\n")
    return left["title"]


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: select_p4_internal_row.py latte.log internal-parity.json")
    print(select_internal_row(Path(sys.argv[1]), Path(sys.argv[2])))
