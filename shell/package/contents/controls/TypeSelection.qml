/*
    SPDX-FileCopyrightText: 2016 Smith AR <audoban@openmailbox.org>
    SPDX-FileCopyrightText: 2016 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.7
import QtQuick.Layouts 1.3

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.kirigami 2.0 as Kirigami

import org.kde.plasma.plasmoid 2.0

import org.kde.latte.components 1.0 as LatteComponents
import org.kde.latte.core 0.2 as LatteCore
import org.kde.latte.private.containment 0.1 as LatteContainment

Grid {
    id: typeRow
    Kirigami.Theme.inherit: false
    Kirigami.Theme.colorSet: Kirigami.Theme.Window

    readonly property var theme: Kirigami.Theme
    readonly property var units: Kirigami.Units

    width: horizontal ? content.width - 4*units.smallSpacing : 9 * theme.defaultFont.pixelSize
    anchors.leftMargin: horizontal ? units.smallSpacing : 0
    anchors.rightMargin: horizontal ? units.smallSpacing : 0

    Layout.minimumWidth: width
    Layout.maximumWidth: width
    Layout.minimumHeight: height
    Layout.maximumHeight: height

    Layout.leftMargin: units.smallSpacing * 2
    Layout.rightMargin: units.smallSpacing * 2

    rows: horizontal ? 1 : 0
    columns: horizontal ? 0 : 1

    spacing: 1

    verticalItemAlignment: Grid.AlignVCenter

    property bool horizontal: false

    LatteComponents.Button {
        id: dockTypeButton
        Kirigami.Theme.inherit: true
        width: horizontal ? (parent.width - parent.spacing)/ 2 : parent.width
        enabled: LatteCore.WindowSystem.compositingActive

        checkable: true
        checked: latteView.type === LatteCore.types.DockView
        text: i18nc("dock type","Dock")
        PlasmaComponents.ToolTip { text: i18n("Change the behavior and appearance to Dock type") }

        onClicked: {
            if (!checked) {
                latteView.userRequestedViewType(LatteCore.types.DockView);

                latteView.visibility.mode = LatteCore.types.DodgeActive;
                plasmoid.configuration.alignment = LatteCore.types.Center;
                plasmoid.configuration.useThemePanel = true;
                plasmoid.configuration.solidPanel = false;
                // In Plasma 6/Wayland, very small dock thickness can make task
                // items effectively disappear on side edges.
                plasmoid.configuration.panelSize = 10;
                plasmoid.configuration.appletShadowsEnabled = true;
                plasmoid.configuration.zoomLevel = 16;
                //plasmoid.configuration.autoDecreaseIconSize = true;

                //! Empty Areas
                plasmoid.configuration.dragActiveWindowEnabled = false;
                plasmoid.configuration.scrollAction = LatteContainment.types.ScrollNone;

                //! Items
                plasmoid.configuration.autoSizeEnabled = true;

                //! Dynamic Background
                plasmoid.configuration.solidBackgroundForMaximized = false;
                plasmoid.configuration.colorizeTransparentPanels = false;
                plasmoid.configuration.backgroundOnlyOnMaximized = false;
                plasmoid.configuration.disablePanelShadowForMaximized = false;
                plasmoid.configuration.plasmaBackgroundForPopups = false;

                //! Floating
                plasmoid.configuration.floatingInternalGapIsForced = true;
            }
        }
    }

    LatteComponents.Button {
        id: panelTypeButton
        Kirigami.Theme.inherit: true
        width: dockTypeButton.width
        enabled: LatteCore.WindowSystem.compositingActive

        checkable: true
        checked: latteView.type === LatteCore.types.PanelView
        text: i18nc("panel type","Panel")
        PlasmaComponents.ToolTip { text: i18n("Change the behavior and appearance to Panel type") }

        onClicked: {
            if (!checked) {
                latteView.userRequestedViewType(LatteCore.types.PanelView);

                latteView.visibility.mode = LatteCore.types.AlwaysVisible;
                plasmoid.configuration.alignment = LatteCore.types.Justify;
                plasmoid.configuration.useThemePanel = true;
                plasmoid.configuration.solidPanel = false;
                plasmoid.configuration.panelSize = 100;
                plasmoid.configuration.panelShadows = true;
                plasmoid.configuration.appletShadowsEnabled = false;
                plasmoid.configuration.zoomLevel = 0;
                plasmoid.configuration.titleTooltips = false;
                //plasmoid.configuration.autoDecreaseIconSize = false;

                //! Empty Areas
                plasmoid.configuration.dragActiveWindowEnabled = true;

                //! Items
                plasmoid.configuration.autoSizeEnabled = false;

                //! Dynamic Background
                plasmoid.configuration.colorizeTransparentPanels = false;
                plasmoid.configuration.backgroundOnlyOnMaximized = false;
                plasmoid.configuration.disablePanelShadowForMaximized = false;
                plasmoid.configuration.plasmaBackgroundForPopups = true;

                //! Floating
                plasmoid.configuration.floatingInternalGapIsForced = false;

                //! Custom Background that overrides Plasma Theme metrics
                plasmoid.configuration.backgroundRadius = -1;
                plasmoid.configuration.backgroundShadowSize = -1;
            }
        }
    }
}
