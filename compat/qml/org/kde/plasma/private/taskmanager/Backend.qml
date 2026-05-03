/*
    SPDX-FileCopyrightText: 2026 Latte Dock Contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0
import org.kde.latte.private.tasks 0.1 as LatteTasks

QtObject {
    id: backend

    // Keep compatibility with APIs used by Latte's tasks QML when the
    // plasma-private taskmanager module is not available on the distro.
    property Item taskManagerItem: null
    property bool highlightWindows: false
    property bool windowViewAvailable: false
    property bool canPresentWindows: false
    property var groupDialog: null
    property var toolTipItem: null

    signal addLauncher(var url)
    signal showAllPlaces()

    readonly property QtObject contextMenuActionsBackend: LatteTasks.ContextMenuActionsBackend {}
    readonly property var contextMenuActionsBridge: Connections {
        target: contextMenuActionsBackend
        function onShowAllPlaces() {
            backend.showAllPlaces()
        }
    }

    function activateWindowView(winIds) {
    }

    function presentWindows(winIds) {
    }

    function windowsHovered(winIds, hovered) {
    }

    function cancelHighlightWindows() {
    }

    function ungrabMouse(item) {
    }

    function setActionGroup(action) {
    }

    function placesActions(launcherUrl, showAllPlacesValue, menu) {
        return contextMenuActionsBackend.placesActions(launcherUrl, showAllPlacesValue, menu);
    }

    function recentDocumentActions(launcherUrl, menu) {
        return contextMenuActionsBackend.recentDocumentActions(launcherUrl, menu);
    }

    function jumpListActions(launcherUrl, menu) {
        return contextMenuActionsBackend.jumpListActions(launcherUrl, menu);
    }

    function parentPid(pid) {
        return -1;
    }

    function isApplication(item) {
        if (!item) {
            return false;
        }

        const value = String(item);
        return value.indexOf(".desktop") !== -1 || value.indexOf("applications:") === 0;
    }

    function jsonArrayToUrlList(urls) {
        if (!urls) {
            return [];
        }

        if (Array.isArray(urls)) {
            return urls;
        }

        if (typeof urls.length === "number") {
            var result = [];
            for (var i = 0; i < urls.length; ++i) {
                result.push(urls[i]);
            }
            return result;
        }

        return [urls];
    }

    function generateMimeData(mimeType, mimeData, launcherUrl) {
        var data = {};

        if (mimeType && mimeData) {
            data[mimeType] = mimeData;
        }

        if (launcherUrl) {
            data["text/uri-list"] = [launcherUrl];
        }

        return data;
    }

    function globalRect(item) {
        if (!item) {
            return Qt.rect(0, 0, 0, 0);
        }

        var p = item.mapToGlobal ? item.mapToGlobal(0, 0) : Qt.point(item.x, item.y);
        return Qt.rect(p.x, p.y, item.width || 0, item.height || 0);
    }
}
