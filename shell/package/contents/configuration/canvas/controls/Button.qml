/*
    SPDX-FileCopyrightText: 2019 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.7
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.15 as QQC2

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.kirigami 2.0 as Kirigami

Item{
    id: button
    readonly property var units: Kirigami.Units
    width: visibleButton.width
    height: visibleButton.height

    signal pressedChanged(bool pressed);

    property bool checked: false

    property bool iconPositionReversed: false
    property string text: "Default Text"
    property string tooltip: ""

    readonly property bool containsMouse: tooltipBtn.hovered
    readonly property int implicitHeight: visibleButton.height

    readonly property color appliedTextColor: checked ? checkedTextColor : textColor
    readonly property color appliedBackgroundColor: checked ? checkedBackgroundColor : backgroundColor
    readonly property color appliedBorderColor: checked ? checkedBorderColor : borderColor

    readonly property bool _hasColorizer: latteView && latteView.colorizer
    readonly property color _fallbackText: settingsRoot.textColor
    readonly property color _fallbackBg: "#2d2d2d"

    readonly property color textColor: containsMouse && _hasColorizer ? latteView.colorizer.buttonTextColor : _fallbackText
    readonly property color backgroundColor: containsMouse ? hoveredBackground :  normalBackground
    readonly property color borderColor: containsMouse ? hoveredBorder : normalBorder

    readonly property color checkedTextColor: _hasColorizer ? latteView.colorizer.buttonTextColor : _fallbackText
    readonly property color checkedBackgroundColor: _hasColorizer ? latteView.colorizer.buttonFocusColor : _fallbackBg
    readonly property color checkedBorderColor: hoveredBorder

    readonly property color normalBackground: _hasColorizer ? Qt.rgba(latteView.colorizer.buttonHoverColor.r,
                                                                       latteView.colorizer.buttonHoverColor.g,
                                                                       latteView.colorizer.buttonHoverColor.b,
                                                                       0.04) : _fallbackBg

    readonly property color hoveredBackground: _hasColorizer ? Qt.rgba(latteView.colorizer.buttonHoverColor.r,
                                                                        latteView.colorizer.buttonHoverColor.g,
                                                                        latteView.colorizer.buttonHoverColor.b,
                                                                        0.7) : _fallbackBg

    readonly property color normalBorder: Qt.rgba(settingsRoot.textColor.r,
                                                  settingsRoot.textColor.g,
                                                  settingsRoot.textColor.b,
                                                  0.7)

    readonly property color hoveredBorder: "#222222"

    property Component icon

    Item{
        id: visibleButtonRoot
        width: visibleButton.width
        height: visibleButton.height

        Rectangle {
            id: visibleButton
            width: buttonRow.width + 4 * margin
            height: buttonRow.height + 2 * margin
            radius: 2
            color: appliedBackgroundColor
            border.width: 1
            border.color: appliedBorderColor

            readonly property int margin: units.smallSpacing

            RowLayout{
                id: buttonRow
                anchors.centerIn: parent
                spacing: units.smallSpacing
                layoutDirection: iconPositionReversed ? Qt.RightToLeft : Qt.LeftToRight

                Loader {
                    width: height
                    height: textLbl.implicitHeight
                    active: button.icon
                    sourceComponent: button.icon
                    visible: active

                    readonly property color iconColor: button.appliedTextColor
                }

                PlasmaComponents.Label{
                    id: textLbl
                    text: button.text
                    color: button.appliedTextColor
                }
            }
        }
    }

    PlasmaComponents.Button {
        id: tooltipBtn
        anchors.fill: visibleButtonRoot
        opacity: 0
        Kirigami.Theme.inherit: true
        Kirigami.Theme.colorSet: Kirigami.Theme.Button
        palette.buttonText: button.appliedTextColor
        hoverEnabled: true

        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        QQC2.ToolTip.visible: hovered && button.tooltip !== ""
        QQC2.ToolTip.text: button.tooltip

        onPressedChanged: button.pressedChanged(pressed)
    }
}
