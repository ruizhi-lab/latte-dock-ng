/*
    SPDX-FileCopyrightText: 2019 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.latte.components as LatteComponents


LatteComponents.ComboBoxButton{
    id: custom
    required property var latteView
    required property var viewConfig
    checkable: true

    buttonToolTip:  custom.type === "install:" ? i18n("Install indicators from KDE Online Store or local files") :
                                                 i18n("Use %1 style for your indicators", buttonText)

    buttonIsTransparent: true
    buttonIsTriggeringMenu: false
    comboBoxButtonIsTransparent: true
    comboBoxButtonIsVisible: custom.latteView.indicator.customPluginsCount > 0

    comboBoxTextRole: "name"
    comboBoxIconRole: "icon"
    comboBoxIconToolTipRole: "iconToolTip"
    comboBoxIconOnlyWhenHoveredRole: "iconOnlyWhenHovered"
    comboBoxBlankSpaceForEmptyIcons: true
    comboBoxForcePressed: custom.latteView.indicator.type === type
    comboBoxPopUpAlignRight: Qt.application.layoutDirection !== Qt.RightToLeft

    property string type: ""

    Component.onCompleted: {
        reloadModel();
        updateButtonInformation();
    }

    ListModel {
        id: actionsModel
    }

    Connections{
        target: custom.latteView.indicator
        function onCustomPluginsChanged() {
            custom.reloadModel();
            custom.updateButtonInformation();
        }
    }

    Connections {
        target: custom.viewConfig
        function onIsReadyChanged() {
            if (custom.viewConfig.isReady) {
                custom.updateButtonInformation();
            }
        }
    }

    Connections{
        target: custom.button
        function onClicked() { custom.onButtonIsPressed(); }
    }

    Connections{
        target: custom.comboBox

        function onActivated() {
            if (custom.comboBox.currentIndex >= 0) {
                var item = actionsModel.get(custom.comboBox.currentIndex);
                if (item.pluginId === "add:") {
                    custom.viewConfig.indicatorUiManager.addIndicator();
                } else if (item.pluginId === "download:") {
                    custom.viewConfig.indicatorUiManager.downloadIndicator();
                } else {
                    custom.latteView.indicator.type = item.pluginId;
                }
            }

            custom.updateButtonInformation();
        }

        function onIconClicked() {
            if (custom.comboBox.currentIndex >= 0) {
                var item = actionsModel.get(custom.comboBox.currentIndex);
                var pluginId = item.pluginId;
                if (custom.latteView.indicator.customLocalPluginIds.indexOf(pluginId)>=0) {
                    custom.viewConfig.indicatorUiManager.removeIndicator(pluginId);
                    custom.comboBox.popup.close();
                }
            }
        }
    }

    Connections{
        target: custom.comboBox.popup
        function onVisibleChanged() {
            if (custom.comboBox.popup.visible) {
                custom.selectChosenType();
            }
        }
    }

    function onButtonIsPressed() {
        if (custom.type === "install:") {
            installPopup.open();
        } else {
        custom.latteView.indicator.type = custom.type;
        }
    }

    function updateButtonInformation() {
        if (custom.latteView.indicator.customPluginsCount === 0) {
            custom.buttonText = i18n("Install...");
            custom.type = "install:";
            custom.checkable = false;
        } else {
            custom.checkable = true;

            var curCustomIndex = custom.latteView.indicator.customPluginIds.indexOf(custom.latteView.indicator.customType);

            if (curCustomIndex>=0) {
                custom.buttonText = actionsModel.get(curCustomIndex).name;
                custom.type = actionsModel.get(curCustomIndex).pluginId;
            } else {
                custom.buttonText = actionsModel.get(0).name;
                custom.type = actionsModel.get(0).pluginId;
            }
        }
    }

    function reloadModel() {
        actionsModel.clear();

        if (custom.latteView.indicator.customPluginsCount > 0) {
            var pluginIds = custom.latteView.indicator.customPluginIds;
            var pluginNames = custom.latteView.indicator.customPluginNames;
            var localPluginIds = custom.latteView.indicator.customLocalPluginIds;

            for(var i=0; i<pluginIds.length; ++i) {
                var canBeRemoved = localPluginIds.indexOf(pluginIds[i])>=0;
                var iconString = canBeRemoved ? 'remove' : '';
                var iconTip = canBeRemoved ? i18n("Remove indicator") : '';
                var iconOnlyForHovered = canBeRemoved ? true : false;

                var element = {
                    pluginId: pluginIds[i],
                    name: pluginNames[i],
                    icon: iconString,
                    iconToolTip: iconTip,
                    iconOnlyWhenHovered: iconOnlyForHovered
                };

                actionsModel.append(element);
            }
        }

        appendDefaults();

        comboBox.model = actionsModel;

        if (custom.type === latteView.indicator.type) {
            selectChosenType();
        } else {
            comboBox.currentIndex = -1;
        }
    }

    function selectChosenType() {
        var found = false;

        for (var i=0; i<actionsModel.count; ++i) {
            if (actionsModel.get(i).pluginId === custom.type) {
                found = true;
                custom.comboBox.currentIndex = i;
                break;
            }
        }

        if (!found) {
            custom.comboBox.currentIndex = -1;
        }
    }

    function emptyModel() {
        actionsModel.clear();
        appendDefaults();

        comboBox.model = actionsModel;
        comboBox.currentIndex = -1;
    }

    function appendDefaults() {
        //! add
        var addElement = {
            pluginId: 'add:',
            name: i18n("Add Indicator..."),
            icon: 'document-import',
            iconToolTip: '',
            iconOnlyWhenHovered: false
        };
        actionsModel.append(addElement);

        //! download
        var downloadElement = {
            pluginId: 'download:',
            name: i18n("Get New Indicators..."),
            icon: 'get-hot-new-stuff',
            iconToolTip: '',
            iconOnlyWhenHovered: false
        };
        actionsModel.append(downloadElement);
    }

    QQC2.Popup {
        id: installPopup
        y: parent.height + 4
        x: Qt.application.layoutDirection === Qt.RightToLeft ? parent.width - width : 0
        width: Math.max(200, parent.width)
        closePolicy: QQC2.Popup.CloseOnEscape | QQC2.Popup.CloseOnPressOutside
        padding: 2

        contentItem: Item {
            implicitWidth: installColumn.implicitWidth
            implicitHeight: installColumn.implicitHeight

            ColumnLayout {
                id: installColumn
                width: parent.width
                spacing: 0

                QQC2.ItemDelegate {
                    Layout.fillWidth: true
                    text: i18n("Add Indicator...")
                    icon.name: "document-import"
                    onClicked: {
                        installPopup.close();
                        custom.viewConfig.indicatorUiManager.addIndicator();
                    }
                }

                QQC2.ItemDelegate {
                    Layout.fillWidth: true
                    text: i18n("Get New Indicators...")
                    icon.name: "get-hot-new-stuff"
                    onClicked: {
                        installPopup.close();
                        custom.viewConfig.indicatorUiManager.downloadIndicator();
                    }
                }
            }
        }
    }

}
