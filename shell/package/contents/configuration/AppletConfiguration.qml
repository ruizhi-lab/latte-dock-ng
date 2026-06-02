/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2020 Nicolas Fella <nicolas.fella@gmx.de>
    SPDX-FileCopyrightText: 2020 Carl Schwan <carlschwan@kde.org>
    SPDX-FileCopyrightText: 2022-2023 ivan tkachenko <me@ratijas.tk>
    SPDX-FileCopyrightText: 2026 Latte Dock NG Contributors

    SPDX-License-Identifier: GPL-2.0-or-later

    Latte-specific variant: replaces Kirigami.ApplicationItem with a simpler
    QQC2.StackView + footer layout to avoid Kirigami OverlayDrawer version
    compatibility issues (onActiveValueChanged signal not available).
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kcmutils as KCMUtils
import org.kde.kirigami as Kirigami
import org.kde.kitemmodels as KItemModels
import org.kde.plasma.configuration
import org.kde.plasma.plasmoid

Rectangle {
    id: root

    implicitWidth: Kirigami.Units.gridUnit * 45
    implicitHeight: Kirigami.Units.gridUnit * 35

    Layout.minimumWidth: Kirigami.Units.gridUnit * 30
    Layout.minimumHeight: Kirigami.Units.gridUnit * 21

    LayoutMirroring.enabled: Application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    color: Kirigami.Theme.backgroundColor

    property bool isContainment: false

    property ConfigModel globalConfigModel: globalAppletConfigModel

    property url currentSource

    property bool wasConfigurationChangedSignalSent: false

    property alias currentConfigPage: pageStack.currentItem

    function closing() {
        if (applyButton.enabled) {
            messageDialog.item = null;
            messageDialog.open();
            return false;
        }
        return true;
    }

    function saveConfig() {
        const config = Plasmoid.configuration;

        // call applet's own config handling first so it can set cfg_ properties if needed
        if (pageStack.currentItem && pageStack.currentItem.saveConfig) {
            pageStack.currentItem.saveConfig()
        }

        config.keys().forEach(key => {
            const cfgKey = "cfg_" + key;
            if (pageStack.currentItem && (cfgKey in pageStack.currentItem)) {
                config[key] = pageStack.currentItem[cfgKey];
            }
        })

        plasmoid.configuration.writeConfig();
    }

    function isConfigurationChanged() {
        const config = Plasmoid.configuration;
        return config.keys().some(key => {
            const cfgKey = "cfg_" + key
            if (!pageStack.currentItem || !pageStack.currentItem.hasOwnProperty(cfgKey))
                return false
            return config[key] != pageStack.currentItem[cfgKey] &&
                   config[key].toString() != pageStack.currentItem[cfgKey].toString()
        })
    }

    Connections {
        target: configDialog
        function onClosing(event) {
            event.accepted = closing();
        }
    }

    ConfigModel {
        id: globalAppletConfigModel
        ConfigCategory {
            name: i18ndc("plasma_shell_org.kde.latte.shell", "@title:group for configuration dialog page", "Keyboard Shortcuts")
            icon: "preferences-desktop-keyboard"
            source: Qt.resolvedUrl("ConfigurationShortcuts.qml")
        }
    }

    KItemModels.KSortFilterProxyModel {
        id: configDialogFilterModel
        sourceModel: configDialog.configModel
        filterRowCallback: (row, parent) => {
            return sourceModel.data(sourceModel.index(row, 0), ConfigModel.VisibleRole);
        }
    }

    function settingValueChanged() {
        applyButton.enabled = wasConfigurationChangedSignalSent || isConfigurationChanged()
            || (pageStack.currentItem && pageStack.currentItem.unsavedChanges !== undefined
                ? pageStack.currentItem.unsavedChanges : false);
    }

    function pushReplace(item, config) {
        var page;
        if (pageStack.depth === 0) {
            page = pageStack.push(item, config);
        } else {
            page = pageStack.replace(item, config);
        }
    }

    function open(item) {
        root.currentSource = item.source

        if (item.configUiModule && item.configUiComponent) {
            root.currentSource = item.configUiModule + item.configUiComponent;
            const config = Plasmoid.configuration;

            const props = {
                "title": item.name,
            };

            config.keys().forEach(key => {
                props["cfg_" + key] = config[key];
            });

            pushReplace(Qt.createComponent(item.configUiModule, item.configUiComponent), props);
        } else if (item.source) {
            const config = Plasmoid.configuration;

            const props = { "title": item.name };

            config.keys().forEach(key => {
                props["cfg_" + key] = config[key];
            });

            pushReplace(Qt.resolvedUrl(item.source), props);
        } else {
            pageStack.pop();
        }

        applyButton.enabled = false
        wasConfigurationChangedSignalSent = false
    }

    Connections {
        target: pageStack.currentItem
        ignoreUnknownSignals: true

        function onSettingValueChanged() {
            wasConfigurationChangedSignalSent = true;
        }

        function onUnsavedChangesChanged() {
            root.settingValueChanged()
        }
    }

    Connections {
        target: pageStack

        function onCurrentItemChanged() {
            if (pageStack.currentItem !== null) {
                const config = Plasmoid.configuration;

                config.keys().forEach(key => {
                    const changedSignal = pageStack.currentItem["cfg_" + key + "Changed"];
                    if (changedSignal) {
                        changedSignal.connect(() => root.settingValueChanged());
                    }
                });

                const configurationChangedSignal = pageStack.currentItem.configurationChanged;
                if (configurationChangedSignal) {
                    configurationChangedSignal.connect(() => {
                        root.wasConfigurationChangedSignalSent = true
                        root.settingValueChanged()
                    });
                }

                const configurationUnsavedChangesSignal = pageStack.currentItem.unsavedChangesChanged
                if (configurationUnsavedChangesSignal) {
                    configurationUnsavedChangesSignal.connect(() => root.settingValueChanged())
                }
            }
        }
    }

    Component.onCompleted: {
        if (isContainment || !configDialog.configModel || configDialog.configModel.count === 0) {
            open(root.globalConfigModel.get(0))
        } else {
            open(configDialog.configModel.get(0))
        }
    }

    function applicationWindow() {
        return root;
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        QQC2.ScrollView {
            id: categoriesScroll
            Layout.preferredWidth: Kirigami.Units.gridUnit * 7
            Layout.fillHeight: true
            contentWidth: availableWidth
            Kirigami.Theme.colorSet: Kirigami.Theme.View
            Kirigami.Theme.inherit: false
            activeFocusOnTab: true
            focus: true
            Accessible.role: Accessible.PageTabList
            background: Rectangle {
                color: Kirigami.Theme.backgroundColor
            }

            Keys.onUpPressed: {
                const buttons = categories.children

                let foundPrevious = false
                for (let i = buttons.length - 1; i >= 0; --i) {
                    const button = buttons[i];
                    if (!button.hasOwnProperty("highlighted")) {
                        continue;
                    }

                    if (foundPrevious) {
                        categories.openCategory(button.item)
                        categoriesScroll.forceActiveFocus(Qt.TabFocusReason)
                        return
                    } else if (button.highlighted) {
                        foundPrevious = true
                    }
                }

                event.accepted = false
            }

            Keys.onDownPressed: {
                const buttons = categories.children

                let foundNext = false
                for (let i = 0, length = buttons.length; i < length; ++i) {
                    const button = buttons[i];
                    if (!button.hasOwnProperty("highlighted")) {
                        continue;
                    }

                    if (foundNext) {
                        categories.openCategory(button.item)
                        categoriesScroll.forceActiveFocus(Qt.TabFocusReason)
                        return
                    } else if (button.highlighted) {
                        foundNext = true
                    }
                }

                event.accepted = false
            }

            ColumnLayout {
                id: categories

                spacing: 0
                width: categoriesScroll.contentWidth
                focus: true

                function openCategory(item) {
                    if (applyButton.enabled) {
                        messageDialog.item = item;
                        messageDialog.open();
                        return;
                    }
                    open(item)
                }

                Component {
                    id: categoryDelegate
                    ConfigCategoryDelegate {
                        id: delegate
                        onActivated: categories.openCategory(model);
                        highlighted: {
                            if (pageStack.currentItem) {
                                if (model.configUiModule && model.configUiComponent) {
                                    return root.currentSource == (model.configUiModule + model.configUiComponent)
                                } else {
                                    return root.currentSource == model.source
                                }
                            }
                            return false
                        }
                        item: model
                    }
                }

                Repeater {
                    Layout.fillWidth: true
                    model: root.isContainment ? globalConfigModel : undefined
                    delegate: categoryDelegate
                }
                Repeater {
                    Layout.fillWidth: true
                    model: configDialogFilterModel
                    delegate: categoryDelegate
                }
                Repeater {
                    Layout.fillWidth: true
                    model: !root.isContainment ? globalConfigModel : undefined
                    delegate: categoryDelegate
                }
                Repeater {
                    Layout.fillWidth: true
                    model: ConfigModel {
                        ConfigCategory{
                            name: i18ndc("plasma_shell_org.kde.latte.shell", "@title:group for About dialog page", "About")
                            icon: "help-about"
                            source: Qt.resolvedUrl("AboutPlugin.qml")
                        }
                    }
                    delegate: categoryDelegate
                }
            }
        }

        Kirigami.Separator {
            id: verticalSeparator
            Layout.fillHeight: true
            implicitWidth: 1
            z: 1
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Kirigami.Separator {
                Layout.fillWidth: true
                implicitHeight: 1
                z: 1
            }

            // Page stack replaces Kirigami.ApplicationItem's built-in pageStack
            QQC2.StackView {
                id: pageStack
                Layout.fillWidth: true
                Layout.fillHeight: true

                property var globalToolBar: null
                property bool wideScreen: true
            }

            Kirigami.Separator {
                id: bottomSeparator
                visible: (pageStack.currentItem
                    && pageStack.currentItem.flickable
                    && !(pageStack.currentItem.flickable instanceof KCMUtils.GridViewKCM)
                    && !(pageStack.currentItem.flickable.atYBeginning
                    && pageStack.currentItem.flickable.atYEnd)) ?? false
                Layout.fillWidth: true
                implicitHeight: 1
            }

            // Footer with action buttons
            QQC2.Pane {
                Layout.fillWidth: true

                padding: Kirigami.Units.largeSpacing

                contentItem: RowLayout {
                    id: buttonsRow
                    spacing: Kirigami.Units.smallSpacing

                    Item {
                        Layout.fillWidth: true
                    }

                    Kirigami.PromptDialog {
                        id: messageDialog
                        property var item
                        title: i18ndc("plasma_shell_org.kde.latte.shell", "@title:window dialog title", "Apply Settings")
                        subtitle: i18ndc("plasma_shell_org.kde.latte.shell", "@label dialog body", "The current page has unsaved changes. Apply the changes or discard them?")
                        standardButtons: Kirigami.Dialog.Apply | Kirigami.Dialog.Discard | Kirigami.Dialog.Cancel
                        onApplied: {
                            applyAction.trigger()
                            discarded();
                        }
                        onDiscarded: {
                            wasConfigurationChangedSignalSent = false;
                            if (item) {
                                root.open(item);
                                messageDialog.close();
                            } else {
                                applyButton.enabled = false;
                                configDialog.close();
                            }
                        }
                    }

                    QQC2.Button {
                        icon.name: "dialog-ok"
                        text: i18ndc("plasma_shell_org.kde.latte.shell", "@action:button", "OK")
                        onClicked: acceptAction.trigger()
                    }
                    QQC2.Button {
                        id: applyButton
                        enabled: false
                        icon.name: "dialog-ok-apply"
                        text: i18ndc("plasma_shell_org.kde.latte.shell", "@action:button", "Apply")
                        onClicked: applyAction.trigger()
                    }
                    QQC2.Button {
                        icon.name: "dialog-cancel"
                        text: i18ndc("plasma_shell_org.kde.latte.shell", "@action:button", "Cancel")
                        onClicked: cancelAction.trigger()
                    }
                }
            }
        }
    }

    QQC2.Action {
        id: acceptAction
        onTriggered: {
            applyAction.trigger();
            configDialog.close();
        }
    }

    QQC2.Action {
        id: applyAction
        onTriggered: {
            root.saveConfig()
            wasConfigurationChangedSignalSent = false
            applyButton.enabled = false;
        }
    }

    QQC2.Action {
        id: cancelAction
        onTriggered: {
            if (root.closing()) {
                configDialog.close();
            }
        }
    }

    Keys.onReturnPressed: acceptAction.trigger();
    Keys.onEscapePressed: cancelAction.trigger();
}
