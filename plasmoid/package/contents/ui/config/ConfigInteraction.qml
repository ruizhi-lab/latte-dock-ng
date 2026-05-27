/*
    SPDX-FileCopyrightText: 2016 Smith AR <audoban@openmailbox.org>
    SPDX-FileCopyrightText: 2016 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.0

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents

import org.kde.latte.private.tasks 0.1 as LatteTasks

Item {
    id: mainItem

    width: childrenRect.width
    height: childrenRect.height

    property bool vertical: (plasmoid.formFactor == PlasmaCore.Types.Vertical)

    property alias cfg_middleClickAction: middleClickAction.currentIndex
    property int cfg_hoverAction: LatteTasks.types.NoneAction

    property alias cfg_showOnlyCurrentScreen: showOnlyCurrentScreen.checked
    property alias cfg_showOnlyCurrentDesktop: showOnlyCurrentDesktop.checked
    property alias cfg_showOnlyCurrentActivity: showOnlyCurrentActivity.checked

    property alias cfg_showInfoBadge: showInfoBadgeChk.checked
    property alias cfg_showWindowActions: windowActionsChk.checked

    function hoverActionToIndex(action) {
        switch(action) {
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

    function hoverIndexToAction(index) {
        switch(index) {
        case 1:
            return LatteTasks.types.PreviewWindows;
        case 2:
            return LatteTasks.types.HighlightWindows;
        case 3:
            return LatteTasks.types.PreviewAndHighlightWindows;
        }

        return LatteTasks.types.NoneAction;
    }

    ColumnLayout{
        spacing: 15

        GroupBox {
            title: ""
            Layout.fillWidth: true

            ColumnLayout {
                Layout.fillWidth: true

                CheckBox {
                    id: windowActionsChk
                    Layout.fillWidth: true
                    text: i18n("Show window actions in the context menu")
                }

                CheckBox {
                    id: showInfoBadgeChk
                    Layout.fillWidth: true
                    text: i18n("Show progress information for tasks")
                }

                GridLayout {
                    columns: 2

                    Label {
                        text: i18n("Middle Click")
                    }

                    ComboBox {
                        id: middleClickAction
                        Layout.fillWidth: true
                        model: [i18nc("The click action", "None"), i18n("Close Window or Group"), i18n("New Instance"), i18n("Minimize/Restore Window or Group")]
                    }

                    Label {
                        text: i18n("Hover")
                    }

                    ComboBox {
                        id: hoverActionCmb
                        Layout.fillWidth: true
                        model: [
                            i18nc("none action", "None"),
                            i18n("Preview Windows"),
                            i18n("Highlight Windows"),
                            i18n("Preview and Highlight Windows"),
                        ]

                        currentIndex: mainItem.hoverActionToIndex(mainItem.cfg_hoverAction)

                        onCurrentIndexChanged: {
                            mainItem.cfg_hoverAction = mainItem.hoverIndexToAction(currentIndex);
                        }
                    }

                }
            }
        }


        ColumnLayout {
            Layout.fillWidth: true


            Label {
                text: i18n("Filters")
               // Layout.fillWidth: true
                anchors.horizontalCenter: parent.horizontalCenter
               // anchors.centerIn: parent
                font.bold: true
                font.italic: true
            }


            CheckBox {
                id: showOnlyCurrentScreen
                text: i18n("Show only tasks from the current screen")
            }

            CheckBox {
                id: showOnlyCurrentDesktop
                text: i18n("Show only tasks from the current desktop")
            }

            CheckBox {
                id: showOnlyCurrentActivity
                text: i18n("Show only tasks from the current activity")
            }
        }

    }

}
