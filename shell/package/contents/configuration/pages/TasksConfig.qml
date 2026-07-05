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

    // Use latteView.extendedInterface to get the SAME configuration
    // object the plasmoid uses, not a stale copy.
    readonly property var cfg: latteView.extendedInterface.configurationForAppletVisualIndex(index)
    property bool disableAllWindowsFunctionality: cfg.hideAllTasks

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
                    value: cfg.showInfoBadge

                    onClicked: {
                        cfg.showInfoBadge = !cfg.showInfoBadge;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Progress information for tasks")
                    tooltip: i18n("Show a progress animation for tasks e.g. when copying files with Dolphin")
                    value: cfg.showProgressBadge

                    onClicked: {
                        cfg.showProgressBadge = !cfg.showProgressBadge;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Audio playing from tasks")
                    tooltip: i18n("Show audio playing from tasks")
                    value: cfg.showAudioBadge

                    onClicked: {
                        cfg.showAudioBadge = !cfg.showAudioBadge;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Prominent color for notification badge")
                    enabled: cfg.showInfoBadge
                    tooltip: i18n("Notification badge uses a more prominent background which is usually red")
                    value: cfg.infoBadgeProminentColorEnabled

                    onClicked: {
                        cfg.infoBadgeProminentColorEnabled = !cfg.infoBadgeProminentColorEnabled;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Change volume when scrolling audio badge")
                    enabled: cfg.showAudioBadge
                    tooltip: i18n("The user is able to mute/unmute with click or change the volume with mouse wheel")
                    value: cfg.audioBadgeActionsEnabled

                    onClicked: {
                        cfg.audioBadgeActionsEnabled = !cfg.audioBadgeActionsEnabled;
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
                    value:cfg.isPreferredForDroppedLaunchers

                    onClicked: {
                        cfg.isPreferredForDroppedLaunchers = !cfg.isPreferredForDroppedLaunchers;
                    }
                }

                LatteComponents.CheckBox {
                    id: windowActionsChk
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Window actions in the context menu")
                    visible: dialog.advancedLevel
                    enabled: !disableAllWindowsFunctionality
                    value: cfg.showWindowActions

                    onClicked: {
                        cfg.showWindowActions = !cfg.showWindowActions;
                    }
                }

                LatteComponents.CheckBox {
                    id: previewPopupChk
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Preview window behaves as popup")
                    visible: dialog.advancedLevel
                    enabled: !disableAllWindowsFunctionality
                    value: cfg.previewWindowAsPopup

                    onClicked: {
                        cfg.previewWindowAsPopup = !cfg.previewWindowAsPopup;
                    }
                }

                LatteComponents.CheckBox {
                    id: unifyGlobalShortcutsChk
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Based on position shortcuts apply only on current tasks")
                    // checked: cfg.isPreferredForPositionShortcuts //! Disabled because it was not updated between multiple Tasks
                    tooltip: i18n("Based on position global shortcuts are enabled only for current tasks and not for other applets")
                    visible: dialog.advancedLevel
                    enabled: latteView.isPreferredForShortcuts || (!latteView.layout.preferredForShortcutsTouched && latteView.isHighestPriorityView())
                    value: cfg.isPreferredForPositionShortcuts

                    onClicked: {
                        cfg.isPreferredForPositionShortcuts = !cfg.isPreferredForPositionShortcuts;
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
                    value: cfg.showOnlyCurrentScreen

                    onClicked: {
                        cfg.showOnlyCurrentScreen = !cfg.showOnlyCurrentScreen;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Show only tasks from the current desktop")
                    enabled: !disableAllWindowsFunctionality
                    value: cfg.showOnlyCurrentDesktop

                    onClicked: {
                        cfg.showOnlyCurrentDesktop = !cfg.showOnlyCurrentDesktop;
                    }
                }

                // KActivities support is unreliable in Plasma 6, hidden until restored
                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Show only tasks from the current activity")
                    visible: false
                    enabled: !disableAllWindowsFunctionality
                    value: cfg.showOnlyCurrentActivity

                    onClicked: {
                        cfg.showOnlyCurrentActivity = !cfg.showOnlyCurrentActivity;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Show only tasks from launchers")
                    visible: dialog.advancedLevel
                    enabled: !disableAllWindowsFunctionality
                    value: cfg.showWindowsOnlyFromLaunchers

                    onClicked: {
                        cfg.showWindowsOnlyFromLaunchers = !cfg.showWindowsOnlyFromLaunchers;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Show only launchers and hide all tasks")
                    tooltip: i18n("Tasks become hidden and only launchers are shown")
                    visible: dialog.advancedLevel
                    value: cfg.hideAllTasks

                    onClicked: {
                        cfg.hideAllTasks = !cfg.hideAllTasks;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Show only grouped tasks for same application")
                    tooltip: i18n("By default group tasks of the same application")
                    visible: dialog.advancedLevel
                    enabled: !disableAllWindowsFunctionality
                    value: cfg.groupTasksByDefault

                    onClicked: {
                        cfg.groupTasksByDefault = !cfg.groupTasksByDefault;
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
                    value: cfg.animationLauncherBouncing
                    enabled: !latteView.indicator.info.providesTaskLauncherAnimation

                    onClicked: {
                        cfg.animationLauncherBouncing = !cfg.animationLauncherBouncing;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Bounce tasks that need attention")
                    value: cfg.animationWindowInAttention
                    enabled: !latteView.indicator.info.providesInAttentionAnimation

                    onClicked: {
                        cfg.animationWindowInAttention = !cfg.animationWindowInAttention;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Slide in and out single windows")
                    value: cfg.animationNewWindowSliding

                    onClicked: {
                        cfg.animationNewWindowSliding = !cfg.animationNewWindowSliding;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Grouped tasks bounce their new windows")
                    value: cfg.animationWindowAddedInGroup
                    enabled: !latteView.indicator.info.providesGroupedWindowAddedAnimation

                    onClicked: {
                        cfg.animationWindowAddedInGroup = !cfg.animationWindowAddedInGroup;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: dialog.optionsWidth
                    text: i18n("Grouped tasks slide out their closed windows")
                    value: cfg.animationWindowRemovedFromGroup
                    enabled: !latteView.indicator.info.providesGroupedWindowRemovedAnimation

                    onClicked: {
                        cfg.animationWindowRemovedFromGroup = !cfg.animationWindowRemovedFromGroup;
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

                    property int group: cfg.launchersGroup

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
                                cfg.launchersGroup = group;
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
                        visible: cfg.launchersGroup === group

                        readonly property int group: LatteCore.types.LayoutLaunchers

                        onPressedChanged: {
                            if (pressed) {
                                cfg.launchersGroup = group;
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
                                cfg.launchersGroup = group;
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
                checked: cfg.scrollTasksEnabled
                text: i18n("Scrolling")
                tooltip: i18n("Enable tasks scrolling when they overflow and exceed the available space");

                onPressed: {
                    cfg.scrollTasksEnabled = !cfg.scrollTasksEnabled;;
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

                        currentIndex: cfg.manualScrollTasksType
                        onCurrentIndexChanged: cfg.manualScrollTasksType = currentIndex;
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

                        currentIndex: cfg.autoScrollTasksEnabled
                        onCurrentIndexChanged: {
                            if (currentIndex === 0) {
                                cfg.autoScrollTasksEnabled = false;
                            } else {
                                cfg.autoScrollTasksEnabled = true;
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
                        model: [i18nc("none action", "None"),
                            i18n("Close Window or Group"),
                            i18n("New Instance"),
                            i18n("Minimize/Restore Window or Group"),
                            i18n("Cycle Through Tasks"),
                            i18n("Toggle Task Grouping"),
                            i18nc("present windows action", "Present Windows"),
                            i18n("Preview Windows"),
                            i18n("Highlight Windows"),
                            i18n("Preview and Highlight Windows")]

                        currentIndex: cfg.leftClickAction
                        onCurrentIndexChanged: cfg.leftClickAction = currentIndex
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
                            i18n("Toggle Task Grouping"),
                            i18nc("present windows action", "Present Windows"),
                            i18n("Preview Windows"),
                            i18n("Highlight Windows"),
                            i18n("Preview and Highlight Windows")
                        ]

                        currentIndex: cfg.middleClickAction
                        onCurrentIndexChanged: cfg.middleClickAction = currentIndex
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
                            switch(cfg.hoverAction) {
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
                                cfg.hoverAction = LatteTasks.types.NoneAction;
                                break;
                            case 1:
                                cfg.hoverAction = LatteTasks.types.PreviewWindows;
                                break;
                            case 2:
                                cfg.hoverAction = LatteTasks.types.HighlightWindows;
                                break;
                            case 3:
                                cfg.hoverAction = LatteTasks.types.PreviewAndHighlightWindows;
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

                        currentIndex: cfg.taskScrollAction
                        onCurrentIndexChanged: cfg.taskScrollAction = currentIndex
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

                            currentIndex: cfg.modifier
                            onCurrentIndexChanged: cfg.modifier = currentIndex
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

                            currentIndex: cfg.modifierClick
                            onCurrentIndexChanged: cfg.modifierClick = currentIndex
                        }

                        PlasmaComponents.Label {
                            text: "="
                        }

                        LatteComponents.ComboBox {
                            id: modifierClickAction
                            Layout.fillWidth: true
                            model: [i18nc("The click action", "None"), i18n("Close Window or Group"),
                                i18n("New Instance"), i18n("Minimize/Restore Window or Group"), i18n("Cycle Through Tasks"), i18n("Toggle Task Grouping"),
                                i18nc("present windows action", "Present Windows"), i18n("Preview Windows"),
                                i18n("Highlight Windows"), i18n("Preview and Highlight Windows")]

                            currentIndex: cfg.modifierClickAction
                            onCurrentIndexChanged: cfg.modifierClickAction = currentIndex
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
