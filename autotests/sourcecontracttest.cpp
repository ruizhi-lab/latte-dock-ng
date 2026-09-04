/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QDir>
#include <QFile>
#include <QStringList>
#include <QTest>

#include "../app/session/shutdownstate.h"

class SourceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void plasmaVolumeBootstrapContractMovedToQmlSmokeTest();
    void compactAppletPopupSizingContractMovedToQmlSmokeTest();
    void applicationLauncherUsesFixedExternalSlot();
    void widgetsZoomCanBeDisabledIndependently();
    void animationsSpeedFactorMapsToDurationMultipliers();
    void effectsConfigAnimationButtonsMatchDurationValues();
    void appletShadowFadesInsteadOfPoppingWhileWidgetsZoom();
    void defaultTemplatesExcludeMailLauncher();
    void latteTasksExposesPlasmaLauncherApi();
    void latteDockDbusExportsLauncherApi();
    void plasmaKickerActionAddsLaunchersToLatteDock();
    void containmentClearsParabolicStateWhenEdgeChanges();
    void duplicateInstanceExitsWithoutQGuiAppExit();
    void allScreensCloneAppletSyncContracts();
    void layoutManagerRepairSkipsAppletsInDestruction();
    void layoutManagerScheduledDestructionTogglesItemVisibility();
    void cloneViewOrderSyncDropsStaleTargetEntries();
    void cloneViewRemovalSyncUsesSyncingFromOriginalGuard();
    void waylandStrutGhostWindowBindsLayerShellScreen();
    void launchersRestoreContractMovedToQmlSmokeTest();
    void sessionShutdownQuitDecisionMatrix_data();
    void sessionShutdownQuitDecisionMatrix();
    void sessionShutdownHandlingMatchesStableWaylandPath();
    void sessionShutdownPollerHidesViewsAndRestoresOnCancel();
    void itemsAlignmentIsSeparateAndJustifyOnly();
    void itemsAlignmentNormalizesDirectionsByFormFactor();
    void itemsAlignmentConfigDefaultsToCenter();
    void appearancePaletteExposesLayoutCustomColors();
    void modernDockBackgroundShadowDefaultIsCompact();
    void layoutDetailsExposeCustomColorSchemeSelector();
    void showWindowAnimationContractMovedToQmlSmokeTest();
    void parabolicItemContractMovedToQmlSmokeTest();
    void autotestAggregateTargetDocumentsFullSuiteBuild();
    void coverageEstimateUsesReusableScript();
    void cmakeTargetResolutionUsesSharedHelpers();
    void cmakeImportedTargetResolutionUsesSharedHelper();
    void cmakeTargetResolutionHelpersLiveInModule();
    void cmakeOffscreenTestsUseSharedHelper();
    void cmakeAutotestRegistrationMaintainsAggregateTarget();
    void cmakePackagingConfigLivesInModule();
    void autostartUsesXdgStateOnly();
    void desktopFileHasAutostartPhaseKey();
    void enableAutostartExitsImmediately();
    void autostartInterfacesUseXdgState();
    void autostartPreservesEntryFields();
    void firstRunEnablesDefaultAutostart();
    void waylandCheckHasRetryMechanism();
    void normalStartupDoesNotMutateAutostart();
    void autoSizeLoopsUseInequalityNotStrictEquality();
    void cmakeWarningRelaxationLivesInModule();
    void cmakeFindsQtCoreToolsBeforeKdeInstallDirs();
    void qtQuickGpuPreferenceKeepsSoftwareFallbackAvailable();
    void knsCompatImportsAreAvailableForSystemInstall();
    void layerShellSetScreenGuardPreventsBuildRegression();
    void genericLayoutUnloadUsesSynchronousDelete();
    void synchronizerUnloadUsesSynchronousDelete();
    void pointerWindowTrackingDisconnectsOnLeave();
    void viewDestructorDropsPointerWindowTrackingConnections();
    void taskIconsRefreshAfterIconThemeChanges();
    void taskAudioBadgesScaleWithParabolicZoom();
    void parabolicScaleAddressingFallsBackToLastValidIndexDuringRemoval();
    void widgetExplorerLaunchesKnsDialogOutOfProcess();
    void widgetExplorerUsesPlasmaTranslationContexts();
    void settingsDialogUiLabelsAreTranslatable();
    void poFilesHaveNoStaleDockPanelMsgids();
    void translationExtractionReferencesValidDirectories();
    void compactAppletDigitalClockWidthCapPreventsLongDateFormatOverflow();
    void contextMenuLayerMiddleClickCloseActiveWindowGuardedCorrectly();
    void appletContextMenuExposesKeepOriginalColorsToggle();
    void trashKeepOriginalColorsDefaultsToCheckedForAllConfigs();
    void appletIconOverrideStripsSymbolicForOriginalColors();
    void widgetOriginalIconColorsFallbackKeepsHoverAndStateSynchronized();
    void mouseHandlerAutoPinOnDragPromotesNonLauncherTasks();
    void scrollToggleMinimizedDownwardUnmaximizesBeforeMinimizing();
    void scrollToggleMinimizedUsesAllScreensTrackerForMinimizeAndMaximize();
    void systrayGuardsInAppletItemPreventLayoutAndInteractionBreakage();
    void volumeAndAppMenuPopupSizingUsesLargerMinimumInCompactApplet();
    void clipboardAndDigitalClockErrorSuppressionInMainCpp();
    void systemTrayAndPlasmoidActAsAppletInsertionBoundary();
    void separatorAndSpacerDetectionAndBehaviorInAppletItem();
    void separatorGuardsAcrossLayoutAndDragDropFiles();
    void separatorContentContainerStaysTightWithFallback();
    void restoreZoomTimerGracePeriodPreventsBoundaryBlip();
    void myViewClientIntPropertiesUseSafeIntGuardAgainstUndefined();
    void clonedViewDefersInitialAppletOrderSyncUntilStructuralReady();
    void indicatorFactoryExcludesBuiltinPluginsFromCustomLists();
    void waylandInterfaceAcceptableWindowHasHardcodedAppIdWhitelist();
    void dynamicWindowDotsAreOptInAndAggregateOverflow();
    void genericLayoutReassertsDefaultContextMenuOnContainmentWiring();
    void layoutManagerCleanupOnStartupRemovesLegacyAndGhostApplets();
    void containmentInterfaceHasSeparatorPluginConstantsAndHelper();
    void appletsModelHasNoPersonalDataAppletList();
    void lattePackageShellPluginNameBranchingIsCorrect();
    void indicatorPlasmaTypeIsRemappedToDefault();
    void appearanceConfigPreservesIndicatorStyleConfiguration();
    void mainCppMessageSuppressionCoversFrameworkWarnings();
    void appletItemInternalViewSplitterAndSortDragGuards();
    void appletItemFallbackTrackedWindowsAndConstraintHints();
    void plasmoidLaunchersAndDragDropHavePluginDetectionGuards();
    void containmentMainQmlHasPlasmoidWheelBypassGuards();
    void compactAppletFallbackSizingAndMinimumDimensionGuards();
    void layoutManagerShouldRetryGatedByMaxRetries();
    void mainCppSelfPipeSigintHandlerAndReplaceLockTimeout();
    // LayoutManager restore() boundary conditions
    void layoutManagerRestoreMaxRetryExhaustionFallsThrough();
    void layoutManagerRestoreBuildsOrderFromLiveAppletsWhenStoredEmpty();
    void layoutManagerRestoreInvalidAppletCleanupPreservesValidIds();
    void layoutManagerRestoreAppletContainerCreationRetryLimit();
    // VisibilityManager QML guard matrix
    void visibilityManagerSlotMustBeShownGuardsStartupAndMode();
    void visibilityManagerSlotMustBeHideGuardsMouseAndBlockHiding();
    void visibilityManagerUpdateMaskAreaClampingPreventsNegativeBounds();
    void visibilityManagerSlidingOutAnimationInitGatesInStartup();
    // Positioner boundary checks
    void positionerStartupPaintOffScreenAtNegative9999();
    void positionerSyncGeometrySuppressedDuringSlideAnimation();
    void positionerResizeWindowEnforcesWaylandMinimumSize();
    void positionerPlasmaPanelGeometryIntersectionGuardsEmptyList();
    // Storage boundary constants
    void storageAvailableIdUpperBoundExcludesId32000();
    void storageNewUniqueIdsFileCircularAssignmentDetection();
    void storageExpectedViewScreenIdAllSecondaryEmptyFallback();
    // Importer version detection
    void importerFileVersionDetectsLayoutVersion2AndConfigVersion1();
    void importerLayoutCanBeImportedRejectsVersionBelow2();
    void importerUniqueLayoutNameCollisionLoopHasGuard();
    // ScreenPool fallback paths
    void screenPoolScreenForIdFallsBackToPrimaryWhenScreenAbsent();
    void plasmaExtendedScreenPoolIdReturnsZeroForPrimaryConnector();
    void screenPoolIsScreenActiveReturnsFalseForStaleDisconnectedId();
    // Launchers QML boundary conditions
    void launchersNormalizeLauncherListExpandsOnlyLength2to4();
    void launchersFreeSeparatorNameExhaustionReturnsEmptyString();
    void launchersTransientEmptyRecoveryCeilingAt8();
    void launchersIsSeparatorGuardsDesktopExtensionPosition();
    void launchersAddDroppedLauncherIconDataTruncationBoundary();
    void launchersRemoveInternalSeparatorRefusesEmptyUrl();
    void taskItemIsSeparatorNeverTurnsUrlLessWindowsIntoDividers();
    void contextMenuSeparatorRemovalRoutesThroughGuardedLauncherPath();
    // main.cpp CLI and startup boundary paths
    void mainCppAvailableLayoutsPrintsDifferentMessageWhenEmpty();
    void mainCppLayoutOptionExitsForMissingLayout();
    void mainCppClearCacheSkipsNonexistentDirectory();
    void mainCppImportLayoutExitsOnImportFailure();
    void mainCppDeferredDeleteDrainHardLimit5Passes();
    void mainCppDetectPlatformPreservesExplicitPlatformArg();
    // main.qml startup sequence contacts
    void mainQmlInStartupSetFalseInSlidingOutAnimationOnStopped();
    void mainQmlStartupDelayerTriggeredByHasRestoredAppletsSignal();
    void mainQmlCreateAppletItemRetryCeilingAt80();
    void mainQmlPanelCfgSyncTransparencySevenInputClasses();
    void mainQmlOnInStartupChangedMustCheckLatteViewExists();
    // itemForApplet migration — safe contexts vs regression-prone exclusions
    void appletItemForAppletInSafeContextsUsesPublicApi();
    void appletItemForAppletExcludedContextsPreservePropertyAccess();
    // Qt5→Qt6 migration guards — patterns that cause regressions
    void mouseButtonEnumUsesMiddleButtonNotMidButton();
    void taskMouseAreaSkipsInactivePreviewChecks();
    void dragDropHandlersUseBindingSyntaxForQt6();
    void environmentActionsDoesNotAcceptMiddleButton();
    void upgraderQmlUsesPlasmoidConfiguration();
    void middleClickActionDefaultIsClose();

    // Infinite-loop guard contracts
    void tasktoolsServicesFromCmdLineGuardsFirstSpace();
    void importerUniqueLayoutNameHasIterationCap();
    void viewscontrollerUniqueViewNameHasIterationCap();
    void layoutscontrollerUniqueLayoutNameHasIterationCap();
    void lattecoronaUnloadUsesCappedLiveListLoop();
    void lattecoronaScreenForContainmentHasDepthGuard();
    void originalViewCleanClonesDrainsFromLocalCopy();
    void layoutManagerResolveAppletQuickItemUsesVisitedSet();
    void levelOptionsHasIsBackgroundIsForegroundReentryGuards();

    // Guard-fix follow-up contracts
    void templatesmanagerUniqueNameLoopsHaveIterationCaps();
    void synchronizerUnloadLayoutsHasIterationCap();
    void uniqueNameExhaustionFallsBackToRandomSuffix();
    void layoutManagerResolveAppletQuickItemThreadsVisitedSet();
    void appdataComponentIdKeepsHyphenInLastSegment();
    void positionShortcutHandlersDeclareSignalParameters();
    void positionShortcutHostLookupIsRecursiveAndResettable();
    void qmlCacheRevisionInvalidatesSameVersionBuilds();

    // Applet menu / popup contracts
    void needsAttentionStatusBlocksHidingWithoutWindowReconfiguration();
    void appletPopupHidesOnWindowDeactivateByDefault();
    void viewTracksPointerWindowsAndResetsCascadingSubmenus();
    void pointerWindowTrackerRemovesDestroyedWindows();
};

void SourceContractTest::plasmaVolumeBootstrapContractMovedToQmlSmokeTest()
{
    QFile qmlSmoke(QStringLiteral(LATTE_SOURCE_DIR "/autotests/qmlsmoketest.cpp"));
    QVERIFY(qmlSmoke.open(QFile::ReadOnly));
    const QString qmlSmokeSource = QString::fromUtf8(qmlSmoke.readAll());
    QVERIFY(qmlSmokeSource.contains(QStringLiteral("plasmaVolumeBootstrapLoadsFromSource")));
    QVERIFY(qmlSmokeSource.contains(QStringLiteral("LATTE_PULSEAUDIO_QML")));
    QVERIFY(qmlSmokeSource.contains(QStringLiteral("org/kde/plasma/private/volume")));
    QVERIFY(qmlSmokeSource.contains(QStringLiteral("bootstrapMaxAttempts")));
    QVERIFY(qmlSmokeSource.contains(QStringLiteral("paFixTimer")));

    QFile sourceContracts(QStringLiteral(LATTE_SOURCE_DIR "/autotests/sourcecontracttest.cpp"));
    QVERIFY(sourceContracts.open(QFile::ReadOnly));
    const QString sourceContractSource = QString::fromUtf8(sourceContracts.readAll());
    const QString oldSourceLock = QStringLiteral("QFile ") + QStringLiteral("pulseAudio");
    QVERIFY(!sourceContractSource.contains(oldSourceLock));
}

void SourceContractTest::compactAppletPopupSizingContractMovedToQmlSmokeTest()
{
    QFile qmlSmoke(QStringLiteral(LATTE_SOURCE_DIR "/autotests/qmlsmoketest.cpp"));
    QVERIFY(qmlSmoke.open(QFile::ReadOnly));
    const QString qmlSmokeSource = QString::fromUtf8(qmlSmoke.readAll());
    QVERIFY(qmlSmokeSource.contains(QStringLiteral("compactAppletPopupSizingLoadsFromSource")));
    QVERIFY(qmlSmokeSource.contains(QStringLiteral("LATTE_COMPACT_APPLET_QML")));
    QVERIFY(qmlSmokeSource.contains(QStringLiteral("popupPreferredWidth")));
    QVERIFY(qmlSmokeSource.contains(QStringLiteral("popupMaximumWidth")));

    QFile sourceContracts(QStringLiteral(LATTE_SOURCE_DIR "/autotests/sourcecontracttest.cpp"));
    QVERIFY(sourceContracts.open(QFile::ReadOnly));
    const QString sourceContractSource = QString::fromUtf8(sourceContracts.readAll());
    const QString oldSourceLock = QStringLiteral("QFile ") + QStringLiteral("compactApplet");
    QVERIFY(!sourceContractSource.contains(oldSourceLock));
}

