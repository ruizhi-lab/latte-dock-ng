/*
    SPDX-FileCopyrightText: 2019 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.0 as Kirigami

QQC2.Slider {
    id: control
    Kirigami.Theme.inherit: true

    readonly property var units: Kirigami.Units
    readonly property bool minimumInternalValueIsSet: (minimumInternalValue !== from && minimumInternalValue !== -10000)
    property int minimumInternalValue: -10000

    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                            handle ? handle.implicitWidth : 0) + leftPadding + rightPadding
    implicitHeight: Math.max(background ? background.implicitHeight : 0,
                             handle ? handle.implicitHeight : 0) + topPadding + bottomPadding

    topPadding: 2 * units.smallSpacing
    bottomPadding: 2 * units.smallSpacing
    leftPadding: units.smallSpacing
    rightPadding: units.smallSpacing
    snapMode: QQC2.Slider.SnapOnRelease

    readonly property color trackColor: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.22)
    readonly property color fillColor: Kirigami.Theme.highlightColor
    readonly property color limitColor: Qt.rgba(Kirigami.Theme.highlightColor.r, Kirigami.Theme.highlightColor.g, Kirigami.Theme.highlightColor.b, 0.35)
    readonly property color handleColor: Kirigami.Theme.backgroundColor
    readonly property color handleBorderColor: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.35)
    readonly property real fillOpacity: (enabled && minimumInternalValueIsSet && value < minimumInternalValue) ? 0.35 : (enabled ? 1 : 0.45)

    background: Item {
        id: sliderBackground
        readonly property bool horizontal: control.orientation === Qt.Horizontal

        implicitWidth: horizontal ? units.gridUnit * 8 : units.gridUnit
        implicitHeight: horizontal ? units.gridUnit : units.gridUnit * 8
        width: horizontal ? control.availableWidth : implicitWidth
        height: horizontal ? implicitHeight : control.availableHeight
        anchors.centerIn: parent

        Rectangle {
            id: track
            anchors.centerIn: parent
            width: sliderBackground.horizontal ? sliderBackground.width : Math.max(3, units.smallSpacing)
            height: sliderBackground.horizontal ? Math.max(3, units.smallSpacing) : sliderBackground.height
            radius: Math.max(1, units.smallSpacing / 2)
            color: control.trackColor
        }

        Rectangle {
            id: highlightTrack
            radius: track.radius
            color: control.fillColor
            opacity: control.fillOpacity

            x: sliderBackground.horizontal ? track.x : track.x
            y: sliderBackground.horizontal
               ? track.y
               : (track.y + (1 - control.visualPosition) * track.height)
            width: sliderBackground.horizontal
                   ? control.position * track.width
                   : track.width
            height: sliderBackground.horizontal
                    ? track.height
                    : control.position * track.height
        }

        Rectangle {
            visible: control.minimumInternalValueIsSet
            color: control.limitColor
            radius: Math.max(1, units.smallSpacing / 2)

            readonly property real limitedPer: (control.to - control.from) === 0
                                                ? 0
                                                : ((control.minimumInternalValue - control.from) / (control.to - control.from))

            x: sliderBackground.horizontal
               ? (track.x + Math.max(0, Math.min(1, limitedPer)) * track.width) - width / 2
               : track.x
            y: sliderBackground.horizontal
               ? track.y
               : (track.y + (1 - Math.max(0, Math.min(1, limitedPer))) * track.height) - height / 2
            width: sliderBackground.horizontal ? Math.max(2, units.smallSpacing / 2) : track.width
            height: sliderBackground.horizontal ? track.height : Math.max(2, units.smallSpacing / 2)
        }
    }

    handle: Rectangle {
        x: control.leftPadding + (control.orientation === Qt.Horizontal
                                  ? control.visualPosition * (control.availableWidth - width)
                                  : (control.availableWidth - width) / 2)
        y: control.topPadding + (control.orientation === Qt.Horizontal
                                 ? (control.availableHeight - height) / 2
                                 : (1 - control.visualPosition) * (control.availableHeight - height))
        width: Math.max(units.gridUnit * 0.9, 16)
        height: width
        radius: width / 2
        color: control.handleColor
        border.width: 1
        border.color: control.handleBorderColor
        opacity: control.enabled ? 1 : 0.6
    }
}
