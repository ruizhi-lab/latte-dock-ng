#!/usr/bin/env python3
"""Local source-level check for the actual P4 diagnostic generator."""

import json
import pathlib
import tempfile

from generate_p4_diagnostics import (
    LOADER_LOG,
    ROOT_LOG,
    ROW_LOG,
    SNAPSHOT_LOG,
    SNAPSHOT_PROPERTY,
    generate,
)
from select_p4_internal_row import select_internal_row


REPO = pathlib.Path(__file__).resolve().parents[4]
PRODUCTION = REPO / "containment/package/contents/ui/applet/AppletItem.qml"
source = PRODUCTION.read_text()
with tempfile.TemporaryDirectory(prefix="p4-diagnostic-generator-") as temp:
    target = pathlib.Path(temp) / "AppletItem.qml"
    target.write_text(source)
    actual = generate(target.read_text())
    target.write_text(actual)
    assert target.read_text() == actual

    # The output path is explicit, even when the input has an arbitrary name;
    # selecting the title must never truncate the source diagnostics.
    activity = "e4cc15c3-9a53-4ccc-890a-9b174ba74c02"
    desktop = "cfc99af5-557d-4416-a7a6-4cd576ab6155"
    screen = {"x": 0, "y": 0, "width": 1280, "height": 800}
    rows = []
    for applet_id in (3, 4):
        rows.append("[Warning] P4_TRACKER_ROW " + json.dumps({
            "appletId": applet_id,
            "appId": "org.kde.dolphin",
            "title": "window-title — Dolphin",
            "winIdList": ["{cb945855-9266-45fe-aeff-36d49e933751}"],
            "isWindow": True,
            "isStartup": False,
            "isActive": True,
            "isMinimized": False,
            "activity": activity,
            "desktop": desktop,
            "screen": screen,
        }))
    arbitrary_input = pathlib.Path(temp) / "arbitrary-input.log"
    parity_output = pathlib.Path(temp) / "internal-parity.json"
    arbitrary_input.write_text("\n".join(rows) + "\n")
    original_log = arbitrary_input.read_bytes()
    selected_title = select_internal_row(arbitrary_input, parity_output)
    assert selected_title == "window-title — Dolphin"
    assert arbitrary_input.read_bytes() == original_log
    parity = json.loads(parity_output.read_text())
    assert parity["activity"] == activity and parity["desktop"] == desktop
    assert parity["screen"] == screen and parity["winIdList"] == ["{cb945855-9266-45fe-aeff-36d49e933751}"]
    try:
        select_internal_row(arbitrary_input, arbitrary_input)
    except SystemExit as error:
        assert "must be different" in str(error)
    else:
        raise AssertionError("selector accepted identical input and output paths")
    assert arbitrary_input.read_bytes() == original_log

assert actual.count("Component.onCompleted:") == source.count("Component.onCompleted:")
assert actual.count("P4_APPLET_ROOT ") == 1
assert actual.count("P4_TRACKER_LOADER ") == 1
assert actual.count("P4_TRACKER_ROW ") == 1
assert "console.warn(\"P4_TRACKER_ROW " in ROW_LOG
assert actual.count("P4_TRACKER_SNAPSHOT ") == 1
assert "p4DiagnosticSnapshotCount < 64" in actual
assert "Math.min(model.count, 16)" in actual
diagnostic_lines = [
    line for line in actual.splitlines()
    if "P4_APPLET_ROOT " in line or "P4_TRACKER_" in line
]
assert all("applet.id" not in line and "appletItem.applet.id" not in line for line in diagnostic_lines)
assert all("appletItem.backendAppletId" in line for line in diagnostic_lines)

# Removing the inserted diagnostic property and log block must recover the
# production source byte-for-byte; generation never rewrites applet logic.
without_logs = actual.replace(ROW_LOG, "")
without_logs = without_logs.replace(LOADER_LOG, "")
without_logs = without_logs.replace(ROOT_LOG, "")
without_logs = without_logs.replace(SNAPSHOT_LOG, "")
without_logs = without_logs.replace(SNAPSHOT_PROPERTY, "")
assert without_logs == source
print("P4 diagnostic generator: PASS (unique anchors, nullable-safe IDs, production source unchanged)")
