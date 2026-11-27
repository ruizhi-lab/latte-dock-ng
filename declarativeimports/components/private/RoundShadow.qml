/*
    SPDX-FileCopyrightText: 2019 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import org.kde.ksvg as KSvg

Item {
    id: main
    state: parent.state
    property alias imagePath: shadowSvg.imagePath
    property string hoverElement: "hover"
    property string focusElement: "focus"
    property alias shadowElement: shadow.elementId

    //used to tell apart this implementation with the touch components one
    property bool hasOverState: true

    KSvg.Svg {
        id: shadowSvg
        imagePath: "widgets/actionbutton"
    }

    KSvg.SvgItem {
        id: hover
        svg: shadowSvg
        elementId: "hover"

        anchors.fill: parent

        opacity: 0
    }

    KSvg.SvgItem {
        id: shadow
        svg: shadowSvg
        elementId: "shadow"

        anchors.fill: parent
    }

    states: [
        State {
            name: "shadow"
            PropertyChanges {
                shadow.opacity: 1
            }
            PropertyChanges {
                hover.opacity: 0
                hover.elementId: hoverElement
            }
        },
        State {
            name: "hover"
            PropertyChanges {
                shadow.opacity: 0
            }
            PropertyChanges {
                hover.opacity: 1
                hover.elementId: hoverElement
            }
        },
        State {
            name: "focus"
            PropertyChanges {
                shadow.opacity: 0
            }
            PropertyChanges {
                hover.opacity: 1
                hover.elementId: focusElement
            }
        },
        State {
            name: "hidden"
            PropertyChanges {
                shadow.opacity: 0
            }
            PropertyChanges {
                hover.opacity: 0
                hover.elementId: hoverElement
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
