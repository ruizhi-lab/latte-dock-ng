/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0
import QtQuick.Effects

import org.kde.kirigami 2.20 as Kirigami
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.plasmoid
import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.kquickcontrolsaddons 2.0

PlasmaCore.ToolTipArea {
    id: root
    objectName: "org.kde.desktop-CompactApplet"
    anchors.fill: parent

    mainText: plasmoidItem ? (plasmoidItem.toolTipMainText !== undefined ? plasmoidItem.toolTipMainText : "") : ""
    subText: plasmoidItem ? (plasmoidItem.toolTipSubText !== undefined ? plasmoidItem.toolTipSubText : "") : ""
    location: plasmoidItem && plasmoidItem.location !== undefined ? plasmoidItem.location : PlasmaCore.Types.BottomEdge
    active: plasmoidItem && plasmoidItem.expanded !== undefined ? !plasmoidItem.expanded : false
    textFormat: plasmoidItem ? (plasmoidItem.toolTipTextFormat !== undefined ? plasmoidItem.toolTipTextFormat : Text.PlainText) : Text.PlainText
    mainItem: plasmoidItem && plasmoidItem.toolTipItem ? plasmoidItem.toolTipItem : null

    property var plasmoidItem
    property Item fullRepresentation: null
    property Item compactRepresentation: null
    /*Discover real visual parent - the following code points to Applet::ItemWrapper*/
    property Item originalCompactRepresenationParent: null
    property Item compactRepresentationVisualParent: originalCompactRepresenationParent && originalCompactRepresenationParent.parent
                                                     ? originalCompactRepresenationParent.parent.parent : null

    property Item appletItem: compactRepresentationVisualParent
                              && compactRepresentationVisualParent.parent
                              && compactRepresentationVisualParent.parent.parent ? compactRepresentationVisualParent.parent.parent.parent : null

    FocusScope {
        id: compactRepresentationParent
        anchors.fill: parent
        activeFocusOnTab: true
        objectName: "expandApplet"
        Accessible.name: root.mainText
        Accessible.description: i18ndc("plasma_shell_org.kde.latte.shell", "@info:whatsthis Accessible description for panel widget %1",  "Open %1", root.subText)
        Accessible.role: Accessible.Button
        Accessible.onPressAction: {
            if (typeof Plasmoid !== "undefined" && typeof Plasmoid.activated === "function") {
                Plasmoid.activated();
            }
        }

        Keys.onPressed: event => {
            switch (event.key) {
            case Qt.Key_Space:
            case Qt.Key_Enter:
            case Qt.Key_Return:
            case Qt.Key_Select:
                if (typeof Plasmoid !== "undefined" && typeof Plasmoid.activated === "function") {
                    Plasmoid.activated();
                }
                break;
            }
        }
    }

    function configureAction() {
        if (typeof Plasmoid !== "undefined") {
            if (typeof Plasmoid.action === "function") {
                return Plasmoid.action("configure");
            }
            if (typeof Plasmoid.internalAction === "function") {
                return Plasmoid.internalAction("configure");
            }
        }
        return null;
    }

    function pluginName() {
        if (typeof Plasmoid !== "undefined" && Plasmoid.pluginName) {
            return Plasmoid.pluginName;
        }
        return appletItem && appletItem.pluginName ? appletItem.pluginName : "";
    }

    function isVolumeApplet() {
        return pluginName() === "org.kde.plasma.volume";
    }

    function isApplicationMenuApplet() {
        var plugin = pluginName();
        return plugin === "org.kde.plasma.kicker"
                || plugin === "org.kde.plasma.kickoff"
                || plugin === "org.kde.plasma.kickerdash"
                || plugin === "org.kde.plasma.appmenu";
    }

    function representationMinimumWidth() {
        return root.fullRepresentation ? root.fullRepresentation.Layout.minimumWidth : 0;
    }

    function representationMinimumHeight() {
        return root.fullRepresentation ? root.fullRepresentation.Layout.minimumHeight : 0;
    }

    function representationPreferredWidth() {
        if (root.fullRepresentation !== null) {
            if (root.fullRepresentation.Layout.preferredWidth > 0) {
                return root.fullRepresentation.Layout.preferredWidth;
            } else if (root.fullRepresentation.implicitWidth > 0) {
                return root.fullRepresentation.implicitWidth;
            }
        }
        return Kirigami.Units.iconSizes.sizeForLabels * 35;
    }

    function representationPreferredHeight() {
        if (root.fullRepresentation !== null) {
            if (root.fullRepresentation.Layout.preferredHeight > 0) {
                return root.fullRepresentation.Layout.preferredHeight;
            } else if (root.fullRepresentation.implicitHeight > 0) {
                return root.fullRepresentation.implicitHeight;
            }
        }
        return Kirigami.Units.iconSizes.sizeForLabels * 25;
    }

    function popupMenuMinimumWidth() {
        var minimum = representationMinimumWidth();
        var resizableMinimum = Kirigami.Units.gridUnit * 18;

        if (minimum > 0) {
            return Math.min(minimum, resizableMinimum);
        }

        return resizableMinimum;
    }

    function popupMinimumWidth() {
        if (isApplicationMenuApplet()) {
            return popupMenuMinimumWidth();
        }

        var minimum = representationMinimumWidth();
        if (isVolumeApplet()) {
            minimum = Math.max(minimum, Kirigami.Units.gridUnit * 27);
        }
        return minimum;
    }

    function popupMenuMinimumHeight() {
        var minimum = representationMinimumHeight();
        var resizableMinimum = Kirigami.Units.gridUnit * 18;

        if (minimum > 0) {
            return Math.min(minimum, resizableMinimum);
        }

        return resizableMinimum;
    }

    function popupMinimumHeight() {
        if (isApplicationMenuApplet()) {
            return popupMenuMinimumHeight();
        }

        var minimum = representationMinimumHeight();
        if (isVolumeApplet()) {
            minimum = Math.max(minimum, Kirigami.Units.gridUnit * 27);
        }
        return minimum;
    }

    function popupPreferredWidth() {
        return Math.max(popupMinimumWidth(), representationPreferredWidth());
    }

    function popupPreferredHeight() {
        return Math.max(popupMinimumHeight(), representationPreferredHeight());
    }

    function popupMaximumWidth() {
        if (isApplicationMenuApplet()) {
            return Infinity;
        }
        return root.fullRepresentation ? root.fullRepresentation.Layout.maximumWidth : Infinity;
    }

    function popupMaximumHeight() {
        if (isApplicationMenuApplet()) {
            return Infinity;
        }
        return root.fullRepresentation ? root.fullRepresentation.Layout.maximumHeight : Infinity;
    }

    function findAppletItem() {
        // Walk up the visual parent chain from CompactApplet's parent
        // (_wrapperContainer) through ItemWrapper, Flow, to AppletItem.
        // Returns the first ancestor that has externalAppletNaturalWidth.
        var item = root.parent;
        var depth = 0;
        while (item && depth < 16) {
            if ("externalAppletNaturalWidth" in item) {
                return item;
            }
            item = item.parent;
            depth++;
        }
        return null;
    }

    function findDeepImplicitWidth(item, depth) {
        if (!item || depth > 12) return 0;
        var best = item.implicitWidth > 0 ? item.implicitWidth : 0;
        for (var i = 0; i < (item.children ? item.children.length : 0); i++) {
            best = Math.max(best, findDeepImplicitWidth(item.children[i], depth + 1));
        }
        return best;
    }

    function findDeepImplicitHeight(item, depth) {
        if (!item || depth > 12) return 0;
        var best = item.implicitHeight > 0 ? item.implicitHeight : 0;
        for (var i = 0; i < (item.children ? item.children.length : 0); i++) {
            best = Math.max(best, findDeepImplicitHeight(item.children[i], depth + 1));
        }
        return best;
    }

    function captureNaturalSize() {
        if (!compactRepresentation) {
            return;
        }
        var target = appletItem || findAppletItem();
        if (!target) {
            return;
        }
        if (!target.externalAppletDrawsAboveTasks) {
            return; // internal applet, nothing to do
        }

        // Text-heavy external applets (clocks, etc.) need wider slots.
        // Match any clock plugin by name, but exclude analog clocks
        // (square clock faces, not text-heavy) so they keep standard
        // icon-size slot behavior.
        // Other external applets keep their existing fixed-slot behavior.
        var plugin = target.pluginName || "";
        var isTextApplet = plugin.indexOf("clock") >= 0
                         && plugin.indexOf("analogclock") < 0;

        if (!isTextApplet) {
            return;
        }

        // Try compactRepresentation.implicitWidth first; if 0, recurse
        // into children to find the deepest content item's implicit size.
        var w = compactRepresentation.implicitWidth;
        if (w <= 0) {
            w = findDeepImplicitWidth(compactRepresentation, 0);
        }
        var h = compactRepresentation.implicitHeight;
        if (h <= 0) {
            h = findDeepImplicitHeight(compactRepresentation, 0);
        }
        // Cap the captured natural width to prevent runaway values during
        // layout transitions (e.g. mode switch, autosize animation) where
        // findDeepImplicitWidth may temporarily report the full container
        // width instead of the content's intrinsic size.
        if (w > 0 && target.metrics && target.metrics.maxIconSize > 0) {
            w = Math.min(w, target.metrics.maxIconSize * 8);
        }
        target.externalAppletNaturalWidth = w;
        target.externalAppletNaturalHeight = h;

        // Start polling for content width changes (e.g. clock text "1:00" → "12:34").
        naturalWidthPollTimer.running = true;
    }

    function updateNaturalWidth() {
        if (!compactRepresentation) {
            return;
        }
        var w = compactRepresentation.implicitWidth;
        if (w <= 0) {
            w = findDeepImplicitWidth(compactRepresentation, 0);
        }
        // Same cap as captureNaturalSize to avoid runaway values.
        var target = appletItem || findAppletItem();
        if (target && target.metrics && target.metrics.maxIconSize > 0) {
            w = Math.min(w, target.metrics.maxIconSize * 8);
        }
        if (target && target.externalAppletDrawsAboveTasks
            && w > 0 && w !== target.externalAppletNaturalWidth) {
            target.externalAppletNaturalWidth = w;
        }
    }

    // Keep the slot width in sync when clock text changes (e.g. "1:00" → "12:34"),
    // or when the user switches between long and short date formats.
    // React to implicit width changes on the compact representation and its
    // children so resizes are applied immediately instead of waiting for the
    // next poll cycle.
    Connections {
        target: compactRepresentation
        enabled: compactRepresentation && naturalWidthPollTimer.running
        function onImplicitWidthChanged() { updateNaturalWidth(); }
        function onChildrenRectChanged() { updateNaturalWidth(); }
    }

    Timer {
        id: naturalWidthPollTimer
        interval: 2000
        repeat: true
        running: false
        onTriggered: updateNaturalWidth();
    }

    onCompactRepresentationChanged: {
        if (compactRepresentation) {
            originalCompactRepresenationParent = compactRepresentation.parent;

            // Match standard Plasma shell: reparent into FocusScope with anchors.fill
            compactRepresentation.anchors.fill = null;
            compactRepresentation.parent = compactRepresentationParent;
            compactRepresentation.anchors.fill = compactRepresentationParent;
            compactRepresentation.visible = true;

            // Capture the natural implicit size for text-based applets
            // (clocks like digital clock, colorful digital clock, etc.)
            // so the dock can allocate a wider slot.  Deferred to avoid
            // touching the hot anchoring path for other applets.
            var pn = "";
            if (typeof Plasmoid !== "undefined") { pn = Plasmoid.pluginName || ""; }
            if (pn.indexOf("clock") >= 0 && pn.indexOf("analogclock") < 0) {
                slotSizeCaptureTimer.start();
            }
        }
        root.visible = true;
    }

    Timer {
        id: slotSizeCaptureTimer
        interval: 0 // defer to next event loop iteration
        repeat: false
        onTriggered: captureNaturalSize();
    }

    onFullRepresentationChanged: {
        if (!fullRepresentation) {
            return;
        }

        // Break anchors before reparenting, otherwise the fullRepresentation
        // can end up with zero geometry inside the popup (blank window).
        fullRepresentation.anchors.fill = null;
        fullRepresentation.parent = appletParent;
        fullRepresentation.anchors.fill = fullRepresentation.parent;
    }

   /* PlasmaCore.FrameSvgItem {
        id: expandedItem
        anchors.fill: parent
        imagePath: "widgets/tabbar"
        visible: fromCurrentTheme && opacity > 0
        prefix: {
            var prefix;
            switch (plasmoid.location) {
                case PlasmaCore.Types.LeftEdge:
                    prefix = "west-active-tab";
                    break;
                case PlasmaCore.Types.TopEdge:
                    prefix = "north-active-tab";
                    break;
                case PlasmaCore.Types.RightEdge:
                    prefix = "east-active-tab";
                    break;
                default:
                    prefix = "south-active-tab";
                }
                if (!hasElementPrefix(prefix)) {
                    prefix = "active-tab";
                }
                return prefix;
            }
        opacity: plasmoid.expanded ? 1 : 0
        Behavior on opacity {
            NumberAnimation {
                duration: PlasmaCore.Units.shortDuration
                easing.type: Easing.InOutQuad
            }
        }
    }*/

    //! This timer is needed in order for the applet popup to not reshow instantly and faulty after the user
    //! clicks compact representation to hide it
    Timer {
        id: expandedSync
        interval: 500
        onTriggered: Plasmoid.expanded = popupWindow.visible;
    }

    Connections {
        target: configureAction()
        function onTriggered() {
            if (typeof Plasmoid !== "undefined") {
                Plasmoid.expanded = false;
            }
        }
    }

    Connections {
        target: Plasmoid
        function onContextualActionsAboutToShow() { root.hideToolTip() }
    }

    PlasmaCore.AppletPopup {
        id: popupWindow
        objectName: "popupWindow"
        visible: !!(plasmoidItem && plasmoidItem.expanded && fullRepresentation)
        visualParent: compactRepresentation ? compactRepresentation : null
        popupDirection: {
            switch (Plasmoid.location) {
            case PlasmaCore.Types.TopEdge:
                return Qt.BottomEdge
            case PlasmaCore.Types.LeftEdge:
                return Qt.RightEdge
            case PlasmaCore.Types.RightEdge:
                return Qt.LeftEdge
            default:
                return Qt.TopEdge
            }
        }
        margin: (Plasmoid.containmentDisplayHints & PlasmaCore.Types.ContainmentPrefersFloatingApplets) ? Kirigami.Units.largeSpacing : 0
        floating: Plasmoid.location === PlasmaCore.Types.Floating
        removeBorderStrategy: Plasmoid.location === PlasmaCore.Types.Floating
            ? PlasmaCore.AppletPopup.AtScreenEdges
            : PlasmaCore.AppletPopup.AtScreenEdges | PlasmaCore.AppletPopup.AtPanelEdges
        hideOnWindowDeactivate: plasmoidItem && plasmoidItem.hideOnWindowDeactivate !== undefined ? plasmoidItem.hideOnWindowDeactivate : false
        backgroundHints: (Plasmoid.containmentDisplayHints & PlasmaCore.Types.ContainmentPrefersOpaqueBackground) ? PlasmaCore.AppletPopup.SolidBackground : PlasmaCore.AppletPopup.StandardBackground
        appletInterface: plasmoidItem

        property var oldStatus: PlasmaCore.Types.UnknownStatus

        //It's a MouseEventListener to get all the events, so the eventfilter will be able to catch them
        mainItem: MouseEventListener {
            id: appletParent

            focus: true

            Keys.onEscapePressed: {
                if (typeof Plasmoid !== "undefined") {
                    Plasmoid.expanded = false;
                }
            }

            LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
            LayoutMirroring.childrenInherit: true

            Layout.minimumWidth: root.popupMinimumWidth()
            Layout.minimumHeight: root.popupMinimumHeight()

            Layout.maximumWidth: root.popupMaximumWidth()
            Layout.maximumHeight: root.popupMaximumHeight()

            implicitWidth: root.popupPreferredWidth()
            implicitHeight: root.popupPreferredHeight()

            onActiveFocusChanged: {
                if (activeFocus && fullRepresentation) {
                    fullRepresentation.forceActiveFocus()
                }
            }
        }

        onVisibleChanged: {
            if (!visible) {
                expandedSync.restart();
                if (typeof Plasmoid !== "undefined") {
                    Plasmoid.status = oldStatus;
                }
            } else {
                if (typeof Plasmoid !== "undefined") {
                    oldStatus = Plasmoid.status;
                    Plasmoid.status = PlasmaCore.Types.RequiresAttentionStatus;
                }
                // This call currently fails and complains at runtime:
                // QWindow::setWindowState: QWindow::setWindowState does not accept Qt::WindowActive
                popupWindow.requestActivate();
            }
        }
    }

    ////Indicators API ////
    Binding {
        target: compactRepresentation ? compactRepresentation.anchors : null
        property: "horizontalCenterOffset"
        when: compactRepresentation
        value: appletItem ? appletItem.iconOffsetX : 0
    }

    Binding {
        target: compactRepresentation ? compactRepresentation.anchors : null
        property: "verticalCenterOffset"
        when: compactRepresentation
        value: appletItem ? appletItem.iconOffsetY : 0
    }

    Binding {
        target: root
        property: "transformOrigin"
        value: appletItem && compactRepresentation ? appletItem.iconTransformOrigin : Item.Center
    }

    Binding {
        target: root
        property: "opacity"
        value: appletItem && compactRepresentation ? appletItem.iconOpacity : 1.0
    }

    Binding {
        target: root
        property: "rotation"
        value: appletItem && compactRepresentation ? appletItem.iconRotation : 0
    }

    Binding {
        target: root
        property: "scale"
        value: appletItem && compactRepresentation ? appletItem.iconScale : 1.0
    }

    ////Clicked Effect ////
    MultiEffect {
        id: _clickedEffect
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: compactRepresentation ? compactRepresentation.anchors.horizontalCenterOffset : 0
        anchors.verticalCenterOffset: compactRepresentation ? compactRepresentation.anchors.verticalCenterOffset : 0
        source: compactRepresentation
        width: root.width
        height: root.height
        visible: appletItem && clickedAnimation.running && !appletItem.indicators.info.providesClickedAnimation
        z:1000
    }

    /////Clicked Animation/////
    SequentialAnimation{
        id: clickedAnimation
        alwaysRunToEnd: true
        running: appletItem
                 && appletItem.animations
                 && appletItem.indicators
                 && appletItem.isSquare
                 && appletItem.pressed
                 && !appletItem.originalAppletBehavior
                 && (appletItem.animations.speedFactor.current > 0)
                 && !appletItem.indicators.info.providesClickedAnimation

        ParallelAnimation{
            PropertyAnimation {
                target: _clickedEffect
                property: "brightness"
                to: -0.35
                duration: appletItem && appletItem.animations ? appletItem.animations.duration.large : 0
                easing.type: Easing.OutQuad
            }
        }
        ParallelAnimation{
            PropertyAnimation {
                target: _clickedEffect
                property: "brightness"
                to: 0
                duration: appletItem && appletItem.animations ? appletItem.animations.duration.large : 0
                easing.type: Easing.OutQuad
            }
        }
    }
    //END animations
}
