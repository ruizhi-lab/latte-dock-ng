/*
    SPDX-FileCopyrightText: 2016 Smith AR <audoban@openmailbox.org>
    SPDX-FileCopyrightText: 2016 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.8
import QtQuick.Layouts 1.1

import QtQuick.Effects

import org.kde.kirigami 2.20 as Kirigami
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.ksvg 1.0 as KSvg
import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.private.mpris as Mpris

import org.kde.taskmanager 0.1 as TaskManager
import org.kde.plasma.private.taskmanager 0.1 as TaskManagerApplet

import org.kde.latte.core 0.2 as LatteCore
import org.kde.latte.components 1.0 as LatteComponents

import org.kde.latte.private.tasks 0.1 as LatteTasks

import "abilities" as Ability
import "previews" as Previews
import "task" as Task
import "taskslayout" as TasksLayout
import "../code/tools.js" as TaskTools
import "../code/activitiesTools.js" as ActivitiesTools
import "../code/ColorizerTools.js" as ColorizerTools

PlasmoidItem {
    id:root
    readonly property var theme: Kirigami.Theme
    readonly property var units: Kirigami.Units

    Layout.fillWidth: scrollingEnabled && !root.vertical
    Layout.fillHeight: scrollingEnabled && root.vertical

    Layout.minimumWidth: inPlasma && root.isHorizontal ? minimumLength : -1
    Layout.minimumHeight: inPlasma && !root.isHorizontal ? minimumLength : -1
    Layout.preferredWidth: tasksWidth
    Layout.preferredHeight: tasksHeight
    Layout.maximumWidth: -1
    Layout.maximumHeight: -1

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft && !root.vertical
    LayoutMirroring.childrenInherit: true

    property bool disableRestoreZoom: false //blocks restore animation in rightClick
    property bool disableAllWindowsFunctionality: plasmoid.configuration.hideAllTasks
    property bool inActivityChange: false
    property bool inDraggingPhase: false
    property bool initializationStep: false //true
    property bool isHovered: false
    property bool showBarLine: plasmoid.configuration.showBarLine
    property bool useThemePanel: plasmoid.configuration.useThemePanel
    property bool taskInAnimation: noTasksInAnimation > 0 ? true : false
    property bool transparentPanel: plasmoid.configuration.transparentPanel
    property bool vertical: plasmoid.formFactor === PlasmaCore.Types.Vertical ? true : false
    property bool isHorizontal: plasmoid.formFactor === PlasmaCore.Types.Horizontal ? true : false

    property bool hasTaskDemandingAttention: false

    property int clearWidth
    property int clearHeight

    property int newDroppedPosition: -1
    property int noTasksInAnimation: 0
    property int themePanelSize: plasmoid.configuration.panelSize

    property int location : {
        if (plasmoid.location === PlasmaCore.Types.LeftEdge
                || plasmoid.location === PlasmaCore.Types.RightEdge
                || plasmoid.location === PlasmaCore.Types.TopEdge) {
            return plasmoid.location;
        }

        return PlasmaCore.Types.BottomEdge;
    }

    ///Don't use Math.floor it adds one pixel in animations and creates glitches
    property int widthMargins: root.vertical ? appletAbilities.metrics.totals.thicknessEdges : appletAbilities.metrics.totals.lengthEdges
    property int heightMargins: !root.vertical ? appletAbilities.metrics.totals.thicknessEdges : appletAbilities.metrics.totals.lengthEdges

    property int internalWidthMargins: root.vertical ? appletAbilities.metrics.totals.thicknessEdges : appletAbilities.metrics.totals.lengthPaddings
    property int internalHeightMargins: !root.vertical ? appletAbilities.metrics.totals.thicknessEdges : appletAbilities.metrics.totals.lengthPaddings

    readonly property int minimumLength: inPlasma ? (root.isHorizontal ? tasksWidth : tasksHeight) : -1;

    property real textColorBrightness: ColorizerTools.colorBrightness(themeTextColor)

    property color themeTextColor: theme.textColor
    property color themeBackgroundColor: theme.backgroundColor

    property color lightTextColor: textColorBrightness > 127.5 ? themeTextColor : themeBackgroundColor

    //a small badgers record (id,value)
    //in order to track badgers when there are changes
    //in launcher reference from libtaskmanager
    property variant badgers:[]
    property variant launchersOnActivities: []

    //global plasmoid reference to the context menu
    property QtObject contextMenu: null
    property QtObject contextMenuComponent: Qt.createComponent("ContextMenu.qml", Component.PreferSynchronous);
    property Item dragSource: null

    property Item tasksExtendedManager: _tasksExtendedManager
    readonly property alias appletAbilities: _appletAbilities

    readonly property alias containsDrag: mouseHandler.containsDrag

    //! Animations
    readonly property bool launcherBouncingEnabled: appletAbilities.animations.active && plasmoid.configuration.animationLauncherBouncing
    readonly property bool newWindowSlidingEnabled: appletAbilities.animations.active && plasmoid.configuration.animationNewWindowSliding
    readonly property bool windowInAttentionEnabled: appletAbilities.animations.active && plasmoid.configuration.animationWindowInAttention
    readonly property bool windowAddedInGroupEnabled: appletAbilities.animations.active && plasmoid.configuration.animationWindowAddedInGroup
    readonly property bool windowRemovedFromGroupEnabled: appletAbilities.animations.active && plasmoid.configuration.animationWindowRemovedFromGroup

    readonly property bool hasHighThicknessAnimation: launcherBouncingEnabled || windowInAttentionEnabled || windowAddedInGroupEnabled

    //BEGIN properties
    property bool groupTasksByDefault: plasmoid.configuration.groupTasksByDefault
    property bool highlightWindows: hoverAction === LatteTasks.types.HighlightWindows || hoverAction === LatteTasks.types.PreviewAndHighlightWindows

    property bool scrollingEnabled: plasmoid.configuration.scrollTasksEnabled
    property bool autoScrollTasksEnabled: scrollingEnabled && plasmoid.configuration.autoScrollTasksEnabled
    property bool manualScrollTasksEnabled: scrollingEnabled &&  manualScrollTasksType !== LatteTasks.types.ManualScrollDisabled
    property int manualScrollTasksType: plasmoid.configuration.manualScrollTasksType

    property bool showInfoBadge: plasmoid.configuration.showInfoBadge
    property bool showProgressBadge: plasmoid.configuration.showProgressBadge
    property bool showAudioBadge: plasmoid.configuration.showAudioBadge
    property bool infoBadgeProminentColorEnabled: plasmoid.configuration.infoBadgeProminentColorEnabled
    property bool audioBadgeActionsEnabled: plasmoid.configuration.audioBadgeActionsEnabled
    property bool showOnlyCurrentScreen: plasmoid.configuration.showOnlyCurrentScreen
    property bool showOnlyCurrentDesktop: plasmoid.configuration.showOnlyCurrentDesktop
    property bool showOnlyCurrentActivity: plasmoid.configuration.showOnlyCurrentActivity
    // Activity id used only for filter enablement.
    // Keep this empty when runtime activities are unavailable to avoid
    // accidental over-filtering to a stale persisted activity id.
    property string tasksModelActivityId: {
        if (!root.showOnlyCurrentActivity) {
            return "";
        }

        // When KActivities is not available, ActivityInfo cannot provide a valid
        // runtime activity id. In that case filtering by a stale persisted id can
        // hide all runtime windows, so disable activity filtering.
        if (activityInfo.numberOfRunningActivities <= 0) {
            return "";
        }

        const currentActivityId = activityInfo.currentActivity;

        if (root.isFilterableActivityId(currentActivityId)) {
            return String(currentActivityId);
        }

        return "";
    }
    property bool shouldFilterByActivity: root.showOnlyCurrentActivity && root.tasksModelActivityId.length > 0
    property bool showPreviews:  hoverAction === LatteTasks.types.PreviewWindows || hoverAction === LatteTasks.types.PreviewAndHighlightWindows
    property bool showWindowActions: plasmoid.configuration.showWindowActions && !disableAllWindowsFunctionality
    property bool showWindowsOnlyFromLaunchers: plasmoid.configuration.showWindowsOnlyFromLaunchers && !disableAllWindowsFunctionality

    property alias windowPreviewIsShown: windowsPreviewDlg.visible

    property int leftClickAction: plasmoid.configuration.leftClickAction
    property int middleClickAction: plasmoid.configuration.middleClickAction
    property int hoverAction: plasmoid.configuration.hoverAction
    property int modifier: plasmoid.configuration.modifier
    property int modifierClickAction: plasmoid.configuration.modifierClickAction
    property int modifierClick: plasmoid.configuration.modifierClick
    property int modifierQt:{
        if (modifier === LatteTasks.types.Shift)
            return Qt.ShiftModifier;
        else if (modifier === LatteTasks.types.Ctrl)
            return Qt.ControlModifier;
        else if (modifier === LatteTasks.types.Alt)
            return Qt.AltModifier;
        else if (modifier === LatteTasks.types.Meta)
            return Qt.MetaModifier;
        else return -1;
    }
    property int taskScrollAction: plasmoid.configuration.taskScrollAction

    onTaskScrollActionChanged: {
        if (taskScrollAction > LatteTasks.types.ScrollToggleMinimized) {
            //! migrating scroll action to LatteTasks.types.ScrollAction
            plasmoid.configuration.taskScrollAction = plasmoid.configuration.taskScrollAction-LatteTasks.types.ScrollToggleMinimized;
        }
    }

    //! Real properties are need in order for parabolic effect to be 1px precise perfect.
    //! This way moving from Tasks to Applets and vice versa is pretty stable when hovering with parabolic effect.
    property real tasksHeight:  mouseHandler.height
    property real tasksWidth: mouseHandler.width
    property real tasksLength: root.vertical ? mouseHandler.height : mouseHandler.width

    readonly property int alignment: appletAbilities.containment.alignment

    property alias tasksCount: tasksModel.count

    //END Latte Dock Panel properties

    readonly property bool inEditMode: latteInEditMode || plasmoid.userConfiguring

    //BEGIN Latte Dock Communicator
    property QtObject latteBridge: null

    // Bridge attachment can be transient during startup in Qt6. When this plasmoid
    // is persisted as part of a Latte layout, isInLatteDock keeps behavior stable.
    readonly property bool inLatteDockEnvironment: (latteBridge !== null) || plasmoid.configuration.isInLatteDock
    readonly property bool inPlasma: !inLatteDockEnvironment
    readonly property bool inPlasmaDesktop: inPlasma && !inPlasmaPanel
    readonly property bool inPlasmaPanel: inPlasma && (plasmoid.location === PlasmaCore.Types.LeftEdge
                                                       || plasmoid.location === PlasmaCore.Types.RightEdge
                                                       || plasmoid.location === PlasmaCore.Types.BottomEdge
                                                       || plasmoid.location === PlasmaCore.Types.TopEdge)
    readonly property bool latteInEditMode: latteBridge && latteBridge.inEditMode
    //END  Latte Dock Communicator

    preferredRepresentation: fullRepresentation
    Plasmoid.backgroundHints: inPlasmaDesktop ? PlasmaCore.Types.StandardBackground : PlasmaCore.Types.NoBackground

    signal draggingFinished();
    signal hiddenTasksUpdated();
    signal activateWindowView(variant winIds);
    signal requestLayout;
    signal signalPreviewsShown();
    //signal signalDraggingState(bool value);
    signal showPreviewForTasks(QtObject group);
    //trigger updating scaling of neighbour delegates of zoomed delegate
    signal updateScale(int delegateIndex, real newScale, real step)
    signal publishTasksGeometries();
    signal windowsHovered(variant winIds, bool hovered)


    onScrollingEnabledChanged: {
        updateListViewParent();
    }

    Connections {
        target: plasmoid
        function onLocationChanged() {
            iconGeometryTimer.start();
        }
    }

    Connections {
        target: plasmoid.configuration

        // onLaunchersChanged: tasksModel.launcherList = plasmoid.configuration.launchers
        function onGroupingAppIdBlacklistChanged() { tasksModel.groupingAppIdBlacklist = plasmoid.configuration.groupingAppIdBlacklist; }
        function onGroupingLauncherUrlBlacklistChanged() { tasksModel.groupingLauncherUrlBlacklist = plasmoid.configuration.groupingLauncherUrlBlacklist; }
    }


    Connections {
        target: appletAbilities.myView
        function onIsHiddenChanged() {
            if (appletAbilities.myView.isHidden) {
                windowsPreviewDlg.hide("3.3");
            }
        }

        function onIsReadyChanged() {
            if (appletAbilities.myView.isReady) {
                // Plasma 6 dropped JS plasmoid.action(); use internalAction()
                // and tolerate older builds that may not expose it yet.
                var configureAction = null;
                if (typeof Plasmoid !== "undefined" && Plasmoid) {
                    if (typeof Plasmoid.internalAction === "function") {
                        configureAction = Plasmoid.internalAction("configure");
                    } else if (typeof Plasmoid.action === "function") {
                        configureAction = Plasmoid.action("configure");
                    }
                }
                if (configureAction) {
                    configureAction.visible = false;
                }
                plasmoid.configuration.isInLatteDock = true;
            }
        }
    }

    Binding {
        target: plasmoid
        property: "status"
        value: {
            var hastaskinattention = root.hasTaskDemandingAttention && tasksModel.anyTaskDemandsAttentionInValidTime;
            return (hastaskinattention || root.dragSource) ? PlasmaCore.Types.NeedsAttentionStatus : PlasmaCore.Types.PassiveStatus;
        }
    }

    Binding {
        target: root
        property: "hasTaskDemandingAttention"
        when: appletAbilities.indexer.isReady
        value: {
            for (var i=0; i<appletAbilities.indexer.layout.children.length; ++i){
                var item = appletAbilities.indexer.layout.children[i];
                if (item && item.isDemandingAttention) {
                    return true;
                }
            }

            return false;
        }
    }

    ///UPDATE
    function updateListViewParent() {
        if (scrollingEnabled) {
            icList.parent = listViewBase;
        } else {
            icList.parent = barLine;
        }
    }

    function launcherExists(url) {
        return (ActivitiesTools.getIndex(url, tasksModel.launcherList)>=0);
    }

    function taskExists(url) {
        var tasks = icList.contentItem.children;
        for(var i=0; i<tasks.length; ++i){
            var task = tasks[i];

            if (task.launcherUrl===url && task.isWindow) {
                return true;
            }
        }
        return false;
    }

    function isFilterableActivityId(activityId) {
        if (activityId === undefined || activityId === null) {
            return false;
        }

        const normalizedId = String(activityId);

        if (normalizedId.length === 0) {
            return false;
        }

        return normalizedId !== "{0}"
                && normalizedId !== "{free-activities}"
                && normalizedId !== "{current-activity}"
                && normalizedId !== "00000000-0000-0000-0000-000000000000";
    }


    function forcePreviewsHiding(debug) {
        // console.log(" org.kde.latte   Tasks: Force hide previews event called: "+debug);
        windowsPreviewDlg.activeItem = null;
        windowsPreviewDlg.visible = false;
    }

    function hidePreview(){
        windowsPreviewDlg.hide(11);
    }

    onDragSourceChanged: {
        if (dragSource == null) {
            root.draggingFinished();
            tasksModel.syncLaunchers();

            restoreDraggingPhaseTimer.start();
        } else {
            inDraggingPhase = true;
        }
    }

    /////Window previews///////////

    Previews.ToolTipDelegate2 {
        id: toolTipDelegate
        visible: false
    }

    ////BEGIN interfaces

    LatteCore.Dialog{
        id: windowsPreviewDlg
        type: plasmoid.configuration.previewWindowAsPopup ? PlasmaCore.Dialog.PopupMenu : PlasmaCore.Dialog.Tooltip
        flags: plasmoid.configuration.previewWindowAsPopup ? Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus | Qt.Popup :
                                                             Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus | Qt.ToolTip
        location: root.location
        edge: root.location
        mainItem: toolTipDelegate
        visible: false

        property bool signalSent: false
        property Item activeItem: null

        Component.onCompleted: mainItem.visible = true;

        onContainsMouseChanged: {
            //! Orchestrate restore zoom and previews window hiding. Both should be
            //! triggered together.
            if (containsMouse) {
                hidePreviewWinTimer.stop();
                appletAbilities.parabolic.setDirectRenderingEnabled(false);
            } else {
                hide(7.3);
            }
        }

        function hide(debug){
            //console.log("   Tasks: hide previews event called: "+debug);
            if (containsMouse || !visible) {
                return;
            }

            if (appletAbilities.myView.isReady && signalSent) {
                //it is used to unblock dock hiding
                signalSent = false;
            }

            if (!root.contextMenu) {
                root.disableRestoreZoom = false;
            }

            hidePreviewWinTimer.start();
        }

        function show(taskItem){
            if (root.disableAllWindowsFunctionality) {
                return;
            }

            hidePreviewWinTimer.stop();

            // console.log("preview show called...");
            if ((!activeItem || (activeItem !== taskItem)) && !root.contextMenu) {
                //console.log("preview show called: accepted...");

                //this can be used from others to hide their appearance
                //e.g but applets from the dock to hide themselves
                if (!visible) {
                    root.signalPreviewsShown();
                }

                activeItem = taskItem;
                toolTipDelegate.parentTask = taskItem;

                if (appletAbilities.myView.isReady && !signalSent) {
                    //it is used to block dock hiding
                    signalSent = true;
                }

                //! Workaround in order to update properly the previews thumbnails
                //! when switching between single thumbnail to another single thumbnail
                //! maybe is not needed any more, let's disable it
                //mainItem.visible = false;
                visible = true;
                //mainItem.visible = true;
            }
        }
    }

    //! Delay windows previews hiding
    Timer {
        id: hidePreviewWinTimer
        interval: 300
        onTriggered: {
            //! Orchestrate restore zoom and previews window hiding. Both should be
            //! triggered together.
            var contains = (windowsPreviewDlg.containsMouse
                            || (windowsPreviewDlg.activeItem && windowsPreviewDlg.activeItem.containsMouse) /*main task*/
                            || (windowsPreviewDlg.activeItem /*dragging file(s) from outside*/
                                && mouseHandler.hoveredItem
                                && !root.dragSource
                                && mouseHandler.hoveredItem === windowsPreviewDlg.activeItem));

            if (!contains) {
                root.forcePreviewsHiding(9.9);
            }
        }
    }

    //! Timer to fix #811, rare cases that both a window preview and context menu are
    //! shown. It is mostly used under wayland in order to avoid crashes. When the context
    //! menu will be shown there is a chance that previews window has already appeared in that
    //! case the previews window must become hidden
    Timer {
        id: windowsPreviewCheckerToNotShowTimer
        interval: 250

        onTriggered: {
            if (windowsPreviewDlg.visible && root.contextMenu) {
                windowsPreviewDlg.hide("8.2");
            }
        }
    }

    //! Timer to delay the removal of the window through the context menu in case the
    //! the window is zoomed
    Timer{
        id: delayWindowRemovalTimer
        //this is the animation time needed in order for tasks to restore their zoom first
        interval: 7 * (appletAbilities.animations.speedFactor.current * appletAbilities.animations.duration.small)

        property var modelIndex

        onTriggered: {
            tasksModel.requestClose(delayWindowRemovalTimer.modelIndex)

            if (appletAbilities.debug.timersEnabled) {
                console.log("plasmoid timer: delayWindowRemovalTimer called...");
            }
        }
    }

    Timer {
        id: activityChangeDelayer
        interval: 150
        onTriggered: {
            root.inActivityChange = false;
            root.publishTasksGeometries();
            activityInfo.previousActivity = activityInfo.currentActivity;

            if (appletAbilities.debug.timersEnabled) {
                console.log("plasmoid timer: activityChangeDelayer called...");
            }
        }
    }

    /////Window Previews/////////


    TaskManager.TasksModel {
        id: tasksModel

        virtualDesktop: virtualDesktopInfo.currentDesktop
        screenGeometry: appletAbilities.myView.screenGeometry
        activity: root.tasksModelActivityId

        filterByVirtualDesktop: root.showOnlyCurrentDesktop
        filterByScreen: root.showOnlyCurrentScreen
        filterByActivity: root.shouldFilterByActivity

        // Match Plasma Icons-Only Task Manager semantics:
        // a running app should replace its launcher representation.
        hideActivatedLaunchers: true
        launchInPlace: true
        separateLaunchers: true
        groupInline: false

        groupMode: groupTasksByDefault ? TaskManager.TasksModel.GroupApplications : TaskManager.TasksModel.GroupDisabled
        sortMode: TaskManager.TasksModel.SortManual
        taskReorderingEnabled: true

        property bool anyTaskDemandsAttentionInValidTime: false

        onActivityChanged: {
            ActivitiesTools.currentActivity = String(activity);
        }

        onGroupingAppIdBlacklistChanged: {
            plasmoid.configuration.groupingAppIdBlacklist = groupingAppIdBlacklist;
        }

        onGroupingLauncherUrlBlacklistChanged: {
            plasmoid.configuration.groupingLauncherUrlBlacklist = groupingLauncherUrlBlacklist;
        }

        onAnyTaskDemandsAttentionChanged: {
            anyTaskDemandsAttentionInValidTime = anyTaskDemandsAttention;

            if (anyTaskDemandsAttention){
                attentionTimer.start();
            } else {
                attentionTimer.stop();
            }
        }

        Component.onCompleted: {
            ActivitiesTools.launchersOnActivities = root.launchersOnActivities
            ActivitiesTools.currentActivity = String(activityInfo.currentActivity);
            ActivitiesTools.plasmoid = plasmoid;

            //var loadedLaunchers = ActivitiesTools.restoreLaunchers();
            ActivitiesTools.importLaunchersToNewArchitecture();

            appletAbilities.launchers.importLauncherListInModel();

            groupingAppIdBlacklist = plasmoid.configuration.groupingAppIdBlacklist;
            groupingLauncherUrlBlacklist = plasmoid.configuration.groupingLauncherUrlBlacklist;

            groupingWindowTasksThreshold = -1;
        }
    }

    //! TaskManagerBackend required a groupDialog setting otherwise it crashes. This patch
    //! sets one just in order not to crash TaskManagerBackend
    PlasmaCore.Dialog {
        //ghost group Dialog to not crash TaskManagerBackend
        id: groupDialogGhost
        visible: false

        type: PlasmaCore.Dialog.PopupMenu
        flags: Qt.WindowStaysOnTopHint
        hideOnWindowDeactivate: true
        location: root.location
    }


    TaskManagerApplet.Backend {
        id: backend
        taskManagerItem: root
        highlightWindows: root.highlightWindows

        onAddLauncher: {
            tasksModel.requestAddLauncher(url);
        }

        Component.onCompleted: {
            groupDialog = groupDialogGhost;
        }
    }

    Item {
        id: dragHelper
        width: 1
        height: 1

        Drag.dragType: Drag.Automatic
        Drag.supportedActions: Qt.CopyAction | Qt.MoveAction | Qt.LinkAction
        Drag.onDragFinished: root.dragSource = null;
    }

    TaskManager.VirtualDesktopInfo {
        id: virtualDesktopInfo
    }

    TaskManager.ActivityInfo {
        id: activityInfo

        property string previousActivity: ""
        onCurrentActivityChanged: {
            root.inActivityChange = true;
            activityChangeDelayer.start();
        }

        Component.onCompleted: previousActivity = currentActivity;
    }

    Mpris.Mpris2Model {
        id: mpris2Source
    }

    Loader {
        id: pulseAudio
        source: "PulseAudio.qml"
        active: root.showAudioBadge
    }

    TasksExtendedManager {
        id: _tasksExtendedManager
    }

    AppletAbilities {
        id: _appletAbilities
        bridge: latteBridge
        layout: icList.contentItem
        tasksModel: tasksModel

        animations.local.speedFactor.current: plasmoid.configuration.durationTime
        animations.local.requirements.zoomFactor: hasHighThicknessAnimation && LatteCore.WindowSystem.compositingActive ? 1.65 : 1.0

        indexer.updateIsBlocked: root.inDraggingPhase || root.inActivityChange || tasksExtendedManager.launchersInPausedStateCount>0

        // Keep a temporary fallback when latteBridge attachment is delayed.
        // Once bridge is ready in Latte, use bridge indicators only to avoid
        // duplicate indicator layers that can interfere with task interaction.
        indicators.local.isEnabled: !plasmoid.configuration.isInLatteDock || latteBridge === null

        launchers.group: plasmoid.configuration.launchersGroup
        launchers.isStealingDroppedLaunchers: plasmoid.configuration.isPreferredForDroppedLaunchers
        launchers.syncer.isBlocked: inDraggingPhase

        metrics.local.iconSize: inPlasmaDesktop ? maxIconSizeInPlasma : (inPlasmaPanel ? Math.max(16, panelThickness - metrics.margin.tailThickness - metrics.margin.headThickness) : maxIconSizeInPlasma)
        metrics.local.backgroundThickness: metrics.totals.thickness
        metrics.local.margin.length: 0.1 * metrics.iconSize
        metrics.local.margin.tailThickness: inPlasmaDesktop ? 0.16 * metrics.iconSize : Math.max(2, (panelThickness - maxIconSizeInPlasma) / 2)
        metrics.local.margin.headThickness: metrics.local.margin.tailThickness
        metrics.local.padding.length: 0.04 * metrics.iconSize

        myView.local.isHidingBlocked: root.contextMenu || root.windowPreviewIsShown
        myView.local.itemShadow.isEnabled: plasmoid.configuration.showShadows
        myView.local.itemShadow.size: Math.ceil(0.12*appletAbilities.metrics.iconSize)

        // Avoid self-referential bindings between parabolic.isEnabled and
        // parabolic.local.factor.zoom that can keep zoom disabled after startup.
        readonly property real localParabolicZoom: 1 + (plasmoid.configuration.zoomLevel / 20)
        readonly property bool localParabolicEnabled: (!root.inPlasma || root.inPlasmaDesktop) && localParabolicZoom > 1.0
        parabolic.local.isEnabled: localParabolicEnabled
        parabolic.local.factor.zoom: localParabolicEnabled ? localParabolicZoom : 1.0
        parabolic.local.factor.maxZoom: localParabolicEnabled ? Math.max(parabolic.local.factor.zoom, 1.6) : 1.0
        parabolic.local.restoreZoomIsBlocked: root.contextMenu || windowsPreviewDlg.containsMouse

        shortcuts.isStealingGlobalPositionShortcuts: plasmoid.configuration.isPreferredForPositionShortcuts

        requires.activeIndicatorEnabled: false
        requires.lengthMarginsEnabled: false
        requires.latteSideColoringEnabled: false
        requires.screenEdgeMarginSupported: true

        thinTooltip.local.showIsBlocked: root.contextMenu || root.windowPreviewIsShown
    }

    Timer{
        id: attentionTimer
        interval:8500
        onTriggered: {
            tasksModel.anyTaskDemandsAttentionInValidTime = false;

            if (appletAbilities.debug.timersEnabled) {
                console.log("plasmoid timer: attentionTimer called...");
            }
        }
    }

    //this timer restores the draggingPhase flag to false
    //after a dragging has finished... This delay is needed
    //in order to not animate any tasks are added after a
    //dragging
    Timer {
        id: restoreDraggingPhaseTimer
        interval: 150
        onTriggered: inDraggingPhase = false;
    }

    ///Red Liner!!! show the upper needed limit for animations
    Rectangle{
        anchors.horizontalCenter: !root.vertical ? parent.horizontalCenter : undefined
        anchors.verticalCenter: root.vertical ? parent.verticalCenter : undefined

        width: root.vertical ? 1 : 2 * appletAbilities.metrics.iconSize
        height: root.vertical ? 2 * appletAbilities.metrics.iconSize : 1
        color: "red"
        x: (root.location === PlasmaCore.Types.LeftEdge) ? neededSpace : parent.width - neededSpace
        y: (root.location === PlasmaCore.Types.TopEdge) ? neededSpace : parent.height - neededSpace

        visible: plasmoid.configuration.zoomHelper

        property int neededSpace: appletAbilities.parabolic.factor.zoom*appletAbilities.metrics.totals.length
    }

    Item{
        id:barLine
        anchors.bottom: (root.location === PlasmaCore.Types.BottomEdge) ? parent.bottom : undefined
        anchors.top: (root.location === PlasmaCore.Types.TopEdge) ? parent.top : undefined
        anchors.left: (root.location === PlasmaCore.Types.LeftEdge) ? parent.left : undefined
        anchors.right: (root.location === PlasmaCore.Types.RightEdge) ? parent.right : undefined

        anchors.horizontalCenter: !root.vertical ? parent.horizontalCenter : undefined
        anchors.verticalCenter: root.vertical ? parent.verticalCenter : undefined

        width: ( icList.orientation === Qt.Horizontal ) ? icList.width + spacing : smallSize
        height: ( icList.orientation === Qt.Vertical ) ? icList.height + spacing : smallSize

        property int spacing: latteBridge ? 0 : appletAbilities.metrics.iconSize / 2
        property int smallSize: Math.max(0.10 * appletAbilities.metrics.iconSize, 16)

        Behavior on opacity{
            NumberAnimation { duration: appletAbilities.animations.speedFactor.current * appletAbilities.animations.duration.large }
        }

        /// plasmoid's default panel
        BorderImage{
            anchors.fill:parent
            source: "../images/panel-west.png"
            border { left:8; right:8; top:8; bottom:8 }

            opacity: (plasmoid.configuration.showBarLine && !plasmoid.configuration.useThemePanel && inPlasma) ? 1 : 0

            visible: (opacity == 0) ? false : true

            horizontalTileMode: BorderImage.Stretch
            verticalTileMode: BorderImage.Stretch

            Behavior on opacity{
                NumberAnimation { duration: appletAbilities.animations.speedFactor.current * appletAbilities.animations.duration.large }
            }
        }


        /// item which is used as anchors for the plasma's theme
        Item{
            id:belower

            width: (root.location === PlasmaCore.Types.LeftEdge) ? shadowsSvgItem.margins.left : shadowsSvgItem.margins.right
            height: (root.location === PlasmaCore.Types.BottomEdge)? shadowsSvgItem.margins.bottom : shadowsSvgItem.margins.top

            anchors.top: (root.location === PlasmaCore.Types.BottomEdge) ? parent.bottom : undefined
            anchors.bottom: (root.location === PlasmaCore.Types.TopEdge) ? parent.top : undefined
            anchors.right: (root.location === PlasmaCore.Types.LeftEdge) ? parent.left : undefined
            anchors.left: (root.location === PlasmaCore.Types.RightEdge) ? parent.right : undefined
        }


        /// the current theme's panel
        KSvg.FrameSvgItem{
            id: shadowsSvgItem

            anchors.bottom: (root.location === PlasmaCore.Types.BottomEdge) ? belower.bottom : undefined
            anchors.top: (root.location === PlasmaCore.Types.TopEdge) ? belower.top : undefined
            anchors.left: (root.location === PlasmaCore.Types.LeftEdge) ? belower.left : undefined
            anchors.right: (root.location === PlasmaCore.Types.RightEdge) ? belower.right : undefined

            anchors.horizontalCenter: !root.vertical ? parent.horizontalCenter : undefined
            anchors.verticalCenter: root.vertical ? parent.verticalCenter : undefined

            width: root.vertical ? panelSize + margins.left + margins.right: parent.width
            height: root.vertical ? parent.height : panelSize + margins.top + margins.bottom

            imagePath: "translucent/widgets/panel-background"
            prefix:"shadow"

            opacity: (plasmoid.configuration.showBarLine && plasmoid.configuration.useThemePanel && inPlasma) ? 1 : 0
            visible: (opacity == 0) ? false : true

            property int panelSize: ((root.location === PlasmaCore.Types.BottomEdge) ||
                                     (root.location === PlasmaCore.Types.TopEdge)) ?
                                        plasmoid.configuration.panelSize + belower.height:
                                        plasmoid.configuration.panelSize + belower.width

            Behavior on opacity{
                NumberAnimation { duration: appletAbilities.animations.speedFactor.current * appletAbilities.animations.duration.large }
            }


            KSvg.FrameSvgItem{
                anchors.margins: belower.width-1
                anchors.fill:parent
                imagePath: plasmoid.configuration.transparentPanel ? "translucent/widgets/panel-background" :
                                                                     "widgets/panel-background"
            }
        }


        TasksLayout.MouseHandler {
            id: mouseHandler
            anchors.bottom: (root.location === PlasmaCore.Types.BottomEdge) ? scrollableList.bottom : undefined
            anchors.top: (root.location === PlasmaCore.Types.TopEdge) ? scrollableList.top : undefined
            anchors.left: (root.location === PlasmaCore.Types.LeftEdge) ? scrollableList.left : undefined
            anchors.right: (root.location === PlasmaCore.Types.RightEdge) ? scrollableList.right : undefined

            anchors.horizontalCenter: !root.vertical ? scrollableList.horizontalCenter : undefined
            anchors.verticalCenter: root.vertical ? scrollableList.verticalCenter : undefined

            width: root.vertical ? maxThickness : icList.width
            height: root.vertical ? icList.height : maxThickness

            target: icList

            property int maxThickness: ((appletAbilities.parabolic.isEnabled && appletAbilities.parabolic.isHovered)
                                        || (appletAbilities.parabolic.isEnabled && windowPreviewIsShown)
                                        || appletAbilities.animations.hasThicknessAnimation) ?
                                           appletAbilities.metrics.mask.thickness.maxZoomedForItems : // dont clip bouncing tasks when zoom=1
                                           appletAbilities.metrics.mask.thickness.normalForItems

            function onlyLaunchersInDroppedList(list){
                return list.every(function (item) {
                    return backend.isApplication(item)
                });
            }

            function onUrlsDropped(urls) {
                //! inform synced docks for new dropped launchers
                if (onlyLaunchersInDroppedList(urls)) {
                    appletAbilities.launchers.addDroppedLaunchers(urls);
                    return;
                }

                //! if the list does not contain only launchers then just open the corresponding
                //! urls with the relevant app

                if (!hoveredItem) {
                    return;
                }

                // DeclarativeMimeData urls is a QJsonArray but requestOpenUrls expects a proper QList<QUrl>.
                var urlsList = backend.jsonArrayToUrlList(urls);

                // Otherwise we'll just start a new instance of the application with the URLs as argument,
                // as you probably don't expect some of your files to open in the app and others to spawn launchers.
                tasksModel.requestOpenUrls(hoveredItem.modelIndex(), urlsList);
            }
        }

        /* Rectangle {
            anchors.fill: scrollableList
            color: "transparent"
            border.width: 1
            border.color: "blue"
        } */

        TasksLayout.ScrollableList {
            id: scrollableList
            width: !root.vertical ? length : thickness
            height: root.vertical ? length : thickness
            contentWidth: icList.width
            contentHeight: icList.height

            //onCurrentPosChanged: console.log("CP :: "+ currentPos + " icW:"+icList.width + " rw: "+root.width + " w:" +width);

            layer.enabled: contentsExceed && root.scrollingEnabled
            layer.effect: MultiEffect {
                maskEnabled: true
                maskSource: TasksLayout.ScrollOpacityMask{
                    width: scrollableList.width
                    height: scrollableList.height
                }
            }

            Binding {
                target: scrollableList
                property: "thickness"
                when: !appletAbilities.myView.inRelocationHiding
                value: {
                    if (appletAbilities.myView.isReady) {
                        return appletAbilities.animations.hasThicknessAnimation ? appletAbilities.metrics.mask.thickness.zoomed : appletAbilities.metrics.mask.thickness.normal;
                    }

                    return appletAbilities.metrics.totals.thickness * appletAbilities.parabolic.factor.zoom;
                }
            }

            Binding {
                target: scrollableList
                property: "length"
                when: !appletAbilities.myView.inRelocationHiding
                value: root.vertical ? Math.min(root.height, root.tasksLength) : Math.min(root.width, root.tasksLength)
            }

            TasksLayout.ScrollPositioner {
                id: listViewBase

                ListView {
                    id:icList
                    model: tasksModel
                    delegate: Task.TaskItem{
                        abilities: appletAbilities
                    }

                    property int currentSpot : -1000
                    property int previousCount : 0

                    property int tasksCount: tasksModel.count

                    //the duration of this animation should be as small as possible
                    //it fixes a small issue with the dragging an item to change it's
                    //position, if the duration is too big there is a point in the
                    //list that an item is going back and forth too fast

                    //more of a trouble
                    moveDisplaced: Transition {
                        NumberAnimation { properties: "x,y"; duration: appletAbilities.animations.speedFactor.current * appletAbilities.animations.duration.large; easing.type: Easing.Linear }
                    }

                    ///this transition can not be used with dragging !!!! I breaks
                    ///the lists indexes !!!!!
                    ///move: Transition {
                    ///    NumberAnimation { properties: "x,y"; duration: 400; easing.type: Easing.Linear }
                    ///}

                    function childAtPos(x, y, ignoredItem){
                        var tasks = icList.contentItem.children;
                        var bestTask = null;
                        var bestDistance = Number.POSITIVE_INFINITY;

                        for(var i=0; i<tasks.length; ++i){
                            var task = tasks[i];

                            if (task.objectName !== "TaskItem") {
                                continue;
                            }

                            if (ignoredItem && task === ignoredItem) {
                                continue;
                            }

                            var choords = mapFromItem(task,0, 0);

                            if ((x>=choords.x) && (x<=choords.x+task.width)
                                    && (y>=choords.y) && (y<=choords.y+task.height)) {
                                // In parabolic mode, task delegates can overlap. Pick the
                                // closest center to the pointer to get stable reorder target.
                                var centerX = choords.x + (task.width / 2);
                                var centerY = choords.y + (task.height / 2);
                                var distance = root.vertical ? Math.abs(y - centerY) : Math.abs(x - centerX);

                                if (distance < bestDistance) {
                                    bestDistance = distance;
                                    bestTask = task;
                                }
                            }
                        }

                        return bestTask;
                    }

                    function childAtIndex(position) {
                        var tasks = icList.contentItem.children;

                        if (position < 0)
                            return;

                        for(var i=0; i<tasks.length; ++i){
                            var task = tasks[i];

                            if (task.lastValidIndex === position
                                    || (task.lastValidIndex === -1 && task.itemIndex === position )) {
                                return task;
                            }
                        }

                        return undefined;
                    }
                }
            } // ScrollPositioner
        } // ScrollableList

        TasksLayout.ScrollEdgeShadows {
            id: scrollShadows
            width: !root.vertical ? scrollableList.width : thickness
            height: !root.vertical ? thickness : scrollableList.height
            visible: scrollableList.contentsExceed

            flickable: scrollableList
        } // ScrollEdgeShadows

        LatteComponents.AddingArea {
            id: newDroppedLauncherVisual
            width: root.vertical ? appletAbilities.metrics.totals.thickness : scrollableList.length
            height: root.vertical ? scrollableList.length : appletAbilities.metrics.totals.thickness

            visible: backgroundOpacity > 0
            radius: appletAbilities.metrics.iconSize/10
            backgroundOpacity: mouseHandler.isDroppingOnlyLaunchers || appletAbilities.launchers.isShowingAddLaunchersMessage ? 0.75 : 0
            duration: appletAbilities.animations.speedFactor.current
            iconSize: appletAbilities.metrics.iconSize
            z: 99

            title: i18n("Tasks Area")

            states: [
                ///Bottom Edge
                State {
                    name: "left"
                    when: root.location===PlasmaCore.Types.LeftEdge

                    AnchorChanges {
                        target: newDroppedLauncherVisual
                        anchors{ top:undefined; bottom:undefined; left:scrollableList.left; right:undefined;
                            horizontalCenter:undefined; verticalCenter:scrollableList.verticalCenter}
                    }

                    PropertyChanges {
                        target: newDroppedLauncherVisual
                        anchors{ topMargin:0; bottomMargin:0; leftMargin: appletAbilities.metrics.margin.screenEdge; rightMargin:0;}
                    }
                },
                State {
                    name: "right"
                    when: root.location===PlasmaCore.Types.RightEdge

                    AnchorChanges {
                        target: newDroppedLauncherVisual
                        anchors{ top:undefined; bottom:undefined; left:undefined; right:scrollableList.right;
                            horizontalCenter:undefined; verticalCenter:scrollableList.verticalCenter}
                    }

                    PropertyChanges {
                        target: newDroppedLauncherVisual
                        anchors{ topMargin:0; bottomMargin:0; leftMargin:0; rightMargin: appletAbilities.metrics.margin.screenEdge;}
                    }
                },
                State {
                    name: "top"
                    when: root.location===PlasmaCore.Types.TopEdge

                    AnchorChanges {
                        target: newDroppedLauncherVisual
                        anchors{ top:scrollableList.top; bottom:undefined; left:undefined; right:undefined;
                            horizontalCenter:scrollableList.horizontalCenter; verticalCenter:undefined}
                    }

                    PropertyChanges {
                        target: newDroppedLauncherVisual
                        anchors{ topMargin: appletAbilities.metrics.margin.screenEdge; bottomMargin:0; leftMargin:0; rightMargin:0;}
                    }
                },
                State {
                    name: "bottom"
                    when: root.location!==PlasmaCore.Types.TopEdge
                          && root.location !== PlasmaCore.Types.LeftEdge
                          && root.location !== PlasmaCore.Types.RightEdge

                    AnchorChanges {
                        target: newDroppedLauncherVisual
                        anchors{ top:undefined; bottom:scrollableList.bottom; left:undefined; right:undefined;
                            horizontalCenter:scrollableList.horizontalCenter; verticalCenter:undefined}
                    }

                    PropertyChanges {
                        target: newDroppedLauncherVisual
                        anchors{ topMargin:0; bottomMargin: appletAbilities.metrics.margin.screenEdge; leftMargin:0; rightMargin:0;}
                    }
                }
            ]
        }
    }

    //// helpers

    Timer {
        id: iconGeometryTimer
        // INVESTIGATE: such big interval but unfortunately it does not work otherwise
        interval: 500
        repeat: false

        onTriggered: {
            root.publishTasksGeometries();

            if (appletAbilities.debug.timersEnabled) {
                console.log("plasmoid timer: iconGeometryTimer called...");
            }
        }
    }


    //// functions
    function activateTaskAtIndex(index) {
        // This is called with Meta+number shortcuts by plasmashell when Tasks are in a plasma panel.
        appletAbilities.shortcuts.sglActivateEntryAtIndex(index);
    }

    function newInstanceForTaskAtIndex(index) {
        // This is called with Meta+Alt+number shortcuts by plasmashell when Tasks are in a plasma panel.
        appletAbilities.shortcuts.sglNewInstanceForEntryAtIndex(index);
    }

    function getBadger(identifier) {
        var ident1 = identifier;
        var n = ident1.lastIndexOf('/');

        var result = n>=0 ? ident1.substring(n + 1) : identifier;

        for(var i=0; i<badgers.length; ++i) {
            if (result.indexOf(badgers[i].id) >= 0) {
                return badgers[i];
            }
        }
    }

    function updateBadge(identifier, value) {
        var tasks = icList.contentItem.children;
        var identifierF = identifier.concat(".desktop");

        for(var i=0; i<tasks.length; ++i){
            var task = tasks[i];

            if (task && task.launcherUrl && task.launcherUrl.indexOf(identifierF) >= 0) {
                task.badgeIndicator = value === "" ? 0 : Number(value);
                var badge = getBadger(identifierF);
                if (badge) {
                    badge.value = value;
                } else {
                    badgers.push({id: identifierF, value: value});
                }
            }
        }
    }

    function getLauncherList() {
        return plasmoid.configuration.launchers59;
    }

    function previewContainsMouse() {
        return windowsPreviewDlg.containsMouse;
    }

    function containsMouse(){
        //console.log("s1...");
        if (disableRestoreZoom && (root.contextMenu || windowsPreviewDlg.visible)) {
            return;
        } else {
            disableRestoreZoom = false;
        }

        //if (previewContainsMouse())
        //  windowsPreviewDlg.hide(4);

        if (previewContainsMouse())
            return true;

        //console.log("s3...");
        var tasks = icList.contentItem.children;

        for(var i=0; i<tasks.length; ++i){
            var task = tasks[i];

            if(task && task.containsMouse){
                // console.log("Checking "+i+" - "+task.index+" - "+task.containsMouse);
                return true;
            }
        }

        return false;
    }

    function createContextMenu(rootTask, modelIndex, args) {
        var initialArgs = args || {}
        initialArgs.visualParent = rootTask;
        initialArgs.modelIndex = modelIndex;
        initialArgs.mpris2Source = mpris2Source;
        initialArgs.backend = backend;

        if (!root.contextMenuComponent || root.contextMenuComponent.status === Component.Error) {
            if (root.contextMenuComponent && root.contextMenuComponent.status === Component.Error) {
                console.warn("ContextMenu component load error:", root.contextMenuComponent.errorString());
            }
            root.contextMenuComponent = Qt.createComponent("ContextMenu.qml", Component.PreferSynchronous);
        }

        if (root.contextMenuComponent.status !== Component.Ready) {
            console.warn("ContextMenu component is not ready:", root.contextMenuComponent.errorString());
            return null;
        }

        root.contextMenu = root.contextMenuComponent.createObject(rootTask, initialArgs);

        if (!root.contextMenu) {
            console.warn("ContextMenu object creation failed:", root.contextMenuComponent.errorString());
        }

        return root.contextMenu;
    }

    Component.onCompleted:  {
        root.activateWindowView.connect(backend.activateWindowView);
        root.windowsHovered.connect(backend.windowsHovered);
        updateListViewParent();

        if (root.contextMenuComponent.status === Component.Error) {
            console.warn("ContextMenu component load error:", root.contextMenuComponent.errorString());
        }
    }

    Component.onDestruction: {
        root.activateWindowView.disconnect(backend.activateWindowView);
        root.windowsHovered.disconnect(backend.windowsHovered);
    }

    //BEGIN states
    // Alignments
    // 0-Center, 1-Left, 2-Right, 3-Top, 4-Bottom
    states: [
        ///Bottom Edge
        State {
            name: "bottomCenter"
            when: (root.location===PlasmaCore.Types.BottomEdge && root.alignment===LatteCore.types.Center)

            AnchorChanges {
                target: barLine
                anchors{ top:undefined; bottom:parent.bottom; left:undefined; right:undefined; horizontalCenter:parent.horizontalCenter; verticalCenter:undefined}
            }
        },
        State {
            name: "bottomLeft"
            when: (root.location===PlasmaCore.Types.BottomEdge && root.alignment===LatteCore.types.Left)

            AnchorChanges {
                target: barLine
                anchors{ top:undefined; bottom:parent.bottom; left:parent.left; right:undefined; horizontalCenter:undefined; verticalCenter:undefined}
            }
        },
        State {
            name: "bottomRight"
            when: (root.location===PlasmaCore.Types.BottomEdge && root.alignment===LatteCore.types.Right)

            AnchorChanges {
                target: barLine
                anchors{ top:undefined; bottom:parent.bottom; left:undefined; right:parent.right; horizontalCenter:undefined; verticalCenter:undefined}
            }
        },
        ///Top Edge
        State {
            name: "topCenter"
            when: (root.location===PlasmaCore.Types.TopEdge && root.alignment===LatteCore.types.Center)

            AnchorChanges {
                target: barLine
                anchors{ top:parent.top; bottom:undefined; left:undefined; right:undefined; horizontalCenter:parent.horizontalCenter; verticalCenter:undefined}
            }
        },
        State {
            name: "topLeft"
            when: (root.location===PlasmaCore.Types.TopEdge && root.alignment===LatteCore.types.Left)

            AnchorChanges {
                target: barLine
                anchors{ top:parent.top; bottom:undefined; left:parent.left; right:undefined; horizontalCenter:undefined; verticalCenter:undefined}
            }
        },
        State {
            name: "topRight"
            when: (root.location===PlasmaCore.Types.TopEdge && root.alignment===LatteCore.types.Right)

            AnchorChanges {
                target: barLine
                anchors{ top:parent.top; bottom:undefined; left:undefined; right:parent.right; horizontalCenter:undefined; verticalCenter:undefined}
            }
        },
        ////Left Edge
        State {
            name: "leftCenter"
            when: (root.location===PlasmaCore.Types.LeftEdge && root.alignment===LatteCore.types.Center)

            AnchorChanges {
                target: barLine
                anchors{ top:undefined; bottom:undefined; left:parent.left; right:undefined; horizontalCenter:undefined; verticalCenter:parent.verticalCenter}
            }
        },
        State {
            name: "leftTop"
            when: (root.location===PlasmaCore.Types.LeftEdge && root.alignment===LatteCore.types.Top)

            AnchorChanges {
                target: barLine
                anchors{ top:parent.top; bottom:undefined; left:parent.left; right:undefined; horizontalCenter:undefined; verticalCenter:undefined}
            }
        },
        State {
            name: "leftBottom"
            when: (root.location===PlasmaCore.Types.LeftEdge && root.alignment===LatteCore.types.Bottom)

            AnchorChanges {
                target: barLine
                anchors{ top:undefined; bottom:parent.bottom; left:parent.left; right:undefined; horizontalCenter:undefined; verticalCenter:undefined}
            }
        },
        ///Right Edge
        State {
            name: "rightCenter"
            when: (root.location===PlasmaCore.Types.RightEdge && root.alignment===LatteCore.types.Center)

            AnchorChanges {
                target: barLine
                anchors{ top:undefined; bottom:undefined; left:undefined; right:parent.right; horizontalCenter:undefined; verticalCenter:parent.verticalCenter}
            }
        },
        State {
            name: "rightTop"
            when: (root.location===PlasmaCore.Types.RightEdge && root.alignment===LatteCore.types.Top)

            AnchorChanges {
                target: barLine
                anchors{ top:parent.top; bottom:undefined; left:undefined; right:parent.right; horizontalCenter:undefined; verticalCenter:undefined}
            }
        },
        State {
            name: "rightBottom"
            when: (root.location===PlasmaCore.Types.RightEdge && root.alignment===LatteCore.types.Bottom)

            AnchorChanges {
                target: barLine
                anchors{ top:undefined; bottom:parent.bottom; left:undefined; right:parent.right; horizontalCenter:undefined; verticalCenter:undefined}
            }
        }

    ]
    //END states

}
