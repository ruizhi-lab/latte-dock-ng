/*
    SPDX-FileCopyrightText: 2019 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.7
import QtQuick.Layouts 1.3

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.kirigami 2.0 as Kirigami

import org.kde.plasma.plasmoid 2.0

import "." as LatteExtraControls

Item {
    id: item
    readonly property var units: Kirigami.Units
    Kirigami.Theme.inherit: true
    Kirigami.Theme.colorSet: Kirigami.Theme.Window

    Layout.rightMargin: {
        if (level === 1) {
            return Qt.application.layoutDirection === Qt.RightToLeft ? 0 : 2 * units.smallSpacing
        }

        return 0;
    }
    Layout.leftMargin: {
        if (level === 1) {
            return Qt.application.layoutDirection === Qt.RightToLeft ? 2 * units.smallSpacing : 0
        }

        return 0;
    }

    property int level:1
    property bool checked: false
    property bool isFirstSubCategory: false

    readonly property int implicitWidth: row.width

    readonly property int implicitHeight: {
        if (level === 1) {
            return Math.max(headerText.implicitHeight, itemSwitch.implicitHeight);
        } else if (level === 2) {
            return Math.max(subHeaderText.implicitHeight, itemSwitch.implicitHeight)
        }

        return Math.max(labelText.implicitHeight, itemSwitch.implicitHeight);
    }

    property string text:""
    property string tooltip:""

    signal pressed();

    Item {
        id: row
        width: parent.width
        height: textElement.height
        anchors.verticalCenter: parent.verticalCenter

        RowLayout {
            id: textElement
            anchors.left: level !== 2 ? parent.left : undefined
            anchors.horizontalCenter: level === 2 ? parent.horizontalCenter : undefined
            anchors.verticalCenter: parent.verticalCenter

            LatteExtraControls.Header {
                id: headerText
                text: item.text
                enabled: item.checked && item.enabled
                visible: level === 1
                Kirigami.Theme.inherit: true
                Kirigami.Theme.colorSet: Kirigami.Theme.Window
            }

            LatteExtraControls.SubHeader {
                id: subHeaderText
                text: item.text
                enabled: item.checked && item.enabled
                visible: level === 2
                isFirstSubCategory: item.isFirstSubCategory
                Kirigami.Theme.inherit: true
                Kirigami.Theme.colorSet: Kirigami.Theme.Window
            }

            PlasmaComponents.Label {
                id: labelText
                text: item.text
                enabled: item.checked && item.enabled
                visible: level > 2
                Kirigami.Theme.inherit: true
                Kirigami.Theme.colorSet: Kirigami.Theme.Window
            }
        }

        PlasmaComponents.Button {
            //tooltip ghost
            anchors.fill: textElement
            opacity: 0
            Kirigami.Theme.inherit: true
            Kirigami.Theme.colorSet: Kirigami.Theme.Window

            PlasmaComponents.ToolTip.delay: Kirigami.Units.toolTipDelay
            PlasmaComponents.ToolTip.visible: hovered && item.tooltip !== ""
            PlasmaComponents.ToolTip.text: item.tooltip

            onPressedChanged: {
                if (pressed) {
                    item.pressed();
                }
            }
        }
    }

    LatteExtraControls.Switch {
        id: itemSwitch
        anchors.verticalCenter: row.verticalCenter
        anchors.right: row.right
        checked: item.checked
        enabled: item.enabled
        Kirigami.Theme.inherit: true
        Kirigami.Theme.colorSet: Kirigami.Theme.Window

        PlasmaComponents.Button {
            //tooltip ghost
            anchors.fill: parent
            opacity: 0
            Kirigami.Theme.inherit: true
            Kirigami.Theme.colorSet: Kirigami.Theme.Window

            PlasmaComponents.ToolTip.delay: Kirigami.Units.toolTipDelay
            PlasmaComponents.ToolTip.visible: hovered && item.tooltip !== ""
            PlasmaComponents.ToolTip.text: item.tooltip

            onPressedChanged: {
                if (pressed) {
                    item.pressed();
                }
            }
        }
    }
}
