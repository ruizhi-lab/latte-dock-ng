/*
    SPDX-FileCopyrightText: 2011 Daker Fernandes Pinheiro <dakerfp@gmail.com>
    SPDX-FileCopyrightText: 2011 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import org.kde.ksvg as KSvg

Item {
    id: main
    state: parent.state
    //used to tell apart this implementation with the touch components one
    property bool hasOverState: true
    property alias enabledBorders: shadow.enabledBorders

    KSvg.FrameSvgItem {
        id: hover

        anchors {
            fill: parent
            leftMargin: -margins.left
            topMargin: -margins.top
            rightMargin: -margins.right
            bottomMargin: -margins.bottom
        }
        opacity: 0
        imagePath: "widgets/button"
        prefix: "hover"
    }

    KSvg.FrameSvgItem {
        id: shadow

        anchors {
            fill: parent
            leftMargin: -margins.left
            topMargin: -margins.top
            rightMargin: -margins.right
            bottomMargin: -margins.bottom
        }
        imagePath: "widgets/button"
        prefix: "shadow"
    }

    states: [
        State {
            name: "shadow"
            PropertyChanges {
                shadow.opacity: 1
            }
            PropertyChanges {
                hover.opacity: 0
                hover.prefix: "hover"
            }
        },
        State {
            name: "hover"
            PropertyChanges {
                shadow.opacity: 0
            }
            PropertyChanges {
                hover.opacity: 1
                hover.prefix: "hover"
            }
        },
        State {
            name: "focus"
            PropertyChanges {
                shadow.opacity: 0
            }
            PropertyChanges {
                hover.opacity: 1
                hover.prefix: "focus"
            }
        },
        State {
            name: "hidden"
            PropertyChanges {
                shadow.opacity: 0
            }
            PropertyChanges {
                hover.opacity: 0
                hover.prefix: "hover"
            }
        }
    ]

    transitions: [
        Transition {
            PropertyAnimation {
                properties: "opacity"
                duration: units.longDuration
                easing.type: Easing.OutQuad
            }
        }
    ]
}
