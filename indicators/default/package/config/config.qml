/*
    SPDX-FileCopyrightText: 2018 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.7
import QtQuick.Layouts 1.3
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.plasma.plasmoid 2.0
import org.kde.kirigami 2.0 as Kirigami

import org.kde.latte.components 1.0 as LatteComponents

ColumnLayout {
    id: root
    Layout.fillWidth: true
    Kirigami.Theme.inherit: false
    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    SystemPalette {
        id: systemPalette
        colorGroup: SystemPalette.Active
    }
    readonly property color cfgBackgroundColor: (typeof dialog !== "undefined" && dialog && dialog.bC !== undefined) ? dialog.bC : systemPalette.window
    readonly property color cfgTextColor: (typeof dialog !== "undefined" && dialog && dialog.tC !== undefined) ? dialog.tC : systemPalette.windowText
    readonly property color cfgHighlightColor: (typeof dialog !== "undefined" && dialog && dialog.hC !== undefined) ? dialog.hC : ((typeof dialog !== "undefined" && dialog && dialog.theme && dialog.theme.highlightColor !== undefined) ? dialog.theme.highlightColor : systemPalette.highlight)
    readonly property color cfgHighlightedTextColor: (typeof dialog !== "undefined" && dialog && dialog.htC !== undefined) ? dialog.htC : ((typeof dialog !== "undefined" && dialog && dialog.theme && dialog.theme.highlightedTextColor !== undefined) ? dialog.theme.highlightedTextColor : systemPalette.highlightedText)
    readonly property int safeOptionsWidth: (typeof dialog !== "undefined" && dialog) ? dialog.optionsWidth : 400

    Kirigami.Theme.backgroundColor: cfgBackgroundColor
    Kirigami.Theme.textColor: cfgTextColor
    Kirigami.Theme.highlightColor: cfgHighlightColor
    Kirigami.Theme.highlightedTextColor: cfgHighlightedTextColor

    readonly property var units: Kirigami.Units
    readonly property bool hasIndicatorConfig: (typeof indicator !== "undefined") && indicator && indicator.configuration
    readonly property bool latteTasksArePresent: (typeof indicator !== "undefined") && indicator && indicator.latteTasksArePresent
    property var indicatorConfigFallback: ({
        activeStyle: 0,
        size: 0.08,
        thickMargin: 0.0,
        lengthPadding: 0.0,
        backgroundCornerMargin: 0.0,
        glowEnabled: true,
        glowApplyTo: 1,
        glowOpacity: 0.55,
        minimizedTaskColoredDifferently: false,
        extraDotOnActive: true,
        enabledForApplets: true,
        reversed: false
    })
    readonly property var indicatorConfig: hasIndicatorConfig ? indicator.configuration : indicatorConfigFallback
    function configBool(value, fallback) {
        if (value === undefined || value === null) {
            return fallback;
        }

        return !!value;
    }

    function configInt(value, fallback) {
        if (value === undefined || value === null) {
            return fallback;
        }

        var numeric = Number(value);
        return isNaN(numeric) ? fallback : numeric;
    }

    function configReal(value, fallback) {
        if (value === undefined || value === null) {
            return fallback;
        }

        var numeric = Number(value);
        return isNaN(numeric) ? fallback : numeric;
    }

    readonly property int activeStyleValue: configInt(indicatorConfig.activeStyle, indicatorConfigFallback.activeStyle)
    readonly property real thicknessValue: configReal(indicatorConfig.size, indicatorConfigFallback.size)
    readonly property real thickMarginValue: configReal(indicatorConfig.thickMargin, indicatorConfigFallback.thickMargin)
    readonly property real lengthPaddingValue: configReal(indicatorConfig.lengthPadding, indicatorConfigFallback.lengthPadding)
    readonly property real backgroundCornerMarginValue: configReal(indicatorConfig.backgroundCornerMargin, indicatorConfigFallback.backgroundCornerMargin)
    readonly property bool glowEnabledValue: configBool(indicatorConfig.glowEnabled, indicatorConfigFallback.glowEnabled)
    readonly property int glowApplyToValue: configInt(indicatorConfig.glowApplyTo, indicatorConfigFallback.glowApplyTo)
    readonly property real glowOpacityValue: configReal(indicatorConfig.glowOpacity, indicatorConfigFallback.glowOpacity)
    readonly property bool minimizedTaskColoredDifferentlyValue: configBool(indicatorConfig.minimizedTaskColoredDifferently, indicatorConfigFallback.minimizedTaskColoredDifferently)
    readonly property bool extraDotOnActiveValue: configBool(indicatorConfig.extraDotOnActive, indicatorConfigFallback.extraDotOnActive)
    readonly property bool enabledForAppletsValue: configBool(indicatorConfig.enabledForApplets, indicatorConfigFallback.enabledForApplets)
    readonly property bool reversedValue: configBool(indicatorConfig.reversed, indicatorConfigFallback.reversed)
    readonly property int valueLabelWidth: valueTextMetrics.advanceWidth + units.smallSpacing * 2

    TextMetrics {
        id: valueTextMetrics
        text: "100 %"
    }

    LatteComponents.SubHeader {
        text: i18nc("indicator style","Style")
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 2

        property int indicatorType: root.activeStyleValue

        readonly property int buttonsCount: 2
        readonly property int buttonSize: (safeOptionsWidth - (spacing * buttonsCount-1)) / buttonsCount

        LatteComponents.Button {
            Layout.minimumWidth: parent.buttonSize
            Layout.maximumWidth: Layout.minimumWidth
            text: i18nc("line indicator","Line")
            checked: parent.indicatorType === indicatorType
            checkable: false
            tooltip: i18n("Show a line indicator for active items")

            readonly property int indicatorType: 0 /*Line*/

            onClicked: root.indicatorConfig.activeStyle = indicatorType
        }

        LatteComponents.Button {
            Layout.minimumWidth: parent.buttonSize
            Layout.maximumWidth: Layout.minimumWidth
            text: i18nc("dots indicator", "Dots")
            checked: parent.indicatorType === indicatorType
            checkable: false
            tooltip: i18n("Show a dot indicator for active items")

            readonly property int indicatorType: 1 /*Dot*/

            onClicked: root.indicatorConfig.activeStyle = indicatorType
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: units.smallSpacing

        PlasmaComponents.Label {
            text: i18n("Thickness")
            horizontalAlignment: Text.AlignLeft
        }

        LatteComponents.Slider {
            id: sizeSlider
            Layout.fillWidth: true

            value: Math.round(root.thicknessValue * 100)
            from: 3
            to: 25
            stepSize: 1
            wheelEnabled: false

            onPressedChanged: {
                if (!pressed) {
                    root.indicatorConfig.size = Number(value / 100).toFixed(2);
                }
            }
        }

        PlasmaComponents.Label {
            text: i18nc("number in percentage, e.g. 85 %","%1 %", currentValue)
            horizontalAlignment: Text.AlignRight
            Layout.minimumWidth: root.valueLabelWidth
            Layout.maximumWidth: root.valueLabelWidth

            readonly property int currentValue: sizeSlider.value
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: units.smallSpacing

        PlasmaComponents.Label {
            text: i18n("Position")
            horizontalAlignment: Text.AlignLeft
        }

        LatteComponents.Slider {
            id: thickMarginSlider
            Layout.fillWidth: true

            value: Math.round(root.thickMarginValue * 100)
            from: 0
            to: 30
            stepSize: 1
            wheelEnabled: false

            onPressedChanged: {
                if (!pressed) {
                    root.indicatorConfig.thickMargin = value / 100;
                }
            }
        }

        PlasmaComponents.Label {
            text: i18nc("number in percentage, e.g. 85 %","%1 %", currentValue)
            horizontalAlignment: Text.AlignRight
            Layout.minimumWidth: root.valueLabelWidth
            Layout.maximumWidth: root.valueLabelWidth

            readonly property int currentValue: thickMarginSlider.value
        }
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

            value: Math.round(root.lengthPaddingValue * 100)
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

            value: Math.round(root.backgroundCornerMarginValue * 100)
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

    LatteComponents.HeaderSwitch {
        id: glowEnabled
        Layout.fillWidth: true
        Layout.minimumHeight: implicitHeight
        Layout.bottomMargin: units.smallSpacing

        checked: root.glowEnabledValue
        level: 2
        text: i18n("Glow")
        tooltip: i18n("Enable/disable indicator glow")

        onPressed: {
            root.indicatorConfig.glowEnabled = !root.indicatorConfig.glowEnabled;
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 2
        enabled: root.glowEnabledValue

        property int option: root.glowApplyToValue

        readonly property int buttonsCount: 2
        readonly property int buttonSize: (safeOptionsWidth - (spacing * buttonsCount-1)) / buttonsCount

        LatteComponents.Button {
            Layout.minimumWidth: parent.buttonSize
            Layout.maximumWidth: Layout.minimumWidth
            text: i18nc("glow only to active task/applet indicators","On Active")
            checked: parent.option === option
            checkable: false
            tooltip: i18n("Add glow only to active task/applet indicator")

            readonly property int option: 1 /*OnActive*/

            onClicked: root.indicatorConfig.glowApplyTo = option
        }

        LatteComponents.Button {
            Layout.minimumWidth: parent.buttonSize
            Layout.maximumWidth: Layout.minimumWidth
            text: i18nc("glow to all task/applet indicators","All")
            checked: parent.option === option
            checkable: false
            tooltip: i18n("Add glow to all task/applet indicators")

            readonly property int option: 2 /*All*/

            onClicked: root.indicatorConfig.glowApplyTo = option
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 2

        enabled: root.glowEnabledValue

        PlasmaComponents.Label {
            Layout.minimumWidth: implicitWidth
            horizontalAlignment: Text.AlignLeft
            Layout.rightMargin: units.smallSpacing
            text: i18n("Opacity")
        }

        LatteComponents.Slider {
            id: glowOpacitySlider
            Layout.fillWidth: true

            leftPadding: 0
            value: root.glowOpacityValue * 100
            from: 0
            to: 100
            stepSize: 5
            wheelEnabled: false

            onPressedChanged: {
                if (!pressed) {
                    root.indicatorConfig.glowOpacity = value/100;
                }
            }
        }

        PlasmaComponents.Label {
            text: i18nc("number in percentage, e.g. 85 %","%1 %", glowOpacitySlider.value)
            horizontalAlignment: Text.AlignRight
            Layout.minimumWidth: root.valueLabelWidth
            Layout.maximumWidth: root.valueLabelWidth
        }
    }

    ColumnLayout {
        spacing: 0
        visible: root.latteTasksArePresent

        LatteComponents.SubHeader {
            enabled: root.glowApplyToValue!==0/*None*/
            text: i18n("Tasks")
        }

        LatteComponents.CheckBoxesColumn {
            LatteComponents.CheckBox {
                Layout.maximumWidth: safeOptionsWidth
                text: i18n("Different color for minimized windows")
                value: root.minimizedTaskColoredDifferentlyValue

                onClicked: {
                    root.indicatorConfig.minimizedTaskColoredDifferently = !root.indicatorConfig.minimizedTaskColoredDifferently;
                }
            }

            LatteComponents.CheckBox {
                Layout.maximumWidth: safeOptionsWidth
                text: i18n("Show an extra dot for grouped windows when active")
                tooltip: i18n("Grouped windows show both a line and a dot when one of them is active and the Line Active Indicator is enabled")
                enabled: root.activeStyleValue === 0 /*Line*/
                value: root.extraDotOnActiveValue

                onClicked: {
                    root.indicatorConfig.extraDotOnActive = !root.indicatorConfig.extraDotOnActive;
                }
            }
        }
    }

    LatteComponents.SubHeader {
        enabled: root.glowApplyToValue!==0/*None*/
        text: i18n("Options")
    }

    LatteComponents.CheckBox {
        Layout.maximumWidth: safeOptionsWidth
        text: i18n("Show indicators for applets")
        tooltip: i18n("Indicators are shown for applets")
        value: root.enabledForAppletsValue

        onClicked: {
            root.indicatorConfig.enabledForApplets = !root.indicatorConfig.enabledForApplets;
        }
    }

    LatteComponents.CheckBox {
        Layout.maximumWidth: safeOptionsWidth
        text: i18n("Reverse indicator style")
        value: root.reversedValue

        onClicked: {
            root.indicatorConfig.reversed = !root.indicatorConfig.reversed;
        }
    }
}
