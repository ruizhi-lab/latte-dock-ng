/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0

import org.kde.plasma.plasmoid 2.0

import org.kde.latte.core 0.2 as LatteCore

import "launchers" as LaunchersPart

Item {
    id: _launchers
    readonly property bool isActive: bridge !== null

    signal launcherChanged(string launcherUrl);
    signal launcherRemoved(string launcherUrl);

    //! triggered just before action happens. They are used mostly for animation purposes
    signal launcherInAdding(string launcherUrl);
    signal launcherInRemoving(string launcherUrl);
    signal launcherInMoving(string launcherUrl, int pos);

    signal disabledIsStealingDroppedLaunchers();

    property bool isStealingDroppedLaunchers: false
    property bool isShowingAddLaunchersMessage: false

    property bool __isLoadedDuringViewStartup: false
    property bool _suppressLauncherListWriteBack: false
    property bool _geometryTransitionInProgress: false
    property int _transientEmptyRecoveries: 0
    property string _pendingRestoreReason: ""
    property var _lastKnownGoodLauncherList: []

    property string appletIndex: bridge && bridge.indexer ? String(bridge.indexer.appletIndex) : ""
    property int group: LatteCore.types.UniqueLaunchers
    property string groupId: view && group === LatteCore.types.UniqueLaunchers ? String(view.groupId) + "#" + appletIndex : ""

    property Item bridge: null
    property Item layout: null
    property Item view: null
    property QtObject tasksModel: null

    readonly property LaunchersPart.Syncer syncer: LaunchersPart.Syncer{}
    readonly property LaunchersPart.Validator validator: LaunchersPart.Validator{}

    readonly property string _NULLACTIVITYID_: "00000000-0000-0000-0000-000000000000"

    function inUniqueGroup() {
        return group === LatteCore.types.UniqueLaunchers;
    }

    function inLayoutGroup() {
        return group === LatteCore.types.LayoutLaunchers;
    }

    function inGlobalGroup() {
        return group === LatteCore.types.GlobalLaunchers;
    }

    function isSeparator(launcher){
        return (launcher.indexOf("latte-separator")!==-1 && launcher.indexOf(".desktop")!==1);
    }

    function separatorExists(separator){
        return (_launchers.tasksModel.launcherPosition(separator)>=0);
    }

    function freeAvailableSeparatorName() {
        var available = false;
        var no = 1;

        var separatorName = "";

        while(!available && no<20) {
            separatorName = "file:///latte-separator"+no+".desktop";
            if (separatorExists(separatorName)) {
                no = no + 1;
            } else {
                available = true;
            }
        }

        if (available) {
            return separatorName;
        } else {
            return "";
        }
    }

    function hasLauncher(url) {
        return _launchers.tasksModel.launcherPosition(url) >= 0;
    }

    function addLauncher(launcherUrl) {
        if (bridge) {
            bridge.launchers.host.addSyncedLauncher(syncer.clientId,
                                                    launchers.group,
                                                    launchers.groupId,
                                                    launcherUrl);
        } else {
            _launchers.tasksModel.requestAddLauncher(launcherUrl);
            _launchers.launcherChanged(launcherUrl);
        }
    }

    function addDroppedLauncher(launcherUrl) {
        //workaround to protect in case the launcher contains the iconData
        var pos = launcherUrl.indexOf("?iconData=");

        if (pos>0) {
            launcherUrl = launcherUrl.substring( 0, launcherUrl.indexOf("?iconData=" ) );
        }

        var path = launcherUrl;
        var filename = path.split("/").pop();
        _launchers.launcherInAdding(filename);

        tasksModel.requestAddLauncher(launcherUrl);
        launchers.launcherChanged(launcherUrl);
        tasksModel.syncLaunchers();
    }

    function addDroppedLaunchers(urls) {
        //! inform synced docks for new dropped launchers
        if (bridge) {
            bridge.launchers.host.addDroppedLaunchers(syncer.clientId,
                                                      launchers.group,
                                                      launchers.groupId,
                                                      urls);
        } else {
            urls.forEach(function (item) {
                addDroppedLauncher(item);
            });
        }
    }

    function addInternalSeparatorAtPos(pos) {
        var separatorName = freeAvailableSeparatorName();

        if (separatorName === "") {
            return false;
        }

        _launchers.launcherInMoving(separatorName, Math.max(0,pos));
        addLauncher(separatorName);
        return true;
    }

    function removeInternalSeparatorAtPos(pos) {
        var item = childAtLayoutIndex(pos);

        if (item && item.isSeparator) {
            removeLauncher(item.launcherUrl);
            return true;
        }

        return false;
    }

    function removeLauncher(launcherUrl) {
        if (bridge) {
            bridge.launchers.host.removeSyncedLauncher(syncer.clientId,
                                                       launchers.group,
                                                       launchers.groupId,
                                                       launcherUrl);
        } else {
            _launchers.launcherInRemoving(launcherUrl);
            _launchers.tasksModel.requestRemoveLauncher(launcherUrl);
            _launchers.launcherRemoved(launcherUrl);
        }
    }

    function addLauncherToActivity(launcherUrl, activityId) {
        if (bridge) {
            bridge.launchers.host.addSyncedLauncherToActivity(syncer.clientId,
                                                              launchers.group,
                                                              launchers.groupId,
                                                              launcherUrl,
                                                              activityId);
        } else {
            if (activityId !== activityInfo.currentActivity && isOnAllActivities(launcherUrl)) {
                _launchers.launcherInRemoving(launcherUrl);
            }

            _launchers.tasksModel.requestAddLauncherToActivity(launcherUrl, activityId);
            _launchers.launcherChanged(launcherUrl);
        }
    }

    function removeLauncherFromActivity(launcherUrl, activityId) {
        if (bridge) {
            bridge.launchers.host.removeSyncedLauncherFromActivity(syncer.clientId,
                                                                   launchers.group,
                                                                   launchers.groupId,
                                                                   launcherUrl,
                                                                   activityId);
        } else {
            if (activityId === activityInfo.currentActivity) {
                _launchers.launcherInRemoving(launcherUrl);
            }
            _launchers.tasksModel.requestRemoveLauncherFromActivity(launcherUrl, activityId);
            _launchers.launcherChanged(launcherUrl);
        }
    }

    function validateSyncedLaunchersOrder() {
        if (bridge) {
            bridge.launchers.host.validateSyncedLaunchersOrder(syncer.clientId,
                                                               launchers.group,
                                                               launchers.groupId,
                                                               currentShownLauncherList());
        } else {
            /*validator.stop();
            validator.launchers = orderedLaunchers;
            validator.start();*/
        }
    }

    function inCurrentActivity(launcherUrl) {
        if (!hasLauncher(launcherUrl)) {
            return false;
        }

        var activities = _launchers.tasksModel.launcherActivities(launcherUrl);

        if (activities.length ===0
                || activities.indexOf(_NULLACTIVITYID_) >= 0
                || activities.indexOf(activityInfo.currentActivity) >= 0) {
            return true;
        }

        return false;
    }

    function isOnAllActivities(launcherUrl) {
        var activities = _launchers.tasksModel.launcherActivities(launcherUrl);
        return (activities.indexOf(_NULLACTIVITYID_) >= 0)
    }

    function childAtLayoutIndex(position) {
        var tasks = layout.children;

        if (position < 0) {
            return;
        }

        for(var i=0; i<tasks.length; ++i){
            var task = tasks[i];

            if (task.lastValidIndex === position
                    || (task.lastValidIndex === -1 && task.itemIndex === position )) {
                return task;
            }
        }

        return undefined;
    }

    function indexOfLayoutLauncher(url) {
        var tasks = layout.children;

        for(var i=0; i<tasks.length; ++i){
            var task = tasks[i];

            if (task && (task.launcherUrl===url)) {
                return task.itemIndex;
            }
        }

        return -1;
    }

    function currentShownLauncherList() {
        var launch = [];

        var tasks = _launchers.layout.children;
        for(var i=0; i<tasks.length; ++i){
            var task = _launchers.childAtLayoutIndex(i);

            if (task!==undefined && task.launcherUrl!=="" && _launchers.inCurrentActivity(task.launcherUrl)) {
                launch.push(task.launcherUrl);
            }
        }

        return launch;
    }


    function currentStoredLauncherList() {
        var launch = [];
        var launchersList = [];

        if (bridge && bridge.launchers.host.isReady && !_launchers.inUniqueGroup()) {
            if (_launchers.inLayoutGroup()) {
                launchersList = bridge.launchers.host.layoutLaunchers;
            } else if (_launchers.inGlobalGroup()) {
                launchersList = bridge.launchers.host.universalLaunchers;
            }
        } else {
            launchersList = plasmoid.configuration.launchers59;
        }


        for(var i=0; i<launchersList.length; ++i){
            var launcherRecord = launchersList[i];

            if (launcherRecord.indexOf("[") === -1) {
                //global launcher
                launch.push(launcherRecord);
            } else {
                //launcher assigned to activities
                var end = launcherRecord.indexOf("\n");
                var explicitLauncher = launcherRecord.substring(end+1,launcherRecord.length);
                if (explicitLauncher !== "" && launcherRecord.indexOf(activityInfo.currentActivity) > -1) {
                    launch.push(explicitLauncher);
                }
            }
        }

        return launch;
    }

    function importLauncherListInModel() {
        if (!tasksModel) {
            return;
        }

        var launchersList = [];

        if (bridge && bridge.launchers.host.isReady && !inUniqueGroup()) {
            if (inLayoutGroup()) {
                console.log("Tasks: Applying LAYOUT Launchers List...");
                launchersList = bridge.launchers.host.layoutLaunchers;
            } else if (inGlobalGroup()) {
                console.log("Tasks: Applying GLOBAL Launchers List...");
                launchersList = bridge.launchers.host.universalLaunchers;
            }
        } else {
            console.log("Tasks: Applying UNIQUE Launchers List...");
            launchersList = plasmoid.configuration.launchers59;
        }

        var normalized = _launchers.normalizeLauncherList(launchersList);

        if (normalized.changed) {
            if (!bridge || !bridge.launchers.host.isReady || inUniqueGroup()) {
                plasmoid.configuration.launchers59 = normalized.list;
            }

            console.log("Tasks: Migrated legacy launcher desktop ids to preferred urls...");
        }

        tasksModel.launcherList = normalized.list.slice(0);
        tasksModel.syncLaunchers();

        if (normalized.list.length > 0) {
            _launchers._lastKnownGoodLauncherList = normalized.list.slice(0);
        }
    }

    function restoreLaunchersFromConfig(reason) {
        if (!tasksModel) {
            return;
        }

        var storedLaunchers = currentStoredLauncherList();
        var sourceLaunchers = storedLaunchers.length > 0
                ? storedLaunchers
                : _lastKnownGoodLauncherList;

        if (!sourceLaunchers || sourceLaunchers.length === 0) {
            console.log("Tasks: launcher restore skipped, no stored launchers (reason:", reason, ")");
            return;
        }

        var normalized = normalizeLauncherList(sourceLaunchers);
        var restoredList = normalized.list.slice(0);

        _suppressLauncherListWriteBack = true;
        tasksModel.launcherList = restoredList;
        tasksModel.syncLaunchers();
        _lastKnownGoodLauncherList = restoredList.slice(0);
        launcherWriteBackUnlockTimer.restart();

        console.log("Tasks: restored launchers after geometry change (reason:", reason,
                    ", launchers:", restoredList.length,
                    ", tasksCount:", tasksModel.count, ")");
    }

    function scheduleLaunchersRestore(reason) {
        _pendingRestoreReason = reason || "unspecified";
        _geometryTransitionInProgress = true;
        geometryTransitionGuardTimer.restart();

        console.log("Tasks: schedule launcher restore (reason:", _pendingRestoreReason,
                    ", viewReady:", appletAbilities.myView.isReady,
                    ", launchers:", currentStoredLauncherList().length,
                    ", tasksCount:", tasksModel ? tasksModel.count : -1, ")");

        launchersRestoreTimer.restart();
        launchersRestoreFollowUpTimer.restart();
        launchersRestoreFinalTimer.restart();
    }

    function normalizeLauncherUrl(url) {
        if (url === "applications:firefox.desktop") {
            return "preferred://browser";
        }

        if (url === "applications:org.kde.dolphin.desktop") {
            return "preferred://filemanager";
        }

        if (url === "preferred://email") {
            return "applications:thunderbird.desktop";
        }

        if (url === "preferred://terminal") {
            return "applications:org.kde.konsole.desktop";
        }

        if (url === "preferred://mailer") {
            return "applications:thunderbird.desktop";
        }

        if (url === "preferred://systemsettings") {
            return "applications:systemsettings.desktop";
        }

        if (url === "applications:systemsettings.desktop"
                || url === "applications:kdesystemsettings.desktop"
                || url === "applications:org.kde.systemsettings.desktop"
                || url === "systemsettings.desktop"
                || url === "kdesystemsettings.desktop"
                || url === "org.kde.systemsettings.desktop") {
            return "applications:systemsettings.desktop";
        }

        return url;
    }

    function normalizeLauncherRecord(record) {
        var end = record.indexOf("\n");

        if (end === -1) {
            return normalizeLauncherUrl(record);
        }

        var activities = record.substring(0, end + 1);
        var launcher = record.substring(end + 1);
        var normalizedLauncher = normalizeLauncherUrl(launcher);

        return activities + normalizedLauncher;
    }

    function normalizeLauncherList(launchersList) {
        var changed = false;
        var normalized = [];
        var plainRecordsOnly = true;

        for (var i=0; i<launchersList.length; ++i) {
            var current = launchersList[i];
            var migrated = normalizeLauncherRecord(current);

            if (current.indexOf("\n") !== -1) {
                plainRecordsOnly = false;
            }

            if (migrated !== current) {
                changed = true;
            }

            normalized.push(migrated);
        }

        // Migrate old Latte defaults to a Plasma 6 stable launcher set.
        // NOTE: some preferred:// launcher schemes are stripped by the task model
        // under Plasma 6, so we use concrete desktop launcher ids for stability.
        // Apply only to plain launcher records to avoid rewriting activity-specific entries.
        var defaultLaunchers = [
            "preferred://browser",
            "preferred://filemanager",
            "applications:org.kde.konsole.desktop",
            "applications:thunderbird.desktop",
            "applications:systemsettings.desktop"
        ];

        if (plainRecordsOnly && normalized.length >= 2 && normalized.length < defaultLaunchers.length) {
            var isDefaultPrefix = true;

            for (var p = 0; p < normalized.length; ++p) {
                if (normalized[p] !== defaultLaunchers[p]) {
                    isDefaultPrefix = false;
                    break;
                }
            }

            if (isDefaultPrefix) {
                for (var n = normalized.length; n < defaultLaunchers.length; ++n) {
                    normalized.push(defaultLaunchers[n]);
                }

                changed = true;
            }
        }

        return {
            "changed": changed,
            "list": normalized
        };
    }


    //! Connections
    Component.onCompleted: {
        if (isActive) {
            bridge.launchers.client = _launchers;
        }
    }

    Component.onDestruction: {
        if (isActive) {
            bridge.launchers.client = null;
        }
    }

    onIsActiveChanged: {
        if (isActive) {
            bridge.launchers.client = _launchers;
        }
    }

    onGroupChanged:{
        if(appletAbilities.myView.isReady) {
            _launchers.importLauncherListInModel();
        }
    }

    Connections {
        target: appletAbilities.myView
        function onIsReadyChanged() {
            if(appletAbilities.myView.isReady) {
                if (!_launchers.inUniqueGroup()) {
                    _launchers.importLauncherListInModel();
                }
            }
        }
    }

    Connections {
        target: plasmoid
        function onLocationChanged() {
            _launchers.scheduleLaunchersRestore("locationChanged");
        }

        function onFormFactorChanged() {
            _launchers.scheduleLaunchersRestore("formFactorChanged");
        }

        function onUserConfiguringChanged() {
            if (!plasmoid.userConfiguring && appletAbilities.myView.isReady) {
                _launchers.scheduleLaunchersRestore("configurationClosed");
            }
        }
    }

    Connections {
        target: bridge ? bridge.launchers.host : null
        function onIsReadyChanged() {
            if (bridge && bridge.launchers.host.isReady && !_launchers.__isLoadedDuringViewStartup) {
                _launchers.__isLoadedDuringViewStartup = true;
                _launchers.importLauncherListInModel();
            }
        }
    }

    Connections {
        target: _launchers.tasksModel
        function onLauncherListChanged() {
            if (_launchers._suppressLauncherListWriteBack) {
                return;
            }

            var currentModelLaunchers = (_launchers.tasksModel && _launchers.tasksModel.launcherList)
                    ? _launchers.tasksModel.launcherList : [];

            if (currentModelLaunchers.length > 0) {
                _launchers._lastKnownGoodLauncherList = currentModelLaunchers.slice(0);
            }

            // During edge/form-factor transitions, TasksModel can briefly publish an
            // empty launcher list before its internal state stabilizes. Guard this
            // transient phase to avoid persisting an empty launcher set.
            if (currentModelLaunchers.length === 0
                    && _launchers._geometryTransitionInProgress
                    && appletAbilities.myView.isReady) {
                var storedLaunchers = _launchers.currentStoredLauncherList();
                var fallbackLaunchers = storedLaunchers.length > 0
                        ? storedLaunchers
                        : _launchers._lastKnownGoodLauncherList;

                if (fallbackLaunchers.length > 0 && _launchers._transientEmptyRecoveries < 8) {
                    _launchers._transientEmptyRecoveries++;
                    console.log("Tasks: transient empty launcher list during geometry transition, restoring from stored launchers...");
                    _launchers.scheduleLaunchersRestore("transientEmptyLauncherList");
                    return;
                }
            } else {
                _launchers._transientEmptyRecoveries = 0;
            }

            if (bridge && bridge.launchers.host.isReady) {
                if (!_launchers.inUniqueGroup()) {
                    if (_launchers.inLayoutGroup()) {
                        bridge.launchers.host.setLayoutLaunchers(_launchers.tasksModel.launcherList);
                    } else if (_launchers.inGlobalGroup()) {
                        bridge.launchers.host.setUniversalLaunchers(_launchers.tasksModel.launcherList);
                    }
                } else {
                    plasmoid.configuration.launchers59 = currentModelLaunchers;
                }

                if (inDraggingPhase) {
                    _launchers.validateSyncedLaunchersOrder();
                }
            } else if (!appletAbilities.myView.isReady) {
                // This way we make sure that a delayed view.layout initialization does not store irrelevant launchers from different
                // group to UNIQUE launchers group.
                // Never persist transient empties during startup/edge transition.
                if (currentModelLaunchers.length > 0) {
                    plasmoid.configuration.launchers59 = currentModelLaunchers;
                }
            }
        }
    }

    Timer {
        id: launchersRestoreTimer
        interval: 450
        repeat: false
        onTriggered: {
            _launchers.restoreLaunchersFromConfig(_launchers._pendingRestoreReason + ":primary");
        }
    }

    Timer {
        id: launchersRestoreFollowUpTimer
        interval: 1200
        repeat: false
        onTriggered: {
            _launchers.restoreLaunchersFromConfig(_launchers._pendingRestoreReason + ":followup");
        }
    }

    Timer {
        id: launchersRestoreFinalTimer
        interval: 2200
        repeat: false
        onTriggered: {
            _launchers.restoreLaunchersFromConfig(_launchers._pendingRestoreReason + ":final");
        }
    }

    Timer {
        id: geometryTransitionGuardTimer
        interval: 2600
        repeat: false
        onTriggered: {
            _launchers._geometryTransitionInProgress = false;
            _launchers._transientEmptyRecoveries = 0;
        }
    }

    Timer {
        id: launcherWriteBackUnlockTimer
        interval: 160
        repeat: false
        onTriggered: {
            _launchers._suppressLauncherListWriteBack = false;
        }
    }
}
