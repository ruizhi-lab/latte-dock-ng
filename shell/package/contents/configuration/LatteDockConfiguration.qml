/*
    SPDX-FileCopyrightText: 2016 Smith AR <audoban@openmailbox.org>
    SPDX-FileCopyrightText: 2016 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.7
import QtQuick.Controls 2.12 as QtQuickControls212
import QtQuick.Layouts 1.3
import QtQuick.Window 2.2

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.ksvg 1.0 as KSvg
import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras

import org.kde.kquickcontrolsaddons 2.0 as KQuickControlAddons

import org.kde.kirigami 2.0 as Kirigami

import org.kde.latte.core 0.2 as LatteCore
import org.kde.latte.components 1.0 as LatteComponents

import "pages" as Pages
import "../controls" as LatteExtraControls

Loader {
    active: plasmoid && plasmoid.configuration && latteView

    sourceComponent: FocusScope {
        id: dialog
        Kirigami.Theme.inherit: false
        Kirigami.Theme.colorSet: Kirigami.Theme.Window
        width: appliedWidth
        height: appliedHeight
        readonly property var theme: Kirigami.Theme

        // Plasma 5 provided 'units' as a global context property; Plasma 6
        // removed it — expose Kirigami.Units under the same name so all
        // descendant items continue to work without individual changes.
        readonly property var units: Kirigami.Units

        readonly property bool basicLevel: !advancedLevel
        readonly property bool advancedLevel: universalSettings.inAdvancedModeForEditSettings

        readonly property bool inConfigureAppletsMode: universalSettings.inConfigureAppletsMode || !LatteCore.WindowSystem.compositingActive

        readonly property bool kirigamiLibraryIsFound: true

        //! max size based on screen resolution
        //!    TODO: if we can access availableScreenGeometry.height this can be improved, currently
        //!    we use 100px. or 50px. in order to give space for other views to be shown and to also
        //!    have some space around the settings window
        property int maxHeight: plasmoid.formFactor === PlasmaCore.Types.Horizontal ?
                                    viewConfig.availableScreenGeometry.height - canvasHeadThickness - units.largeSpacing :
                                    viewConfig.availableScreenGeometry.height - 2 * units.largeSpacing

        property int maxWidth: 0.6 * latteView.screenGeometry.width

        property int canvasThickness: plasmoid.formFactor === PlasmaCore.Types.Vertical ? latteView.positioner.canvasGeometry.width : latteView.positioner.canvasGeometry.height
        property int canvasHeadThickness: {
            return Math.max(0, canvasThickness - latteView.maxNormalThickness)
        }

        //! propose size based on font size
        property int proposedWidth: 0.82 * proposedHeight + units.smallSpacing * 2
        property int proposedHeight: 36 * Kirigami.Units.gridUnit

        //! chosen size to be applied, if the user has set or not a different scale for the settings window
        property int chosenWidth: userScaleWidth !== 1 ? userScaleWidth * proposedWidth : proposedWidth
        property int chosenHeight: userScaleHeight !== 1 ? userScaleHeight * heightLevel * proposedHeight : heightLevel * proposedHeight

        readonly property int optionsWidth: appliedWidth - units.smallSpacing * 10

        //! user set scales based on its preference, e.g. 96% of the proposed size
        property real userScaleWidth: 1
        property real userScaleHeight: 1

        readonly property real heightLevel: (dialog.advancedLevel ? 100 : 1) //in order to use all available space

        onHeightChanged: viewConfig.syncGeometry();

        //! applied size in order to not be out of boundaries
        //! width can be between 200px - maxWidth
        //! height can be between 400px - maxHeight
        property int appliedWidth: Math.min(maxWidth, Math.max(200, chosenWidth))
        property int appliedHeight: universalSettings.inAdvancedModeForEditSettings ? maxHeight : Math.min(maxHeight, Math.max(400, chosenHeight))

        Layout.minimumWidth: width
        Layout.minimumHeight: height
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        property bool panelIsVertical: plasmoid.formFactor === PlasmaCore.Types.Vertical
        property int subGroupSpacing: units.largeSpacing + units.smallSpacing * 1.5

        property color bC: theme.backgroundColor
        property color tC: theme.textColor
        property color transparentBackgroundColor: Qt.rgba(bC.r, bC.g, bC.b, 0.7)
        property color borderColor: Qt.rgba(tC.r, tC.g, tC.b, 0.12)

        readonly property Item currentPage: pagesStackView.currentItem

        onAdvancedLevelChanged: {
            //! switch to appearancePage when effectsPage becomes hidden because
            //! advancedLevel was disabled by the user
            if (!advancedLevel && pagesStackView.currentItem === effectsPage) {
                tabBar.currentIndex = 1;
            }
        }

        Component.onCompleted: {
            updateScales();
            viewConfig.updateEffects();
        }

        Connections {
            target: latteView.positioner
            function onCurrentScreenNameChanged() { dialog.updateScales(); }
        }

        function updateScales() {
            userScaleWidth = universalSettings.screenWidthScale(latteView.positioner.currentScreenName);
            userScaleHeight = universalSettings.screenHeightScale(latteView.positioner.currentScreenName);
        }

        Rectangle {
            anchors.fill: parent
            color: dialog.bC
            border.color: dialog.borderColor
            border.width: 1
            radius: Math.max(4, units.smallSpacing)
        }

        LatteExtraControls.DragCorner {
            id: dragCorner
        }

        Connections {
            target: viewConfig
            function onEnabledBordersChanged() {
                viewConfig.updateEffects();
            }
        }
        PlasmaComponents.Label{
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            text: dialog.advancedLevel ?
                      i18nc("view settings width scale","Width %1%",userScaleWidth * 100) :
                      i18nc("view settings width scale","Width %1% / Height %2%", userScaleWidth * 100, userScaleHeight * 100)
            visible: dragCorner.isActive
        }

        ColumnLayout {
            id: content
            Kirigami.Theme.inherit: false
            Kirigami.Theme.colorSet: Kirigami.Theme.Window

            Layout.minimumWidth: width
            Layout.minimumHeight: calculatedHeight
            Layout.preferredWidth: width
            Layout.preferredHeight: calculatedHeight
            width: (dialog.appliedWidth - units.smallSpacing * 2)

            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            spacing: units.smallSpacing

            property int calculatedHeight: header.height + headerSpacer.height+ tabBar.height + pagesBackground.height + actionButtons.height + spacing * 3

            Keys.onPressed: {
                if (event.key === Qt.Key_Escape) {
                    viewConfig.hideConfigWindow();
                }
            }

            Component.onCompleted: forceActiveFocus();

            RowLayout {
                id: header
                Layout.fillWidth: true

                spacing: 0

                Item {
                    id: trademark
                    Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                    Layout.fillWidth: false
                    Layout.topMargin: units.smallSpacing
                    Layout.preferredWidth: width
                    Layout.preferredHeight: height

                    width: latteTrademark.width + units.smallSpacing
                    height: trademarkHeight

                    readonly property int trademarkHeight: 48

                    KSvg.SvgItem{
                        id: latteTrademark
                        width: Qt.application.layoutDirection !== Qt.RightToLeft ? Math.ceil(1.70 * height) : height
                        height: trademark.height

                        svg: KSvg.Svg{
                            imagePath: Qt.application.layoutDirection !== Qt.RightToLeft ? universalSettings.trademarkPath() : universalSettings.trademarkIconPath()
                        }
                    }
                }

                Item{
                    id: headerSpacer
                    Layout.minimumHeight: advancedSettings.height + 2*units.smallSpacing
                }

                ColumnLayout {
                    PlasmaComponents3.ToolButton {
                        id: pinButton
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                        Layout.bottomMargin: units.smallSpacing * 1.5
                        Layout.topMargin: units.smallSpacing * 3
                        Layout.rightMargin: units.smallSpacing * 2
                        icon.name: "window-pin"
                        checkable: true

                        property bool inStartup: true

                        onClicked: {
                            plasmoid.configuration.configurationSticker = checked
                            viewConfig.setSticker(checked)
                        }

                        Component.onCompleted: {
                            checked = plasmoid.configuration.configurationSticker
                            viewConfig.setSticker(plasmoid.configuration.configurationSticker)
                        }
                    }

                    RowLayout {
                        id: advancedSettings
                        Layout.fillWidth: true
                        Layout.rightMargin: units.smallSpacing * 2
                        Layout.alignment: Qt.AlignRight | Qt.AlignTop

                        PlasmaComponents.Label {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignRight
                        }

                        PlasmaComponents.Label {
                            id: advancedLbl
                            Layout.alignment: Qt.AlignRight
                            //  opacity: dialog.basicLevel ? basicOpacity : 1

                            //! TODO: the term here is not accurate because the expert settings mode
                            //! is used currently. In the future this term will be rethought if
                            //! it must remain or be changed
                            text: i18nc("advanced settings", "Advanced")

                            readonly property real textColorBrightness: colorBrightness(dialog.tC)
                            readonly property real basicOpacity: textColorBrightness > 127 ? 0.7 : 0.3

                            color: {
                                if (dialog.basicLevel) {
                                    return textColorBrightness > 127 ? Qt.darker(dialog.tC, 1.4) : Qt.lighter(dialog.tC, 2.8);
                                }

                                return dialog.tC;
                            }

                            function colorBrightness(color) {
                                return colorBrightnessFromRGB(color.r * 255, color.g * 255, color.b * 255);
                            }

                            // formula for brightness according to:
                            // https://www.w3.org/TR/AERT/#color-contrast
                            function colorBrightnessFromRGB(r, g, b) {
                                return (r * 299 + g * 587 + b * 114) / 1000
                            }

                            MouseArea {
                                id: advancedMouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: {
                                    advancedSwitch.checked = !advancedSwitch.checked;
                                }
                            }
                        }

                        LatteComponents.Switch {
                            id: advancedSwitch
                            checked: universalSettings.inAdvancedModeForEditSettings && viewConfig.isReady

                            onCheckedChanged: {
                                if (viewConfig.isReady) {
                                    universalSettings.inAdvancedModeForEditSettings = checked;
                                }
                            }
                        }
                    }
                }
            }

            PlasmaComponents.TabBar {
                id: tabBar
                Layout.fillWidth: true
                Layout.maximumWidth: (dialog.appliedWidth - units.smallSpacing * 2)

                readonly property int visibleStaticPages: dialog.advancedLevel ? 3 : 2

                PlasmaComponents.TabButton {
                    id: behaviorTabBtn
                    text: i18n("Behavior")
                    onCheckedChanged: {
                        var stack = pagesStackView;
                        if (!checked || !stack) {
                            return;
                        }

                        if (stack.currentItem && stack.currentItem !== behaviorPage) {
                            stack.forwardSliding = true;
                            stack.replace(stack.currentItem, behaviorPage);
                        } else if (!stack.currentItem) {
                            stack.push(behaviorPage);
                        }
                    }

                    Connections {
                        target: viewConfig
                        function onIsReadyChanged() {
                            if (viewConfig.isReady) {
                                tabBar.currentIndex = 0;
                            }
                        }
                    }
                }

                PlasmaComponents.TabButton {
                    id: appearanceTabBtn
                    text: i18n("Appearance")
                    onCheckedChanged: {
                        var stack = pagesStackView;
                        if (!checked || !stack) {
                            return;
                        }

                        if (stack.currentItem && stack.currentItem !== appearancePage) {
                            stack.forwardSliding = (stack.currentItem.pageIndex > 1);
                            stack.replace(stack.currentItem, appearancePage);
                        } else if (!stack.currentItem) {
                            stack.push(appearancePage);
                        }
                    }
                }
                PlasmaComponents.TabButton {
                    id: effectsTabBtn
                    text: i18n("Effects")
                    visible: dialog.advancedLevel

                    onCheckedChanged: {
                        var stack = pagesStackView;
                        if (!checked || !stack) {
                            return;
                        }

                        if (stack.currentItem && stack.currentItem !== effectsPage) {
                            stack.forwardSliding = (stack.currentItem.pageIndex > 2);
                            stack.replace(stack.currentItem, effectsPage);
                        } else if (!stack.currentItem) {
                            stack.push(effectsPage);
                        }
                    }
                }

                Repeater {
                    id: tasksTabButtonRepeater
                    model: latteView.extendedInterface.latteTasksModel

                    PlasmaComponents.TabButton {
                        text: index >= 1 ? i18nc("tasks header and index","Tasks <%1>", index+1) : i18n("Tasks")
                        onCheckedChanged: {
                            var stack = pagesStackView;
                            var targetItem = tasksRepeater.itemAt(index);
                            if (!checked || !stack || !targetItem) {
                                return;
                            }

                            if (stack.currentItem && stack.currentItem !== targetItem) {
                                stack.forwardSliding = (stack.currentItem.pageIndex > (tabBar.visibleStaticPages + index));
                                stack.replace(stack.currentItem, targetItem);
                            } else if (!stack.currentItem) {
                                stack.push(targetItem);
                            }
                        }
                    }
                }
            }

            Item {
                id: pagesBackground
                Layout.fillWidth: true
                Layout.fillHeight: false
                Layout.minimumWidth: dialog.appliedWidth - units.smallSpacing * 4
                Layout.minimumHeight: height
                Layout.maximumHeight: height

                width: dialog.appliedWidth - units.smallSpacing * 3
                height: availableFreeHeight + units.smallSpacing * 4

                //fix the height binding loop when showing the configuration window
                property int availableFreeHeight: dialog.appliedHeight - header.height - headerSpacer.height - tabBar.height - actionButtons.height - 2 * units.smallSpacing

                // Header
                Rectangle {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.topMargin: -units.smallSpacing + 2
                    anchors.leftMargin: -2*units.smallSpacing
                    anchors.rightMargin: -2*units.smallSpacing

                    height: parent.height // dialog.height - (header.height + tabBar.height + units.smallSpacing * 1.5) + 2
                    color: dialog.bC
                    border.width: 1
                    border.color: dialog.borderColor
                }

                PlasmaComponents.ScrollView {
                    id: scrollArea

                    anchors.fill: parent
                    QtQuickControls212.ScrollBar.vertical.policy: QtQuickControls212.ScrollBar.AsNeeded
                    QtQuickControls212.ScrollBar.horizontal.policy: QtQuickControls212.ScrollBar.AlwaysOff

                    QtQuickControls212.StackView {
                        id: pagesStackView
                        width: currentItem ? currentItem.width : pagesBackground.width
                        height: currentItem ? currentItem.height : pagesBackground.height

                        property bool forwardSliding: true

                        replaceEnter: Transition {
                            ParallelAnimation {
                                PropertyAnimation {
                                    property: "x"
                                    from: pagesStackView.forwardSliding ? -pagesBackground.width : pagesBackground.width
                                    to: 0
                                    duration: 350
                                }

                                PropertyAnimation {
                                    property: "opacity"
                                    from: 0
                                    to: 1
                                    duration: 350
                                }
                            }
                        }

                        replaceExit: Transition {
                            ParallelAnimation {
                                PropertyAnimation {
                                    property: "x"
                                    from: 0
                                    to: pagesStackView.forwardSliding ? pagesBackground.width : -pagesBackground.width
                                    duration: 350
                                }

                                PropertyAnimation {
                                    property: "opacity"
                                    from: 1
                                    to: 0
                                    duration: 350
                                }
                            }
                        }

                        onDepthChanged:  {
                            if (depth === 0) {
                                pagesStackView.forwardSliding = true;
                                push(behaviorPage);
                            }
                        }
                    }
                }

                Item {
                    id:hiddenPages
                    anchors.fill: parent
                    visible: false

                    Pages.BehaviorConfig {
                        id: behaviorPage
                        readonly property int pageIndex:0

                        Component.onCompleted: {
                            pagesStackView.push(behaviorPage);
                        }
                    }

                    Pages.AppearanceConfig {
                        id: appearancePage
                        readonly property int pageIndex:1
                    }

                    Pages.EffectsConfig {
                        id: effectsPage
                        readonly property int pageIndex:2
                    }

                    Repeater {
                        id: tasksRepeater
                        model: plasmoid && plasmoid.configuration && latteView ? latteView.extendedInterface.latteTasksModel : 0

                        Pages.TasksConfig {
                            readonly property int pageIndex: tabBar.visibleStaticPages+index
                        }
                    }
                }
            }

            RowLayout {
                id: actionButtons
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                spacing: units.largeSpacing

                LatteComponents.ComboBoxButton {
                    id: actionsComboBtn
                    Layout.fillWidth: true
                    implicitWidth: removeView.implicitWidth
                    implicitHeight: removeView.implicitHeight

                    buttonEnabled: true
                    buttonIsTriggeringMenu: true
                    buttonText: i18n("Add...")
                    buttonIconSource: "list-add"
                    buttonToolTip: i18n("Add new docks from various templates")

                    comboBoxEnabled: true
                    comboBoxBlankSpaceForEmptyIcons: true
                    comboBoxPopUpAlignRight: Qt.application.layoutDirection === Qt.RightToLeft
                    comboBoxEnabledRole: "enabled"
                    comboBoxTextRole: "name"
                    comboBoxIconRole: "icon"
                    comboBoxIsSeparatorRole: "isSeparator"
                    comboBoxMinimumPopUpWidth: actionsModel.count > 1 ? dialog.width / 2 : 150

                    property var centralLayoutsNames: []

                    Component.onCompleted: {
                        comboBox.model = actionsModel;
                    }

                    ListModel {
                        id: actionsModel
                    }

                    Connections{
                        target: actionsComboBtn.comboBox

                        Component.onCompleted:actionsComboBtn.updateModel();

                        function onActivated() {
                            var item = actionsModel.get(index);

                            if (item && item.actionId === "add:") {
                                latteView.newView(item.templateId);
                            } else if (item && item.actionId === "duplicate:") {
                                latteView.duplicateView();
                            }

                            actionsComboBtn.comboBox.currentIndex = -1;
                        }

                        function onEnabledChanged() {
                            if (enabled) {
                                actionsComboBtn.updateModel();
                            } else {
                                actionsComboBtn.emptyModel();
                            }
                        }
                    }

                    Connections{
                        target: viewConfig
                        function onIsReadyChanged() {
                            if (viewConfig.isReady) {
                                actionsComboBtn.updateModel();
                            }
                        }
                    }

                    Connections{
                        target: latteView
                        function onTypeChanged() { actionsComboBtn.updateDuplicateText(); }
                    }

                    Connections {
                        target: layoutsManager
                        function onViewTemplatesChanged() { actionsComboBtn.updateModel(); }
                    }

                    function updateModel() {
                        actionsModel.clear();

                        var duplicate = {actionId: 'duplicate:', enabled: true, name: '', icon: 'edit-copy'};
                        actionsModel.append(duplicate);
                        updateDuplicateText();

                        var separator = {isSeparator: true};
                        actionsModel.append(separator);

                        var viewTemplateIds = layoutsManager.viewTemplateIds();
                        var viewTemplateNames = layoutsManager.viewTemplateNames();

                        for(var i=viewTemplateIds.length-1; i>=0; --i) {
                            //! add view templates on reverse
                            var viewtemplate = {
                                actionId: 'add:',
                                enabled: true,
                                templateId: viewTemplateIds[i],
                                name: viewTemplateNames[i],
                                icon: 'list-add'
                            };
                            actionsModel.append(viewtemplate);
                        }

                        actionsComboBtn.comboBox.currentIndex = -1;
                    }

                    function emptyModel() {
                        actionsModel.clear();
                        actionsComboBtn.comboBox.currentIndex = -1;
                    }

                    function updateDuplicateText() {
                        for (var i=0; i<actionsModel.count; ++i) {
                            var item = actionsModel.get(i);
                            if (item.actionId === "duplicate:") {
                                item.name = i18n("Duplicate Dock");
                                break;
                            }
                        }
                    }

                }

                LatteComponents.Button {
                    id: removeView
                    Layout.fillWidth: true
                    enabled: dialog.advancedLevel
                    text: i18n("Remove")
                    tooltip: i18n("Remove current dock")
                    icon.name: "delete"
                    opacity: enabled ? 1 : 0

                    onClicked: latteView.removeView()
                }

                LatteComponents.Button {
                    id: closeButton
                    Layout.fillWidth: true

                    text: i18n("Close")
                    tooltip: i18n("Close settings window")
                    icon.name: "dialog-close"

                    onClicked: viewConfig.hideConfigWindow();
                }
            }
        }

        /*PlasmaExtras.PlasmoidHeading {
            id: plasmoidFooter
            location: PlasmaExtras.PlasmoidHeading.Location.Footer
            anchors {
                bottom: parent.bottom
                left: parent.left
                right: parent.right
            }
            height: actionButtons.height + units.smallSpacing * 2.5
            // So that it doesn't appear over the content view, which results in
            // the footer controls being inaccessible
            z: -9999
        }*/
    }
}
