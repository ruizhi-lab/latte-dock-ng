/* SPDX-FileCopyrightText: 2026 Latte Dock Contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */
import QtQuick
import org.kde.pipewire as PipeWire
import org.kde.taskmanager as TaskManager
PipeWire.PipeWireSourceItem {
    id: source
    required property string uuid
    // KPipeWire 6.7.5 can truncate its offered video formats at a 4 KiB buffer,
    // omitting the BGRA/BGRx formats KWin provides. The resulting
    // "no more input formats" stream error leaves ready false and this window
    // on the loading placeholder. Track KDE's buffer-size fix (MR 283) and
    // remove this workaround note once the minimum packaged KPipeWire includes it.
    // Invisible sources pause capture, so never bind visibility to ready.
    nodeId: request.nodeId
    TaskManager.ScreencastingRequest {
        id: request
        uuid: source.uuid
    }
}
