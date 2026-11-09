# R8a source-contract inventory

Scope: the 229 test functions of `autotests/sourcecontracttest.cpp` as they stood at 6bdd43086, before any R8a conversion. Line ranges and source paths come from [contracts-before.md](R8a-tools/contracts-before.md), the output of [list_contracts.py](R8a-tools/list_contracts.py) on that file. Each row gives one class and a reason of at most 15 words. The reason for a converted row says what replaced it; the conversions and their mutation proofs are in [R8a.md](R8a.md).

The 14 D rows were checked by mutation in [R8f](R8f.md): one was removed, four were trimmed and nine kept, with the classes they have now. Row 107, one of the kept rows, became a behavioural case in [R8g](R8g.md), and its contract was removed. Row 105, also kept, became the no-copy contract `containmentInterfaceUsesSharedSeparatorPluginIds` in [R8h](R8h.md) when `ContainmentInterface` dropped its copies of the ids. The rows below are not edited; they record the classification at 6bdd43086.

## Class counts

| Class | Meaning | Count |
|---|---|---|
| K | keep: protects text that no behavioural test can reach | 113 |
| B1 | behavioural candidate: class constructible in a test, has a target | 6 |
| B2 | behavioural candidate that needs a fixture or fake first | 45 |
| B3 | needs a Corona, a display or a live session; not testable here | 45 |
| D | duplicate of another contract or an existing behavioural test | 14 |
| S | stale: pins an implementation detail that protects no failure mode | 6 |
| total | | 229 |

## Rows

