/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.1

import org.kde.plasma.plasmoid 2.0

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents

Item{
    Component.onCompleted:  {
        v010_upgrade();
    }

    function v010_upgrade() {
        root.upgrader_v010_alignment();

        if (!plasmoid.configuration.shadowsUpgraded) {
            if (plasmoid.configuration.shadows > 0) {
                plasmoid.configuration.appletShadowsEnabled = true;
            } else {
                plasmoid.configuration.appletShadowsEnabled = false;
            }

            plasmoid.configuration.shadowsUpgraded = true;
        }

        if (!plasmoid.configuration.tasksUpgraded) {
            v010_tasksMigrateTimer.start();
        }

    }

    Item {
        id: v010_tasksUpgrader
        Repeater {
            id: v010_tasksRepeater
            model: latteView && !plasmoid.configuration.tasksUpgraded ? latteView.extendedInterface.latteTasksModel : null
            Item {
                id: tasksApplet
                Component.onCompleted: {
                    if (index === 0) {
                        console.log(" !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! !!!!!!!!!!!!!!!!");
                        console.log(" !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! !!!!!!!!!!!!!!!!");
                        console.log(" !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! !!!!!!!!!!!!!!!!");
                        console.log(" !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! !!!!!!!!!!!!!!!!");
                        console.log(" !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! !!!!!!!!!!!!!!!!");
                        console.log(" Migrating Tasks Applet for v0.10...");
                        tasks.plasmoid.configuration.launchersGroup = plasmoid.configuration.launchersGroup;
                        tasks.plasmoid.configuration.showWindowActions = plasmoid.configuration.showWindowActions;
                        tasks.plasmoid.configuration.showWindowsOnlyFromLaunchers = plasmoid.configuration.showWindowsOnlyFromLaunchers;
                        tasks.plasmoid.configuration.groupTasksByDefault = plasmoid.configuration.groupTasksByDefault;
                        tasks.plasmoid.configuration.showOnlyCurrentScreen = plasmoid.configuration.showOnlyCurrentScreen;
                        tasks.plasmoid.configuration.showOnlyCurrentDesktop = plasmoid.configuration.showOnlyCurrentDesktop;
                        tasks.plasmoid.configuration.showOnlyCurrentActivity = plasmoid.configuration.showOnlyCurrentActivity;
                        tasks.plasmoid.configuration.showInfoBadge = plasmoid.configuration.showInfoBadge;
                        tasks.plasmoid.configuration.showProgressBadge = plasmoid.configuration.showProgressBadge;
                        tasks.plasmoid.configuration.showAudioBadge = plasmoid.configuration.showAudioBadge;
                        tasks.plasmoid.configuration.audioBadgeActionsEnabled = plasmoid.configuration.audioBadgeActionsEnabled;
                        tasks.plasmoid.configuration.infoBadgeProminentColorEnabled = plasmoid.configuration.infoBadgeProminentColorEnabled;
                        tasks.plasmoid.configuration.animationLauncherBouncing = plasmoid.configuration.animationLauncherBouncing;
                        tasks.plasmoid.configuration.animationWindowInAttention = plasmoid.configuration.animationWindowInAttention;
                        tasks.plasmoid.configuration.animationNewWindowSliding = plasmoid.configuration.animationNewWindowSliding;
                        tasks.plasmoid.configuration.animationWindowAddedInGroup = plasmoid.configuration.animationWindowAddedInGroup;
                        tasks.plasmoid.configuration.animationWindowRemovedFromGroup = plasmoid.configuration.animationWindowRemovedFromGroup;
                        tasks.plasmoid.configuration.scrollTasksEnabled = plasmoid.configuration.scrollTasksEnabled;
                        tasks.plasmoid.configuration.autoScrollTasksEnabled = plasmoid.configuration.autoScrollTasksEnabled;
                        tasks.plasmoid.configuration.manualScrollTasksType = plasmoid.configuration.manualScrollTasksType;
                        tasks.plasmoid.configuration.leftClickAction = plasmoid.configuration.leftClickAction;
                        tasks.plasmoid.configuration.middleClickAction = plasmoid.configuration.middleClickAction;
                        tasks.plasmoid.configuration.hoverAction = plasmoid.configuration.hoverAction;
                        tasks.plasmoid.configuration.taskScrollAction = plasmoid.configuration.taskScrollAction;
                        tasks.plasmoid.configuration.modifierClickAction = plasmoid.configuration.modifierClickAction;
                        tasks.plasmoid.configuration.modifier = plasmoid.configuration.modifier;
                        tasks.plasmoid.configuration.modifierClick = plasmoid.configuration.modifierClick;
                        tasks.plasmoid.configuration.isDroppedLauncherAddedOnlyInCurrentTasks = plasmoid.configuration.addLaunchersInTaskManager;
                        console.log("Migrating Tasks Applet for v0.10 succeeded ...");

                        plasmoid.configuration.tasksUpgraded = true;
                    }
                }
            }
        }
    }


    //! v0.10 Timer to check that first-upgrade process ended
    //! when View does not have any Tasks plasmoid
    Timer {
        id: v010_tasksMigrateTimer
        interval: 10000
        onTriggered: {
            plasmoid.configuration.tasksUpgraded = true;
            console.log("Tasks Migration ended....");
        }
    }
}
