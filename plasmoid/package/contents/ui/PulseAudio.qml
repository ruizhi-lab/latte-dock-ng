/*
    SPDX-FileCopyrightText: 2017 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick

import org.kde.plasma.private.volume
import "../code/AudioStreamMatch.js" as AudioStreamMatch

QtObject {
    id: pulseAudio

    signal streamsChanged

    // The task model supplies all identities, including offscreen delegates.
    // The audio loader owns this observer and the stream objects; no task item
    // pointers or time-based ownership guesses survive model changes or unload.
    property var taskModel: null
    property var streamOwners: ({})
    property bool streamOwnersDirty: true
    property Timer streamOwnerRefreshTimer: Timer {
        interval: 0
        onTriggered: pulseAudio.streamsChanged()
    }
    function invalidateStreamOwners() {
        streamOwnersDirty = true;
        if (streamOwnerRefreshTimer) { streamOwnerRefreshTimer.restart(); }
    }
    property Instantiator taskIdentities: Instantiator {
        model: pulseAudio.taskModel
        delegate: QtObject {
            id: taskIdentity
            required property var model
            readonly property bool isWindow: model.IsWindow === true || model.IsGroupParent === true
            // Typed intermediates suppress unchanged role notifications; rebuilding
            // object snapshots on every activation would repeatedly walk /proc.
            readonly property int pid: Number(model.AppPid || 0)
            readonly property string appId: String(model.AppId || "")
            readonly property string appName: String(model.AppName || "")
            readonly property string launcherName: String(model.LauncherUrlWithoutIcon || model.LauncherUrl || "")
            readonly property var identity: ({pid: taskIdentity.pid, appId: taskIdentity.appId,
                appName: taskIdentity.appName, launcherName: taskIdentity.launcherName})
            onIdentityChanged: pulseAudio.invalidateStreamOwners()
            onIsWindowChanged: pulseAudio.invalidateStreamOwners()
        }
        onObjectAdded: pulseAudio.invalidateStreamOwners()
        onObjectRemoved: pulseAudio.invalidateStreamOwners()
    }

    property int maxVolumePercent: 125
    property int maxVolumeValue: Math.round(maxVolumePercent * PulseAudio.NormalVolume / 100.0)
    property int volumeStep: Math.round(5 * PulseAudio.NormalVolume / 100.0)
    property int bootstrapAttempts: 0
    readonly property int bootstrapMaxAttempts: 5

    function boundVolume(volume) {
        return Math.max(PulseAudio.MinimalVolume, Math.min(volume, maxVolumeValue));
    }

    function isStreamAssigned(stream) {
        for (var key in streamOwners) {
            if (streamOwners[key].indexOf(stream) !== -1) { return true; }
        }
        return false;
    }

    function streamsForTask(appId, launcherName, appName, pid) {
        if (streamOwnersDirty) {
            var tasks = [];
            var streams = [];
            for (var i = 0; i < taskIdentities.count; ++i) {
                var task = taskIdentities.objectAt(i);
                if (task && task.isWindow) { tasks.push(task.identity); }
            }
            for (var j = 0; j < instantiator.count; ++j) {
                var stream = instantiator.objectAt(j);
                if (stream) { streams.push(stream); }
            }
            streamOwners = AudioStreamMatch.assign(tasks, streams, function(processId) { return backend.parentPid(processId); });
            streamOwnersDirty = false;
        }
        var key = AudioStreamMatch.token({appId: appId, launcherName: launcherName, appName: appName, pid: pid});
        return streamOwners[key] ? streamOwners[key].slice() : [];
    }

    // Prime the SinkModel (output devices) so the Plasma volume applet gets
    // correct initial state when first added after a cold boot.  Without this
    // the sink subscription is only established on first consumer access,
    // which races with the async Plasma volume backend response. On modern
    // desktops this is commonly backed by PipeWire through pipewire-pulse,
    // while Plasma's private QML API still exposes PulseAudio-compatible names.
    // An Instantiator is needed to drive the model query — a bare  property var
    // creates the model but never triggers the initial data fetch.
    property Instantiator _sinkModelPrimer: Instantiator {
        model: SinkModel {}
        delegate: QtObject {}
    }

    // QtObject has no default property, hence adding the Instantiator to one explicitly.
    property Instantiator instantiator: Instantiator {
        model: PulseObjectFilterModel {
            filters: [ { role: "VirtualStream", value: false } ]
            sourceModel: SinkInputModel {}
        }

        delegate: QtObject {
            id: streamInfo
            // Older Plasma backends may omit per-stream Properties. Prefer
            // them when available, falling back to the client's shared metadata.
            readonly property var streamProperties: (typeof Properties !== "undefined" && Properties) ? Properties : ({})
            function metadata(key) {
                if (streamProperties[key] !== undefined && String(streamProperties[key]) !== "") {
                    return String(streamProperties[key]);
                }
                return Client && Client.properties && Client.properties[key] !== undefined ? String(Client.properties[key]) : "";
            }
            readonly property int pid: Number(metadata("application.process.id"))
            readonly property string appName: metadata("application.name")
            readonly property string appId: metadata("application.id")
            readonly property string portalAppId: metadata("pipewire.access.portal.app_id")
            readonly property string desktopId: metadata("application.desktop")
            readonly property string iconName: metadata("application.icon_name")
            readonly property string binaryName: metadata("application.process.binary")
            onPidChanged: pulseAudio.invalidateStreamOwners()
            onAppNameChanged: pulseAudio.invalidateStreamOwners()
            onAppIdChanged: pulseAudio.invalidateStreamOwners()
            onPortalAppIdChanged: pulseAudio.invalidateStreamOwners()
            onDesktopIdChanged: pulseAudio.invalidateStreamOwners()
            onIconNameChanged: pulseAudio.invalidateStreamOwners()
            onBinaryNameChanged: pulseAudio.invalidateStreamOwners()
            readonly property bool muted: Muted
            // whether there is nothing actually going on on that stream
            readonly property bool corked: Corked

            readonly property int volume: Math.round(pulseVolume / PulseAudio.NormalVolume * 100.0)
            readonly property int pulseVolume: Volume

            readonly property real activityThreshold: 0.00001
            property bool recentlyAudible: false
            readonly property bool activityAvailable: activityMonitor.available
            readonly property real peak: Math.max(0, activityMonitor.volume)
            property Timer silenceTimer: Timer {
                interval: 1000
                onTriggered: streamInfo.recentlyAudible = false
            }
            function clearActivity() {
                recentlyAudible = false;
                silenceTimer.stop();
            }
            function updateActivity() {
                if (corked || !activityMonitor.available) {
                    clearActivity();
                } else if (activityMonitor.volume > activityThreshold) {
                    recentlyAudible = true;
                    silenceTimer.stop();
                } else if (recentlyAudible) {
                    silenceTimer.restart();
                }
            }
            // One native peak monitor per assigned active output stream, never
            // per task window. Releasing target on cork/unload avoids keeping an
            // idle device awake; this observes output peaks, not microphone input.
            property VolumeMonitor activityMonitor: VolumeMonitor {
                target: !streamInfo.corked && pulseAudio.isStreamAssigned(streamInfo)
                        && typeof PulseObject !== "undefined" ? PulseObject : null
                onVolumeChanged: streamInfo.updateActivity()
                onAvailableChanged: { if (!available) { streamInfo.clearActivity(); } }
                onTargetChanged: {
                    // VolumeMonitor retains its previous sample across targets.
                    // Clear it so a new/reused stream cannot inherit an old peak.
                    volume = 0;
                    streamInfo.clearActivity();
                }
            }
            onCorkedChanged: { if (corked) { clearActivity(); } }

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

        onObjectAdded: {
            pulseAudio.invalidateStreamOwners();
        }
        onObjectRemoved: {
            // Drop removed stream references synchronously, then refresh once
            // more after the model has finished its removal notifications.
            pulseAudio.streamOwnersDirty = true;
            pulseAudio.streamsChanged();
            pulseAudio.invalidateStreamOwners();
        }
    }

    Component.onCompleted: {
        console.log("Plasma volume Latte interface was loaded...");
        bootstrapAttempts = 0;
        paFixTimer.start();
    }

    // ── Plasma volume PreferredDevice workaround ────────────────────────
    // PreferredDevice's constructor only connects to Server.defaultSinkChanged
    // — it never calls updatePreferredSink() initially.  If the Plasma volume
    // backend is already available when PreferredDevice is first created, the
    // signal never fires and PreferredDevice.sink stays null forever, causing
    // the volume applet to show a muted icon. PipeWire systems normally reach
    // this path through the pipewire-pulse compatibility service.
    //
    // We wait briefly for the Plasma volume context to be warm, then emit
    // defaultSinkChanged once on the Server singleton to force PreferredDevice
    // to call updatePreferredSink() and read the initial sink state.
    property Timer paFixTimer: Timer {
        interval: 1000
        repeat: true
        running: false
        onTriggered: {
            bootstrapAttempts++;

            try {
                var ds = Server.defaultSink
                if (ds && PreferredDevice) {
                    Server.defaultSinkChanged(ds)
                    paFixTimer.stop()
                    return;
                }
            } catch (e) {
                // Server/PreferredDevice not registered in this module version
            }

            if (bootstrapAttempts >= bootstrapMaxAttempts) {
                paFixTimer.stop();
            }
        }
    }
}
