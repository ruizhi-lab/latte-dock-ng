/*
    SPDX-FileCopyrightText: 2017 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.2

import org.kde.plasma.private.volume 0.1

QtObject {
    id: pulseAudio

    signal streamsChanged

    // It's a JS object so we can do key lookup and don't need to take care of filtering duplicates.
    property var pidMatches: ({})

    property int maxVolumePercent: 125
    property int maxVolumeValue: Math.round(maxVolumePercent * PulseAudio.NormalVolume / 100.0)
    property int volumeStep: Math.round(5 * PulseAudio.NormalVolume / 100.0)

    function boundVolume(volume) {
        return Math.max(PulseAudio.MinimalVolume, Math.min(volume, maxVolumeValue));
    }

    // TODO Evict cache at some point, preferably if all instances of an application closed.
    function registerPidMatch(appName) {
        if (!hasPidMatch(appName)) {
            pidMatches[appName] = true;

            // In case this match is new, notify that streams might have changed.
            // This way we also catch the case when the non-playing instance
            // shows up first.
            // Only notify if we changed to avoid infinite recursion.
            streamsChanged();
        }
    }

    function hasPidMatch(appName) {
        return pidMatches[appName] === true;
    }

    function normalizeId(value) {
        if (!value || value === "") {
            return "";
        }

        var normalized = String(value).toLowerCase();

        var slash = normalized.lastIndexOf("/");
        if (slash !== -1) {
            normalized = normalized.substring(slash + 1);
        }

        var colon = normalized.lastIndexOf(":");
        if (colon !== -1) {
            normalized = normalized.substring(colon + 1);
        }

        if (normalized.endsWith(".desktop")) {
            normalized = normalized.substring(0, normalized.length - 8);
        }

        // Reverse-DNS app ids such as org.mozilla.firefox or com.google.Chrome
        // should be comparable with launcher ids like firefox / chrome.
        var dot = normalized.lastIndexOf(".");
        if (dot !== -1 && dot < (normalized.length - 1)) {
            normalized = normalized.substring(dot + 1);
        }

        return normalized;
    }

    function equivalentId(left, right) {
        if (!left || !right) {
            return false;
        }

        if (left === right) {
            return true;
        }

        // Handle distro-specific suffix variants such as foo vs foo-esr.
        return left.startsWith(right + "-") || right.startsWith(left + "-");
    }

    function hasAncestorPid(streamPid, pid) {
        var current = streamPid;
        var depth = 0;

        while (current > 1 && depth < 16) {
            current = backend.parentPid(current);

            if (current === pid) {
                return true;
            }

            ++depth;
        }

        return false;
    }

    function findStreams(key, value) {
        var streams = []
        for (var i = 0, length = instantiator.count; i < length; ++i) {
            var stream = instantiator.objectAt(i);
            if (stream[key] === value || (key==="appName" && stream[key].toLowerCase() === value.toLowerCase())) {
                streams.push(stream);
            }
        }
        return streams
    }

    function streamsForAppName(appName) {
        return findStreams("appName", appName);
    }

    function streamsForAppId(appId) {
        if (!appId || appId === "") {
            return [];
        }

        // Portal app id is the most reliable match for sandboxed/portal clients.
        var streams = findStreams("portalAppId", appId);

        if (streams.length === 0) {
            streams = findStreams("appId", appId);
        }

        return streams;
    }

    function streamsForPid(pid) {
        if (pid <= 0) {
            return [];
        }

        var streams = findStreams("pid", pid);

        if (streams.length === 0) {
            for (var i = 0, length = instantiator.count; i < length; ++i) {
                var stream = instantiator.objectAt(i);

                if (stream.parentPid === -1) {
                    stream.parentPid = backend.parentPid(stream.pid);
                }

                if (stream.parentPid === pid || hasAncestorPid(stream.parentPid, pid)) {
                    streams.push(stream);
                }
            }
        }

        return streams;
    }

    function streamsForAppIdentity(appId, launcherName, appName) {
        var idCandidates = [];
        var normalizedAppId = normalizeId(appId);
        var normalizedLauncherName = normalizeId(launcherName);
        var normalizedAppName = normalizeId(appName);

        if (normalizedAppId !== "") {
            idCandidates.push(normalizedAppId);
        }

        if (normalizedLauncherName !== "" && idCandidates.indexOf(normalizedLauncherName) === -1) {
            idCandidates.push(normalizedLauncherName);
        }

        if (normalizedAppName !== "" && idCandidates.indexOf(normalizedAppName) === -1) {
            idCandidates.push(normalizedAppName);
        }

        if (idCandidates.length === 0) {
            return [];
        }

        var streams = [];

        for (var i = 0, length = instantiator.count; i < length; ++i) {
            var stream = instantiator.objectAt(i);
            var streamCandidates = [
                normalizeId(stream.appId),
                normalizeId(stream.iconName),
                normalizeId(stream.binaryName),
                normalizeId(stream.appName)
            ];

            var matched = false;

            for (var c = 0; c < idCandidates.length && !matched; ++c) {
                var target = idCandidates[c];

                for (var s = 0; s < streamCandidates.length; ++s) {
                    if (equivalentId(target, streamCandidates[s])) {
                        matched = true;
                        break;
                    }
                }
            }

            if (matched) {
                streams.push(stream);
            }
        }

        return streams;
    }

    // QtObject has no default property, hence adding the Instantiator to one explicitly.
    property var instantiator: Instantiator {
        model: PulseObjectFilterModel {
            filters: [ { role: "VirtualStream", value: false } ]
            sourceModel: SinkInputModel {}
        }

        delegate: QtObject {
            readonly property int pid: Client ? Client.properties["application.process.id"] : 0
            // Determined on demand.
            property int parentPid: -1
            readonly property string appName: (Client && Client.properties && Client.properties["application.name"] !== undefined)
                                              ? String(Client.properties["application.name"]) : ""
            readonly property string appId: (Client && Client.properties && Client.properties["application.id"] !== undefined)
                                            ? String(Client.properties["application.id"]) : ""
            readonly property string portalAppId: (Client && Client.properties && Client.properties["pipewire.access.portal.app_id"] !== undefined)
                                                  ? String(Client.properties["pipewire.access.portal.app_id"]) : ""
            readonly property string iconName: (Client && Client.properties && Client.properties["application.icon_name"] !== undefined)
                                               ? String(Client.properties["application.icon_name"]) : ""
            readonly property string binaryName: (Client && Client.properties && Client.properties["application.process.binary"] !== undefined)
                                                 ? String(Client.properties["application.process.binary"]) : ""
            readonly property bool muted: Muted
            // whether there is nothing actually going on on that stream
            readonly property bool corked: Corked

            readonly property int volume: Math.round(pulseVolume / PulseAudio.NormalVolume * 100.0)
            readonly property int pulseVolume: Volume

            function mute() {
                Muted = true
            }
            function unmute() {
                Muted = false
            }

            function increaseVolume() {
                var bVolume = pulseAudio.boundVolume(Volume + pulseAudio.volumeStep);
                Volume = bVolume;
            }

            function decreaseVolume() {
                var bVolume = pulseAudio.boundVolume(Volume - pulseAudio.volumeStep);
                Volume = bVolume;
            }
        }

        onObjectAdded: pulseAudio.streamsChanged()
        onObjectRemoved: pulseAudio.streamsChanged()
    }

    Component.onCompleted: {
        console.log("PulseAudio Latte interface was loaded...");
    }
}
