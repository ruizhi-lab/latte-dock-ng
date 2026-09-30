/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick

import org.kde.plasma.core as PlasmaCore

import org.kde.latte.core as LatteCore
import org.kde.latte.private.tasks as LatteTasks

MouseArea {
    id: taskMouseArea
    anchors.fill: parent
    acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
    // Qt6 can let parent flickables steal mouse sequences during small motion,
    // resulting in press without release and missed task activation.
    preventStealing: true

    // Keep hover enabled during launcher bounce so parabolic zoom continues
    // tracking mouse distance, but still guard separators and startup items
    // which should never participate in hover-triggered behaviour.
    hoverEnabled: taskItem.visible && !isStartup && !isSeparator

    // Preserve the task API's pressed state independently of QQuickMouseArea.pressed.
    // qmllint disable property-override
    property bool pressed: false
    // qmllint enable property-override
    // Drag should start only after resistance delay expires and pointer
    // moved enough. This avoids getting stuck in drag state on canceled clicks.
    property bool dragReady: false

    readonly property alias hoveredTimer: _hoveredTimer

    Connections {
        target: taskMouseArea
        function onPressed(mouse) { taskItem.mousePressed(mouse.x, mouse.y, mouse.button) }
        function onReleased(mouse) { taskItem.mouseReleased(mouse.x, mouse.y, mouse.button) }
    }

    onEntered: {
        if (isLauncher && windowsPreviewDlg.visible) {
            windowsPreviewDlg.hide(1);
        }

        //! show previews if enabled
        // Avoid preview-state checks entirely when both preview and window
        // highlighting are disabled. Tooltip and task auto-scroll handling
        // below remain active in that configuration.
        if((root.showPreviews || root.highlightWindows)
                && isAbleToShowPreview && !showPreviewsIsBlockedFromReleaseEvent && !isLauncher
                && (((root.showPreviews || (windowsPreviewDlg.visible && !isLauncher))
                     && windowsPreviewDlg.activeItem !== taskItem)
                    || root.highlightWindows)){

            if (!root.disableAllWindowsFunctionality) {
                //! don't delay showing preview in normal states,
                //! that is when the dock wasn't hidden
                if (!hoveredTimer.running && !windowsPreviewDlg.visible) {
                    //! first task with no previews shown can trigger the delay
                    hoveredTimer.start();
                } else if (windowsPreviewDlg.visible) {
                    //! when the previews are already shown, update them immediately
                    taskItem.showPreviewWindow();

                }
            }
        }

        taskItem.showPreviewsIsBlockedFromReleaseEvent = false;

        if (root.autoScrollTasksEnabled) {
            scrollableList.autoScrollFor(taskItem, false);
        }
    }

    onExited: {
        pressed = false;
        dragReady = false;
        taskItem.clearParabolicFromExternalPosition();
        taskItem.isAbleToShowPreview = true;

        if (root.showPreviews) {
            root.hidePreview(17.5);
        }
    }

    onPositionChanged: (mouse) => {
        if (taskItem.abilities.myView.isReady && !taskItem.abilities.myView.isShownFully) {
            return;
        }

        // Hover moves do not affect task dragging.  Skip the remaining work
        // until a press establishes a drag origin or a drag is already active.
        if (pressX === -1 && !taskItem.isDragged) {
            return;
        }

        if((inAnimation == false)&&(!root.taskInAnimation)&&(!root.disableRestoreZoom) && hoverEnabled){
            // In Qt6, move events can report inconsistent mouse.buttons during an
            // active press/drag sequence. Fall back to our pressed state.
            if (pressX != -1 && ((mouse.buttons & Qt.LeftButton) || pressed)
                    && dragReady
                    && !taskItem.isDragged
                    && (Math.abs(pressX - mouse.x) + Math.abs(pressY - mouse.y) >= Qt.styleHints.startDragDistance) ) {
                // Mark real drag start; TaskItem handles async drag image setup.
                taskItem.isDragged = true;
                dragReady = false;
            }

            // Keep local fallback active during the whole drag session.
            // On some Qt6/Wayland setups DropArea drag-move callbacks are sparse;
            // this path guarantees continuous reordering as the pointer moves.
            if (taskItem.isDragged && mouseHandler.reorderFromDragPosition) {
                var posInMouseHandler = taskMouseArea.mapToItem(mouseHandler, mouse.x, mouse.y);
                mouseHandler.reorderFromDragPosition(taskItem, posInMouseHandler.x, posInMouseHandler.y);
            }
        }
    }

    onContainsMouseChanged:{
        if(!containsMouse) {
            pressed=false;
            dragReady = false;
            taskItem.clearParabolicFromExternalPosition();
        }

    }

    // The root combines the Latte bridge, Plasma configuration and the
    // containment's direct assignment. Reading that notifying state avoids a
    // second idle poll and keeps click/wheel guards aligned with edit changes.
    readonly property bool _containmentEditing: root.inEditMode

    function isContainmentEditing() {
        return _containmentEditing;
    }

    onPressed: (mouse) => {
        //console.log("Pressed Task Delegate..");
        slotPublishGeometries();

        var modAccepted = modifierAccepted(mouse);

        if ((mouse.button == Qt.LeftButton)||(mouse.button == Qt.MiddleButton) || modAccepted) {
            lastButtonClicked = mouse.button;
            pressed = true;
            dragReady = false;
            pressX = mouse.x;
            pressY = mouse.y;

            if(!modAccepted){
                _resistanerTimer.start();
            }
        } else if (mouse.button === Qt.RightButton && !modAccepted && !isContainmentEditing()) {
            // When we're a launcher, there's no window controls, so we can show all
            // places without the menu getting super huge.
            if (model.IsLauncher === true && !isSeparator) {
                showContextMenu({showAllPlaces: true})
            } else {
                showContextMenu();
            }
        }
    }

    onReleased: (mouse) => {
        //console.log("Released Task Delegate...");
        _resistanerTimer.stop();
        dragReady = false;

        if (!isContainmentEditing()
                && pressed
                && !isSeparator
                && !(taskItem.isDragged || dragHelper.Drag.active || root.dragSource === taskItem)) {

            if (modifierAccepted(mouse) && !root.disableAllWindowsFunctionality){
                if( !taskItem.isLauncher ){
                    if (root.modifierClickAction == LatteTasks.types.Close) {
                        tasksModel.requestClose(modelIndex());
                    } else if (root.modifierClickAction == LatteTasks.types.NewInstance) {
                        tasksModel.requestNewInstance(modelIndex());
                    } else if (root.modifierClickAction == LatteTasks.types.ToggleMinimized) {
                        tasksModel.requestToggleMinimized(modelIndex());
                    } else if (root.modifierClickAction == LatteTasks.types.CycleThroughTasks) {
                        if (isGroupParent)
                            subWindows.activateNextTask();
                        else
                            activateTask();
                    } else if (root.modifierClickAction == LatteTasks.types.ToggleGrouping) {
                        tasksModel.requestToggleGrouping(modelIndex());
                    } else if (root.modifierClickAction == LatteTasks.types.PresentWindows) {
                        taskItem.presentWindows();
                    } else if (root.modifierClickAction == LatteTasks.types.PreviewWindows) {
                        if (isGroupParent)
                            subWindows.activateNextTask();
                        else
                            activateTask();
                    } else if (root.modifierClickAction == LatteTasks.types.HighlightWindows) {
                        root.windowsHovered(model.WinIdList, true);
                    } else if (root.modifierClickAction == LatteTasks.types.PreviewAndHighlightWindows) {
                        if (isGroupParent)
                            subWindows.activateNextTask();
                        else
                            activateTask();
                        root.windowsHovered(model.WinIdList, true);
                    }
                    // NoneAction: do nothing
                } else {
                    activateTask();
                }
            } else if (mouse.button == Qt.MiddleButton && !root.disableAllWindowsFunctionality){
                if( !taskItem.isLauncher ){
                    if (root.middleClickAction == LatteTasks.types.Close) {
                        tasksModel.requestClose(modelIndex());
                    } else if (root.middleClickAction == LatteTasks.types.NewInstance) {
                        tasksModel.requestNewInstance(modelIndex());
                    } else if (root.middleClickAction == LatteTasks.types.ToggleMinimized) {
                        tasksModel.requestToggleMinimized(modelIndex());
                    } else if ( root.middleClickAction == LatteTasks.types.CycleThroughTasks) {
                        if (isGroupParent)
                            subWindows.activateNextTask();
                        else
                            activateTask();
                    } else if (root.middleClickAction == LatteTasks.types.ToggleGrouping) {
                        tasksModel.requestToggleGrouping(modelIndex());
                    } else if (root.middleClickAction == LatteTasks.types.PresentWindows) {
                        taskItem.presentWindows();
                    } else if (root.middleClickAction == LatteTasks.types.PreviewWindows) {
                        if (isGroupParent)
                            subWindows.activateNextTask();
                        else
                            activateTask();
                    } else if (root.middleClickAction == LatteTasks.types.HighlightWindows) {
                        root.windowsHovered(model.WinIdList, true);
                    } else if (root.middleClickAction == LatteTasks.types.PreviewAndHighlightWindows) {
                        if (isGroupParent)
                            subWindows.activateNextTask();
                        else
                            activateTask();
                        root.windowsHovered(model.WinIdList, true);
                    }
                    // NoneAction: do nothing
                } else {
                    activateTask();
                }
            } else if (mouse.button == Qt.LeftButton){
                if (taskItem.isLauncher || root.disableAllWindowsFunctionality) {
                    activateTask();
                } else if (root.leftClickAction === LatteTasks.types.Close) {
                    tasksModel.requestClose(modelIndex());
                } else if (root.leftClickAction === LatteTasks.types.NewInstance) {
                    tasksModel.requestNewInstance(modelIndex());
                } else if (root.leftClickAction === LatteTasks.types.ToggleMinimized) {
                    tasksModel.requestToggleMinimized(modelIndex());
                } else if (root.leftClickAction === LatteTasks.types.CycleThroughTasks) {
                    if (isGroupParent) {
                        subWindows.activateNextTask();
                    } else {
                        activateTask();
                    }
                } else if (root.leftClickAction === LatteTasks.types.ToggleGrouping) {
                    tasksModel.requestToggleGrouping(modelIndex());
                } else if (root.leftClickAction === LatteTasks.types.PresentWindows) {
                    taskItem.presentWindows();
                } else if (root.leftClickAction === LatteTasks.types.PreviewWindows) {
                    if (isGroupParent) {
                        subWindows.activateNextTask();
                    } else {
                        activateTask();
                    }
                } else if (root.leftClickAction === LatteTasks.types.HighlightWindows) {
                    root.windowsHovered(model.WinIdList, true);
                } else if (root.leftClickAction === LatteTasks.types.PreviewAndHighlightWindows) {
                    if (isGroupParent) {
                        subWindows.activateNextTask();
                    } else {
                        activateTask();
                    }
                    root.windowsHovered(model.WinIdList, true);
                }
                // NoneAction or any unhandled action: do nothing
            }

            root.cancelHighlightWindows();
        }

        pressed = false;
    }

    onCanceled: {
        // Keep internal press/drag state sane when event ownership changes.
        _resistanerTimer.stop();
        dragReady = false;
        pressed = false;
        pressX = -1;
        pressY = -1;

        // When a real DnD session is active, cancellation is expected because
        // pointer grab moves to drag handling. Keep drag state in that case only.
        if (dragHelper.Drag.active) {
            return;
        }

        taskItem.isDragged = false;
    }

    onWheel: (wheel) => {
        var wheelActionsEnabled = (root.taskScrollAction !== LatteTasks.types.ScrollNone || root.manualScrollTasksEnabled);

        if (isContainmentEditing()
                || isSeparator
                || wheelIsBlocked
                || !wheelActionsEnabled
                || inBouncingAnimation
                || !taskItem.abilities.myView.isShownFully){

            return;
        }

        var angleVertical = wheel.angleDelta.y / 8;
        var angleHorizontal = wheel.angleDelta.x / 8;

        wheelIsBlocked = true;
        scrollDelayer.start();

        var verticalDirection = (Math.abs(angleVertical) > Math.abs(angleHorizontal));
        var mainAngle = verticalDirection ? angleVertical : angleHorizontal;

        var positiveDirection = (mainAngle > 12);
        var negativeDirection = (mainAngle < -12);

        var parallelScrolling = (verticalDirection && plasmoid.formFactor === PlasmaCore.Types.Vertical)
                || (!verticalDirection && plasmoid.formFactor === PlasmaCore.Types.Horizontal);

        if (positiveDirection) {
            slotPublishGeometries();

            var overflowScrollingAccepted = (root.manualScrollTasksEnabled
                                             && scrollableList.contentsExceed
                                             && (root.manualScrollTasksType === LatteTasks.types.ManualScrollVerticalHorizontal
                                                 || (root.manualScrollTasksType === LatteTasks.types.ManualScrollOnlyParallel && parallelScrolling)) );


            if (overflowScrollingAccepted) {
                scrollableList.decreasePos();
            } else {
                if (isLauncher || root.disableAllWindowsFunctionality) {
                    taskItem.activateLauncher();
                } else if (isGroupParent) {
                    subWindows.activateNextTask();
                } else {
                    var taskIndex = modelIndex();

                    if (isMinimized) {
                        tasksModel.requestToggleMinimized(taskIndex);
                    }

                    tasksModel.requestActivate(taskIndex);
                }

                // hidePreviewWindow();
            }
        } else if (negativeDirection) {
            slotPublishGeometries();

            var overflowScrollingAccepted = (root.manualScrollTasksEnabled
                                             && scrollableList.contentsExceed
                                             && (root.manualScrollTasksType === LatteTasks.types.ManualScrollVerticalHorizontal
                                                 || (root.manualScrollTasksType === LatteTasks.types.ManualScrollOnlyParallel && parallelScrolling)) );


            if (overflowScrollingAccepted) {
                scrollableList.increasePos();
            } else {
                if (isLauncher || root.disableAllWindowsFunctionality) {
                    // do nothing
                } else if (isGroupParent) {
                    if (root.taskScrollAction === LatteTasks.types.ScrollToggleMinimized) {
                        subWindows.minimizeTask();
                    } else {
                        subWindows.activatePreviousTask();
                    }
                } else {
                    var taskIndex = modelIndex();

                    var hidingTask = (!isMinimized && root.taskScrollAction === LatteTasks.types.ScrollToggleMinimized);

                    if (isMinimized || hidingTask) {
                        tasksModel.requestToggleMinimized(taskIndex);
                    }

                    if (!hidingTask) {
                        tasksModel.requestActivate(taskIndex);
                    }
                }

                // hidePreviewWindow();
            }
        }
    }

    //A Timer to check how much time the task is hovered in order to check if we must
    //show window previews
    Timer {
        id: _hoveredTimer
        interval: Math.max(150,plasmoid.configuration.previewsDelay)
        repeat: false

        onTriggered: {
            if (root.disableAllWindowsFunctionality || !isAbleToShowPreview) {
                return;
            }

            if (taskItem.containsMouse) {
                if (root.showPreviews || (windowsPreviewDlg.visible && !isLauncher)) {
                    taskItem.showPreviewWindow();
                }

            }
        }
    }

    //A Timer to help in resist a bit to dragging, the user must try
    //to press a little first before dragging Started
    Timer {
        id: _resistanerTimer
        interval: taskItem.resistanceDelay
        repeat: false

        onTriggered: {
            if (!taskItem.inBlockingAnimation){
                taskMouseArea.dragReady = true;
            }

            if (taskItem.abilities.debug.timersEnabled) {
                console.log("plasmoid timer: resistanerTimer called...");
            }
        }
    }

}
