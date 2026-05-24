/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.8
import QtQuick.Controls 2.15 as QQC2
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.kirigami 2.0 as Kirigami

import org.kde.latte.private.app 0.1 as LatteApp
import org.kde.latte.core 0.2 as LatteCore
import org.kde.latte.private.containment 0.1 as LatteContainment

import "canvas" as CanvasComponent

Loader {
    active: plasmoid && plasmoid.configuration && latteView

    sourceComponent: Item{
        id: root
        readonly property var theme: Kirigami.Theme
        readonly property var units: Kirigami.Units
        readonly property bool isVertical: plasmoid.formFactor === PlasmaCore.Types.Vertical
        readonly property bool isHorizontal: !isVertical

        property int animationSpeed: LatteCore.WindowSystem.compositingActive ? 500 : 0
        property int panelAlignment: plasmoid.configuration.alignment

        property real offset: {
            if (root.isHorizontal) {
                return width * (plasmoid.configuration.offset/100);
            } else {
                return height * (plasmoid.configuration.offset/100)
            }
        }

        property string appChosenShadowColor: {
            if (plasmoid.configuration.shadowColorType === LatteContainment.types.ThemeColorShadow) {
                var strC = String(theme.textColor);
                return strC.indexOf("#") === 0 ? strC.substr(1) : strC;
            } else if (plasmoid.configuration.shadowColorType === LatteContainment.types.UserColorShadow) {
                return plasmoid.configuration.shadowColor;
            }

            // default shadow color
            return "080808";
        }

        property string appShadowColorSolid: "#" + appChosenShadowColor

        //// BEGIN OF Behaviors
        Behavior on offset {
            NumberAnimation {
                id: offsetAnimation
                duration: animationSpeed
                easing.type: Easing.OutCubic
            }
        }
        //// END OF Behaviors

        Item {
            id: graphicsSystem
            readonly property bool isAccelerated: (GraphicsInfo.api !== GraphicsInfo.Software)
                                                  && (GraphicsInfo.api !== GraphicsInfo.Unknown)
        }

        LatteApp.ContextMenuLayer {
            id: contextMenuLayer
            anchors.fill: parent
            view: latteView
        }

        MouseArea {
            id: editBackMouseArea
            anchors.fill: parent
            visible: !universalSettings.inConfigureAppletsMode
            hoverEnabled: true

            property bool wheelIsBlocked: false
            readonly property int opacityStep: 5
            readonly property string tooltip: i18nc("opacity for dock background, %1 is opacity percentage",
                                                    "You can use mouse wheel to change background opacity of %1%",
                                                    plasmoid.configuration.panelTransparency < 0 ? 100 : plasmoid.configuration.panelTransparency)

            onWheel: function(wheel) {
                if (wheelIsBlocked) return
                wheelIsBlocked = true
                scrollDelayer.start()
                var angle = wheel.angleDelta.y / 8
                var current = plasmoid.configuration.panelTransparency
                if (current < 0) current = 100
                var newVal = -1
                if (angle > 10) newVal = Math.min(100, current + opacityStep)
                else if (angle < -10) newVal = Math.max(0, current - opacityStep)
                if (newVal >= 0) {
                    plasmoid.configuration.panelTransparency = newVal
                }
            }

            Timer {
                id: scrollDelayer
                interval: 80
                onTriggered: editBackMouseArea.wheelIsBlocked = false
            }
        }

        PlasmaComponents.Button {
            anchors.fill: editBackMouseArea
            opacity: 0
            Kirigami.Theme.inherit: true
            hoverEnabled: true

            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            QQC2.ToolTip.visible: hovered && editBackMouseArea.tooltip !== ""
            QQC2.ToolTip.text: editBackMouseArea.tooltip
        }

        //! Settings Overlay
        CanvasComponent.SettingsOverlay {
            id: settingsOverlay
            anchors.fill: parent
        }
    }
}
