/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import org.kde.plasma.core as PlasmaCore
import org.kde.kirigami as Kirigami

import org.kde.latte.private.app as LatteApp
import org.kde.latte.core as LatteCore
import org.kde.latte.private.containment as LatteContainment

import "canvas" as CanvasComponent

Loader {
    id: configurationLoader

    // Inputs published by C++ (SubConfigView::setRootInput(), see
    // app/view/settings/subconfigview.h); they replace the context properties
    // of the same names. Every window root declares the same six, or
    // setInitialProperties() fails for the missing one; this tree only reads
    // latteView, plasmoid and themeExtended. They are nullable and notify,
    // because the window is re-pointed at another dock (setParentView()).
    property QtObject latteView
    property QtObject plasmoid
    property QtObject viewConfig
    property QtObject universalSettings
    property QtObject layoutsManager
    property QtObject themeExtended

    // The inputs are C++ objects without a registered QML type, so qmllint only
    // sees QtObject here; their members are resolved at runtime.
    // qmllint disable missing-property
    active: plasmoid && plasmoid.configuration && latteView
    // qmllint enable missing-property

    sourceComponent: Item{
        id: root
        // Read from the Loader explicitly: a property that reads its own name
        // would bind to itself instead of this root input.
        property var latteView: configurationLoader.latteView
        property var plasmoid: configurationLoader.plasmoid
        property var themeExtended: configurationLoader.themeExtended
        readonly property var canvasRootReference: root
        readonly property var latteViewReference: latteView
        readonly property var plasmoidReference: plasmoid
        readonly property var themeExtendedReference: themeExtended
        readonly property var graphicsSystemReference: graphicsSystem
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

        //! Settings Overlay
        CanvasComponent.SettingsOverlay {
            id: settingsOverlay
            root: canvasRootReference
            latteView: latteViewReference
            plasmoid: plasmoidReference
            themeExtended: themeExtendedReference
            graphicsSystem: graphicsSystemReference
            anchors.fill: parent
        }
    }
}
