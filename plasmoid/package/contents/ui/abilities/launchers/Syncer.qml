/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick

Item {
    id:_syncer
    readonly property var launcherClient: _launchers
    property bool isBlocked: false
    readonly property bool isActive: bridge !== null && bridge.launchers.host !==null
    readonly property int clientId: plasmoid.id
    // used to identify launchers that need to be synced event though their launchers group type
    // does not support it e.g. taskmanagers synced in different screens
    readonly property string syncedGroupId: _launchers.groupId

    //! Connections
    Component.onCompleted: {
        if (isActive) {
            bridge.launchers.host.addAbilityClient(_syncer);
        }
    }

    Component.onDestruction: {
        if (bridge) {
            bridge.launchers.host.removeAbilityClient(_syncer);
        }
    }

    onIsActiveChanged: {
        if (isActive) {
            bridge.launchers.host.addAbilityClient(_syncer);
        } else if (bridge) {
            bridge.launchers.host.removeAbilityClient(_syncer);
        }
    }

    Connections {
        target: isActive ? bridge.launchers.host : null
        function onIsReadyChanged() {
            if (bridge.launchers.host.isReady && _syncer.isActive) {
                bridge.launchers.host.addAbilityClient(_syncer);
            }
        }
    }

    //! All following actions are triggered from Main SyncedLaunchers handler
    function addSyncedLauncher(group, launcher) {
        if (group === _syncer.launcherClient.group && _syncer.launcherClient.addLauncherToModel(launcher)) {
            _syncer.launcherClient.launcherChanged(launcher);
            tasksModel.syncLaunchers();
        }
    }

    function addSyncedLauncherAtPosition(group, launcher, position) {
        if (group === _syncer.launcherClient.group) {
            _syncer.launcherClient.launcherInMoving(launcher, Math.max(0, position));
            // The positioning timer commits the order after the move.
            // Syncing the temporary append order here can reset live delegates.
            if (_syncer.launcherClient.addLauncherToModel(launcher)) {
                _syncer.launcherClient.launcherChanged(launcher);
            }
        }
    }

    function removeSyncedLauncher(group, launcher) {
        if (group === _launchers.group) {
            _launchers.launcherInRemoving(launcher)
            tasksModel.requestRemoveLauncher(launcher);
            _launchers.launcherChanged(launcher);
            tasksModel.syncLaunchers();
        }
    }

    function addSyncedLauncherToActivity(group, launcher, activity) {
        if (group === _launchers.group) {
            if (activity !== activityInfo.currentActivity && _launchers.isOnAllActivities(launcher)) {
                _launchers.launcherInRemoving(launcher);
            }

            tasksModel.requestAddLauncherToActivity(launcher, activity);
            _launchers.launcherChanged(launcher);
            tasksModel.syncLaunchers();
        }
    }

    function removeSyncedLauncherFromActivity(group, launcher, activity) {
        if (group === _launchers.group) {
            if (activity === activityInfo.currentActivity) {
                _launchers.launcherInRemoving(launcher)
            }

            tasksModel.requestRemoveLauncherFromActivity(launcher, activity);
            _launchers.launcherChanged(launcher);
            tasksModel.syncLaunchers();
        }
    }

    function dropSyncedUrls(group, urls) {
        if (group === _launchers.group) {
            urls.forEach(function (item) {
                _launchers.addDroppedLauncher(item);
            });
        }
    }

    function validateSyncedLaunchersOrder(group, orderedLaunchers) {
        if (group === _launchers.group && !isBlocked) {
            validator.validateOrder(orderedLaunchers);
        }
    }

}
