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

    onCompactRepresentationChanged: {
        if (compactRepresentation) {
            originalCompactRepresenationParent = compactRepresentation.parent;

            // Match standard Plasma shell: reparent into FocusScope with anchors.fill
            compactRepresentation.anchors.fill = null;
            compactRepresentation.parent = compactRepresentationParent;
            compactRepresentation.anchors.fill = compactRepresentationParent;
            compactRepresentation.visible = true;
        }
        root.visible = true;
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
        hideOnWindowDeactivate: plasmoidItem && plasmoidItem.hideOnWindowDeactivate !== undefined ? plasmoidItem.hideOnWindowDeactivate : true
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

            Layout.minimumWidth: root.fullRepresentation ? root.fullRepresentation.Layout.minimumWidth : 0
            Layout.minimumHeight: root.fullRepresentation ? root.fullRepresentation.Layout.minimumHeight : 0

            Layout.maximumWidth: root.fullRepresentation ? root.fullRepresentation.Layout.maximumWidth : Infinity
            Layout.maximumHeight: root.fullRepresentation ? root.fullRepresentation.Layout.maximumHeight : Infinity

            implicitWidth: {
                if (root.fullRepresentation !== null) {
                    if (root.fullRepresentation.Layout.preferredWidth > 0) {
                        return root.fullRepresentation.Layout.preferredWidth;
                    } else if (root.fullRepresentation.implicitWidth > 0) {
                        return root.fullRepresentation.implicitWidth;
                    }
                }
                return Kirigami.Units.iconSizes.sizeForLabels * 35;
            }
            implicitHeight: {
                if (root.fullRepresentation !== null) {
                    if (root.fullRepresentation.Layout.preferredHeight > 0) {
                        return root.fullRepresentation.Layout.preferredHeight;
                    } else if (root.fullRepresentation.implicitHeight > 0) {
                        return root.fullRepresentation.implicitHeight;
                    }
                }
                return Kirigami.Units.iconSizes.sizeForLabels * 25;
            }

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
