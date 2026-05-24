/*
    SPDX-FileCopyrightText: 2016 Smith AR <audoban@openmailbox.org>
    SPDX-FileCopyrightText: 2016 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.8
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.kquickcontrolsaddons 2.0
import org.kde.plasma.plasmoid 2.0
import org.kde.kirigami 2.20 as Kirigami

import org.kde.latte.abilities.host 0.1 as AbilityHost

import org.kde.latte.core 0.2 as LatteCore
import org.kde.latte.components 1.0 as LatteComponents
import org.kde.latte.private.app 0.1 as LatteApp
import org.kde.latte.private.containment 0.1 as LatteContainment

import "abilities" as Ability
import "applet" as Applet
import "colorizer" as Colorizer
import "editmode" as EditMode
import "layouts" as Layouts
import "./background" as Background
import "./debugger" as Debugger

ContainmentItem {
    id: root
    objectName: "containmentViewLayout"

    property real panelBgOpacity: 1.0
    property bool panelCustomTransparency: false

    Timer {
        id: panelCfgSync
        interval: 300
        repeat: true
        running: true
        onTriggered: {
            var pt = plasmoid.configuration.panelTransparency
            var isDefault = (pt === -1 || pt === "-1" || pt === undefined || pt === null || pt === "" || Number(pt) >= 100)
            root.panelBgOpacity = isDefault ? 1.0 : Number(pt) / 100.0
            root.panelCustomTransparency = !isDefault
        }
    }

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft && !root.isVertical
    LayoutMirroring.childrenInherit: true

    //// BEGIN SIGNALS
    signal destroyInternalViewSplitters();
    signal emptyAreasWheel(QtObject wheel);
    signal separatorsUpdated();
    signal updateEffectsArea();
    signal updateIndexes();

    signal broadcastedToApplet(string pluginName, string action, variant value);
    //// END SIGNALS

    ////BEGIN properties
    readonly property int version: LatteCore.Environment.makeVersion(0,9,75)
    readonly property bool kirigamiLibraryIsFound: true

    property bool backgroundOnlyOnMaximized: plasmoid.configuration.backgroundOnlyOnMaximized

    readonly property bool viewIsAvailable: latteView && latteView.visibility && latteView.effects

    // Latte Dock NG intentionally supports only dock-style views.
    // KConfig enum values can be exposed as integer indexes or enum names.
    function dockStyleToIndex(styleValue) {
        return (styleValue === 1 || styleValue === "1" || styleValue === "Modern") ? 1 : 0;
    }
    readonly property int currentDockStyleIndex: dockStyleToIndex(plasmoid.configuration.dockStyle)
    readonly property bool isModernDockStyle: currentDockStyleIndex === 1
    property bool modernAlignmentNormalizationGuard: false

    function normalizedAlignmentForModern(alignmentValue) {
        return alignmentValue;
    }

    function enforceModernAlignmentCompatibility() {
        if (!root.isModernDockStyle || root.modernAlignmentNormalizationGuard) {
            return false;
        }

        var normalizedConfigAlignment = root.normalizedAlignmentForModern(plasmoid.configuration.alignment);
        var normalizedViewAlignment = latteView ? root.normalizedAlignmentForModern(latteView.alignment) : normalizedConfigAlignment;
        var changed = false;

        root.modernAlignmentNormalizationGuard = true;

        if (plasmoid.configuration.alignment !== normalizedConfigAlignment) {
            plasmoid.configuration.alignment = normalizedConfigAlignment;
            changed = true;
        }

        if (latteView && latteView.alignment !== normalizedViewAlignment) {
            latteView.alignment = normalizedViewAlignment;
            changed = true;
        }

        root.modernAlignmentNormalizationGuard = false;
        return changed;
    }

    property bool blurEnabled: !panelCustomTransparency && plasmoid.configuration.blurEnabled && (!forceTransparentPanel || forcePanelForBusyBackground)

    readonly property bool inDraggingOverAppletOrOutOfContainment: latteView && latteView.containsDrag && !backDropArea.containsDrag

    readonly property Item dragInfo: Item {
        property bool entered: backDropArea.dragInfo.entered
        property bool isTask: backDropArea.dragInfo.isTask
        property bool isPlasmoid: backDropArea.dragInfo.isPlasmoid
        property bool isSeparator: backDropArea.dragInfo.isSeparator
        property bool isLatteTasks: backDropArea.dragInfo.isLatteTasks
        property bool onlyLaunchers: backDropArea.dragInfo.onlyLaunchers
    }

    property bool containsOnlyPlasmaTasks: latteView ? latteView.extendedInterface.hasPlasmaTasks && !latteView.extendedInterface.hasLatteTasks : false
    property bool dockContainsMouse: latteView && latteView.visibility ? latteView.visibility.containsMouse : false

    property bool disablePanelShadowMaximized: plasmoid.configuration.disablePanelShadowForMaximized && LatteCore.WindowSystem.compositingActive
    property bool drawShadowsExternal: false

    property bool editMode: plasmoid.userConfiguring
    property bool windowIsTouching: latteView && latteView.windowsTracker
                                    && (latteView.windowsTracker.currentScreen.activeWindowTouching
                                        || latteView.windowsTracker.currentScreen.activeWindowTouchingEdge
                                        || hasExpandedApplet)

    property bool floatingInternalGapIsForced: plasmoid.configuration.floatingInternalGapIsForced

    property bool hasFloatingGapInputEventsDisabled: root.screenEdgeMarginEnabled
                                                     && !root.inConfigureAppletsMode
                                                     && !parabolic.isEnabled
                                                     && !root.floatingInternalGapIsForced

    property bool forceSolidPanel: (latteView && latteView.visibility
                                    && LatteCore.WindowSystem.compositingActive
                                    && !inConfigureAppletsMode
                                    && userShowPanelBackground
                                    && ( (plasmoid.configuration.solidBackgroundForMaximized
                                          && !(hasExpandedApplet && !plasmaBackgroundForPopups)
                                          && (latteView.windowsTracker.currentScreen.existsWindowTouching
                                              || latteView.windowsTracker.currentScreen.existsWindowTouchingEdge))
                                        || (hasExpandedApplet && plasmaBackgroundForPopups) ))
                                   || !LatteCore.WindowSystem.compositingActive

    property bool forceTransparentPanel: root.backgroundOnlyOnMaximized
                                         && latteView && latteView.visibility
                                         && LatteCore.WindowSystem.compositingActive
                                         && !inConfigureAppletsMode
                                         && !forceSolidPanel
                                         && !(latteView.windowsTracker.currentScreen.existsWindowTouching
                                              || latteView.windowsTracker.currentScreen.existsWindowTouchingEdge)
                                         && !(windowColors === LatteContainment.types.ActiveWindowColors && selectedWindowsTracker.existsWindowActive)

    property bool forcePanelForBusyBackground: userShowPanelBackground && (normalBusyForTouchingBusyVerticalView
                                                                           || ( root.forceTransparentPanel
                                                                               && colorizerManager.backgroundIsBusy
                                                                               && root.themeColors === LatteContainment.types.SmartThemeColors))

    property bool normalBusyForTouchingBusyVerticalView: (latteView && latteView.windowsTracker /*is touching a vertical view that is in busy state and the user prefers isBusy transparency*/
                                                          && latteView.windowsTracker.currentScreen.isTouchingBusyVerticalView
                                                          && plasmoid.configuration.backgroundOnlyOnMaximized)

    property bool appletIsDragged: root.dragOverlay && root.dragOverlay.pressed
    property bool hideThickScreenGap: false /*set through binding*/
    property bool hideLengthScreenGaps: false /*set through binding*/

    property bool mirrorScreenGap: screenEdgeMarginEnabled
                                   && plasmoid.configuration.floatingGapIsMirrored
                                   && latteView.visibility.mode === LatteCore.types.AlwaysVisible



    property int themeColors: plasmoid.configuration.themeColors
    property int windowColors: plasmoid.configuration.windowColors

    property bool colorizerEnabled: themeColors !== LatteContainment.types.PlasmaThemeColors || windowColors !== LatteContainment.types.NoneWindowColors

    property bool plasmaBackgroundForPopups: plasmoid.configuration.plasmaBackgroundForPopups

    readonly property bool hasExpandedApplet: latteView && latteView.extendedInterface.hasExpandedApplet;
    readonly property bool hasUserSpecifiedBackground: (latteView && latteView.layout && latteView.layout.background.startsWith("/")) ?
                                                           true : false

    readonly property bool inConfigureAppletsMode: universalSettings && universalSettings.inConfigureAppletsMode

    property bool closeActiveWindowEnabled: plasmoid.configuration.closeActiveWindowEnabled
    property bool dragActiveWindowEnabled: plasmoid.configuration.dragActiveWindowEnabled
    property bool immutable: plasmoid.immutable
    property bool inFullJustify: (plasmoid.configuration.alignment === LatteCore.types.Justify) && (maxLengthPerCentage===100)
    property bool inStartup: true

    // Use edge location as the source of truth during relocation; formFactor can
    // be temporarily stale while the view is moving between edges.
    readonly property bool _edgeIsVertical: plasmoid.location === PlasmaCore.Types.LeftEdge
                                            || plasmoid.location === PlasmaCore.Types.RightEdge
    readonly property bool _edgeIsHorizontal: plasmoid.location === PlasmaCore.Types.TopEdge
                                              || plasmoid.location === PlasmaCore.Types.BottomEdge
    property bool isHorizontal: _edgeIsHorizontal ? true
                                                  : (_edgeIsVertical ? false : (plasmoid.formFactor === PlasmaCore.Types.Horizontal))
    property bool isVertical: !isHorizontal

    property bool mouseWheelActions: plasmoid.configuration.mouseWheelActions
    property bool onlyAddingStarup: true //is used for the initialization phase in startup where there aren't removals, this variable provides a way to grow icon size

    //FIXME: possibly this is going to be the default behavior, this user choice
    //has been dropped from the Dock Configuration Window
    //property bool smallAutomaticIconJumps: plasmoid.configuration.smallAutomaticIconJumps
    property bool smallAutomaticIconJumps: true

    property bool userShowPanelBackground: LatteCore.WindowSystem.compositingActive ? plasmoid.configuration.useThemePanel : true
    property bool useThemePanel: noApplets === 0 || !LatteCore.WindowSystem.compositingActive ?
                                     true : (plasmoid.configuration.useThemePanel || plasmoid.configuration.solidBackgroundForMaximized)

    readonly property int minAppletLengthInConfigure: 16
    readonly property int maxJustifySplitterSize: 64

    property bool maximizeWhenMaximized: plasmoid.configuration.maximizeWhenMaximized;
    property real minLengthPerCentage: plasmoid.configuration.minLength
    property real maxLengthPerCentage: hideLengthScreenGaps ? 100 : plasmoid.configuration.maxLength

    property int minLength: {
        if (myView.alignment === LatteCore.types.Justify) {
            return maxLength;
        }

        if (root.isHorizontal) {
            return width * (minLengthPerCentage/100)
        } else {
            return height * (minLengthPerCentage/100)
        }
    }

    property int maxLength: {
        const maximize = maximizeWhenMaximized && latteView.windowsTracker.currentScreen.existsWindowMaximized;
        if (root.isHorizontal) {
            return maximize ? width : width * (maxLengthPerCentage/100)
        } else {
            return maximize ? height : height * (maxLengthPerCentage/100)
        }
    }

    property int scrollAction: plasmoid.configuration.scrollAction

    property bool panelOutline: plasmoid.configuration.panelOutline
    property int panelEdgeSpacing: Math.max(background.lengthMargins, 1.5*myView.itemShadow.size)

    property bool backgroundShadowsInRegularStateEnabled: LatteCore.WindowSystem.compositingActive
                                                          && userShowPanelBackground
                                                          && plasmoid.configuration.panelShadows

    property bool panelShadowsActive: {
        if (!userShowPanelBackground) {
            return false;
        }

        if (inConfigureAppletsMode) {
            return plasmoid.configuration.panelShadows;
        }

        var forcedNoShadows = (plasmoid.configuration.panelShadows && disablePanelShadowMaximized
                               && latteView && latteView.windowsTracker && latteView.windowsTracker.currentScreen.activeWindowMaximized);

        if (forcedNoShadows) {
            return false;
        }

        var transparencyCheck = (blurEnabled || (!blurEnabled && background.currentOpacity>20));

        //! Draw shadows for isBusy state only when current background opacity is greater than 10%
        if (plasmoid.configuration.panelShadows && root.forcePanelForBusyBackground && transparencyCheck) {
            return true;
        }

        if (( (plasmoid.configuration.panelShadows && !root.backgroundOnlyOnMaximized)
             || (plasmoid.configuration.panelShadows && root.backgroundOnlyOnMaximized && !root.forceTransparentPanel))
                && !forcedNoShadows) {
            return true;
        }

        if (hasExpandedApplet && plasmaBackgroundForPopups) {
            return true;
        }

        return false;
    }

    property int offset: {
        if (root.isHorizontal) {
            return width * (plasmoid.configuration.offset/100);
        } else {
            return height * (plasmoid.configuration.offset/100)
        }
    }

    property int editShadow: {
        if (!LatteCore.WindowSystem.compositingActive) {
            return 0;
        } else if (latteView && latteView.screenGeometry) {
            return latteView.screenGeometry.height/90;
        } else {
            return 7;
        }
    }

    property bool screenEdgeMarginEnabled: plasmoid.configuration.screenEdgeMargin >= 0

    property int widthMargins: root.isVertical ? metrics.totals.thicknessEdges : metrics.totals.lengthEdges
    property int heightMargins: root.isHorizontal ? metrics.totals.thicknessEdges : metrics.totals.lengthEdges

    property var iconsArray: [16, 22, 32, 48, 64, 96, 128, 256]

    property Item dragOverlay: _dragOverlay
    property Item toolBox

    readonly property alias animations: _animations
    readonly property alias autosize: _autosize
    readonly property alias background: _background
    readonly property alias debug: _debug
    readonly property alias environment: _environment
    readonly property alias indexer: _indexer
    readonly property alias indicators: _indicators
    readonly property alias layouter: _layouter
    readonly property alias launchers: _launchers
    readonly property alias metrics: _metrics
    readonly property alias myView: _myView
    readonly property alias parabolic: _parabolic
    readonly property alias thinTooltip: _thinTooltip
    readonly property alias userRequests: _userRequests

    readonly property alias maskManager: visibilityManager
    readonly property alias layoutsContainerItem: layoutsContainer

    readonly property alias latteView: _interfaces.view
    readonly property alias layoutsManager: _interfaces.layoutsManager
    readonly property alias shortcutsEngine: _interfaces.globalShortcuts
    readonly property alias themeExtended: _interfaces.themeExtended
    readonly property alias universalSettings: _interfaces.universalSettings

    readonly property QtObject selectedWindowsTracker: {
        if (latteView && latteView.windowsTracker) {
            switch(plasmoid.configuration.activeWindowFilter) {
            case LatteContainment.types.ActiveInCurrentScreen:
                return latteView.windowsTracker.currentScreen;
            case LatteContainment.types.ActiveFromAllScreens:
                return latteView.windowsTracker.allScreens;
            }
        }

        return null;
    }

    Plasmoid.backgroundHints: PlasmaCore.Types.NoBackground

    //// BEGIN properties in functions
    property int noApplets: {
        var count1 = 0;
        var count2 = 0;

        count1 = layoutsContainer.mainLayout.children.length;
        var tempLength = layoutsContainer.mainLayout.children.length;

        for (var i=tempLength-1; i>=0; --i) {
            var applet = layoutsContainer.mainLayout.children[i];
            if (applet && (applet === dndSpacer ||  applet.isInternalViewSplitter))
                count1--;
        }

        count2 = layoutsContainer.endLayout.children.length;
        tempLength = layoutsContainer.endLayout.children.length;

        for (var i=tempLength-1; i>=0; --i) {
            var applet = layoutsContainer.endLayout.children[i];
            if (applet && (applet === dndSpacer || applet.isInternalViewSplitter))
                count2--;
        }

        return (count1 + count2);
    }

    ///The index of user's current icon size
    property int currentIconIndex:{
        for(var i=iconsArray.length-1; i>=0; --i){
            if(iconsArray[i] === metrics.iconSize){
                return i;
            }
        }
        return 3;
    }

    //// END properties in functions

    ////////////////END properties

    //////////////START OF BINDINGS

    //! Wait until the mouse leaves the view
    Binding {
        target: root
        property: "hideThickScreenGap"
        when: !(plasmoid.configuration.floatingGapHidingWaitsMouse && dockContainsMouse)
        value: screenEdgeMarginEnabled
               && plasmoid.configuration.hideFloatingGapForMaximized
               && latteView && latteView.windowsTracker
               && latteView.windowsTracker.currentScreen.existsWindowMaximized
    }

    //! Binding is needed in order for hideLengthScreenGaps to be activated or not only after
    //! View sliding in/out has finished. This way the animation is smoother.
    Binding{
        target: root
        property: "hideLengthScreenGaps"
        when: latteView && latteView.positioner && latteView.visibility
              && !(plasmoid.configuration.floatingGapHidingWaitsMouse && dockContainsMouse)
        value: (hideThickScreenGap
                && (latteView.visibility.mode === LatteCore.types.AlwaysVisible
                    || latteView.visibility.mode === LatteCore.types.WindowsGoBelow)
                && (plasmoid.configuration.alignment === LatteCore.types.Justify)
                && plasmoid.configuration.maxLength>85)
    }

    //////////////END OF BINDINGS


    //////////////START OF CONNECTIONS
    onEditModeChanged: {
        if (!editMode) {
            layouter.updateSizeForAppletsInFill();
        }

        //! This is used in case the dndspacer has been left behind
        //! e.g. the user drops a folder and a context menu is appearing
        //! but the user decides to not make a choice for the applet type
        if (dndSpacer.parent !== root) {
            dndSpacer.parent = root;
        }

        //Block Hiding events
        if (editMode) {
            latteView.visibility.addBlockHidingEvent("main[qml]::inEditMode()");
        } else {
            latteView.visibility.removeBlockHidingEvent("main[qml]::inEditMode()");
        }
    }

    onInConfigureAppletsModeChanged: {
        updateIndexes();
    }

    //! It is used only when the user chooses different alignment types and not during startup
    Connections {
        target: latteView ? latteView : null
        function onAlignmentChanged() {
            if (root.inStartup) {
                return;
            }

            if (root.modernAlignmentNormalizationGuard) {
                return;
            }

            if (latteView.alignment === LatteCore.types.NoneAlignment) {
                return;
            }

            if (root.isModernDockStyle) {
                var effectiveAlignment = root.normalizedAlignmentForModern(latteView.alignment);

                if (latteView.alignment !== effectiveAlignment) {
                    root.modernAlignmentNormalizationGuard = true;
                    latteView.alignment = effectiveAlignment;
                    root.modernAlignmentNormalizationGuard = false;
                    return;
                }

                layouter.appletsInParentChange = true;

                if (effectiveAlignment === LatteCore.types.Justify) {
                    if (parabolic) {
                        parabolic.sglClearZoom();
                    }

                    // Modern style keeps a single-main applets flow. The legacy
                    // three-layout split used for classic Justify can drift to
                    // side-only placement under Qt6 and ends up left/right aligned.
                    fastLayoutManager.joinLayoutsToMainLayout();
                    root.forceModernAppletsToMainLayout();
                    root.destroyInternalViewSplitters();
                    root.forceDestroyInternalSplittersNow();
                    root.resetModernParabolicOffsets();
                } else {
                    fastLayoutManager.joinLayoutsToMainLayout();
                    root.forceModernAppletsToMainLayout();

                    // Keep this unconditional for modern mode. Depending on the
                    // configuration signal order, previousalignment can already
                    // be updated when this handler runs.
                    root.destroyInternalViewSplitters();
                    root.forceDestroyInternalSplittersNow();
                    root.resetModernParabolicOffsets();

                    // Catch delayed splitter creation/connection ordering.
                    Qt.callLater(function() {
                        root.forceDestroyInternalSplittersNow();
                        root.resetModernParabolicOffsets();
                    });
                }

                layouter.appletsInParentChange = false;

                root.updateIndexes();
                plasmoid.configuration.alignment = effectiveAlignment;
                fastLayoutManager.save();

                if (effectiveAlignment === LatteCore.types.Justify) {
                    root.forceModernAppletsToMainLayout();
                    layouter.updateSizeForAppletsInFill();
                    root.forceModernAppletsToMainLayout();
                } else {
                    root.forceModernAppletsToMainLayout();
                    layouter.updateSizeForAppletsInFill();
                    root.forceModernAppletsToMainLayout();
                }
                Qt.callLater(function() {
                    root.forceModernAppletsToMainLayout();
                    root.resetModernParabolicOffsets();
                    layouter.updateSizeForAppletsInFill();
                });
                return;
            }

            layouter.appletsInParentChange = true;

            if (latteView.alignment === LatteCore.types.Justify) {
                fastLayoutManager.joinLayoutsToMainLayout();
                root.destroyInternalViewSplitters();
                root.forceDestroyInternalSplittersNow();
            } else {
                fastLayoutManager.joinLayoutsToMainLayout();
                root.destroyInternalViewSplitters();
                root.forceDestroyInternalSplittersNow();
            }

            layouter.appletsInParentChange = false;

            root.updateIndexes();
            plasmoid.configuration.alignment = latteView.alignment;
            fastLayoutManager.save();
            layouter.updateSizeForAppletsInFill();
        }
    }

    onCurrentDockStyleIndexChanged: {
        if (root.inStartup) {
            return;
        }

        if (!latteView || root.modernAlignmentNormalizationGuard) {
            return;
        }

        layouter.appletsInParentChange = true;

        if (root.isModernDockStyle) {
            root.enforceModernAlignmentCompatibility();
            fastLayoutManager.joinLayoutsToMainLayout();
            root.forceModernAppletsToMainLayout();
            root.destroyInternalViewSplitters();
            root.forceDestroyInternalSplittersNow();
            root.resetModernParabolicOffsets();
        } else if (latteView.alignment === LatteCore.types.Justify) {
            fastLayoutManager.joinLayoutsToMainLayout();
            root.destroyInternalViewSplitters();
            root.forceDestroyInternalSplittersNow();
        } else {
            fastLayoutManager.joinLayoutsToMainLayout();
            root.destroyInternalViewSplitters();
            root.forceDestroyInternalSplittersNow();
        }

        layouter.appletsInParentChange = false;

        root.updateIndexes();
        fastLayoutManager.save();
        layouter.updateSizeForAppletsInFill();
    }

    onLatteViewChanged: {
        if (latteView) {
            if (latteView.positioner) {
                latteView.positioner.hidingForRelocationStarted.connect(visibilityManager.slotHideDockDuringLocationChange);
                latteView.positioner.showingAfterRelocationFinished.connect(visibilityManager.slotShowDockAfterLocationChange);
            }

            if (latteView.visibility) {
                latteView.visibility.onContainsMouseChanged.connect(visibilityManager.slotContainsMouseChanged);
                latteView.visibility.onMustBeHide.connect(visibilityManager.slotMustBeHide);
                latteView.visibility.onMustBeShown.connect(visibilityManager.slotMustBeShown);
            }
        }
    }

    Connections {
        target: latteView
        function onPositionerChanged() {
            if (latteView.positioner) {
                latteView.positioner.hidingForRelocationStarted.connect(visibilityManager.slotHideDockDuringLocationChange);
                latteView.positioner.showingAfterRelocationFinished.connect(visibilityManager.slotShowDockAfterLocationChange);
                repairAppletContainersTimer.restart();
            }
        }

        function onVisibilityChanged() {
            if (latteView.visibility) {
                latteView.visibility.onContainsMouseChanged.connect(visibilityManager.slotContainsMouseChanged);
                latteView.visibility.onMustBeHide.connect(visibilityManager.slotMustBeHide);
                latteView.visibility.onMustBeShown.connect(visibilityManager.slotMustBeShown);
            }
        }
    }

    Connections {
        target: plasmoid
        function onLocationChanged() {
            repairAppletContainersTimer.restart();
        }
        function onFormFactorChanged() {
            repairAppletContainersTimer.restart();
        }
    }

    Connections {
        target: latteView && latteView.positioner ? latteView.positioner : null
        function onShowingAfterRelocationFinished() {
            repairAppletContainersTimer.restart();
        }
    }


    onMaxLengthChanged: {
        layouter.updateSizeForAppletsInFill();
    }

    onToolBoxChanged: {
        if (toolBox) {
            toolBox.visible = false;
        }
    }

    onIsVerticalChanged: {
        if (isVertical) {
            if (plasmoid.configuration.alignment === LatteCore.types.Left)
                plasmoid.configuration.alignment = LatteCore.types.Top;
            else if (plasmoid.configuration.alignment === LatteCore.types.Right)
                plasmoid.configuration.alignment = LatteCore.types.Bottom;
        } else {
            if (plasmoid.configuration.alignment === LatteCore.types.Top)
                plasmoid.configuration.alignment = LatteCore.types.Left;
            else if (plasmoid.configuration.alignment === LatteCore.types.Bottom)
                plasmoid.configuration.alignment = LatteCore.types.Right;
        }
    }

    function configureAction() {
        if (typeof plasmoid.action === "function") {
            return plasmoid.action("configure");
        }

        if (typeof Plasmoid.internalAction === "function") {
            return Plasmoid.internalAction("configure");
        }

        if (Plasmoid.contextualActions) {
            for (var i = 0; i < Plasmoid.contextualActions.length; ++i) {
                var candidate = Plasmoid.contextualActions[i];
                if (!candidate) {
                    continue;
                }

                if (candidate.objectName === "configure" || candidate.name === "configure") {
                    return candidate;
                }
            }
        }

        return null;
    }

    Component.onCompleted: {
        upgrader_v010_alignment();

        root.enforceModernAlignmentCompatibility();

        fastLayoutManager.restore();

        universalSettings.inConfigureAppletsMode = false;

        initTimer.start();

        var action = configureAction();
        if (action) {
            action.visible = !plasmoid.immutable;
            action.enabled = !plasmoid.immutable;
        }
    }

    Rectangle {
        id: editModeBtn
        z: 999
        visible: !root.immutable && !root.inConfigureAppletsMode && !root.editMode
        width: 24
        height: 24
        radius: 12
        color: Qt.rgba(0,0,0,0.3)
        border.color: Qt.rgba(1,1,1,0.4)
        border.width: 1
        opacity: editModeBtnMouse.containsMouse ? 0.9 : 0.35

        Behavior on opacity { NumberAnimation { duration: 200 } }

        state: {
            if (plasmoid.location === PlasmaCore.Types.BottomEdge) return "bottom"
            if (plasmoid.location === PlasmaCore.Types.TopEdge) return "top"
            if (plasmoid.location === PlasmaCore.Types.LeftEdge) return "left"
            return "right"
        }

        states: [
            State {
                name: "bottom"
                AnchorChanges { target: editModeBtn; anchors { top: undefined; bottom: parent.bottom; left: undefined; right: parent.right; horizontalCenter: undefined; verticalCenter: undefined } }
                PropertyChanges { target: editModeBtn; anchors { rightMargin: 4; bottomMargin: 4 } }
            },
            State {
                name: "top"
                AnchorChanges { target: editModeBtn; anchors { top: parent.top; bottom: undefined; left: undefined; right: parent.right; horizontalCenter: undefined; verticalCenter: undefined } }
                PropertyChanges { target: editModeBtn; anchors { rightMargin: 4; topMargin: 4 } }
            },
            State {
                name: "left"
                AnchorChanges { target: editModeBtn; anchors { top: undefined; bottom: parent.bottom; left: parent.left; right: undefined; horizontalCenter: undefined; verticalCenter: undefined } }
                PropertyChanges { target: editModeBtn; anchors { leftMargin: 4; bottomMargin: 4 } }
            },
            State {
                name: "right"
                AnchorChanges { target: editModeBtn; anchors { top: undefined; bottom: parent.bottom; left: undefined; right: parent.right; horizontalCenter: undefined; verticalCenter: undefined } }
                PropertyChanges { target: editModeBtn; anchors { rightMargin: 4; bottomMargin: 4 } }
            }
        ]

        Kirigami.Icon {
            anchors.centerIn: parent
            width: 14; height: 14
            source: "document-edit"
        }

        MouseArea {
            id: editModeBtnMouse
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                universalSettings.inConfigureAppletsMode = true;
            }
        }
    }

    Rectangle {
        id: editModeExitBtn
        z: 999
        visible: root.inConfigureAppletsMode
        width: 24
        height: 24
        radius: 12
        color: Qt.rgba(0.6,0,0,0.3)
        border.color: Qt.rgba(1,1,1,0.5)
        border.width: 1
        opacity: editModeExitBtnMouse.containsMouse ? 0.9 : 0.5

        Behavior on opacity { NumberAnimation { duration: 200 } }

        state: editModeBtn.state

        states: [
            State {
                name: "bottom"
                AnchorChanges { target: editModeExitBtn; anchors { top: undefined; bottom: parent.bottom; left: undefined; right: parent.right; horizontalCenter: undefined; verticalCenter: undefined } }
                PropertyChanges { target: editModeExitBtn; anchors { rightMargin: 4; bottomMargin: 4 } }
            },
            State {
                name: "top"
                AnchorChanges { target: editModeExitBtn; anchors { top: parent.top; bottom: undefined; left: undefined; right: parent.right; horizontalCenter: undefined; verticalCenter: undefined } }
                PropertyChanges { target: editModeExitBtn; anchors { rightMargin: 4; topMargin: 4 } }
            },
            State {
                name: "left"
                AnchorChanges { target: editModeExitBtn; anchors { top: undefined; bottom: parent.bottom; left: parent.left; right: undefined; horizontalCenter: undefined; verticalCenter: undefined } }
                PropertyChanges { target: editModeExitBtn; anchors { leftMargin: 4; bottomMargin: 4 } }
            },
            State {
                name: "right"
                AnchorChanges { target: editModeExitBtn; anchors { top: undefined; bottom: parent.bottom; left: undefined; right: parent.right; horizontalCenter: undefined; verticalCenter: undefined } }
                PropertyChanges { target: editModeExitBtn; anchors { rightMargin: 4; bottomMargin: 4 } }
            }
        ]

        Kirigami.Icon {
            anchors.centerIn: parent
            width: 14; height: 14
            source: "dialog-close"
        }

        MouseArea {
            id: editModeExitBtnMouse
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                universalSettings.inConfigureAppletsMode = false;
            }
        }
    }

    Component.onDestruction: {
        layouter.appletsInParentChange = true;
        fastLayoutManager.save();

        if (latteView) {
            if (latteView.positioner) {
                latteView.positioner.hidingForRelocationStarted.disconnect(visibilityManager.slotHideDockDuringLocationChange);
                latteView.positioner.showingAfterRelocationFinished.disconnect(visibilityManager.slotShowDockAfterLocationChange);
            }

            if (latteView.visibility) {
                latteView.visibility.onContainsMouseChanged.disconnect(visibilityManager.slotContainsMouseChanged);
                latteView.visibility.onMustBeHide.disconnect(visibilityManager.slotMustBeHide);
                latteView.visibility.onMustBeShown.disconnect(visibilityManager.slotMustBeShown);
            }
        }
    }

    Containment.onAppletAdded: function(applet, x, y) {
        if (fastLayoutManager.isMasqueradedIndex(x, y)) {
            var index = fastLayoutManager.masquearadedIndex(x, y);
            fastLayoutManager.addAppletItem(applet, index);
        } else {
            fastLayoutManager.addAppletItem(applet, x, y);
        }

        runtimeAppletRepairTimer.schedule();
    }

    Containment.onAppletRemoved: function(applet) { fastLayoutManager.removeAppletItem(applet); }

    Plasmoid.onImmutableChanged: {
        var action = configureAction();
        if (action) {
            action.visible = !plasmoid.immutable;
            action.enabled = !plasmoid.immutable;
        }
    }
    //////////////END OF CONNECTIONS

    //////////////START OF FUNCTIONS
    function createAppletItem(applet) {
        var appletContainer = appletItemComponent.createObject(dndSpacer.parent);
        appletContainer.backendAppletRef = applet;
        appletContainer.sourceAppletPluginName = appletPluginName(applet);

        if (!initAppletContainer(appletContainer, applet)) {
            // The applet's QML graphic object may not be ready yet at startup.
            // Defer with a Timer and retry; if it still fails after a few
            // attempts give up silently to avoid log noise.
            var retryCount = 0;
            var retryTimer = Qt.createQmlObject(
                'import QtQuick 2.0; Timer { interval: 50; repeat: true }',
                appletContainer);
            retryTimer.triggered.connect(function() {
                retryCount++;
                if (initAppletContainer(appletContainer, applet)) {
                    retryTimer.stop();
                    retryTimer.destroy();
                    appletContainer.visible = Qt.binding(function() {
                        return appletContainerShouldBeVisible(appletContainer);
                    });
                } else if (retryCount >= 80) { // ~4s
                    retryTimer.stop();
                    retryTimer.destroy();
                    appletContainer.destroy();
                }
            });
            retryTimer.start();
            return appletContainer;
        }

        // don't show applet if it chooses to be hidden but still make it  accessible in the panelcontroller
        appletContainer.visible = Qt.binding(function() {
            return appletContainerShouldBeVisible(appletContainer);
        });
        return appletContainer;
    }

    function appletContainerShouldBeVisible(appletContainer) {
        return ((appletContainer.applet
                 && (appletContainer.applet.status !== PlasmaCore.Types.HiddenStatus
                     || appletContainer.keepVisibleOnHiddenStatus))
                || (!plasmoid.immutable && root.inConfigureAppletsMode))
                && !appletContainer.isHidden;
    }

    function appletPluginName(applet) {
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
            var backendName = appletPluginName(applet.applet);
            if (backendName !== "") {
                return backendName;
            }
        }

        if (applet.plasmoid !== undefined && applet.plasmoid && applet.plasmoid !== applet) {
            var plasmoidName = appletPluginName(applet.plasmoid);
            if (plasmoidName !== "") {
                return plasmoidName;
            }
        }

        return "";
    }

    function resolveAppletItem(applet) {
        if (!applet) {
            return null;
        }

        if (applet.anchors !== undefined) {
            return applet;
        }

        if (fastLayoutManager && typeof fastLayoutManager.resolveAppletQuickItem === "function") {
            var resolvedAppletItem = fastLayoutManager.resolveAppletQuickItem(applet);
            if (resolvedAppletItem && resolvedAppletItem.anchors !== undefined) {
                return resolvedAppletItem;
            }
        }

        if (typeof Containment.itemFor === "function") {
            var containmentApplet = Containment.itemFor(applet);
            if (containmentApplet) {
                return containmentApplet;
            }
        }

        if (typeof Plasmoid.itemFor === "function") {
            var containmentAppletItem = Plasmoid.itemFor(applet);
            if (containmentAppletItem) {
                return containmentAppletItem;
            }
        }

        if (applet.hasOwnProperty("item") && applet.item) {
            return applet.item;
        }

        if (applet.hasOwnProperty("_plasma_graphicObject") && applet._plasma_graphicObject) {
            return applet._plasma_graphicObject;
        }

        return applet;
    }

    function initAppletContainer(appletContainer, applet) {
        appletContainer.backendAppletRef = applet;
        appletContainer.sourceAppletPluginName = appletPluginName(applet);

        var appletItem = resolveAppletItem(applet);
        if (!appletItem || appletItem.anchors === undefined) {
            return false;
        }

        appletContainer.applet = appletItem;
        appletItem.parent = appletContainer.appletWrapper;
        appletItem.anchors.fill = appletContainer.appletWrapper;
        appletItem.visible = true;
        return true;
    }

    function createJustifySplitter() {
        var splitter = appletItemComponent.createObject(root);
        splitter.internalSplitterId = internalViewSplittersCount()+1;
        splitter.visible = true;
        return splitter;
    }

    function forceDestroyInternalSplittersNow() {
        var layouts = [layoutsContainer.startLayout, layoutsContainer.mainLayout, layoutsContainer.endLayout];

        for (var li = 0; li < layouts.length; ++li) {
            var layout = layouts[li];
            if (!layout) {
                continue;
            }

            for (var i = layout.children.length - 1; i >= 0; --i) {
                var item = layout.children[i];
                if (item && item.isInternalViewSplitter) {
                    item.destroy();
                }
            }
        }
    }

    function resetModernParabolicOffsets() {
        if (layoutsContainer && layoutsContainer.mainLayout) {
            if (layoutsContainer.mainLayout.startParabolicSpacer) {
                layoutsContainer.mainLayout.startParabolicSpacer.length = 0;
            }

            if (layoutsContainer.mainLayout.endParabolicSpacer) {
                layoutsContainer.mainLayout.endParabolicSpacer.length = 0;
            }
        }

        if (parabolic) {
            parabolic.setCurrentParabolicItem(null);
            parabolic.setCurrentParabolicItemIndex(-1);
            parabolic.setDirectRenderingEnabled(false);
            parabolic.sglClearZoom();
        }
    }

    function forceModernAppletsToMainLayout() {
        if (!root.isModernDockStyle || !layoutsContainer || !layoutsContainer.mainLayout) {
            return;
        }

        var startLayout = layoutsContainer.startLayout;
        var mainLayout = layoutsContainer.mainLayout;
        var endLayout = layoutsContainer.endLayout;

        if (!startLayout || !endLayout) {
            return;
        }

        var appletItems = [];
        var layouts = [startLayout, mainLayout, endLayout];

        for (var li = 0; li < layouts.length; ++li) {
            var layout = layouts[li];

            for (var i = 0; i < layout.children.length; ++i) {
                var child = layout.children[i];
                if (!child || child === dndSpacer) {
                    continue;
                }

                if (child.isInternalViewSplitter === true || child.isParabolicEdgeSpacer === true) {
                    continue;
                }

                if (appletItems.indexOf(child) === -1) {
                    appletItems.push(child);
                }
            }
        }

        appletItems.sort(function(a, b) {
            var apos = a.mapToItem(root, 0, 0);
            var bpos = b.mapToItem(root, 0, 0);
            return root.isVertical ? (apos.y - bpos.y) : (apos.x - bpos.x);
        });

        for (var j = 0; j < appletItems.length; ++j) {
            appletItems[j].parent = mainLayout;
        }

        root.forceDestroyInternalSplittersNow();

        var startSpacer = mainLayout.startParabolicSpacer;
        var endSpacer = mainLayout.endParabolicSpacer;

        if (startSpacer) {
            startSpacer.parent = mainLayout;
        }

        if (endSpacer) {
            endSpacer.parent = mainLayout;
        }

        if (appletItems.length > 0) {
            if (startSpacer) {
                if (fastLayoutManager && typeof fastLayoutManager.insertBefore === "function") {
                    fastLayoutManager.insertBefore(appletItems[0], startSpacer);
                }
            }

            if (endSpacer) {
                if (fastLayoutManager && typeof fastLayoutManager.insertAfter === "function") {
                    fastLayoutManager.insertAfter(appletItems[appletItems.length - 1], endSpacer);
                }
            }
        }
    }

    //! it is used in order to check the right click position
    //! the only way to take into account the visual appearance
    //! of the applet (including its spacers)
    function appletContainsPos(appletId, pos){
        for (var i = 0; i < layoutsContainer.startLayout.children.length; ++i) {
            var child = layoutsContainer.startLayout.children[i];

            if (child && child.applet && child.applet.id === appletId && child.containsPos(pos))
                return true;
        }

        for (var i = 0; i < layoutsContainer.mainLayout.children.length; ++i) {
            var child = layoutsContainer.mainLayout.children[i];

            if (child && child.applet && child.applet.id === appletId && child.containsPos(pos))
                return true;
        }

        for (var i = 0; i < layoutsContainer.endLayout.children.length; ++i) {
            var child = layoutsContainer.endLayout.children[i];

            if (child && child.applet && child.applet.id === appletId && child.containsPos(pos))
                return true;
        }

        return false;
    }

    function internalViewSplittersCount(){
        var splitters = 0;
        for (var container in layoutsContainer.startLayout.children) {
            var item = layoutsContainer.startLayout.children[container];
            if(item && item.isInternalViewSplitter) {
                splitters = splitters + 1;
            }
        }

        for (var container in layoutsContainer.mainLayout.children) {
            var item = layoutsContainer.mainLayout.children[container];
            if(item && item.isInternalViewSplitter) {
                splitters = splitters + 1;
            }
        }

        for (var container in layoutsContainer.endLayout.children) {
            var item = layoutsContainer.endLayout.children[container];
            if(item && item.isInternalViewSplitter) {
                splitters = splitters + 1;
            }
        }

        return splitters;
    }

    function mouseInCanBeHoveredApplet(){
        var applets = layoutsContainer.startLayout.children;

        for(var i=0; i<applets.length; ++i){
            var applet = applets[i];

            if(applet && applet.containsMouse && !applet.originalAppletBehavior && applet.parabolicEffectIsSupported){
                return true;
            }
        }

        applets = layoutsContainer.mainLayout.children;

        for(var i=0; i<applets.length; ++i){
            var applet = applets[i];

            if(applet && applet.containsMouse && !applet.originalAppletBehavior && applet.parabolicEffectIsSupported){
                return true;
            }
        }

        ///check second layout also
        applets = layoutsContainer.endLayout.children;

        for(var i=0; i<applets.length; ++i){
            var applet = applets[i];

            if(applet && applet.containsMouse && !applet.originalAppletBehavior && applet.parabolicEffectIsSupported){
                return true;
            }
        }

        return false;
    }

    function sizeIsFromAutomaticMode(size){

        for(var i=iconsArray.length-1; i>=0; --i){
            if(iconsArray[i] === size){
                return true;
            }
        }

        return false;
    }

    function upgrader_v010_alignment() {
        //! IMPORTANT, special case because it needs to be loaded on Component constructor
        if (!plasmoid.configuration.alignmentUpgraded) {
            plasmoid.configuration.alignment = plasmoid.configuration.panelPosition;
            plasmoid.configuration.alignmentUpgraded = true;
        }
    }
    //END functions

    ///////////////BEGIN components
    Component {
        id: appletItemComponent
        Applet.AppletItem{
            animations: _animations
            debug: _debug
            environment: _environment
            indexer: _indexer
            indicators: _indicators
            launchers: _launchers
            layouter: _layouter
            layouts: layoutsContainer
            metrics: _metrics
            myView: _myView
            parabolic: _parabolic
            shortcuts: _shortcuts
            thinTooltip: _thinTooltip
            userRequests: _userRequests
        }
    }

    Upgrader {
        id: upgrader
    }

    ///////////////END components

    LatteContainment.LayoutManager{
        id:fastLayoutManager
        plasmoidObj: Plasmoid
        rootItem: root
        dndSpacerItem: dndSpacer
        mainLayout: layoutsContainer.mainLayout
        startLayout: layoutsContainer.startLayout
        endLayout: layoutsContainer.endLayout
        metrics: _metrics

        onAppletOrderChanged: root.updateIndexes();
        onSplitterPositionChanged: root.updateIndexes();
        onSplitterPosition2Changed: root.updateIndexes();
    }

    Timer {
        id: repairAppletContainersTimer
        interval: 180
        repeat: false
        onTriggered: {
            fastLayoutManager.repairAppletContainers();
            root.updateIndexes();
        }
    }

    Timer {
        id: runtimeAppletRepairTimer
        interval: 150
        repeat: true

        property int remainingRuns: 0

        function schedule() {
            remainingRuns = 40;
            restart();
        }

        onTriggered: {
            fastLayoutManager.repairAppletContainers();
            root.updateIndexes();

            remainingRuns--;
            if (remainingRuns <= 0) {
                stop();
            }
        }
    }

    ///////////////BEGIN UI elements

    Loader{
        active: debug.windowEnabled
        sourceComponent: Debugger.DebugWindow{}
    }

    Loader{
        anchors.fill: parent
        active: debug.graphicsEnabled
        z:10

        sourceComponent: Item{
            Rectangle{
                anchors.fill: parent
                color: "yellow"
                opacity: 0.06
            }
        }
    }

    BindingsExternal {
        id: bindingsExternal
    }

    VisibilityManager{
        id: visibilityManager
        layouts: layoutsContainer
    }

    DragDropArea {
        id: backDropArea
        anchors.fill: parent

        Item{
            anchors.fill: layoutsContainer

            Background.MultiLayered{
                id: _background
            }
        }

        Layouts.LayoutsContainer {
            id: layoutsContainer
        }
    }

    // Bridge to receive wheel events outside DragDrop.DropArea, which blocks them in Qt 6
    MouseArea {
        anchors.fill: parent
        z: 10000
        acceptedButtons: Qt.NoButton
        property bool wheelIsBlocked: false

        function rootWheelActivateTask(next) {
            for (var ai = 0; ai < plasmoid.applets.length; ++ai) {
                var applet = plasmoid.applets[ai];
                if (applet.pluginName === "org.kde.latte.plasmoid") {
                    var quickItem = fastLayoutManager.resolveAppletQuickItem(applet);
                    if (quickItem && quickItem.activateWheelTask) {
                        quickItem.activateWheelTask(next);
                        return;
                    }
                }
            }
            layoutsContainer.activateWheelTask(next);
        }

        Timer {
            id: rootWheelDelayer
            interval: 200
            repeat: false
            onTriggered: parent.wheelIsBlocked = false
        }

        onWheel: function(wheel) {
            if (wheelIsBlocked) return;
            if (root.scrollAction === LatteContainment.types.ScrollNone) {
                root.emptyAreasWheel(wheel);
                return;
            }

            wheelIsBlocked = true;
            rootWheelDelayer.start();

            var delta = (wheel.angleDelta.y>=0 && wheel.angleDelta.x>=0)
                ? Math.max(wheel.angleDelta.y, wheel.angleDelta.x)
                : Math.min(wheel.angleDelta.y, wheel.angleDelta.x);
            var angle = delta / 8;
            var ctrlPressed = (wheel.modifiers & Qt.ControlModifier);

            if (angle > 10) {
                // Scroll UP = go LEFT (previous item in visual order)
                if (root.scrollAction === LatteContainment.types.ScrollDesktops) {
                    latteView.windowsTracker.switchToPreviousVirtualDesktop();
                } else if (root.scrollAction === LatteContainment.types.ScrollActivities) {
                    latteView.windowsTracker.switchToPreviousActivity();
                } else if (root.scrollAction === LatteContainment.types.ScrollToggleMinimized) {
                    if (!ctrlPressed) rootWheelActivateTask(false);
                    else if (!selectedWindowsTracker.lastActiveWindow.isMaximized)
                        selectedWindowsTracker.lastActiveWindow.requestToggleMaximized();
                } else {
                    rootWheelActivateTask(false);
                }
            } else if (angle < -10) {
                // Scroll DOWN = go RIGHT (next item in visual order)
                if (root.scrollAction === LatteContainment.types.ScrollDesktops) {
                    latteView.windowsTracker.switchToNextVirtualDesktop();
                } else if (root.scrollAction === LatteContainment.types.ScrollActivities) {
                    latteView.windowsTracker.switchToNextActivity();
                } else if (root.scrollAction === LatteContainment.types.ScrollToggleMinimized) {
                    if (!ctrlPressed) rootWheelActivateTask(true);
                    else {
                        var lw = selectedWindowsTracker.lastActiveWindow;
                        if (lw.isValid && !lw.isMinimized && lw.isMaximized) lw.requestToggleMaximized();
                        else if (lw.isValid && !lw.isMinimized && !lw.isMaximized) lw.requestToggleMinimized();
                    }
                } else {
                    rootWheelActivateTask(true);
                }
            }
        }
    }

    Colorizer.Manager {
        id: colorizerManager
    }

    EditMode.ConfigOverlay{
        id: _dragOverlay
    }

    Item {
        id: dndSpacer

        width: root.isHorizontal ? length : thickness
        height: root.isHorizontal ? thickness : length

        property int length: opacity > 0 ? (dndSpacerAddItem.length + metrics.totals.lengthEdges + metrics.totals.lengthPaddings) : 0

        readonly property bool isDndSpacer: true
        readonly property int thickness: metrics.totals.thickness + metrics.margin.screenEdge
        readonly property int maxLength: 96

        Layout.minimumWidth: width
        Layout.minimumHeight: height
        Layout.preferredWidth: Layout.minimumWidth
        Layout.preferredHeight: Layout.minimumHeight
        Layout.maximumWidth: Layout.minimumWidth
        Layout.maximumHeight: Layout.minimumHeight
        opacity: 0
        visible: parent === layoutsContainer.startLayout
                 || parent === layoutsContainer.mainLayout
                 || parent === layoutsContainer.endLayout

        z:1500

        Behavior on length {
            NumberAnimation {
                duration: animations.duration.large
                easing.type: Easing.InQuad
            }
        }

        Behavior on opacity {
            NumberAnimation {
                duration: animations.duration.large
                easing.type: Easing.InQuad
            }
        }

        Item {
            id: dndSpacerAddItemContainer
            width: root.isHorizontal ? parent.length : parent.thickness - metrics.margin.screenEdge
            height: root.isHorizontal ? parent.thickness - metrics.margin.screenEdge : parent.length

            property int thickMargin: metrics.margin.screenEdge

            LatteComponents.AddItem{
                id: dndSpacerAddItem
                anchors.centerIn: parent
                width: length
                height: width

                readonly property int length: Math.min(metrics.iconSize, 96)
            }

            states:[
                State{
                    name: "bottom"
                    when: plasmoid.location === PlasmaCore.Types.BottomEdge

                    AnchorChanges{
                        target: dndSpacerAddItemContainer;
                        anchors.horizontalCenter: parent.horizontalCenter; anchors.verticalCenter: undefined;
                        anchors.right: undefined; anchors.left: undefined; anchors.top: undefined; anchors.bottom: parent.bottom;
                    }
                    PropertyChanges{
                        target: dndSpacerAddItemContainer;
                        anchors.leftMargin: 0;    anchors.rightMargin: 0;     anchors.topMargin:0;    anchors.bottomMargin: dndSpacerAddItemContainer.thickMargin;
                        anchors.horizontalCenterOffset: 0; anchors.verticalCenterOffset: 0;
                    }
                },
                State{
                    name: "top"
                    when: plasmoid.location === PlasmaCore.Types.TopEdge

                    AnchorChanges{
                        target: dndSpacerAddItemContainer;
                        anchors.horizontalCenter: parent.horizontalCenter; anchors.verticalCenter: undefined;
                        anchors.right: undefined; anchors.left: undefined; anchors.top: parent.top; anchors.bottom: undefined;
                    }
                    PropertyChanges{
                        target: dndSpacerAddItemContainer;
                        anchors.leftMargin: 0;    anchors.rightMargin: 0;     anchors.topMargin: dndSpacerAddItemContainer.thickMargin;    anchors.bottomMargin: 0;
                        anchors.horizontalCenterOffset: 0; anchors.verticalCenterOffset: 0;
                    }
                },
                State{
                    name: "left"
                    when: plasmoid.location === PlasmaCore.Types.LeftEdge

                    AnchorChanges{
                        target: dndSpacerAddItemContainer;
                        anchors.horizontalCenter: undefined; anchors.verticalCenter: parent.verticalCenter;
                        anchors.right: undefined; anchors.left: parent.left; anchors.top: undefined; anchors.bottom: undefined;
                    }
                    PropertyChanges{
                        target: dndSpacerAddItemContainer;
                        anchors.leftMargin: dndSpacerAddItemContainer.thickMargin;    anchors.rightMargin: 0;     anchors.topMargin:0;    anchors.bottomMargin: 0;
                        anchors.horizontalCenterOffset: 0; anchors.verticalCenterOffset: 0;
                    }
                },
                State{
                    name: "right"
                    when: plasmoid.location === PlasmaCore.Types.RightEdge

                    AnchorChanges{
                        target: dndSpacerAddItemContainer;
                        anchors.horizontalCenter: undefined; anchors.verticalCenter: parent.verticalCenter;
                        anchors.right: parent.right; anchors.left: undefined; anchors.top: undefined; anchors.bottom: undefined;
                    }
                    PropertyChanges{
                        target: dndSpacerAddItemContainer;
                        anchors.leftMargin: 0;    anchors.rightMargin: dndSpacerAddItemContainer.thickMargin;     anchors.topMargin:0;    anchors.bottomMargin: 0;
                        anchors.horizontalCenterOffset: 0; anchors.verticalCenterOffset: 0;
                    }
                }
            ]
        }
    }

    Behavior on maxLengthPerCentage {
        enabled: plasmoid.configuration.floatingGapHidingWaitsMouse && dockContainsMouse
        NumberAnimation {
            duration: animations.duration.short
            easing.type: Easing.InQuad
        }
    }

    ///////////////END UI elements

    ///////////////BEGIN ABILITIES

    Ability.Animations {
        id: _animations
        layouts: layoutsContainer
        metrics: _metrics
        settings: universalSettings
    }

    Ability.AutoSize {
        id: _autosize
        layouts: layoutsContainer
        layouter: _layouter
        metrics: _metrics
        visibility: visibilityManager
    }

    Ability.Debug {
        id: _debug
    }

    AbilityHost.Environment{
        id: _environment
    }

    Ability.Indexer {
        id: _indexer
        layouts: layoutsContainer
    }

    Ability.Indicators{
        id: _indicators
        view: latteView
    }

    Ability.Launchers {
        id: _launchers
        layouts: layoutsContainer
        layoutName: latteView && latteView.layout ? latteView.layout.name : ""
    }

    Ability.Layouter {
        id: _layouter
        animations: _animations
        indexer: _indexer
        layouts: layoutsContainer
    }

    Ability.Metrics {
        id: _metrics
        animations: _animations
        autosize: _autosize
        background: _background
        indicators: _indicators
        parabolic: _parabolic
    }

    Ability.MyView {
        id: _myView
        layouts: layoutsContainer
    }

    Ability.ParabolicEffect {
        id: _parabolic
        animations: _animations
        debug: _debug
        layouts: layoutsContainer
        view: latteView
        settings: universalSettings
    }

    Ability.PositionShortcuts {
        id: _shortcuts
        layouts: layoutsContainer
    }

    Ability.ThinTooltip {
        id: _thinTooltip
        debug: _debug
        layouts: layoutsContainer
        view: latteView
    }

    Ability.UserRequests {
        id: _userRequests
        view: latteView
    }

    LatteApp.Interfaces {
        id: _interfaces
        plasmoidInterface: plasmoid

        Component.onCompleted: {
            if (view) {
                view.interfacesGraphicObj = _interfaces;
            }
        }

        onViewChanged: {
            if (view) {
                view.interfacesGraphicObj = _interfaces;

                if (!root.inStartup) {
                    //! used from recreating views
                    root.inStartup = true;
                    startupDelayer.start();
                }
            }
        }
    }

    ///////////////END ABILITIES

    ///////////////BEGIN TIMER elements

    //! It is used in order to slide-in the latteView on startup
    onInStartupChanged: {
        if (!inStartup) {
            if (latteView && latteView.positioner) {
                latteView.positioner.startupFinished();
                latteView.positioner.slideInDuringStartup();
            }

            if (visibilityManager) {
                visibilityManager.slotMustBeShown();
            }
        }
    }

    Connections {
        target:fastLayoutManager
        function onHasRestoredAppletsChanged() {
            if (fastLayoutManager.hasRestoredApplets) {
                startupDelayer.start();
            }
        }
    }

    Timer {
        //! Give a little more time to layouter and applets to load and be positioned properly during startup when
        //! the view is drawn out-of-screen and afterwards trigger the startup animation sequence:
        //! 1.slide-out when out-of-screen //slotMustBeHide()
        //! 2.be positioned properly at correct screen //slideInDuringStartup(), triggers Positioner::syncGeometry()
        //! 3.slide-in properly in correct screen //slotMustBeShown();
        id: startupDelayer
        interval: 1000
        onTriggered: {
            visibilityManager.slotMustBeHide();
        }
    }

    ///////////////END TIMER elements

    Loader{
        anchors.fill: parent
        active: debug.localGeometryEnabled
        sourceComponent: Rectangle{
            x: latteView.localGeometry.x
            y: latteView.localGeometry.y
            //! when view is resized there is a chance that geometry is faulty stacked in old values
            width: Math.min(latteView.localGeometry.width, root.width) //! fixes updating
            height: Math.min(latteView.localGeometry.height, root.height) //! fixes updating

            color: "blue"
            border.width: 2
            border.color: "red"

            opacity: 0.35
        }
    }

    Loader{
        anchors.fill: parent
        active: latteView && latteView.effects && debug.inputMaskEnabled
        sourceComponent: Rectangle{
            x: latteView.effects.inputMask.x
            y: latteView.effects.inputMask.y
            //! when view is resized there is a chance that geometry is faulty stacked in old values
            width: Math.min(latteView.effects.inputMask.width, root.width) //! fixes updating
            height: Math.min(latteView.effects.inputMask.height, root.height) //! fixes updating

            color: "purple"
            border.width: 1
            border.color: "black"

            opacity: 0.20
        }
    }

    Timer {
        id: initTimer
        interval: 500
        onTriggered: universalSettings.inConfigureAppletsMode = false
    }
}
