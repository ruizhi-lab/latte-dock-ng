/*
    SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick

import org.kde.pipewire as PipeWire
import org.kde.taskmanager 0.1 as TaskManager

// Mirrors plasma-desktop's applets/taskmanager/qml/PipeWireThumbnail.qml.
// Latte previously wrapped this in an opacity-animation Item with
// `enabled: false` and `visible: waylandItem.nodeId > 0` guards, which
// prevented the source from ever producing frames (you'd get the
// thumbnail frame but no contents, plus periodic "No QSGTexture provided
// from updateSampledImage()" warnings while nodeId was 0). Sticking close
// to upstream lets PipeWireSourceItem manage its own readiness via the
// `ready` property; ToolTipInstance gates visibility on that.
PipeWire.PipeWireSourceItem {
    id: pipeWireSourceItem

    readonly property alias hasThumbnail: pipeWireSourceItem.ready

    anchors.fill: parent
    nodeId: waylandItem.nodeId

    TaskManager.ScreencastingRequest {
        id: waylandItem
        uuid: thumbnailSourceItem.winId
    }
}
