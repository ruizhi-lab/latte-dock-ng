/*
    SPDX-FileCopyrightText: 2016 Smith AR <audoban@openmailbox.org>
    SPDX-FileCopyrightText: 2016 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import org.kde.kirigami as Kirigami
import org.kde.ksvg as KSvg
import org.kde.plasma.components as PlasmaComponents

import org.kde.latte.core as LatteCore

KSvg.FrameSvgItem {
    id: dialog

    imagePath: "widgets/panel-background"
    prefix:""
    enabledBorders: infoWindow ? infoWindow.enabledBorders : KSvg.FrameSvg.AllBorders

    // Plasma 6 has no global `units`; an undefined name here left the height
    // at 0, and a zero-sized window is a Wayland protocol error that makes
    // KWin disconnect Latte (the docks vanished when switching layouts).
    width: Screen.width + 1
    height: Math.min(Screen.height - Kirigami.Units.gridUnit * 8, logo.height + messageLbl.height + 2 * Kirigami.Units.gridUnit)

    property var infoWindow
    property string message

    ColumnLayout {
        id: root

        anchors.centerIn: parent

        Item{
            id: iconItem
            Layout.fillWidth: true
            Layout.minimumHeight: logo.height

            LatteCore.IconItem {
                id: logo
                anchors.centerIn: parent

                width: Kirigami.Units.iconSizes.huge
                height: width

                source: "latte-dock-ng"
            }
        }

        PlasmaComponents.Label {
            id: messageLbl
            Layout.fillWidth: true

            horizontalAlignment: Text.AlignHCenter
            text: dialog.message
            font.italic: true
        }
    }
}
