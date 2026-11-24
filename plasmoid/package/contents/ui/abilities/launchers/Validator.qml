/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick

//! Launchers Validate Timer
Timer{
    id:launchersOrderValidatorTimer
    interval: retryInterval

    readonly property int retryInterval: 400

    property var launchers: []
    property int emptyModelRetries: 0

    function validateOrder(orderedLaunchers) {
        launchers = orderedLaunchers;
        emptyModelRetries = 5;
        // Apply a new order on the next frame. The longer interval lets the
        // peer's task model and delegates settle between follow-up attempts.
        interval = 0;
        restart();
    }

    function arraysAreEqual(list1, list2) {
        if (list1.length !== list2.length) {
            console.log("  arrays have different size...")
            return false;
        }

        for (var i=0; i<list1.length; ++i) {
            if (list1[i] !== list2[i]) {
                return false;
            }
        }

        return true;
    }

    //! true if upward is the best way to iterate through current
    //! in order to make it equal with goal
    function upwardIsBetter(current, goal)
    {
        var tCurrent = current.slice();

        if (!arraysAreEqual(tCurrent, goal)) {
            for (var i=0; i<tCurrent.length; ++i) {
                if (tCurrent[i] !== goal[i]) {
                    var val = tCurrent[i];
                    tCurrent.splice(i, 1);
                    tCurrent.splice(goal.indexOf(val), 0, val);

                    if (arraysAreEqual(tCurrent, goal)){
                        return true;
                    } else {
                        return false;
                    }
                }
            }
        }

        return false;
    }

    onTriggered: {
        interval = retryInterval;
        var currentLaunchers = _launchers.currentShownLauncherList();
        if (currentLaunchers.length === 0 && launchers.length > 0) {
            // A peer can receive the order before its task delegates exist.
            // Keep the latest request until they appear, with a bounded wait.
            if (emptyModelRetries > 0) {
                --emptyModelRetries;
                restart();
            }
            return;
        }

        // Peers can expose different URLs for the same launcher. Use the shared
        // identity for ordering and keep the peer's URL for its delegate lookup.
        const currentOrder = currentLaunchers.map(url => _launchers.launcherIdentity(url));
        const targetOrder = launchers.map(url => _launchers.launcherIdentity(url));

        if (arraysAreEqual(currentOrder, targetOrder)) {
            stop();
            console.log("launchers synced at:" + launchers);
            launchers.length = 0;
            _launchers.syncLauncherOrder();
        } else {
            if (upwardIsBetter(currentOrder, targetOrder)) {
                console.log("UPWARD....");
                for (var i=0; i<currentLaunchers.length; ++i) {
                    if (currentOrder[i] !== targetOrder[i]) {
                        var p = targetOrder.indexOf(currentOrder[i]);
                        if (p === -1) {
                            console.log("No pos found for :"+currentLaunchers[i] + " at: "+launchers);
                            restart();
                            return;
                        }
                        var launcherLayoutIndex = _launchers.indexOfLayoutLauncher(currentLaunchers[i]);

                        if (launcherLayoutIndex === -1) {
                            console.log(" launcher was not found in model, syncing stopped...");
                            stop();
                            return;
                        }

                        console.log(" moving:" +launcherLayoutIndex + " _ " + p );
                        _launchers.tasksModel.move(launcherLayoutIndex, p);
                        restart();
                        return;
                    }
                }
            } else {
                console.log("DOWNWARD....");
                for (var i=currentLaunchers.length-1; i>=0; --i) {
                    if (currentOrder[i] !== targetOrder[i]) {
                        var p = targetOrder.indexOf(currentOrder[i]);
                        if (p === -1) {
                            console.log("No pos found for :"+currentLaunchers[i] + " at: "+launchers);
                            restart();
                            return;
                        }
                        var launcherLayoutIndex = _launchers.indexOfLayoutLauncher(currentLaunchers[i]);

                        if (launcherLayoutIndex === -1) {
                            console.log(" launcher was not found in model, syncing stopped...");
                            stop();
                            return;
                        }

                        console.log(" moving:" +launcherLayoutIndex + " _ " + p );
                        _launchers.tasksModel.move(launcherLayoutIndex, p);
                        restart();
                        return;
                    }
                }
            }

            console.log("why we reached ??? ");
            console.log("CURRENT ::: " + currentLaunchers);
            console.log("VALID   ::: " + launchers);
        }
    }
}
