/*
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.5
import QtQuick.Layouts 1.3
import QtQuick.Templates 2.2 as T
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.kirigami 2.2 as Kirigami

import org.kde.latte.components 1.0 as LatteComponents

import "private" as Private

T.CheckDelegate {
    id: control
    implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding
    implicitHeight: isSeparator ? 1 : Math.max(contentItem.implicitHeight, indicator ? indicator.implicitHeight : 0) + topPadding + bottomPadding
    hoverEnabled: !isSeparator

    readonly property var units: Kirigami.Units
    readonly property var safeTheme: (typeof theme !== "undefined" && theme) ? theme : Kirigami.Theme

    topPadding: margin
    bottomPadding: margin
    leftPadding: isSeparator ? 0 : margin
    rightPadding: isSeparator ? 0 : margin
    spacing: units.smallSpacing

    property bool isSeparator: false

    property bool blankSpaceForEmptyIcons: false
    property string iconName
    property string iconToolTip
    property bool iconOnlyWhenHovered
    property string toolTip

    property int textHorizontalAlignment: Text.AlignLeft

    readonly property bool isHovered: hovered || iconMouseArea.containsMouse
    readonly property int margin: isSeparator ? 1 : 4

    contentItem: RowLayout {
        Layout.leftMargin: control.mirrored && !isSeparator ? (control.indicator ? control.indicator.width : 0) + control.spacing : 0
        Layout.rightMargin: !control.mirrored && !isSeparator ? (control.indicator ? control.indicator.width : 0) + control.spacing : 0
        spacing: isSeparator ? 0 : units.smallSpacing
        enabled: control.enabled

        Rectangle {
            Layout.minimumWidth: parent.height
            Layout.maximumWidth: parent.height
            Layout.minimumHeight: parent.height
            Layout.maximumHeight: parent.height
            visible: !isSeparator && iconName && (!control.iconOnlyWhenHovered || (control.iconOnlyWhenHovered && control.isHovered))
            color: control.iconToolTip && iconMouseArea.containsMouse ? safeTheme.highlightColor : "transparent"

            Kirigami.Icon {
                id: iconElement
                anchors.fill: parent
                source: control.iconName
            }

            LatteComponents.ToolTip{
                parent: iconElement
                text: iconToolTip
                visible: iconMouseArea.containsMouse
                delay: 6 * units.longDuration
            }

            MouseArea {
                id: iconMouseArea
                anchors.fill: parent
                hoverEnabled: true
                visible: control.iconToolTip

                onClicked: control.ListView.view.iconClicked(index);
            }
        }

        Rectangle {
            //blank space when no icon is shown
            Layout.minimumHeight: parent.height
            Layout.minimumWidth: parent.height
            visible: !isSeparator && control.blankSpaceForEmptyIcons && (!iconName || (control.iconOnlyWhenHovered && !control.isHovered) )
            color: "transparent"
        }

        Label {
            Layout.fillWidth: true
            text: control.text
            font: control.font
            color: safeTheme.textColor
            elide: Text.ElideRight
            visible: !isSeparator && control.text
            horizontalAlignment: control.textHorizontalAlignment
            verticalAlignment: Text.AlignVCenter
        }

        Rectangle {
            width: parent.width
            height: 1
            color: safeTheme.textColor
            opacity: 0.25
            visible: isSeparator
        }
    }

    //background: Private.DefaultListItemBackground {}
    background: Rectangle {
        visible: isSeparator ? false : (control.ListView.view ? control.ListView.view.highlight === null : true)
        enabled: control.enabled
        opacity: {
            if (control.highlighted || control.pressed) {
                return 0.6;
            } else if (control.isHovered && !control.pressed) {
                return 0.3;
            }

            return 0;
        }

        color: safeTheme.highlightColor
    }
}
