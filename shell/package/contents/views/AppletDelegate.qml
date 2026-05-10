/*
    SPDX-FileCopyrightText: 2011 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2015 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2021 Michail Vourlakos <mvourlakos@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.1

import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.kirigami 2.0 as Kirigami
import org.kde.kwindowsystem 1.0
import org.kde.draganddrop 2.0

Item {
    id: delegate

    readonly property string pluginName: model.pluginName
    readonly property bool pendingUninstall: pendingUninstallTimer.applets.indexOf(pluginName) > -1
    readonly property int runningInstances: main.runningInstancesFor(pluginName)

    width: list.cellWidth
    height: list.cellHeight

    HoverHandler {
        onHoveredChanged: if (hovered) delegate.GridView.view.currentIndex = index
    }

    DragArea {
        anchors.fill: parent
        supportedActions: Qt.MoveAction | Qt.LinkAction
        delegateImage: decoration
        enabled: !delegate.pendingUninstall
        mimeData {
            source: parent
        }
        Component.onCompleted: mimeData.setData("text/x-plasmoidservicename", pluginName)

        onDragStarted: {
            KWindowSystem.showingDesktop = true;
            main.draggingWidget = true;
        }
        onDrop: {
            main.draggingWidget = false;
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            onDoubleClicked: {
                if (!delegate.pendingUninstall) {
                    widgetExplorer.addApplet(pluginName);
                    main.scheduleRunningCountRefresh();
                }
            }
        }

        ColumnLayout {
            id: mainLayout
            spacing: Kirigami.Units.smallSpacing
            anchors {
                left: parent.left
                right: parent.right
                margins: Kirigami.Units.smallSpacing * 2
                rightMargin: Kirigami.Units.smallSpacing * 2
                top: parent.top
            }

            Item {
                id: iconContainer
                width: Kirigami.Units.iconSizes.enormous
                height: width
                Layout.alignment: Qt.AlignHCenter
                opacity: delegate.pendingUninstall ? 0.6 : 1
                Behavior on opacity {
                    OpacityAnimator {
                        duration: Kirigami.Units.longDuration
                        easing.type: Easing.InOutQuad
                    }
                }

                Kirigami.Icon {
                    anchors.fill: parent
                    source: model.decoration
                    visible: model.screenshot === ""
                }
                Image {
                    anchors.fill: parent
                    fillMode: Image.PreserveAspectFit
                    source: model.screenshot
                }

                // Top-left: instance-count badge (or "New!" badge)
                Rectangle {
                    id: overlayedBadge
                    anchors {
                        top: parent.top
                        left: parent.left
                    }
                    width: countLabel.implicitWidth + height
                    height: Math.round(Kirigami.Units.iconSizes.sizeForLabels * 1.3)
                    radius: height / 2
                    color: (delegate.runningInstances > 0 && delegate.GridView.isCurrentItem) ? Kirigami.Theme.highlightColor : Kirigami.Theme.positiveTextColor
                    visible: ((delegate.runningInstances > 0 && delegate.GridView.isCurrentItem) || model.recent) ?? false

                    PlasmaComponents.Label {
                        id: countLabel
                        anchors.centerIn: parent
                        text: (delegate.runningInstances > 0 && delegate.GridView.isCurrentItem)
                            ? delegate.runningInstances
                            : i18ndc("plasma_shell_org.kde.plasma.desktop", "Text displayed on top of newly installed widgets", "New!")
                        color: "white"
                    }
                }

                // Top-right (when uninstall present): remove all running instances
                PlasmaComponents.ToolButton {
                    id: removeInstancesButton
                    Kirigami.Theme.colorSet: Kirigami.Theme.Complementary
                    Kirigami.Theme.inherit: false
                    anchors {
                        top: parent.top
                        right: uninstallButton.visible ? uninstallButton.left : parent.right
                        rightMargin: uninstallButton.visible ? Kirigami.Units.smallSpacing : 0
                    }
                    icon.name: "edit-clear-all"
                    display: PlasmaComponents.AbstractButton.IconOnly
                    flat: false
                    visible: (delegate.runningInstances > 0 && delegate.GridView.isCurrentItem) ?? false

                    text: i18ndc("plasma_shell_org.kde.plasma.desktop", "@action:button", "Remove all instances")
                    PlasmaComponents.ToolTip.delay: Kirigami.Units.toolTipDelay
                    PlasmaComponents.ToolTip.visible: hovered
                    PlasmaComponents.ToolTip.text: text

                    onHoveredChanged: {
                        if (hovered) {
                            delegate.GridView.view.currentIndex = index
                        }
                    }

                    onClicked: {
                        if (typeof widgetExplorer.removeAllInstances === "function") {
                            widgetExplorer.removeAllInstances(pluginName)
                        }
                    }
                }

                // Top-right: uninstall (only for user-installed widgets)
                PlasmaComponents.ToolButton {
                    id: uninstallButton
                    Kirigami.Theme.colorSet: Kirigami.Theme.Complementary
                    Kirigami.Theme.inherit: false
                    anchors {
                        top: parent.top
                        right: parent.right
                    }
                    icon.name: delegate.pendingUninstall ? "edit-undo" : "edit-delete"
                    display: PlasmaComponents.AbstractButton.IconOnly
                    flat: false
                    visible: (model.local && delegate.GridView.isCurrentItem) ?? false

                    text: delegate.pendingUninstall
                        ? i18nd("plasma_shell_org.kde.plasma.desktop", "Cancel Uninstallation")
                        : i18nd("plasma_shell_org.kde.plasma.desktop", "Uninstall widget")
                    PlasmaComponents.ToolTip.delay: Kirigami.Units.toolTipDelay
                    PlasmaComponents.ToolTip.visible: hovered
                    PlasmaComponents.ToolTip.text: text

                    onHoveredChanged: {
                        if (hovered) {
                            delegate.GridView.view.currentIndex = index
                        }
                    }

                    onClicked: {
                        var pending = pendingUninstallTimer.applets
                        if (delegate.pendingUninstall) {
                            var idx = pending.indexOf(pluginName)
                            if (idx > -1) {
                                pending.splice(idx, 1)
                            }
                        } else {
                            pending.push(pluginName)
                        }
                        pendingUninstallTimer.applets = pending

                        if (pending.length) {
                            pendingUninstallTimer.restart()
                        } else {
                            pendingUninstallTimer.stop()
                        }
                    }
                }
            }
            PlasmaExtras.Heading {
                id: heading
                Layout.fillWidth: true
                level: 4
                text: model.name
                elide: Text.ElideRight
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                lineHeight: 0.95
                horizontalAlignment: Text.AlignHCenter
            }
            PlasmaComponents.Label {
                Layout.fillWidth: true
                height: implicitHeight
                text: model.description
                font: Kirigami.Theme.smallFont
                wrapMode: Text.WordWrap
                elide: Text.ElideRight
                maximumLineCount: heading.lineCount === 1 ? 3 : 2
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
