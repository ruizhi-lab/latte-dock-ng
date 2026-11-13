/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick

Item {
    id: policy
    required property var configuration
    visible: false

    property date currentTime: new Date()
    readonly property int minuteOfDay: currentTime.getHours() * 60 + currentTime.getMinutes()
    readonly property int dayStart: Math.max(0, Math.min(1439, configuration.backgroundDayStart))
    readonly property int nightStart: Math.max(0, Math.min(1439, configuration.backgroundNightStart))
    readonly property bool isDay: dayStart === nightStart
                                  || (dayStart < nightStart ? minuteOfDay >= dayStart && minuteOfDay < nightStart
                                                           : minuteOfDay >= dayStart || minuteOfDay < nightStart)
    readonly property color backgroundColor: configuration.backgroundColorScheduleEnabled && !isDay
                                               ? configuration.customNightBackgroundColor : configuration.customBackgroundColor
    readonly property real brightness: (backgroundColor.r * 299 + backgroundColor.g * 587 + backgroundColor.b * 114) / 1000
    readonly property color textColor: brightness > 0.5 ? "#202020" : "#ffffff"

    // The persisted colors and local wall clock own the schedule. Resample wall
    // time instead of accumulating timer ticks so resume, clock and timezone
    // changes cannot leave the dock on yesterday's color. Equal boundaries mean all day.
    Timer {
        interval: 1000
        repeat: true
        triggeredOnStart: true
        running: policy.configuration.customBackgroundColorEnabled && policy.configuration.backgroundColorScheduleEnabled
        onTriggered: policy.currentTime = new Date()
    }
}
