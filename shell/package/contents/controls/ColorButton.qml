/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

QQC2.Button {
    id: control
    required property color selectedColor
    readonly property string colorName: String(control.selectedColor)
    Accessible.name: control.colorName
    // Native desktop styles can use negative content padding. Our swatch and
    // label need explicit insets, and the outer layout must not shrink below them.
    leftPadding: Kirigami.Units.smallSpacing * 2
    rightPadding: Kirigami.Units.smallSpacing * 2
    topPadding: Kirigami.Units.smallSpacing
    bottomPadding: Kirigami.Units.smallSpacing
    Layout.minimumWidth: implicitWidth
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                           contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                            contentItem.implicitHeight + topPadding + bottomPadding)

    // The desktop style also paints Button.text in its native background.
    // Keep it empty: this content item owns both swatch and label, otherwise
    // native text overlaps the label. Layout sizes reserve space for both.
    text: ""
    contentItem: RowLayout {
        spacing: Kirigami.Units.smallSpacing
        Rectangle {
            objectName: "colorSwatch"
            Layout.preferredWidth: Kirigami.Units.gridUnit * 1.5
            Layout.preferredHeight: Kirigami.Units.gridUnit
            Layout.alignment: Qt.AlignVCenter
            color: control.selectedColor
            border.color: control.palette.buttonText
        }
        QQC2.Label {
            objectName: "colorLabel"
            Layout.alignment: Qt.AlignVCenter
            Layout.minimumWidth: implicitWidth
            text: control.colorName
            font: control.font
            color: control.palette.buttonText
        }
    }
}
