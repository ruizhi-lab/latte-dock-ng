/*
    SPDX-FileCopyrightText: 2017 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kirigami 2.0 as Kirigami

Item {
    id: audioStreamIconBox

    readonly property bool activeAudioState: taskItem.playingAudio || taskItem.muted
    readonly property real indicatorScale: 1.2
    readonly property real iconBoxWidth: parent ? parent.width : 0
    readonly property real iconBoxHeight: parent ? parent.height : 0
    readonly property real iconBoxSize: Math.min(iconBoxWidth, iconBoxHeight)
    // iconBoxSize already reflects the zoomed parent size — don't
    // multiply by parabolicZoom again, which causes the badge to
    // distort (double-scaling) during zoom animation.
    readonly property real maximumBadgeSize: Kirigami.Units.iconSizes.smallMedium
    readonly property real compactBadgeSize: Math.min(iconBoxSize * 0.4, maximumBadgeSize)
    readonly property int outerMargin: Math.round(Math.max(1, iconBoxSize * 0.04))
    readonly property real requiredSpace: iconBoxSize + compactBadgeSize * 2
    readonly property bool badgeHovered: {
        if (!root.audioBadgeActionsEnabled || !audioStreamIconBox.visible) {
            return false;
        }

        if (!taskItem || !taskItem.mouseArea || !taskItem.mouseArea.containsMouse) {
            return false;
        }

        var pos = audioStreamIconBox.mapFromItem(taskItem.mouseArea,
                                                 taskItem.mouseArea.mouseX,
                                                 taskItem.mouseArea.mouseY);

        return pos.x >= -audioStreamIconBox.hitPadding
                && pos.x <= audioStreamIconBox.width + audioStreamIconBox.hitPadding
                && pos.y >= -audioStreamIconBox.hitPadding
                && pos.y <= audioStreamIconBox.height + audioStreamIconBox.hitPadding;
    }
    readonly property int hitPadding: Math.max(4, Math.round(width * 0.12))
    property bool badgePressed: false
    property bool showVolumeLevel: false

    function flashVolumeLevel() {
        showVolumeLevel = true;
        volumeLevelTimer.restart();
    }

    onBadgeHoveredChanged: {
        if (!badgeHovered) {
            badgePressed = false;
        }
    }

    function roundedIconSize(size) {
        if (size < 16) {
            return size;
        }

        if (size < 22) {
            return 16;
        }

        if (size < 32) {
            return 22;
        }

        if (size < 48) {
            return 32;
        }

        if (size < 64) {
            return 48;
        }

        return size;
    }

    z: 200
    width: Math.max(1, Math.round(compactBadgeSize))
    height: width

    anchors.top: parent.top
    anchors.topMargin: outerMargin
    anchors.right: parent.right
    anchors.rightMargin: outerMargin

    opacity: activeAudioState ? 1 : 0
    visible: opacity > 0

    Behavior on opacity {
        NumberAnimation {
            duration: Kirigami.Units.longDuration
        }
    }

    PlasmaExtras.Highlight {
        anchors.fill: audioStreamIcon
        hovered: audioStreamIconBox.badgeHovered
        pressed: audioStreamIconBox.badgePressed
    }

    Kirigami.Icon {
        id: audioStreamIcon

        readonly property real requiredSpace: audioStreamIconBox.requiredSpace

        source: {
            if (taskItem.hasAudioStream && (taskItem.muted || taskItem.volume <= 0)) {
                return "audio-volume-muted-symbolic" + (Qt.application.layoutDirection === Qt.RightToLeft ? "-rtl" : "");
            }

            return "audio-volume-high-symbolic" + (Qt.application.layoutDirection === Qt.RightToLeft ? "-rtl" : "");
        }

        // Use the containment-level colorizer palette so the audio badge
        // follows the user's colour palette choice (Plasma Theme / Dark /
        // Light / Reverse / etc.) rather than the global Kirigami theme.
        isMask: true
        readonly property color contrastColor: (typeof latteBridge !== "undefined" && latteBridge && latteBridge.palette)
                                               ? latteBridge.palette.textColor
                                               : Kirigami.Theme.textColor
        color: contrastColor

        height: Math.round(Math.min(parent.height * audioStreamIconBox.indicatorScale, audioStreamIconBox.maximumBadgeSize))
        width: height

        anchors {
            verticalCenter: parent.verticalCenter
            horizontalCenter: parent.horizontalCenter
        }

        states: [
            State {
                name: "verticalIconsOnly"
                when: root.vertical && audioStreamIconBox.iconBoxWidth < audioStreamIcon.requiredSpace

                PropertyChanges {
                    target: audioStreamIconBox
                    anchors.rightMargin: Math.round(audioStreamIconBox.outerMargin * audioStreamIconBox.indicatorScale)
                }
            },

            State {
                name: "horizontal"
                when: !root.vertical && audioStreamIconBox.iconBoxWidth > audioStreamIcon.requiredSpace

                AnchorChanges {
                    target: audioStreamIconBox
                    anchors.top: undefined
                    anchors.verticalCenter: parent.verticalCenter
                }

                PropertyChanges {
                    target: audioStreamIconBox
                    width: audioStreamIconBox.roundedIconSize(Math.min(audioStreamIconBox.iconBoxSize, audioStreamIconBox.maximumBadgeSize))
                }

                PropertyChanges {
                    target: audioStreamIcon
                    height: audioStreamIcon.parent.height
                    width: audioStreamIcon.parent.width
                }
            },

            State {
                name: "vertical"
                when: root.vertical && audioStreamIconBox.iconBoxHeight > audioStreamIcon.requiredSpace

                AnchorChanges {
                    target: audioStreamIconBox
                    anchors.right: undefined
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                PropertyChanges {
                    target: audioStreamIconBox
                    anchors.topMargin: audioStreamIconBox.outerMargin
                    width: audioStreamIconBox.roundedIconSize(Math.min(audioStreamIconBox.iconBoxSize, audioStreamIconBox.maximumBadgeSize))
                }

                PropertyChanges {
                    target: audioStreamIcon
                    height: audioStreamIcon.parent.height
                    width: audioStreamIcon.parent.width
                }
            }
        ]

        active: audioStreamIconBox.badgeHovered
        selected: audioStreamIconBox.badgePressed
    }

    MouseArea {
        id: audioBadgeMouseArea
        anchors.fill: parent
        anchors.margins: -audioStreamIconBox.hitPadding
        acceptedButtons: Qt.LeftButton
        hoverEnabled: true
        enabled: root.audioBadgeActionsEnabled
        preventStealing: true
        property bool wheelIsBlocked: false

        onPressed: (mouse) => {
            audioStreamIconBox.badgePressed = true;
        }

        onReleased: (_mouse) => {
            audioStreamIconBox.badgePressed = false;
        }

        onCanceled: {
            audioStreamIconBox.badgePressed = false;
        }

        onExited: {
            audioStreamIconBox.badgePressed = false;
        }

        onClicked: (_mouse) => {
            taskItem.toggleMuted();
        }

        onWheel: (wheel) => {
            wheel.accepted = true;
            if (wheelIsBlocked) {
                return;
            }

            wheelIsBlocked = true;
            scrollDelayer.start();

            var angle = wheel.angleDelta.y / 8;

            if (angle > 2) {
                taskItem.increaseVolume();
            } else if (angle < -2) {
                taskItem.decreaseVolume();
            }
        }
    }

    // Volume level indicator — bar + percentage label above the badge.
    // Appears briefly when volume is adjusted via mouse wheel.
    Item {
        id: volumeLevelIndicator
        anchors {
            bottom: audioStreamIcon.top
            bottomMargin: 3
            horizontalCenter: audioStreamIcon.horizontalCenter
        }
        width: Math.max(36, audioStreamIconBox.maximumBadgeSize * 2)
        height: childrenRect.height
        opacity: showVolumeLevel ? 1 : 0
        visible: opacity > 0

        Behavior on opacity {
            NumberAnimation { duration: 200 }
        }

        // Volume level text (e.g. "75%")
        Text {
            id: volumeLabel
            anchors {
                horizontalCenter: parent.horizontalCenter
            }
            text: Math.round(Math.min(taskItem.volume, 100)) + "%"
            color: audioStreamIcon.contrastColor
            font.pixelSize: Math.max(10, Math.round(audioStreamIconBox.height * 0.38))
            font.weight: Font.DemiBold
        }

        // Volume bar (background + fill)
        Rectangle {
            id: volumeBar
            anchors {
                top: volumeLabel.bottom
                topMargin: 1
                left: parent.left
                right: parent.right
            }
            height: Math.max(3, Math.round(audioStreamIcon.height * 0.18))
            radius: height / 2
            color: audioStreamIcon.contrastColor
            opacity: 0.25

            // Volume bar fill — clipped to the parent radius via clip.
            // Using a child rectangle ensures the fill is always positioned
            // correctly regardless of binding evaluation order.
            clip: true
            Rectangle {
                anchors {
                    left: parent.left
                    top: parent.top
                    bottom: parent.bottom
                }
                width: Math.round(parent.width * Math.min(taskItem.volume, 100) / 100)
                color: audioStreamIcon.contrastColor
            }
        }
    }

    Timer {
        id: volumeLevelTimer
        interval: 1500
        repeat: false
        onTriggered: audioStreamIconBox.showVolumeLevel = false
    }

    // Limit wheel volume events for high-frequency touchpad scrolling.
    Timer {
        id: scrollDelayer
        interval: 80

        onTriggered: audioBadgeMouseArea.wheelIsBlocked = false;
    }
}
