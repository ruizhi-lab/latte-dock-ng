/*
    SPDX-FileCopyrightText: 2011 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2021 Michail Vourlakos <mvourlakos@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.7
import QtQuick.Controls 2.5 as QQC2

import org.kde.plasma.components 3.0 as PC3
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.ksvg 1.0 as KSvg
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kwindowsystem 1.0
import org.kde.kirigami 2.19 as Kirigami

import QtQuick.Window 2.1
import QtQuick.Layouts 1.1

import org.kde.plasma.private.shell 2.0 as PlasmaShell

PC3.Page {
    id: main
    // FrameSvg "dialogs/background" follows the Plasma theme. If the plasma
    // theme is dark, use Complementary for light text on dark background.
    // If light, use the default (Window) for dark text on light background.
    Kirigami.Theme.colorSet: (themeExtended && themeExtended.isDarkTheme)
        ? Kirigami.Theme.Complementary
        : Kirigami.Theme.Window
    Kirigami.Theme.inherit: false

    // Timer constants
    readonly property int runningCountRefreshInterval: 100
    readonly property int setModelDelay: 20
    readonly property int pendingUninstallDelay: 200

    width: Math.max(heading.paintedWidth, Kirigami.Units.iconSizes.enormous * 3 + Kirigami.Units.smallSpacing * 4 + Kirigami.Units.gridUnit * 2)
  //  height: 800//Screen.height
    opacity: draggingWidget ? 0.3 : 1
    visible: viewConfig.visible

    //property QtObject containment

    property PlasmaCore.Dialog sidePanel

    //external drop events can cause a raise event causing us to lose focus and
    //therefore get deleted whilst we are still in a drag exec()
    //this is a clue to the owning dialog that hideOnWindowDeactivate should be deleted
    //See https://bugs.kde.org/show_bug.cgi?id=332733
    property bool getNewWidgetsDialogActive: false
    property bool preventWindowHide: draggingWidget || getNewWidgetsDialogActive
                                  || categoriesDialog.status !== PlasmaExtras.Menu.Closed
                                  || getWidgetsDialog.status !== PlasmaExtras.Menu.Closed

    property bool outputOnly: draggingWidget

    property Item categoryButton

    property bool draggingWidget: false

    property QtObject widgetExplorer: widgetExplorerLoader.active ? widgetExplorerLoader.item : null
    property int runningCountRevision: 0

    signal closed();

    function forceClose() {
        getNewWidgetsWindowHideRestoreTimer.stop()
        getNewWidgetsDialogActive = false
        viewConfig.hideConfigWindow()
    }

    onClosed: {
        if (main.preventWindowHide) {
            return;
        }

        viewConfig.hideConfigWindow();
    }

    onVisibleChanged: {
        if (!visible) {
            KWindowSystem.showingDesktop = false;
        }
    }

    onWidgetExplorerChanged: {
        if (widgetExplorer) {
            setModelTimer.start();
            scheduleRunningCountRefresh();
            // Also bind immediately in case the timer already fired with a null model
            if (widgetExplorer.widgetsModel) {
                list.model = widgetExplorer.widgetsModel;
            }
        }
    }

    Component.onDestruction: {
        if (pendingUninstallTimer.running && widgetExplorer && widgetExplorer.widgetsModel) {
            // we're not being destroyed so at least reset the filters
            widgetExplorer.widgetsModel.filterQuery = ""
            widgetExplorer.widgetsModel.filterType = ""
            widgetExplorer.widgetsModel.searchTerm = ""
        }
    }

    function addCurrentApplet() {
        var pluginName = list.currentItem ? list.currentItem.pluginName : ""
        if (pluginName && widgetExplorer && typeof widgetExplorer.addApplet === "function") {
            widgetExplorer.addApplet(pluginName);
            scheduleRunningCountRefresh();
        }
    }

    function pluginNameForApplet(applet) {
        if (!applet) {
            return "";
        }

        var directName = (applet.pluginName !== undefined && applet.pluginName !== null) ? String(applet.pluginName) : "";
        if (directName !== "") {
            return directName;
        }

        if (applet.metaData !== undefined && applet.metaData && applet.metaData.pluginId !== undefined) {
            var metadataName = (applet.metaData.pluginId !== null) ? String(applet.metaData.pluginId) : "";
            if (metadataName !== "") {
                return metadataName;
            }
        }

        if (applet.applet !== undefined && applet.applet && applet.applet !== applet) {
            var backendName = pluginNameForApplet(applet.applet);
            if (backendName !== "") {
                return backendName;
            }
        }

        if (applet.plasmoid !== undefined && applet.plasmoid && applet.plasmoid !== applet) {
            var plasmoidName = pluginNameForApplet(aplet.plasmoid);
            if (plasmoidName !== "") {
                return plasmoidName;
            }
        }

        return "";
    }

    function runningInstancesFor(pluginName) {
        // Plasma's WidgetExplorer model can briefly over-count right after
        // addApplet(). Use the live containment state for Latte's badge.
        runningCountRevision;

        if (!pluginName || !containmentFromView || !containmentFromView.applets) {
            return 0;
        }

        var count = 0;
        for (var i = 0; i < containmentFromView.applets.length; ++i) {
            if (pluginNameForApplet(containmentFromView.applets[i]) === pluginName) {
                count++;
            }
        }

        return count;
    }

    function shouldOpenExternalGetNewWidgetsDialog(actionModel) {
        var label = "";

        if (actionModel) {
            label = String(actionModel.display || actionModel.text || actionModel.name || "");
        }

        return label.indexOf("Get New") !== -1
            || label.indexOf("Add New") !== -1
            || label.indexOf("Download New") !== -1
            || label.indexOf("添加新") !== -1
            || label.indexOf("获取新") !== -1
            || label.indexOf("下载新") !== -1;
    }

    function scheduleRunningCountRefresh() {
        runningCountRevision++;
        runningCountRefreshTimer.remainingRuns = 20;
        runningCountRefreshTimer.restart();
    }

    Timer {
        id: runningCountRefreshTimer
        interval: main.runningCountRefreshInterval
        repeat: true

        property int remainingRuns: 0

        onTriggered: {
            main.runningCountRevision++;
            remainingRuns--;
            if (remainingRuns <= 0) {
                stop();
            }
        }
    }

    Connections {
        target: containmentFromView
        ignoreUnknownSignals: true

        function onAppletsChanged() {
            main.scheduleRunningCountRefresh();
        }

        function onAppletAdded() {
            main.scheduleRunningCountRefresh();
        }

        function onAppletRemoved() {
            main.scheduleRunningCountRefresh();
        }
    }

    QQC2.Action {
        shortcut: "Escape"
        onTriggered: {
            if (searchInput.length > 0) {
                searchInput.text = ""
            } else {
                main.closed();
            }
        }
    }

    QQC2.Action {
        shortcut: "Enter"
        onTriggered: addCurrentApplet()
    }
    QQC2.Action {
        shortcut: "Return"
        onTriggered: addCurrentApplet()
    }

    Timer {
        id: getNewWidgetsWindowHideRestoreTimer
        interval: 30000
        repeat: false
        onTriggered: main.getNewWidgetsDialogActive = false
    }

    KSvg.FrameSvgItem{
        id: backgroundFrameSvgItem
        anchors.top: parent.top
        anchors.topMargin: -headerMargin
        width: parent.width
        height: parent.height + headerMargin
        imagePath: "dialogs/background"
        enabledBorders: viewConfig.enabledBorders

        // The FrameSvg "dialogs/background" has an intrinsic top margin that the
        // header sits inside. This offset compensates so the background extends
        // above the header to align with the view's top edge.
        readonly property int frameSvgTopOverhang: 50
        readonly property int headerMargin: header.height + frameSvgTopOverhang

        onEnabledBordersChanged: viewConfig.updateEffects()
        Component.onCompleted: viewConfig.updateEffects()
    }

    Loader {
        id: widgetExplorerLoader
        active: main.visible
        sourceComponent: PlasmaShell.WidgetExplorer {
            //id:widgetExplorer
             //view: desktop
            containment: containmentFromView
            onShouldClose: main.closed();
        }
    }

    PlasmaExtras.ModelContextMenu {
        id: getWidgetsDialog
        visualParent: getWidgetsButton
        // model set on first invocation

        onClicked: function(model) {
            if (main.shouldOpenExternalGetNewWidgetsDialog(model)) {
                main.getNewWidgetsDialogActive = true
                getNewWidgetsWindowHideRestoreTimer.restart()
                viewConfig.openGetNewWidgetsDialog()
                return
            }

            model.trigger()
        }
    }

    PlasmaExtras.ModelContextMenu {
        id: categoriesDialog
        visualParent: categoryButton
        // model set on first invocation

        onClicked: {
            list.contentX = 0
            list.contentY = 0
            categoryButton.text = (model.filterData ? model.display : i18ndc("plasma_shell_org.kde.plasma.desktop", "@action:button like listbox, switches category to all widgets", "All Widgets"))
            if (widgetExplorer && widgetExplorer.widgetsModel) {
                widgetExplorer.widgetsModel.filterQuery = model.filterData
                widgetExplorer.widgetsModel.filterType = model.filterType
            }
        }
    }

    /*
    PlasmaCore.Dialog {
        id: tooltipDialog
        property Item appletDelegate
        location: PlasmaCore.Types.RightEdge //actually we want this to be the opposite location of the explorer itself
        type: PlasmaCore.Dialog.Tooltip
        flags:Qt.Window|Qt.WindowStaysOnTopHint
        onAppletDelegateChanged: {
            if (!appletDelegate) {
                toolTipHideTimer.restart()
                toolTipShowTimer.running = false
            } else if (tooltipDialog.visible) {
                tooltipDialog.visualParent = appletDelegate
            } else {
                tooltipDialog.visualParent = appletDelegate
                toolTipShowTimer.restart()
                toolTipHideTimer.running = false
            }
        }
        mainItem: Tooltip { id: tooltipWidget }
        Behavior on y {
            NumberAnimation { duration: Kirigami.Units.longDuration }
        }
    }
    Timer {
        id: toolTipShowTimer
        interval: 500
        repeat: false
        onTriggered: {
            tooltipDialog.visible = true
        }
    }
    Timer {
        id: toolTipHideTimer
        interval: 1000
        repeat: false
        onTriggered: tooltipDialog.visible = false
    }
    */


    header: PlasmaExtras.PlasmoidHeading {
        ColumnLayout {
            id: header
            anchors.fill: parent

            RowLayout {
                PlasmaExtras.Heading {
                    id: heading
                    level: 1
                    text: i18ndc("plasma_shell_org.kde.plasma.desktop", "@title:group for widget grid", "Widgets")
                    elide: Text.ElideRight
                    Kirigami.Theme.colorSet: (themeExtended && themeExtended.isDarkTheme)
                        ? Kirigami.Theme.Complementary
                        : Kirigami.Theme.View
                    Kirigami.Theme.inherit: false

                    Layout.fillWidth: true
                }
                PC3.ToolButton {
                    id: getWidgetsButton
                    icon.name: "get-hot-new-stuff"
                    Kirigami.Theme.colorSet: (themeExtended && themeExtended.isDarkTheme)
                        ? Kirigami.Theme.Complementary
                        : Kirigami.Theme.Button
                    Kirigami.Theme.inherit: false
                    text: i18ndc("plasma_shell_org.kde.plasma.desktop", "@action:button The word 'new' refers to widgets", "Get New…")
                    Accessible.name: i18ndc("plasma_shell_org.kde.plasma.desktop", "@action:button", "Get New Widgets…")
                    onClicked: {
                        if (!widgetExplorer) return
                        getWidgetsDialog.model = widgetExplorer.widgetsMenuActions
                        getWidgetsDialog.open(0, getWidgetsButton.height)
                    }
                }
                PC3.ToolButton {
                    id: closeButton
                    icon.name: "window-close"
                    Kirigami.Theme.colorSet: (themeExtended && themeExtended.isDarkTheme)
                        ? Kirigami.Theme.Complementary
                        : Kirigami.Theme.Button
                    Kirigami.Theme.inherit: false
                    onClicked: main.forceClose()
                }
            }

            RowLayout {
                PC3.TextField {
                    id: searchInput
                    Kirigami.Theme.colorSet: (themeExtended && themeExtended.isDarkTheme)
                        ? Kirigami.Theme.Complementary
                        : Kirigami.Theme.View
                    Kirigami.Theme.inherit: false
                    Layout.fillWidth: true
                    clearButtonShown: true
                    placeholderText: i18ndc("plasma_shell_org.kde.plasma.desktop", "@label:textbox accessible", "Search through widgets")

                    inputMethodHints: Qt.ImhNoPredictiveText

                    onTextChanged: {
                        list.positionViewAtBeginning()
                        list.currentIndex = -1
                        if (widgetExplorer && widgetExplorer.widgetsModel) {
                            widgetExplorer.widgetsModel.searchTerm = text
                        }
                    }

                    Component.onCompleted: {
                        try {
                            if (!Kirigami.InputMethod || !Kirigami.InputMethod.willShowOnActive) { forceActiveFocus(); }
                        } catch(e) { forceActiveFocus(); }
                    }
                }
                PC3.ToolButton {
                    id: categoryButton
                    Kirigami.Theme.colorSet: (themeExtended && themeExtended.isDarkTheme)
                        ? Kirigami.Theme.Complementary
                        : Kirigami.Theme.Button
                    Kirigami.Theme.inherit: false
                    text: i18ndc("plasma_shell_org.kde.plasma.desktop", "@action:button like listbox, switches category to all widgets", "All Widgets")
                    icon.name: "view-filter"
                    onClicked: {
                        if (widgetExplorer && widgetExplorer.filterModel) {
                            categoriesDialog.model = widgetExplorer.filterModel
                            categoriesDialog.open(0, categoryButton.height)
                        }
                    }

                    PC3.ToolTip {
                        text: i18ndc("plasma_shell_org.kde.plasma.desktop", "@action:button tooltip only", "Categories")
                    }
                }
            }
        }
    }

    Timer {
        id: setModelTimer
        interval: main.setModelDelay
        running: true
        onTriggered: {
            if (widgetExplorer && widgetExplorer.widgetsModel) {
                list.model = widgetExplorer.widgetsModel
            }
        }
    }

    PC3.ScrollView {
        anchors.fill: parent
        //anchors.rightMargin: - main.sidePanel.margins.right

        // hide the flickering by fading in nicely
        opacity: setModelTimer.running ? 0 : 1
        Behavior on opacity {
            OpacityAnimator {
                duration: Kirigami.Units.longDuration
                easing.type: Easing.InOutQuad
            }
        }

        GridView {
            id: list

            // model set delayed by Timer above

            activeFocusOnTab: true
            keyNavigationWraps: true
            cellWidth: Math.floor(width / 3)
            cellHeight: cellWidth + Kirigami.Units.gridUnit * 4 + Kirigami.Units.smallSpacing * 2

            delegate: AppletDelegate {}
            highlight: PlasmaExtras.Highlight {}
            highlightMoveDuration: 0
            //highlightResizeDuration: 0

            //slide in to view from the left
            add: Transition {
                NumberAnimation {
                    properties: "x"
                    from: -list.width
                    duration: Kirigami.Units.shortDuration
                }
            }

            //slide out of view to the right
            remove: Transition {
                NumberAnimation {
                    properties: "x"
                    to: list.width
                    duration: Kirigami.Units.shortDuration
                }
            }

            //if we are adding other items into the view use the same animation as normal adding
            //this makes everything slide in together
            //if we make it move everything ends up weird
            addDisplaced: list.add

            //moved due to filtering
            displaced: Transition {
                NumberAnimation {
                    properties: "x,y"
                    duration: Kirigami.Units.shortDuration
                }
            }
        }
    }

    PlasmaExtras.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - (Kirigami.Units.largeSpacing * 4)
        text: searchInput.text.length > 0
            ? i18ndc("plasma_shell_org.kde.plasma.desktop", "@info placeholdermessage", "No widgets matched the search terms")
            : i18ndc("plasma_shell_org.kde.plasma.desktop", "@info placeholdermessage", "No widgets available")
        visible: list.count == 0
    }

    //! Bindings
    Binding{
        target: viewConfig
        property: "hideOnWindowDeactivate"
        value: !main.preventWindowHide
    }

    //! Timers
    Timer {
        id: pendingUninstallTimer
        // keeps track of the applets the user wants to uninstall
        property var applets: []

        interval: main.pendingUninstallDelay
        onTriggered: {
            if (!widgetExplorer || typeof widgetExplorer.uninstall !== "function") {
                applets = []
                return
            }

            for (var i = 0, length = applets.length; i < length; ++i) {
                widgetExplorer.uninstall(applets[i])
            }
            applets = []

            /*if (sidePanelStack.state !== "widgetExplorer" && widgetExplorer) {
                widgetExplorer.destroy()
                widgetExplorer = null
            }*/
        }
    }
}
