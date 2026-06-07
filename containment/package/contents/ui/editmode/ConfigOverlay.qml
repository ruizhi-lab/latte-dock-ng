/*
    SPDX-FileCopyrightText: 2016 Smith AR <audoban@openmailbox.org>
    SPDX-FileCopyrightText: 2016 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.7
import QtQuick.Layouts 1.0

import org.kde.kirigami 2.20 as Kirigami
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents

import org.kde.latte.core 0.2 as LatteCore

MouseArea {
    id: hoverTooltip
    z: 900
    readonly property var units: Kirigami.Units

    width: plasmoid.formFactor === PlasmaCore.Types.Horizontal ? root.width : thickness
    height: plasmoid.formFactor === PlasmaCore.Types.Vertical ? root.height : thickness

    visible: root.inConfigureAppletsMode
    hoverEnabled: root.inConfigureAppletsMode
    cursorShape: Qt.ArrowCursor

    readonly property int thickness: metrics.mask.thickness.maxNormal - metrics.extraThicknessForNormal

    property Item currentApplet

    function contextualAction(appletObj, actionName) {
        if (!appletObj || !actionName) return null;
        if (typeof appletObj.action === "function") return appletObj.action(actionName);
        if (appletObj.contextualActions) {
            for (var i = 0; i < appletObj.contextualActions.length; ++i) {
                var c = appletObj.contextualActions[i];
                if (!c) continue;
                if (c.objectName === actionName || c.name === actionName) return c;
            }
        }
        return null;
    }

    function hoveredItem(x, y) {
        var rl, item;
        rl = mapFromItem(layoutsContainer.mainLayout, 0, 0);
        item = layoutsContainer.mainLayout.childAt(x-rl.x, y-rl.y);
        if (!item) { rl = mapFromItem(layoutsContainer.startLayout,0,0); item = layoutsContainer.startLayout.childAt(x-rl.x, y-rl.y); }
        if (!item) { rl = mapFromItem(layoutsContainer.endLayout,0,0); item = layoutsContainer.endLayout.childAt(x-rl.x, y-rl.y); }
        return item;
    }

    onPositionChanged: {
        var item = hoveredItem(mouse.x, mouse.y);
        if (item && !item.isParabolicEdgeSpacer) {
            currentApplet = item;
            hideTimer.stop();
            nullifyAppletTimer.stop();
            tooltip.visible = true;
            tooltip.raise();
        }
    }

    onExited: {
        // Don't start the hide timer if the cursor is already in the tooltip
        // dialog (a separate window) — the tooltip should remain interactive.
        if (tooltip.visible && tooltipMouseArea.containsMouse) {
            return;
        }
        hideTimer.restart();
    }

    onCurrentAppletChanged: {
        if (!currentApplet) {
            // Debounce nullification: a short delay prevents the tooltip
            // from flickering when hover briefly lands between items or when
            // the cursor passes over a ParabolicEdgeSpacer.
            nullifyAppletTimer.start();
            return;
        }
        nullifyAppletTimer.stop();
        lockButton.checked = currentApplet.lockZoom;
        colorizingButton.checked = !currentApplet.userBlocksColorizing;
    }

    Timer {
        id: nullifyAppletTimer
        interval: 80
        onTriggered: {
            hideTimer.restart();
        }
    }

    Timer {
        id: hideTimer
        interval: animations.duration.large * 2
        onTriggered: {
            if (!tooltipMouseArea.containsMouse) {
                tooltip.visible = false;
                currentApplet = null;
            }
        }
    }

    PlasmaCore.Dialog {
        id: tooltip
        visualParent: currentApplet
        type: PlasmaCore.Dialog.Dock
        flags: Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus | Qt.BypassWindowManagerHint | Qt.ToolTip
        location: plasmoid.location

        onVisualParentChanged: {
            if (!visualParent || !currentApplet) return;
            if (!(currentApplet.applet || currentApplet.isSeparator || currentApplet.isInternalViewSplitter)) return;

            configureButton.visible = !currentApplet.isInternalViewSplitter
                && currentApplet.applet && currentApplet.applet.pluginName !== "org.kde.latte.plasmoid"
                && contextualAction(currentApplet.applet, "configure")
                && contextualAction(currentApplet.applet, "configure").enabled;
            closeButton.visible = !currentApplet.isInternalViewSplitter
                && currentApplet.applet
                && contextualAction(currentApplet.applet, "remove")
                && contextualAction(currentApplet.applet, "remove").enabled;
            lockButton.visible = !currentApplet.isInternalViewSplitter
                && currentApplet.communicator && !currentApplet.communicator.indexerIsSupported
                && !currentApplet.communicator.appletBlocksParabolicEffect
                && !currentApplet.isSeparator;
            colorizingButton.visible = root.colorizerEnabled && !currentApplet.appletBlocksColorizing && !currentApplet.isInternalViewSplitter;

            var labelText = "";
            if (currentApplet.isInternalViewSplitter) labelText = i18n("Justify Splitter");
            else if (currentApplet.isSeparator) labelText = i18n("Separator");
            else if (currentApplet.applet) {
                labelText = currentApplet.applet.title || currentApplet.applet.pluginName || i18n("Applet");
            }
            label.text = labelText;
        }

        mainItem: MouseArea {
            id: tooltipMouseArea
            enabled: currentApplet
            width: handleRow.childrenRect.width + (2 * handleRow.spacing)
            height: Math.max(configureButton.height, label.contentHeight, closeButton.height)
            hoverEnabled: true
            LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
            LayoutMirroring.childrenInherit: true
            onEntered: hideTimer.stop();
            onExited: hideTimer.restart();

            Row {
                id: handleRow
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 2*units.smallSpacing
                Row {
                    spacing: units.smallSpacing
                    PlasmaComponents.ToolButton {
                        id: configureButton
                        anchors.verticalCenter: parent.verticalCenter
                        icon.name: "configure"
                        onClicked: {
                            tooltip.visible = false;
                            var a = contextualAction(currentApplet.applet, "configure");
                            if (a) a.trigger();
                        }
                    }
                    PlasmaComponents.Label {
                        id: label
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.rightMargin: units.smallSpacing
                        textFormat: Text.PlainText
                        maximumLineCount: 1
                    }
                    Row {
                        spacing: units.smallSpacing/2
                        PlasmaComponents.ToolButton {
                            id: colorizingButton
                            checkable: true
                            icon.name: "color-picker"
                            onClicked: { fastLayoutManager.setOption(currentApplet.applet.id, "userBlocksColorizing", !checked); }
                        }
                        PlasmaComponents.ToolButton {
                            id: lockButton
                            checkable: true
                            icon.name: checked ? "lock" : "unlock"
                            onClicked: { fastLayoutManager.setOption(currentApplet.applet.id, "lockZoom", checked); }
                        }
                        PlasmaComponents.ToolButton {
                            id: closeButton
                            anchors.verticalCenter: parent.verticalCenter
                            icon.name: "delete"
                            onClicked: {
                                tooltip.visible = false;
                                if (currentApplet && currentApplet.applet) {
                                    var a = contextualAction(currentApplet.applet, "remove");
                                    if (a) a.trigger();
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    states: [
        State {
            name: "bottom"; when: (plasmoid.location === PlasmaCore.Types.BottomEdge)
            AnchorChanges { target: hoverTooltip; anchors { top:undefined; bottom:parent.bottom; left:undefined; right:undefined; horizontalCenter:parent.horizontalCenter; verticalCenter:undefined } }
        },
        State {
            name: "top"; when: (plasmoid.location === PlasmaCore.Types.TopEdge)
            AnchorChanges { target: hoverTooltip; anchors { top:parent.top; bottom:undefined; left:undefined; right:undefined; horizontalCenter:parent.horizontalCenter; verticalCenter:undefined } }
        },
        State {
            name: "left"; when: (plasmoid.location === PlasmaCore.Types.LeftEdge)
            AnchorChanges { target: hoverTooltip; anchors { top:undefined; bottom:undefined; left:parent.left; right:undefined; horizontalCenter:undefined; verticalCenter:parent.verticalCenter } }
        },
        State {
            name: "right"; when: (plasmoid.location === PlasmaCore.Types.RightEdge)
            AnchorChanges { target: hoverTooltip; anchors { top:undefined; bottom:undefined; left:undefined; right:parent.right; horizontalCenter:undefined; verticalCenter:parent.verticalCenter } }
        }
    ]
}
