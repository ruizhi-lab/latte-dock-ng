/*
    SPDX-FileCopyrightText: 2016 Smith AR <audoban@openmailbox.org>
    SPDX-FileCopyrightText: 2016 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.components as PlasmaComponents

import org.kde.latte.core as LatteCore
import org.kde.latte.components as LatteComponents

import org.kde.latte.private.tasks as LatteTasks

PlasmaComponents.Page {
    id: _tasksPage
    required property var dialog
    required property var latteView
    required property int index
    readonly property var units: _tasksPage.dialog.units
    width: content.width + content.Layout.leftMargin * 2
    height: content.height + units.smallSpacing * 2

    // Use latteView.extendedInterface to get the SAME configuration
    // object the plasmoid uses, not a stale copy.
    readonly property var cfg: latteView.extendedInterface.configurationForAppletVisualIndex(_tasksPage.index)
    property bool disableAllWindowsFunctionality: cfg.hideAllTasks

    readonly property bool isCurrentPage: (dialog.currentPage === _tasksPage)

    onIsCurrentPageChanged: {
        if (isCurrentPage && latteView.extendedInterface.latteTasksModel.count>1) {
            latteView.extendedInterface.appletRequestedVisualIndicator(tasks.plasmoid.id);
        }
    }

    ColumnLayout {
        id: content

        width: (_tasksPage.dialog.appliedWidth - units.smallSpacing * 2) - Layout.leftMargin * 2
        spacing: _tasksPage.dialog.subGroupSpacing
        anchors.horizontalCenter: parent.horizontalCenter
        Layout.leftMargin: units.smallSpacing * 2
        Layout.rightMargin: units.smallSpacing * 2

        //! BEGIN: Badges
        ColumnLayout {
            spacing: units.smallSpacing
            Layout.topMargin: units.smallSpacing
            visible: _tasksPage.dialog.advancedLevel

            LatteComponents.Header {
                text: i18n("Badges")
            }

            LatteComponents.CheckBoxesColumn {
                Layout.leftMargin: units.smallSpacing * 2
                Layout.rightMargin: units.smallSpacing * 2

                LatteComponents.CheckBox {
                    Layout.maximumWidth: _tasksPage.dialog.optionsWidth
                    text: i18n("Notifications from tasks")
                    tooltip: i18n("Show unread messages or notifications from tasks")
                    value: _tasksPage.cfg.showInfoBadge

                    onClicked: {
                        _tasksPage.cfg.showInfoBadge = !_tasksPage.cfg.showInfoBadge;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: _tasksPage.dialog.optionsWidth
                    text: i18n("Progress information for tasks")
                    tooltip: i18n("Show a progress animation for tasks e.g. when copying files with Dolphin")
                    value: _tasksPage.cfg.showProgressBadge

                    onClicked: {
                        _tasksPage.cfg.showProgressBadge = !_tasksPage.cfg.showProgressBadge;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: _tasksPage.dialog.optionsWidth
                    text: i18n("Audio playing from tasks")
                    tooltip: i18n("Show audio playing from tasks")
                    value: _tasksPage.cfg.showAudioBadge

                    onClicked: {
                        _tasksPage.cfg.showAudioBadge = !_tasksPage.cfg.showAudioBadge;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: _tasksPage.dialog.optionsWidth
                    text: i18n("Prominent color for notification badge")
                    enabled: _tasksPage.cfg.showInfoBadge
                    tooltip: i18n("Notification badge uses a more prominent background which is usually red")
                    value: _tasksPage.cfg.infoBadgeProminentColorEnabled

                    onClicked: {
                        _tasksPage.cfg.infoBadgeProminentColorEnabled = !_tasksPage.cfg.infoBadgeProminentColorEnabled;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: _tasksPage.dialog.optionsWidth
                    text: i18n("Change volume when scrolling audio badge")
                    enabled: _tasksPage.cfg.showAudioBadge
                    tooltip: i18n("The user is able to mute/unmute with click or change the volume with mouse wheel")
                    value: _tasksPage.cfg.audioBadgeActionsEnabled

                    onClicked: {
                        _tasksPage.cfg.audioBadgeActionsEnabled = !_tasksPage.cfg.audioBadgeActionsEnabled;
                    }
                }
            }
        }
        //! END: Badges

        //! BEGIN: Tasks Interaction
        ColumnLayout {
            Layout.topMargin: _tasksPage.dialog.basicLevel ? units.smallSpacing : 0
            spacing: units.smallSpacing

            LatteComponents.Header {
                text: i18n("Interaction")
            }

            LatteComponents.CheckBoxesColumn {
                Layout.leftMargin: units.smallSpacing * 2
                Layout.rightMargin: units.smallSpacing * 2

                LatteComponents.CheckBox {
                    Layout.maximumWidth: _tasksPage.dialog.optionsWidth
                    text: i18n("Launchers are added only in current tasks applet")
                    tooltip: i18n("Launchers are added only in current tasks applet and not as regular applets or in any other applet")
                    value:_tasksPage.cfg.isPreferredForDroppedLaunchers

                    onClicked: {
                        _tasksPage.cfg.isPreferredForDroppedLaunchers = !_tasksPage.cfg.isPreferredForDroppedLaunchers;
                    }
                }

                LatteComponents.CheckBox {
                    id: windowActionsChk
                    Layout.maximumWidth: _tasksPage.dialog.optionsWidth
                    text: i18n("Window actions in the context menu")
                    visible: _tasksPage.dialog.advancedLevel
                    enabled: !_tasksPage.disableAllWindowsFunctionality
                    value: _tasksPage.cfg.showWindowActions

                    onClicked: {
                        _tasksPage.cfg.showWindowActions = !_tasksPage.cfg.showWindowActions;
                    }
                }

                LatteComponents.CheckBox {
                    id: previewPopupChk
                    Layout.maximumWidth: _tasksPage.dialog.optionsWidth
                    text: i18n("Preview window behaves as popup")
                    visible: _tasksPage.dialog.advancedLevel
                    enabled: !_tasksPage.disableAllWindowsFunctionality
                    value: _tasksPage.cfg.previewWindowAsPopup

                    onClicked: {
                        _tasksPage.cfg.previewWindowAsPopup = !_tasksPage.cfg.previewWindowAsPopup;
                    }
                }

                LatteComponents.CheckBox {
                    id: unifyGlobalShortcutsChk
                    Layout.maximumWidth: _tasksPage.dialog.optionsWidth
                    text: i18n("Based on position shortcuts apply only on current tasks")
                    // checked: cfg.isPreferredForPositionShortcuts //! Disabled because it was not updated between multiple Tasks
                    tooltip: i18n("Based on position global shortcuts are enabled only for current tasks and not for other applets")
                    visible: _tasksPage.dialog.advancedLevel
                    enabled: _tasksPage.latteView.isPreferredForShortcuts || (!_tasksPage.latteView.layout.preferredForShortcutsTouched && _tasksPage.latteView.isHighestPriorityView())
                    value: _tasksPage.cfg.isPreferredForPositionShortcuts

                    onClicked: {
                        _tasksPage.cfg.isPreferredForPositionShortcuts = !_tasksPage.cfg.isPreferredForPositionShortcuts;
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
                    Layout.maximumWidth: _tasksPage.dialog.optionsWidth
                    text: i18n("Show only tasks from the current screen")
                    enabled: !_tasksPage.disableAllWindowsFunctionality
                    value: _tasksPage.cfg.showOnlyCurrentScreen

                    onClicked: {
                        _tasksPage.cfg.showOnlyCurrentScreen = !_tasksPage.cfg.showOnlyCurrentScreen;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: _tasksPage.dialog.optionsWidth
                    text: i18n("Show only tasks from the current desktop")
                    enabled: !_tasksPage.disableAllWindowsFunctionality
                    value: _tasksPage.cfg.showOnlyCurrentDesktop

                    onClicked: {
                        _tasksPage.cfg.showOnlyCurrentDesktop = !_tasksPage.cfg.showOnlyCurrentDesktop;
                    }
                }

                // KActivities support is unreliable in Plasma 6, hidden until restored
                LatteComponents.CheckBox {
                    Layout.maximumWidth: _tasksPage.dialog.optionsWidth
                    text: i18n("Show only tasks from the current activity")
                    visible: false
                    enabled: !_tasksPage.disableAllWindowsFunctionality
                    value: _tasksPage.cfg.showOnlyCurrentActivity

                    onClicked: {
                        _tasksPage.cfg.showOnlyCurrentActivity = !_tasksPage.cfg.showOnlyCurrentActivity;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: _tasksPage.dialog.optionsWidth
                    text: i18n("Show Minimized Window Section")
                    enabled: !_tasksPage.disableAllWindowsFunctionality
                    value: _tasksPage.cfg.showMinimizedWindowSection
                    onClicked: _tasksPage.cfg.showMinimizedWindowSection = !value
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: _tasksPage.dialog.optionsWidth
                    text: i18n("Show only tasks from launchers")
                    visible: _tasksPage.dialog.advancedLevel
                    enabled: !_tasksPage.disableAllWindowsFunctionality
                    value: _tasksPage.cfg.showWindowsOnlyFromLaunchers

                    onClicked: {
                        _tasksPage.cfg.showWindowsOnlyFromLaunchers = !_tasksPage.cfg.showWindowsOnlyFromLaunchers;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: _tasksPage.dialog.optionsWidth
                    text: i18n("Show only launchers and hide all tasks")
                    tooltip: i18n("Tasks become hidden and only launchers are shown")
                    visible: _tasksPage.dialog.advancedLevel
                    value: _tasksPage.cfg.hideAllTasks

                    onClicked: {
                        _tasksPage.cfg.hideAllTasks = !_tasksPage.cfg.hideAllTasks;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: _tasksPage.dialog.optionsWidth
                    text: i18n("Show only grouped tasks for same application")
                    tooltip: i18n("By default group tasks of the same application")
                    visible: _tasksPage.dialog.advancedLevel
                    enabled: !_tasksPage.disableAllWindowsFunctionality
                    value: _tasksPage.cfg.groupTasksByDefault

                    onClicked: {
                        _tasksPage.cfg.groupTasksByDefault = !_tasksPage.cfg.groupTasksByDefault;
                    }
                }
            }
        }

        //! END: Tasks Filters

        //! BEGIN: Animations
        ColumnLayout {
            spacing: units.smallSpacing
            enabled: plasmoid.configuration.animationsEnabled
            visible: _tasksPage.dialog.advancedLevel

            LatteComponents.Header {
                text: i18n("Animations")
            }

            LatteComponents.CheckBoxesColumn {
                Layout.leftMargin: units.smallSpacing * 2
                Layout.rightMargin: units.smallSpacing * 2

                LatteComponents.CheckBox {
                    Layout.maximumWidth: _tasksPage.dialog.optionsWidth
                    text: i18n("Bounce launchers when triggered")
                    value: _tasksPage.cfg.animationLauncherBouncing
                    enabled: !_tasksPage.latteView.indicator.info.providesTaskLauncherAnimation

                    onClicked: {
                        _tasksPage.cfg.animationLauncherBouncing = !_tasksPage.cfg.animationLauncherBouncing;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: _tasksPage.dialog.optionsWidth
                    text: i18n("Bounce tasks that need attention")
                    value: _tasksPage.cfg.animationWindowInAttention
                    enabled: !_tasksPage.latteView.indicator.info.providesInAttentionAnimation

                    onClicked: {
                        _tasksPage.cfg.animationWindowInAttention = !_tasksPage.cfg.animationWindowInAttention;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: _tasksPage.dialog.optionsWidth
                    text: i18n("Slide in and out single windows")
                    value: _tasksPage.cfg.animationNewWindowSliding

                    onClicked: {
                        _tasksPage.cfg.animationNewWindowSliding = !_tasksPage.cfg.animationNewWindowSliding;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: _tasksPage.dialog.optionsWidth
                    text: i18n("Grouped tasks bounce their new windows")
                    value: _tasksPage.cfg.animationWindowAddedInGroup
                    enabled: !_tasksPage.latteView.indicator.info.providesGroupedWindowAddedAnimation

                    onClicked: {
                        _tasksPage.cfg.animationWindowAddedInGroup = !_tasksPage.cfg.animationWindowAddedInGroup;
                    }
                }

                LatteComponents.CheckBox {
                    Layout.maximumWidth: _tasksPage.dialog.optionsWidth
                    text: i18n("Grouped tasks slide out their closed windows")
                    value: _tasksPage.cfg.animationWindowRemovedFromGroup
                    enabled: !_tasksPage.latteView.indicator.info.providesGroupedWindowRemovedAnimation

                    onClicked: {
                        _tasksPage.cfg.animationWindowRemovedFromGroup = !_tasksPage.cfg.animationWindowRemovedFromGroup;
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

                    property int group: _tasksPage.cfg.launchersGroup

                    readonly property int buttonsCount: layoutGroupButton.visible ? 3 : 2
                    readonly property int buttonSize: (_tasksPage.dialog.optionsWidth - (spacing * buttonsCount-1)) / buttonsCount

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
                                _tasksPage.cfg.launchersGroup = group;
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
                        visible: _tasksPage.cfg.launchersGroup === group

                        readonly property int group: LatteCore.types.LayoutLaunchers

                        onPressedChanged: {
                            if (pressed) {
                                _tasksPage.cfg.launchersGroup = group;
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
                                _tasksPage.cfg.launchersGroup = group;
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
            visible: _tasksPage.dialog.advancedLevel

            LatteComponents.HeaderSwitch {
                id: scrollingHeader
                Layout.minimumWidth: _tasksPage.dialog.optionsWidth + 2 *units.smallSpacing
                Layout.maximumWidth: Layout.minimumWidth
                Layout.minimumHeight: implicitHeight
                Layout.bottomMargin: units.smallSpacing
                checked: _tasksPage.cfg.scrollTasksEnabled
                text: i18n("Scrolling")
                tooltip: i18n("Enable tasks scrolling when they overflow and exceed the available space");

                onPressed: {
                    _tasksPage.cfg.scrollTasksEnabled = !_tasksPage.cfg.scrollTasksEnabled;;
                }
            }

            ColumnLayout {
                Layout.leftMargin: units.smallSpacing * 2
                Layout.rightMargin: units.smallSpacing * 2
                spacing: 0
                enabled: scrollingHeader.checked

                GridLayout {
                    columns: 2
                    Layout.minimumWidth: _tasksPage.dialog.optionsWidth
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
                            _tasksPage.dialog.panelIsVertical ? i18n("Only vertical scrolling") : i18n("Only horizontal scrolling"),
                            i18n("Horizontal and vertical scrolling")]

                        currentIndex: _tasksPage.cfg.manualScrollTasksType
                        onCurrentIndexChanged: _tasksPage.cfg.manualScrollTasksType = currentIndex;
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

                        currentIndex: _tasksPage.cfg.autoScrollTasksEnabled
                        onCurrentIndexChanged: {
                            if (currentIndex === 0) {
                                _tasksPage.cfg.autoScrollTasksEnabled = false;
                            } else {
                                _tasksPage.cfg.autoScrollTasksEnabled = true;
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
            visible: _tasksPage.dialog.advancedLevel

            LatteComponents.Header {
                text: i18n("Actions")
            }

            ColumnLayout {
                Layout.leftMargin: units.smallSpacing * 2
                Layout.rightMargin: units.smallSpacing * 2
                spacing: 0

                GridLayout {
                    columns: 2
                    Layout.minimumWidth: _tasksPage.dialog.optionsWidth
                    Layout.maximumWidth: Layout.minimumWidth

                    Layout.topMargin: units.smallSpacing
                    enabled: !_tasksPage.disableAllWindowsFunctionality

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

                        currentIndex: _tasksPage.cfg.leftClickAction
                        onCurrentIndexChanged: _tasksPage.cfg.leftClickAction = currentIndex
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

                        currentIndex: _tasksPage.cfg.middleClickAction
                        onCurrentIndexChanged: _tasksPage.cfg.middleClickAction = currentIndex
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
                            switch(_tasksPage.cfg.hoverAction) {
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
                                _tasksPage.cfg.hoverAction = LatteTasks.types.NoneAction;
                                break;
                            case 1:
                                _tasksPage.cfg.hoverAction = LatteTasks.types.PreviewWindows;
                                break;
                            case 2:
                                _tasksPage.cfg.hoverAction = LatteTasks.types.HighlightWindows;
                                break;
                            case 3:
                                _tasksPage.cfg.hoverAction = LatteTasks.types.PreviewAndHighlightWindows;
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

                        currentIndex: _tasksPage.cfg.taskScrollAction
                        onCurrentIndexChanged: _tasksPage.cfg.taskScrollAction = currentIndex
                    }

                    RowLayout {
                        spacing: units.smallSpacing
                        enabled: !_tasksPage.disableAllWindowsFunctionality

                        Layout.minimumWidth: middleClickText.width
                        Layout.maximumWidth: middleClickText.width

                        LatteComponents.ComboBox {
                            id: modifier
                            Layout.fillWidth: true
                            model: ["Shift", "Ctrl", "Alt", "Meta"]

                            currentIndex: _tasksPage.cfg.modifier
                            onCurrentIndexChanged: _tasksPage.cfg.modifier = currentIndex
                        }

                        PlasmaComponents.Label {
                            text: "+"
                        }
                    }

                    RowLayout {
                        spacing: units.smallSpacing
                        enabled: !_tasksPage.disableAllWindowsFunctionality

                        readonly property int maxSize: 0.4 * _tasksPage.dialog.optionsWidth

                        LatteComponents.ComboBox {
                            id: modifierClick
                            Layout.preferredWidth: 0.7 * parent.maxSize
                            Layout.maximumWidth: parent.maxSize
                            model: [i18n("Left Click"), i18n("Middle Click"), i18n("Right Click")]

                            currentIndex: _tasksPage.cfg.modifierClick
                            onCurrentIndexChanged: _tasksPage.cfg.modifierClick = currentIndex
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

                            currentIndex: _tasksPage.cfg.modifierClickAction
                            onCurrentIndexChanged: _tasksPage.cfg.modifierClickAction = currentIndex
                        }
                    }
                }

                RowLayout {
                    Layout.minimumWidth: _tasksPage.dialog.optionsWidth
                    Layout.maximumWidth: Layout.minimumWidth
                    Layout.topMargin: units.smallSpacing
                    spacing: units.smallSpacing
                    enabled: !_tasksPage.disableAllWindowsFunctionality

                }
            }
        }
        //! END: Actions


    }
}
