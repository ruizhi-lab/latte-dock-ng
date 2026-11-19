/*
    SPDX-FileCopyrightText: 2019 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick

//! Trying to WORKAROUND all the Plasma LibTaskManager limitations
//! concerning Tasks AND Launchers.
//!
//! Plasma LibTaskManager constantly creates ADDITIONS/REMOVALS when
//! a Task is changing its type from Launcher<->Startup<->Window.
//! This libtaskmanager behavior limits a lot the animations that
//! can be created in order to provide a moder user experience
//!
//! All the logic that is trying to improve the mentioned limits is provided
//! from this class

Item {
    id: tasksExtManager

    /// Launchers that are playing an ADD or REMOVAL animation
    /// and their Startups/Windows should be aware of
    property var waitingLaunchers: []

    //! Launchers that must be shown IMMEDIATELY after a window removal
    //! because they are already present from a present libtaskmanager state
    property var immediateLaunchers: []

    //! New launchers in order to be moved in correct place:
    //! launcher, pos)
    property var launchersToBeMoved: []

    //! Launchers that are added from user actions. They can be used in order
    //! to provide addition animations properly
    property var launchersToBeAdded: []

    //! Launchers that are added from user actions. They can be used in order
    //! to provide removal animations properly
    property var launchersToBeRemoved: []

    //! Tasks that change state (launcher,startup,window) and
    //! at the next state must look the same concerning the parabolic effect:
    //! (id, zoom)
    property var frozenTasks: []

    readonly property int launchersInPausedStateCount: launchersToBeMovedCount + launchersToBeAddedCount + launchersToBeRemovedCount

    property int launchersToBeMovedCount: 0 //is used to update instantly relevant bindings
    property int launchersToBeAddedCount: 0 //is used to update instantly relevant bindings
    property int launchersToBeRemovedCount: 0 //is used to update instantly relevant bindings

    signal waitingLauncherRemoved(string launch);
    /////////// FUNCTIONALITY ////////////////////

    // LibTaskManager can briefly insert a window/startup delegate before the
    // matching pinned launcher delegate finishes removal (and does the reverse
    // on close). Keep only the incoming row out of the layout during that
    // handoff; the outgoing delegate remains the authoritative visual until its
    // own removal animation completes. Restrict this to adjacent, same-identity
    // launcher/task rows so grouped or multiple-window tasks keep their normal
    // independent geometry and animation.
    function isAdjacentTaskRowReplacement(incoming, outgoing, launcherIsPinned) {
        if (!launcherIsPinned || !incoming || !outgoing || incoming === outgoing
                || incoming.isGroupParent || outgoing.isGroupParent
                || incoming.windowsCount > 1 || outgoing.windowsCount > 1
                || incoming.itemIndex < 0 || outgoing.itemIndex >= 0) {
            return false;
        }

        var sameLauncher = (incoming.launcherUrl !== ""
                            && incoming.launcherUrl === outgoing.launcherUrl)
                || (incoming.launcherUrlWithIcon !== ""
                    && incoming.launcherUrlWithIcon === outgoing.launcherUrlWithIcon);
        if (!sameLauncher) {
            return false;
        }

        var incomingIsTask = incoming.isWindow || incoming.isStartup;
        var outgoingIsTask = outgoing.isWindow || outgoing.isStartup;
        var isLauncherToTask = outgoing.isLauncher && incomingIsTask;
        var isTaskToLauncher = incoming.isLauncher && outgoingIsTask;
        if (!isLauncherToTask && !isTaskToLauncher) {
            return false;
        }

        var outgoingIndex = outgoing.lastValidIndex;
        return outgoingIndex >= 0 && Math.abs(incoming.itemIndex - outgoingIndex) <= 1;
    }

    function holdTaskRowForReplacement(incoming, outgoing, launcherIsPinned) {
        if (!isAdjacentTaskRowReplacement(incoming, outgoing, launcherIsPinned)
                || outgoing.taskRowReplacementSuccessor || incoming.waitingForTaskRowReplacement) {
            return false;
        }

        incoming.pendingTaskRowReplacementOutgoing = outgoing;
        incoming.waitingForTaskRowReplacement = true;
        outgoing.taskRowReplacementSuccessor = incoming;
        incoming.parabolicItem.zoomLength = 0;
        incoming.parabolicItem.zoomThickness = 0;
        incoming.parabolicItem.opacity = 0;
        return true;
    }

    function releaseTaskRowReplacement(incoming, outgoing, removalAnimationFinished) {
        if (!incoming || !outgoing || incoming.pendingTaskRowReplacementOutgoing !== outgoing) {
            return false;
        }

        incoming.pendingTaskRowReplacementOutgoing = null;
        incoming.waitingForTaskRowReplacement = false;
        if (outgoing.taskRowReplacementSuccessor === incoming) {
            outgoing.taskRowReplacementSuccessor = null;
        }

        // The removal animation has already passed the RemovalScheduler's
        // zero-interval defer, so its final frame is safe for an atomic slot
        // handoff. Other callers (index cancellation/destruction) still defer.
        if (removalAnimationFinished) {
            incoming.resumeTaskRowReplacementAnimation();
        } else {
            incoming.scheduleTaskRowReplacementAnimation();
        }
        return true;
    }


    /// WAITING LAUNCHERS
    function addWaitingLauncher(launch){
        arraysGarbageCollectorTimer.restart();

        if (waitingLauncherExists(launch)) {
            return;
        }

        waitingLaunchers.push(launch);
    }

    function removeWaitingLauncher(launch){
        for(var i=0; i<waitingLaunchers.length; ++i){
            if (equals(waitingLaunchers[i], launch)) {
                waitingLaunchers.splice(i,1);
                waitingLauncherRemoved(launch);
                return;
            }
        }
    }

    function waitingLauncherExists(launch){
        for(var i=0; i<waitingLaunchers.length; ++i){
            if (equals(waitingLaunchers[i], launch)) {
                return true;
            }
        }

        return false;
    }

    function equals(waitingLauncher, launcher) {
        var equals = ( launcher !== ""
                      && waitingLauncher !== ""
                      && (launcher.indexOf(waitingLauncher) >= 0 || waitingLauncher.indexOf(launcher) >= 0));

        return equals;
    }

    function waitingLaunchersLength() {
        return waitingLaunchers.length;
    }

    function printWaitingLaunchers() {
        console.log("WAITING LAUNCHERS ::: " + waitingLaunchers);
    }

    //! LAUNCHERSTOBEADDED
    function addToBeAddedLauncher(launcher){
        arraysGarbageCollectorTimer.restart();

        if (toBeAddedLauncherExists(launcher)) {
            return;
        }

        launchersToBeAdded.push(launcher);
        launchersToBeAddedCount++;
    }

    function removeToBeAddedLauncher(launcher){
        for(var i=0; i<launchersToBeAdded.length; ++i){
            if (equals(launchersToBeAdded[i], launcher)) {
                launchersToBeAdded.splice(i,1);
                launchersToBeAddedCount--;
                return;
            }
        }
    }

    function toBeAddedLauncherExists(launcher) {
        for(var i=0; i<launchersToBeAdded.length; ++i){
            if (equals(launchersToBeAdded[i], launcher)) {
                return true;
            }
        }

        return false;
    }


    function printToBeAddedLaunchers() {
        console.log("TO BE ADDED LAUNCHERS ::: " + launchersToBeAdded);
    }

    //! LAUNCHERSTOBEREMOVED
    function addToBeRemovedLauncher(launcher){
        arraysGarbageCollectorTimer.restart();

        if (isLauncherToBeRemoved(launcher)) {
            return;
        }

        launchersToBeRemoved.push(launcher);
        launchersToBeRemovedCount++;
    }

    function removeToBeRemovedLauncher(launcher){
        if (!isLauncherToBeRemoved(launcher)) {
            return;
        }

        for(var i=0; i<launchersToBeRemoved.length; ++i){
            if (launchersToBeRemoved[i] === launcher) {
                launchersToBeRemoved.splice(i,1);
                launchersToBeRemovedCount--;
                return;
            }
        }
    }

    function isLauncherToBeRemoved(launcher) {
        return launchersToBeRemoved.indexOf(launcher)>=0;
    }


    function printToBeRemovedLaunchers() {
        console.log("TO BE REMOVED LAUNCHERS ::: " + launchersToBeRemoved);
    }

    //! IMMEDIATELAUNCHERS
    function addImmediateLauncher(launch){
        arraysGarbageCollectorTimer.restart();

        if (!immediateLauncherExists(launch)) {
            //console.log("Immediate Launcher Added::: "+launch);
            immediateLaunchers.push(launch);
        }
    }

    function removeImmediateLauncher(launch){
        for(var i=0; i<immediateLaunchers.length; ++i){
            if (immediateLaunchers[i]===launch) {
                immediateLaunchers.splice(i,1);
                //console.log("Immediate Launcher Removed::: "+launch);
                return;
            }
        }
    }

    function immediateLauncherExists(launch){
        for(var i=0; i<immediateLaunchers.length; ++i){
            if (immediateLaunchers[i]===launch) {
                return true;
            }
        }

        return false;
    }

    function printImmediateLaunchers() {
        console.log("IMMEDIATE LAUNCHERS ::: " + immediateLaunchers);
    }
    //!

    //! FROZENTASKS
    function getFrozenTask(identifier) {
        for(var i=0; i<frozenTasks.length; ++i) {
            if (frozenTasks[i].id === identifier) {
                return frozenTasks[i];
            }
        }
    }

    function removeFrozenTask(identifier) {
        var taskIndex = -1;
        for(var i=0; i<frozenTasks.length; ++i) {
            if (frozenTasks[i].id === identifier) {
                taskIndex = i;
            }
        }

        if (taskIndex > -1) {
            frozenTasks.splice(taskIndex, 1);
        }
    }

    function setFrozenTask(identifier, scale) {
        arraysGarbageCollectorTimer.restart();

        var frozenTaskExists = false;
        //console.log("SET FROZEN :: "+identifier+" - "+scale);
        var frozenTask = getFrozenTask(identifier);

        if (frozenTask) {
            frozenTask.zoom = scale;
        } else {
            frozenTasks.push({id: identifier, zoom: scale});
        }
    }

    function printFrozenTasks() {
        var fzTasks= "";

        for(var i=0; i<frozenTasks.length; ++i) {
            fzTasks = frozenTasks[i].id + "," + frozenTasks[i].zoom + "__";
        }

        console.log("FROZEN TASKS ::: " + fzTasks);
    }

    //! LAUNCHERSTOBEMOVED

    //! launchersToBeMoved, new launchers to have been added and must be repositioned
    function addLauncherToBeMoved(launcherUrl, toPos) {
        arraysGarbageCollectorTimer.restart();

        if (!isLauncherToBeMoved(launcherUrl)) {
            launchersToBeMoved.push({launcher: launcherUrl, pos: Math.max(0,toPos)});
            launchersToBeMovedCount++;
        }
    }

    function moveLauncherToCorrectPos(launcherUrl, from) {
        if (isLauncherToBeMoved(launcherUrl)) {
            launchersToBeMovedTimer.from = from;
            launchersToBeMovedTimer.to = posOfLauncherToBeMoved(launcherUrl);
            launchersToBeMovedTimer.launcherUrl = launcherUrl

            removeLauncherToBeMoved(launcherUrl);
            launchersToBeMovedTimer.start();
        }
    }

    function removeLauncherToBeMoved(launcherUrl) {
        if (isLauncherToBeMoved(launcherUrl)) {
            var sLength = launchersToBeMoved.length;
            var index = -1;

            for (var i=0; i<sLength; ++i) {
                //!safety checker
                if (i>=launchersToBeMoved.length)
                    return -1;

                if (launchersToBeMoved[i].launcher === launcherUrl) {
                    index = i;
                    break;
                }
            }

            if (index > -1) {
                // console.log("removing launcher to be moved:: "+launcherUrl);
                launchersToBeMoved.splice(index, 1);
            }
        }
    }

    function posOfLauncherToBeMoved(launcherUrl) {
        var sLength = launchersToBeMoved.length;

        for (var i=0; i<sLength; ++i) {
            //!safety checker
            if (i>=launchersToBeMoved.length)
                return -1;

            if (launchersToBeMoved[i].launcher === launcherUrl)
                return launchersToBeMoved[i].pos;
        }

        return -1;
    }

    function isLauncherToBeMoved(launcher) {
        return (posOfLauncherToBeMoved(launcher) >= 0);
    }

    function printToBeMovedLaunchers() {
        var tbmLaunchers= "";

        for(var i=0; i<launchersToBeMoved.length; ++i) {
            tbmLaunchers = launchersToBeMoved[i].launcher + "," + launchersToBeMoved[i].pos + "__";
        }

        console.log("TO BE MOVED LAUNCHERS ::: " + tbmLaunchers);
    }

    //! Connections
    Connections {
        target: appletAbilities.launchers
        function onLauncherInRemoving(launcherUrl) { tasksExtManager.addToBeRemovedLauncher(launcherUrl); }
        function onLauncherInAdding(launcherUrl) { tasksExtManager.addToBeAddedLauncher(launcherUrl); }
        function onLauncherInMoving(launcherUrl, pos) { tasksExtManager.addLauncherToBeMoved(launcherUrl, pos); }
    }


    //!Trying to avoid a binding loop in TaskItem for modelLauncherUrl
    Timer {
        id: launchersToBeMovedTimer
        interval: 50
        property int from: -1
        property int to: -1

        property string launcherUrl: ""

        onTriggered: {
            tasksModel.move(from, to);
            delayedLaynchersSyncTimer.start();
        }
    }

    //! delay a bit  the launchers syncing in order to avoid a crash
    //! the crash was caused from TasksExtendedManager when adding and moving a launcher (e.g. internal separator)
    //! and there were more than one synced docks
    Timer {
        id: delayedLaynchersSyncTimer
        interval: 450
        onTriggered: {
            tasksModel.syncLaunchers();
            appletAbilities.launchers.validateSyncedLaunchersOrder();
            //! In case there are multiple launchers in moving state
            launchersToBeMovedCount = 0;
        }
    }


    //! Timer to clean up all arrays used from TasksExtendedManager after a specified interval
    //! The arrays may have ghost records that were not used from animations or other plasmoid parts.
    //! Each record of the arrays is usually only a matter of secs to be used, cleaning them after
    //! a big interval from the last addition it is safe
    Timer {
        id: arraysGarbageCollectorTimer
        interval: 30 * 1000
        onTriggered: {
            console.log(" TASKS EXTENDED MANAGER Garbage Collector...");
            tasksExtManager.printImmediateLaunchers();
            tasksExtManager.printToBeAddedLaunchers();
            tasksExtManager.printToBeMovedLaunchers();
            tasksExtManager.printToBeRemovedLaunchers();
            tasksExtManager.printWaitingLaunchers();
            tasksExtManager.printFrozenTasks();

            immediateLaunchers.splice(0, immediateLaunchers.length);
            launchersToBeAdded.splice(0, launchersToBeAdded.length);
            launchersToBeMoved.splice(0, launchersToBeMoved.length);
            launchersToBeRemoved.splice(0, launchersToBeRemoved.length);
            waitingLaunchers.splice(0, waitingLaunchers.length);
            frozenTasks.splice(0, frozenTasks.length);

            //! clear up launchers counters
            tasksExtManager.launchersToBeMovedCount = 0;
            tasksExtManager.launchersToBeAddedCount = 0;
            tasksExtManager.launchersToBeRemovedCount = 0;
        }
    }
}
