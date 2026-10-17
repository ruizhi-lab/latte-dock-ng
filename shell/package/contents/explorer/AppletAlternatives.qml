/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.plasmoid
import org.kde.kirigami as Kirigami

import org.kde.plasma.private.shell

PlasmaCore.Dialog {
    id: dialog

    //! Latte's copy of Plasma's applet alternatives dialog. Since libplasma 6.8
    //! the Plasma version requires the AlternativesHelper type of
    //! org.kde.plasma.shell, a module compiled into plasmashell: in Latte the
    //! import resolves to an empty module and the shared fallback file failed
    //! to load, so "Show Alternatives..." did nothing. Latte publishes its own
    //! Latte::AlternativesHelper as the alternativesHelper context property
    //! before completing creation (Corona::showAlternativesForApplet()).
    readonly property QtObject helper: alternativesHelper

    visualParent: helper.applet
    location: helper.applet.Plasmoid.location
    hideOnWindowDeactivate: true
    backgroundHints: (helper.applet.Plasmoid.containmentDisplayHints & PlasmaCore.Types.ContainmentPrefersOpaqueBackground) ? PlasmaCore.Dialog.SolidBackground : PlasmaCore.Dialog.StandardBackground

    Component.onCompleted: {
        flags = flags |  Qt.WindowStaysOnTopHint;
        dialog.show();
    }

    ColumnLayout {
        id: root

        signal configurationChanged

        Layout.minimumWidth: Kirigami.Units.gridUnit * 20
        Layout.minimumHeight: Math.min(Screen.height - Kirigami.Units.gridUnit * 10, implicitHeight)

        LayoutMirroring.enabled: Application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        property string currentPlugin: ""

        Shortcut {
            sequence: "Escape"
            onActivated: dialog.close()
        }
        Shortcut {
            sequence: "Return"
            onActivated: root.savePluginAndClose()
        }
        Shortcut {
            sequence: "Enter"
            onActivated: root.savePluginAndClose()
        }


        WidgetExplorer {
            id: widgetExplorer
            provides: dialog.helper.appletProvides
        }

        PlasmaExtras.PlasmoidHeading {
            Kirigami.Heading {
                id: heading
                text: i18ndc("plasma_shell_org.kde.plasma.desktop", "@title:window for widget alternatives explorer", "Alternative Widgets")
                textFormat: Text.PlainText
            }
        }

        // This timer checks with a short delay whether a new item in the list has been hovered by the cursor.
        // If not, then the cursor has left the view and thus no item should be selected.
        Timer {
            id: resetCurrentIndex
            property string oldPlugin
            interval: 100
            onTriggered: {
                if (root.currentPlugin === oldPlugin) {
                    mainList.currentIndex = -1
                    root.currentPlugin = ""
                }
            }
        }

        function savePluginAndClose() {
            dialog.helper.loadAlternative(currentPlugin);
            dialog.close();
        }

        PlasmaComponents3.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Layout.preferredHeight: mainList.contentHeight

            focus: true

            ListView {
                id: mainList

                focus: dialog.visible
                model: widgetExplorer.widgetsModel
                boundsBehavior: Flickable.StopAtBounds
                highlight: PlasmaExtras.Highlight {
                    pressed: mainList.currentItem && mainList.currentItem.pressed
                }
                highlightMoveDuration : 0
                highlightResizeDuration: 0

                height: contentHeight+Kirigami.Units.smallSpacing

                delegate: PlasmaComponents3.ItemDelegate {
                    id: listItem

                    implicitHeight: contentLayout.implicitHeight + Kirigami.Units.smallSpacing * 2
                    width: ListView.view.width

                    Accessible.name: model.name
                    Accessible.description: model.description

                    onHoveredChanged: {
                        if (hovered) {
                            resetCurrentIndex.stop()
                            mainList.currentIndex = index
                        } else {
                            resetCurrentIndex.oldPlugin = model.pluginName
                            resetCurrentIndex.restart()
                        }
                    }

                    Connections {
                        target: mainList
                        function onCurrentIndexChanged() {
                            if (mainList.currentIndex === model.index) {
                                root.currentPlugin = model.pluginName
                            }
                        }
                    }

                    onClicked: root.savePluginAndClose()

                    Component.onCompleted: {
                        if (model.pluginName === dialog.helper.currentPlugin) {
                            root.currentPlugin = model.pluginName
                            setAsCurrent.restart()
                        }
                    }

                    // we don't want to select any entry by default
                    // this cannot be set in Component.onCompleted
                    Timer {
                        id: setAsCurrent
                        interval: 100
                        onTriggered: {
                            mainList.currentIndex = index
                        }
                    }

                    contentItem: RowLayout {
                        id: contentLayout
                        spacing: Kirigami.Units.largeSpacing

                        Kirigami.Icon {
                            implicitWidth: Kirigami.Units.iconSizes.huge
                            implicitHeight: Kirigami.Units.iconSizes.huge
                            source: model.decoration
                        }

                        ColumnLayout {
                            id: labelLayout

                            readonly property color textColor: listItem.pressed ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor

                            Layout.fillHeight: true
                            Layout.fillWidth: true
                            spacing: 0 // The labels bring their own bottom margins

                            Kirigami.Heading {
                                level: 4
                                Layout.fillWidth: true
                                text: model.name
                                textFormat: Text.PlainText
                                elide: Text.ElideRight
                                type: model.pluginName === dialog.helper.currentPlugin ? PlasmaExtras.Heading.Type.Primary : PlasmaExtras.Heading.Type.Normal
                                color: labelLayout.textColor
                            }

                            PlasmaComponents3.Label {
                                Layout.fillWidth: true
                                text: model.description
                                textFormat: Text.PlainText
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                font.family: Kirigami.Theme.smallFont.family
                                font.bold: model.pluginName === dialog.helper.currentPlugin
                                opacity: 0.75
                                maximumLineCount: 2
                                wrapMode: Text.WordWrap
                                elide: Text.ElideRight
                                color: labelLayout.textColor
                            }
                        }
                    }
                }
            }
        }
    }
}