void SourceContractTest::applicationLauncherUsesFixedExternalSlot()
{
    QFile appletItem(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/applet/AppletItem.qml"));
    QVERIFY(appletItem.open(QFile::ReadOnly));

    const QString source = QString::fromUtf8(appletItem.readAll());
    QVERIFY(source.contains(QStringLiteral("isApplicationLauncherApplet")));
    QVERIFY(source.contains(QStringLiteral("org.kde.plasma.kickoff")));
    QVERIFY(source.contains(QStringLiteral("|| (!communicator.appletMainIconIsFound")));
}

void SourceContractTest::widgetsZoomCanBeDisabledIndependently()
{
    // The user-facing toggle that lets widgets stay sharp (no raster-scale
    // zoom) while task icons/launchers keep zooming. It must exist in the
    // containment config, be wired into the applet zoom lock, and be shown
    // in the shell Behavior settings.
    QFile configFile(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/config/main.xml"));
    QVERIFY(configFile.open(QFile::ReadOnly));
    const QString config = QString::fromUtf8(configFile.readAll());
    QVERIFY(config.contains(QStringLiteral("appletsZoomEnabled")));

    QFile appletItem(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/applet/AppletItem.qml"));
    QVERIFY(appletItem.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(appletItem.readAll());
    QVERIFY(source.contains(QStringLiteral("plasmoid.configuration.appletsZoomEnabled")));
    QVERIFY(source.contains(QStringLiteral("appletZoomIsLocked")));
    // The lock must only apply to external widgets: Latte's own tasks
    // plasmoid must keep relaying the parabolic wave so neighbouring task
    // icons restore correctly at the widget boundary.
    QVERIFY(source.contains(QStringLiteral("!appletsZoomEnabled && isExternalPlasmaApplet")));

    QFile parabolicArea(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/applet/ParabolicArea.qml"));
    QVERIFY(parabolicArea.open(QFile::ReadOnly));
    const QString parabolicSource = QString::fromUtf8(parabolicArea.readAll());
    QVERIFY(parabolicSource.contains(QStringLiteral("appletItem.appletZoomIsLocked")));

    QFile behaviorConfig(QStringLiteral(LATTE_SOURCE_DIR "/shell/package/contents/configuration/pages/BehaviorConfig.qml"));
    QVERIFY(behaviorConfig.open(QFile::ReadOnly));
    const QString behaviorSource = QString::fromUtf8(behaviorConfig.readAll());
    QVERIFY(behaviorSource.contains(QStringLiteral("plasmoid.configuration.appletsZoomEnabled")));
}

void SourceContractTest::animationsSpeedFactorMapsToDurationMultipliers()
{
    // Issue #39: the containment speedFactor must map durationTime x1/x2/x3
    // to 1x/2x/3x durations so the animation speed setting is actually
    // perceptible, and the config default must be x1 (normal).
    QFile animations(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/abilities/Animations.qml"));
    QVERIFY(animations.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(animations.readAll());
    QVERIFY(source.contains(QStringLiteral("case 1:")));
    QVERIFY(source.contains(QStringLiteral("return 1.0;")));
    QVERIFY(source.contains(QStringLiteral("case 2:")));
    QVERIFY(source.contains(QStringLiteral("return 2.0;")));
    QVERIFY(source.contains(QStringLiteral("case 3:")));
    QVERIFY(source.contains(QStringLiteral("return 3.0;")));
    QVERIFY(!source.contains(QStringLiteral("return 0.75;")));
    QVERIFY(!source.contains(QStringLiteral("return 1.15;")));

    QFile config(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/config/main.xml"));
    QVERIFY(config.open(QFile::ReadOnly));
    const QString configSource = QString::fromUtf8(config.readAll());
    QVERIFY(configSource.contains(QStringLiteral("<default>1</default>")));
}

void SourceContractTest::effectsConfigAnimationButtonsMatchDurationValues()
{
    // Issue #39: the shell Effects settings x1/x2/x3 buttons must set the
    // matching durationTime enum value (1/2/3) — they were previously
    // inverted, so "x1" actually selected the slowest duration.
    QFile effects(QStringLiteral(LATTE_SOURCE_DIR "/shell/package/contents/configuration/pages/EffectsConfig.qml"));
    QVERIFY(effects.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(effects.readAll());

    const int x1 = source.indexOf(QStringLiteral("text: i18n(\"x1\")"));
    QVERIFY(x1 >= 0);
    QVERIFY(source.mid(x1).contains(QStringLiteral("readonly property int duration: 1")));

    const int x2 = source.indexOf(QStringLiteral("text: i18n(\"x2\")"));
    QVERIFY(x2 >= 0);
    QVERIFY(source.mid(x2).contains(QStringLiteral("readonly property int duration: 2")));

    const int x3 = source.indexOf(QStringLiteral("text: i18n(\"x3\")"));
    QVERIFY(x3 >= 0);
    QVERIFY(source.mid(x3).contains(QStringLiteral("readonly property int duration: 3")));
}

void SourceContractTest::appletShadowFadesInsteadOfPoppingWhileWidgetsZoom()
{
    // Issue #38: Qt6 MultiEffect composites a full-color copy of its source,
    // so while fixed-slot widgets zoom the effect would show the original-size
    // copy through transparent icons. The shadow must fade out (not pop) so
    // the zoom transition stays fluid.
    QFile itemWrapper(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/applet/ItemWrapper.qml"));
    QVERIFY(itemWrapper.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(itemWrapper.readAll());
    QVERIFY(source.contains(QStringLiteral("id: appletShadow")));
    QVERIFY(source.contains(QStringLiteral("opacity: (wrapper.zoomScale === 1 || !appletItem.externalAppletUsesFixedSlotSizing) ? 1 : 0")));
    QVERIFY(source.contains(QStringLiteral("Behavior on opacity")));
}

void SourceContractTest::defaultTemplatesExcludeMailLauncher()
{
    // The default layout templates must not include the mail launcher.
    QFile layout(QStringLiteral(LATTE_SOURCE_DIR "/shell/package/contents/templates/Default.layout.latte"));
    QVERIFY(layout.open(QFile::ReadOnly));
    const QString layoutSource = QString::fromUtf8(layout.readAll());
    QVERIFY(!layoutSource.contains(QStringLiteral("thunderbird")));

    QFile dockView(QStringLiteral(LATTE_SOURCE_DIR "/shell/package/contents/templates/Default Dock.view.latte"));
    QVERIFY(dockView.open(QFile::ReadOnly));
    const QString dockSource = QString::fromUtf8(dockView.readAll());
    QVERIFY(!dockSource.contains(QStringLiteral("thunderbird")));
}

void SourceContractTest::latteTasksExposesPlasmaLauncherApi()
{
    QFile latteTasks(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/main.qml"));
    QVERIFY(latteTasks.open(QFile::ReadOnly));

    const QString source = QString::fromUtf8(latteTasks.readAll());
    QVERIFY(source.contains(QStringLiteral("readonly property bool supportsLaunchers: true")));
    QVERIFY(source.contains(QStringLiteral("function hasLauncher(url)")));
    QVERIFY(source.contains(QStringLiteral("appletAbilities.launchers.hasLauncher(url)")));
    QVERIFY(source.contains(QStringLiteral("function addLauncher(url)")));
    QVERIFY(source.contains(QStringLiteral("appletAbilities.launchers.addLauncher(url)")));
    QVERIFY(source.contains(QStringLiteral("function removeLauncher(url)")));
    QVERIFY(source.contains(QStringLiteral("appletAbilities.launchers.removeLauncher(url)")));
}

void SourceContractTest::latteDockDbusExportsLauncherApi()
{
    QFile dbusXml(QStringLiteral(LATTE_SOURCE_DIR "/app/dbus/org.kde.LatteDock.xml"));
    QVERIFY(dbusXml.open(QFile::ReadOnly));

    const QString xml = QString::fromUtf8(dbusXml.readAll());
    QVERIFY(xml.contains(QStringLiteral("<method name=\"hasLauncher\">")));
    QVERIFY(xml.contains(QStringLiteral("<method name=\"addLauncher\">")));
    QVERIFY(xml.contains(QStringLiteral("<method name=\"removeLauncher\">")));
    QVERIFY(xml.contains(QStringLiteral("<arg name=\"launcherUrl\" type=\"s\" direction=\"in\"/>")));
    QVERIFY(xml.contains(QStringLiteral("<arg name=\"screenName\" type=\"s\" direction=\"in\"/>")));
    QVERIFY(xml.contains(QStringLiteral("<arg name=\"success\" type=\"b\" direction=\"out\"/>")));

    QFile coronaHeader(QStringLiteral(LATTE_SOURCE_DIR "/app/lattecorona.h"));
    QVERIFY(coronaHeader.open(QFile::ReadOnly));

    const QString header = QString::fromUtf8(coronaHeader.readAll());
    QVERIFY(header.contains(QStringLiteral("bool hasLauncher(QString launcherUrl, QString screenName);")));
    QVERIFY(header.contains(QStringLiteral("bool addLauncher(QString launcherUrl, QString screenName);")));
    QVERIFY(header.contains(QStringLiteral("bool removeLauncher(QString launcherUrl, QString screenName);")));
}

void SourceContractTest::plasmaKickerActionAddsLaunchersToLatteDock()
{
    QFile kickerAction(QStringLiteral(LATTE_SOURCE_DIR "/app/org.kde.latte-dock.kickeractions.desktop.cmake"));
    QVERIFY(kickerAction.open(QFile::ReadOnly));
    const QString desktop = QString::fromUtf8(kickerAction.readAll());
    QVERIFY(desktop.contains(QStringLiteral("Type=Service")));
    QVERIFY(desktop.contains(QStringLiteral("Actions=addToLatteDock")));
    QVERIFY(desktop.contains(QStringLiteral("[Desktop Action addToLatteDock]")));
    QVERIFY(desktop.contains(QStringLiteral("Exec=@CMAKE_INSTALL_PREFIX@/bin/latte-dock-ng-add-launcher %u")));
    QVERIFY(desktop.contains(QStringLiteral("Name=Add to Latte Dock")));
    QVERIFY(desktop.contains(QStringLiteral("Name[zh_CN]=添加到 Latte 停靠栏")));
    QVERIFY(desktop.contains(QStringLiteral("Name[fr]=Ajouter au dock Latte")));

    QFile appCMake(QStringLiteral(LATTE_SOURCE_DIR "/app/CMakeLists.txt"));
    QVERIFY(appCMake.open(QFile::ReadOnly));
    const QString cmake = QString::fromUtf8(appCMake.readAll());
    QVERIFY(cmake.contains(QStringLiteral("add_executable(latte-dock-ng-add-launcher launcherhelper.cpp)")));
    QVERIFY(cmake.contains(QStringLiteral("target_link_libraries(latte-dock-ng-add-launcher Qt6::Core Qt6::DBus)")));
    QVERIFY(cmake.contains(QStringLiteral("install(TARGETS latte-dock-ng-add-launcher ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})")));
    QVERIFY(cmake.contains(QStringLiteral("configure_file(org.kde.latte-dock.kickeractions.desktop.cmake org.kde.latte-dock.kickeractions.desktop)")));
    QVERIFY(cmake.contains(QStringLiteral("DESTINATION ${KDE_INSTALL_DATADIR}/plasma/kickeractions")));
    QVERIFY(cmake.contains(QStringLiteral("LATTE_INSTALL_USER_KICKERACTION_EXECUTABLE")));
    QVERIFY(cmake.contains(QStringLiteral("set(latte_kickeraction_permissions OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)")));
    QVERIFY(cmake.contains(QStringLiteral("OWNER_EXECUTE")));
    QVERIFY(cmake.contains(QStringLiteral("GROUP_EXECUTE")));
    QVERIFY(cmake.contains(QStringLiteral("WORLD_EXECUTE")));

    QFile installScript(QStringLiteral(LATTE_SOURCE_DIR "/install.sh"));
    QVERIFY(installScript.open(QFile::ReadOnly));
    const QString installSource = QString::fromUtf8(installScript.readAll());
    QVERIFY(installSource.contains(QStringLiteral("-DLATTE_INSTALL_USER_KICKERACTION_EXECUTABLE=ON")));
    QVERIFY(installSource.contains(QStringLiteral("-DLATTE_INSTALL_USER_KICKERACTION_EXECUTABLE=OFF")));

    QFile helperSourceFile(QStringLiteral(LATTE_SOURCE_DIR "/app/launcherhelper.cpp"));
    QVERIFY(helperSourceFile.open(QFile::ReadOnly));
    const QString helperSource = QString::fromUtf8(helperSourceFile.readAll());
    QVERIFY(helperSource.contains(QStringLiteral("QCoreApplication app(argc, argv);")));
    QVERIFY(helperSource.contains(QStringLiteral("QStringLiteral(\"addLauncher\")")));
    QVERIFY(helperSource.contains(QStringLiteral("org.kde.lattedock")));
    QVERIFY(helperSource.contains(QStringLiteral("/Latte")));
    QVERIFY(!helperSource.contains(QStringLiteral("QApplication")));
}

void SourceContractTest::containmentClearsParabolicStateWhenEdgeChanges()
{
    QFile containment(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/main.qml"));
    QVERIFY(containment.open(QFile::ReadOnly));

    const QString source = QString::fromUtf8(containment.readAll());
    QVERIFY(source.contains(QStringLiteral("function onLocationChanged() {\n            root.resetModernParabolicOffsets();")));
    QVERIFY(source.contains(QStringLiteral("function onFormFactorChanged() {\n            root.resetModernParabolicOffsets();")));
    QVERIFY(source.contains(QStringLiteral("function onShowingAfterRelocationFinished() {\n            root.resetModernParabolicOffsets();")));
}

void SourceContractTest::duplicateInstanceExitsWithoutQGuiAppExit()
{
    QFile mainSource(QStringLiteral(LATTE_SOURCE_DIR "/app/main.cpp"));
    QVERIFY(mainSource.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(mainSource.readAll());

    const int lockFail = source.indexOf(QStringLiteral("if (!lockFile.tryLock(timeout)) {"));
    const int clearCache = source.indexOf(QStringLiteral("//! clear-cache option"), lockFail);
    QVERIFY(lockFail >= 0);
    QVERIFY(clearCache > lockFail);

    const QString duplicateInstanceBlock = source.mid(lockFail, clearCache - lockFail);
    QVERIFY(!duplicateInstanceBlock.contains(QStringLiteral("qGuiApp->exit();")));
    QVERIFY(!duplicateInstanceBlock.contains(QStringLiteral("SharedQmlEngine")));
    QVERIFY(duplicateInstanceBlock.contains(QStringLiteral("i18n(\"An instance is already running!, use --replace to restart Latte\")")));
    QVERIFY(duplicateInstanceBlock.contains(QStringLiteral("return 0;")));
}

void SourceContractTest::allScreensCloneAppletSyncContracts()
{
    QFile interfaceHeader(QStringLiteral(LATTE_SOURCE_DIR "/app/view/containmentinterface.h"));
    QVERIFY(interfaceHeader.open(QFile::ReadOnly));
    const QString interfaceHeaderSource = QString::fromUtf8(interfaceHeader.readAll());
    QVERIFY(interfaceHeaderSource.contains(QStringLiteral("Q_INVOKABLE void suppressNextAppletCreatedSignal();")));
    QVERIFY(interfaceHeaderSource.contains(QStringLiteral("int m_suppressedAppletCreations{0};")));
    QVERIFY(interfaceHeaderSource.contains(QStringLiteral("bool m_initializationCompleted{false};")));
    QVERIFY(interfaceHeaderSource.contains(QStringLiteral("bool isInitialized() const;")));
    QVERIFY(interfaceHeaderSource.contains(QStringLiteral("QStringList provides;")));

    QFile interfaceSource(QStringLiteral(LATTE_SOURCE_DIR "/app/view/containmentinterface.cpp"));
    QVERIFY(interfaceSource.open(QFile::ReadOnly));
    const QString interfaceCpp = QString::fromUtf8(interfaceSource.readAll());
    QVERIFY(interfaceCpp.contains(QStringLiteral("suppressNextAppletCreatedSignal();\n    Plasma::Applet *createdApplet = m_view->containment()->createApplet(pluginId);")));
    QVERIFY(!interfaceCpp.contains(QStringLiteral("Latte::Layouts::Importer::standardPaths();\n    QString pluginpath;")));
    QVERIFY(interfaceCpp.contains(QStringLiteral("Q_EMIT appletCreated(currentPluginId);")));
    // Since ed0afd054 a missing cached AppletQuickItem falls back to
    // itemForApplet() resolution instead of bailing out with nullptr.
    QVERIFY(interfaceCpp.contains(QStringLiteral("if (!ai) {\n                ai = PlasmaQuick::AppletQuickItem::itemForApplet(m_appletData[id].applet);\n            }")));

    const int trackAllAppletsComment = interfaceCpp.indexOf(QStringLiteral("//! Track all applets, for example to support syncing between different docks"));
    const int trackAllAppletsData = interfaceCpp.indexOf(QStringLiteral("ViewPart::AppletInterfaceData data;"), trackAllAppletsComment);
    const int previousAiOnlyBlock = interfaceCpp.lastIndexOf(QStringLiteral("if (ai) {"), trackAllAppletsData);
    QVERIFY(trackAllAppletsComment >= 0);
    QVERIFY(trackAllAppletsData > trackAllAppletsComment);
    QVERIFY(previousAiOnlyBlock < trackAllAppletsComment);

    QFile dragDropArea(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/DragDropArea.qml"));
    QVERIFY(dragDropArea.open(QFile::ReadOnly));
    const QString dragDropSource = QString::fromUtf8(dragDropArea.readAll());
    const int suppressSync = dragDropSource.indexOf(QStringLiteral("latteView.extendedInterface.suppressNextAppletCreatedSignal();"));
    const int droppedSync = dragDropSource.indexOf(QStringLiteral("latteView.extendedInterface.appletDropped(event.mimeData, eventx, eventy);"), suppressSync);
    QVERIFY(suppressSync >= 0);
    QVERIFY(droppedSync > suppressSync);

    QFile clonedView(QStringLiteral(LATTE_SOURCE_DIR "/app/view/clonedview.cpp"));
    QVERIFY(clonedView.open(QFile::ReadOnly));
    const QString clonedViewSource = QString::fromUtf8(clonedView.readAll());
    QVERIFY(clonedViewSource.contains(QStringLiteral("onCloneAppletRemoved")));
    QVERIFY(clonedViewSource.contains(QStringLiteral("onCloneAppletsOrderChanged")));
    QVERIFY(clonedViewSource.contains(QStringLiteral("translateToOriginalsOrder")));
    QVERIFY(clonedViewSource.contains(QStringLiteral("orderWithUnmappedAppletsPreserved")));
    QVERIFY(clonedViewSource.contains(QStringLiteral("structuralSyncReady")));
    QVERIFY(clonedViewSource.contains(QStringLiteral("bool ClonedView::refreshAppletIdsHash()")));
    QVERIFY(clonedViewSource.contains(QStringLiteral("if (refreshAppletIdsHash()) {\n            onOriginalAppletsOrderChanged();\n        }")));
    QVERIFY(clonedViewSource.contains(QStringLiteral("extendedInterface()->addApplet(pluginId);\n        m_syncingFromOriginal = false;\n        onOriginalAppletsOrderChanged();")));
    QVERIFY(clonedViewSource.contains(QStringLiteral("m_originalView->addApplet(data, x, y, containment()->id());\n        onCloneAppletsOrderChanged();")));
    QVERIFY(clonedViewSource.contains(QStringLiteral("extendedInterface()->addApplet(data, x, y);\n        m_syncingFromOriginal = false;\n        onOriginalAppletsOrderChanged();")));
    QVERIFY(clonedViewSource.contains(QStringLiteral("data.provides.contains(QLatin1String(Latte::PluginId::kLauncherMenu))")));
    QVERIFY(clonedViewSource.contains(QStringLiteral("m_cloneRemovalsFromOriginal")));

    // Commit 8bc9c0e fix 1: onOriginalAppletRemoved wraps removeApplet with
    // m_syncingFromOriginal so the clone-removed handler does not
    // bounce the removal back to the original view.
    const int syncingTrue = clonedViewSource.indexOf(QStringLiteral("m_syncingFromOriginal = true;"));
    const int removeAppletCall = clonedViewSource.indexOf(QStringLiteral("extendedInterface()->removeApplet(clonedid);"), syncingTrue);
    const int syncingFalse = clonedViewSource.indexOf(QStringLiteral("m_syncingFromOriginal = false;"), removeAppletCall);
    QVERIFY(syncingTrue >= 0);
    QVERIFY(removeAppletCall > syncingTrue);
    QVERIFY(syncingFalse > removeAppletCall);

    // Commit 8bc9c0e fix 2: orderWithUnmappedAppletsPreserved drops
    // mapped-but-removed target entries instead of leaving stale IDs.
    QVERIFY(clonedViewSource.contains(QStringLiteral("// Mapped but no translated source entry: the applet was removed")));
    QVERIFY(clonedViewSource.contains(QStringLiteral("// on the source side — drop it from the result.")));

    // Commit 8bc9c0e fix 3: setAppletInScheduledDestruction immediately
    // hides/shows the item on all screens.
    // Verify in the layoutmanager source (not clonedview — the visibility
    // toggle is in LayoutManager::setAppletInScheduledDestruction).
    QFile layoutManagerSourceFile(QStringLiteral(LATTE_SOURCE_DIR "/containment/plugin/layoutmanager.cpp"));
    QVERIFY(layoutManagerSourceFile.open(QFile::ReadOnly));
    const QString layoutManagerSource = QString::fromUtf8(layoutManagerSourceFile.readAll());

    QVERIFY(layoutManagerSource.contains(QStringLiteral("item->setVisible(false);")));
    QVERIFY(layoutManagerSource.contains(QStringLiteral("item->setVisible(true);")));
    QVERIFY(layoutManagerSource.contains(QStringLiteral("// Immediately hide the item so that the widget disappears on")));
    QVERIFY(layoutManagerSource.contains(QStringLiteral("// Undo: re-show the item that was hidden when destruction was")));

    // Commit 8bc9c0e fix 3: repairAppletContainers skips applets being
    // destroyed to avoid re-creating UI containers for dying applets.
    QVERIFY(layoutManagerSource.contains(QStringLiteral("m_appletsInScheduledDestruction.contains(id)")));
    QVERIFY(layoutManagerSource.contains(QStringLiteral("backendApplet && backendApplet->destroyed()")));

    const int scheduledGuard = layoutManagerSource.indexOf(QStringLiteral("if (m_appletsInScheduledDestruction.contains(id)) {"));
    const int destroyedGuard = layoutManagerSource.indexOf(QStringLiteral("if (backendApplet && backendApplet->destroyed()) {"), scheduledGuard);
    QVERIFY(scheduledGuard >= 0);
    QVERIFY(destroyedGuard > scheduledGuard);
}

void SourceContractTest::layoutManagerRepairSkipsAppletsInDestruction()
{
    QFile source(QStringLiteral(LATTE_SOURCE_DIR "/containment/plugin/layoutmanager.cpp"));
    QVERIFY(source.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(source.readAll());

    // Verify repairAppletContainers() skips applets being destroyed.
    // Returns bool since 452fa2d82 so callers can stop early when no
    // repair work was performed.
    QVERIFY(src.contains(QStringLiteral("bool LayoutManager::repairAppletContainers()")));

    // Guard 1: skip applets in m_appletsInScheduledDestruction.
    QVERIFY(src.contains(QStringLiteral("if (m_appletsInScheduledDestruction.contains(id)) {\n            continue;\n        }")));

    // Guard 2: skip applets whose Plasma::Applet reports destroyed().
    QVERIFY(src.contains(QStringLiteral("if (backendApplet && backendApplet->destroyed()) {\n                continue;\n            }")));

    // The two guards must appear in the correct order: scheduled-destruction
    // check first, then the general destroyed() check.
    const int scheduledGuard = src.indexOf(QStringLiteral("m_appletsInScheduledDestruction.contains(id)"));
    const int destroyedGuard = src.indexOf(QStringLiteral("backendApplet->destroyed()"), scheduledGuard);
    QVERIFY(scheduledGuard >= 0);
    QVERIFY(destroyedGuard > scheduledGuard);
}

void SourceContractTest::layoutManagerScheduledDestructionTogglesItemVisibility()
{
    QFile source(QStringLiteral(LATTE_SOURCE_DIR "/containment/plugin/layoutmanager.cpp"));
    QVERIFY(source.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(source.readAll());

    // Verify setAppletInScheduledDestruction() toggles item visibility.
    QVERIFY(src.contains(QStringLiteral("void LayoutManager::setAppletInScheduledDestruction(const int &id, const bool &enabled)")));

    // Enabled path: item saved before setVisible(false).
    const int enabledPath = src.indexOf(QStringLiteral("} else if (!m_appletsInScheduledDestruction.contains(id) && enabled) {"));
    QVERIFY(enabledPath >= 0);

    const int assignItem = src.indexOf(QStringLiteral("m_appletsInScheduledDestruction[id] = item;"), enabledPath);
    const int hideCall = src.indexOf(QStringLiteral("item->setVisible(false);"), assignItem);
    QVERIFY(assignItem > enabledPath);
    QVERIFY(hideCall > assignItem);

    // Disabled path: item retrieved then setVisible(true).
    const int disabledPath = src.indexOf(QStringLiteral("if (m_appletsInScheduledDestruction.contains(id) && !enabled) {"));
    QVERIFY(disabledPath >= 0);

    const int retrieveItem = src.indexOf(QStringLiteral("m_appletsInScheduledDestruction.value(id)"), disabledPath);
    const int showCall = src.indexOf(QStringLiteral("item->setVisible(true);"), retrieveItem);
    QVERIFY(retrieveItem > disabledPath);
    QVERIFY(showCall > retrieveItem);

    // The hide comment must still be present (regression lock).
    QVERIFY(src.contains(QStringLiteral("// Immediately hide the item so that the widget disappears on")));
    QVERIFY(src.contains(QStringLiteral("// Undo: re-show the item that was hidden when destruction was")));
}

void SourceContractTest::cloneViewOrderSyncDropsStaleTargetEntries()
{
    QFile source(QStringLiteral(LATTE_SOURCE_DIR "/app/view/clonedview.cpp"));
    QVERIFY(source.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(source.readAll());

    // Verify orderWithUnmappedAppletsPreserved() drops stale entries.
    QVERIFY(src.contains(QStringLiteral("QList<int> ClonedView::orderWithUnmappedAppletsPreserved")));

    // The drop-comment must exist (documents the intentional skip).
    QVERIFY(src.contains(QStringLiteral("// Mapped but no translated source entry: the applet was removed")));
    QVERIFY(src.contains(QStringLiteral("// on the source side — drop it from the result.")));

    // The else-if branch that emits translated entries must come before the
    // implicit drop (the drop is the else case, with only a comment).
    const int mappedCheck = src.indexOf(QStringLiteral("} else if (translatedIndex < translated.count()) {"));
    const int dropComment = src.indexOf(QStringLiteral("// Mapped but no translated source entry: the applet was removed"), mappedCheck);
    QVERIFY(mappedCheck >= 0);
    QVERIFY(dropComment > mappedCheck);

    // The appended-remaining block must exist for newly added applets.
    QVERIFY(src.contains(QStringLiteral("// Append any remaining translated entries (newly added applets).")));
}

void SourceContractTest::cloneViewRemovalSyncUsesSyncingFromOriginalGuard()
{
    QFile source(QStringLiteral(LATTE_SOURCE_DIR "/app/view/clonedview.cpp"));
    QVERIFY(source.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(source.readAll());

    // Verify onOriginalAppletRemoved() uses m_syncingFromOriginal.
    QVERIFY(src.contains(QStringLiteral("void ClonedView::onOriginalAppletRemoved(const int &id)")));

    // The syncing flag must be:
    //   1. Set to true BEFORE removeApplet.
    //   2. Set to false AFTER removeApplet.
    const int funcStart = src.indexOf(QStringLiteral("void ClonedView::onOriginalAppletRemoved(const int &id)"));
    QVERIFY(funcStart >= 0);

    const int syncingTrue = src.indexOf(QStringLiteral("m_syncingFromOriginal = true;"), funcStart);
    const int removeAppletCall = src.indexOf(QStringLiteral("extendedInterface()->removeApplet(clonedid);"), syncingTrue);
    const int syncingFalse = src.indexOf(QStringLiteral("m_syncingFromOriginal = false;"), removeAppletCall);
    const int funcEnd = src.indexOf(QStringLiteral("void ClonedView::onOriginalAppletConfigPropertyChanged"), syncingFalse);

    QVERIFY(syncingTrue > funcStart);
    QVERIFY(removeAppletCall > syncingTrue);
    QVERIFY(syncingFalse > removeAppletCall);
    QVERIFY(funcEnd > syncingFalse || funcEnd < 0);
}

void SourceContractTest::waylandStrutGhostWindowBindsLayerShellScreen()
{
    QFile waylandSource(QStringLiteral(LATTE_SOURCE_DIR "/app/wm/waylandinterface.cpp"));
    QVERIFY(waylandSource.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(waylandSource.readAll());

    const int qwindowScreen = source.indexOf(QStringLiteral("setScreen(screen);"));
    const int layerWindow = source.indexOf(QStringLiteral("auto *layerWindow = LayerShellQt::Window::get(this);"), qwindowScreen);
    const int layerScreen = source.indexOf(QStringLiteral("layerWindow->setScreen(screen);"), layerWindow);

    QVERIFY(qwindowScreen >= 0);
    QVERIFY(layerWindow > qwindowScreen);
    QVERIFY(layerScreen > layerWindow);
}

void SourceContractTest::launchersRestoreContractMovedToQmlSmokeTest()
{
    QFile qmlSmoke(QStringLiteral(LATTE_SOURCE_DIR "/autotests/qmlsmoketest.cpp"));
    QVERIFY(qmlSmoke.open(QFile::ReadOnly));
    const QString qmlSmokeSource = QString::fromUtf8(qmlSmoke.readAll());
    QVERIFY(qmlSmokeSource.contains(QStringLiteral("launchersGeometryRestoreSchedulingLoadsFromSource")));
    QVERIFY(qmlSmokeSource.contains(QStringLiteral("LATTE_LAUNCHERS_QML")));
    QVERIFY(qmlSmokeSource.contains(QStringLiteral("scheduleLaunchersRestore")));
    QVERIFY(qmlSmokeSource.contains(QStringLiteral("launchersRestoreFinalTimer")));

    QFile sourceContracts(QStringLiteral(LATTE_SOURCE_DIR "/autotests/sourcecontracttest.cpp"));
    QVERIFY(sourceContracts.open(QFile::ReadOnly));
    const QString sourceContractSource = QString::fromUtf8(sourceContracts.readAll());
    const QString oldSourceLock = QStringLiteral("QFile ") + QStringLiteral("launchers");
    QVERIFY(!sourceContractSource.contains(oldSourceLock));
}

void SourceContractTest::sessionShutdownQuitDecisionMatrix_data()
{
    QTest::addColumn<bool>("sawBlockingWindows");
    QTest::addColumn<bool>("shutdownServiceActive");
    QTest::addColumn<bool>("hasBlockingWindows");
    QTest::addColumn<bool>("shouldQuit");

    QTest::newRow("initial-confirmation-cancellable") << false << false << false << false;
    QTest::newRow("sleep-lock-or-idle-session") << false << false << true << false;
    QTest::newRow("ordinary-window-still-blocking") << true << true << true << false;
    QTest::newRow("ordinary-window-cancelled-close") << true << false << true << false;
    QTest::newRow("ordinary-window-closed-after-commit") << true << true << false << true;
    QTest::newRow("no-ordinary-windows-after-commit") << false << true << false << true;
}

void SourceContractTest::sessionShutdownQuitDecisionMatrix()
{
    QFETCH(bool, sawBlockingWindows);
    QFETCH(bool, shutdownServiceActive);
    QFETCH(bool, hasBlockingWindows);
    QFETCH(bool, shouldQuit);

    QCOMPARE(Latte::Session::shouldQuitForCommittedShutdown(sawBlockingWindows, shutdownServiceActive, hasBlockingWindows), shouldQuit);
}

void SourceContractTest::sessionShutdownHandlingMatchesStableWaylandPath()
{
    QFile mainSourceFile(QStringLiteral(LATTE_SOURCE_DIR "/app/main.cpp"));
    QVERIFY(mainSourceFile.open(QFile::ReadOnly));
    const QString mainSource = QString::fromUtf8(mainSourceFile.readAll());

    QVERIFY(mainSource.contains(QStringLiteral("KSignalHandler::self()->watchSignal(SIGTERM);")));
    QVERIFY(mainSource.contains(QStringLiteral("KSignalHandler::self()->watchSignal(SIGHUP);")));
    // SIGINT is excluded from KSignalHandler — handled by self-pipe fallback.
    QVERIFY(!mainSource.contains(QStringLiteral("KSignalHandler::self()->watchSignal(SIGINT);")));
    QVERIFY(mainSource.contains(QStringLiteral("sigaction(SIGINT, &sa, nullptr)")));
    QVERIFY(mainSource.contains(QStringLiteral("QCoreApplication::setAttribute(Qt::AA_DisableSessionManager);")));
    QVERIFY(mainSource.contains(QStringLiteral("QCoreApplication::setQuitLockEnabled(false);")));
    QVERIFY(mainSource.contains(QStringLiteral("qputenv(\"LATTE_SESSION_ENDING\", \"1\");")));
    QVERIFY(mainSource.contains(QStringLiteral("app.setProperty(\"latte_session_ending\", true);")));
    QVERIFY(mainSource.contains(QStringLiteral("#include \"session/shutdownstate.h\"")));
    QVERIFY(mainSource.contains(QStringLiteral("inline bool isPlasmaShutdownServiceActive();")));
    QVERIFY(mainSource.contains(QStringLiteral("auto disableSessionManagement = [](QSessionManager & sm)")));
    QVERIFY(mainSource.contains(QStringLiteral("QObject::connect(&app, &QGuiApplication::commitDataRequest")));
    QVERIFY(mainSource.contains(QStringLiteral("sm.setRestartHint(QSessionManager::RestartNever);")));
    QVERIFY(mainSource.contains(QStringLiteral("QObject::connect(&app, &QGuiApplication::saveStateRequest")));
    QVERIFY(mainSource.contains(QStringLiteral("sessionShutdownPoll.setInterval(500);")));
    QVERIFY(mainSource.contains(QStringLiteral("bool sessionShutdownSawBlockingWindows = false;")));
    QVERIFY(mainSource.contains(QStringLiteral("corona.wm()->hasSessionBlockingWindows()")));
    QVERIFY(mainSource.contains(QStringLiteral("[shutdown] session blocking windows closed; quitting.")));
    QVERIFY(mainSource.contains(QStringLiteral("qunsetenv(\"LATTE_SESSION_ENDING\");")));
    QVERIFY(!mainSource.contains(QStringLiteral("flagSetTimer.hasExpired(5000)")));
    QVERIFY(!mainSource.contains(QStringLiteral("triggering quit() from poller")));
    QVERIFY(!mainSource.contains(QStringLiteral("plasma-shutdown is active; quitting committed session logout")));

    const int sharedEngineDetach = mainSource.indexOf(QStringLiteral("sharedEngine->setParent(nullptr);"));
    const int postExecDeferredDeleteFlush = mainSource.indexOf(QStringLiteral("QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);"), sharedEngineDetach);
    const int sharedEngineReset = mainSource.indexOf(QStringLiteral("sharedEngine.reset();"), sharedEngineDetach);
    const int mainReturn = mainSource.indexOf(QStringLiteral("return result;"), sharedEngineDetach);
    QVERIFY(sharedEngineDetach >= 0);
    QVERIFY(postExecDeferredDeleteFlush > sharedEngineDetach);
    QVERIFY(sharedEngineReset > postExecDeferredDeleteFlush);
    QVERIFY(mainReturn > sharedEngineReset);

    const int disableSessionManager = mainSource.indexOf(QStringLiteral("QCoreApplication::setAttribute(Qt::AA_DisableSessionManager);"));
    const int appCreation = mainSource.indexOf(QStringLiteral("QApplication app(argc, argv);"));
    QVERIFY(disableSessionManager >= 0);
    QVERIFY(appCreation > disableSessionManager);

    const int saveStateStart = mainSource.indexOf(QStringLiteral("QObject::connect(&app, &QGuiApplication::saveStateRequest"));
    const int pollerStart = mainSource.indexOf(QStringLiteral("QTimer sessionShutdownPoll"));
    const int commitDataStart = mainSource.indexOf(QStringLiteral("QObject::connect(&app, &QGuiApplication::commitDataRequest"));
    QVERIFY(saveStateStart >= 0);
    QVERIFY(commitDataStart >= 0);
    QVERIFY(saveStateStart > commitDataStart);
    QVERIFY(pollerStart > saveStateStart);
    const QString commitDataBlock = mainSource.mid(commitDataStart, saveStateStart - commitDataStart);
    QVERIFY(commitDataBlock.contains(QStringLiteral("disableSessionManagement")));
    QVERIFY(!commitDataBlock.contains(QStringLiteral("app.quit()")));
    QVERIFY(!commitDataBlock.contains(QStringLiteral("markSessionEnding")));
    const QString saveStateBlock = mainSource.mid(saveStateStart, pollerStart - saveStateStart);
    QVERIFY(saveStateBlock.contains(QStringLiteral("disableSessionManagement")));
    QVERIFY(!saveStateBlock.contains(QStringLiteral("app.quit()")));
    QVERIFY(!saveStateBlock.contains(QStringLiteral("markSessionEnding")));

    const int pollerBodyStart = mainSource.indexOf(QStringLiteral("QObject::connect(&sessionShutdownPoll"));
    const int pollerBodyEnd = mainSource.indexOf(QStringLiteral("sessionShutdownPoll.start();"), pollerBodyStart);
    QVERIFY(pollerBodyStart >= 0);
    QVERIFY(pollerBodyEnd > pollerBodyStart);
    const QString pollerBody = mainSource.mid(pollerBodyStart, pollerBodyEnd - pollerBodyStart);
    QVERIFY(pollerBody.contains(QStringLiteral("const bool hasBlockingWindows = corona.wm()->hasSessionBlockingWindows();")));
    QVERIFY(pollerBody.contains(QStringLiteral("const bool shutdownServiceActive = isPlasmaShutdownServiceActive();")));
    QVERIFY(pollerBody.contains(QStringLiteral("sessionShutdownSawBlockingWindows = true;")));
    QVERIFY(pollerBody.contains(QStringLiteral("if (app.property(\"latte_session_ending\").toBool()")));
    QVERIFY(pollerBody.contains(QStringLiteral("Latte::Session::shouldQuitForCommittedShutdown(sessionShutdownSawBlockingWindows, shutdownServiceActive, hasBlockingWindows)")));
    QVERIFY(pollerBody.contains(QStringLiteral("app.quit();")));
    QVERIFY(pollerBody.indexOf(QStringLiteral("Latte::Session::shouldQuitForCommittedShutdown(sessionShutdownSawBlockingWindows, shutdownServiceActive, hasBlockingWindows)")) < pollerBody.indexOf(
                QStringLiteral("app.quit();")));

    QFile abstractWmHeaderFile(QStringLiteral(LATTE_SOURCE_DIR "/app/wm/abstractwindowinterface.h"));
    QVERIFY(abstractWmHeaderFile.open(QFile::ReadOnly));
    const QString abstractWmHeader = QString::fromUtf8(abstractWmHeaderFile.readAll());
    QVERIFY(abstractWmHeader.contains(QStringLiteral("virtual bool hasSessionBlockingWindows() const;")));

    QFile waylandWmSourceFile(QStringLiteral(LATTE_SOURCE_DIR "/app/wm/waylandinterface.cpp"));
    QVERIFY(waylandWmSourceFile.open(QFile::ReadOnly));
    const QString waylandWmSource = QString::fromUtf8(waylandWmSourceFile.readAll());
    QVERIFY(waylandWmSource.contains(QStringLiteral("bool WaylandInterface::hasSessionBlockingWindows() const")));
    QVERIFY(waylandWmSource.contains(QStringLiteral("App::matchesSelfAppId(w->appId())")));
    QVERIFY(waylandWmSource.contains(QStringLiteral("w->appId() == QLatin1String(\"org.kde.plasmashell\")")));
    QVERIFY(waylandWmSource.contains(QStringLiteral("w->skipTaskbar() && w->skipSwitcher()")));

    QFile coronaSourceFile(QStringLiteral(LATTE_SOURCE_DIR "/app/lattecorona.cpp"));
    QVERIFY(coronaSourceFile.open(QFile::ReadOnly));
    const QString coronaSource = QString::fromUtf8(coronaSourceFile.readAll());
    QVERIFY(coronaSource.contains(QStringLiteral("qEnvironmentVariableIntValue(\"LATTE_SESSION_ENDING\") == 1")));
    QVERIFY(coronaSource.contains(QStringLiteral("qApp->property(\"latte_session_ending\").toBool()")));
    QVERIFY(coronaSource.contains(QStringLiteral("m_layoutsManager->synchronizer()->hideAllViews();")));
    QVERIFY(coronaSource.contains(QStringLiteral("fast shutdown path for session logout")));

    QFile viewSourceFile(QStringLiteral(LATTE_SOURCE_DIR "/app/view/view.cpp"));
    QVERIFY(viewSourceFile.open(QFile::ReadOnly));
    const QString viewSource = QString::fromUtf8(viewSourceFile.readAll());
    QVERIFY(viewSource.contains(QStringLiteral("case QEvent::Close:")));
    QVERIFY(viewSource.contains(QStringLiteral("qEnvironmentVariableIntValue(\"LATTE_SESSION_ENDING\") == 1")));
    QVERIFY(viewSource.contains(QStringLiteral("qApp->property(\"latte_session_ending\").toBool()")));
    QVERIFY(viewSource.contains(QStringLiteral("QMetaObject::invokeMethod(qGuiApp, &QCoreApplication::quit, Qt::QueuedConnection);")));
}

void SourceContractTest::sessionShutdownPollerHidesViewsAndRestoresOnCancel()
{
    QFile mainSourceFile(QStringLiteral(LATTE_SOURCE_DIR "/app/main.cpp"));
    QVERIFY(mainSourceFile.open(QFile::ReadOnly));
    const QString mainSource = QString::fromUtf8(mainSourceFile.readAll());

    //! Phase 1 (logout announced): the poller must unmap all views so KWin's
    //! closeWaylandWindows() never waits on the dock windows during shutdown.
    const int phase1Log = mainSource.indexOf(QStringLiteral("[shutdown] isShuttingDown() = true; setting flag (not quitting yet)."));
    const int flagSet = mainSource.indexOf(QStringLiteral("markSessionEnding();"), phase1Log);
    const int hideViews = mainSource.indexOf(QStringLiteral("synchronizer()->hideAllViews();"), flagSet);
    QVERIFY(phase1Log >= 0);
    QVERIFY(flagSet > phase1Log);
    QVERIFY(hideViews > flagSet);

    //! Cancel branch: views hidden by the announcement must be restored.
    const int cancelLog = mainSource.indexOf(QStringLiteral("[shutdown] logout cancelled; clearing session-ending flag."));
    const int currentViews = mainSource.indexOf(QStringLiteral("synchronizer()->currentViews()"), cancelLog);
    const int restoreViews = mainSource.indexOf(QStringLiteral("view->setVisible(true)"), currentViews);
    QVERIFY(cancelLog >= 0);
    QVERIFY(currentViews > cancelLog);
    QVERIFY(restoreViews > currentViews);
}

void SourceContractTest::needsAttentionStatusBlocksHidingWithoutWindowReconfiguration()
{
    //! The Application Menu widget reports NeedsAttentionStatus while its menu
    //! bar menu (and cascading submenus) are open.  Reconfiguring the dock
    //! window flags / plasma shell surface at that point invalidates the
    //! QMenu's child xdg_popup surfaces on Wayland and the menu closes as soon
    //! as the pointer reaches it.  The NeedsAttention branch must therefore
    //! only block dock hiding — mirroring the RequiresAttentionStatus fix — and
    //! keep the window/surface configuration untouched.
    QFile viewSourceFile(QStringLiteral(LATTE_SOURCE_DIR "/app/view/view.cpp"));
    QVERIFY(viewSourceFile.open(QFile::ReadOnly));
    const QString viewSource = QString::fromUtf8(viewSourceFile.readAll());

    const int statusChanged = viewSource.indexOf(QStringLiteral("void View::statusChanged(Plasma::Types::ItemStatus status)"));
    QVERIFY(statusChanged >= 0);

    const int needsAttention = viewSource.indexOf(QStringLiteral("status == Plasma::Types::NeedsAttentionStatus"), statusChanged);
    const int requiresAttention = viewSource.indexOf(QStringLiteral("status == Plasma::Types::RequiresAttentionStatus"), needsAttention);
    QVERIFY(needsAttention > statusChanged);
    QVERIFY(requiresAttention > needsAttention);

    //! The NeedsAttention branch keeps the hiding block (so the dock cannot
    //! retract underneath the open menu) but must not touch the window flags,
    //! the view flags or the shell surface.
    const QString block = QStringLiteral("m_visibility->addBlockHidingEvent(BLOCKHIDINGNEEDSATTENTIONTYPE);");
    const int blockInNeedsAttention = viewSource.indexOf(block, needsAttention);
    QVERIFY(blockInNeedsAttention > needsAttention);
    QVERIFY(blockInNeedsAttention < requiresAttention);

    const int nextBranch = viewSource.indexOf(QStringLiteral("} else if"), blockInNeedsAttention);
    QVERIFY(nextBranch > blockInNeedsAttention);
    const QString branchBody = viewSource.mid(blockInNeedsAttention + block.size(), nextBranch - blockInNeedsAttention - block.size());
    QVERIFY(!branchBody.contains(QStringLiteral("setFlags")));
    QVERIFY(!branchBody.contains(QStringLiteral("initViewFlags")));
    QVERIFY(!branchBody.contains(QStringLiteral("setPanelTakesFocus")));
}

void SourceContractTest::appletPopupHidesOnWindowDeactivateByDefault()
{
    //! The applet popup must close on outside interaction (click outside),
    //! matching the standard Plasma Desktop shell.  The first-level menu
    //! staying open on pointer-leave is handled separately by the cascade
    //! logic in View::eventFilter, so hideOnWindowDeactivate defaults to true.
    QFile appletPopupFile(QStringLiteral(LATTE_SOURCE_DIR "/shell/package/contents/applet/CompactApplet.qml"));
    QVERIFY(appletPopupFile.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(appletPopupFile.readAll());

    const int hideOnDeactivate = source.indexOf(QStringLiteral("hideOnWindowDeactivate:"));
    QVERIFY(hideOnDeactivate >= 0);
    const QString line = source.mid(hideOnDeactivate, source.indexOf(QStringLiteral("\n"), hideOnDeactivate) - hideOnDeactivate);
    QVERIFY(line.endsWith(QStringLiteral(": true")));
}

void SourceContractTest::viewTracksPointerWindowsAndResetsCascadingSubmenus()
{
    //! The dock must close cascading submenus when the pointer leaves the
    //! whole menu tree, driven by Enter/Leave events (QCursor::pos is stale
    //! on Wayland once the pointer leaves this process's surfaces).  It must
    //! track the pointer windows and, on a submenu window's Leave, invoke the
    //! applet's reset so the first-level menu stays open.
    QFile viewSourceFile(QStringLiteral(LATTE_SOURCE_DIR "/app/view/view.cpp"));
    QVERIFY(viewSourceFile.open(QFile::ReadOnly));
    const QString viewSource = QString::fromUtf8(viewSourceFile.readAll());

    QVERIFY(viewSource.contains(QStringLiteral("qApp->installEventFilter(this)")));
    QVERIFY(viewSource.contains(QStringLiteral("bool View::eventFilter(QObject *watched, QEvent *event)")));
    QVERIFY(viewSource.contains(QStringLiteral("m_pointerWindows.insert(window)")));
    QVERIFY(viewSource.contains(QStringLiteral("m_pointerWindows.remove(window)")));
    QVERIFY(viewSource.contains(QStringLiteral("QMetaObject::invokeMethod(appletGuard.data(), \"reset\", Qt::QueuedConnection)")));
    QVERIFY(viewSource.contains(QStringLiteral("qobject_cast<PlasmaQuick::AppletPopup *>(popupWindow)")));
    QVERIFY(viewSource.contains(QStringLiteral("windowBelongsToThisDock")));
}

void SourceContractTest::pointerWindowTrackerRemovesDestroyedWindows()
{
    //! The pointer-window tracker stores raw QWindow pointers; a submenu or
    //! popup can be destroyed while tracked, so the destroyed signal must
    //! remove it from the set — otherwise the deferred close check would
    //! dereference a dangling pointer (segfault in libQt6Gui).
    QFile viewSourceFile(QStringLiteral(LATTE_SOURCE_DIR "/app/view/view.cpp"));
    QVERIFY(viewSourceFile.open(QFile::ReadOnly));
    const QString viewSource = QString::fromUtf8(viewSourceFile.readAll());

    QVERIFY(viewSource.contains(QStringLiteral("connect(window, &QObject::destroyed, this, &View::onPointerWindowDestroyed, Qt::UniqueConnection)")));
    QVERIFY(viewSource.contains(QStringLiteral("void View::onPointerWindowDestroyed(QObject *window)")));
    QVERIFY(viewSource.contains(QStringLiteral("m_pointerWindows.remove(w)")));
}

void SourceContractTest::qtQuickGpuPreferenceKeepsSoftwareFallbackAvailable()
{
    QFile mainSourceFile(QStringLiteral(LATTE_SOURCE_DIR "/app/main.cpp"));
    QVERIFY(mainSourceFile.open(QFile::ReadOnly));
    const QString mainSource = QString::fromUtf8(mainSourceFile.readAll());

    QVERIFY(mainSource.contains(QStringLiteral("#include <QSGRendererInterface>")));
    QVERIFY(mainSource.contains(QStringLiteral("inline void configureQtQuickGraphicsPreference();")));
    QVERIFY(mainSource.contains(QStringLiteral("void configureQtQuickGraphicsPreference()")));
    QVERIFY(mainSource.contains(QStringLiteral("qEnvironmentVariableIsSet(\"QT_QUICK_BACKEND\")")));
    QVERIFY(mainSource.contains(QStringLiteral("qEnvironmentVariableIsSet(\"QSG_RHI_BACKEND\")")));
    QVERIFY(mainSource.contains(QStringLiteral("QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL)")));
    QVERIFY(mainSource.contains(QStringLiteral("requested Qt Quick OpenGL rendering")));
    QVERIFY(mainSource.contains(QStringLiteral("respecting explicit Qt Quick graphics override")));

    const int gpuPreferenceCall = mainSource.indexOf(QStringLiteral("configureQtQuickGraphicsPreference();"));
    const int applicationCreation = mainSource.indexOf(QStringLiteral("QApplication app(argc, argv);"));
    QVERIFY(gpuPreferenceCall >= 0);
    QVERIFY(applicationCreation > gpuPreferenceCall);

    QFile environmentQml(QStringLiteral(LATTE_SOURCE_DIR "/declarativeimports/abilities/client/Environment.qml"));
    QVERIFY(environmentQml.open(QFile::ReadOnly));
    const QString environmentSource = QString::fromUtf8(environmentQml.readAll());
    QVERIFY(environmentSource.contains(QStringLiteral("GraphicsInfo.api !== GraphicsInfo.Software")));
    QVERIFY(environmentSource.contains(QStringLiteral("GraphicsInfo.api !== GraphicsInfo.Unknown")));
    QVERIFY(environmentSource.contains(QStringLiteral("isGraphicsSystemAccelerated: ref.environment.isGraphicsSystemAccelerated")));

    QFile viewSourceFile(QStringLiteral(LATTE_SOURCE_DIR "/app/view/view.cpp"));
    QVERIFY(viewSourceFile.open(QFile::ReadOnly));
    const QString viewSource = QString::fromUtf8(viewSourceFile.readAll());
    QVERIFY(viewSource.contains(QStringLiteral("#include <QSGRendererInterface>")));
    QVERIFY(viewSource.contains(QStringLiteral("actualQtQuickGraphicsApiName")));
    QVERIFY(viewSource.contains(QStringLiteral("isActualQtQuickGraphicsApiAccelerated")));
    QVERIFY(viewSource.contains(QStringLiteral("&QQuickWindow::sceneGraphInitialized")));
    QVERIFY(viewSource.contains(QStringLiteral("rendererInterface()")));
    QVERIFY(viewSource.contains(QStringLiteral("graphicsApi()")));
    QVERIFY(viewSource.contains(QStringLiteral("Latte Dock actual Qt Quick scene graph graphics API")));
    QVERIFY(viewSource.contains(QStringLiteral("GPU accelerated")));
}

void SourceContractTest::knsCompatImportsAreAvailableForSystemInstall()
{
    QFile mainSourceFile(QStringLiteral(LATTE_SOURCE_DIR "/app/main.cpp"));
    QVERIFY(mainSourceFile.open(QFile::ReadOnly));
    const QString mainSource = QString::fromUtf8(mainSourceFile.readAll());

    // The KNS compat QML root factory must still be available.
    QVERIFY(mainSource.contains(QStringLiteral("knsCompatUserQmlRoot()")));

    // Paths are now engine-scoped (addImportPath) instead of env vars.
    // collectUserLocalPaths() gathers user-local QML + plugin paths at
    // startup without touching environment variables.
    QVERIFY(mainSource.contains(QStringLiteral("inline void collectUserLocalPaths(int argc, char **argv);")));
    QVERIFY(mainSource.contains(QStringLiteral("void addLatteQmlImportPaths(QQmlEngine *engine)")));

    // addLatteQmlImportPaths is called on the shared engine after creation.
    const int appCreation = mainSource.indexOf(QStringLiteral("QApplication app(argc, argv);"));
    QVERIFY(appCreation >= 0);
    const int ensureCompatCall = mainSource.indexOf(QStringLiteral("ensureKnsCompat();"), appCreation);
    const int sharedEngineCreation = mainSource.indexOf(QStringLiteral("std::make_shared<PlasmaQuick::SharedQmlEngine>(&app);"), appCreation);
    const int addImportPathsCall = mainSource.indexOf(QStringLiteral("addLatteQmlImportPaths(sharedEngine->engine().get());"), sharedEngineCreation);
    QVERIFY(ensureCompatCall > appCreation);
    QVERIFY(sharedEngineCreation > ensureCompatCall);
    QVERIFY(addImportPathsCall > sharedEngineCreation);

    // The old env var approach must NOT be present.
    QVERIFY(!mainSource.contains(QStringLiteral("ensureKnsCompatQmlImportPaths")));
    QVERIFY(!mainSource.contains(QStringLiteral("prependEnvironmentPath")));

    // KNS compat header still exports the root-finder.
    QFile compatHeader(QStringLiteral(LATTE_SOURCE_DIR "/app/knscompat.h"));
    QVERIFY(compatHeader.open(QFile::ReadOnly));
    const QString compatHeaderSource = QString::fromUtf8(compatHeader.readAll());
    QVERIFY(compatHeaderSource.contains(QStringLiteral("QString knsCompatUserQmlRoot();")));
}

void SourceContractTest::layerShellSetScreenGuardPreventsBuildRegression()
{
    QFile wmSource(QStringLiteral(LATTE_SOURCE_DIR "/app/wm/waylandinterface.cpp"));
    QVERIFY(wmSource.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(wmSource.readAll());

    // The unconditional QWindow::setScreen must come before the #ifdef guard.
    QVERIFY(src.contains(QStringLiteral("setScreen(screen);")));
    const int qwindowCall = src.indexOf(QStringLiteral("setScreen(screen);"));
    QVERIFY(qwindowCall >= 0);

    // The layerWindow->setScreen must be guarded.
    const int ifdefGuard = src.indexOf(QStringLiteral("#ifdef LATTE_LAYERSHELL_HAS_SET_SCREEN"), qwindowCall);
    const int layerCall = src.indexOf(QStringLiteral("layerWindow->setScreen(screen);"), ifdefGuard);
    const int endifGuard = src.indexOf(QStringLiteral("#endif"), layerCall);
    QVERIFY(ifdefGuard > qwindowCall);
    QVERIFY(layerCall > ifdefGuard);
    QVERIFY(endifGuard > layerCall);

    // The try_compile probe must exist.
    QFile probeFile(QStringLiteral(LATTE_SOURCE_DIR "/cmake/CheckLayerShellSetScreen.cpp"));
    QVERIFY(probeFile.exists());

    // The CMakeLists must contain the try_compile block.
    QFile cmakeFile(QStringLiteral(LATTE_SOURCE_DIR "/CMakeLists.txt"));
    QVERIFY(cmakeFile.open(QFile::ReadOnly));
    const QString cmake = QString::fromUtf8(cmakeFile.readAll());
    QVERIFY(cmake.contains(QStringLiteral("check_cxx_source_compiles")));
    QVERIFY(cmake.contains(QStringLiteral("LATTE_LAYERSHELL_HAS_SET_SCREEN")));
    QVERIFY(cmake.contains(QStringLiteral("CheckLayerShellSetScreen.cpp")));
}

void SourceContractTest::taskIconsRefreshAfterIconThemeChanges()
{
    QFile taskIcon(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/task/TaskIcon.qml"));
    QVERIFY(taskIcon.open(QFile::ReadOnly));

    const QString source = QString::fromUtf8(taskIcon.readAll());
    QVERIFY(source.contains(QStringLiteral("function forceRefreshTaskIconSource()")));
    QVERIFY(source.contains(QStringLiteral("taskIconItem.source = \"\"")));
    QVERIFY(source.contains(QStringLiteral("Qt.callLater(resetTaskIconSourceBinding)")));

    const int clearSource = source.indexOf(QStringLiteral("taskIconItem.source = \"\""));
    const int delayedRebind = source.indexOf(QStringLiteral("Qt.callLater(resetTaskIconSourceBinding)"), clearSource);
    QVERIFY(clearSource >= 0);
    QVERIFY(delayedRebind > clearSource);

    QFile environment(QStringLiteral(LATTE_SOURCE_DIR "/declarativeimports/core/environment.cpp"));
    QVERIFY(environment.open(QFile::ReadOnly));

    const QString environmentSource = QString::fromUtf8(environment.readAll());
    QVERIFY(environmentSource.contains(QStringLiteral("readEntry(QStringLiteral(\"Theme\"), QStringLiteral(\"breeze\"))")));
    QVERIFY(environmentSource.contains(QStringLiteral("QIcon::setThemeName(currentIconTheme())")));
    QVERIFY(environmentSource.contains(QStringLiteral("QPixmapCache::clear()")));
    QVERIFY(!environmentSource.contains(QStringLiteral("if (!iconTheme.isEmpty())")));
}

void SourceContractTest::genericLayoutUnloadUsesSynchronousDelete()
{
    //! Layout switching unloads containments in place. Deferred deletion
    //! left stale containments in the corona registry, which broke the
    //! unload-to-load sequence and crashed in the
    //! availableScreenRectChangedFrom signal chain, so the direct delete
    //! in the unload paths is a regression contract.
    QFile layout(QStringLiteral(LATTE_SOURCE_DIR "/app/layout/genericlayout.cpp"));
    QVERIFY(layout.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(layout.readAll());

    QVERIFY(src.contains(QStringLiteral("delete sub;")));
    QVERIFY(src.contains(QStringLiteral("delete containment;")));
    QVERIFY(!src.contains(QStringLiteral("sub->deleteLater();")));
    QVERIFY(!src.contains(QStringLiteral("containment->deleteLater();")));
}

void SourceContractTest::synchronizerUnloadUsesSynchronousDelete()
{
    //! Same rationale as genericLayoutUnloadUsesSynchronousDelete: the
    //! CentralLayout and its layout wrapper must be destroyed in place
    //! during unload so a subsequent load never sees stale state.
    QFile synchronizer(QStringLiteral(LATTE_SOURCE_DIR "/app/layouts/synchronizer.cpp"));
    QVERIFY(synchronizer.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(synchronizer.readAll());

    QVERIFY(src.contains(QStringLiteral("delete central;")));
    QVERIFY(src.contains(QStringLiteral("delete layout;")));
    QVERIFY(!src.contains(QStringLiteral("central->deleteLater();")));
    QVERIFY(!src.contains(QStringLiteral("layout->deleteLater();")));
}

void SourceContractTest::pointerWindowTrackingDisconnectsOnLeave()
{
    //! The cascading-menu pointer tracker connects a tracked window's
    //! destroyed() signal to View::onPointerWindowDestroyed when the pointer
    //! enters it (eventFilter Enter branch). If that connection survives the
    //! pointer leaving, it later fires against a View that is already being
    //! destroyed during logout/shutdown teardown: ~QObject emits destroyed()
    //! and Qt's internal qobject_cast assertion aborts on the half-destroyed
    //! receiver. Leave must therefore drop the connection it mirrors.
    QFile viewSourceFile(QStringLiteral(LATTE_SOURCE_DIR "/app/view/view.cpp"));
    QVERIFY(viewSourceFile.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(viewSourceFile.readAll());

    const QString enterConnect = QStringLiteral(
            "connect(window, &QObject::destroyed, this, &View::onPointerWindowDestroyed, Qt::UniqueConnection);");
    const QString windowDisconnect = QStringLiteral(
            "disconnect(window, &QObject::destroyed, this, &View::onPointerWindowDestroyed);");

    //! Exactly one connect site (eventFilter Enter) and at least two disconnect
    //! sites (eventFilter Leave plus ~View) keep the tracking connections from
    //! outliving their window's membership in m_pointerWindows.
    QCOMPARE(src.count(enterConnect), 1);
    QVERIFY(src.count(windowDisconnect) >= 2);

    //! The Leave branch drops the connection right after it stops tracking the
    //! window, so a tracked window never keeps a live destroyed() connection
    //! after the pointer has left it.
    const int removeIndex = src.indexOf(QStringLiteral("m_pointerWindows.remove(window);"));
    const int leaveDisconnect = src.indexOf(windowDisconnect, removeIndex);
    QVERIFY(removeIndex >= 0);
    QVERIFY(leaveDisconnect > removeIndex);
}

void SourceContractTest::viewDestructorDropsPointerWindowTrackingConnections()
{
    //! Teardown while the pointer is still over a dock window (or over a popup
    //! that never received Leave) must not leave destroyed() connections
    //! behind: ~View disconnects every still-tracked window before member
    //! deletion and clears the tracking set, so the base ~QObject can emit
    //! destroyed() without activating onPointerWindowDestroyed against this
    //! half-destroyed View.
    QFile viewSourceFile(QStringLiteral(LATTE_SOURCE_DIR "/app/view/view.cpp"));
    QVERIFY(viewSourceFile.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(viewSourceFile.readAll());

    const int destructor = src.indexOf(QStringLiteral("View::~View()"));
    QVERIFY(destructor >= 0);

    const int removeEventFilter = src.indexOf(QStringLiteral("qApp->removeEventFilter(this);"), destructor);
    const int loopStart = src.indexOf(QStringLiteral("for (QWindow *window : m_pointerWindows) {"), removeEventFilter);
    const int windowDisconnect = src.indexOf(QStringLiteral(
            "disconnect(window, &QObject::destroyed, this, &View::onPointerWindowDestroyed);"),
            loopStart);
    const int clear = src.indexOf(QStringLiteral("m_pointerWindows.clear();"), windowDisconnect);
    QVERIFY(removeEventFilter > destructor);
    QVERIFY(loopStart > removeEventFilter);
    QVERIFY(windowDisconnect > loopStart);
    QVERIFY(clear > windowDisconnect);
}

void SourceContractTest::taskAudioBadgesScaleWithParabolicZoom()
{
    QFile audioStream(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/task/AudioStream.qml"));
    QVERIFY(audioStream.open(QFile::ReadOnly));

    const QString source = QString::fromUtf8(audioStream.readAll());
    // parabolicZoom property was removed — iconBoxSize already reflects the
    // zoomed parent size; multiplying again caused double-scaling distortion.
    QVERIFY(!source.contains(QStringLiteral("readonly property real parabolicZoom")));
    // maximumBadgeSize is a fixed cap, not zoom-dependent
    QVERIFY(source.contains(QStringLiteral("readonly property real maximumBadgeSize: Kirigami.Units.iconSizes.smallMedium")));
    QVERIFY(source.contains(QStringLiteral("compactBadgeSize: Math.min(iconBoxSize * 0.4, maximumBadgeSize)")));
    QVERIFY(source.contains(QStringLiteral("Math.min(parent.height * audioStreamIconBox.indicatorScale, audioStreamIconBox.maximumBadgeSize)")));

    const int iconBoxSize = source.indexOf(QStringLiteral("readonly property real iconBoxSize"));
    const int maximumSize = source.indexOf(QStringLiteral("readonly property real maximumBadgeSize"), iconBoxSize);
    const int compactSize = source.indexOf(QStringLiteral("compactBadgeSize: Math.min(iconBoxSize * 0.4, maximumBadgeSize)"), maximumSize);
    QVERIFY(iconBoxSize >= 0);
    QVERIFY(maximumSize > iconBoxSize);
    QVERIFY(compactSize > maximumSize);
}

void SourceContractTest::parabolicScaleAddressingFallsBackToLastValidIndexDuringRemoval()
{
    // Issue: during launcher->window/startup conversions ListView.delayRemove
    // keeps the bouncing delegate alive while its model row is gone, turning
    // its index into -1. Parabolic scale broadcasts are addressed by index, so
    // a -1 delegate could never receive updates (zoom frozen) yet always
    // accepted the clear signal (zoom pinned to 1). The fix addresses every
    // parabolic operation through effectiveIndex, which falls back to the last
    // valid index captured before removal.
    QFile eventsArea(QStringLiteral(LATTE_SOURCE_DIR "/declarativeimports/abilities/items/basicitem/ParabolicEventsArea.qml"));
    QVERIFY(eventsArea.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(eventsArea.readAll());

    // 1. The fallback property itself (same pattern as TaskItem.isSeparatorHidden)
    QVERIFY(source.contains(QStringLiteral("readonly property int effectiveIndex: index >= 0 ? index : (taskItem.lastValidIndex >= 0 ? taskItem.lastValidIndex : index)")));
    // 2. Outgoing broadcasts use effectiveIndex (no more -1 hijack addresses)
    QVERIFY(source.contains(QStringLiteral("applyParabolicEffect(effectiveIndex")));
    // 3. Incoming update branch matches on effectiveIndex (dying delegate keeps tracking)
    QVERIFY(source.contains(QStringLiteral("if (delegateIndex === effectiveIndex)")));
    // 4. Incoming clear branch compares and resets through effectiveIndex
    QVERIFY(source.contains(QStringLiteral("(effectiveIndex < delegateIndex)")));
    QVERIFY(source.contains(QStringLiteral("updateScale(effectiveIndex, 1)")));
}

void SourceContractTest::widgetExplorerLaunchesKnsDialogOutOfProcess()
{
    QFile widgetExplorer(QStringLiteral(LATTE_SOURCE_DIR "/shell/package/contents/views/WidgetExplorer.qml"));
    QVERIFY(widgetExplorer.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(widgetExplorer.readAll());

    QVERIFY(source.contains(QStringLiteral("function shouldOpenExternalGetNewWidgetsDialog(actionModel)")));
    QVERIFY(source.contains(QStringLiteral("property bool getNewWidgetsDialogActive: false")));
    QVERIFY(source.contains(QStringLiteral("property bool preventWindowHide: draggingWidget || getNewWidgetsDialogActive")));
    QVERIFY(source.contains(QStringLiteral("|| getWidgetsDialog.status !== PlasmaExtras.Menu.Closed")));
    QVERIFY(source.contains(QStringLiteral("id: getNewWidgetsWindowHideRestoreTimer")));
    QVERIFY(source.contains(QStringLiteral("main.getNewWidgetsDialogActive = false")));
    QVERIFY(source.contains(QStringLiteral("label.indexOf(\"添加新\") !== -1")));
    QVERIFY(source.contains(QStringLiteral("function holdWidgetExplorerForExternalDialog()")));
    QVERIFY(source.contains(QStringLiteral("function forceClose()")));
    QVERIFY(source.contains(QStringLiteral("getNewWidgetsWindowHideRestoreTimer.stop()")));
    QVERIFY(source.contains(QStringLiteral("viewConfig.hideConfigWindow()")));
    QVERIFY(source.contains(QStringLiteral("onClicked: main.forceClose()")));
    QVERIFY(source.contains(QStringLiteral("id: getWidgetsDialog")));
    QVERIFY(source.contains(QStringLiteral("getWidgetsDialog.model = widgetExplorer.widgetsMenuActions")));
    QVERIFY(source.contains(QStringLiteral("getWidgetsDialog.open(0, getWidgetsButton.height)")));
    QVERIFY(source.contains(QStringLiteral("main.getNewWidgetsDialogActive = true")));
    QVERIFY(source.contains(QStringLiteral("getNewWidgetsWindowHideRestoreTimer.restart()")));
    QVERIFY(source.contains(QStringLiteral("main.holdWidgetExplorerForExternalDialog()")));
    QVERIFY(source.contains(QStringLiteral("viewConfig.openGetNewWidgetsDialog()")));
    QVERIFY(source.contains(QStringLiteral("model.trigger()")));

    const int getWidgetsMenu = source.indexOf(QStringLiteral("id: getWidgetsDialog"));
    QVERIFY(getWidgetsMenu >= 0);
    const int externalDialogCall = source.indexOf(QStringLiteral("viewConfig.openGetNewWidgetsDialog()"), getWidgetsMenu);
    const int fallbackTrigger = source.indexOf(QStringLiteral("model.trigger()"), getWidgetsMenu);
    const int fallbackHold = source.lastIndexOf(QStringLiteral("main.holdWidgetExplorerForExternalDialog()"), fallbackTrigger);
    QVERIFY(externalDialogCall > getWidgetsMenu);
    QVERIFY(fallbackTrigger > externalDialogCall);
    QVERIFY(fallbackHold > externalDialogCall);
    QVERIFY(fallbackHold < fallbackTrigger);

    QFile widgetExplorerHeader(QStringLiteral(LATTE_SOURCE_DIR "/app/view/settings/widgetexplorerview.h"));
    QVERIFY(widgetExplorerHeader.open(QFile::ReadOnly));
    const QString headerSource = QString::fromUtf8(widgetExplorerHeader.readAll());
    QVERIFY(headerSource.contains(QStringLiteral("Q_INVOKABLE bool openGetNewWidgetsDialog();")));

    QFile widgetExplorerCpp(QStringLiteral(LATTE_SOURCE_DIR "/app/view/settings/widgetexplorerview.cpp"));
    QVERIFY(widgetExplorerCpp.open(QFile::ReadOnly));
    const QString cppSource = QString::fromUtf8(widgetExplorerCpp.readAll());
    QVERIFY(cppSource.contains(QStringLiteral("#include <QProcess>")));
    QVERIFY(cppSource.contains(QStringLiteral("#include <QProcessEnvironment>")));
    QVERIFY(cppSource.contains(QStringLiteral("#include <QStandardPaths>")));
    QVERIFY(cppSource.contains(QStringLiteral("QStandardPaths::findExecutable(QStringLiteral(\"knewstuff-dialog6\"))")));
    QVERIFY(cppSource.contains(QStringLiteral("defaultWaylandDisplay()")));
    QVERIFY(cppSource.contains(QStringLiteral("environment.insert(QStringLiteral(\"WAYLAND_DISPLAY\")")));
    QVERIFY(cppSource.contains(QStringLiteral("environment.insert(QStringLiteral(\"QT_QPA_PLATFORM\"), QStringLiteral(\"wayland\"))")));
    QVERIFY(cppSource.contains(QStringLiteral("process.setProcessEnvironment(environment)")));
    QVERIFY(cppSource.contains(QStringLiteral("process.startDetached()")));
}

void SourceContractTest::widgetExplorerUsesPlasmaTranslationContexts()
{
    QFile widgetExplorer(QStringLiteral(LATTE_SOURCE_DIR "/shell/package/contents/views/WidgetExplorer.qml"));
    QVERIFY(widgetExplorer.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(widgetExplorer.readAll());

    QVERIFY(source.contains(QStringLiteral("i18ndc(\"plasma_shell_org.kde.plasma.desktop\", \"@title:group for widget grid\", \"Widgets\")")));
    QVERIFY(source.contains(QStringLiteral("i18ndc(\"plasma_shell_org.kde.plasma.desktop\", \"@action:button The word 'new' refers to widgets\", \"Get New…\")")));
    QVERIFY(source.contains(QStringLiteral("i18ndc(\"plasma_shell_org.kde.plasma.desktop\", \"@action:button\", \"Get New Widgets…\")")));
    QVERIFY(source.contains(QStringLiteral("i18ndc(\"plasma_shell_org.kde.plasma.desktop\", \"@action:button like listbox, switches category to all widgets\", \"All Widgets\")")));
    QVERIFY(source.contains(QStringLiteral("i18ndc(\"plasma_shell_org.kde.plasma.desktop\", \"@action:button tooltip only\", \"Categories\")")));
    QVERIFY(source.contains(QStringLiteral("i18ndc(\"plasma_shell_org.kde.plasma.desktop\", \"@info placeholdermessage\", \"No widgets available\")")));
    QVERIFY(source.contains(QStringLiteral("i18ndc(\"plasma_shell_org.kde.plasma.desktop\", \"@info placeholdermessage\", \"No widgets matched the search terms\")")));

    QVERIFY(!source.contains(QStringLiteral("i18nd(\"plasma_shell_org.kde.plasma.desktop\", \"Widgets\")")));
    QVERIFY(!source.contains(QStringLiteral("i18nd(\"plasma_shell_org.kde.plasma.desktop\", \"Get New Widgets…\")")));
    QVERIFY(!source.contains(QStringLiteral("i18nd(\"plasma_shell_org.kde.plasma.desktop\", \"All Widgets\")")));
    QVERIFY(!source.contains(QStringLiteral("i18n(\"No widgets available\")")));
}

void SourceContractTest::itemsAlignmentIsSeparateAndJustifyOnly()
{
    QFile config(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/config/main.xml"));
    QVERIFY(config.open(QFile::ReadOnly));
    const QString configSource = QString::fromUtf8(config.readAll());
    QVERIFY(configSource.contains(QStringLiteral("<entry name=\"itemsAlignment\" type=\"Int\">")));
    QVERIFY(configSource.contains(QStringLiteral("dock icons/items alignment used only when alignment is Justify")));

    QFile myViewDefinition(QStringLiteral(LATTE_SOURCE_DIR "/declarativeimports/abilities/definition/MyView.qml"));
    QVERIFY(myViewDefinition.open(QFile::ReadOnly));
    const QString myViewDefinitionSource = QString::fromUtf8(myViewDefinition.readAll());
    QVERIFY(myViewDefinitionSource.contains(QStringLiteral("property int itemsAlignment: LatteCore.types.Center")));

    QFile myViewHost(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/abilities/MyView.qml"));
    QVERIFY(myViewHost.open(QFile::ReadOnly));
    const QString myViewHostSource = QString::fromUtf8(myViewHost.readAll());
    QVERIFY(myViewHostSource.contains(QStringLiteral("itemsAlignment: plasmoid.configuration.itemsAlignment")));

    QFile containmentHost(QStringLiteral(LATTE_SOURCE_DIR "/declarativeimports/abilities/host/Containment.qml"));
    QVERIFY(containmentHost.open(QFile::ReadOnly));
    const QString containmentHostSource = QString::fromUtf8(containmentHost.readAll());
    QVERIFY(containmentHostSource.contains(QStringLiteral("readonly property int effectiveItemsAlignment: !myView ? LatteCore.types.Center")));
    QVERIFY(containmentHostSource.contains(QStringLiteral(": myView.alignment === LatteCore.types.Justify")));
    QVERIFY(containmentHostSource.contains(QStringLiteral("? normalizedItemsAlignment(myView.itemsAlignment)")));
    QVERIFY(containmentHostSource.contains(QStringLiteral(": myView.alignment")));
    QVERIFY(containmentHostSource.contains(QStringLiteral("function normalizedItemsAlignment(alignment)")));

    QFile behavior(QStringLiteral(LATTE_SOURCE_DIR "/shell/package/contents/configuration/pages/BehaviorConfig.qml"));
    QVERIFY(behavior.open(QFile::ReadOnly));
    const QString behaviorSource = QString::fromUtf8(behavior.readAll());
    QVERIFY(behaviorSource.contains(QStringLiteral("text: i18nc(\"dock items alignment\", \"Items alignment\")")));
    QVERIFY(behaviorSource.contains(QStringLiteral("enabled: alignmentRow.currentAlignment === LatteCore.types.Justify")));
    QVERIFY(behaviorSource.contains(QStringLiteral("opacity: enabled ? 1 : 0.45")));
    QVERIFY(behaviorSource.contains(QStringLiteral("readonly property int currentItemsAlignment: normalizedItemsAlignment(plasmoid.configuration.itemsAlignment)")));
    QVERIFY(behaviorSource.contains(QStringLiteral("plasmoid.configuration.itemsAlignment = alignment")));
    QVERIFY(behaviorSource.contains(QStringLiteral("i18nc(\"left items alignment\", \"Left\")")));
    QVERIFY(behaviorSource.contains(QStringLiteral("i18nc(\"center items alignment\", \"Center\")")));
    QVERIFY(behaviorSource.contains(QStringLiteral("i18nc(\"right items alignment\", \"Right\")")));
    QVERIFY(behaviorSource.contains(QStringLiteral("i18nc(\"top items alignment\", \"Top\")")));
    QVERIFY(behaviorSource.contains(QStringLiteral("i18nc(\"bottom items alignment\", \"Bottom\")")));

    QFile zhCnCatalog(QStringLiteral(LATTE_SOURCE_DIR "/po/zh_CN/latte-dock.po"));
    QVERIFY(zhCnCatalog.open(QFile::ReadOnly));
    const QString zhCnSource = QString::fromUtf8(zhCnCatalog.readAll());
    QVERIFY(zhCnSource.contains(QStringLiteral("msgctxt \"dock items alignment\"\nmsgid \"Items alignment\"\nmsgstr \"图标对齐\"")));
    QVERIFY(zhCnSource.contains(QStringLiteral("msgctxt \"left items alignment\"\nmsgid \"Left\"\nmsgstr \"左对齐\"")));
    QVERIFY(zhCnSource.contains(QStringLiteral("msgctxt \"center items alignment\"\nmsgid \"Center\"\nmsgstr \"居中\"")));
    QVERIFY(zhCnSource.contains(QStringLiteral("msgctxt \"right items alignment\"\nmsgid \"Right\"\nmsgstr \"右对齐\"")));
    QVERIFY(zhCnSource.contains(QStringLiteral("msgctxt \"top items alignment\"\nmsgid \"Top\"\nmsgstr \"顶部对齐\"")));
    QVERIFY(zhCnSource.contains(QStringLiteral("msgctxt \"bottom items alignment\"\nmsgid \"Bottom\"\nmsgstr \"底部对齐\"")));

    QFile layoutsContainer(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/layouts/LayoutsContainer.qml"));
    QVERIFY(layoutsContainer.open(QFile::ReadOnly));
    const QString layoutsSource = QString::fromUtf8(layoutsContainer.readAll());
    QVERIFY(layoutsSource.contains(QStringLiteral("readonly property int effectiveItemsAlignment: root.myView.alignment === LatteCore.types.Justify")));
    QVERIFY(layoutsSource.contains(QStringLiteral("? normalizedItemsAlignment(root.myView.itemsAlignment)")));
    QVERIFY(layoutsSource.contains(QStringLiteral(": root.myView.alignment")));
    QVERIFY(layoutsSource.contains(QStringLiteral("function normalizedItemsAlignment(alignment)")));

    QFile scrollableList(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/taskslayout/ScrollableList.qml"));
    QVERIFY(scrollableList.open(QFile::ReadOnly));
    const QString scrollableSource = QString::fromUtf8(scrollableList.readAll());
    QVERIFY(scrollableSource.contains(QStringLiteral("readonly property bool centered: root.alignment === LatteCore.types.Center")));
}

void SourceContractTest::itemsAlignmentNormalizesDirectionsByFormFactor()
{
    QFile containmentHost(QStringLiteral(LATTE_SOURCE_DIR "/declarativeimports/abilities/host/Containment.qml"));
    QVERIFY(containmentHost.open(QFile::ReadOnly));
    const QString containmentHostSource = QString::fromUtf8(containmentHost.readAll());

    QVERIFY(containmentHostSource.contains(QStringLiteral("if (plasmoid.formFactor === PlasmaCore.Types.Vertical)")));
    QVERIFY(containmentHostSource.contains(QStringLiteral("return alignment === LatteCore.types.Top || alignment === LatteCore.types.Bottom ? alignment : LatteCore.types.Center;")));
    QVERIFY(containmentHostSource.contains(QStringLiteral("return alignment === LatteCore.types.Left || alignment === LatteCore.types.Right ? alignment : LatteCore.types.Center;")));
    QVERIFY(!containmentHostSource.contains(QStringLiteral("return plasmoid.formFactor === PlasmaCore.Types.Horizontal ? LatteCore.types.Left : LatteCore.types.Top;")));
    QVERIFY(!containmentHostSource.contains(QStringLiteral("return plasmoid.formFactor === PlasmaCore.Types.Horizontal ? LatteCore.types.Right : LatteCore.types.Bottom;")));

    QFile layoutsContainer(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/layouts/LayoutsContainer.qml"));
    QVERIFY(layoutsContainer.open(QFile::ReadOnly));
    const QString layoutsSource = QString::fromUtf8(layoutsContainer.readAll());

    QVERIFY(layoutsSource.contains(QStringLiteral("if (root.isVertical)")));
    QVERIFY(layoutsSource.contains(QStringLiteral("return alignment === LatteCore.types.Top || alignment === LatteCore.types.Bottom ? alignment : LatteCore.types.Center;")));
    QVERIFY(layoutsSource.contains(QStringLiteral("return alignment === LatteCore.types.Left || alignment === LatteCore.types.Right ? alignment : LatteCore.types.Center;")));
    QVERIFY(layoutsSource.contains(QStringLiteral("if (effectiveItemsAlignment === LatteCore.types.Top) return LatteCore.types.LeftEdgeTopAlign;")));
    QVERIFY(layoutsSource.contains(QStringLiteral("if (effectiveItemsAlignment === LatteCore.types.Bottom) return LatteCore.types.LeftEdgeBottomAlign;")));
    QVERIFY(layoutsSource.contains(QStringLiteral("if ((effectiveItemsAlignment === LatteCore.types.Left && !reversed)")));
    QVERIFY(layoutsSource.contains(QStringLiteral("|| (effectiveItemsAlignment === LatteCore.types.Right && reversed))")));
}

void SourceContractTest::itemsAlignmentConfigDefaultsToCenter()
{
    QFile config(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/config/main.xml"));
    QVERIFY(config.open(QFile::ReadOnly));
    const QString configSource = QString::fromUtf8(config.readAll());

    const int entryStart = configSource.indexOf(QStringLiteral("<entry name=\"itemsAlignment\" type=\"Int\">"));
    QVERIFY(entryStart >= 0);
    const int entryEnd = configSource.indexOf(QStringLiteral("</entry>"), entryStart);
    QVERIFY(entryEnd > entryStart);

    const QString entry = configSource.mid(entryStart, entryEnd - entryStart);
    QVERIFY(entry.contains(QStringLiteral("<default>0</default>")));
    QVERIFY(entry.contains(QStringLiteral("dock icons/items alignment used only when alignment is Justify")));
}

void SourceContractTest::appearancePaletteExposesLayoutCustomColors()
{
    QFile appearance(QStringLiteral(LATTE_SOURCE_DIR "/shell/package/contents/configuration/pages/AppearanceConfig.qml"));
    QVERIFY(appearance.open(QFile::ReadOnly));
    const QString appearanceSource = QString::fromUtf8(appearance.readAll());

    QVERIFY(appearanceSource.contains(QStringLiteral("text: i18n(\"Colors\")")));
    QVERIFY(appearanceSource.contains(QStringLiteral("text: i18n(\"Palette\")")));
    QVERIFY(appearanceSource.contains(QStringLiteral("name: i18nc(\"layout custom colors\", \"Layout Custom Colors\")")));
    QVERIFY(appearanceSource.contains(QStringLiteral("value: LatteContainment.types.LayoutThemeColors")));
    QVERIFY(appearanceSource.contains(QStringLiteral("currentIndex: colorsToIndex(plasmoid.configuration.themeColors)")));
    QVERIFY(appearanceSource.contains(QStringLiteral("onCurrentIndexChanged: plasmoid.configuration.themeColors = model[currentIndex].value")));

    QFile containmentTypes(QStringLiteral(LATTE_SOURCE_DIR "/containment/plugin/types.h"));
    QVERIFY(containmentTypes.open(QFile::ReadOnly));
    const QString typesSource = QString::fromUtf8(containmentTypes.readAll());
    QVERIFY(typesSource.contains(QStringLiteral("LayoutThemeColors")));

    QFile colorizer(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/colorizer/Manager.qml"));
    QVERIFY(colorizer.open(QFile::ReadOnly));
    const QString colorizerSource = QString::fromUtf8(colorizer.readAll());
    QVERIFY(colorizerSource.contains(QStringLiteral("root.themeColors === LatteContainment.types.LayoutThemeColors")));
    QVERIFY(colorizerSource.contains(QStringLiteral("latteView && latteView.layout && latteView.layout.scheme")));
    QVERIFY(colorizerSource.contains(QStringLiteral("return latteView.layout.scheme;")));
}

void SourceContractTest::modernDockBackgroundShadowDefaultIsCompact()
{
    QFile constantsFile(QStringLiteral(LATTE_SOURCE_DIR "/declarativeimports/abilities/definition/metrics/Constants.qml"));
    QVERIFY(constantsFile.open(QFile::ReadOnly));
    const QString constantsSource = QString::fromUtf8(constantsFile.readAll());
    QVERIFY(constantsSource.contains(QStringLiteral("kModernBackgroundShadowMinPixels: 6")));

    QFile qmlDirFile(QStringLiteral(LATTE_SOURCE_DIR "/declarativeimports/abilities/definition/qmldir"));
    QVERIFY(qmlDirFile.open(QFile::ReadOnly));
    const QString qmlDirSource = QString::fromUtf8(qmlDirFile.readAll());
    QVERIFY(qmlDirSource.contains(QStringLiteral("singleton MetricsConstants 0.1 metrics/Constants.qml")));

    QFile backgroundFile(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/background/MultiLayered.qml"));
    QVERIFY(backgroundFile.open(QFile::ReadOnly));
    const QString backgroundSource = QString::fromUtf8(backgroundFile.readAll());
    QVERIFY(backgroundSource.contains(QStringLiteral("import org.kde.latte.abilities.definition as AbilityDefinition")));
    QVERIFY(backgroundSource.contains(QStringLiteral("AbilityDefinition.MetricsConstants.kModernBackgroundShadowMinPixels")));
    QVERIFY(backgroundSource.contains(
                QStringLiteral("if (modernDockStyle && customDefShadowIsEnabled) {\n            return AbilityDefinition.MetricsConstants.kModernBackgroundShadowMinPixels;\n        }")));
    QVERIFY(backgroundSource.contains(QStringLiteral("return customShadow; //! Modern default")));
    QVERIFY(!backgroundSource.contains(QStringLiteral("return Math.max(10, customShadow); //! Modern default")));
    QVERIFY(!backgroundSource.contains(QStringLiteral("return Math.max(AbilityDefinition.MetricsConstants.kModernBackgroundShadowMinPixels, customShadow);")));
}

void SourceContractTest::layoutDetailsExposeCustomColorSchemeSelector()
{
    QFile detailsUi(QStringLiteral(LATTE_SOURCE_DIR "/app/settings/detailsdialog/detailsdialog.ui"));
    QVERIFY(detailsUi.open(QFile::ReadOnly));
    const QString uiSource = QString::fromUtf8(detailsUi.readAll());
    QVERIFY(uiSource.contains(QStringLiteral("<string>Custom Colors:</string>")));
    QVERIFY(uiSource.contains(QStringLiteral("Latte::Settings::SchemesComboBox")));
    QVERIFY(uiSource.contains(QStringLiteral("name=\"customSchemeCmb\"")));

    QFile detailsHandler(QStringLiteral(LATTE_SOURCE_DIR "/app/settings/detailsdialog/detailshandler.cpp"));
    QVERIFY(detailsHandler.open(QFile::ReadOnly));
    const QString handlerSource = QString::fromUtf8(detailsHandler.readAll());
    QVERIFY(handlerSource.contains(QStringLiteral("m_ui->customSchemeCmb->setModel(m_schemesModel);")));
    QVERIFY(handlerSource.contains(QStringLiteral("connect(m_ui->customSchemeCmb, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DetailsHandler::onCurrentSchemeIndexChanged);")));
    QVERIFY(handlerSource.contains(QStringLiteral("QString selectedScheme = m_ui->customSchemeCmb->itemData(row, Model::Schemes::IDROLE).toString();")));
    QVERIFY(handlerSource.contains(QStringLiteral("c_data.schemeFile = file;")));
}

void SourceContractTest::showWindowAnimationContractMovedToQmlSmokeTest()
{
    QFile qmlSmoke(QStringLiteral(LATTE_SOURCE_DIR "/autotests/qmlsmoketest.cpp"));
    QVERIFY(qmlSmoke.open(QFile::ReadOnly));
    const QString qmlSmokeSource = QString::fromUtf8(qmlSmoke.readAll());
    QVERIFY(qmlSmokeSource.contains(QStringLiteral("showWindowAnimationFrozenZoomDecisionLoadsFromSource")));
    QVERIFY(qmlSmokeSource.contains(QStringLiteral("LATTE_SHOW_WINDOW_ANIMATION_QML")));
    QVERIFY(qmlSmokeSource.contains(QStringLiteral("keepFrozenZoomForCurrentTask")));

    QFile sourceContracts(QStringLiteral(LATTE_SOURCE_DIR "/autotests/sourcecontracttest.cpp"));
    QVERIFY(sourceContracts.open(QFile::ReadOnly));
    const QString sourceContractSource = QString::fromUtf8(sourceContracts.readAll());
    const QString oldSourceLock = QStringLiteral("QFile ") + QStringLiteral("showWindowAnimation");
    QVERIFY(!sourceContractSource.contains(oldSourceLock));
}

void SourceContractTest::parabolicItemContractMovedToQmlSmokeTest()
{
    QFile qmlSmoke(QStringLiteral(LATTE_SOURCE_DIR "/autotests/qmlsmoketest.cpp"));
    QVERIFY(qmlSmoke.open(QFile::ReadOnly));
    const QString qmlSmokeSource = QString::fromUtf8(qmlSmoke.readAll());
    QVERIFY(qmlSmokeSource.contains(QStringLiteral("parabolicItemZoomRecoveryLoadsFromSource")));
    QVERIFY(qmlSmokeSource.contains(QStringLiteral("LATTE_PARABOLIC_ITEM_QML")));
    QVERIFY(qmlSmokeSource.contains(QStringLiteral("sendEndOfNeedBothAxisAnimation")));

    QFile sourceContracts(QStringLiteral(LATTE_SOURCE_DIR "/autotests/sourcecontracttest.cpp"));
    QVERIFY(sourceContracts.open(QFile::ReadOnly));
    const QString sourceContractSource = QString::fromUtf8(sourceContracts.readAll());
    const QString oldSourceLock = QStringLiteral("QFile ") + QStringLiteral("parabolicItem");
    QVERIFY(!sourceContractSource.contains(oldSourceLock));
}

void SourceContractTest::autotestAggregateTargetDocumentsFullSuiteBuild()
{
    QFile autotestsCMake(QStringLiteral(LATTE_SOURCE_DIR "/autotests/CMakeLists.txt"));
    QVERIFY(autotestsCMake.open(QFile::ReadOnly));
    const QString cmakeSource = QString::fromUtf8(autotestsCMake.readAll());
    QVERIFY(cmakeSource.contains(QStringLiteral("set(latte_autotest_targets")));
    QVERIFY(cmakeSource.contains(QStringLiteral("add_custom_target(latte-autotests")));
    QVERIFY(cmakeSource.contains(QStringLiteral("DEPENDS ${latte_autotest_targets}")));
    QVERIFY(cmakeSource.contains(QStringLiteral("sourcecontracttest")));
    QVERIFY(cmakeSource.contains(QStringLiteral("packagingcontracttest")));

    QFile testingGuide(QStringLiteral(LATTE_SOURCE_DIR "/docs/development-testing-guide.md"));
    QVERIFY(testingGuide.open(QFile::ReadOnly));
    const QString guideSource = QString::fromUtf8(testingGuide.readAll());
    QVERIFY(guideSource.contains(QStringLiteral("cmake --build build-autotests-gcc --target latte-autotests --parallel 8")));
    QVERIFY(guideSource.contains(QStringLiteral("cmake --build build-autotests-clang --target latte-autotests --parallel 8")));
}

void SourceContractTest::coverageEstimateUsesReusableScript()
{
    QFile coverageScript(QStringLiteral(LATTE_SOURCE_DIR "/autotests/coverageestimate.py"));
    QVERIFY(coverageScript.open(QFile::ReadOnly));
    const QString scriptSource = QString::fromUtf8(coverageScript.readAll());
    QVERIFY(scriptSource.contains(QStringLiteral("\"ls-files\"")));
    QVERIFY(scriptSource.contains(QStringLiteral("git_files(\"*.cpp\")")));
    QVERIFY(scriptSource.contains(QStringLiteral("lattecoreplugin.cpp")));

    QFile testingGuide(QStringLiteral(LATTE_SOURCE_DIR "/docs/development-testing-guide.md"));
    QVERIFY(testingGuide.open(QFile::ReadOnly));
    const QString guideSource = QString::fromUtf8(testingGuide.readAll());
    QVERIFY(guideSource.contains(QStringLiteral("python3 autotests/coverageestimate.py")));
    QVERIFY(!guideSource.contains(QStringLiteral("python3 - <<'PY'")));
}

void SourceContractTest::cmakeTargetResolutionUsesSharedHelpers()
{
    QFile module(QStringLiteral(LATTE_SOURCE_DIR "/cmake/LatteTargetResolution.cmake"));
    QVERIFY(module.open(QFile::ReadOnly));
    const QString moduleSource = QString::fromUtf8(module.readAll());

    QFile cmake(QStringLiteral(LATTE_SOURCE_DIR "/CMakeLists.txt"));
    QVERIFY(cmake.open(QFile::ReadOnly));
    const QString cmakeSource = QString::fromUtf8(cmake.readAll());

    QVERIFY(moduleSource.contains(QStringLiteral("function(latte_resolve_target_from_candidates")));
    QVERIFY(moduleSource.contains(QStringLiteral("function(latte_resolve_library_variable")));
    QVERIFY(cmakeSource.contains(QStringLiteral("latte_resolve_target_from_candidates(LATTE_NEWSTUFF_TARGET")));
    QVERIFY(cmakeSource.contains(QStringLiteral("latte_resolve_library_variable(LATTE_NEWSTUFF_TARGET")));
    QVERIFY(cmakeSource.contains(QStringLiteral("latte_resolve_target_from_candidates(LATTE_WAYLANDCLIENT_TARGET")));
    QVERIFY(cmakeSource.contains(QStringLiteral("latte_resolve_library_variable(LATTE_WAYLANDCLIENT_TARGET")));
}

void SourceContractTest::cmakeImportedTargetResolutionUsesSharedHelper()
{
    QFile module(QStringLiteral(LATTE_SOURCE_DIR "/cmake/LatteTargetResolution.cmake"));
    QVERIFY(module.open(QFile::ReadOnly));
    const QString moduleSource = QString::fromUtf8(module.readAll());

    QFile cmake(QStringLiteral(LATTE_SOURCE_DIR "/CMakeLists.txt"));
    QVERIFY(cmake.open(QFile::ReadOnly));
    const QString cmakeSource = QString::fromUtf8(cmake.readAll());

    QVERIFY(moduleSource.contains(QStringLiteral("function(latte_resolve_imported_target")));
    QVERIFY(cmakeSource.contains(QStringLiteral("latte_resolve_imported_target(LATTE_NEWSTUFF_TARGET \"newstuff\" \"widget\")")));
    QVERIFY(cmakeSource.contains(QStringLiteral("latte_resolve_imported_target(LATTE_NEWSTUFF_TARGET \"newstuff\")")));
    QVERIFY(cmakeSource.contains(QStringLiteral("latte_resolve_imported_target(LATTE_WAYLANDCLIENT_TARGET \"waylandclient|kwayland::client\")")));
    QVERIFY(cmakeSource.contains(QStringLiteral("latte_resolve_imported_target(LATTE_CONFIGQML_TARGET \"configqml\")")));
    QVERIFY(cmakeSource.contains(QStringLiteral("latte_resolve_imported_target(LATTE_SVG_TARGET \"::svg|ksvg\")")));
}

void SourceContractTest::cmakeTargetResolutionHelpersLiveInModule()
{
    QFile module(QStringLiteral(LATTE_SOURCE_DIR "/cmake/LatteTargetResolution.cmake"));
    QVERIFY(module.open(QFile::ReadOnly));
    const QString moduleSource = QString::fromUtf8(module.readAll());
    QVERIFY(moduleSource.contains(QStringLiteral("function(latte_resolve_target_from_candidates")));
    QVERIFY(moduleSource.contains(QStringLiteral("function(latte_resolve_library_variable")));
    QVERIFY(moduleSource.contains(QStringLiteral("function(latte_resolve_imported_target")));

    QFile cmake(QStringLiteral(LATTE_SOURCE_DIR "/CMakeLists.txt"));
    QVERIFY(cmake.open(QFile::ReadOnly));
    const QString cmakeSource = QString::fromUtf8(cmake.readAll());
    QVERIFY(cmakeSource.contains(QStringLiteral("include(LatteTargetResolution)")));
    QVERIFY(!cmakeSource.contains(QStringLiteral("function(latte_resolve_target_from_candidates")));
    QVERIFY(!cmakeSource.contains(QStringLiteral("function(latte_resolve_library_variable")));
    QVERIFY(!cmakeSource.contains(QStringLiteral("function(latte_resolve_imported_target")));
}

void SourceContractTest::cmakeOffscreenTestsUseSharedHelper()
{
    QFile autotestsCMake(QStringLiteral(LATTE_SOURCE_DIR "/autotests/CMakeLists.txt"));
    QVERIFY(autotestsCMake.open(QFile::ReadOnly));
    const QString cmakeSource = QString::fromUtf8(autotestsCMake.readAll());

    QVERIFY(cmakeSource.contains(QStringLiteral("function(latte_add_offscreen_test")));
    // APPEND keeps the offscreen platform from clobbering the forced-English
    // locale environment set by latte_add_test().
    QVERIFY(cmakeSource.contains(QStringLiteral("set_property(TEST ${_test_name} APPEND PROPERTY ENVIRONMENT \"QT_QPA_PLATFORM=offscreen\")")));
    QVERIFY(cmakeSource.contains(QStringLiteral("set_property(TEST ${_test_name} APPEND PROPERTY ENVIRONMENT \"LANGUAGE=en_US\")")));
    QVERIFY(cmakeSource.contains(QStringLiteral("latte_add_offscreen_test(coreunittest)")));
    QVERIFY(cmakeSource.contains(QStringLiteral("latte_add_offscreen_test(qmlsmoketest)")));
    QVERIFY(cmakeSource.contains(QStringLiteral("latte_add_offscreen_test(sourcecontracttest)")));
    QVERIFY(cmakeSource.contains(QStringLiteral("latte_add_offscreen_test(containmentactionmenuunittest)")));
    QVERIFY(!cmakeSource.contains(QStringLiteral("set_tests_properties(coreunittest PROPERTIES ENVIRONMENT \"QT_QPA_PLATFORM=offscreen\")")));
    QVERIFY(!cmakeSource.contains(QStringLiteral("set_tests_properties(settingsviewunittest PROPERTIES ENVIRONMENT \"QT_QPA_PLATFORM=offscreen\")")));
}

void SourceContractTest::cmakeAutotestRegistrationMaintainsAggregateTarget()
{
    QFile autotestsCMake(QStringLiteral(LATTE_SOURCE_DIR "/autotests/CMakeLists.txt"));
    QVERIFY(autotestsCMake.open(QFile::ReadOnly));
    const QString cmakeSource = QString::fromUtf8(autotestsCMake.readAll());

    QVERIFY(cmakeSource.contains(QStringLiteral("function(latte_add_test _test_name)")));
    QVERIFY(cmakeSource.contains(QStringLiteral("add_test(NAME ${_test_name} COMMAND ${_test_name})")));
    QVERIFY(cmakeSource.contains(QStringLiteral("list(APPEND latte_autotest_targets ${_test_name})")));
    QVERIFY(cmakeSource.contains(QStringLiteral("set(latte_autotest_targets ${latte_autotest_targets} PARENT_SCOPE)")));
    QVERIFY(cmakeSource.contains(QStringLiteral("set(latte_autotest_targets)")));
    QVERIFY(cmakeSource.contains(QStringLiteral("latte_add_test(dataunittest)")));
    QVERIFY(cmakeSource.contains(QStringLiteral("latte_add_test(modelunittest)")));
    QVERIFY(cmakeSource.contains(QStringLiteral("latte_add_offscreen_test(qmlsmoketest)")));
    QVERIFY(cmakeSource.contains(QStringLiteral("DEPENDS ${latte_autotest_targets}")));
    QVERIFY(!cmakeSource.contains(QStringLiteral("add_test(NAME dataunittest COMMAND dataunittest)")));
    QVERIFY(!cmakeSource.contains(QStringLiteral("set(latte_autotest_targets\n    dataunittest")));
}

void SourceContractTest::cmakePackagingConfigLivesInModule()
{
    QFile module(QStringLiteral(LATTE_SOURCE_DIR "/cmake/LattePackaging.cmake"));
    QVERIFY(module.open(QFile::ReadOnly));
    const QString moduleSource = QString::fromUtf8(module.readAll());
    QVERIFY(moduleSource.contains(QStringLiteral("set(CPACK_PACKAGE_NAME \"latte-dock-ng\")")));
    QVERIFY(moduleSource.contains(QStringLiteral("set(CPACK_RPM_PACKAGE_REQUIRES \"libKirigami.so.6, libKF6KCMUtils.so.6, libKF6NewStuffCore.so.6\")")));
    QVERIFY(moduleSource.contains(QStringLiteral("set(CPACK_DEBIAN_PACKAGE_DEPENDS \"qml6-module-org-kde-kirigami, qml6-module-org-kde-kcmutils, qml6-module-org-kde-newstuff\")")));
    QVERIFY(moduleSource.contains(QStringLiteral("include(CPack)")));

    QFile cmake(QStringLiteral(LATTE_SOURCE_DIR "/CMakeLists.txt"));
    QVERIFY(cmake.open(QFile::ReadOnly));
    const QString cmakeSource = QString::fromUtf8(cmake.readAll());
    QVERIFY(cmakeSource.contains(QStringLiteral("include(LattePackaging)")));
    QVERIFY(!cmakeSource.contains(QStringLiteral("set(CPACK_PACKAGE_NAME \"latte-dock-ng\")")));
    QVERIFY(!cmakeSource.contains(QStringLiteral("set(CPACK_RPM_PACKAGE_REQUIRES")));
    QVERIFY(!cmakeSource.contains(QStringLiteral("set(CPACK_DEBIAN_PACKAGE_DEPENDS")));

    QFile testingGuide(QStringLiteral(LATTE_SOURCE_DIR "/docs/development-testing-guide.md"));
    QVERIFY(testingGuide.open(QFile::ReadOnly));
    const QString guideSource = QString::fromUtf8(testingGuide.readAll());
    QVERIFY(guideSource.contains(QStringLiteral("CMake helper modules keep target resolution, compiler warning relaxation, and packaging metadata out of the top-level build file.")));
}

void SourceContractTest::autostartUsesXdgStateOnly()
{
    QFile universalHeader(QStringLiteral(LATTE_SOURCE_DIR "/app/settings/universalsettings.h"));
    QVERIFY(universalHeader.open(QFile::ReadOnly));
    const QString headerSource = QString::fromUtf8(universalHeader.readAll());
    QVERIFY(headerSource.contains(QStringLiteral("Q_PROPERTY(bool autostart READ autostart WRITE setAutostart NOTIFY autostartChanged)")));
    QVERIFY(!headerSource.contains(QStringLiteral("ensureAutostart")));
    QVERIFY(!headerSource.contains(QStringLiteral("Systemd"), Qt::CaseInsensitive));

    QFile universalSettings(QStringLiteral(LATTE_SOURCE_DIR "/app/settings/universalsettings.cpp"));
    QVERIFY(universalSettings.open(QFile::ReadOnly));
    const QString universalSource = QString::fromUtf8(universalSettings.readAll());
    QVERIFY(!universalSource.contains(QStringLiteral("ensureAutostart")));
    QVERIFY(universalSource.contains(QStringLiteral("return Layouts::Importer::isAutostartEnabled();")));
}

void SourceContractTest::desktopFileHasAutostartPhaseKey()
{
    //! The shipped desktop file must include X-KDE-autostart-phase=2 so
    //! ksmserver launches latte-dock after the session is fully restored,
    //! avoiding races with Wayland compositor and DBus initialization.
    QFile desktopTemplate(QStringLiteral(LATTE_SOURCE_DIR "/app/org.kde.latte-dock.desktop.cmake"));
    QVERIFY(desktopTemplate.open(QFile::ReadOnly));
    const QString content = QString::fromUtf8(desktopTemplate.readAll());
    QVERIFY(content.contains(QStringLiteral("X-KDE-autostart-phase=2")));
}

void SourceContractTest::enableAutostartExitsImmediately()
{
    //! --enable-autostart must only set up the autostart file and exit,
    //! not proceed to full application startup (which would require a
    //! running Wayland compositor).
    QFile mainCpp(QStringLiteral(LATTE_SOURCE_DIR "/app/main.cpp"));
    QVERIFY(mainCpp.open(QFile::ReadOnly));
    const QString content = QString::fromUtf8(mainCpp.readAll());

    const int enableIdx = content.indexOf(QStringLiteral("\"enable-autostart\""));
    QVERIFY(enableIdx >= 0);

    const int exitCall = content.indexOf(QStringLiteral("qGuiApp->exit();"), enableIdx);
    QVERIFY(exitCall > enableIdx);

    const int returnZero = content.indexOf(QStringLiteral("return 0;"), exitCall);
    QVERIFY(returnZero > exitCall);
}

void SourceContractTest::autostartInterfacesUseXdgState()
{
    QFile preferences(QStringLiteral(LATTE_SOURCE_DIR "/app/settings/settingsdialog/tabpreferenceshandler.cpp"));
    QVERIFY(preferences.open(QFile::ReadOnly));
    const QString preferencesSource = QString::fromUtf8(preferences.readAll());
    QVERIFY(preferencesSource.contains(QStringLiteral("universalSettings()->autostart()")));
    QVERIFY(preferencesSource.contains(QStringLiteral("universalSettings()->setAutostart(m_preferences.autostart)")));

    QFile corona(QStringLiteral(LATTE_SOURCE_DIR "/app/lattecorona.cpp"));
    QVERIFY(corona.open(QFile::ReadOnly));
    const QString coronaSource = QString::fromUtf8(corona.readAll());
    QVERIFY(coronaSource.contains(QStringLiteral("m_universalSettings->setAutostart(enabled);")));

    QFile mainCpp(QStringLiteral(LATTE_SOURCE_DIR "/app/main.cpp"));
    QVERIFY(mainCpp.open(QFile::ReadOnly));
    const QString mainSource = QString::fromUtf8(mainCpp.readAll());
    QVERIFY(!mainSource.contains(QStringLiteral("ensureAutostart")));
}

void SourceContractTest::autostartPreservesEntryFields()
{
    QFile autostart(QStringLiteral(LATTE_SOURCE_DIR "/app/settings/autostart.cpp"));
    QVERIFY(autostart.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(autostart.readAll());

    QVERIFY(source.contains(QStringLiteral("writeEntry(QStringLiteral(\"Hidden\"), false)")));
    QVERIFY(source.contains(QStringLiteral("writeEntry(QStringLiteral(\"Hidden\"), true)")));
    QVERIFY(!source.contains(QStringLiteral("lastModified")));
    QVERIFY(!source.contains(QStringLiteral("systemctl"), Qt::CaseInsensitive));
}

void SourceContractTest::firstRunEnablesDefaultAutostart()
{
    QFile manager(QStringLiteral(LATTE_SOURCE_DIR "/app/layouts/manager.cpp"));
    QVERIFY(manager.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(manager.readAll());

    const int firstRun = source.indexOf(QStringLiteral("if (firstRun)"));
    const int nextBranch = source.indexOf(QStringLiteral("} else if (configVer < 2"), firstRun);
    QVERIFY(firstRun >= 0);
    QVERIFY(nextBranch > firstRun);

    const QString firstRunBody = source.mid(firstRun, nextBranch - firstRun);
    QVERIFY(firstRunBody.contains(QStringLiteral("Layouts::Importer::enableAutostart();")));
    QVERIFY(firstRunBody.contains(QStringLiteral("Layouts::Importer::autostartEntryExists()")));
}

void SourceContractTest::waylandCheckHasRetryMechanism()
{
    //! The Wayland platform check during startup must retry instead of
    //! failing immediately, so cold-boot timing races with the compositor
    //! do not cause a permanent autostart failure.
    QFile mainCpp(QStringLiteral(LATTE_SOURCE_DIR "/app/main.cpp"));
    QVERIFY(mainCpp.open(QFile::ReadOnly));
    const QString content = QString::fromUtf8(mainCpp.readAll());

    const int waylandCheck = content.indexOf(QStringLiteral("isPlatformWayland()"));
    QVERIFY(waylandCheck >= 0);

    const int maxRetries = content.indexOf(QStringLiteral("maxRetries"), waylandCheck);
    QVERIFY(maxRetries > waylandCheck);

    const int msleep = content.indexOf(QStringLiteral("QThread::msleep"), waylandCheck);
    QVERIFY(msleep > waylandCheck);
}

void SourceContractTest::normalStartupDoesNotMutateAutostart()
{
    QFile universalSettings(QStringLiteral(LATTE_SOURCE_DIR "/app/settings/universalsettings.cpp"));
    QVERIFY(universalSettings.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(universalSettings.readAll());

    const int loadStart = source.indexOf(QStringLiteral("void UniversalSettings::load()"));
    const int nextMethod = source.indexOf(QStringLiteral("bool UniversalSettings::inAdvancedModeForEditSettings() const"), loadStart);
    QVERIFY(loadStart >= 0);
    QVERIFY(nextMethod > loadStart);
    const QString loadBody = source.mid(loadStart, nextMethod - loadStart);
    QVERIFY(!loadBody.contains(QStringLiteral("Autostart"), Qt::CaseInsensitive));
}

void SourceContractTest::autoSizeLoopsUseInequalityNotStrictEquality()
{
    //! AutoSize.qml uses automaticStep=8 in its shrink/grow do-while
    //! loops.  The termination guards must use inequality (> / <), not
    //! strict equality (!==), otherwise icon sizes whose difference from
    //! the boundary is not a multiple of 8 skip past the guard value and
    //! loop forever at ~100% CPU.
    QFile autoSize(QStringLiteral(LATTE_SOURCE_DIR
                                  "/containment/package/contents/ui/abilities/AutoSize.qml"));
    QVERIFY(autoSize.open(QFile::ReadOnly));
    const QString content = QString::fromUtf8(autoSize.readAll());

    QVERIFY(content.contains(QStringLiteral("(nextIconSize > 16)")));
    QVERIFY(!content.contains(QStringLiteral("(nextIconSize !== 16)")));

    QVERIFY(content.contains(QStringLiteral("(nextIconSize2 < metrics.maxIconSize )")));
    QVERIFY(!content.contains(QStringLiteral("(nextIconSize2 !== metrics.maxIconSize )")));
}

void SourceContractTest::cmakeWarningRelaxationLivesInModule()
{
    // The relaxed-warning-flags module has been removed: KDECompilerSettings
    // warning flags must stay enabled so regressions surface as warnings.
    QFile module(QStringLiteral(LATTE_SOURCE_DIR "/cmake/LatteCompilerWarnings.cmake"));
    QVERIFY(!module.exists());

    QFile cmake(QStringLiteral(LATTE_SOURCE_DIR "/CMakeLists.txt"));
    QVERIFY(cmake.open(QFile::ReadOnly));
    const QString cmakeSource = QString::fromUtf8(cmake.readAll());
    QVERIFY(!cmakeSource.contains(QStringLiteral("include(LatteCompilerWarnings)")));
    QVERIFY(!cmakeSource.contains(QStringLiteral("latte_apply_relaxed_warning_flags()")));
}

void SourceContractTest::cmakeFindsQtCoreToolsBeforeKdeInstallDirs()
{
    QFile cmake(QStringLiteral(LATTE_SOURCE_DIR "/CMakeLists.txt"));
    QVERIFY(cmake.open(QFile::ReadOnly));
    const QString cmakeSource = QString::fromUtf8(cmake.readAll());

    const qsizetype qtCoreToolsIndex = cmakeSource.indexOf(QStringLiteral("find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED NO_MODULE COMPONENTS CoreTools"));
    const qsizetype kdeInstallDirsIndex = cmakeSource.indexOf(QStringLiteral("include(KDEInstallDirs)"));
    QVERIFY(qtCoreToolsIndex >= 0);
    QVERIFY(kdeInstallDirsIndex >= 0);
    QVERIFY(qtCoreToolsIndex < kdeInstallDirsIndex);
}

void SourceContractTest::compactAppletDigitalClockWidthCapPreventsLongDateFormatOverflow()
{
    QFile appletFile(QStringLiteral(LATTE_SOURCE_DIR "/shell/package/contents/applet/CompactApplet.qml"));
    QVERIFY(appletFile.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(appletFile.readAll());

    // Both captureNaturalSize and updateNaturalWidth must cap at 5×
    // maxIconSize to prevent text-heavy clocks (e.g. Colorful Digital
    // Clock) from occupying excessive horizontal space, while still
    // accommodating long / iso date formats.
    QVERIFY(source.contains(QStringLiteral("maxIconSize * 5")));
    QVERIFY(!source.contains(QStringLiteral("maxIconSize * 8")));

    // maxIconSize * 3 is used for the height cap, not the width cap.
    // Verify it only appears in the height-capping context.
    const int heightCap = source.indexOf(QStringLiteral("maxIconSize * 3"));
    QVERIFY(heightCap > 0);
    QVERIFY(source.lastIndexOf(QStringLiteral("maxIconSize * 3")) == heightCap);

    // Both sizing functions must still exist.
    QVERIFY(source.contains(QStringLiteral("function captureNaturalSize()")));
    QVERIFY(source.contains(QStringLiteral("function updateNaturalWidth()")));

    // The isTextApplet gate must match clock plugins by the generic
    // "clock" substring so third-party clocks like Colorful Digital Clock
    // (co.n7k.plasma.digitalclock) are included.
    QVERIFY(source.contains(QStringLiteral("indexOf(\"clock\") >= 0")));

    // "analogclock" must be explicitly excluded from the isTextApplet
    // check so that square analog clock faces keep the standard
    // icon-size slot behavior.
    QVERIFY(source.contains(QStringLiteral("indexOf(\"analogclock\") < 0")));

    // externalAppletDrawsAboveTasks must guard both functions so
    // internal applets never receive the wider slot treatment.
    QVERIFY(source.contains(QStringLiteral("externalAppletDrawsAboveTasks")));

    // findDeepImplicitWidth fallback must exist — the digital clock's
    // compactRepresentation has implicitWidth=0 and relies on child
    // recursion to discover the actual content width. It must skip
    // hidden children so third-party clocks (e.g. Colorful Digital
    // Clock) with format-specific labels don't report stale widths
    // from invisible long-format labels.
    QVERIFY(source.contains(QStringLiteral("function findDeepImplicitWidth")));
    QVERIFY(source.contains(QStringLiteral("!child.visible")));

    // The 500 ms polling timer must exist to keep the slot width in sync
    // when clock text content changes (e.g. "1:00" → "12:34") and the
    // compact representation does not emit implicitWidthChanged on its own.
    QVERIFY(source.contains(QStringLiteral("naturalWidthPollTimer")));

    // The deferred capture timer must exist to avoid touching the hot
    // anchoring path for non-clock applets.
    QVERIFY(source.contains(QStringLiteral("slotSizeCaptureTimer")));

    // Signal-driven updates must supplement the polling timer so the slot
    // width responds immediately when the clock's compact representation
    // signals a change.  onImplicitHeightChanged and onChildrenChanged are
    // also wired for third-party clocks (e.g. Colorful Digital Clock) whose
    // compact representation may not emit implicitWidthChanged on format switch.
    QVERIFY(source.contains(QStringLiteral("onImplicitWidthChanged")));
    QVERIFY(source.contains(QStringLiteral("onImplicitHeightChanged")));
    QVERIFY(source.contains(QStringLiteral("onChildrenRectChanged")));
    QVERIFY(source.contains(QStringLiteral("onChildrenChanged")));
    QVERIFY(source.contains(QStringLiteral("updateNaturalWidth();")));

    // The cap must appear inside captureNaturalSize (the first occurrence)
    // and inside updateNaturalWidth (the second occurrence).
    const int captureFn = source.indexOf(QStringLiteral("function captureNaturalSize()"));
    const int updateFn = source.indexOf(QStringLiteral("function updateNaturalWidth()"));
    const int firstCap = source.indexOf(QStringLiteral("maxIconSize * 5"), captureFn);
    const int secondCap = source.indexOf(QStringLiteral("maxIconSize * 5"), firstCap + QStringLiteral("maxIconSize * 5").length());
    QVERIFY(captureFn >= 0);
    QVERIFY(updateFn > captureFn);
    QVERIFY(firstCap > captureFn);
    QVERIFY(firstCap < updateFn);
    QVERIFY(secondCap > updateFn);
}

void SourceContractTest::contextMenuLayerMiddleClickCloseActiveWindowGuardedCorrectly()
{
    QFile implFile(QStringLiteral(LATTE_SOURCE_DIR "/app/declarativeimports/contextmenulayerquickitem.cpp"));
    QVERIFY(implFile.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(implFile.readAll());

    QFile headerFile(QStringLiteral(LATTE_SOURCE_DIR "/app/declarativeimports/contextmenulayerquickitem.h"));
    QVERIFY(headerFile.open(QFile::ReadOnly));
    const QString hdr = QString::fromUtf8(headerFile.readAll());

    // The middle-click close-active-window action must be guarded by both
    // the Q_PROPERTY toggle and an explicit Qt::MiddleButton check inside
    // mousePressEvent.  Without the MiddleButton check the handler would
    // intercept middle clicks even when no plugin is registered (commit
    // 14c98b9cc).
    QVERIFY(src.contains(QStringLiteral("m_closeActiveWindowEnabled && event->button() == Qt::MiddleButton")));

    // Middle-click guard must be inside mousePressEvent, not elsewhere.
    const int mpe = src.indexOf(QStringLiteral("ContextMenuLayerQuickItem::mousePressEvent"));
    const int midGuard = src.indexOf(QStringLiteral("m_closeActiveWindowEnabled && event->button() == Qt::MiddleButton"));
    QVERIFY(mpe >= 0);
    QVERIFY(midGuard > mpe);

    // The close action must call requestClose() on the last active window.
    const int requestClose = src.indexOf(QStringLiteral("requestClose()"), midGuard);
    QVERIFY(requestClose > midGuard);

    // Q_PROPERTY declaration must exist so QML can bridge the config value.
    QVERIFY(hdr.contains(QStringLiteral("Q_PROPERTY(bool closeActiveWindowEnabled")));

    // The C++ setter must exist.
    QVERIFY(hdr.contains(QStringLiteral("setCloseActiveWindowEnabled")));
    QVERIFY(src.contains(QStringLiteral("ContextMenuLayerQuickItem::setCloseActiveWindowEnabled")));
}

void SourceContractTest::appletIconOverrideStripsSymbolicForOriginalColors()
{
    QFile implFile(QStringLiteral(LATTE_SOURCE_DIR "/app/view/containmentinterface.cpp"));
    QVERIFY(implFile.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(implFile.readAll());

    QFile headerFile(QStringLiteral(LATTE_SOURCE_DIR "/app/view/containmentinterface.h"));
    QVERIFY(headerFile.open(QFile::ReadOnly));
    const QString hdr = QString::fromUtf8(headerFile.readAll());

    // "Keep original icon colors" must undo Plasma's panel "-symbolic"
    // icon substitution (issue #44): strip the suffix via setIcon, gated on
    // the userBlocksColorizing list, and only when a full-color variant
    // exists so symbolic-only applets are untouched. This must apply to every
    // applet that the user asks to keep in its original icon-theme colors.
    QVERIFY(src.contains(QStringLiteral("applyOriginalIconColors")));
    QVERIFY(src.contains(QStringLiteral("-symbolic")));
    QVERIFY(src.contains(QStringLiteral("QIcon::fromTheme")));
    QVERIFY(src.contains(QStringLiteral("setIcon")));
    QVERIFY(src.contains(QStringLiteral("originalColorIconName")));
    QVERIFY(src.contains(QStringLiteral("iconExistsInCurrentTheme")));
    QVERIFY(src.contains(QStringLiteral("KIconTheme::current")));
    QVERIFY(src.contains(QStringLiteral("QStandardPaths::locateAll")));
    QVERIFY(src.contains(QStringLiteral("QStandardPaths::standardLocations")));
    QVERIFY(src.contains(QStringLiteral("lastIndexOf(QLatin1Char('-'))")));
    QVERIFY(!src.contains(QStringLiteral("if (pluginIdFromMetaData(applet->pluginMetaData())")));
    //! Both directions must be immediate: stripping on enable and restoring
    //! the original on disable (tracked via m_iconOverrideOriginal).
    QVERIFY(src.contains(QStringLiteral("m_iconOverrideOriginal")));

    QFile appletItemFile(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/applet/AppletItem.qml"));
    QVERIFY(appletItemFile.open(QFile::ReadOnly));
    const QString appletItemSource = QString::fromUtf8(appletItemFile.readAll());
    QVERIFY(appletItemSource.contains(QStringLiteral("userKeepsOriginalIconColors")));

    QFile wrapperFile(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/applet/ItemWrapper.qml"));
    QVERIFY(wrapperFile.open(QFile::ReadOnly));
    const QString wrapperSource = QString::fromUtf8(wrapperFile.readAll());
        QVERIFY(wrapperSource.contains(QStringLiteral("refreshIconPath")));
        QVERIFY(wrapperSource.contains(QStringLiteral("interval: 100")));
        QVERIFY(appletItemSource.contains(QStringLiteral("currentBackendAppletIconPath")));
        QVERIFY(wrapperSource.contains(QStringLiteral("hoveredOriginalIconEffect")));
    QVERIFY(hdr.contains(QStringLiteral("applyOriginalIconColors")));

    QFile layoutManagerHeader(QStringLiteral(LATTE_SOURCE_DIR "/containment/plugin/layoutmanager.h"));
    QVERIFY(layoutManagerHeader.open(QFile::ReadOnly));
    QVERIFY(QString::fromUtf8(layoutManagerHeader.readAll()).contains(QStringLiteral("Q_INVOKABLE int appletId(QObject *applet) const")));
    QFile layoutManagerSource(QStringLiteral(LATTE_SOURCE_DIR "/containment/plugin/layoutmanager.cpp"));
    QVERIFY(layoutManagerSource.open(QFile::ReadOnly));
    const QString layoutManagerSourceText = QString::fromUtf8(layoutManagerSource.readAll());
    QVERIFY(layoutManagerSourceText.contains(QStringLiteral("QString LayoutManager::appletIcon(QObject *applet) const")));
    QVERIFY(layoutManagerSourceText.contains(QStringLiteral("KIconLoader::global()->iconPath")));
}

void SourceContractTest::widgetOriginalIconColorsFallbackKeepsHoverAndStateSynchronized()
{
    QFile appletItemFile(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/applet/AppletItem.qml"));
    QVERIFY(appletItemFile.open(QFile::ReadOnly));
    const QString appletItemSource = QString::fromUtf8(appletItemFile.readAll());

    QFile wrapperFile(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/applet/ItemWrapper.qml"));
    QVERIFY(wrapperFile.open(QFile::ReadOnly));
    const QString wrapperSource = QString::fromUtf8(wrapperFile.readAll());

    // Widgets whose compact representation has no discoverable IconItem use
    // an Image fallback. The fallback must call the backend directly so
    // dynamic states such as volume mute/unmute are not frozen in QML's
    // getter-backed property cache.
    QVERIFY(appletItemSource.contains(QStringLiteral("function currentBackendAppletIconPath()")));
    QVERIFY(wrapperSource.contains(QStringLiteral("appletItem.currentBackendAppletIconPath()")));
    QVERIFY(wrapperSource.contains(QStringLiteral("interval: 100")));

    // The fallback is above the native hover copy and therefore needs its own
    // MultiEffect. It must use the same mouse state, brightness and fixed-slot
    // zoom transform as the generic widget hover path.
    QVERIFY(wrapperSource.contains(QStringLiteral("id: hoveredOriginalIconEffect")));
    QVERIFY(wrapperSource.contains(QStringLiteral("source: originalIconFallback")));
    QVERIFY(wrapperSource.contains(QStringLiteral("z: 1200")));
    QVERIFY(wrapperSource.contains(QStringLiteral("scale: originalIconFallback.scale")));
    QVERIFY(wrapperSource.contains(QStringLiteral("transformOrigin: originalIconFallback.transformOrigin")));
    QVERIFY(wrapperSource.contains(QStringLiteral("opacity: visible && appletItem.containsMouse ? 1 : 0")));
    QVERIFY(wrapperSource.contains(QStringLiteral("brightness: 0.30")));
    QVERIFY(wrapperSource.contains(QStringLiteral("contrast: 0.1")));
}

void SourceContractTest::appletContextMenuExposesKeepOriginalColorsToggle()
{
    QFile implFile(QStringLiteral(LATTE_SOURCE_DIR "/app/declarativeimports/contextmenulayerquickitem.cpp"));
    QVERIFY(implFile.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(implFile.readAll());

    // The per-applet "keep original icon colors" toggle must live in the
    // applet context menu and drive LayoutManager::setOption with the
    // "userBlocksColorizing" option, so the QML userBlocksColorizing gate in
    // AppletItem.qml stops colorizing that applet (issue #44). It must be
    // offered for every applet rather than only the Trash widget.
    QVERIFY(src.contains(QStringLiteral("Keep Original Icon Colors")));
    QVERIFY(src.contains(QStringLiteral("userBlocksColorizing")));
    QVERIFY(src.contains(QStringLiteral("setOption")));
    QVERIFY(!src.contains(QStringLiteral("applet->pluginMetaData().pluginId() == QLatin1String(\"org.kde.plasma.trash\")")));

    // The toggle must be inside addAppletActions (per-applet menu), not the
    // containment menu, and its checked state must reflect the layout
    // manager's current list.
    const int aaa = src.indexOf(QStringLiteral("ContextMenuLayerQuickItem::addAppletActions"));
    const int toggle = src.indexOf(QStringLiteral("Keep Original Icon Colors"));
    QVERIFY(aaa >= 0);
    QVERIFY(toggle > aaa);
    QVERIFY(src.contains(QStringLiteral("setCheckable(true)")));
    QVERIFY(src.contains(QStringLiteral("disabledColoring.contains(appletId)")));
}

void SourceContractTest::trashKeepOriginalColorsDefaultsToCheckedForAllConfigs()
{
    // The Trash widget must keep its original full-color icon by default in
    // every dock configuration (issue #44): a freshly added trash applet is
    // automatically added to userBlocksColorizingApplets, and older layouts
    // get a one-time migration that marks every present trash widget.
    QFile layoutManagerSourceFile(QStringLiteral(LATTE_SOURCE_DIR "/containment/plugin/layoutmanager.cpp"));
    QVERIFY(layoutManagerSourceFile.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(layoutManagerSourceFile.readAll());

    // The default must be scoped to the trash plugin only.
    QVERIFY(src.contains(QStringLiteral("org.kde.plasma.trash")));

    // A freshly added trash applet is defaulted to keep original colors.
    const int addAppletItem = src.indexOf(QStringLiteral("void LayoutManager::addAppletItem(QObject *applet, int index)"));
    QVERIFY(addAppletItem >= 0);
    const int applyDefaultCall = src.indexOf(QStringLiteral("applyTrashKeepOriginalColorsDefault(applet, id)"), addAppletItem);
    QVERIFY(applyDefaultCall > addAppletItem);

    // The default must only apply to applets not already in the saved order
    // (so an explicit user choice on a restored applet is never overwritten)
    // and must not duplicate an existing entry.
    const int applyDefaultImpl = src.indexOf(QStringLiteral("void LayoutManager::applyTrashKeepOriginalColorsDefault"));
    QVERIFY(applyDefaultImpl >= 0);
    QVERIFY(src.indexOf(QStringLiteral("m_appletOrder.contains(id)"), applyDefaultImpl) > applyDefaultImpl);
    QVERIFY(src.indexOf(QStringLiteral("m_userBlocksColorizingApplets.contains(id)"), applyDefaultImpl) > applyDefaultImpl);
    QVERIFY(src.indexOf(QStringLiteral("isTrashWidget(applet)"), applyDefaultImpl) > applyDefaultImpl);

    // Older layouts get a one-time migration that marks every present trash
    // widget and flags itself as applied.
    const int cleanupOptions = src.indexOf(QStringLiteral("void LayoutManager::cleanupOptions()"));
    QVERIFY(cleanupOptions >= 0);
    QVERIFY(src.indexOf(QStringLiteral("applyTrashKeepOriginalColorsMigration()"), cleanupOptions) > cleanupOptions);

    const int migrationImpl = src.indexOf(QStringLiteral("void LayoutManager::applyTrashKeepOriginalColorsMigration"));
    QVERIFY(migrationImpl >= 0);
    QVERIFY(src.indexOf(QStringLiteral("trashKeepOriginalColorsDefaulted"), migrationImpl) > migrationImpl);
    QVERIFY(src.indexOf(QStringLiteral("setUserBlocksColorizingApplets(applets)"), migrationImpl) > migrationImpl);

    // The trash detection helper must resolve both a Plasma::Applet and an
    // AppletQuickItem to the trash plugin id.
    const int isTrashImpl = src.indexOf(QStringLiteral("bool LayoutManager::isTrashWidget"));
    QVERIFY(isTrashImpl >= 0);
    QVERIFY(src.indexOf(QStringLiteral("Plasma::Applet"), isTrashImpl) > isTrashImpl);
    QVERIFY(src.indexOf(QStringLiteral("PlasmaQuick::AppletQuickItem"), isTrashImpl) > isTrashImpl);

    // The helpers must be declared in the header.
    QFile headerFile(QStringLiteral(LATTE_SOURCE_DIR "/containment/plugin/layoutmanager.h"));
    QVERIFY(headerFile.open(QFile::ReadOnly));
    const QString hdr = QString::fromUtf8(headerFile.readAll());
    QVERIFY(hdr.contains(QStringLiteral("isTrashWidget(QObject *applet) const")));
    QVERIFY(hdr.contains(QStringLiteral("applyTrashKeepOriginalColorsDefault")));
    QVERIFY(hdr.contains(QStringLiteral("applyTrashKeepOriginalColorsMigration")));
}

void SourceContractTest::settingsDialogUiLabelsAreTranslatable()
{
    // User-facing labels in the Qt Designer .ui dialogs must not be excluded
    // from translation via notr="true". The parabolic "Thickness margin
    // influence" label and the Views dialog buttons were fixed after the
    // Dock/Panel removal, while pure percentages and placeholder paths stay
    // non-translatable on purpose.
    QFile settingsUi(QStringLiteral(LATTE_SOURCE_DIR "/app/settings/settingsdialog/settingsdialog.ui"));
    QVERIFY(settingsUi.open(QFile::ReadOnly));
    const QString settingsSource = QString::fromUtf8(settingsUi.readAll());
    QVERIFY(settingsSource.contains(QStringLiteral("<string>Thickness margin influence</string>")));
    QVERIFY(!settingsSource.contains(QStringLiteral("notr=\"true\">Thickness margin influence")));
    // percentages stay notr
    QVERIFY(settingsSource.contains(QStringLiteral("<string notr=\"true\">0%</string>")));
    QVERIFY(settingsSource.contains(QStringLiteral("<string notr=\"true\">100%</string>")));

    QFile viewsUi(QStringLiteral(LATTE_SOURCE_DIR "/app/settings/viewsdialog/viewsdialog.ui"));
    QVERIFY(viewsUi.open(QFile::ReadOnly));
    const QString viewsSource = QString::fromUtf8(viewsUi.readAll());
    QVERIFY(viewsSource.contains(QStringLiteral("<string>New</string>")));
    QVERIFY(viewsSource.contains(QStringLiteral("<string>Remove</string>")));
    QVERIFY(viewsSource.contains(QStringLiteral("<string>Import...</string>")));
    QVERIFY(viewsSource.contains(QStringLiteral("<string>Export...</string>")));
    QVERIFY(!viewsSource.contains(QStringLiteral("notr=\"true\">New")));
    QVERIFY(!viewsSource.contains(QStringLiteral("notr=\"true\">Remove")));

    QFile exportUi(QStringLiteral(LATTE_SOURCE_DIR "/app/settings/exporttemplatedialog/exporttemplatedialog.ui"));
    QVERIFY(exportUi.open(QFile::ReadOnly));
    const QString exportSource = QString::fromUtf8(exportUi.readAll());
    QVERIFY(exportSource.contains(QStringLiteral("<string>Export Template</string>")));
    QVERIFY(!exportSource.contains(QStringLiteral("notr=\"true\">Export Template")));
    QVERIFY(exportSource.contains(QStringLiteral("<string notr=\"true\">~/.config/latte/layouts/.....</string>")));
}

void SourceContractTest::poFilesHaveNoStaleDockPanelMsgids()
{
    // The Dock/Panel -> Dock migration changed the source strings but the po
    // files were not regenerated, so the stale msgids silently fell back to
    // English. No active msgid in any domain may still reference a panel.
    QDir poDir(QStringLiteral(LATTE_SOURCE_DIR "/po"));
    const QStringList languages = poDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    QVERIFY(languages.size() > 10);

    for (const QString &lang : languages) {
        const QDir langDir(poDir.filePath(lang));
        const QStringList poFiles = langDir.entryList(QStringList() << QStringLiteral("*.po"), QDir::Files);
        for (const QString &poName : poFiles) {
            QFile poFile(langDir.filePath(poName));
            QVERIFY(poFile.open(QFile::ReadOnly));
            const QStringList lines = QString::fromUtf8(poFile.readAll()).split(QLatin1Char('\n'));
            for (const QString &line : lines) {
                if (line.startsWith(QLatin1String("msgid "))
                    && !line.startsWith(QLatin1String("#~ msgid"))
                    && (line.contains(QLatin1String("Dock/Panel"))
                        || line.contains(QLatin1String(" or panel"))
                        || line.contains(QLatin1String("Docks/Panels"))
                        || line.contains(QLatin1String("dock, panels"))
                        || line.contains(QLatin1String("Dock panel")))) {
                    qCritical() << "stale msgid" << line << "in" << poFile.fileName();
                    QVERIFY2(false, "stale Dock/Panel msgid remains in po file");
                }
            }
        }
    }

    // The newly exposed strings must be present in at least the zh_CN file.
    QFile zhPo(poDir.filePath(QStringLiteral("zh_CN/latte-dock.po")));
    QVERIFY(zhPo.open(QFile::ReadOnly));
    const QString zhSource = QString::fromUtf8(zhPo.readAll());
    QVERIFY(zhSource.contains(QStringLiteral("msgid \"Thickness margin influence\"")));
    QVERIFY(zhSource.contains(QStringLiteral("msgstr \"厚度边距影响\"")));
    QVERIFY(zhSource.contains(QStringLiteral("msgid \"Export Template\"")));
    QVERIFY(zhSource.contains(QStringLiteral("msgstr \"导出模板\"")));
    QVERIFY(zhSource.contains(QStringLiteral("msgid \"Keep Original Icon Colors\"")));
    QVERIFY(zhSource.contains(QStringLiteral("msgstr \"保留原始图标颜色\"")));
}

void SourceContractTest::translationExtractionReferencesValidDirectories()
{
    // The indicators Messages.sh used to reference the removed
    // org.kde.latte.plasma indicator directory, which silently skipped its
    // extraction. Every `find` path in a Messages.sh must exist.
    const QStringList messagesScripts = {
        QStringLiteral("Messages.sh"),
        QStringLiteral("containment/Messages.sh"),
        QStringLiteral("containmentactions/contextmenu/Messages.sh"),
        QStringLiteral("indicators/Messages.sh"),
        QStringLiteral("plasmoid/Messages.sh")
    };

    for (const QString &rel : messagesScripts) {
        QFile script(QStringLiteral(LATTE_SOURCE_DIR "/") + rel);
        const QString openMsg = QStringLiteral("must exist: ") + rel;
        QVERIFY2(script.open(QFile::ReadOnly), qPrintable(openMsg));
        const QString content = QString::fromUtf8(script.readAll());

        // No stale reference to the removed plasma indicator. The existing
        // plasmatabstyle directory legitimately shares the prefix, so match
        // the exact `find` path, not the substring.
        const QString staleMsg = QStringLiteral("must not reference the removed org.kde.latte.plasma indicator: ") + rel;
        QVERIFY2(!content.contains(QStringLiteral("find org.kde.latte.plasma ")),
                 qPrintable(staleMsg));
    }

    // The `find` directories referenced by indicators/Messages.sh must exist.
    QVERIFY(QDir(QStringLiteral(LATTE_SOURCE_DIR "/indicators/default")).exists());
    QVERIFY(QDir(QStringLiteral(LATTE_SOURCE_DIR "/indicators/org.kde.latte.plasmatabstyle")).exists());
}

void SourceContractTest::mouseHandlerAutoPinOnDragPromotesNonLauncherTasks()
{
    QFile mouseHandler(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/taskslayout/MouseHandler.qml"));
    QVERIFY(mouseHandler.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(mouseHandler.readAll());

    // schedulePromoteToLauncherAndMove must exist — it is the auto-pin
    // promotion path for non-launcher tasks dragged between pinned
    // launchers (commit 4386dfd9d).
    QVERIFY(source.contains(QStringLiteral("function schedulePromoteToLauncherAndMove")));

    // The function must actually be called from drop handling.
    QVERIFY(source.contains(QStringLiteral("schedulePromoteToLauncherAndMove(sourceItem, insertAt, launcherUrl)")));

    // The call site must be inside the drop handler, not orphaned.
    const int fnDef = source.indexOf(QStringLiteral("function schedulePromoteToLauncherAndMove"));
    const int callSite = source.indexOf(QStringLiteral("schedulePromoteToLauncherAndMove(sourceItem, insertAt, launcherUrl)"));
    QVERIFY(callSite > fnDef);
}

void SourceContractTest::scrollToggleMinimizedDownwardUnmaximizesBeforeMinimizing()
{
    QFile mainQml(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/main.qml"));
    QVERIFY(mainQml.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(mainQml.readAll());

    // The downward scroll handler (scrollDown case) for
    // ScrollToggleMinimized must check the window state and choose
    // between un-maximize and minimize, NOT just cycle tasks
    // (commit a72c7e582).
    QVERIFY(source.contains(QStringLiteral("ScrollToggleMinimized")));

    // The second ScrollToggleMinimized occurrence is the downward handler.
    const int first = source.indexOf(QStringLiteral("ScrollToggleMinimized"));
    const int second = source.indexOf(QStringLiteral("ScrollToggleMinimized"), first + QStringLiteral("ScrollToggleMinimized").length());
    QVERIFY(first >= 0);
    QVERIFY(second > first);

    // The downward handler must attempt requestToggleMaximized before
    // requestToggleMinimized, matching the intended behaviour from
    // EnvironmentActions.qml.
    const int downBlock = source.indexOf(QStringLiteral("requestToggleMaximized"), second);
    const int downMinimize = source.indexOf(QStringLiteral("requestToggleMinimized"), second);
    QVERIFY(downBlock > second);
    QVERIFY(downMinimize > downBlock);
}

void SourceContractTest::scrollToggleMinimizedUsesAllScreensTrackerForMinimizeAndMaximize()
{
    // The ScrollToggleMinimized handlers in both main.qml and
    // EnvironmentActions.qml must target the allScreens (global)
    // lastActiveWindow tracker instead of selectedWindowsTracker
    // (per-screen).  Otherwise docks on secondary screens can never
    // minimize/maximize windows because their per-screen tracker
    // has never been populated (commit b4bd3a3e6 → f4ab344ee).

    auto checkFileUsesAllScreens = [](const QString & filePath) {
        QFile f(filePath);
        QVERIFY2(f.open(QFile::ReadOnly), qPrintable(filePath));
        const QString src = QString::fromUtf8(f.readAll());

        QVERIFY(src.contains(QStringLiteral("ScrollToggleMinimized")));

        // Both ScrollToggleMinimized branches (upward Ctrl and
        // downward) should access allScreens.lastActiveWindow for
        // the minimize / maximize-toggle actions.
        const int first = src.indexOf(QStringLiteral("ScrollToggleMinimized"));
        const int second = src.indexOf(QStringLiteral("ScrollToggleMinimized"),
                                       first + QStringLiteral("ScrollToggleMinimized").length());

        // At least one allScreens.lastActiveWindow access must appear
        // within each ScrollToggleMinimized block.
        const int upAllScreens = src.indexOf(QStringLiteral("allScreens.lastActiveWindow"), first);
        const int downAllScreens = src.indexOf(QStringLiteral("allScreens.lastActiveWindow"), second);

        QVERIFY(upAllScreens > first);
        QVERIFY(upAllScreens < (second > 0 ? second : src.length()));
        QVERIFY(downAllScreens > second);
    };

    checkFileUsesAllScreens(
        QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/main.qml"));
    checkFileUsesAllScreens(
        QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/layouts/EnvironmentActions.qml"));
}

void SourceContractTest::systrayGuardsInAppletItemPreventLayoutAndInteractionBreakage()
{
    QFile appletItem(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/applet/AppletItem.qml"));
    QVERIFY(appletItem.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(appletItem.readAll());

    // The isSystray property must detect both KDE and Nomad system trays.
    QVERIFY(src.contains(QStringLiteral("isSystray: pluginName === \"org.kde.plasma.systemtray\" || pluginName === \"org.nomad.systemtray\"")));

    // z-order: systray must NOT be promoted above tasks like other external
    // applets; the z=1000 path excludes systray.
    QVERIFY(src.contains(QStringLiteral("externalAppletDrawsAboveTasks && !isSystray")));

    // Colorizing: systray must block latte-side colorization to preserve
    // the native appearance of tray icons.
    QVERIFY(src.contains(QStringLiteral("appletBlocksColorizing")));
    const int colorizingLine = src.indexOf(QStringLiteral("appletBlocksColorizing"));
    QVERIFY(src.indexOf(QStringLiteral("isSystray"), colorizingLine) > colorizingLine);

    // Fixed slot sizing: systray excluded so it follows its own sizing.
    QVERIFY(src.contains(QStringLiteral("externalAppletUsesFixedSlotSizing")));
    const int fixedSlotLine = src.indexOf(QStringLiteral("externalAppletUsesFixedSlotSizing"));
    QVERIFY(src.indexOf(QStringLiteral("!isSystray"), fixedSlotLine) > fixedSlotLine);

    // Parabolic effect: systray disables parabolic zoom.
    QVERIFY(src.contains(QStringLiteral("isSystray\n                    || appletItem.isAutoFillApplet")));

    // Wheel events: systray must receive wheel events for volume/brightness
    // controls; the onWheelScrolled handler returns early for systray.
    QVERIFY(src.contains(QStringLiteral("if (appletItem.isSystray) {\n                // System tray applets")));
}

void SourceContractTest::volumeAndAppMenuPopupSizingUsesLargerMinimumInCompactApplet()
{
    QFile appletFile(QStringLiteral(LATTE_SOURCE_DIR "/shell/package/contents/applet/CompactApplet.qml"));
    QVERIFY(appletFile.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(appletFile.readAll());

    // Volume applet detection must exist.
    QVERIFY(src.contains(QStringLiteral("function isVolumeApplet()")));
    QVERIFY(src.contains(QStringLiteral("\"org.kde.plasma.volume\"")));

    // Volume popup minimum width ≥ gridUnit * 27 so controls fit.
    const int volW = src.indexOf(QStringLiteral("isVolumeApplet()"));
    const int volMinW = src.indexOf(QStringLiteral("Kirigami.Units.gridUnit * 27"), volW);
    QVERIFY(volMinW > volW);

    // Volume popup minimum height similarly sized.
    const int volH = src.indexOf(QStringLiteral("isVolumeApplet()"), volMinW);
    const int volMinH = src.indexOf(QStringLiteral("Kirigami.Units.gridUnit * 27"), volH);
    QVERIFY(volMinH > volH);

    // Application menu detection must exist.
    QVERIFY(src.contains(QStringLiteral("function isApplicationMenuApplet()")));
    QVERIFY(src.contains(QStringLiteral("\"org.kde.plasma.kicker\"")));
    QVERIFY(src.contains(QStringLiteral("\"org.kde.plasma.kickoff\"")));
    QVERIFY(src.contains(QStringLiteral("\"org.kde.plasma.kickerdash\"")));
    QVERIFY(src.contains(QStringLiteral("\"org.kde.plasma.appmenu\"")));

    // Application menu popup maximum width/height must be Infinity so the
    // menu can fill the screen.
    QVERIFY(src.contains(QStringLiteral("return Infinity;")));
}

void SourceContractTest::clipboardAndDigitalClockErrorSuppressionInMainCpp()
{
    QFile mainCpp(QStringLiteral(LATTE_SOURCE_DIR "/app/main.cpp"));
    QVERIFY(mainCpp.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(mainCpp.readAll());

    // Digital clock tooltip TypeError from Plasma framework is harmless
    // and must be suppressed to keep the log clean.
    QVERIFY(src.contains(QStringLiteral("digitalclock/Tooltip.qml:40: TypeError")));

    // Clipboard applet QML type mismatch with Plasma 6 framework must be
    // suppressed — the clipboard still functions despite the warning.
    QVERIFY(src.contains(QStringLiteral("org.kde.plasma.clipboard")));
    QVERIFY(src.contains(QStringLiteral("error when loading")));

    // Both suppressions must appear inside the message filter block.
    const int digitalClockSuppress = src.indexOf(QStringLiteral("digitalclock/Tooltip.qml:40: TypeError"));
    const int clipboardSuppress = src.indexOf(QStringLiteral("org.kde.plasma.clipboard"));
    const int filterReturn = src.indexOf(QStringLiteral("//! block warnings from dependencies"), digitalClockSuppress);
    QVERIFY(digitalClockSuppress > 0);
    QVERIFY(clipboardSuppress > 0);
    QVERIFY(filterReturn > digitalClockSuppress);
    QVERIFY(filterReturn > clipboardSuppress);
}

void SourceContractTest::systemTrayAndPlasmoidActAsAppletInsertionBoundary()
{
    QFile containmentInterfaceCpp(QStringLiteral(LATTE_SOURCE_DIR "/app/view/containmentinterface.cpp"));
    QVERIFY(containmentInterfaceCpp.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(containmentInterfaceCpp.readAll());

    // System tray (both KDE and Nomad) and latte plasmoid act as insertion
    // boundaries: newly created applets are inserted before them.
    QVERIFY(src.contains(QStringLiteral("kSystemTray")));
    QVERIFY(src.contains(QStringLiteral("\"org.nomad.systemtray\"")));
    QVERIFY(src.contains(QStringLiteral("kPlasmoid")));

    // All three must be in the same boundary-check block.
    const int blockStart = src.indexOf(QStringLiteral("kSystemTray"));
    const int nomadInBlock = src.indexOf(QStringLiteral("\"org.nomad.systemtray\""), blockStart);
    const int plasmoidInBlock = src.indexOf(QStringLiteral("kPlasmoid"), blockStart);
    QVERIFY(nomadInBlock > blockStart);
    QVERIFY(plasmoidInBlock > nomadInBlock);

    // The boundary check must be followed by boundaryIds.insert.
    const int insertCall = src.indexOf(QStringLiteral("boundaryIds.insert"), plasmoidInBlock);
    QVERIFY(insertCall > plasmoidInBlock);
}

void SourceContractTest::separatorAndSpacerDetectionAndBehaviorInAppletItem()
{
    QFile appletItem(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/applet/AppletItem.qml"));
    QVERIFY(appletItem.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(appletItem.readAll());

    // isSeparator must recognize both the legacy separator and the latte
    // separator so that layouts from older Latte versions continue to work.
    QVERIFY(src.contains(QStringLiteral("isSeparator: pluginName === \"audoban.applet.separator\"\n                               || pluginName === \"org.kde.latte.separator\"")));

    // isSpacer must recognize the latte spacer.
    QVERIFY(src.contains(QStringLiteral("isSpacer: pluginName === \"org.kde.latte.spacer\"")));

    // isMarginsAreaSeparator must use the Plasma constraint hint; it
    // identifies separators that live in the margin areas at dock edges.
    QVERIFY(src.contains(QStringLiteral("isMarginsAreaSeparator: applet")));
    QVERIFY(src.contains(QStringLiteral("MarginAreasSeparator")));

    // externalAppletDrawsAboveTasks must exclude separators and spacers
    // so they don't get elevated z-order or text-applet sizing.
    const int extDrawsAbove = src.indexOf(QStringLiteral("externalAppletDrawsAboveTasks: isExternalPlasmaApplet"));
    QVERIFY(extDrawsAbove >= 0);
    const int exclSeparatorInExt = src.indexOf(QStringLiteral("!isSeparator"), extDrawsAbove);
    const int exclSpacerInExt = src.indexOf(QStringLiteral("!isSpacer"), exclSeparatorInExt);
    QVERIFY(exclSeparatorInExt > extDrawsAbove);
    QVERIFY(exclSpacerInExt > exclSeparatorInExt);

    // Applet number badge must not render on separators, margins area
    // separators, or spacers — they are not user-interactive applets.
    const int badgeLine = src.indexOf(QStringLiteral("canShowAppletNumberBadge"));
    QVERIFY(badgeLine >= 0);
    QVERIFY(src.indexOf(QStringLiteral("!isSeparator"), badgeLine) > badgeLine);
    QVERIFY(src.indexOf(QStringLiteral("!isMarginsAreaSeparator"), badgeLine) > badgeLine);
    QVERIFY(src.indexOf(QStringLiteral("!isSpacer"), badgeLine) > badgeLine);

    // latteStyleApplet must resolve the first child for separator and
    // spacer so that custom indicators operate on the visual item rather
    // than the bare applet wrapper.
    QVERIFY(src.contains(QStringLiteral("latteStyleApplet: applet && ((pluginName === \"org.kde.latte.spacer\") || (pluginName === \"org.kde.latte.separator\"))")));
}

void SourceContractTest::separatorGuardsAcrossLayoutAndDragDropFiles()
{
    // Separators during parabolic effect must return length = -1 so the
    // layout engine can skip them during zoom calculations.
    {
        QFile itemWrapper(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/applet/ItemWrapper.qml"));
        QVERIFY(itemWrapper.open(QFile::ReadOnly));
        const QString src = QString::fromUtf8(itemWrapper.readAll());
        QVERIFY(src.contains(QStringLiteral("(isSeparator && appletItem.parabolic.isEnabled)\n                || (isMarginsAreaSeparator && appletItem.parabolic.isEnabled)")));
        QVERIFY(src.contains(QStringLiteral("return -1;")));

        // localLengthMargins must be 0 for separators/margins separators
        // so they don't add extra spacing.
        QVERIFY(src.contains(QStringLiteral("localLengthMargins: isSeparator\n                                     || appletItem.isMarginsAreaSeparator")));
    }

    // Indicator level must not draw on separators or margins area
    // separators — they are visual dividers, not hoverable items.
    {
        QFile indicatorLevel(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/applet/IndicatorLevel.qml"));
        QVERIFY(indicatorLevel.open(QFile::ReadOnly));
        const QString src = QString::fromUtf8(indicatorLevel.readAll());
        QVERIFY(src.contains(QStringLiteral("isDrawn: !appletItem.isSeparator\n                   && !appletItem.isMarginsAreaSeparator")));
    }

    // Drag-and-drop must detect separators from both the legacy and latte
    // plugin IDs so that dragged applets can correctly detect gaps.
    {
        QFile dragDropArea(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/DragDropArea.qml"));
        QVERIFY(dragDropArea.open(QFile::ReadOnly));
        const QString src = QString::fromUtf8(dragDropArea.readAll());
        QVERIFY(src.contains(QStringLiteral("\"audoban.applet.separator\"")));
        QVERIFY(src.contains(QStringLiteral("\"org.kde.latte.separator\"")));
    }

    // Parabolic area signal acceptance must skip separators and margins
    // area separators so they don't consume or block parabolic zoom.
    {
        QFile parabolicArea(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/applet/ParabolicArea.qml"));
        QVERIFY(parabolicArea.open(QFile::ReadOnly));
        const QString src = QString::fromUtf8(parabolicArea.readAll());
        QVERIFY(src.contains(QStringLiteral("!appletItem.isSeparator && !appletItem.isMarginsAreaSeparator")));
    }

    // Hidden spacer must return 0 length for separators.
    {
        QFile hiddenSpacer(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/applet/HiddenSpacer.qml"));
        QVERIFY(hiddenSpacer.open(QFile::ReadOnly));
        const QString src = QString::fromUtf8(hiddenSpacer.readAll());
        QVERIFY(src.contains(QStringLiteral("if (isSeparator || !communicator.requires.lengthMarginsEnabled) {\n            return 0;")));
    }
}

void SourceContractTest::separatorContentContainerStaysTightWithFallback()
{
    // The separator applet never resolves Layout.preferredWidth (it stays -1),
    // so the content container must fall back to a valid separator length.
    // Otherwise a freshly added separator renders its 1px line in a -1px
    // container and becomes invisible, and an icon-sized (56px) container
    // overflows into the neighbouring applets.
    QFile itemWrapper(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/applet/ItemWrapper.qml"));
    QVERIFY(itemWrapper.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(itemWrapper.readAll());

    const int layoutLengthBinding = src.indexOf(QStringLiteral("property: \"layoutLength\""));
    QVERIFY(layoutLengthBinding >= 0);

    // The separator branch must sit inside the layoutLength binding value.
    const int separatorBranch = src.indexOf(QStringLiteral("if (isSeparator) {"), layoutLengthBinding);
    QVERIFY(separatorBranch > layoutLengthBinding);
    QVERIFY(separatorBranch < src.indexOf(QStringLiteral("externalAppletUsesFixedSlotSizing)"), separatorBranch));

    // Use the resolved preferred length when valid ...
    QVERIFY(src.indexOf(QStringLiteral("appletPreferredLength > 0"), separatorBranch) > separatorBranch);
    QVERIFY(src.indexOf(QStringLiteral("return appletPreferredLength;"), separatorBranch) > separatorBranch);

    // ... otherwise fall back to the visual separator length so the line
    // renders immediately after the separator is added.
    const int fallback = src.indexOf(QStringLiteral("return LatteCore.Environment.separatorLength;"), separatorBranch);
    QVERIFY(fallback > separatorBranch);

    // The separator slot itself must stay zero/negative width so it occupies
    // no dock space (the neighbours' hidden spacers reserve the gap).
    QVERIFY(src.contains(QStringLiteral("(isSeparator && appletItem.parabolic.isEnabled)")));
    QVERIFY(src.contains(QStringLiteral("return -1;")));
}

void SourceContractTest::restoreZoomTimerGracePeriodPreventsBoundaryBlip()
{
    // At a separator boundary the top-most neighbour (external widget drawing
    // above the tasks) can briefly own the junction, zoom the adjacent task
    // and then be pushed away by the resulting layout growth.  Its onExited
    // nullifies the current item immediately, so the restore timer must keep
    // a grace period long enough for the task that now covers the cursor to
    // re-enter and cancel the clear — otherwise the icon blips (jitter).
    QFile parabolicPrivate(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/abilities/privates/ParabolicEffectPrivate.qml"));
    QVERIFY(parabolicPrivate.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(parabolicPrivate.readAll());

    const int timerId = src.indexOf(QStringLiteral("id: restoreZoomTimer"));
    QVERIFY(timerId >= 0);

    const int interval = src.indexOf(QStringLiteral("interval: 150"), timerId);
    QVERIFY(interval > timerId);

    // The clear must still be skipped while a current parabolic item exists,
    // so the grace period only delays the actual sglClearZoom() broadcast.
    const int trigger = src.indexOf(QStringLiteral("onTriggered:"), timerId);
    QVERIFY(trigger > interval);
    QVERIFY(src.indexOf(QStringLiteral("currentParabolicItem"), trigger) > trigger);
    QVERIFY(src.indexOf(QStringLiteral("sglClearZoom()"), trigger) > trigger);
}

void SourceContractTest::myViewClientIntPropertiesUseSafeIntGuardAgainstUndefined()
{
    QFile clientFile(QStringLiteral(LATTE_SOURCE_DIR "/declarativeimports/abilities/client/MyView.qml"));
    QVERIFY(clientFile.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(clientFile.readAll());

    // The safeInt helper must exist to guard int property bindings against
    // undefined values from the bridge host during initialization/transition.
    QVERIFY(src.contains(QStringLiteral("function safeInt(source, propertyName, fallback)")));

    // alignment, itemsAlignment, and visibilityMode must all use safeInt
    // so that undefined values from the host (e.g. before plasmoid config
    // is loaded) fall back to valid int defaults instead of emitting
    // "Unable to assign [undefined] to int" warnings.
    QVERIFY(src.contains(QStringLiteral("alignment: safeInt(ref.myView, \"alignment\", 0)")));
    QVERIFY(src.contains(QStringLiteral("itemsAlignment: safeInt(ref.myView, \"itemsAlignment\", 0)")));
    QVERIFY(src.contains(QStringLiteral("visibilityMode: safeInt(ref.myView, \"visibilityMode\", -1)")));

    // The old unguarded binding pattern must not reappear.
    QVERIFY(!src.contains(QStringLiteral("alignment: ref.myView.alignment")));
    QVERIFY(!src.contains(QStringLiteral("itemsAlignment: ref.myView.itemsAlignment")));
    QVERIFY(!src.contains(QStringLiteral("visibilityMode: ref.myView.visibilityMode")));
}

void SourceContractTest::indicatorFactoryExcludesBuiltinPluginsFromCustomLists()
{
    QFile factoryCpp(QStringLiteral(LATTE_SOURCE_DIR "/app/indicator/factory.cpp"));
    QVERIFY(factoryCpp.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(factoryCpp.readAll());

    // Built-in indicators must be excluded from custom-plugin lists so
    // the user-facing "custom indicators" UI only shows user-installed ones.
    QVERIFY(src.contains(QStringLiteral("\"org.kde.latte.default\"")));
    QVERIFY(src.contains(QStringLiteral("\"org.kde.latte.plasma\"")));
    QVERIFY(src.contains(QStringLiteral("\"org.kde.latte.plasmatabstyle\"")));

    // isCustomType must blacklist the same three plugin IDs.
    QVERIFY(src.contains(QStringLiteral("isCustomType")));
}

void SourceContractTest::waylandInterfaceAcceptableWindowHasHardcodedAppIdWhitelist()
{
    QFile waylandCpp(QStringLiteral(LATTE_SOURCE_DIR "/app/wm/waylandinterface.cpp"));
    QVERIFY(waylandCpp.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(waylandCpp.readAll());

    // isAcceptableWindow must whitelist yakuake and krunner by app ID
    // so they remain tracked even when both skipTaskbar and skipSwitcher are set.
    QVERIFY(src.contains(QStringLiteral("yakuake")));
    QVERIFY(src.contains(QStringLiteral("krunner")));

    // Plasmashell windows must be handled specially: sidepanels are
    // whitelisted, panel/fullscreen windows are ignored.
    QVERIFY(src.contains(QStringLiteral("org.kde.plasmashell")));
}

void SourceContractTest::genericLayoutReassertsDefaultContextMenuOnContainmentWiring()
{
    QFile layoutCpp(QStringLiteral(LATTE_SOURCE_DIR "/app/layout/genericlayout.cpp"));
    QVERIFY(layoutCpp.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(layoutCpp.readAll());

    // Every containment wiring must reassert the default right-click
    // context menu action so it is never accidentally lost.
    QVERIFY(src.contains(QStringLiteral("org.kde.latte.contextmenu")));
    QVERIFY(src.contains(QStringLiteral("RightButton;NoModifier")));
}

void SourceContractTest::layoutManagerCleanupOnStartupRemovesLegacyAndGhostApplets()
{
    QFile managerCpp(QStringLiteral(LATTE_SOURCE_DIR "/app/layouts/manager.cpp"));
    QVERIFY(managerCpp.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(managerCpp.readAll());

    // Startup cleanup must remove the legacy org.kde.contextmenu action group.
    QVERIFY(src.contains(QStringLiteral("org.kde.contextmenu")));

    // Startup cleanup must remove ghost desktop containments.
    QVERIFY(src.contains(QStringLiteral("kDesktopContainment")));

    // Legacy "Default Dock" migration must detect analog clock applets
    // by their clashing plugin IDs and remove them.
    QVERIFY(src.contains(QStringLiteral("kAnalogClock")));
}

void SourceContractTest::containmentInterfaceHasSeparatorPluginConstantsAndHelper()
{
    QFile interfaceCpp(QStringLiteral(LATTE_SOURCE_DIR "/app/view/containmentinterface.cpp"));
    QVERIFY(interfaceCpp.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(interfaceCpp.readAll());

    // The anonymous-namespace separator constants must recognise both
    // the latte separator and the legacy audoban separator.
    QVERIFY(src.contains(QStringLiteral("\"org.kde.latte.separator\"")));
    QVERIFY(src.contains(QStringLiteral("\"audoban.applet.separator\"")));

    // The internal view splitter plugin ID must be defined.
    QVERIFY(src.contains(QStringLiteral("\"org.kde.latte.splitter\"")));
}

void SourceContractTest::appletsModelHasNoPersonalDataAppletList()
{
    QFile modelCpp(QStringLiteral(LATTE_SOURCE_DIR "/app/settings/exporttemplatedialog/appletsmodel.cpp"));
    QVERIFY(modelCpp.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(modelCpp.readAll());

    // The export-template dialog must pre-select applets that contain
    // no personal data so users can share layouts safely.
    QVERIFY(src.contains(QStringLiteral("\"org.kde.latte.separator\"")));
    QVERIFY(src.contains(QStringLiteral("\"org.kde.latte.spacer\"")));
    QVERIFY(src.contains(QStringLiteral("\"org.kde.latte.plasmoid\"")));
}

void SourceContractTest::lattePackageShellPluginNameBranchingIsCorrect()
{
    QFile packageCpp(QStringLiteral(LATTE_SOURCE_DIR "/app/package/lattepackage.cpp"));
    QVERIFY(packageCpp.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(packageCpp.readAll());

    // The shell plugin name must gate fallback package assignment so
    // the latte shell package inherits from the correct fallback.
    QVERIFY(src.contains(QStringLiteral("\"org.kde.latte.shell\"")));
}

void SourceContractTest::indicatorPlasmaTypeIsRemappedToDefault()
{
    QFile indicatorCpp(QStringLiteral(LATTE_SOURCE_DIR "/app/view/indicator/indicator.cpp"));
    QVERIFY(indicatorCpp.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(indicatorCpp.readAll());

    // The "org.kde.latte.plasma" indicator type must be transparently
    // remapped to "org.kde.latte.default" for backwards compatibility.
    QVERIFY(src.contains(QStringLiteral("\"org.kde.latte.default\"")));
    QVERIFY(src.contains(QStringLiteral("kPlasmaIndicator")));
}

void SourceContractTest::dynamicWindowDotsAreOptInAndAggregateOverflow()
{
    QFile configXml(QStringLiteral(LATTE_SOURCE_DIR "/indicators/default/package/config/main.xml"));
    QVERIFY(configXml.open(QFile::ReadOnly));
    const QString config = QString::fromUtf8(configXml.readAll());
    const int dynamicEntry = config.indexOf(QStringLiteral("<entry name=\"dynamicWindowDots\" type=\"Bool\">"));
    QVERIFY(dynamicEntry >= 0);
    QVERIFY(config.indexOf(QStringLiteral("<default>false</default>"), dynamicEntry) > dynamicEntry);

    QFile indicator(QStringLiteral(LATTE_SOURCE_DIR "/indicators/default/package/ui/main.qml"));
    QVERIFY(indicator.open(QFile::ReadOnly));
    const QString indicatorSrc = QString::fromUtf8(indicator.readAll());

    // The legacy two-dot behavior must remain active unless the option is
    // explicitly enabled, while the dynamic mode is capped at five dots.
    QVERIFY(indicatorSrc.contains(QStringLiteral("dynamicWindowDotsEnabled: dynamicWindowDots")));
    QVERIFY(indicatorSrc.contains(QStringLiteral("effectiveActiveStyle: modernDockStyle ? 1 /*Dot*/ : activeStyle")));
    QVERIFY(indicatorSrc.contains(QStringLiteral("value === \"Dot\" || value === \"Dots\" ? 1 : 0")));
    QVERIFY(indicatorSrc.contains(QStringLiteral("Math.min(5, indicator.windowsCount")));
    QVERIFY(indicatorSrc.contains(QStringLiteral("indicator.activeWindowIndex >= 4")));
    QVERIFY(indicatorSrc.contains(QStringLiteral("for (var i = 4; i < minimizedList.length; ++i)")));
    QVERIFY(indicatorSrc.contains(QStringLiteral("visible: !root.dynamicWindowDotsEnabled")));
    QVERIFY(indicatorSrc.contains(QStringLiteral("lineDotReserve: extraDotsCount * (size + dotSpacing) + glowMargins")));
    QVERIFY(indicatorSrc.contains(QStringLiteral("return root.width - root.lineDotReserve")));
    QVERIFY(indicatorSrc.contains(QStringLiteral("return root.height - root.lineDotReserve")));

    QFile subWindows(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/task/SubWindows.qml"));
    QVERIFY(subWindows.open(QFile::ReadOnly));
    const QString subWindowsSrc = QString::fromUtf8(subWindows.readAll());
    QVERIFY(subWindowsSrc.contains(QStringLiteral("property int activeWindowIndex: -1")));
    QVERIFY(subWindowsSrc.contains(QStringLiteral("property var windowsMinimizedList: []")));
    QVERIFY(subWindowsSrc.contains(QStringLiteral("if (childs.get(i).model.IsWindow === true)")));
    QVERIFY(subWindowsSrc.contains(QStringLiteral("target: windowsRepeater")));
    QVERIFY(subWindowsSrc.contains(QStringLiteral("function onCountChanged() { windowsContainer.updateStates(); }")));

    QFile indicatorHeader(QStringLiteral(LATTE_SOURCE_DIR "/app/view/indicator/indicator.h"));
    QVERIFY(indicatorHeader.open(QFile::ReadOnly));
    const QString indicatorHeaderSrc = QString::fromUtf8(indicatorHeader.readAll());
    QVERIFY(indicatorHeaderSrc.contains(QStringLiteral("Q_PROPERTY(bool isModernDockStyle READ isModernDockStyle CONSTANT)")));
}

void SourceContractTest::appearanceConfigPreservesIndicatorStyleConfiguration()
{
    QFile appearance(QStringLiteral(LATTE_SOURCE_DIR "/shell/package/contents/configuration/pages/AppearanceConfig.qml"));
    QVERIFY(appearance.open(QFile::ReadOnly));
    const QString appearanceSource = QString::fromUtf8(appearance.readAll());

    // AppearanceConfig must not run applyDockStylePreset on Component.onCompleted,
    // which would clobber user configuration when the settings dialog is opened.
    QVERIFY(!appearanceSource.contains(QStringLiteral("applyDockStylePreset(plasmoid.configuration.dockStyle")));

    // Preset application must not overwrite activeStyle back to Line (0).
    QVERIFY(!appearanceSource.contains(QStringLiteral("activeStyle = targetStyle === 1 ? 1 : 0")));

    QFile indicatorCpp(QStringLiteral(LATTE_SOURCE_DIR "/app/view/indicator/indicator.cpp"));
    QVERIFY(indicatorCpp.open(QFile::ReadOnly));
    const QString indicatorCppSrc = QString::fromUtf8(indicatorCpp.readAll());

    // Indicator scheme update must connect valueChanged to immediately sync config to disk.
    QVERIFY(indicatorCppSrc.contains(QStringLiteral("connect(m_configuration, &QQmlPropertyMap::valueChanged")));
}

void SourceContractTest::mainCppMessageSuppressionCoversFrameworkWarnings()
{
    QFile mainCpp(QStringLiteral(LATTE_SOURCE_DIR "/app/main.cpp"));
    QVERIFY(mainCpp.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(mainCpp.readAll());

    // The framework warning filter must suppress:
    // - PlasmaQuick::Dialog and DeclarativeDropArea/DragArea member overrides
    QVERIFY(src.contains(QStringLiteral("PlasmaQuick::Dialog")));
    QVERIFY(src.contains(QStringLiteral("DeclarativeDropArea")));
    QVERIFY(src.contains(QStringLiteral("DeclarativeDragArea")));
    // - QFont setPointSizeF warnings
    QVERIFY(src.contains(QStringLiteral("setPointSizeF")));
    // - DBus notification/job registration failures (expected in latte's context)
    QVERIFY(src.contains(QStringLiteral("Failed to register Notification")));
    QVERIFY(src.contains(QStringLiteral("Failed to register JobViewServer")));
    // - Qt 6 internal Drag.imageSource grabToImage warning (harmless)
    QVERIFY(src.contains(QStringLiteral("sourceSize request for image url that came from grabToImage")));
    // - ToolTipDialog location warning
    QVERIFY(src.contains(QStringLiteral("ToolTipDialog: location")));
}

void SourceContractTest::appletItemInternalViewSplitterAndSortDragGuards()
{
    QFile appletItem(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/applet/AppletItem.qml"));
    QVERIFY(appletItem.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(appletItem.readAll());

    // isInternalViewSplitter must force width/height to 0.
    QVERIFY(src.contains(QStringLiteral("isInternalViewSplitter ? 0")));

    // pluginName must return org.kde.latte.splitter for internal splitters.
    QVERIFY(src.contains(QStringLiteral("\"org.kde.latte.splitter\"")));

    // isSortDragging must elevate z to 1600.
    QVERIFY(src.contains(QStringLiteral("isSortDragging ? 1600")));

    // isExpandedIndicatorActive must exclude activeWindowControl and appmenu.
    QVERIFY(src.contains(QStringLiteral("\"org.kde.activeWindowControl\"")));
    QVERIFY(src.contains(QStringLiteral("\"org.kde.plasma.appmenu\"")));

    // DragHandler must set an explicit dragThreshold ≥ 16 px so Qt 6's
    // near-zero default does not activate sort-drag on micro-movements.
    QVERIFY(src.contains(QStringLiteral("dragThreshold:")));

    // Wayland fallback: a MouseArea (editModeDragArea) must be present
    // for when DragHandler fails to activate with the config window open.
    QVERIFY(src.contains(QStringLiteral("editModeDragArea")));

    // sortDragCommitCooldownMs guard against rapid same-target recommits.
    QVERIFY(src.contains(QStringLiteral("sortDragCommitCooldownMs: 400")));

    // sortDragCenterDeadZoneRatio ≥ 0.44 keeps the centre 88 % of each
    // applet as a no-reorder zone — only the outer 6 % edges can trigger.
    QVERIFY(src.contains(QStringLiteral("sortDragCenterDeadZoneRatio: 0.44")));

    // sortDragMinDistance ≥ 30 px Manhattan so commits require deliberate
    // pointer movement and are not fed by sub-frame micro-motion.
    QVERIFY(src.contains(QStringLiteral("sortDragMinDistance: 30")));
}

void SourceContractTest::appletItemFallbackTrackedWindowsAndConstraintHints()
{
    QFile appletItem(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/applet/AppletItem.qml"));
    QVERIFY(appletItem.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(appletItem.readAll());

    // fallbackTrackedWindowAppIdsByPlugin must map trash and folder
    // applets to Dolphin so window tracking works correctly.
    QVERIFY(src.contains(QStringLiteral("\"org.kde.plasma.trash\"")));
    QVERIFY(src.contains(QStringLiteral("\"org.kde.plasma.folder\"")));
    QVERIFY(src.contains(QStringLiteral("\"org.kde.dolphin\"")));

    // canFillThickness must use the Plasma constraintHints bitflag.
    QVERIFY(src.contains(QStringLiteral("canFillThickness")));
    QVERIFY(src.contains(QStringLiteral("constraintHints")));
    QVERIFY(src.contains(QStringLiteral("CanFillArea")));

    // canFillScreenEdge must exist.
    QVERIFY(src.contains(QStringLiteral("canFillScreenEdge")));

    // isRequestingFill must bail out for modern dock style with indexer.
    QVERIFY(src.contains(QStringLiteral("isModernDockStyle && indexerIsSupported")));
}

void SourceContractTest::plasmoidLaunchersAndDragDropHavePluginDetectionGuards()
{
    // Launchers.qml must detect latte-internal separators by URL pattern.
    {
        QFile lncFile(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/abilities/Launchers.qml"));
        QVERIFY(lncFile.open(QFile::ReadOnly));
        const QString src = QString::fromUtf8(lncFile.readAll());
        QVERIFY(src.contains(QStringLiteral("latte-separator")));
    }

    // DragDropArea must detect the latte plasmoid and latte tasks.
    {
        QFile dragQml(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/DragDropArea.qml"));
        QVERIFY(dragQml.open(QFile::ReadOnly));
        const QString src = QString::fromUtf8(dragQml.readAll());
        QVERIFY(src.contains(QStringLiteral("\"org.kde.latte.plasmoid\"")));
    }
}

void SourceContractTest::containmentMainQmlHasPlasmoidWheelBypassGuards()
{
    QFile mainQml(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/main.qml"));
    QVERIFY(mainQml.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(mainQml.readAll());

    // Wheel event activation must target the Tasks plasmoid specifically.
    QVERIFY(src.contains(QStringLiteral("\"org.kde.latte.plasmoid\"")));
}

void SourceContractTest::compactAppletFallbackSizingAndMinimumDimensionGuards()
{
    QFile caFile(QStringLiteral(LATTE_SOURCE_DIR "/shell/package/contents/applet/CompactApplet.qml"));
    QVERIFY(caFile.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(caFile.readAll());

    // representationPreferredWidth must fall back to implicitWidth,
    // then to a hardcoded icon-based minimum.
    QVERIFY(src.contains(QStringLiteral("Kirigami.Units.iconSizes.sizeForLabels")));

    // Popup menu minimum dimensions must be capped at gridUnit * 18
    // for resizable applet popups.
    QVERIFY(src.contains(QStringLiteral("Kirigami.Units.gridUnit * 18")));
}

void SourceContractTest::clonedViewDefersInitialAppletOrderSyncUntilStructuralReady()
{
    QFile clonedViewFile(QStringLiteral(LATTE_SOURCE_DIR "/app/view/clonedview.cpp"));
    QVERIFY(clonedViewFile.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(clonedViewFile.readAll());

    // structuralSyncReady() must be defined as a gate for deferred sync
    QVERIFY(src.contains(QStringLiteral("bool ClonedView::structuralSyncReady() const")));

    // Clone-side initializationCompleted handler: after setting m_cloneInitialized
    // and updating the applet IDs hash, it must check structuralSyncReady() and
    // fire onOriginalAppletsOrderChanged() when the structure becomes ready.
    // This is the deferred initial order sync — without it, the clone keeps
    // Plasma's default order and the task-manager plasmoid lands at a different
    // appletIndex, breaking launcher sync (syncedGroupId mismatch).
    QVERIFY(src.contains(QStringLiteral("m_cloneInitialized = true;")));
    QVERIFY(src.contains(QStringLiteral("if (structuralSyncReady())")));
    QVERIFY(src.contains(QStringLiteral("onOriginalAppletsOrderChanged();")));

    // Original-side initializationCompleted handler must contain the same
    // deferred-sync pattern so whichever side initializes last triggers
    // the initial order sync.
    QVERIFY(src.contains(QStringLiteral("m_originalInitialized = true;")));

    // The structuralSyncReady() guard inside onOriginalAppletsOrderChanged()
    // itself must remain — it protects against spurious syncs during early
    // startup before both sides and the mapping hash are ready.
    QVERIFY(src.contains(QStringLiteral("void ClonedView::onOriginalAppletsOrderChanged()")));
    QVERIFY(src.contains(QStringLiteral("if (!structuralSyncReady())")));
}

void SourceContractTest::layoutManagerShouldRetryGatedByMaxRetries()
{
    QFile layoutManagerCpp(QStringLiteral(LATTE_SOURCE_DIR "/containment/plugin/layoutmanager.cpp"));
    QVERIFY(layoutManagerCpp.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(layoutManagerCpp.readAll());

    // The shouldRetry condition must include m_restoreRetryCount < 60 so that
    // when max retries are exhausted the function falls through to start the
    // hasRestoredApplets timer.  Without this gate the dock stays off-screen
    // permanently because shouldRetry stays true forever while no more retries
    // are scheduled.
    QVERIFY(src.contains(QStringLiteral(
                             "const bool shouldRetry = ((expectedAppletCount > 0) || initialWarmupRetries) && (m_restoreRetryCount < 60);")));
}

void SourceContractTest::mainCppSelfPipeSigintHandlerAndReplaceLockTimeout()
{
    QFile mainCpp(QStringLiteral(LATTE_SOURCE_DIR "/app/main.cpp"));
    QVERIFY(mainCpp.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(mainCpp.readAll());

    // SIGINT must NOT be watched by KSignalHandler — the self-pipe handles it.
    QVERIFY(!src.contains(QStringLiteral("watchSignal(SIGINT)")));

    // SIGTERM and SIGHUP must still be watched by KSignalHandler.
    QVERIFY(src.contains(QStringLiteral("watchSignal(SIGTERM)")));
    QVERIFY(src.contains(QStringLiteral("watchSignal(SIGHUP)")));

    // Self-pipe infrastructure must be present.
    QVERIFY(src.contains(QStringLiteral("static int sigPipe[2]")));
    QVERIFY(src.contains(QStringLiteral("sigaction(SIGINT, &sa, nullptr)")));
    QVERIFY(src.contains(QStringLiteral("new QSocketNotifier(sigPipe[0]")));
    QVERIFY(src.contains(QStringLiteral("SIGINT received via self-pipe")));

    // --replace lock acquisition must use a bounded timeout, not -1 (infinite).
    QVERIFY(!src.contains(QStringLiteral("timeout = -1")));
    QVERIFY(src.contains(QStringLiteral("timeout = 5000")));

    // Better error message when old instance doesn't exit in time.
    QVERIFY(src.contains(QStringLiteral("Old instance (PID %1) did not exit in time")));
}

// ------------------------------------------------------------------------
// LayoutManager restore() boundary conditions
// ------------------------------------------------------------------------

void SourceContractTest::layoutManagerRestoreMaxRetryExhaustionFallsThrough()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/containment/plugin/layoutmanager.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // After 60 retries the function must proceed past the applets.isEmpty()
    // early-return gate and eventually call m_hasRestoredAppletsTimer.start().
    // The gate: shouldRetry must become false when m_restoreRetryCount >= 60.
    QVERIFY(src.contains(QStringLiteral("m_restoreRetryCount < 60")));
    QVERIFY(src.contains(QStringLiteral("m_hasRestoredAppletsTimer.start()")));
    // The retry-scheduling inner if must also respect the 60-retry limit.
    QVERIFY(src.contains(QStringLiteral("shouldRetry && m_restoreRetryCount < 60")));
}

void SourceContractTest::layoutManagerRestoreBuildsOrderFromLiveAppletsWhenStoredEmpty()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/containment/plugin/layoutmanager.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // When the stored appletIdsOrder is empty but live applets exist,
    // the order must be built from the live applet list.
    QVERIFY(src.contains(QStringLiteral("appletIdsOrder.isEmpty() && !applets.isEmpty()")));
    QVERIFY(src.contains(QStringLiteral("appletIdsOrder << currentAppletId")));
}

void SourceContractTest::layoutManagerRestoreInvalidAppletCleanupPreservesValidIds()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/containment/plugin/layoutmanager.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // Invalid applet tracking: only positive IDs are checked, negative/zero skipped.
    QVERIFY(src.contains(QStringLiteral("aid > 0 && !isValidApplet(aid)")));
    // Invalid applets must be removed from the order.
    QVERIFY(src.contains(QStringLiteral("invalidApplets")));
    QVERIFY(src.contains(QStringLiteral("removeAll")));
}

void SourceContractTest::layoutManagerRestoreAppletContainerCreationRetryLimit()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/containment/plugin/layoutmanager.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // Applet container creation retry path must be bounded by 60 retries.
    QVERIFY(src.contains(QStringLiteral("appletContainerCreationFailed")));
    QVERIFY(src.contains(QStringLiteral("applet containers not ready yet, postponing restore")));
}

// ------------------------------------------------------------------------
// VisibilityManager QML guard matrix
// ------------------------------------------------------------------------

void SourceContractTest::visibilityManagerSlotMustBeShownGuardsStartupAndMode()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR
                           "/containment/package/contents/ui/VisibilityManager.qml"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // slotMustBeShown must gate on inStartup, visibility mode, slide animation,
    // relocation hiding, and isHidden state.
    QVERIFY(src.contains(QStringLiteral("function slotMustBeShown")));
    QVERIFY(src.contains(QStringLiteral("root.inStartup")));
    QVERIFY(src.contains(QStringLiteral("WindowsCanCover")));
    QVERIFY(src.contains(QStringLiteral("inRelocationHiding")));
}

void SourceContractTest::visibilityManagerSlotMustBeHideGuardsMouseAndBlockHiding()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR
                           "/containment/package/contents/ui/VisibilityManager.qml"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // slotMustBeHide must gate on isHidden, containsMouse, blockHiding,
    // inStartup, inSlidingIn, inRelocationHiding, and SidebarOnDemand mode.
    QVERIFY(src.contains(QStringLiteral("function slotMustBeHide")));
    QVERIFY(src.contains(QStringLiteral("!latteView.visibility.blockHiding")));
    QVERIFY(src.contains(QStringLiteral("SidebarOnDemand")));
    QVERIFY(src.contains(QStringLiteral("containsMouse")));
}

void SourceContractTest::visibilityManagerUpdateMaskAreaClampingPreventsNegativeBounds()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR
                           "/containment/package/contents/ui/VisibilityManager.qml"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // Mask area updates must clamp coordinates to non-negative values
    // and constrain dimensions within the latteView bounds.
    QVERIFY(src.contains(QStringLiteral("updateMaskArea")));
    QVERIFY(src.contains(QStringLiteral("Math.max(0")));
    QVERIFY(src.contains(QStringLiteral("Math.min")));
}

void SourceContractTest::visibilityManagerSlidingOutAnimationInitGatesInStartup()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR
                           "/containment/package/contents/ui/VisibilityManager.qml"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // The sliding-out animation init() must start the animation when
    // inRelocationAnimation, inStartup, or !blockHiding is true.
    QVERIFY(src.contains(QStringLiteral("function init()")));
    QVERIFY(src.contains(QStringLiteral("inRelocationAnimation || root.inStartup")));
}

// ------------------------------------------------------------------------
// Positioner boundary checks
// ------------------------------------------------------------------------

void SourceContractTest::positionerStartupPaintOffScreenAtNegative9999()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/view/positioner.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // During startup the dock must be painted off-screen at (-9999, -9999)
    // so the initial layout happens invisibly.
    QVERIFY(src.contains(QStringLiteral("m_inStartup")));
    QVERIFY(src.contains(QStringLiteral("-9999")));
}

void SourceContractTest::positionerSyncGeometrySuppressedDuringSlideAnimation()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/view/positioner.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // syncGeometry must return early when a slide animation is in progress
    // or the slide offset is non-zero.
    QVERIFY(src.contains(QStringLiteral("m_slideOffset != 0 || inSlideAnimation()")));
}

void SourceContractTest::positionerResizeWindowEnforcesWaylandMinimumSize()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/view/positioner.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // Width and height must be clamped to at least 1 to prevent
    // Wayland protocol violations from zero-size windows.
    QVERIFY(src.contains(QStringLiteral("qMax(1, size.width())")));
    QVERIFY(src.contains(QStringLiteral("qMax(1, size.height())")));
}

void SourceContractTest::positionerPlasmaPanelGeometryIntersectionGuardsEmptyList()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/view/positioner.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // When plasma panel geometries are empty, the intersection step is skipped.
    QVERIFY(src.contains(QStringLiteral("panelGeometries.isEmpty()")));
    QVERIFY(src.contains(QStringLiteral("plasmaPanelGeometries")));
}

// ------------------------------------------------------------------------
// Storage boundary constants
// ------------------------------------------------------------------------

void SourceContractTest::storageAvailableIdUpperBoundExcludesId32000()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/layouts/storage.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // The ID search loop uses i < 32000, so ID 32000 is never returned.
    QVERIFY(src.contains(QStringLiteral("32000")));
    // The loop constrain i < 32000 must be present.
    QVERIFY(src.contains(QStringLiteral("i < 32000")) || src.contains(QStringLiteral("i<32000")));
}

void SourceContractTest::storageNewUniqueIdsFileCircularAssignmentDetection()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/layouts/storage.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // Circular assignment detection: cId != assigned[cId] && cId == value2.
    QVERIFY(src.contains(QStringLiteral("cId != assigned[cId]")));
    QVERIFY(src.contains(QStringLiteral("cId == value2")));
}

void SourceContractTest::storageExpectedViewScreenIdAllSecondaryEmptyFallback()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/layouts/storage.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // When AllSecondaryScreensGroup is selected and the secondary list is empty,
    // the view must fall back to the primary screen.
    QVERIFY(src.contains(QStringLiteral("AllSecondaryScreensGroup")));
    QVERIFY(src.contains(QStringLiteral("secondaryscreens.isEmpty()")));
}

// ------------------------------------------------------------------------
// Importer version detection
// ------------------------------------------------------------------------

void SourceContractTest::importerFileVersionDetectsLayoutVersion2AndConfigVersion1()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/layouts/importer.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // fileVersion must distinguish between ConfigVersion1, ConfigVersion2,
    // and UnknownFileType.
    QVERIFY(src.contains(QStringLiteral("ConfigVersion1")));
    QVERIFY(src.contains(QStringLiteral("ConfigVersion2")));
    QVERIFY(src.contains(QStringLiteral("UnknownFileType")));
    QVERIFY(src.contains(QStringLiteral("LayoutVersion2")));
}

void SourceContractTest::importerLayoutCanBeImportedRejectsVersionBelow2()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/layouts/importer.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // layoutCanBeImported must reject layout files whose version is below 2.
    QVERIFY(src.contains(QStringLiteral("layoutCanBeImported")));
    QVERIFY(src.contains(QStringLiteral("version")) && src.contains(QStringLiteral(">=2")));
}

void SourceContractTest::importerUniqueLayoutNameCollisionLoopHasGuard()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/layouts/importer.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // The name collision resolution loop must not be unbounded.
    QVERIFY(src.contains(QStringLiteral("uniqueLayoutName")));
    QVERIFY(src.contains(QStringLiteral("layoutExists(name)")));
}

// ------------------------------------------------------------------------
// ScreenPool fallback paths
// ------------------------------------------------------------------------

void SourceContractTest::screenPoolScreenForIdFallsBackToPrimaryWhenScreenAbsent()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/screenpool.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // When the requested screen ID is valid but the screen is not currently
    // connected, the function must fall back to the primary screen.
    QVERIFY(src.contains(QStringLiteral("screenForId")));
    QVERIFY(src.contains(QStringLiteral("primaryScreen()")));
}

void SourceContractTest::plasmaExtendedScreenPoolIdReturnsZeroForPrimaryConnector()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/plasma/extended/screenpool.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // When the connector matches the primary screen name but is not in the
    // internal map, id() must return 0 as the primary-screen sentinel.
    QVERIFY(src.contains(QStringLiteral("primaryScreen()->name() == connector")));
}

void SourceContractTest::screenPoolIsScreenActiveReturnsFalseForStaleDisconnectedId()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/screenpool.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // isScreenActive must return false when the screen ID exists in the
    // mapping table but the physical screen is no longer connected.
    QVERIFY(src.contains(QStringLiteral("isScreenActive")));
    QVERIFY(src.contains(QStringLiteral("qGuiApp->screens()")));
}

// ------------------------------------------------------------------------
// Launchers QML boundary conditions
// ------------------------------------------------------------------------

void SourceContractTest::launchersNormalizeLauncherListExpandsOnlyLength2to4()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR
                           "/plasmoid/package/contents/ui/abilities/Launchers.qml"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // Default expansion only applies when normalized.length >= 2 and < 5
    // (i.e. lengths 2, 3, 4 — not 0, 1, or 5+).
    QVERIFY(src.contains(QStringLiteral("normalized.length >= 2")));
    QVERIFY(src.contains(QStringLiteral("normalized.length < defaultLaunchers.length")));
    QVERIFY(src.contains(QStringLiteral("defaultLaunchers = [")));
}

void SourceContractTest::launchersFreeSeparatorNameExhaustionReturnsEmptyString()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR
                           "/plasmoid/package/contents/ui/abilities/Launchers.qml"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // When all 19 separator names are taken, freeAvailableSeparatorName
    // must return an empty string.
    QVERIFY(src.contains(QStringLiteral("no<20")));
    QVERIFY(src.contains(QStringLiteral("return \"\"")));
}

void SourceContractTest::launchersTransientEmptyRecoveryCeilingAt8()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR
                           "/plasmoid/package/contents/ui/abilities/Launchers.qml"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // Transient empty launcher list recovery is capped at 8 attempts.
    QVERIFY(src.contains(QStringLiteral("_transientEmptyRecoveries < 8")));
}

void SourceContractTest::launchersIsSeparatorGuardsDesktopExtensionPosition()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR
                           "/plasmoid/package/contents/ui/abilities/Launchers.qml"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // isSeparator must check that ".desktop" is NOT at string index 1,
    // because genuine .desktop files have it at index 0.
    QVERIFY(src.contains(QStringLiteral("latte-separator")));
    QVERIFY(src.contains(QStringLiteral(".desktop\")!==1")));
}

void SourceContractTest::launchersAddDroppedLauncherIconDataTruncationBoundary()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR
                           "/plasmoid/package/contents/ui/abilities/Launchers.qml"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // addDroppedLauncher must strip ?iconData= suffix, but only when
    // the separator appears at position > 0 (not at the start).
    QVERIFY(src.contains(QStringLiteral("?iconData=")));
    QVERIFY(src.contains(QStringLiteral("pos>0")));
}

void SourceContractTest::launchersRemoveInternalSeparatorRefusesEmptyUrl()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR
                           "/plasmoid/package/contents/ui/abilities/Launchers.qml"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // removeInternalSeparatorAtPos must only remove separators that carry a
    // real launcher URL. A task that merely renders like a separator (e.g. a
    // window without launcher identity) has an empty launcherUrl, and
    // removeLauncher("") is a silent no-op — reporting success would make
    // the phantom separator impossible to remove from the context menu.
    const int fnStart = src.indexOf(QStringLiteral("function removeInternalSeparatorAtPos"));
    QVERIFY(fnStart > 0);
    const int fnEnd = src.indexOf(QStringLiteral("function removeLauncher"), fnStart);
    QVERIFY(fnEnd > fnStart);
    const QString fn = src.mid(fnStart, fnEnd - fnStart);

    QVERIFY(fn.contains(QStringLiteral("item.isSeparator")));
    QVERIFY(fn.contains(QStringLiteral("item.launcherUrl !== \"\"")));
}

void SourceContractTest::taskItemIsSeparatorNeverTurnsUrlLessWindowsIntoDividers()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR
                           "/plasmoid/package/contents/ui/task/TaskItem.qml"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // A task whose modelLauncherUrl is empty must not default to the thin
    // separator placeholder when it is a window or startup item: such tasks
    // have no launcher URL for their whole lifetime (an app that ships no
    // .desktop file), so the placeholder would turn them into phantom
    // separators that cannot be removed through the launcher list.
    const int bindingStart = src.indexOf(QStringLiteral("isSeparator: {"));
    QVERIFY(bindingStart > 0);
    const int bindingEnd = src.indexOf(QStringLiteral("isSeparatorHidden"), bindingStart);
    QVERIFY(bindingEnd > bindingStart);
    const QString binding = src.mid(bindingStart, bindingEnd - bindingStart);

    QVERIFY(binding.contains(QStringLiteral("!isWindow")));
    QVERIFY(binding.contains(QStringLiteral("!isStartup")));
}

void SourceContractTest::contextMenuSeparatorRemovalRoutesThroughGuardedLauncherPath()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR
                           "/plasmoid/package/contents/ui/ContextMenu.qml"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // The "Remove Right/Left Separator" menu items must first try the
    // launcher-local removal (which now refuses empty-URL phantoms) and only
    // fall back to the containment boundary path when that reports failure.
    const int headItem = src.indexOf(QStringLiteral("id: removeFollowingInternalSeparatorItem"));
    QVERIFY(headItem > 0);
    const int tailItem = src.indexOf(QStringLiteral("id: removeTailInternalSeparatorItem"));
    QVERIFY(tailItem > headItem);

    const QString headBlock = src.mid(headItem, tailItem - headItem);
    QVERIFY(headBlock.contains(QStringLiteral("supportsLauncherSeparators()")));
    QVERIFY(headBlock.contains(QStringLiteral("visualParent.headItemIsSeparator")));
    QVERIFY(headBlock.contains(QStringLiteral("removeInternalSeparatorAtPos(rightIndex)")));
    QVERIFY(headBlock.contains(QStringLiteral("tryRemoveBoundarySeparator(true)")));

    const QString tailBlock = src.mid(tailItem,
                                      src.indexOf(QStringLiteral("id: alternativesMenuItem"), tailItem) - tailItem);
    QVERIFY(tailBlock.contains(QStringLiteral("visualParent.tailItemIsSeparator")));
    QVERIFY(tailBlock.contains(QStringLiteral("removeInternalSeparatorAtPos(leftIndex)")));
    QVERIFY(tailBlock.contains(QStringLiteral("tryRemoveBoundarySeparator(false)")));
}

// ------------------------------------------------------------------------
// main.cpp CLI and startup boundary paths
// ------------------------------------------------------------------------

void SourceContractTest::mainCppAvailableLayoutsPrintsDifferentMessageWhenEmpty()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/main.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // --available-layouts must print a different message when the list is empty
    // vs. non-empty.
    QVERIFY(src.contains(QStringLiteral("layouts.count() > 0")));
    QVERIFY(src.contains(QStringLiteral("There are no available layouts")));
}

void SourceContractTest::mainCppLayoutOptionExitsForMissingLayout()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/main.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // --layout <name> must validate the layout exists and exit if it doesn't.
    QVERIFY(src.contains(QStringLiteral("layoutExists")));
    QVERIFY(src.contains(QStringLiteral("doesn't exist")));
}

void SourceContractTest::mainCppClearCacheSkipsNonexistentDirectory()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/main.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // --clear-cache must skip removal when the cache directory doesn't exist.
    QVERIFY(src.contains(QStringLiteral("cacheDir.exists()")));
    QVERIFY(src.contains(QStringLiteral("removeRecursively")));
}

void SourceContractTest::mainCppImportLayoutExitsOnImportFailure()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/main.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // --import-layout must exit when the import fails (importedLayout.isEmpty()).
    QVERIFY(src.contains(QStringLiteral("importLayoutHelper")));
    QVERIFY(src.contains(QStringLiteral("cannot be imported")));
}

void SourceContractTest::mainCppDeferredDeleteDrainHardLimit5Passes()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/main.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // The post-exec DeferredDelete drain loop must have a hard limit of 5 passes.
    QVERIFY(src.contains(QStringLiteral("pass < 5")));
    QVERIFY(src.contains(QStringLiteral("sendPostedEvents(nullptr, QEvent::DeferredDelete)")));
}

void SourceContractTest::mainCppDetectPlatformPreservesExplicitPlatformArg()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/main.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // When the user passes an explicit -platform or --platform argument,
    // detectPlatform must not override QT_QPA_PLATFORM.
    QVERIFY(src.contains(QStringLiteral("detectPlatform")));
    QVERIFY(src.contains(QStringLiteral("-platform")));
    QVERIFY(src.contains(QStringLiteral("qt.qpa.platform")) || src.contains(QStringLiteral("QT_QPA_PLATFORM")));
}

// ------------------------------------------------------------------------
// main.qml startup sequence contracts
// ------------------------------------------------------------------------

void SourceContractTest::mainQmlInStartupSetFalseInSlidingOutAnimationOnStopped()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR
                           "/containment/package/contents/ui/VisibilityManager.qml"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // root.inStartup must be set to false inside the sliding-out animation's
    // onStopped handler, which triggers the startup-finished transition.
    QVERIFY(src.contains(QStringLiteral("root.inStartup = false")));
    QVERIFY(src.contains(QStringLiteral("root.inStartup")));
}

void SourceContractTest::mainQmlStartupDelayerTriggeredByHasRestoredAppletsSignal()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR
                           "/containment/package/contents/ui/main.qml"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // The startupDelayer timer must be started when hasRestoredApplets becomes true.
    QVERIFY(src.contains(QStringLiteral("hasRestoredApplets")));
    QVERIFY(src.contains(QStringLiteral("startupDelayer.start()")));
}

void SourceContractTest::mainQmlCreateAppletItemRetryCeilingAt80()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR
                           "/containment/package/contents/ui/main.qml"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // The applet item creation retry loop must stop at 80 attempts.
    QVERIFY(src.contains(QStringLiteral("retryCount >= 80")));
    QVERIFY(src.contains(QStringLiteral("createAppletItem")));
}

void SourceContractTest::mainQmlPanelCfgSyncTransparencySevenInputClasses()
{
    // The old panelCfgSync polling timer and its ad-hoc parsing of
    // panelTransparency values (-1, "-1", undefined, null, "",
    // Number(pt) >= 100) have been removed.  kcfg bindings are
    // reactive and the actual transparency path uses
    // themeExtendedBackground.maxOpacity for the default setting
    // and background.currentOpacity for custom values.
    //
    // Verify that the removed dead code is gone and the new
    // direct binding is in place.

    {
        QFile f(QStringLiteral(LATTE_SOURCE_DIR
                               "/containment/package/contents/ui/main.qml"));
        QVERIFY(f.open(QFile::ReadOnly));
        const QString src = QString::fromUtf8(f.readAll());

        // The polling timer must not be present.
        QVERIFY(!src.contains(QStringLiteral("panelCfgSync")));

        // blurEnabled must NOT be gated on a custom-transparency flag
        // so that blur can be used with user-chosen opacity values.
        QVERIFY(!src.contains(QStringLiteral("panelCustomTransparency")));
        QVERIFY(!src.contains(QStringLiteral("panelBgOpacity")));

        // blurEnabled still responds to the user's blur toggle and the
        // force-transparent / force-panel-for-busy-background guard.
        QVERIFY(src.contains(QStringLiteral("blurEnabled")));
        QVERIFY(src.contains(QStringLiteral("plasmoid.configuration.blurEnabled")));
        QVERIFY(src.contains(QStringLiteral("forceTransparentPanel")));
    }

    {
        // effectiveBackgroundOpacity is now set from BindingsExternal.
        // The default (-1) case reports 1.0 to preserve the ghosting
        // guard in effects.cpp (blur is skipped when opacity >= 0.95).
        // The C++ gate now uses backgroundOpacity instead, which is -1
        // at default and correctly allows effects through.
        QFile f(QStringLiteral(LATTE_SOURCE_DIR
                               "/containment/package/contents/ui/BindingsExternal.qml"));
        QVERIFY(f.open(QFile::ReadOnly));
        const QString src = QString::fromUtf8(f.readAll());

        QVERIFY(src.contains(QStringLiteral("effectiveBackgroundOpacity")));
        QVERIFY(src.contains(QStringLiteral("background.currentOpacity")));
        QVERIFY(src.contains(QStringLiteral("panelTransparency === -1")));
    }
}

void SourceContractTest::mainQmlOnInStartupChangedMustCheckLatteViewExists()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR
                           "/containment/package/contents/ui/main.qml"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // onInStartupChanged must guard against null latteView before accessing
    // latteView.positioner.
    QVERIFY(src.contains(QStringLiteral("onInStartupChanged")));
    QVERIFY(src.contains(QStringLiteral("latteView && latteView.positioner")));
}

void SourceContractTest::appletItemForAppletInSafeContextsUsesPublicApi()
{
    // Safe contexts: verified through testing that itemForApplet works correctly.
    QFile contextMenu(QStringLiteral(LATTE_SOURCE_DIR "/app/declarativeimports/contextmenulayerquickitem.cpp"));
    QVERIFY(contextMenu.open(QFile::ReadOnly));
    const QString ctxSource = QString::fromUtf8(contextMenu.readAll());
    // popUpTopLeft: direct itemForApplet call (non-const)
    QVERIFY(ctxSource.contains(QStringLiteral("PlasmaQuick::AppletQuickItem::itemForApplet(applet)")));
    QVERIFY(!ctxSource.contains(QStringLiteral("applet->property(\"_plasma_graphicObject\")")));
    // mousePressEvent loop: itemForApplet with const_cast for const applet pointer
    QVERIFY(ctxSource.contains(QStringLiteral("itemForApplet(const_cast<Plasma::Applet *>(appletTemp))")));

    QFile ci(QStringLiteral(LATTE_SOURCE_DIR "/app/view/containmentinterface.cpp"));
    QVERIFY(ci.open(QFile::ReadOnly));
    const QString ciSource = QString::fromUtf8(ci.readAll());

    // applicationLauncherInPopup — itemForApplet with const_cast (launcher lookup)
    QVERIFY(ciSource.contains(QStringLiteral("appLauncherItem = PlasmaQuick::AppletQuickItem::itemForApplet(const_cast<Plasma::Applet *>(applet))")));

    // updateBadgeForLatteTask — itemForApplet (non-const auto *applet)
    QVERIFY(ciSource.contains(QStringLiteral("appletInterface = PlasmaQuick::AppletQuickItem::itemForApplet(applet)")));

    // collapseAllApplets / appletIsExpandable / appletIsActivationTogglesExpanded
    // — itemForApplet with const_cast (const auto from applets())
    QVERIFY(ciSource.contains(QStringLiteral("itemForApplet(const_cast<Plasma::Applet *>(applet))")));

    // appletConfiguration — itemForApplet with const_cast (const param)
    QVERIFY(ciSource.contains(QStringLiteral("itemForApplet(const_cast<Plasma::Applet *>(applet))")));
}

void SourceContractTest::appletItemForAppletExcludedContextsPreservePropertyAccess()
{
    // Excluded contexts: itemForApplet returns nullptr — keep _plasma_graphicObject.

    // alternativeshelper.cpp: applet() getter and loadAlternative (applet not registered yet)
    QFile alt(QStringLiteral(LATTE_SOURCE_DIR "/app/alternativeshelper.cpp"));
    QVERIFY(alt.open(QFile::ReadOnly));
    const QString altSource = QString::fromUtf8(alt.readAll());
    QVERIFY(altSource.contains(QStringLiteral("_plasma_graphicObject")));
    QVERIFY(!altSource.contains(QStringLiteral("itemForApplet")));

    // view.cpp: containment-level lookups (containment graphic item positioning)
    QFile view(QStringLiteral(LATTE_SOURCE_DIR "/app/view/view.cpp"));
    QVERIFY(view.open(QFile::ReadOnly));
    const QString viewSource = QString::fromUtf8(view.readAll());
    QVERIFY(viewSource.contains(QStringLiteral("_plasma_graphicObject")));

    // subconfigview.cpp: findGraphicContextObject (container-level lookup)
    QFile subconfig(QStringLiteral(LATTE_SOURCE_DIR "/app/view/settings/subconfigview.cpp"));
    QVERIFY(subconfig.open(QFile::ReadOnly));
    const QString subconfigSource = QString::fromUtf8(subconfig.readAll());
    QVERIFY(subconfigSource.contains(QStringLiteral("_plasma_graphicObject")));

    // containmentinterface.cpp: containment-level lookups (containment graphic item
    // for view, subcontainment configuration) — keep _plasma_graphicObject.
    // onAppletAdded and toggleAppletExpanded now use itemForApplet (Plasma 6 API).
    QFile ci(QStringLiteral(LATTE_SOURCE_DIR "/app/view/containmentinterface.cpp"));
    QVERIFY(ci.open(QFile::ReadOnly));
    const QString ciSource = QString::fromUtf8(ci.readAll());
    // onAppletAdded: uses itemForApplet (Plasma 6)
    QVERIFY(ciSource.contains(QStringLiteral("itemForApplet(applet)")));
    // toggleAppletExpanded: uses itemForApplet (Plasma 6)
    const int toggleIdx = ciSource.indexOf(QStringLiteral("toggleAppletExpanded"));
    QVERIFY(toggleIdx >= 0);
    const int itemForAppletIdx = ciSource.indexOf(QStringLiteral("itemForApplet"), toggleIdx);
    QVERIFY(itemForAppletIdx >= 0);
    // Still has _plasma_graphicObject for containment-level lookups
    QVERIFY(ciSource.contains(QStringLiteral("_plasma_graphicObject")));
}

void SourceContractTest::mouseButtonEnumUsesMiddleButtonNotMidButton()
{
    // Qt.MidButton was removed in Qt 6. QML must use Qt.MiddleButton
    // to avoid evaluating to undefined and silently breaking middle-click
    // handling. EnvironmentActions delegates middle-click to the C++
    // ContextMenuLayerQuickItem handler and only accepts Qt.LeftButton.
    QFile env(QStringLiteral(LATTE_SOURCE_DIR
                             "/containment/package/contents/ui/layouts/EnvironmentActions.qml"));
    QVERIFY(env.open(QFile::ReadOnly));
    const QString envSource = QString::fromUtf8(env.readAll());
    // EnvironmentActions only accepts LeftButton (middle-click handled by C++)
    QVERIFY(envSource.contains(QStringLiteral("acceptedButtons: Qt.LeftButton")));
    QVERIFY(!envSource.contains(QStringLiteral("Qt.MidButton")));
    QVERIFY(!envSource.contains(QStringLiteral("Qt.MiddleButton")));

    QFile taskMouse(QStringLiteral(LATTE_SOURCE_DIR
                                   "/plasmoid/package/contents/ui/task/TaskMouseArea.qml"));
    QVERIFY(taskMouse.open(QFile::ReadOnly));
    const QString taskMouseSource = QString::fromUtf8(taskMouse.readAll());
    QVERIFY(taskMouseSource.contains(QStringLiteral("Qt.MiddleButton")));
    QVERIFY(!taskMouseSource.contains(QStringLiteral("Qt.MidButton")));

    QFile clickedAnim(QStringLiteral(LATTE_SOURCE_DIR
                                     "/plasmoid/package/contents/ui/task/animations/ClickedAnimation.qml"));
    QVERIFY(clickedAnim.open(QFile::ReadOnly));
    const QString clickedAnimSource = QString::fromUtf8(clickedAnim.readAll());
    QVERIFY(clickedAnimSource.contains(QStringLiteral("Qt.MiddleButton")));
    QVERIFY(!clickedAnimSource.contains(QStringLiteral("Qt.MidButton")));
}

void SourceContractTest::taskMouseAreaSkipsInactivePreviewChecks()
{
    QFile taskMouse(QStringLiteral(LATTE_SOURCE_DIR
                                   "/plasmoid/package/contents/ui/task/TaskMouseArea.qml"));
    QVERIFY(taskMouse.open(QFile::ReadOnly));
    const QString taskMouseSource = QString::fromUtf8(taskMouse.readAll());

    // Preview eligibility is inactive when both previews and window
    // highlighting are disabled; keep the guard before the state lookup.
    const QString guard = QStringLiteral("if((root.showPreviews || root.highlightWindows)\n"
                                         "                && isAbleToShowPreview");
    QVERIFY(taskMouseSource.contains(guard));
}

void SourceContractTest::environmentActionsDoesNotAcceptMiddleButton()
{
    // EnvironmentActions delegates middle-click to the C++
    // ContextMenuLayerQuickItem handler and must not accept MiddleButton
    // in its MouseArea — doing so would swallow the event before it
    // reaches the C++ layer.
    QFile env(QStringLiteral(LATTE_SOURCE_DIR
                             "/containment/package/contents/ui/layouts/EnvironmentActions.qml"));
    QVERIFY(env.open(QFile::ReadOnly));
    const QString envSource = QString::fromUtf8(env.readAll());
    QVERIFY(envSource.contains(QStringLiteral("acceptedButtons: Qt.LeftButton")));
    QVERIFY(!envSource.contains(QStringLiteral("Qt.MidButton")));
    QVERIFY(!envSource.contains(QStringLiteral("Qt.MiddleButton")));
}

void SourceContractTest::upgraderQmlUsesPlasmoidConfiguration()
{
    // In Plasma 6, tasks is an AppletQuickItem wrapper.  Accessing
    // tasks.configuration directly returns undefined.  The correct
    // path is tasks.plasmoid.configuration.
    QFile upgrader(QStringLiteral(LATTE_SOURCE_DIR
                                  "/containment/package/contents/ui/Upgrader.qml"));
    QVERIFY(upgrader.open(QFile::ReadOnly));
    const QString upgraderSource = QString::fromUtf8(upgrader.readAll());
    QVERIFY(upgraderSource.contains(QStringLiteral("tasks.plasmoid.configuration")));
    QVERIFY(!upgraderSource.contains(QStringLiteral("tasks.configuration")));
}

void SourceContractTest::middleClickActionDefaultIsClose()
{
    // The default middleClickAction should be Close (1), not NewInstance (2).
    // Middle-click on a task icon should close the task window by default.
    QFile configXml(QStringLiteral(LATTE_SOURCE_DIR
                                   "/plasmoid/package/contents/config/main.xml"));
    QVERIFY(configXml.open(QFile::ReadOnly));
    const QString xmlSource = QString::fromUtf8(configXml.readAll());

    // Find the middleClickAction entry and verify its default
    const int entryIdx = xmlSource.indexOf(QStringLiteral("middleClickAction"));
    QVERIFY(entryIdx >= 0);
    const int defaultIdx = xmlSource.indexOf(QStringLiteral("<default>"), entryIdx);
    QVERIFY(defaultIdx >= 0);
    QVERIFY(xmlSource.indexOf(QStringLiteral("<default>1</default>"), entryIdx) >= 0);
    QVERIFY(!(xmlSource.indexOf(QStringLiteral("<default>2</default>"), entryIdx) >= 0
              && xmlSource.indexOf(QStringLiteral("<default>2</default>"), entryIdx) < xmlSource.indexOf(QStringLiteral("</entry>"), entryIdx)));
}

void SourceContractTest::dragDropHandlersUseBindingSyntaxForQt6()
{
    // function onDragEnter/onDragMove/onDrop does not connect to
    // DragDrop.DropArea signals in Qt 6. Arrow-function binding form
    // is required. Widget Explorer drops (text/x-plasmoidservicename)
    // are handled by the C++ path; QML onDrop returns early for this
    // mime to avoid double-creation with handlePlasmoidDrop().
    QFile dnd(QStringLiteral(LATTE_SOURCE_DIR
                             "/containment/package/contents/ui/DragDropArea.qml"));
    QVERIFY(dnd.open(QFile::ReadOnly));
    const QString dndSource = QString::fromUtf8(dnd.readAll());
    QVERIFY(dndSource.contains(QStringLiteral("onDragEnter:")));
    QVERIFY(dndSource.contains(QStringLiteral("onDragMove:")));
    QVERIFY(dndSource.contains(QStringLiteral("onDrop:")));
    QVERIFY(!dndSource.contains(QStringLiteral("function onDragEnter(event)")));
    QVERIFY(!dndSource.contains(QStringLiteral("function onDragMove(event)")));
    QVERIFY(!dndSource.contains(QStringLiteral("function onDrop(event)")));
    // Mime split guard: prevents QML double-handling Widget Explorer drops
    QVERIFY(dndSource.contains(QStringLiteral("text/x-plasmoidservicename")));
}

// ── Infinite-loop guard contracts ────────────────────────────────────

void SourceContractTest::tasktoolsServicesFromCmdLineGuardsFirstSpace()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/wm/tasktools.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // servicesFromCmdLine recursion must guard firstSpace >= 0 so a
    // single-word cmdLine in TryIgnoreRuntimes does not infinitely
    // recurse (firstSpace == -1 → mid(0) == identical string).
    QVERIFY(src.contains(QStringLiteral("firstSpace >= 0")));
}

void SourceContractTest::importerUniqueLayoutNameHasIterationCap()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/layouts/importer.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // uniqueLayoutName while-loop must have an iteration cap to prevent
    // an unbounded walk when layoutExists() never returns false.
    QVERIFY(src.contains(QStringLiteral("i < 10000")));
}

void SourceContractTest::viewscontrollerUniqueViewNameHasIterationCap()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR
                           "/app/settings/viewsdialog/viewscontroller.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // uniqueViewName while-loop must have an iteration cap.
    QVERIFY(src.contains(QStringLiteral("i < 10000")));
}

void SourceContractTest::layoutscontrollerUniqueLayoutNameHasIterationCap()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR
                           "/app/settings/settingsdialog/layoutscontroller.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // uniqueLayoutName while-loop must have an iteration cap.
    QVERIFY(src.contains(QStringLiteral("i < 10000")));
}

void SourceContractTest::lattecoronaUnloadUsesCappedLiveListLoop()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/lattecorona.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // Corona::unload() must delete from the live containments() list —
    // a snapshot would double-delete containments destroyed by cascade
    // through their parent containment — with an iteration cap as a
    // backstop against pathological re-entrant additions.
    QVERIFY(src.contains(QStringLiteral("containments().isEmpty()")));
    QVERIFY(src.contains(QStringLiteral("guard++ < 10000")));
}

void SourceContractTest::lattecoronaScreenForContainmentHasDepthGuard()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/lattecorona.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // screenForContainment parent-applet chain walk must be
    // depth-limited (max 16 hops) instead of unbounded recursion.
    QVERIFY(src.contains(QStringLiteral("depth < 16")));
}

void SourceContractTest::originalViewCleanClonesDrainsFromLocalCopy()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/view/originalview.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // cleanClones() must drain from a local copy so that re-entrant
    // signal chains during removeClone() cannot grow m_clones underneath
    // the iteration.
    QVERIFY(src.contains(QStringLiteral("snapshot = m_clones")));
}

void SourceContractTest::layoutManagerResolveAppletQuickItemUsesVisitedSet()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR
                           "/containment/plugin/layoutmanager.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // resolveAppletQuickItemObject must use a visited-set to detect
    // cyclic property graphs and prevent infinite recursion.
    QVERIFY(src.contains(QStringLiteral("visited")));
    QVERIFY(src.contains(QStringLiteral("visited.contains(candidate)")));
}

void SourceContractTest::levelOptionsHasIsBackgroundIsForegroundReentryGuards()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR
                           "/declarativeimports/abilities/items/indicators/LevelOptions.qml"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // isBackground ↔ isForeground mutual-assignment handlers must have
    // reentry guards to prevent flip-flopping when both are set to the
    // same value simultaneously.
    QVERIFY(src.contains(QStringLiteral("_updatingBackground")));
    QVERIFY(src.contains(QStringLiteral("_updatingForeground")));
}

void SourceContractTest::templatesmanagerUniqueNameLoopsHaveIterationCaps()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/templates/templatesmanager.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // The unique template-name loops must be capped like every other
    // unique-name loop; an unbounded walk would hang when the template
    // table keeps matching every generated name.
    QVERIFY(src.contains(QStringLiteral("while (hasLayoutTemplate(name) && i < 10000)")));
    QVERIFY(src.contains(QStringLiteral("while (hasViewTemplate(name) && i < 10000)")));
}

void SourceContractTest::synchronizerUnloadLayoutsHasIterationCap()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/layouts/synchronizer.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // unloadLayouts must drain the central-layout list with an
    // iteration cap so re-entrant additions cannot hang shutdown.
    QVERIFY(src.contains(QStringLiteral("guard++ < 10000")));
    QVERIFY(src.contains(QStringLiteral("unloadLayouts: iteration cap reached")));
}

void SourceContractTest::uniqueNameExhaustionFallsBackToRandomSuffix()
{
    const QStringList sources = {
        QStringLiteral("/app/layouts/importer.cpp"),
        QStringLiteral("/app/settings/settingsdialog/layoutscontroller.cpp"),
        QStringLiteral("/app/settings/viewsdialog/viewscontroller.cpp"),
        QStringLiteral("/app/templates/templatesmanager.cpp"),
    };

    for (const QString &path : sources) {
        QFile f(QStringLiteral(LATTE_SOURCE_DIR) + path);
        QVERIFY2(f.open(QFile::ReadOnly), qPrintable(path));
        const QString src = QString::fromUtf8(f.readAll());

        // After the numbered-suffix loop hits its cap, a random suffix
        // must be used instead of returning a name that still exists.
        QVERIFY2(src.contains(QStringLiteral("QRandomGenerator::global()->generate()")),
                 qPrintable(path));
    }
}

void SourceContractTest::layoutManagerResolveAppletQuickItemThreadsVisitedSet()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/containment/plugin/layoutmanager.cpp"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // The visited-set must be threaded through recursion frames; a
    // per-frame set would let A↔B cyclic property graphs recurse forever.
    QVERIFY(src.contains(QStringLiteral("resolveAppletQuickItemObjectInternal(candidate, visited)")));
}

void SourceContractTest::appdataComponentIdKeepsHyphenInLastSegment()
{
    QFile f(QStringLiteral(LATTE_SOURCE_DIR "/app/org.kde.latte-dock.appdata.xml.cmake"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QString src = QString::fromUtf8(f.readAll());

    // AppStream only allows hyphens in the last ID segment; the
    // ".desktop" suffix moved the hyphen out of the last segment and
    // appstreamcli validation fails on it.  Keep the ID without the
    // suffix and use a developer block instead of developer_name.
    QVERIFY(src.contains(QStringLiteral("<id>org.kde.latte-dock</id>")));
    QVERIFY(!src.contains(QStringLiteral("<id>org.kde.latte-dock.desktop</id>")));
    QVERIFY(src.contains(QStringLiteral("<developer id=")));
    QVERIFY(!src.contains(QStringLiteral("<developer_name>")));
}

void SourceContractTest::positionShortcutHandlersDeclareSignalParameters()
{
    QFile basicItem(QStringLiteral(LATTE_SOURCE_DIR "/declarativeimports/abilities/items/BasicItem.qml"));
    QVERIFY(basicItem.open(QFile::ReadOnly));
    const QString basicSource = QString::fromUtf8(basicItem.readAll());
    QVERIFY(basicSource.contains(QStringLiteral("function onSglActivateEntryAtIndex(entryIndex)")));
    QVERIFY(basicSource.contains(QStringLiteral("function onSglNewInstanceForEntryAtIndex(entryIndex)")));
    QVERIFY(!basicSource.contains(QStringLiteral("shortcutIndex(taskItem.itemIndex)")));

    QFile appletItem(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/applet/AppletItem.qml"));
    QVERIFY(appletItem.open(QFile::ReadOnly));
    const QString appletSource = QString::fromUtf8(appletItem.readAll());
    QVERIFY(appletSource.contains(QStringLiteral("function onSglActivateEntryAtIndex(entryIndex)")));
    QVERIFY(appletSource.contains(QStringLiteral("function onSglNewInstanceForEntryAtIndex(entryIndex)")));
}

void SourceContractTest::positionShortcutHostLookupIsRecursiveAndResettable()
{
    QFile sourceFile(QStringLiteral(LATTE_SOURCE_DIR "/app/view/containmentinterface.cpp"));
    QVERIFY(sourceFile.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(sourceFile.readAll());
    QVERIFY(source.contains(QStringLiteral("findQuickItemByObjectName(searchRoot")));
    QVERIFY(source.contains(QStringLiteral("m_layoutManager->property(\"rootItem\")")));
    QVERIFY(source.contains(QStringLiteral("clearShortcutsHost();")));
    QVERIFY(source.contains(QStringLiteral("m_shortcutsHostDestroyedConnection")));

    QFile bridgeFile(QStringLiteral(LATTE_SOURCE_DIR "/declarativeimports/abilities/bridge/PositionShortcuts.qml"));
    QVERIFY(bridgeFile.open(QFile::ReadOnly));
    const QString bridgeSource = QString::fromUtf8(bridgeFile.readAll());
    QVERIFY(bridgeSource.contains(QStringLiteral("function onSglActivateEntryAtIndex(entryIndex)")));
    QVERIFY(bridgeSource.contains(QStringLiteral("client.sglActivateEntryAtIndex(entryIndex)")));
    QVERIFY(!bridgeSource.contains(QStringLiteral("host.sglActivateEntryAtIndex.connect")));
}

void SourceContractTest::qmlCacheRevisionInvalidatesSameVersionBuilds()
{
    QFile appTypes(QStringLiteral(LATTE_SOURCE_DIR "/app/apptypes.h"));
    QVERIFY(appTypes.open(QFile::ReadOnly));
    const QString appTypesSource = QString::fromUtf8(appTypes.readAll());
    QVERIFY(appTypesSource.contains(QStringLiteral("QMLCACHEREVISION[] = \"")));
    QVERIFY(!appTypesSource.contains(QStringLiteral("QMLCACHEREVISION[] = \"\"")));
    QFile mainSourceFile(QStringLiteral(LATTE_SOURCE_DIR "/app/main.cpp"));
    QVERIFY(mainSourceFile.open(QFile::ReadOnly));
    const QString mainSource = QString::fromUtf8(mainSourceFile.readAll());
    QVERIFY(mainSource.contains(QStringLiteral("Latte::App::QMLCACHEREVISION")));
    QVERIFY(mainSource.contains(QStringLiteral("cachedVersion != currentVersion")));
}

QTEST_MAIN(SourceContractTest)

#include "sourcecontracttest.moc"
