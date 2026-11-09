/*
    SPDX-FileCopyrightText: 2016 Smith AR <audoban@openmailbox.org>
    SPDX-FileCopyrightText: 2016 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Window
import QtQuick.Controls as QQC2

import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents

import org.kde.latte.core as LatteCore

import ".." as ContainmentUi
import "../abilities" as Ability
import "../background" as Background
import "../layouts" as Layouts

Window{
    id: debugWindow

    //! main.qml sets these inputs when it loads the window, so the window reads
    //! no name from the containment's context and qmllint checks the typed
    //! ones. Two stay var because qmllint cannot see their members: the
    //! containment root declares its properties in main.qml, which is not an
    //! importable type, and the layouter publishes its three layouts as Item.
    required property var containmentRoot
    required property var layouter
    required property Ability.Animations animations
    required property Ability.AutoSize autosize
    required property Background.MultiLayered background
    required property Ability.Indexer indexer
    required property Layouts.LayoutsContainer layoutsContainer
    required property Ability.Metrics metrics
    required property Ability.ParabolicEffect parabolic
    required property ContainmentUi.VisibilityManager visibilityManager

    //! Bound through the root, so they follow it: Interfaces resolves the view
    //! after the containment loads and clears it when the view goes away.
    //! Neither has a QML type name (the view is a QObject from private.app,
    //! Plasma::Containment is not exported).
    readonly property var latteView: debugWindow.containmentRoot.latteView
    readonly property var plasmoid: debugWindow.containmentRoot.plasmoid

    width: mainGrid.width + 10
    height: Math.min(mainGrid.height+10, Screen.height - debugWindow.metrics.mask.thickness.maxNormalForItems)
    visible: true
    title: "#debugwindow#"

    property string space:" :   "

    PlasmaComponents.ScrollView {
        id: scrollArea

        anchors.fill: parent
        QQC2.ScrollBar.vertical.policy: QQC2.ScrollBar.AsNeeded
        QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff

        Grid{
            id:mainGrid
            columns: 2

            Text{
                text: "Name"+debugWindow.space
            }

            Text{
                text: debugWindow.latteView && debugWindow.latteView.name.length > 0 ? debugWindow.latteView.name : "___"
            }

            Text{
                text: "Screen id"+debugWindow.space
            }

            Text{
                text: debugWindow.latteView && debugWindow.latteView.positioner ? debugWindow.latteView.positioner.currentScreenName : "___"
            }

            Text{
                text: "Screen Geometry"+debugWindow.space
            }

            Text{
                text: {
                    if (debugWindow.latteView && debugWindow.latteView.screenGeometry){
                        return debugWindow.latteView.screenGeometry.x+","+debugWindow.latteView.screenGeometry.y+ " "+debugWindow.latteView.screenGeometry.width+"x"+debugWindow.latteView.screenGeometry.height;
                    } else {
                        return "_,_ _x_";
                    }
                }
            }

            Text{
                text: "Window Geometry"+debugWindow.space
            }

            Text{
                text: {
                    if (debugWindow.latteView) {
                        return  debugWindow.latteView.x + "," + debugWindow.latteView.y + " "+debugWindow.latteView.width+ "x"+debugWindow.latteView.height;
                    } else {
                        return "_,_ _x_";
                    }
                }
            }

            Text{
                text: "On Primary"+debugWindow.space
            }

            Text{
                text: {
                    if (debugWindow.latteView && debugWindow.latteView.onPrimary)
                        return "Yes";
                    else
                        return "No";
                }
            }

            Text{
                text: "   -----------   "
            }

            Text{
                text: " -----------   "
            }

            Text{
                text: "Contents Width"+debugWindow.space
            }

            Text{
                text: debugWindow.layoutsContainer.contentsWidth
            }

            Text{
                text: "Contents Height"+debugWindow.space
            }

            Text{
                text: debugWindow.layoutsContainer.contentsHeight
            }

            Text{
                text: "Max Length"+debugWindow.space
            }

            Text{
                text: debugWindow.containmentRoot.maxLengthPerCentage +"%"
            }

            Text{
                text: "Max Length (pixels)"+debugWindow.space
            }

            Text{
                text: debugWindow.containmentRoot.maxLength
            }

            Text{
                text: "Min Length"+debugWindow.space
            }

            Text{
                text: debugWindow.containmentRoot.minLengthPerCentage +"%"
            }

            Text{
                text: "Min Length (pixels)"+debugWindow.space
            }

            Text{
                text: debugWindow.containmentRoot.minLength
            }

            Text{
                text: "Offset (pixels)"+debugWindow.space
            }

            Text{
                text: debugWindow.containmentRoot.offset
            }

            Text{
                text: "Mask"+debugWindow.space
            }

            Text{
                text: {
                    if (debugWindow.latteView && debugWindow.latteView.effects && debugWindow.latteView.effects.mask) {
                        return debugWindow.latteView.effects.mask.x +", "+ debugWindow.latteView.effects.mask.y+"  "+debugWindow.latteView.effects.mask.width+"x"+debugWindow.latteView.effects.mask.height;
                    } else {
                        return "_,_ _x_";
                    }
                }
            }

            Text{
                text: "Input "+debugWindow.space
            }

            Text{
                text: {
                    if (debugWindow.latteView && debugWindow.latteView.effects && debugWindow.latteView.effects.inputMask) {
                        return debugWindow.latteView.effects.inputMask.x +", "+ debugWindow.latteView.effects.inputMask.y+"  "+debugWindow.latteView.effects.inputMask.width+"x"+debugWindow.latteView.effects.inputMask.height;
                    } else {
                        return "_,_ _x_";
                    }
                }
            }

            Text{
                text: "Local Geometry"+debugWindow.space
            }

            Text{
                text: {
                    if (debugWindow.latteView && debugWindow.latteView.localGeometry) {
                        return debugWindow.latteView.localGeometry.x + ", " + debugWindow.latteView.localGeometry.y + "  " + debugWindow.latteView.localGeometry.width + "x" + debugWindow.latteView.localGeometry.height;
                    } else {
                        return "_,_ _x_";
                    }
                }
            }

            Text{
                text: "Absolute Geometry"+debugWindow.space
            }

            Text{
                text: {
                    if (debugWindow.latteView && debugWindow.latteView.absoluteGeometry) {
                        return debugWindow.latteView.absoluteGeometry.x + ", " + debugWindow.latteView.absoluteGeometry.y + "  " + debugWindow.latteView.absoluteGeometry.width + "x" + debugWindow.latteView.absoluteGeometry.height;
                    } else {
                        return "_,_ _x_";
                    }
                }
            }


            Text{
                text: "Draw Effects"+debugWindow.space
            }

            Text{
                text: {
                    if (debugWindow.latteView && debugWindow.latteView.effects && debugWindow.latteView.effects.drawEffects)
                        return "Yes";
                    else
                        return "No";
                }
            }

            Text{
                text: "Effects Area"+debugWindow.space
            }

            Text{
                text: {
                    if (debugWindow.latteView && debugWindow.latteView.effects && debugWindow.latteView.effects.rect) {
                        return debugWindow.latteView.effects.rect.x + ", " + debugWindow.latteView.effects.rect.y + "  " +debugWindow.latteView.effects.rect.width + "x" + debugWindow.latteView.effects.rect.height;
                    } else {
                        return "_,_ _x_";
                    }
                }
            }

            Text{
                text: "   -----------   "
            }

            Text{
                text: " -----------   "
            }

            Text{
                text: "Is Hidden (flag)"+debugWindow.space
            }

            Text{
                text: {
                    if (debugWindow.latteView && debugWindow.latteView.visibility && debugWindow.latteView.visibility.isHidden)
                        return "Yes";
                    else
                        return "No";
                }
            }

            Text{
                text: "Contains Mouse (flag)"+debugWindow.space
            }

            Text{
                text: {
                    if (debugWindow.latteView && debugWindow.latteView.visibility && debugWindow.latteView.visibility.containsMouse)
                        return "Yes";
                    else
                        return "No";
                }
            }

            Text{
                text: "Edit Mode"+debugWindow.space
            }

            Text{
                text: {
                    if (debugWindow.containmentRoot.editMode)
                        return "Yes";
                    else
                        return "No";
                }
            }

            Text{
                text: "   -----------   "
            }

            Text{
                text: " -----------   "
            }

            Text{
                text: "Location"+debugWindow.space
            }

            Text{
                text: {
                    switch(debugWindow.plasmoid.location){
                    case PlasmaCore.Types.LeftEdge:
                        return "Left Edge";
                    case PlasmaCore.Types.RightEdge:
                        return "Right Edge";
                    case PlasmaCore.Types.TopEdge:
                        return "Top Edge";
                    case PlasmaCore.Types.BottomEdge:
                        return "Bottom Edge";
                    }

                    return " <unknown> : " + debugWindow.plasmoid.location;
                }
            }

            Text{
                text: "Alignment"+debugWindow.space
            }

            Text{
                text: {
                    switch(debugWindow.plasmoid.configuration.alignment){
                    case LatteCore.types.Left:
                        return "Left";
                    case LatteCore.types.Right:
                        return "Right";
                    case LatteCore.types.Center:
                        return "Center";
                    case LatteCore.types.Top:
                        return "Top";
                    case LatteCore.types.Bottom:
                        return "Bottom";
                    case LatteCore.types.Justify:
                        return "Justify";
                    }

                    return "<unknown> : " + debugWindow.plasmoid.configuration.alignment;
                }
            }

            Text{
                text: "Visibility"+debugWindow.space
            }

            Text{
                text: {
                    if (!debugWindow.latteView || !debugWindow.latteView.visibility)
                        return "";

                    switch(debugWindow.latteView.visibility.mode){
                    case LatteCore.types.AlwaysVisible:
                        return "Always Visible";
                    case LatteCore.types.AutoHide:
                        return "Auto Hide";
                    case LatteCore.types.DodgeActive:
                        return "Dodge Active";
                    case LatteCore.types.DodgeMaximized:
                        return "Dodge Maximized";
                    case LatteCore.types.DodgeAllWindows:
                        return "Dodge All Windows";
                    case LatteCore.types.DodgeAllWindows:
                        return "Dodge All Windows";
                    case LatteCore.types.WindowsGoBelow:
                        return "Windows Go Below";
                    case LatteCore.types.WindowsCanCover:
                        return "Windows Can Cover";
                    case LatteCore.types.WindowsAlwaysCover:
                        return "Windows Always Cover";
                    case LatteCore.types.SidebarOnDemand:
                        return "OnDemand Sidebar";
                    case LatteCore.types.SidebarAutoHide:
                        return "AutoHide Sidebar";
                    case LatteCore.types.NormalWindow:
                        return "Normal Window";
                    case LatteCore.types.None:
                        return "None";
                    }

                    return "<unknown> : " + debugWindow.latteView.visibility.mode;
                }
            }

            Text{
                text: "Zoom Factor"+debugWindow.space
            }

            Text{
                text: debugWindow.parabolic.factor.zoom
            }

            Text{
                text: "   -----------   "
            }

            Text{
                text: " -----------   "
            }

            Text{
                text: "Icon Size (current)"+debugWindow.space
            }

            Text{
                text: debugWindow.metrics.iconSize
            }

            Text{
                text: "Icon Size (user)"+debugWindow.space
            }

            Text{
                text: debugWindow.plasmoid.configuration.iconSize
            }

            Text{
                text: "Icon Size (portion)"+debugWindow.space
            }

            Text{
                text: debugWindow.metrics.portionIconSize
            }

            Text{
                text: "Icon Size (auto decrease), Enabled"+debugWindow.space
            }

            Text{
                text: {
                    if (debugWindow.autosize.isActive)
                        return "Yes";
                    else
                        return "No";
                }
            }

            Text{
                text: "Icon Size (auto decrease)"+debugWindow.space
            }

            Text{
                text: debugWindow.autosize.iconSize
            }

            Text{
                text: "Length Padding (pixels)"+debugWindow.space
            }

            Text{
                text: debugWindow.metrics.padding.length
            }

            Text{
                text: "Length Margin (pixels)"+debugWindow.space
            }

            Text{
                text: debugWindow.metrics.margin.length
            }

            Text{
                text: "Head Thickness Margin"+debugWindow.space
            }

            Text{
                text: debugWindow.metrics.margin.headThickness
            }

            Text{
                text: "Tail Thickness Margin"+debugWindow.space
            }

            Text{
                text: debugWindow.metrics.margin.tailThickness
            }

            Text{
                text: "   -----------   "
            }

            Text{
                text: " -----------   "
            }

            Text{
                text: "Show Panel Background (user)"+debugWindow.space
            }

            Text{
                text: {
                    if (debugWindow.plasmoid.configuration.useThemePanel)
                        return "Yes";
                    else
                        return "No";
                }
            }

            Text{
                text: "Force Transparent Panel (auto)"+debugWindow.space
            }

            Text{
                text: {
                    if (debugWindow.containmentRoot.forceTransparentPanel)
                        return "Yes";
                    else
                        return "No";
                }
            }

            Text{
                text: "Background Length"+debugWindow.space
            }

            Text{
                text: debugWindow.background.totals.visualLength
            }

            Text{
                text: "Background Thickness(user)"+debugWindow.space
            }

            Text{
                text: debugWindow.plasmoid.configuration.panelSize + "%"
            }

            Text{
                text: "Background Thickness(auto)"+debugWindow.space
            }

            Text{
                text: debugWindow.background.totals.visualThickness
            }

            Text{
                text: "Background Opacity"+debugWindow.space
            }

            Text{
                text: (debugWindow.containmentRoot.myView.backgroundOpacity * 100) + "%"
            }

            Text{
                text: "Background Shadows Active"+debugWindow.space
            }

            Text{
                text: {
                    if (debugWindow.containmentRoot.panelShadowsActive)
                        return "Yes";
                    else
                        return "No";
                }
            }

            Text{
                text: "Background Shadow"+debugWindow.space
            }

            Text{
                text: debugWindow.background.shadows.headThickness
            }

            Text{
                text: "Background Head Thickness Padding"+debugWindow.space
            }

            Text{
                text: debugWindow.background.paddings.headThickness
            }

            Text{
                text: "   -----------   "
            }

            Text{
                text: " -----------   "
            }

            Text{
                text: "Mask - Normal Thickness"+debugWindow.space
            }

            Text{
                text: debugWindow.metrics.mask.thickness.normal
            }

            Text{
                text: "Thickness Uses Panel Size"+debugWindow.space
            }

            Text{
                text: debugWindow.background.isGreaterThanItemThickness
            }

            Text{
                text: "Draw Shadows (external)"+debugWindow.space
            }

            Text{
                text: {
                    if (debugWindow.containmentRoot.drawShadowsExternal)
                        return "Yes";
                    else
                        return "No";
                }
            }

            Text{
                text: "   -----------   "
            }

            Text{
                text: " -----------   "
            }

            Text{
                text: "Applet Hovered"+debugWindow.space
            }

            Text{
                text: "--"//layoutsContainer.hoveredIndex
            }

            Text{
                text: "In Normal State"+debugWindow.space
            }

            Text{
                text: debugWindow.visibilityManager.inNormalState
            }

            Text{
                text: "Animations Both Axis"+debugWindow.space
            }

            Text{
                text: debugWindow.animations.needBothAxis.count
            }

            Text{
                text: "Animations Only Length"+debugWindow.space
            }

            Text{
                text: debugWindow.animations.needLength.count
            }

            Text{
                text: "Animations Need Thickness"+debugWindow.space
            }

            Text{
                text: debugWindow.animations.needThickness.count
            }

            Text{
                text: "   -----------   "
            }

            Text{
                text: " -----------   "
            }

            Text{
                text: "Start Layout Shown Applets"+debugWindow.space
            }

            Text{
                text: debugWindow.layouter.startLayout.shownApplets
            }

            Text{
                text: "Start Layout Applets (with fill)"+debugWindow.space
            }

            Text{
                text: debugWindow.layouter.startLayout.fillApplets
            }

            Text{
                text: "Start Layout Size (no fill applets)"+debugWindow.space
            }

            Text{
                text: debugWindow.layouter.startLayout.sizeWithNoFillApplets+" px."
            }

            Text{
                text: "   -----------   "
            }

            Text{
                text: " -----------   "
            }

            Text{
                text: "Main Layout Shown Applets"+debugWindow.space
            }

            Text{
                text: debugWindow.layouter.mainLayout.shownApplets
            }

            Text{
                text: "Main Layout Applets (with fill)"+debugWindow.space
            }

            Text{
                text: debugWindow.layouter.mainLayout.fillApplets
            }

            Text{
                text: "Main Layout Size (no fill applets)"+debugWindow.space
            }

            Text{
                text: debugWindow.layouter.mainLayout.sizeWithNoFillApplets+" px."
            }

            Text{
                text: "   -----------   "
            }

            Text{
                text: " -----------   "
            }

            Text{
                text: "End Layout Shown Applets"+debugWindow.space
            }

            Text{
                text: debugWindow.layouter.endLayout.shownApplets
            }

            Text{
                text: "End Layout Applets (with fill)"+debugWindow.space
            }

            Text{
                text: debugWindow.layouter.endLayout.fillApplets
            }

            Text{
                text: "End Layout Size (no fill applets)"+debugWindow.space
            }

            Text{
                text: debugWindow.layouter.endLayout.sizeWithNoFillApplets+" px."
            }

            Text{
                text: "   -----------   "
            }

            Text{
                text: " -----------   "
            }

            Text{
                text: "Animations ZoomFactor" + debugWindow.space
            }

            Text{
                text: debugWindow.animations.requirements.zoomFactor
            }

            Text{
                text: "   -----------   "
            }

            Text{
                text: " -----------   "
            }

            Text{
                text: "Applets need Windows Tracking"+debugWindow.space
            }

            Text{
                text: debugWindow.indexer.clientsTrackingWindowsCount
            }

            Text{
                text: "Last Active Window Current Screen (id)"+debugWindow.space
            }

            Text{
                text: debugWindow.latteView && debugWindow.latteView.windowsTracker && debugWindow.latteView.windowsTracker.currentScreen.lastActiveWindow && debugWindow.latteView.windowsTracker.currentScreen.lastActiveWindow.isValid ?
                          debugWindow.latteView.windowsTracker.currentScreen.lastActiveWindow.winId : "--"
            }

            Text{
                text: "Last Active Window Current Screen (title)"+debugWindow.space
            }

            Text{
                text: debugWindow.latteView && debugWindow.latteView.windowsTracker && debugWindow.latteView.windowsTracker.currentScreen.lastActiveWindow && debugWindow.latteView.windowsTracker.currentScreen.lastActiveWindow.isValid ?
                          debugWindow.latteView.windowsTracker.currentScreen.lastActiveWindow.display : "--"
                elide: Text.ElideRight
            }

            Text{
                text: "Last Active Window All Screens (id)"+debugWindow.space
            }

            Text{
                text: debugWindow.latteView && debugWindow.latteView.windowsTracker && debugWindow.latteView.windowsTracker.allScreens.lastActiveWindow && debugWindow.latteView.windowsTracker.allScreens.lastActiveWindow.isValid ?
                          debugWindow.latteView.windowsTracker.allScreens.lastActiveWindow.winId : "--"
            }

            Text{
                text: "Last Active Window All Screens (title)"+debugWindow.space
            }

            Text{
                text: debugWindow.latteView && debugWindow.latteView.windowsTracker && debugWindow.latteView.windowsTracker.allScreens.lastActiveWindow && debugWindow.latteView.windowsTracker.allScreens.lastActiveWindow.isValid ?
                          debugWindow.latteView.windowsTracker.allScreens.lastActiveWindow.display : "--"
                elide: Text.ElideRight
            }
        }

    }
}
