#!/usr/bin/env python3
"""Insert temporary P4 diagnostics into the staged AppletItem.qml copy."""

import pathlib
import sys


ROOT_LOG = '''        console.warn("P4_APPLET_ROOT " + JSON.stringify({appletId: appletItem.backendAppletId, tracked: trackedWindowAppIds, loaderActive: trackedWindowsModelLoader.active, indexer: indexerIsSupported}));\n'''
LOADER_LOG = '''        onActiveChanged: console.warn("P4_TRACKER_LOADER " + JSON.stringify({appletId: appletItem.backendAppletId, active: active, indexer: appletItem.indexerIsSupported, appIds: appletItem.trackedWindowAppIds}))\n'''
ROW_LOG = '''            console.warn("P4_TRACKER_ROW " + JSON.stringify({appletId: appletItem.backendAppletId, appId: appId, title: model.data(modelIndex, Qt.DisplayRole), winIdList: model.data(modelIndex, roles.WinIdList), isWindow: model.data(modelIndex, roles.IsWindow), isStartup: model.data(modelIndex, roles.IsStartup), isActive: model.data(modelIndex, roles.IsActive), isMinimized: model.data(modelIndex, roles.IsMinimized), activity: model.activity, desktop: model.virtualDesktop, screen: {x: model.screenGeometry.x, y: model.screenGeometry.y, width: model.screenGeometry.width, height: model.screenGeometry.height}}));\n'''
SNAPSHOT_PROPERTY = '''    property int p4DiagnosticSnapshotCount: 0\n'''
SNAPSHOT_LOG = '''        if (appletItem.p4DiagnosticSnapshotCount < 64) {
            appletItem.p4DiagnosticSnapshotCount += 1;
            var diagnosticRows = [];
            for (var diagnosticRow = 0; diagnosticRow < Math.min(model.count, 16); ++diagnosticRow) {
                var diagnosticIndex = model.makeModelIndex(diagnosticRow);
                if (!diagnosticIndex) {
                    diagnosticRows.push({index: diagnosticRow, valid: false});
                    continue;
                }
                diagnosticRows.push({index: diagnosticRow, appId: model.data(diagnosticIndex, roles.AppId), title: model.data(diagnosticIndex, Qt.DisplayRole), winIdList: model.data(diagnosticIndex, roles.WinIdList), isWindow: model.data(diagnosticIndex, roles.IsWindow), isStartup: model.data(diagnosticIndex, roles.IsStartup)});
            }
            console.warn("P4_TRACKER_SNAPSHOT " + JSON.stringify({appletId: appletItem.backendAppletId, count: model.count, activity: model.activity, desktop: model.virtualDesktop, screen: {x: model.screenGeometry.x, y: model.screenGeometry.y, width: model.screenGeometry.width, height: model.screenGeometry.height}, rows: diagnosticRows}));
        }
'''


def insert_once(source: str, needle: str, insertion: str, description: str) -> str:
    if source.count(needle) != 1:
        raise SystemExit(f"expected unique {description} insertion point")
    return source.replace(needle, needle + insertion, 1)


def generate(source: str) -> str:
    # backendAppletId is the nullable-safe diagnostic identity; applet.id may
    # be null while an applet is being detached from its containment.
    source = insert_once(
        source,
        "    id: appletItem\n",
        SNAPSHOT_PROPERTY,
        "snapshot-counter property",
    )
    source = insert_once(
        source,
        "    Component.onCompleted: {\n        checkIndex();\n",
        ROOT_LOG,
        "applet-root",
    )
    source = insert_once(
        source,
        "        active: !appletItem.indexerIsSupported\n"
        "                && appletItem.trackedWindowAppIds.length > 0\n"
        "                && !!latteView\n",
        LOADER_LOG,
        "tracker-loader",
    )
    source = insert_once(
        source,
        "        if (!model || !roles) {\n"
        "            trackedWindowsCount = 0;\n"
        "            trackedShownWindowsCount = 0;\n"
        "            trackedActiveWindowsCount = 0;\n"
        "            return;\n"
        "        }\n\n",
        SNAPSHOT_LOG,
        "pre-filter bounded snapshot",
    )
    source = insert_once(
        source,
        "            if (trackedWindowAppIds.indexOf(appId) < 0) {\n"
        "                continue;\n"
        "            }\n",
        ROW_LOG,
        "matching-row",
    )
    return source


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: generate_p4_diagnostics.py AppletItem.qml")
    target = pathlib.Path(sys.argv[1])
    target.write_text(generate(target.read_text()))
