/*
    SPDX-FileCopyrightText: 2026 Latte Dock Contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.6

import org.kde.taskmanager 0.1 as TaskManager
import org.kde.latte.private.tasks 0.1 as LatteTasks

/**
 * Background PipeWire screenshot via C++ worker thread.
 *
 * ScreencastingRequest obtains the PipeWire nodeId from KWin, then
 * captureViaPipeWire() runs the stream + frame-capture in a worker
 * thread — zero main-thread jank.  The result PNG is displayed as a
 * static Image.
 */
Item {
    id: root

    property string windowUuid: ""
    readonly property bool hasThumbnail: screenshotImage.status === Image.Ready
    property LatteTasks.WindowScreenshotProvider provider: null

    TaskManager.ScreencastingRequest {
        id: screencastRequest
        uuid: root.windowUuid
        onNodeIdChanged: {
            if (nodeId > 0 && provider && windowUuid) {
                var cached = provider.cachedScreenshot(windowUuid);
                if (!cached && !provider.hasPending(windowUuid)) {
                    provider.captureViaPipeWire(nodeId, windowUuid);
                }
            }
        }
    }

    Connections {
        target: provider
        enabled: provider !== null
        function onScreenshotReady(uuid, filePath) {
            if (uuid === root.windowUuid)
                screenshotImage.source = "file://" + filePath;
        }
    }

    Image {
        id: screenshotImage
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        visible: status === Image.Ready
        asynchronous: true
        smooth: true
    }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width, parent.height) * 0.25
        height: width
        radius: width / 2
        color: Qt.rgba(1, 1, 1, 0.08)
        visible: !screenshotImage.visible && windowUuid !== ""
    }

    // Secondary trigger: fire when windowUuid changes after nodeId is ready
    onWindowUuidChanged: {
        if (windowUuid && provider && screencastRequest.nodeId > 0) {
            var cached = provider.cachedScreenshot(windowUuid);
            if (!cached && !provider.hasPending(windowUuid)) {
                provider.captureViaPipeWire(screencastRequest.nodeId, windowUuid);
            }
        }
    }
}
