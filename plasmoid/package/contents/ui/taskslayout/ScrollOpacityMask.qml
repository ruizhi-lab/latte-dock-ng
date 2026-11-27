/*
    SPDX-FileCopyrightText: 2019 Michail Vourlakos <mvourlakos@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import org.kde.plasma.core as PlasmaCore

Item {
    // MultiEffect samples maskSource directly as a texture. Without a layer,
    // this composite of rectangles has no texture and overflowing tasks vanish
    // as soon as hover zoom enables the scroll mask.
    layer.enabled: true

    readonly property int gradientLength: appletAbilities.metrics.iconSize / 3

    // Zoom changes the remaining scroll distance each frame. Fade only as
    // much as that distance warrants so a one-pixel overflow cannot flash
    // a full-strength shadow over the last task.
    function edgeAlpha(hiddenLength: real): real {
        return 1 - Math.min(1, Math.max(0, hiddenLength) / Math.max(1, gradientLength));
    }

    Rectangle {
        id: firstGradient
        width: !root.vertical ? gradientLength : scrollableList.thickness
        height: !root.vertical ? scrollableList.thickness : gradientLength

        gradient: Gradient {
            orientation: !root.vertical ? Gradient.Horizontal : Gradient.Vertical
            GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, edgeAlpha(scrollableList.currentPos - scrollableList.scrollFirstPos)) }
            GradientStop { position: 1.0; color: "white" }
        }
    }

    Rectangle {
        id: centralRectangle
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        width: !root.vertical ? length : firstGradient.width
        height: !root.vertical ? firstGradient.height : length
        color: "white"

        property int length: !root.vertical ? parent.width - 2*gradientLength : parent.height - 2*gradientLength
    }

    Rectangle {
        id: lastGradient
        anchors.top: parent.top
        anchors.right: parent.right
        width: firstGradient.width
        height: firstGradient.height

        gradient: Gradient {
            orientation: !root.vertical ? Gradient.Horizontal : Gradient.Vertical
            GradientStop { position: 0.0; color: "white" }
            GradientStop { position: 1.0; color: Qt.rgba(1, 1, 1, edgeAlpha(scrollableList.scrollLastPos - scrollableList.currentPos)) }
        }
    }

    states: [
        State {
            name: "bottom"
            when: root.location === PlasmaCore.Types.BottomEdge

            AnchorChanges {
                target: firstGradient
                anchors{ top:undefined; bottom:parent.bottom; left:parent.left; right:undefined;
                    horizontalCenter:undefined; verticalCenter:undefined}
            }
            AnchorChanges {
                target: centralRectangle
                anchors{ top:undefined; bottom:parent.bottom; left:undefined; right:undefined;
                    horizontalCenter:parent.horizontalCenter; verticalCenter:undefined}
            }
            AnchorChanges {
                target: lastGradient
                anchors{ top:undefined; bottom:parent.bottom; left:undefined; right:parent.right;
                    horizontalCenter:undefined; verticalCenter:undefined}
            }
        },
        State {
            name: "top"
            when: root.location === PlasmaCore.Types.TopEdge

            AnchorChanges {
                target: firstGradient
                anchors{ top:parent.top; bottom:undefined; left:parent.left; right:undefined;
                    horizontalCenter:undefined; verticalCenter:undefined}
            }
            AnchorChanges {
                target: centralRectangle
                anchors{ top:parent.top; bottom:undefined; left:undefined; right:undefined;
                    horizontalCenter:parent.horizontalCenter; verticalCenter:undefined}
            }
            AnchorChanges {
                target: lastGradient
                anchors{ top:parent.top; bottom:undefined; left:undefined; right:parent.right;
                    horizontalCenter:undefined; verticalCenter:undefined}
            }
        },
        State {
            name: "left"
            when: root.location === PlasmaCore.Types.LeftEdge

            AnchorChanges {
                target: firstGradient
                anchors{ top:parent.top; bottom:undefined; left:parent.left; right:undefined;
                    horizontalCenter:undefined; verticalCenter:undefined}
            }
            AnchorChanges {
                target: centralRectangle
                anchors{ top:undefined; bottom:undefined; left:parent.left; right:undefined;
                    horizontalCenter:undefined; verticalCenter:parent.verticalCenter}
            }
            AnchorChanges {
                target: lastGradient
                anchors{ top:undefined; bottom:parent.bottom; left:parent.left; right:undefined;
                    horizontalCenter:undefined; verticalCenter:undefined}
            }
        },
        State {
            name: "right"
            when: root.location === PlasmaCore.Types.RightEdge

            AnchorChanges {
                target: firstGradient
                anchors{ top:parent.top; bottom:undefined; left:undefined; right:parent.right;
                    horizontalCenter:undefined; verticalCenter:undefined}
            }
            AnchorChanges {
                target: centralRectangle
                anchors{ top:undefined; bottom:undefined; left:undefined; right:parent.right;
                    horizontalCenter:undefined; verticalCenter:parent.verticalCenter}
            }
            AnchorChanges {
                target: lastGradient
                anchors{ top:undefined; bottom:parent.bottom; left:undefined; right:parent.right;
                    horizontalCenter:undefined; verticalCenter:undefined}
            }
        }
    ]
}
