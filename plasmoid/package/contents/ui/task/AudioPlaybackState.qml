/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQml

QtObject {
    id: state
    required property var streams
    property bool enabled: true
    property bool playerPlaying: false
    property bool playerPaused: false
    readonly property real activityThreshold: 0.00001
    readonly property bool hasStreams: enabled && streams.length > 0
    readonly property bool muted: hasStreams && streams.every(function(stream) { return stream && stream.muted; })
    readonly property bool playing: hasStreams && streams.some(function(stream) {
        if (!stream || stream.corked) { return false; }
        // An explicit playing session preserves the badge through quiet passages.
        // Paused sessions must not inherit a retained/muted audio connection.
        if (state.playerPlaying) { return true; }
        if (stream.muted || stream.volume <= 0) { return !state.playerPaused; }
        if (stream.activityAvailable) {
            return state.playerPaused ? stream.peak > state.activityThreshold : stream.recentlyAudible;
        }
        // Older backends without an available monitor retain the corked fallback,
        // with an explicit media-player pause taking precedence over connection life.
        return !state.playerPaused;
    })
}
