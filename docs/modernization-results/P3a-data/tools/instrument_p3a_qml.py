#!/usr/bin/env python3
"""Add bounded observability markers to a disposable main.qml overlay."""

from pathlib import Path
import sys


def replace_once(source: str, before: str, after: str, label: str) -> str:
    if source.count(before) != 1:
        raise SystemExit(f"expected exactly one {label} insertion point")
    return source.replace(before, before + after, 1)


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit("usage: instrument_p3a_qml.py OVERLAY_TASKS_MAIN_QML OVERLAY_CONTAINMENT_MAIN_QML")

    tasks_path = Path(sys.argv[1])
    source = tasks_path.read_text()
    source = replace_once(
        source,
        "    Component.onCompleted:  {\n",
        '        console.warn("P3A_SOURCE_LOADED " + Qt.resolvedUrl("main.qml"));\n',
        "root Component.onCompleted",
    )
    source = replace_once(
        source,
        "        function onLocationChanged() {\n",
        '            console.warn("P3A_EDGE_SIGNAL locationChanged value=" + root.plasmoid.location);\n',
        "locationChanged handler",
    )
    source = replace_once(
        source,
        "        function onFormFactorChanged() {\n",
        '            console.warn("P3A_EDGE_SIGNAL formFactorChanged");\n',
        "formFactorChanged handler",
    )
    source = replace_once(
        source,
        "    function refreshTaskLayoutPass(reason, pass) {\n",
        '        console.warn("P3A_REFRESH_PASS reason=" + reason + " pass=" + pass);\n',
        "refreshTaskLayoutPass",
    )
    tasks_path.write_text(source)

    containment_path = Path(sys.argv[2])
    source = containment_path.read_text()
    driver = '''
    // Private P3a probe only: use the same Positioner API as the Dock edge buttons.
    Timer {
        id: p3aTopEdgeDriver
        interval: 15000
        running: true
        repeat: false
        onTriggered: {
            if (!root.latteView || !root.latteView.positioner) {
                console.warn("P3A_DRIVER_NO_GO positioner-not-ready");
                return;
            }
            console.warn("P3A_DRIVER_REQUEST edge=TopEdge value=" + PlasmaCore.Types.TopEdge);
            root.latteView.positioner.setNextLocation("", root.latteView.screensGroup, "",
                                                      PlasmaCore.Types.TopEdge, LatteCore.types.NoneAlignment);
            p3aBottomEdgeDriver.start();
        }
    }

    Timer {
        id: p3aBottomEdgeDriver
        interval: 6000
        repeat: false
        onTriggered: {
            console.warn("P3A_DRIVER_REQUEST edge=BottomEdge value=" + PlasmaCore.Types.BottomEdge);
            root.latteView.positioner.setNextLocation("", root.latteView.screensGroup, "",
                                                      PlasmaCore.Types.BottomEdge, LatteCore.types.NoneAlignment);
        }
    }

'''
    source = replace_once(
        source,
        "ContainmentItem {\n    id: root\n",
        driver,
        "containment-root child edge-driver insertion",
    )
    containment_path.write_text(source)


if __name__ == "__main__":
    main()
