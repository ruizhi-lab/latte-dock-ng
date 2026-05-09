/*
    SPDX-FileCopyrightText: 2018 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.7
import QtQuick.Layouts 1.3

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.kirigami 2.0 as Kirigami

import org.kde.latte.components 1.0 as LatteComponents

ColumnLayout {
    id: root
    Layout.fillWidth: true
    Kirigami.Theme.inherit: true
    Kirigami.Theme.colorSet: Kirigami.Theme.Window

    readonly property var units: Kirigami.Units
    readonly property bool hasIndicatorConfig: (typeof indicator !== "undefined") && indicator && indicator.configuration
    property var indicatorConfigFallback: ({
        lengthPadding: 0.08,
        backgroundCornerMargin: 1.00,
        clickedAnimationEnabled: true
    })
    readonly property var indicatorConfig: hasIndicatorConfig ? indicator.configuration : indicatorConfigFallback
    readonly property int valueLabelWidth: 4 * units.gridUnit

    function configBool(value, fallback) {
        if (value === undefined || value === null) {
            return fallback;
        }

        return !!value;
    }

    function configReal(value, fallback) {
        if (value === undefined || value === null) {
            return fallback;
        }

        var numeric = Number(value);
        return isNaN(numeric) ? fallback : numeric;
    }

    LatteComponents.SubHeader {
        text: i18n("Style")
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: units.smallSpacing

        PlasmaComponents.Label {
            text: i18n("Padding")
            horizontalAlignment: Text.AlignLeft
        }

        LatteComponents.Slider {
            id: lengthIntMarginSlider
            Layout.fillWidth: true

            value: Math.round(root.configReal(root.indicatorConfig.lengthPadding, root.indicatorConfigFallback.lengthPadding) * 100)
            from: 0
            to: maxMargin
            stepSize: 1
            wheelEnabled: false

            readonly property int maxMargin: 80

            onPressedChanged: {
                if (!pressed) {
                    root.indicatorConfig.lengthPadding = value / 100;
                }
            }
        }

        PlasmaComponents.Label {
            text: i18nc("number in percentage, e.g. 85 %","%1 %", currentValue)
            horizontalAlignment: Text.AlignRight
            Layout.minimumWidth: root.valueLabelWidth
            Layout.maximumWidth: root.valueLabelWidth

            readonly property int currentValue: lengthIntMarginSlider.value
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: units.smallSpacing

        PlasmaComponents.Label {
            text: i18n("Corner Margin")
            horizontalAlignment: Text.AlignLeft
        }

        LatteComponents.Slider {
            id: backgroundCornerMarginSlider
            Layout.fillWidth: true

            value: Math.round(root.configReal(root.indicatorConfig.backgroundCornerMargin, root.indicatorConfigFallback.backgroundCornerMargin) * 100)
            from: 0
            to: 100
            stepSize: 1
            wheelEnabled: false

            onPressedChanged: {
                if (!pressed) {
                    root.indicatorConfig.backgroundCornerMargin = value / 100;
                }
            }
        }

        PlasmaComponents.Label {
            text: i18nc("number in percentage, e.g. 85 %","%1 %", currentValue)
            horizontalAlignment: Text.AlignRight
            Layout.minimumWidth: root.valueLabelWidth
            Layout.maximumWidth: root.valueLabelWidth

            readonly property int currentValue: backgroundCornerMarginSlider.value
        }
    }

    LatteComponents.SubHeader {
        text: i18n("Options")
    }

    LatteComponents.CheckBoxesColumn {
        Layout.topMargin: 1.5 * units.smallSpacing

       /* LatteComponents.CheckBox {
            Layout.maximumWidth: dialog.optionsWidth
            text: i18n("Reverse indicator style")
            value: indicator.configuration.reversed

            onClicked: {
                indicator.configuration.reversed = !indicator.configuration.reversed;
            }
        }*/

        LatteComponents.CheckBox {
            Layout.maximumWidth: dialog.optionsWidth
            text: i18n("Growing circle animation when clicked")
            value: root.configBool(root.indicatorConfig.clickedAnimationEnabled, root.indicatorConfigFallback.clickedAnimationEnabled)

            onClicked: {
                root.indicatorConfig.clickedAnimationEnabled = !root.configBool(root.indicatorConfig.clickedAnimationEnabled, root.indicatorConfigFallback.clickedAnimationEnabled);
            }
        }

      /*  LatteComponents.CheckBox {
            Layout.maximumWidth: dialog.optionsWidth
            text: i18n("Show indicators for applets")
            tooltip: i18n("Indicators are shown for applets")
            value: indicator.configuration.enabledForApplets

            onClicked: {
                indicator.configuration.enabledForApplets = !indicator.configuration.enabledForApplets;
            }
        }*/
    }
}
