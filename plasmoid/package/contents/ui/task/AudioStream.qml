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
    readonly property real compactBadgeSize: Math.min(iconBoxSize * 0.4, Kirigami.Units.iconSizes.smallMedium)
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
            if (taskItem.muted || taskItem.volume <= 0) {
                return "audio-volume-muted-symbolic" + (Qt.application.layoutDirection === Qt.RightToLeft ? "-rtl" : "");
            }

            return "audio-volume-high-symbolic" + (Qt.application.layoutDirection === Qt.RightToLeft ? "-rtl" : "");
        }

        height: Math.round(Math.min(parent.height * audioStreamIconBox.indicatorScale, Kirigami.Units.iconSizes.smallMedium))
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
                    width: audioStreamIconBox.roundedIconSize(Math.min(audioStreamIconBox.iconBoxSize, Kirigami.Units.iconSizes.smallMedium))
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
                    width: audioStreamIconBox.roundedIconSize(Math.min(audioStreamIconBox.iconBoxSize, Kirigami.Units.iconSizes.smallMedium))
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

        onPressed: function(mouse) {
            audioStreamIconBox.badgePressed = true;
        }

        onReleased: function(_mouse) {
            audioStreamIconBox.badgePressed = false;
        }

        onCanceled: {
            audioStreamIconBox.badgePressed = false;
        }

        onExited: {
            audioStreamIconBox.badgePressed = false;
        }

        onClicked: function(_mouse) {
            taskItem.toggleMuted();
        }

        onWheel: function(wheel) {
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

    // Limit wheel volume events for high-frequency touchpad scrolling.
    Timer {
        id: scrollDelayer
        interval: 80

        onTriggered: audioBadgeMouseArea.wheelIsBlocked = false;
    }
}
