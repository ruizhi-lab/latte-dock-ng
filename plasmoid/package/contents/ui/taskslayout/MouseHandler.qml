/*
    SPDX-FileCopyrightText: 2016 Smith AR <audoban@openmailbox.org>
    SPDX-FileCopyrightText: 2016 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/


import QtQuick 2.0

import org.kde.plasma.plasmoid 2.0
import org.kde.draganddrop 2.0

import org.kde.taskmanager 0.1 as TaskManager

import "../../code/tools.js" as TaskTools

Item {
    // signal urlDropped(url url)
    id: dArea
    signal urlsDropped(var urls)

    property Item target
    property Item ignoredItem
    property Item dragParabolicItem
    property bool moved: false
    property bool containsDrag: false
    property Item pendingPinnedSource: null
    property int pendingPinnedTargetIndex: -1
    property string pendingPinnedLauncherUrl: ""
    property int pendingPinnedAttempts: 0

    property alias hoveredItem: dropHandler.hoveredItem

    readonly property alias isMovingTask: dropHandler.inMovingTask
    readonly property alias isDroppingFiles: dropHandler.inDroppingFiles
    readonly property alias isDroppingOnlyLaunchers: dropHandler.inDroppingOnlyLaunchers
    readonly property alias isDroppingSeparator: dropHandler.inDroppingSeparator

    function isLauncherTask(task) {
        return task && task.m && task.m.IsLauncher === true;
    }

    function launcherUrlForTask(task) {
        if (!task) {
            return "";
        }

        if (task.launcherUrl && task.launcherUrl.length > 0) {
            return task.launcherUrl;
        }

        if (task.launcherUrlWithIcon && task.launcherUrlWithIcon.length > 0) {
            return task.launcherUrlWithIcon;
        }

        return "";
    }

    function schedulePromoteToLauncherAndMove(sourceItem, targetIndex, launcherUrl) {
        pendingPinnedSource = sourceItem;
        pendingPinnedTargetIndex = Math.max(0, targetIndex);
        pendingPinnedLauncherUrl = launcherUrl;
        pendingPinnedAttempts = 0;
        promotePinnedTaskTimer.restart();
    }

    function clearPendingPromoteMove() {
        pendingPinnedSource = null;
        pendingPinnedTargetIndex = -1;
        pendingPinnedLauncherUrl = "";
        pendingPinnedAttempts = 0;
    }

    Timer {
        id: ignoreItemTimer

        repeat: false
        interval: 200

        onTriggered: {
            ignoredItem = null;
        }
    }

    Timer {
        id: promotePinnedTaskTimer

        interval: 40
        repeat: true

        onTriggered: {
            if (!pendingPinnedSource || pendingPinnedTargetIndex < 0) {
                clearPendingPromoteMove();
                stop();
                return;
            }

            var sourceItem = pendingPinnedSource;
            var launcherReady = isLauncherTask(sourceItem)
                    || tasksModel.launcherPosition(pendingPinnedLauncherUrl) >= 0;

            if (launcherReady) {
                var from = sourceItem.itemIndex;
                var to = Math.max(0, Math.min(pendingPinnedTargetIndex, tasksModel.count - 1));

                if (from >= 0 && to >= 0 && from !== to) {
                    tasksModel.move(from, to);
                }

                clearPendingPromoteMove();
                stop();
                return;
            }

            pendingPinnedAttempts = pendingPinnedAttempts + 1;
            if (pendingPinnedAttempts > 25) {
                clearPendingPromoteMove();
                stop();
            }
        }
    }

    Connections {
        target: root
        function onDragSourceChanged() {
            if (!dragSource) {
                if (dragParabolicItem && dragParabolicItem.clearParabolicFromExternalPosition) {
                    dragParabolicItem.clearParabolicFromExternalPosition();
                }
                dragParabolicItem = null;
                ignoredItem = null;
                ignoreItemTimer.stop();
                clearPendingPromoteMove();
                promotePinnedTaskTimer.stop();
            }
        }
    }

    function updateDragParabolic(hoverItem, rootX, rootY) {
        if (dragParabolicItem && dragParabolicItem !== hoverItem && dragParabolicItem.clearParabolicFromExternalPosition) {
            dragParabolicItem.clearParabolicFromExternalPosition();
        }

        if (hoverItem && hoverItem.updateParabolicFromExternalPosition
                && hoverItem.updateParabolicFromExternalPosition(dArea, rootX, rootY)) {
            dragParabolicItem = hoverItem;
        } else {
            dragParabolicItem = null;
        }
    }

    // Public entry point used by task delegates while dragging. Keep this on
    // the MouseHandler root item so callers can reliably access it.
    function reorderFromDragPosition(sourceItem, rootX, rootY) {
        if (!sourceItem || !target || tasksModel.sortMode !== TaskManager.TasksModel.SortManual) {
            return;
        }

        if (target.animating) {
            return;
        }

        var eventToTarget = mapToItem(target, rootX, rootY);
        var above = target.childAtPos(eventToTarget.x, eventToTarget.y, sourceItem);
        updateDragParabolic(above, rootX, rootY);
        var insertAt = TaskTools.insertIndexAt(above, eventToTarget.x, eventToTarget.y);

        if (insertAt < 0 || insertAt >= tasksModel.count) {
            return;
        }

        if (sourceItem === above || sourceItem.itemIndex === insertAt) {
            return;
        }

        var sourceIsLauncher = isLauncherTask(sourceItem);
        var targetTask = above ? above : (target.childAtIndex ? target.childAtIndex(insertAt) : null);
        var targetIsLauncher = isLauncherTask(targetTask);

        // Keep pinned launchers inside the launcher area.
        if (sourceIsLauncher && targetTask && !targetIsLauncher) {
            return;
        }

        // Allow dragging a non-pinned running app into launcher area by pinning it first.
        if (!sourceIsLauncher && targetTask && targetIsLauncher) {
            var launcherUrl = launcherUrlForTask(sourceItem);
            if (launcherUrl.length <= 0) {
                return;
            }

            if (tasksModel.launcherPosition(launcherUrl) === -1) {
                appletAbilities.launchers.addLauncher(launcherUrl);
            }

            schedulePromoteToLauncherAndMove(sourceItem, insertAt, launcherUrl);
            ignoredItem = targetTask;
            ignoreItemTimer.restart();
            return;
        }

        sourceItem.z = 100;
        ignoredItem = above;

        var from = sourceItem.itemIndex;
        tasksModel.move(from, insertAt);

        ignoreItemTimer.restart();
    }

    DropArea {
        id: dropHandler
        anchors.fill: parent
        preventStealing: true

        property bool inDroppingOnlyLaunchers: false
        property bool inDroppingSeparator: false
        property bool inMovingTask: false
        property bool inDroppingFiles: false

        readonly property bool eventIsAccepted: inMovingTask || inDroppingSeparator || inDroppingOnlyLaunchers || inDroppingFiles

        property int droppedPosition: -1;
        property Item hoveredItem

        function isDroppingSeparator(event) {
            var appletName = String(event.mimeData.getDataAsByteArray("text/x-plasmoidservicename"));
            var isSeparator = (appletName === "audoban.applet.separator" || appletName === "org.kde.latte.separator");

            return ((event.mimeData.formats.indexOf("text/x-plasmoidservicename") === 0) && isSeparator);
        }

        function isDroppingOnlyLaunchers(event) {
            if (event.mimeData.hasUrls || (event.mimeData.formats.indexOf("text/x-plasmoidservicename") !== 0)) {
                var onlyLaunchers = event.mimeData.urls.every(function (item) {
                    return backend.isApplication(item)
                });

                return onlyLaunchers;
            }

            return false;
        }

        function isMovingTask(event) {
            if (root.dragSource) {
                return true;
            }

            if (!event || !event.mimeData) {
                return false;
            }

            var source = event.mimeData.source;

            while (source) {
                if (source.objectName === "TaskItem") {
                    return true;
                }

                source = source.parent;
            }

            if (event.mimeData.formats === undefined) {
                return false;
            }

            for (var i = 0; i < event.mimeData.formats.length; ++i) {
                var format = String(event.mimeData.formats[i]);

                if (format === "application/x-orgkdeplasmataskmanager_taskbuttonitem"
                        || format.indexOf("plasmataskmanager") >= 0) {
                    return true;
                }
            }

            return false;
        }

        function clearDroppingFlags() {
            inDroppingFiles = false;
            inDroppingOnlyLaunchers = false;
            inDroppingSeparator = false;
            inMovingTask = false;
        }

        onHoveredItemChanged: {
            if (hoveredItem && windowsPreviewDlg.activeItem && hoveredItem !== windowsPreviewDlg.activeItem ) {
                windowsPreviewDlg.hide(6.7);
            }
        }

        function onDragEnter(event) {
            inMovingTask = isMovingTask(event);
            inDroppingOnlyLaunchers = !inMovingTask && isDroppingOnlyLaunchers(event);
            inDroppingSeparator = !inMovingTask && isDroppingSeparator(event);
            inDroppingFiles = !inDroppingOnlyLaunchers && event.mimeData.hasUrls;

            /*console.log(" tasks moving task :: " + inMovingTask);
            console.log(" tasks only launchers :: " + inDroppingOnlyLaunchers);
            console.log(" tasks separator :: " + inDroppingSeparator);
            console.log(" tasks only files :: " + inDroppingFiles);
            console.log(" tasks event accepted :: " + eventIsAccepted);*/

            if (!eventIsAccepted) {
                clearDroppingFlags();
                event.ignore();
                return;
            }

            dArea.containsDrag = true;
        }

        function onDragMove(event) {
            if (!eventIsAccepted) {
                clearDroppingFlags();
                event.ignore();
                return;
            }

            dArea.containsDrag = true;

            if (target.animating) {
                return;
            }

            var eventToTarget = mapToItem(target, event.x, event.y);

            var above = target.childAtPos(eventToTarget.x, eventToTarget.y, root.dragSource);

            // If we're mixing launcher tasks with other tasks and are moving
            // a (small) launcher task across a non-launcher task, don't allow
            // the latter to be the move target twice in a row for a while, as
            // it will naturally be moved underneath the cursor as result of the
            // initial move, due to being far larger than the launcher delegate.
            // TODO: This restriction (minus the timer, which improves things)
            // has been proven out in the EITM fork, but could be improved later
            // by tracking the cursor movement vector and allowing the drag if
            // the movement direction has reversed, establishing user intent to
            // move back.
            if (root.dragSource == null
                    && ignoredItem == above)
                return;

            if (root.dragSource != null
                    && root.dragSource.m.IsLauncher === true && above != null
                    && above.m != null
                    && above.m.IsLauncher !== true && above == ignoredItem) {
                return;
            } else {
                //ignoredItem = null;
            }

            //at some point it was needed the following  && above != ignoredItem
            //but know not... strange... && above != ignoredItem
            //I use the ignoredItem in order to reduce the move calls as much
            //as possible
            if (tasksModel.sortMode == TaskManager.TasksModel.SortManual && root.dragSource && ignoredItem == null) {
                dArea.reorderFromDragPosition(root.dragSource, event.x, event.y);
            } else if (!root.dragSource && above && hoveredItem != above) {
                hoveredItem = above;
                activationTimer.restart();
            } else if (!above) {
                hoveredItem = null;
                activationTimer.stop();
            }

            if (hoveredItem && windowsPreviewDlg.visible && toolTipDelegate.rootIndex !== hoveredItem.modelIndex() ) {
                windowsPreviewDlg.hide(6);
            }
        }

        onDragLeave: {
            dArea.containsDrag = false;
            if (dragParabolicItem && dragParabolicItem.clearParabolicFromExternalPosition) {
                dragParabolicItem.clearParabolicFromExternalPosition();
            }
            dragParabolicItem = null;
            hoveredItem = null;
            clearDroppingFlags();

            activationTimer.stop();
        }

        function onDrop(event) {
            if (!eventIsAccepted) {
                clearDroppingFlags();
                event.ignore();
                return;
            }

            // Reject internal drops.
            dArea.containsDrag = false;
            if (dragParabolicItem && dragParabolicItem.clearParabolicFromExternalPosition) {
                dragParabolicItem.clearParabolicFromExternalPosition();
            }
            dragParabolicItem = null;

            if (inDroppingSeparator) {
                if (hoveredItem && hoveredItem.itemIndex >=0){
                    appletAbilities.launchers.addInternalSeparatorAtPos(hoveredItem.itemIndex);
                } else {
                    appletAbilities.launchers.addInternalSeparatorAtPos(0);
                }
            } else if (inDroppingOnlyLaunchers || inDroppingFiles) {
                parent.urlsDropped(event.mimeData.urls);
            }

            clearDroppingFlags();
        }

        Timer {
            id: activationTimer

            interval: 250
            repeat: false

            onTriggered: {
                if (dropHandler.inDroppingOnlyLaunchers || dropHandler.inDroppingSeparator) {
                    return;
                }

                if (parent.hoveredItem.m.IsGroupParent === true) {
                    root.showPreviewForTasks(parent.hoveredItem);
                    // groupDialog.visualParent = parent.hoveredItem;
                    // groupDialog.visible = true;
                } else if (parent.hoveredItem.m.IsLauncher !== true) {
                    if(windowsPreviewDlg.visible && toolTipDelegate.currentItem !==parent.hoveredItem.itemIndex ) {
                        windowsPreviewDlg.hide(5);
                        toolTipDelegate.currentItem=-1;
                    }

                    tasksModel.requestActivate(parent.hoveredItem.modelIndex());
                }
            }
        }
    }
    /*
    MouseArea {
        id: wheelHandler

        anchors.fill: parent
        property int wheelDelta: 0;
        enabled: plasmoid.configuration.wheelEnabled

        onWheel: wheelDelta = TaskTools.wheelActivateNextPrevTask(wheelDelta, wheel.angleDelta.y);
    } */
}
