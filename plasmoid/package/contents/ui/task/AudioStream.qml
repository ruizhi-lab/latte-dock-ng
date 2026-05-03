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

    readonly property int outerMargin: Math.round(Math.max(1, Math.min(parent.width, parent.height) * 0.04))
    readonly property bool activeAudioState: taskItem.playingAudio || taskItem.muted

    z: 200
    width: Math.round(Math.max(18, Math.min(Math.min(parent.width, parent.height) * 0.42, Kirigami.Units.iconSizes.medium)))
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
        hovered: audioBadgeHover.hovered
        pressed: audioBadgeTap.pressed
    }

    Kirigami.Icon {
        id: audioStreamIcon
        anchors.fill: parent
        source: {
            if (taskItem.muted || taskItem.volume <= 0) {
                return "audio-volume-muted-symbolic" + (Qt.application.layoutDirection === Qt.RightToLeft ? "-rtl" : "");
            }

            return "audio-volume-high-symbolic" + (Qt.application.layoutDirection === Qt.RightToLeft ? "-rtl" : "");
        }
        active: audioBadgeHover.hovered
        selected: audioBadgeTap.pressed
    }

    HoverHandler {
        id: audioBadgeHover
        enabled: root.audioBadgeActionsEnabled
    }

    TapHandler {
        id: audioBadgeTap
        enabled: root.audioBadgeActionsEnabled
        gesturePolicy: TapHandler.ReleaseWithinBounds

        onTapped: function(_eventPoint, _button) {
            taskItem.toggleMuted();
        }
    }

    MouseArea {
        id: audioBadgeWheelArea
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        enabled: root.audioBadgeActionsEnabled

        property bool wheelIsBlocked: false

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

        // Limit wheel volume events for high-frequency touchpad scrolling.
        Timer {
            id: scrollDelayer
            interval: 80

            onTriggered: audioBadgeWheelArea.wheelIsBlocked = false;
        }
    }
}
