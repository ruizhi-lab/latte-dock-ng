/*
    SPDX-FileCopyrightText: 2019 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0
import QtQuick.Shapes 1.0

import org.kde.plasma.components 2.0 as Components
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.plasmoid 2.0

Item{
    //   property string color
    id: glowItem

    property bool glow3D: true
    property bool roundCorners: true
    property bool showBorder: false
    property bool showAttention: false
    property bool showGlow: false

    property int animation: 250
    property int location: PlasmaCore.Types.BottomEdge
    property int size: 10

    property real glowOpacity: 0.75

    property color attentionColor: "red"
    property color basicColor: "blue"
    property color contrastColor: "#b0b0b0"
    property color currentColor: glowItem.showAttention ? animationColorAlpha : basicColorAlpha

    property color animationColor: "red" // it is used only internally for the animation
    readonly property color basicColorAlpha: Qt.rgba(basicColor.r, basicColor.g, basicColor.b, glowOpacity)
    readonly property color animationColorAlpha: Qt.rgba(animationColor.r, animationColor.g, animationColor.b, glowOpacity)
    readonly property color contrastColorAlpha: Qt.rgba(contrastColor.r, contrastColor.g, contrastColor.b, Math.min(glowOpacity+0.25,1))
    readonly property color contrastColorAlpha2: Qt.rgba(contrastColor.r, contrastColor.g, contrastColor.b, 0.3)

    readonly property bool isVertical: (location === PlasmaCore.Types.LeftEdge) || (location === PlasmaCore.Types.RightEdge)
    readonly property bool isHorizontal: !isVertical

    Grid{
        id: mainGlow
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        visible: glowItem.showGlow

        rows: isHorizontal ? 1 : 0
        columns: isVertical ? 1 : 0

        property int halfCorner: 3*glowItem.size
        property int fullCorner: 6*glowItem.size

        Item {
            id: firstGlowCorner
            width: isHorizontal ? mainGlow.halfCorner : mainGlow.fullCorner
            height: isHorizontal ? mainGlow.fullCorner : mainGlow.halfCorner
            clip: true

            Item {
                id: firstGlowCornerFull
                width: mainGlow.fullCorner
                height: mainGlow.fullCorner

                Shape {
                    id: firstCornerShape
                    anchors.fill: parent
                    ShapePath {
                        strokeWidth: 0
                        strokeColor: "transparent"
                        fillGradient: RadialGradient {
                            centerX: firstCornerShape.width / 2
                            centerY: firstCornerShape.height / 2
                            centerRadius: firstCornerShape.width / 2
                            focalX: firstCornerShape.width / 2
                            focalY: firstCornerShape.height / 2
                            GradientStop { position: 0.0; color: "transparent" }
                            GradientStop { position: 0.07; color: glowItem.contrastColorAlpha }
                            GradientStop { position: 0.125; color: glowItem.currentColor }
                            GradientStop { position: 0.4; color: "transparent" }
                            GradientStop { position: 1; color: "transparent" }
                        }
                        startX: 0; startY: 0
                        PathLine { x: firstCornerShape.width; y: 0 }
                        PathLine { x: firstCornerShape.width; y: firstCornerShape.height }
                        PathLine { x: 0; y: firstCornerShape.height }
                        PathLine { x: 0; y: 0 }
                    }
                }

                states: [
                    State{
                        name: "*"
                        when:  isHorizontal

                        AnchorChanges{
                            target:firstGlowCornerFull;
                            anchors{ bottom: undefined; left:parent.left;}
                        }
                    },
                    State{
                        name: "vertical"
                        when:  isVertical

                        AnchorChanges{
                            target:firstGlowCornerFull;
                            anchors{ top: parent.top; left:undefined;}
                        }
                    }
                ]
            }
        }

        Item {
            id:mainGlowPart
            width: isHorizontal ? glowItem.width - glowItem.size : mainGlow.fullCorner
            height: isHorizontal ? mainGlow.fullCorner : glowItem.height - glowItem.size

            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    orientation: isHorizontal ? Gradient.Vertical : Gradient.Horizontal
                    GradientStop { position: 0.0; color: "transparent" }
                    GradientStop { position: 0.08; color: "transparent" }
                    GradientStop { position: 0.37; color: glowItem.currentColor }
                    GradientStop { position: 0.43; color: glowItem.contrastColorAlpha }
                    GradientStop { position: 0.57; color: glowItem.contrastColorAlpha }
                    GradientStop { position: 0.63; color: glowItem.currentColor }
                    GradientStop { position: 0.92; color: "transparent" }
                    GradientStop { position: 1; color: "transparent" }
                }
            }
        }

        Item {
            id:lastGlowCorner
            width: isHorizontal ? mainGlow.halfCorner : mainGlow.fullCorner
            height: isHorizontal ? mainGlow.fullCorner : mainGlow.halfCorner
            clip: true

            Item {
                id: lastGlowCornerFull
                width: mainGlow.fullCorner
                height: mainGlow.fullCorner

                Shape {
                    id: lastCornerShape
                    anchors.fill: parent
                    ShapePath {
                        strokeWidth: 0
                        strokeColor: "transparent"
                        fillGradient: RadialGradient {
                            centerX: lastCornerShape.width / 2
                            centerY: lastCornerShape.height / 2
                            centerRadius: lastCornerShape.width / 2
                            focalX: lastCornerShape.width / 2
                            focalY: lastCornerShape.height / 2
                            GradientStop { position: 0.0; color: "transparent" }
                            GradientStop { position: 0.07; color: glowItem.contrastColorAlpha }
                            GradientStop { position: 0.125; color: glowItem.currentColor }
                            GradientStop { position: 0.4; color: "transparent" }
                            GradientStop { position: 1; color: "transparent" }
                        }
                        startX: 0; startY: 0
                        PathLine { x: lastCornerShape.width; y: 0 }
                        PathLine { x: lastCornerShape.width; y: lastCornerShape.height }
                        PathLine { x: 0; y: lastCornerShape.height }
                        PathLine { x: 0; y: 0 }
                    }
                }

                states: [
                    State{
                        name: "*"
                        when:  isHorizontal

                        AnchorChanges{
                            target:lastGlowCornerFull;
                            anchors{ bottom: undefined; right:parent.right;}
                        }
                    },
                    State{
                        name: "vertical"
                        when:  isVertical

                        AnchorChanges{
                            target:lastGlowCornerFull;
                            anchors{ bottom: parent.bottom; right:undefined;}
                        }
                    }
                ]
            }
        }
    }

    //! add border around indicator without reducing its size
    Loader{
        anchors.centerIn: mainElement
        active: glowItem.showBorder

        sourceComponent:Rectangle {
            width: mainElement.width + size
            height: mainElement.height + size
            anchors.centerIn: parent

            color: contrastColorAlpha2
            radius: glowItem.roundCorners ? Math.min(width,height) / 2 : 0

            property int size: Math.min(2*Math.max(1,mainElement.width/5 ),
                                        2*Math.max(1,mainElement.height/5 ))
        }
    }

    Item{
        id:mainElement

        width: Math.max(glowItem.size, parent.width)
        height: Math.max(glowItem.size, parent.height)
        anchors.centerIn: parent

        Rectangle {
            id: smallCircle
            anchors.centerIn: parent
            anchors.fill: parent

            color: glowItem.basicColor
            radius: glowItem.roundCorners ? Math.min(width,height) / 2 : 0
            visible: !glowItem.showAttention
        }

        Loader{
            anchors.centerIn: parent
            anchors.fill: parent

            active: glowItem.showAttention

            sourceComponent:Rectangle {
                id: smallCircleInAttention

                color: glowItem.animationColor
                radius: smallCircle.radius

                SequentialAnimation{
                    running: glowItem.showAttention
                    loops: Animation.Infinite
                    alwaysRunToEnd: true

                    PropertyAnimation {
                        target: glowItem
                        property: "animationColor"
                        to: glowItem.animationColor
                        duration: glowItem.animation
                        easing.type: Easing.InOutQuad
                    }

                    PropertyAnimation {
                        target: glowItem
                        property: "animationColor"
                        to: glowItem.basicColor
                        duration: glowItem.animation
                        easing.type: Easing.InOutQuad
                    }
                }
            }
        }

        Rectangle {
            visible: glowItem.showGlow && glowItem.glow3D
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter

            anchors.horizontalCenterOffset: {
                if (isHorizontal)
                    return 0;
                else if (location === PlasmaCore.Types.LeftEdge)
                    return -glowItem.width / 7;
                else if (location === PlasmaCore.Types.RightEdge)
                    return glowItem.width / 7;

                return 0;
            }
            anchors.verticalCenterOffset: {
                if (isVertical)
                    return 0;
                else if (location === PlasmaCore.Types.BottomEdge)
                    return glowItem.height / 7;
                else if (location === PlasmaCore.Types.TopEdge)
                    return -glowItem.height / 7;

                return 0;
            }

            width: isHorizontal ? Math.max(mainGlowPart.width, shadow) : shadow
            height: isHorizontal ? shadow : Math.max(mainGlowPart.height, shadow)
            radius: isHorizontal ? height/2 : width/2

            property int shadow: glowItem.size / 3

            color: glowItem.contrastColorAlpha
            opacity: 0.2
        }
    }
}
