/*
    SPDX-FileCopyrightText: 2019 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2

import org.kde.kirigami 2.0 as Kirigami

QQC2.TextField {
    id: textField
    Kirigami.Theme.inherit: true

    palette.base: Kirigami.Theme.backgroundColor
    palette.text: Kirigami.Theme.textColor
    palette.button: Kirigami.Theme.backgroundColor
    palette.buttonText: Kirigami.Theme.textColor
    palette.highlight: Kirigami.Theme.highlightColor
    palette.highlightedText: Kirigami.Theme.highlightedTextColor

    validator: IntValidator {
        bottom: minValue
        top: maxValue
    }

    onTextChanged: {
        if (text.trim() === minValue.toString())
            text = ""
    }

    font.italic: true
    inputMethodHints: Qt.ImhDigitsOnly
    placeholderText: i18n("none")
    horizontalAlignment: Text.AlignLeft

    // implicitWidth is FINAL on T.TextField in Qt 6; use a binding instead of
    // shadowing with `readonly property`. Use Kirigami.Units.gridUnit as the
    // Plasma 6 replacement for the removed theme.mSize() helper.
    implicitWidth: internalContent.implicitWidth + Kirigami.Units.gridUnit * 6
    rightPadding: internalContent.implicitWidth + Kirigami.Units.smallSpacing

    readonly property int value: text === "" ? minValue : parseInt(text)
    property int step: 100
    property int minValue: 0
    property int maxValue: 3000

    function increment() {
        var val = text === "" ? minValue : parseInt(text)
        text = Math.min(val + step, maxValue).toString()
    }

    function decrement() {
        var val = text === "" ? minValue : parseInt(text)
        val = Math.max(val - step, minValue)
        text = val === minValue ? "" : val.toString()
    }

    RowLayout {
        id: internalContent
        spacing: 0
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right

        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        QQC2.Label {
            Layout.alignment: Qt.AlignVCenter
            text: i18n("ms.")
            font.italic: true
            opacity: value === 0 ? 0 : 0.6
        }
        QQC2.Button {
            id: downButton
            Kirigami.Theme.inherit: true

            Layout.fillHeight: true
            Layout.preferredWidth: height
            Layout.maximumWidth: height
            Layout.leftMargin: Qt.application.layoutDirection === Qt.RightToLeft ? 0 : 0.7 * Kirigami.Units.gridUnit
            Layout.rightMargin: Qt.application.layoutDirection === Qt.RightToLeft ? 0.7 * Kirigami.Units.gridUnit : 0

            text: "-"
            onClicked: decrement()
        }
        QQC2.Button {
            id: upButton
            Kirigami.Theme.inherit: true

            Layout.fillHeight: true
            Layout.preferredWidth: height
            Layout.maximumWidth: height
            text: "+"
            onClicked: increment()
        }
    }

    Timer {
        id: holdPressed
        running: upButton.pressed || downButton.pressed
        interval: 200
        repeat: true

        onRunningChanged: {
            if (!running)
                interval = 200
        }

        onTriggered: {
            if (interval === 200)
                interval = 150
            else if (upButton.pressed)
                increment()
            else
                decrement()
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.MiddleButton

        onWheel: {
            var angle = wheel.angleDelta.y / 8

            if (angle > 0) {
                increment()
            } else if (angle < 0) {
                decrement()
            }
        }
    }
}
