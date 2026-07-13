/*
    SPDX-FileCopyrightText: 2026 Latte Dock Contributors
    SPDX-License-Identifier: GPL-2.0-or-later

    On-demand window screenshot via KWin ScreenShot2 D-Bus API.

    When windowUuid is set, captureWindow() is called on the provider
    which talks to KWin's ScreenShot2 interface directly.
    The result is delivered asynchronously via screenshotReady() and
    displayed as a static Image. No PipeWire stream, no worker threads.
*/

import QtQuick 2.6

import org.kde.latte.private.tasks 0.1 as LatteTasks

Item {
    id: root

    property string windowUuid: ""
    readonly property bool hasThumbnail: screenshotImage.status === Image.Ready
    property LatteTasks.WindowScreenshotProvider provider: null

    // Trigger capture when windowUuid is set
    onWindowUuidChanged: {
        if (!windowUuid || !provider)
            return;

        var cached = provider.cachedScreenshot(windowUuid);
        if (cached) {
            screenshotImage.source = "file://" + cached;
        } else if (!provider.hasPending(windowUuid)) {
            provider.captureWindow(windowUuid);
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

    // Placeholder circle while waiting
    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width, parent.height) * 0.25
        height: width
        radius: width / 2
        color: Qt.rgba(1, 1, 1, 0.08)
        visible: !screenshotImage.visible && windowUuid !== ""
    }
}
