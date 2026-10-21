/*
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick
import QtQuick.Controls as Controls

Controls.ToolButton {
    id: helpButton
    required property string guideText
    signal guideRequested(string section)

    icon.name: "help-contents"
    Accessible.name: helpButton.text
    onClicked: helpMenu.popup()

    Controls.Menu {
        id: helpMenu
        objectName: "helpMenu"
        y: helpButton.height
        // Some styles give menus a fixed implicit width. Translated labels
        // are authoritative here; size for the guide label to prevent elision.
        width: Math.max(implicitWidth, guideAction.implicitWidth + leftPadding + rightPadding)
        Controls.MenuItem {
            id: guideAction
            objectName: "guideAction"
            text: helpButton.guideText
            icon.name: "help-contents"
            onTriggered: helpButton.guideRequested("edit-dock")
        }
    }
}