| # | Function | Lines | Class | Reason |
|---|---|---|---|---|
| 1 | `visibilityManagerBlockerWrappersDelegateTransitions` | 428-455 | B3 | Blocker call order inside VisibilityManager; needs a View or Corona. |
| 2 | `plasmaVolumeBootstrapContractMovedToQmlSmokeTest` | 457-473 | S | Forwarding stub: only checks that the moved qmlsmoketest case still exists. |
| 3 | `compactAppletPopupSizingContractMovedToQmlSmokeTest` | 475-490 | S | Forwarding stub: only checks that the moved qmlsmoketest case still exists. |
| 4 | `applicationLauncherUsesFixedExternalSlot` | 492-501 | K | QML text in AppletItem.qml. |
| 5 | `widgetsZoomCanBeDisabledIndependently` | 503-533 | K | QML and config XML text for the zoom toggle. |
| 6 | `animationsSpeedFactorMapsToDurationMultipliers` | 535-556 | K | QML function text in Animations.qml. |
| 7 | `effectsConfigAnimationButtonsMatchDurationValues` | 558-578 | K | QML page text in EffectsConfig.qml. |
| 8 | `appletShadowFadesInsteadOfPoppingWhileWidgetsZoom` | 580-592 | K | QML text in ItemWrapper.qml. |
| 9 | `defaultTemplatesExcludeMailLauncher` | 594-606 | K | Layout and view template text under shell/package. |
| 10 | `latteTasksExposesPlasmaLauncherApi` | 608-621 | K | QML API text in plasmoid main.qml. |
| 11 | `latteDockDbusExportsLauncherApi` | 623-643 | K | DBus XML and header text; the interface is a compatibility surface. |
| 12 | `plasmaKickerActionAddsLaunchersToLatteDock` | 645-686 | K | Desktop action file, CMake and install script text. |
| 13 | `containmentClearsParabolicStateWhenEdgeChanges` | 688-697 | K | QML text in containment main.qml. |
| 14 | `duplicateInstanceExitsWithoutQGuiAppExit` | 699-729 | B3 | Lock-before-QGuiApplication order in main(); needs a running binary. |
| 15 | `allScreensCloneAppletSyncContracts` | 731-820 | B3 | Clone sync across ContainmentInterface, ClonedView and LayoutManager; needs live views. |
| 16 | `layoutManagerRepairSkipsAppletsInDestruction` | 822-845 | B2 | LayoutManager repair path; needs a containment and applet fake. |
| 17 | `layoutManagerScheduledDestructionTogglesItemVisibility` | 847-877 | B2 | LayoutManager destruction visibility; needs a containment and applet fake. |
| 18 | `cloneViewOrderSyncDropsStaleTargetEntries` | 879-901 | B2 | ClonedView order sync; clonedviewpolicytest covers preferences only. |
| 19 | `cloneViewRemovalSyncUsesSyncingFromOriginalGuard` | 903-927 | B2 | ClonedView removal guard; needs a view and original applet fixture. |
| 20 | `waylandStrutGhostWindowBindsLayerShellScreen` | 929-942 | B3 | Wayland layer-shell binding; needs a Wayland display. |
| 21 | `launchersRestoreContractMovedToQmlSmokeTest` | 944-959 | S | Forwarding stub: only checks that the moved qmlsmoketest case still exists. |
| 22 | `sessionShutdownHandlingMatchesStableWaylandPath` | 961-1104 | B3 | Signal and logout wiring across main(), Corona and views; needs a session. |
| 23 | `sessionShutdownMonitorHidesViewsAndRestoresOnCancel` | 1106-1176 | D | Duplicate of shutdownmonitortest confirmedLogoutHidesViewsOnce and cancelledLogoutRestoresViewsWhenServiceLeaves. |
| 24 | `coronaTeardownSkipsFactoryFreedByQuitCleanup` | 1178-1214 | D | Duplicate of teardowntest deleteNowSkipsServiceFreedByQuitCleanup. |
| 25 | `plasmaThemeDestructorToleratesMissingSchemes` | 1216-1238 | B2 | theme.cpp is in no test target; needs a Plasma theme fixture. |
| 26 | `needsAttentionStatusBlocksHidingWithoutWindowReconfiguration` | 1240-1275 | B3 | View hiding and window reconfiguration; needs a View and display. |
| 27 | `appletPopupHidesOnWindowDeactivateByDefault` | 1277-1291 | K | QML and config default text in CompactApplet.qml. |
| 28 | `viewTracksPointerWindowsAndResetsCascadingSubmenus` | 1293-1311 | B3 | View event filter and submenu reset; needs a View and window system. |
| 29 | `pointerWindowTrackerRemovesDestroyedWindows` | 1313-1335 | B3 | Destroyed-window cleanup lives in View; needs a View with windows. |
| 30 | `qtQuickGpuPreferenceKeepsSoftwareFallbackAvailable` | 1337-1382 | B3 | Graphics preference and software fallback; needs a GPU or Wayland session. |
| 31 | `knsCompatImportsAreAvailableForSystemInstall` | 1384-1429 | B3 | knscompat calls are wired in main(); needs the executable. |
| 32 | `mainCppKeepsQtContainerStaticsFunctionLocal` | 1431-1457 | K | Static-destruction style rule in main.cpp; main() is not reachable by tests. |
| 33 | `layerShellSetScreenGuardPreventsBuildRegression` | 1459-1489 | K | Build-probe text: CMake check file and Wayland call site. |
| 34 | `taskIconsRefreshAfterIconThemeChanges` | 1491-1517 | K | QML text in TaskIcon.qml. |
| 35 | `genericLayoutUnloadUsesSynchronousDelete` | 1519-1534 | B3 | Unload delete order in GenericLayout; needs a Corona-owned layout. |
| 36 | `synchronizerUnloadUsesSynchronousDelete` | 1536-1549 | B2 | Synchronizer unload delete order; needs layout fixtures; no class-level test. |
| 37 | `pointerWindowTrackingDisconnectsOnLeave` | 1551-1582 | B3 | View pointer-window connections; needs a View. |
| 38 | `viewDestructorDropsPointerWindowTrackingConnections` | 1584-1609 | B3 | View destructor cleanup; needs a View. |
| 39 | `taskAudioBadgesScaleWithParabolicZoom` | 1611-1631 | K | QML text in AudioStream.qml. |
| 40 | `parabolicScaleAddressingFallsBackToLastValidIndexDuringRemoval` | 1633-1655 | K | QML text in ParabolicEventsArea.qml. |
| 41 | `widgetExplorerLaunchesKnsDialogOutOfProcess` | 1657-1711 | K | QML text in WidgetExplorer.qml; the out-of-process launch needs a session. |
| 42 | `widgetExplorerUsesPlasmaTranslationContexts` | 1713-1731 | K | QML translation-context text in WidgetExplorer.qml. |
| 43 | `settingsQmlWaitsForLiveViewContext` | 1733-1748 | K | QML text in LatteDockConfiguration.qml. |
| 44 | `qmlConnectionsMatchQtNotifySignals` | 1750-1763 | K | QML Connections text in settings files. |
| 45 | `itemsAlignmentIsSeparateAndJustifyOnly` | 1765-1828 | K | Config XML, QML and translation text for alignment. |
| 46 | `itemsAlignmentNormalizesDirectionsByFormFactor` | 1830-1853 | K | QML text in Containment.qml and LayoutsContainer.qml. |
| 47 | `itemsAlignmentConfigDefaultsToCenter` | 1855-1869 | K | Config XML default text. |
| 48 | `appearancePaletteExposesLayoutCustomColors` | 1871-1895 | K | QML text in AppearanceConfig.qml and Manager.qml. |
| 49 | `modernDockBackgroundShadowDefaultIsCompact` | 1897-1919 | K | QML constants and qmldir text. |
| 50 | `classicCustomRadiusPreservesThemeDefaultShadow` | 1921-1931 | K | QML text in MultiLayered.qml. |
| 51 | `panelShadowSelectionMatrixIsStable` | 1933-1946 | K | QML text in MultiLayered.qml. |
| 52 | `layoutDetailsExposeCustomColorSchemeSelector` | 1948-1964 | K | Qt Designer .ui text in the details dialog. |
| 53 | `showWindowAnimationContractMovedToQmlSmokeTest` | 1966-1980 | S | Forwarding stub: only checks that the moved qmlsmoketest case still exists. |
| 54 | `parabolicItemContractMovedToQmlSmokeTest` | 1982-1996 | S | Forwarding stub: only checks that the moved qmlsmoketest case still exists. |
| 55 | `autotestAggregateTargetDocumentsFullSuiteBuild` | 1998-2014 | K | CMake and docs text for the aggregate target. |
| 56 | `coverageEstimateUsesReusableScript` | 2016-2030 | K | Script and docs text. |
| 57 | `cmakeTargetResolutionUsesSharedHelpers` | 2032-2049 | K | CMake module text. |
| 58 | `cmakeImportedTargetResolutionUsesSharedHelper` | 2051-2067 | K | CMake module text. |
| 59 | `cmakeTargetResolutionHelpersLiveInModule` | 2069-2085 | K | CMake module text. |
| 60 | `cmakeOffscreenTestsUseSharedHelper` | 2087-2104 | K | CMake test registration text. |
| 61 | `cmakeAutotestRegistrationMaintainsAggregateTarget` | 2106-2123 | K | CMake test registration text. |
| 62 | `cmakePackagingConfigLivesInModule` | 2125-2148 | K | CMake packaging text. |
| 63 | `autostartUsesXdgStateOnly` | 2150-2164 | B2 | UniversalSettings autostart; needs a KConfig fixture; no test covers the class. |
| 64 | `screensPropertyNotifiesOnScreenChanges` | 2166-2194 | B2 | UniversalSettings screens notify; needs a screen-change fixture. |
| 65 | `desktopFileHasAutostartPhaseKey` | 2196-2205 | K | Desktop file text. |
| 66 | `enableAutostartExitsImmediately` | 2207-2231 | B3 | --enable-autostart exit path in main(); needs a running binary. |
| 67 | `autostartInterfacesUseXdgState` | 2233-2250 | B2 | Settings handler and corona autostart wiring; needs a settings fixture. |
| 68 | `autostartPreservesEntryFields` | 2252-2262 | D | Duplicate of autostartlogictest enableUnhidesEntryAndPreservesFields and disableHidesEntryAndPreservesFields. |
| 69 | `firstRunEnablesDefaultAutostart` | 2264-2278 | B2 | LayoutsManager first-run path; no test target; needs a layouts fixture. |
| 70 | `waylandCheckFailsWithoutDeadRetryLoop` | 2280-2301 | B3 | Wayland startup check in main(); needs a session. |
| 71 | `normalStartupDoesNotMutateAutostart` | 2303-2315 | B2 | UniversalSettings startup load; needs a KConfig fixture. |
| 72 | `autoSizeLoopsUseInequalityNotStrictEquality` | 2317-2334 | K | QML text in AutoSize.qml. |
| 73 | `cmakeWarningRelaxationLivesInModule` | 2336-2348 | K | CMake module text. |
| 74 | `cmakeFindsQtCoreToolsBeforeKdeInstallDirs` | 2350-2361 | K | Top-level CMake ordering text. |
| 75 | `compactAppletDigitalClockWidthCapPreventsLongDateFormatOverflow` | 2363-2440 | K | QML text in CompactApplet.qml. |
| 76 | `contextMenuLayerMiddleClickCloseActiveWindowGuardedCorrectly` | 2442-2475 | B2 | Middle-click guard in a QQuickItem not linked into contextmenuunittest. |
| 77 | `appletIconOverrideStripsSymbolicForOriginalColors` | 2477-2529 | B3 | Icon override across ContainmentInterface, LayoutManager and QML; needs live applets. |
| 78 | `widgetOriginalIconColorsFallbackKeepsHoverAndStateSynchronized` | 2531-2588 | K | QML text in AppletItem.qml and ItemWrapper.qml. |
| 79 | `containmentLayoutAutotestLinksIconThemes` | 2590-2609 | K | CMake test link and compat text. |
| 80 | `appletContextMenuExposesKeepOriginalColorsToggle` | 2611-2638 | K | Menu label text in the context menu source. |
| 81 | `appletQmlParentInputsDoNotShadowContextIds` | 2640-2670 | K | QML text in AppletItem.qml and main.qml. |
| 82 | `trashKeepOriginalColorsDefaultsToCheckedForAllConfigs` | 2672-2725 | B2 | LayoutManager default config for the trash applet; needs a containment fixture. |
| 83 | `settingsDialogUiLabelsAreTranslatable` | 2727-2759 | K | Qt Designer .ui translation text. |
| 84 | `poFilesHaveNoStaleDockPanelMsgids` | 2761-2802 | K | Translation (.po) file text. |
| 85 | `translationExtractionReferencesValidDirectories` | 2804-2834 | K | Messages.sh script text. |
| 86 | `mouseHandlerAutoPinOnDragPromotesNonLauncherTasks` | 2836-2854 | K | QML text in MouseHandler.qml. |
| 87 | `scrollToggleMinimizedDownwardUnmaximizesBeforeMinimizing` | 2856-2881 | K | QML text in containment main.qml. |
| 88 | `scrollToggleMinimizedUsesAllScreensTrackerForMinimizeAndMaximize` | 2883-2922 | K | QML text in containment main.qml and EnvironmentActions.qml. |
| 89 | `globalMenuUsesNativeVisibilityAndSizing` | 2924-2965 | K | QML text in applet and containment files. |
| 90 | `systrayGuardsInAppletItemPreventLayoutAndInteractionBreakage` | 2967-2998 | K | QML text in AppletItem.qml. |
| 91 | `volumeAndAppMenuPopupSizingUsesLargerMinimumInCompactApplet` | 3000-3030 | K | QML text in CompactApplet.qml. |
| 92 | `clipboardAndDigitalClockErrorSuppressionInMainCpp` | 3032-3055 | K | Message-filter text in main.cpp; not observable without the binary. |
| 93 | `systemTrayAndPlasmoidActAsAppletInsertionBoundary` | 3057-3079 | B3 | Insertion boundary in ContainmentInterface; needs a Corona. |
| 94 | `separatorAndSpacerDetectionAndBehaviorInAppletItem` | 3081-3120 | K | QML text in AppletItem.qml. |
| 95 | `separatorGuardsAcrossLayoutAndDragDropFiles` | 3122-3173 | K | QML text across applet and drag-drop files. |
| 96 | `separatorContentContainerStaysTightWithFallback` | 3175-3207 | K | QML text in ItemWrapper.qml. |
| 97 | `restoreZoomTimerGracePeriodPreventsBoundaryBlip` | 3209-3233 | K | QML text in ParabolicEffectPrivate.qml. |
| 98 | `myViewClientIntPropertiesUseSafeIntGuardAgainstUndefined` | 3235-3257 | K | QML text in MyView.qml. |
| 99 | `indicatorFactoryExcludesBuiltinPluginsFromCustomLists` | 3259-3273 | B1 | Converted in R8a: indicatorunittest factoryBuiltInIndicatorsAreNotCustom. Target: indicatorunittest. |
| 100 | `waylandInterfaceAcceptableWindowHasHardcodedAppIdWhitelist` | 3275-3289 | B3 | Window acceptance in WaylandInterface; needs a Wayland display. |
| 101 | `windowHintPerformanceTraceMarksScheduledAndExecutedWork` | 3291-3346 | B3 | Trace markers in the window tracker; needs a running tracker. |
| 102 | `genericLayoutReassertsDefaultContextMenuOnContainmentWiring` | 3348-3358 | B3 | Context-menu wiring in GenericLayout; needs a Corona. |
| 103 | `genericLayoutSyncLatteViewsToScreensIteratesSnapshots` | 3360-3406 | B3 | View-to-screen sync in GenericLayout; needs live views. |
| 104 | `layoutManagerCleanupOnStartupRemovesLegacyAndGhostApplets` | 3408-3423 | B2 | Layouts manager startup cleanup; no test target; needs layout fixtures. |
| 105 | `containmentInterfaceHasSeparatorPluginConstantsAndHelper` | 3425-3438 | D | Same separator plugin IDs classified by dataunittest pluginIdClassifiesLatteBuiltInsAndSeparators. |
| 106 | `appletsModelHasNoPersonalDataAppletList` | 3440-3451 | B2 | Applets model in the export dialog; needs an applet catalogue fixture. |
| 107 | `lattePackageShellPluginNameBranchingIsCorrect` | 3453-3462 | D | Duplicate of packageunittest lattePackageResolvesShellDefinitions. |
| 108 | `indicatorPlasmaTypeIsRemappedToDefault` | 3464-3474 | B3 | Indicator type remap inside view indicator code; needs a View. |
| 109 | `dynamicWindowDotsAreOptInAndAggregateOverflow` | 3476-3515 | K | Indicator config XML and QML text. |
| 110 | `appearanceConfigPreservesIndicatorStyleConfiguration` | 3517-3538 | K | QML text in AppearanceConfig.qml. |
| 111 | `mainCppMessageSuppressionCoversFrameworkWarnings` | 3540-3560 | K | Message-filter text in main.cpp; not observable without the binary. |
| 112 | `appletItemInternalViewSplitterAndSortDragGuards` | 3562-3599 | K | QML text in AppletItem.qml. |
| 113 | `appletItemFallbackTrackedWindowsAndConstraintHints` | 3601-3623 | K | QML text in AppletItem.qml. |
| 114 | `plasmoidLaunchersAndDragDropHavePluginDetectionGuards` | 3625-3642 | K | QML text in Launchers.qml and DragDropArea.qml. |
| 115 | `containmentMainQmlHasPlasmoidWheelBypassGuards` | 3644-3652 | K | QML text in containment main.qml. |
| 116 | `compactAppletFallbackSizingAndMinimumDimensionGuards` | 3654-3667 | K | QML text in CompactApplet.qml. |
| 117 | `clonedViewDefersInitialAppletOrderSyncUntilStructuralReady` | 3669-3718 | B2 | ClonedView startup sync; needs a view fixture. |
| 118 | `layoutManagerShouldRetryGatedByMaxRetries` | 3720-3733 | B2 | LayoutManager retry gate; needs a containment fixture. |
| 119 | `mainCppSelfPipeSigintHandlerAndReplaceLockTimeout` | 3735-3763 | B3 | Signal handler and lock timeout in main(); needs a running binary. |
| 120 | `layoutManagerRestoreMaxRetryExhaustionFallsThrough` | 3769-3782 | B2 | LayoutManager restore retries; needs a containment fixture. |
| 121 | `layoutManagerRestoreBuildsOrderFromLiveAppletsWhenStoredEmpty` | 3784-3794 | B2 | LayoutManager restore order; needs live applet fixtures. |
| 122 | `layoutManagerRestoreInvalidAppletCleanupPreservesValidIds` | 3796-3807 | B2 | LayoutManager cleanup of invalid applets; needs a containment fixture. |
| 123 | `layoutManagerRestoreAppletContainerCreationRetryLimit` | 3809-3818 | B2 | LayoutManager retry limit; needs a containment fixture. |
| 124 | `visibilityManagerSlotMustBeShownGuardsStartupAndMode` | 3824-3837 | K | QML text in VisibilityManager.qml. |
| 125 | `visibilityManagerSlotMustBeHideGuardsMouseAndBlockHiding` | 3839-3852 | K | QML text in VisibilityManager.qml. |
| 126 | `visibilityManagerUpdateMaskAreaClampingPreventsNegativeBounds` | 3854-3866 | K | QML text in VisibilityManager.qml. |
| 127 | `visibilityManagerSlidingOutAnimationInitGatesInStartup` | 3868-3879 | K | QML text in VisibilityManager.qml. |
| 128 | `positionerStartupPaintOffScreenAtNegative9999` | 3885-3895 | B3 | Positioner startup position; needs a View and screen. |
| 129 | `positionerSyncGeometrySuppressedDuringSlideAnimation` | 3897-3906 | B3 | Positioner geometry sync; needs a View. |
| 130 | `positionerResizeWindowEnforcesWaylandMinimumSize` | 3908-3918 | B3 | Positioner window resize; needs a Wayland window. |
| 131 | `positionerPlasmaPanelGeometryIntersectionGuardsEmptyList` | 3920-3929 | B3 | Panel intersection guard runs under Corona window manager; positionergeometrytest covers the helper only. |
| 132 | `storageAvailableIdUpperBoundExcludesId32000` | 3935-3945 | D | Duplicate of storagelogictest availableIdEmptyWhenExhausted. |
| 133 | `storageNewUniqueIdsFileCircularAssignmentDetection` | 3947-3956 | B2 | Storage id-file rewrite; needs a layout file fixture. |
| 134 | `storageExpectedViewScreenIdAllSecondaryEmptyFallback` | 3958-3968 | B3 | Expected screen id takes a Corona; needs a live Corona. |
| 135 | `importerFileVersionDetectsLayoutVersion2AndConfigVersion1` | 3974-3986 | D | Layout-version part duplicates importerlogictest layoutV2Recognized. |
| 136 | `importerLayoutCanBeImportedRejectsVersionBelow2` | 3988-3997 | D | Duplicate of importerlogictest layoutV1IsUnknown. |
| 137 | `importerUniqueLayoutNameCollisionLoopHasGuard` | 3999-4008 | B2 | Importer unique-name loop; needs a layouts directory fixture. |
| 138 | `screenPoolScreenForIdFallsBackToPrimaryWhenScreenAbsent` | 4014-4024 | B1 | Converted in R8a: corecomponentstest screenPoolScreenForIdFallsBackToPrimaryScreen. Target: corecomponentstest. |
| 139 | `plasmaExtendedScreenPoolIdReturnsZeroForPrimaryConnector` | 4026-4035 | B1 | Converted in R8a: corecomponentstest plasmaExtendedScreenPoolMapsPrimaryConnectorToIdZero. Target: corecomponentstest. |
| 140 | `screenPoolIsScreenActiveReturnsFalseForStaleDisconnectedId` | 4037-4047 | B1 | Converted in R8a: corecomponentstest screenPoolIsScreenActiveRequiresAttachedScreen. Target: corecomponentstest. |
| 141 | `launchersNormalizeLauncherListExpandsOnlyLength2to4` | 4053-4065 | K | QML text in Launchers.qml. |
| 142 | `launchersFreeSeparatorNameExhaustionReturnsEmptyString` | 4067-4078 | K | QML text in Launchers.qml. |
| 143 | `launchersTransientEmptyRecoveryCeilingAt8` | 4080-4089 | K | QML text in Launchers.qml. |
| 144 | `launchersIsSeparatorGuardsDesktopExtensionPosition` | 4091-4102 | K | QML text in Launchers.qml. |
| 145 | `launchersAddDroppedLauncherIconDataTruncationBoundary` | 4104-4115 | K | QML text in Launchers.qml. |
| 146 | `launchersRemoveInternalSeparatorRefusesEmptyUrl` | 4117-4137 | K | QML text in Launchers.qml. |
| 147 | `taskItemIsSeparatorNeverTurnsUrlLessWindowsIntoDividers` | 4139-4159 | K | QML text in TaskItem.qml. |
| 148 | `contextMenuSeparatorRemovalRoutesThroughGuardedLauncherPath` | 4161-4187 | K | QML text in ContextMenu.qml. |
| 149 | `contextMenuFactoriesUseDeclaredComponentsNotStringCompilation` | 4189-4220 | K | QML text in ContextMenu.qml. |
| 150 | `mainCppAvailableLayoutsPrintsDifferentMessageWhenEmpty` | 4226-4236 | B3 | CLI output in main(); needs running the binary. |
| 151 | `mainCppLayoutOptionExitsForMissingLayout` | 4238-4247 | B3 | CLI exit path in main(); needs running the binary. |
| 152 | `mainCppClearCacheSkipsNonexistentDirectory` | 4249-4258 | B3 | CLI cache path in main(); needs running the binary. |
| 153 | `mainCppImportLayoutExitsOnImportFailure` | 4260-4269 | B3 | CLI import exit in main(); needs running the binary. |
| 154 | `mainCppDeferredDeleteDrainHardLimit5Passes` | 4271-4280 | B3 | Deferred-delete drain in main(); needs running the binary. |
| 155 | `mainCppDetectPlatformPreservesExplicitPlatformArg` | 4282-4293 | B3 | Platform argument handling in main(); needs running the binary. |
| 156 | `mainCppMainFunctionStaysWithinLineLimit` | 4295-4312 | S | Pins main() length, a style proxy that protects no runtime failure. |
| 157 | `mainQmlInStartupSetFalseInSlidingOutAnimationOnStopped` | 4318-4329 | K | QML text in VisibilityManager.qml. |
| 158 | `mainQmlStartupDelayerTriggeredByHasRestoredAppletsSignal` | 4331-4341 | K | QML text in containment main.qml. |
| 159 | `mainQmlCreateAppletItemRetryCeilingAt80` | 4343-4353 | K | QML text in containment main.qml. |
| 160 | `mainQmlRetryTimerUsesDeclaredComponentNotStringCompilation` | 4355-4379 | K | QML text in containment main.qml. |
| 161 | `mainQmlPanelCfgSyncTransparencySevenInputClasses` | 4381-4429 | K | QML text in containment main.qml and BindingsExternal.qml. |
| 162 | `mainQmlOnInStartupChangedMustCheckLatteViewExists` | 4431-4442 | K | QML text in containment main.qml. |
| 163 | `appletItemForAppletInSafeContextsUsesPublicApi` | 4444-4472 | B2 | Public API for applet items; needs a QML engine and containment fake. |
| 164 | `appletItemForAppletExcludedContextsPreservePropertyAccess` | 4474-4510 | B3 | Property access across View and settings views; needs live views. |
| 165 | `mouseButtonEnumUsesMiddleButtonNotMidButton` | 4512-4540 | K | QML enum text in task and layout files. |
| 166 | `taskMouseAreaSkipsInactivePreviewChecks` | 4542-4562 | K | QML text in TaskMouseArea.qml. |
| 167 | `taskMouseAreaUsesAuthoritativeEditModeState` | 4564-4592 | K | QML text in task and containment files. |
| 168 | `appletIconResourcePathsUseQmlUrlScheme` | 4594-4610 | B2 | Icon resource path in LayoutManager; needs a containment fixture. |
| 169 | `isolatedWindowPreviewProcessIsFailClosed` | 4612-4768 | D | Fail-closed rules duplicate previewprocessunittest disabledGateStaysClosed and forgedCloseReplyIsRejected; QML text remains. |
| 170 | `latteCoreQmlModuleDeclaresPlasmaCoreDependency` | 4770-4779 | K | CMake dependency text for the QML module. |
| 171 | `taskFallbackTooltipRespectsLatteTooltipSetting` | 4781-4821 | K | QML and layout template text. |
| 172 | `taskTooltipUsesHoveredVisualState` | 4823-4847 | K | QML text in TaskItem.qml. |
| 173 | `thinTooltipHandlesMissingTextAndRepositionsAfterShowing` | 4849-4886 | K | QML text in ThinTooltip.qml. |
| 174 | `tooltipDialogRoutesRepositioningThroughPlasmaGeometry` | 4888-4914 | B3 | Popup repositioning needs a Wayland popup surface. |
| 175 | `contextMenuHandlesIncompleteDbusData` | 4916-4930 | B2 | Menu decodes DBus data into private state; needs a fake service. |
| 176 | `contextMenuMarksCurrentLayoutsInMoveMenu` | 4932-4950 | D | Duplicate of containmentactionmenuunittest onlyListedCurrentLayoutsAreMarked and emptyCurrentListMarksNothing. |
| 177 | `dockViewFollowsSystemColorSchemeAtRuntime` | 4952-4975 | B3 | View color-scheme follow; needs a View and palette change. |
| 178 | `environmentActionsDoesNotAcceptMiddleButton` | 4977-4990 | K | QML text in EnvironmentActions.qml. |
| 179 | `upgraderQmlUsesPlasmoidConfiguration` | 4992-5003 | K | QML text in Upgrader.qml. |
| 180 | `middleClickActionDefaultIsClose` | 5005-5022 | K | Plasmoid config XML default text. |
| 181 | `dragDropHandlersUseBindingSyntaxForQt6` | 5024-5043 | K | QML text in DragDropArea.qml. |
| 182 | `tasktoolsServicesFromCmdLineGuardsFirstSpace` | 5047-5057 | B1 | Converted in R8a: wmunittest servicesFromCmdLineSingleWordRuntimeDoesNotRecurse. Target: wmunittest. |
| 183 | `importerUniqueLayoutNameHasIterationCap` | 5059-5068 | B2 | Importer unique-name loop cap; needs a layouts directory fixture. |
| 184 | `viewscontrollerUniqueViewNameHasIterationCap` | 5070-5079 | B2 | Views controller name loop; no test target; needs a settings fixture. |
| 185 | `layoutscontrollerUniqueLayoutNameHasIterationCap` | 5081-5090 | B2 | Layouts controller name loop; no test target; needs a settings fixture. |
| 186 | `lattecoronaUnloadUsesCappedLiveListLoop` | 5092-5104 | B3 | Corona unload loop; needs a live Corona. |
| 187 | `lattecoronaScreenForContainmentHasDepthGuard` | 5106-5115 | B3 | Corona screen lookup depth guard; needs a live Corona. |
| 188 | `lattecoronaAlternativesCallbacksFollowTheirOwners` | 5117-5133 | B3 | Corona alternatives callbacks; needs a live Corona. |
| 189 | `originalViewCleanClonesDrainsFromLocalCopy` | 5135-5145 | B3 | Original view clone cleanup; needs a live view. |
| 190 | `layoutManagerResolveAppletQuickItemUsesVisitedSet` | 5147-5158 | B2 | LayoutManager cycle guard; needs a QML item graph fixture. |
| 191 | `levelOptionsHasIsBackgroundIsForegroundReentryGuards` | 5160-5172 | K | QML text in LevelOptions.qml. |
| 192 | `templatesmanagerUniqueNameLoopsHaveIterationCaps` | 5174-5185 | B2 | Templates manager name loops; needs a templates directory fixture. |
| 193 | `synchronizerUnloadLayoutsHasIterationCap` | 5187-5197 | B2 | Synchronizer unload loop; needs layout fixtures. |
| 194 | `layoutsSaveRewritesRenamedIdsFromOneSnapshot` | 5199-5210 | B2 | LayoutsController save; needs a settings and file fixture. |
| 195 | `layoutsSaveLocksRenamedLayoutsAtTheirNewFile` | 5212-5224 | B2 | LayoutsController save; needs a settings and file fixture. |
| 196 | `layoutsSaveHandsSynchronizerTableAfterRename` | 5226-5242 | B2 | LayoutsController rename hand-off; needs a Synchronizer fixture. |
| 197 | `synchronizerGuardsStoredLayoutLookups` | 5244-5262 | D | Duplicate of synchronizerlogictest lookupExistingAndNonExisting. |
| 198 | `layoutsSaveMovesFilesThroughCheckedTemporaryDir` | 5264-5289 | B2 | LayoutsController save; needs a settings and file fixture. |
| 199 | `layoutsSaveMovesBeforeRewritingIds` | 5291-5306 | B2 | LayoutsController save order; needs a file fixture. |
| 200 | `layoutsSaveRenamesOnlyMovedLayouts` | 5308-5325 | B2 | LayoutsController save; needs a file fixture. |
| 201 | `layoutFileMoveRenamesOnlyThroughNoOverwriteCheck` | 5327-5337 | D | Duplicate of storagelogictest layoutMoveChainBlockedByExistingTargetKeepsEveryFile. |
| 202 | `layoutScannersListOnlyFiles` | 5339-5355 | B2 | Directory scanners in several classes; each needs a fixture. |
| 203 | `uniqueNameExhaustionFallsBackToRandomSuffix` | 5357-5376 | B2 | Unique-name fallback in several classes; each needs a fixture. |
| 204 | `layoutManagerResolveAppletQuickItemThreadsVisitedSet` | 5378-5387 | B2 | LayoutManager recursion frames; needs a QML item graph fixture. |
| 205 | `appdataComponentIdKeepsHyphenInLastSegment` | 5389-5403 | K | Appdata XML CMake text. |
| 206 | `applicationMetadataUsesCurrentIconName` | 5405-5418 | K | Appdata XML and metadata JSON text. |
| 207 | `positionShortcutHandlersDeclareSignalParameters` | 5420-5434 | K | QML text in BasicItem.qml and AppletItem.qml. |
| 208 | `positionShortcutHostLookupIsRecursiveAndResettable` | 5436-5452 | B3 | Host lookup in ContainmentInterface; needs a live Corona. |
| 209 | `panelShadowRoundnessRejectsIncompleteThemeAssets` | 5454-5471 | B2 | Private updater in PanelBackground; needs a KSvg theme fixture. |
| 210 | `qmlCacheRevisionInvalidatesSameVersionBuilds` | 5473-5485 | B3 | QML cache across builds; needs two installed builds. |
| 211 | `taskRemovalDefersAnimationOutsideGeometryBindings` | 5487-5499 | K | QML text in TaskItem.qml. |
| 212 | `externalTouchingBindingsGuardClearedScreenGeometry` | 5501-5510 | K | QML text in BindingsExternal.qml. |
| 213 | `viewTeardownBindingsGuardClearedVisibility` | 5512-5525 | K | QML text in MyView.qml and VisibilityManager.qml. |
| 214 | `taskBackgroundThicknessKeepsItsMetricsContract` | 5527-5542 | K | QML text in task and main files. |
| 215 | `plasma68ContainmentScreenAndImportContracts` | 5544-5593 | K | Plasma 6.8 API call-site text; a compile probe covers it. |
| 216 | `importedAppletIdsStayUniqueIncludingNestedApplets` | 5595-5641 | D | Duplicate of appletidsunittest duplicatedTrayGetsUniqueNestedIds and deeplyNestedAppletsAreResolved. |
| 217 | `sharedConfigWindowFollowsEveryDockSwitch` | 5643-5709 | B3 | Settings windows follow the dock switch; needs live views. |
| 218 | `settingsWindowsUseRootInputsInsteadOfContextProperties` | 5711-5822 | K | QML root-input text in settings windows. |
| 219 | `settingsQmlRootsDeclareEveryInput` | 5824-5873 | K | QML root declarations in settings files. |
| 220 | `waylandWindowsAcceptedAfterCreationAreTracked` | 5875-5922 | B3 | Wayland window tracking; needs a Wayland display. |
| 221 | `viewBindingsKeepTheirValueWhileFrozen` | 5924-5969 | K | QML text in BindingsExternal.qml. |
| 222 | `layoutInfoWindowNeverAppliesAnEmptySize` | 5971-5987 | B2 | InfoView window size; needs a QML window fixture. |
| 223 | `appletAlternativesUseLattesOwnDialogOnPlasma65` | 5989-6040 | D | Duplicate of packageunittest appletAlternativesDialogMatchesPlasmaRelease; QML text remains. |
| 224 | `qmlParameterlessHandlersDoNotReadUndeclaredIndex` | 6042-6099 | K | Scans every QML root for undeclared index use. |
| 225 | `taskScrollMaskProvidesWindowAssociatedTexture` | 6101-6118 | K | QML text in task scroll mask files. |
| 226 | `dataRecordsKeepRuleOfZero` | 6120-6224 | B1 | Not converted: a single-class test cannot replace the scan of all app/data headers. Target: dataunittest. |
| 227 | `metaHoldPreferenceIsAnOptInThatTheDialogDoesNotForce` | 6226-6251 | B2 | Settings handler and dialog; needs a handler fixture. |
| 228 | `shortcutBadgeShadowEffectsAnchorToTheirLoader` | 6253-6275 | K | QML text in ShortcutBadge.qml. |
| 229 | `activitiesDelegatePaintSkipsUnknownActivityIds` | 6277-6292 | B2 | Delegate paint; needs a model index and painter fixture. |
