/*
    SPDX-FileCopyrightText: 2016 Smith AR <audoban@openmailbox.org>
    SPDX-FileCopyrightText: 2016 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick 2.7
import QtQuick.Layouts 1.3
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.plasma.components 3.0 as PlasmaComponents3

import org.kde.latte.core 0.2 as LatteCore
import org.kde.latte.components 1.0 as LatteComponents

import org.kde.latte.private.tasks 0.1 as LatteTasks

PlasmaComponents.Page {
    id: _tasksPage
    width: content.width + content.Layout.leftMargin * 2
    height: content.height + units.smallSpacing * 2

    property bool disableAllWindowsFunctionality: tasks.plasmoid.configuration.hideAllTasks

    readonly property bool isCurrentPage: (dialog.currentPage === _tasksPage)

    onIsCurrentPageChanged: {
        if (isCurrentPage && latteView.extendedInterface.latteTasksModel.count>1) {
            latteView.extendedInterface.appletRequestedVisualIndicator(tasks.plasmoid.id);
        }
    }

    ColumnLayout {
        id: content

        width: (dialog.appliedWidth - units.smallSpacing * 2) - Layout.leftMargin * 2
        spacing: dialog.subGroupSpacing
        anchors.horizontalCenter: parent.horizontalCenter
        Layout.leftMargin: units.smallSpacing * 2
        Layout.rightMargin: units.smallSpacing * 2

        //! BEGIN: Badges
        ColumnLayout {
            spacing: units.smallSpacing
            Layout.topMargin: units.smallSpacing
            visible: dialog.advancedLevel

            LatteComponents.Header {
                text: i18n("Badges")
            }

            LatteComponents.CheckBoxesColumn {
                Layout.leftMargin: units.smallSpacing * 2
                Layout.rightMargin: units.smallSpacing * 2

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Notifications from tasks")
                    tooltip: i18n("Show unread messages or notifications from tasks")
                    value: tasks.plasmoid.configuration.showInfoBadge

                    onClicked: {
                        tasks.plasmoid.configuration.showInfoBadge = !tasks.plasmoid.configuration.showInfoBadge;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Progress information for tasks")
                    tooltip: i18n("Show a progress animation for tasks e.g. when copying files with Dolphin")
                    value: tasks.plasmoid.configuration.showProgressBadge

                    onClicked: {
                        tasks.plasmoid.configuration.showProgressBadge = !tasks.plasmoid.configuration.showProgressBadge;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Audio playing from tasks")
                    tooltip: i18n("Show audio playing from tasks")
                    value: tasks.plasmoid.configuration.showAudioBadge

                    onClicked: {
                        tasks.plasmoid.configuration.showAudioBadge = !tasks.plasmoid.configuration.showAudioBadge;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Prominent color for notification badge")
                    enabled: tasks.plasmoid.configuration.showInfoBadge
                    tooltip: i18n("Notification badge uses a more prominent background which is usually red")
                    value: tasks.plasmoid.configuration.infoBadgeProminentColorEnabled

                    onClicked: {
                        tasks.plasmoid.configuration.infoBadgeProminentColorEnabled = !tasks.plasmoid.configuration.infoBadgeProminentColorEnabled;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Change volume when scrolling audio badge")
                    enabled: tasks.plasmoid.configuration.showAudioBadge
                    tooltip: i18n("The user is able to mute/unmute with click or change the volume with mouse wheel")
                    value: tasks.plasmoid.configuration.audioBadgeActionsEnabled

                    onClicked: {
                        tasks.plasmoid.configuration.audioBadgeActionsEnabled = !tasks.plasmoid.configuration.audioBadgeActionsEnabled;
                    }
                }
            }
        }
        //! END: Badges

        //! BEGIN: Tasks Interaction
        ColumnLayout {
            Layout.topMargin: dialog.basicLevel ? units.smallSpacing : 0
            spacing: units.smallSpacing

            LatteComponents.Header {
                text: i18n("Interaction")
            }

            LatteComponents.CheckBoxesColumn {
                Layout.leftMargin: units.smallSpacing * 2
                Layout.rightMargin: units.smallSpacing * 2

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Launchers are added only in current tasks applet")
                    tooltip: i18n("Launchers are added only in current tasks applet and not as regular applets or in any other applet")
                    value:tasks.plasmoid.configuration.isPreferredForDroppedLaunchers

                    onClicked: {
                        tasks.plasmoid.configuration.isPreferredForDroppedLaunchers = !tasks.plasmoid.configuration.isPreferredForDroppedLaunchers;
                    }
                }

                LatteComponents.CheckBox {
                    id: windowActionsChk
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Window actions in the context menu")
                    visible: dialog.advancedLevel
                    enabled: !disableAllWindowsFunctionality
                    value: tasks.plasmoid.configuration.showWindowActions

                    onClicked: {
                        tasks.plasmoid.configuration.showWindowActions = !tasks.plasmoid.configuration.showWindowActions;
                    }
                }

                LatteComponents.CheckBox {
                    id: previewPopupChk
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Preview window behaves as popup")
                    visible: dialog.advancedLevel
                    enabled: !disableAllWindowsFunctionality
                    value: tasks.plasmoid.configuration.previewWindowAsPopup

                    onClicked: {
                        tasks.plasmoid.configuration.previewWindowAsPopup = !tasks.plasmoid.configuration.previewWindowAsPopup;
                    }
                }

                LatteComponents.CheckBox {
                    id: unifyGlobalShortcutsChk
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Based on position shortcuts apply only on current tasks")
                    // checked: tasks.plasmoid.configuration.isPreferredForPositionShortcuts //! Disabled because it was not updated between multiple Tasks
                    tooltip: i18n("Based on position global shortcuts are enabled only for current tasks and not for other applets")
                    visible: dialog.advancedLevel
                    enabled: latteView.isPreferredForShortcuts || (!latteView.layout.preferredForShortcutsTouched && latteView.isHighestPriorityView())
                    value: tasks.plasmoid.configuration.isPreferredForPositionShortcuts

                    onClicked: {
                        tasks.plasmoid.configuration.isPreferredForPositionShortcuts = !tasks.plasmoid.configuration.isPreferredForPositionShortcuts;
                    }
                }
            }
        }
        //! END: Tasks Interaction

        //! BEGIN: Tasks Filters
        ColumnLayout {
            spacing: units.smallSpacing


            LatteComponents.Header {
                text: i18n("Filters")
            }

            LatteComponents.CheckBoxesColumn {
                Layout.leftMargin: units.smallSpacing * 2
                Layout.rightMargin: units.smallSpacing * 2

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Show only tasks from the current screen")
                    enabled: !disableAllWindowsFunctionality
                    value: tasks.plasmoid.configuration.showOnlyCurrentScreen

                    onClicked: {
                        tasks.plasmoid.configuration.showOnlyCurrentScreen = !tasks.plasmoid.configuration.showOnlyCurrentScreen;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Show only tasks from the current desktop")
                    enabled: !disableAllWindowsFunctionality
                    value: tasks.plasmoid.configuration.showOnlyCurrentDesktop

                    onClicked: {
                        tasks.plasmoid.configuration.showOnlyCurrentDesktop = !tasks.plasmoid.configuration.showOnlyCurrentDesktop;
                    }
                }

                // KActivities support is unreliable in Plasma 6, hidden until restored
                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Show only tasks from the current activity")
                    visible: false
                    enabled: !disableAllWindowsFunctionality
                    value: tasks.plasmoid.configuration.showOnlyCurrentActivity

                    onClicked: {
                        tasks.plasmoid.configuration.showOnlyCurrentActivity = !tasks.plasmoid.configuration.showOnlyCurrentActivity;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Show only tasks from launchers")
                    visible: dialog.advancedLevel
                    enabled: !disableAllWindowsFunctionality
                    value: tasks.plasmoid.configuration.showWindowsOnlyFromLaunchers

                    onClicked: {
                        tasks.plasmoid.configuration.showWindowsOnlyFromLaunchers = !tasks.plasmoid.configuration.showWindowsOnlyFromLaunchers;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Show only launchers and hide all tasks")
                    tooltip: i18n("Tasks become hidden and only launchers are shown")
                    visible: dialog.advancedLevel
                    value: tasks.plasmoid.configuration.hideAllTasks

                    onClicked: {
                        tasks.plasmoid.configuration.hideAllTasks = !tasks.plasmoid.configuration.hideAllTasks;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Show only grouped tasks for same application")
                    tooltip: i18n("By default group tasks of the same application")
                    visible: dialog.advancedLevel
                    enabled: !disableAllWindowsFunctionality
                    value: tasks.plasmoid.configuration.groupTasksByDefault

                    onClicked: {
                        tasks.plasmoid.configuration.groupTasksByDefault = !tasks.plasmoid.configuration.groupTasksByDefault;
                    }
                }
            }
        }

        //! END: Tasks Filters

        //! BEGIN: Animations
        ColumnLayout {
            spacing: units.smallSpacing
            enabled: plasmoid.configuration.animationsEnabled
            visible: dialog.advancedLevel

            LatteComponents.Header {
                text: i18n("Animations")
            }

            LatteComponents.CheckBoxesColumn {
                Layout.leftMargin: units.smallSpacing * 2
                Layout.rightMargin: units.smallSpacing * 2

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Bounce launchers when triggered")
                    value: tasks.plasmoid.configuration.animationLauncherBouncing
                    enabled: !latteView.indicator.info.providesTaskLauncherAnimation

                    onClicked: {
                        tasks.plasmoid.configuration.animationLauncherBouncing = !tasks.plasmoid.configuration.animationLauncherBouncing;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Bounce tasks that need attention")
                    value: tasks.plasmoid.configuration.animationWindowInAttention
                    enabled: !latteView.indicator.info.providesInAttentionAnimation

                    onClicked: {
                        tasks.plasmoid.configuration.animationWindowInAttention = !tasks.plasmoid.configuration.animationWindowInAttention;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Slide in and out single windows")
                    value: tasks.plasmoid.configuration.animationNewWindowSliding

                    onClicked: {
                        tasks.plasmoid.configuration.animationNewWindowSliding = !tasks.plasmoid.configuration.animationNewWindowSliding;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Grouped tasks bounce their new windows")
                    value: tasks.plasmoid.configuration.animationWindowAddedInGroup
                    enabled: !latteView.indicator.info.providesGroupedWindowAddedAnimation

                    onClicked: {
                        tasks.plasmoid.configuration.animationWindowAddedInGroup = !tasks.plasmoid.configuration.animationWindowAddedInGroup;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Grouped tasks slide out their closed windows")
                    value: tasks.plasmoid.configuration.animationWindowRemovedFromGroup
                    enabled: !latteView.indicator.info.providesGroupedWindowRemovedAnimation

                    onClicked: {
                        tasks.plasmoid.configuration.animationWindowRemovedFromGroup = !tasks.plasmoid.configuration.animationWindowRemovedFromGroup;
                    }
                }
            }
        }
        //! END: Animations


        //! BEGIN: Launchers Group
        ColumnLayout {
            spacing: units.smallSpacing


            LatteComponents.Header {
                text: i18n("Launchers")
            }

            ColumnLayout {
                Layout.leftMargin: units.smallSpacing * 2
                Layout.rightMargin: units.smallSpacing * 2
                spacing: 0

                RowLayout {
                    Layout.fillWidth: true

                    spacing: 2

                    property int group: tasks.plasmoid.configuration.launchersGroup

                    readonly property int buttonsCount: layoutGroupButton.visible ? 3 : 2
                    readonly property int buttonSize: (dialog.optionsWidth - (spacing * buttonsCount-1)) / buttonsCount

                    LatteComponents.Button {
                        Layout.minimumWidth: parent.buttonSize
                        Layout.maximumWidth: Layout.minimumWidth
                        text: i18nc("unique launchers group","Unique Group")
                        tooltip: i18n("Use a unique set of launchers for this view which is independent from any other view")
                        checked: parent.group === group
                        checkable: false

                        readonly property int group: LatteCore.types.UniqueLaunchers

                        onPressedChanged: {
                            if (pressed) {
                                tasks.plasmoid.configuration.launchersGroup = group;
                            }
                        }
                    }

                    LatteComponents.Button {
                        id: layoutGroupButton
                        Layout.minimumWidth: parent.buttonSize
                        Layout.maximumWidth: Layout.minimumWidth
                        text: i18nc("layout launchers group","Layout Group")
                        tooltip: i18n("Use the current layout set of launchers for this latteView. This group provides launchers <b>synchronization</b> between different views in the <b>same layout</b>")
                        checked: parent.group === group
                        checkable: false
                        //! it is shown only when the user has activated that option manually from the text layout file
                        visible: tasks.plasmoid.configuration.launchersGroup === group

                        readonly property int group: LatteCore.types.LayoutLaunchers

                        onPressedChanged: {
                            if (pressed) {
                                tasks.plasmoid.configuration.launchersGroup = group;
                            }
                        }
                    }

                    LatteComponents.Button {
                        Layout.minimumWidth: parent.buttonSize
                        Layout.maximumWidth: Layout.minimumWidth
                        text: i18nc("global launchers group","Global Group")
                        tooltip: i18n("Use the global set of launchers for this latteView. This group provides launchers <b>synchronization</b> between different views and between <b>different layouts</b>")
                        checked: parent.group === group
                        checkable: false

                        readonly property int group: LatteCore.types.GlobalLaunchers

                        onPressedChanged: {
                            if (pressed) {
                                tasks.plasmoid.configuration.launchersGroup = group;
                            }
                        }
                    }
                }
            }
        }
        //! END: Launchers Group

        //! BEGIN: Scrolling
        ColumnLayout {
            spacing: units.smallSpacing
            visible: dialog.advancedLevel

            LatteComponents.HeaderSwitch {
                id: scrollingHeader
                Layout.minimumWidth: dialog.optionsWidth + 2 *units.smallSpacing
                Layout.maximumWidth: Layout.minimumWidth
                Layout.minimumHeight: implicitHeight
                Layout.bottomMargin: units.smallSpacing
                checked: tasks.plasmoid.configuration.scrollTasksEnabled
                text: i18n("Scrolling")
                tooltip: i18n("Enable tasks scrolling when they overflow and exceed the available space");

                onPressed: {
                    tasks.plasmoid.configuration.scrollTasksEnabled = !tasks.plasmoid.configuration.scrollTasksEnabled;;
                }
            }

            ColumnLayout {
                Layout.leftMargin: units.smallSpacing * 2
                Layout.rightMargin: units.smallSpacing * 2
                spacing: 0
                enabled: scrollingHeader.checked

                GridLayout {
                    columns: 2
                    Layout.minimumWidth: dialog.optionsWidth
                    Layout.maximumWidth: Layout.minimumWidth

                    Layout.topMargin: units.smallSpacing

                    PlasmaComponents.Label {
                        Layout.fillWidth: true
                        text: i18n("Manual")
                    }

                    LatteComponents.ComboBox {
                        id: manualScrolling
                        Layout.minimumWidth: leftClickAction.width
                        Layout.maximumWidth: leftClickAction.width
                        model: [i18nc("disabled manual scrolling", "Disabled scrolling"),
                            dialog.panelIsVertical ? i18n("Only vertical scrolling") : i18n("Only horizontal scrolling"),
                            i18n("Horizontal and vertical scrolling")]

                        currentIndex: tasks.plasmoid.configuration.manualScrollTasksType
                        onCurrentIndexChanged: tasks.plasmoid.configuration.manualScrollTasksType = currentIndex;
                    }

                    PlasmaComponents.Label {
                        id: autoScrollText
                        Layout.fillWidth: true
                        text: i18n("Automatic")
                    }

                    LatteComponents.ComboBox {
                        id: autoScrolling
                        Layout.minimumWidth: leftClickAction.width
                        Layout.maximumWidth: leftClickAction.width
                        model: [
                            i18n("Disabled"),
                            i18n("Enabled")
                        ]

                        currentIndex: tasks.plasmoid.configuration.autoScrollTasksEnabled
                        onCurrentIndexChanged: {
                            if (currentIndex === 0) {
                                tasks.plasmoid.configuration.autoScrollTasksEnabled = false;
                            } else {
                                tasks.plasmoid.configuration.autoScrollTasksEnabled = true;
                            }
                        }
                    }
                }
            }
        }
        //! END: Scrolling


        //! BEGIN: Actions
        ColumnLayout {
            spacing: units.smallSpacing
            visible: dialog.advancedLevel

            LatteComponents.Header {
                text: i18n("Actions")
            }

            ColumnLayout {
                Layout.leftMargin: units.smallSpacing * 2
                Layout.rightMargin: units.smallSpacing * 2
                spacing: 0

                GridLayout {
                    columns: 2
                    Layout.minimumWidth: dialog.optionsWidth
                    Layout.maximumWidth: Layout.minimumWidth

                    Layout.topMargin: units.smallSpacing
                    enabled: !disableAllWindowsFunctionality

                    PlasmaComponents.Label {
                        id: leftClickLbl
                        text: i18n("Left Click")
                    }

                    LatteComponents.ComboBox {
                        id: leftClickAction
                        Layout.fillWidth: true
                        model: [i18nc("present windows action", "Present Windows"),
                            i18n("Cycle Through Tasks"),
                            i18n("Preview Windows")]

                        currentIndex: {
                            switch(tasks.plasmoid.configuration.leftClickAction) {
                            case LatteTasks.types.PresentWindows:
                                return 0;
                            case LatteTasks.types.CycleThroughTasks:
                                return 1;
                            case LatteTasks.types.PreviewWindows:
                                return 2;
                            }

                            return 0;
                        }

                        onCurrentIndexChanged: {
                            switch(currentIndex) {
                            case 0:
                                tasks.plasmoid.configuration.leftClickAction = LatteTasks.types.PresentWindows;
                                break;
                            case 1:
                                tasks.plasmoid.configuration.leftClickAction = LatteTasks.types.CycleThroughTasks;
                                break;
                            case 2:
                                tasks.plasmoid.configuration.leftClickAction = LatteTasks.types.PreviewWindows;
                                break;
                            }
                        }
                    }

                    PlasmaComponents.Label {
                        id: middleClickText
                        text: i18n("Middle Click")
                    }

                    LatteComponents.ComboBox {
                        id: middleClickAction
                        Layout.fillWidth: true
                        model: [
                            i18nc("The click action", "None"),
                            i18n("Close Window or Group"),
                            i18n("New Instance"),
                            i18n("Minimize/Restore Window or Group"),
                            i18n("Cycle Through Tasks"),
                            i18n("Toggle Task Grouping")
                        ]

                        currentIndex: tasks.plasmoid.configuration.middleClickAction
                        onCurrentIndexChanged: tasks.plasmoid.configuration.middleClickAction = currentIndex
                    }

                    PlasmaComponents.Label {
                        text: i18n("Hover")
                    }

                    LatteComponents.ComboBox {
                        id: hoverAction
                        Layout.fillWidth: true
                        model: [
                            i18nc("none action", "None"),
                            i18n("Preview Windows"),
                            i18n("Highlight Windows"),
                            i18n("Preview and Highlight Windows"),
                        ]

                        currentIndex: {
                            switch(tasks.plasmoid.configuration.hoverAction) {
                            case LatteTasks.types.NoneAction:
                                return 0;
                            case LatteTasks.types.PreviewWindows:
                                return 1;
                            case LatteTasks.types.HighlightWindows:
                                return 2;
                            case LatteTasks.types.PreviewAndHighlightWindows:
                                return 3;
                            }

                            return 0;
                        }

                        onCurrentIndexChanged: {
                            switch(currentIndex) {
                            case 0:
                                tasks.plasmoid.configuration.hoverAction = LatteTasks.types.NoneAction;
                                break;
                            case 1:
                                tasks.plasmoid.configuration.hoverAction = LatteTasks.types.PreviewWindows;
                                break;
                            case 2:
                                tasks.plasmoid.configuration.hoverAction = LatteTasks.types.HighlightWindows;
                                break;
                            case 3:
                                tasks.plasmoid.configuration.hoverAction = LatteTasks.types.PreviewAndHighlightWindows;
                                break;
                            }
                        }
                    }

                    PlasmaComponents.Label {
                        text: i18n("Wheel")
                    }

                    LatteComponents.ComboBox {
                        id: wheelAction
                        Layout.fillWidth: true
                        model: [
                            i18nc("none action", "None"),
                            i18n("Cycle Through Tasks"),
                            i18n("Cycle And Minimize Tasks")
                        ]

                        currentIndex: tasks.plasmoid.configuration.taskScrollAction
                        onCurrentIndexChanged: tasks.plasmoid.configuration.taskScrollAction = currentIndex
                    }

                    RowLayout {
                        spacing: units.smallSpacing
                        enabled: !disableAllWindowsFunctionality

                        Layout.minimumWidth: middleClickText.width
                        Layout.maximumWidth: middleClickText.width

                        LatteComponents.ComboBox {
                            id: modifier
                            Layout.fillWidth: true
                            model: ["Shift", "Ctrl", "Alt", "Meta"]

                            currentIndex: tasks.plasmoid.configuration.modifier
                            onCurrentIndexChanged: tasks.plasmoid.configuration.modifier = currentIndex
                        }

                        PlasmaComponents.Label {
                            text: "+"
                        }
                    }

                    RowLayout {
                        spacing: units.smallSpacing
                        enabled: !disableAllWindowsFunctionality

                        readonly property int maxSize: 0.4 * dialog.optionsWidth

                        LatteComponents.ComboBox {
                            id: modifierClick
                            Layout.preferredWidth: 0.7 * parent.maxSize
                            Layout.maximumWidth: parent.maxSize
                            model: [i18n("Left Click"), i18n("Middle Click"), i18n("Right Click")]

                            currentIndex: tasks.plasmoid.configuration.modifierClick
                            onCurrentIndexChanged: tasks.plasmoid.configuration.modifierClick = currentIndex
                        }

                        PlasmaComponents.Label {
                            text: "="
                        }

                        LatteComponents.ComboBox {
                            id: modifierClickAction
                            Layout.fillWidth: true
                            model: [i18nc("The click action", "None"), i18n("Close Window or Group"),
                                i18n("New Instance"), i18n("Minimize/Restore Window or Group"),  i18n("Cycle Through Tasks"), i18n("Toggle Task Grouping")]

                            currentIndex: tasks.plasmoid.configuration.modifierClickAction
                            onCurrentIndexChanged: tasks.plasmoid.configuration.modifierClickAction = currentIndex
                        }
                    }
                }

                RowLayout {
                    Layout.minimumWidth: dialog.optionsWidth
                    Layout.maximumWidth: Layout.minimumWidth
                    Layout.topMargin: units.smallSpacing
                    spacing: units.smallSpacing
                    enabled: !disableAllWindowsFunctionality

                }
            }
        }
        //! END: Actions


    }
}
