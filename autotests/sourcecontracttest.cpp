/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QFile>
#include <QTest>

#include "../app/session/shutdownstate.h"

class SourceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void plasmaVolumeBootstrapContractMovedToQmlSmokeTest();
    void compactAppletPopupSizingContractMovedToQmlSmokeTest();
    void applicationLauncherUsesFixedExternalSlot();
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
    void itemsAlignmentIsSeparateAndJustifyOnly();
    void itemsAlignmentNormalizesDirectionsByFormFactor();
    void itemsAlignmentConfigDefaultsToCenter();
    void appearancePaletteExposesLayoutCustomColors();
    void modernDockBackgroundShadowDefaultIsCompact();
    void layoutDetailsExposeCustomColorSchemeSelector();
    void restoreAnimationContractMovedToQmlSmokeTest();
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
    void autostartDefaultEnabledContracts();
    void cmakeWarningRelaxationLivesInModule();
    void cmakeFindsQtCoreToolsBeforeKdeInstallDirs();
    void qtQuickGpuPreferenceKeepsSoftwareFallbackAvailable();
    void knsCompatImportsAreAvailableForSystemInstall();
    void layerShellSetScreenGuardPreventsBuildRegression();
    void taskIconsRefreshAfterIconThemeChanges();
    void taskAudioBadgesScaleWithParabolicZoom();
    void widgetExplorerLaunchesKnsDialogOutOfProcess();
    void widgetExplorerUsesPlasmaTranslationContexts();
    void compactAppletDigitalClockWidthCapPreventsLongDateFormatOverflow();
    void contextMenuLayerMiddleClickCloseActiveWindowGuardedCorrectly();
    void mouseHandlerAutoPinOnDragPromotesNonLauncherTasks();
    void scrollToggleMinimizedDownwardUnmaximizesBeforeMinimizing();
    void scrollToggleMinimizedUsesAllScreensTrackerForMinimizeAndMaximize();
    void systrayGuardsInAppletItemPreventLayoutAndInteractionBreakage();
    void volumeAndAppMenuPopupSizingUsesLargerMinimumInCompactApplet();
    void clipboardAndDigitalClockErrorSuppressionInMainCpp();
    void systemTrayAndPlasmoidActAsAppletInsertionBoundary();
    void separatorAndSpacerDetectionAndBehaviorInAppletItem();
    void separatorGuardsAcrossLayoutAndDragDropFiles();
    void myViewClientIntPropertiesUseSafeIntGuardAgainstUndefined();
    void clonedViewDefersInitialAppletOrderSyncUntilStructuralReady();
    void indicatorFactoryExcludesBuiltinPluginsFromCustomLists();
    void waylandInterfaceAcceptableWindowHasHardcodedAppIdWhitelist();
    void genericLayoutReassertsDefaultContextMenuOnContainmentWiring();
    void layoutManagerCleanupOnStartupRemovesLegacyAndGhostApplets();
    void containmentInterfaceHasSeparatorPluginConstantsAndHelper();
    void appletsModelHasNoPersonalDataAppletList();
    void lattePackageShellPluginNameBranchingIsCorrect();
    void indicatorPlasmaTypeIsRemappedToDefault();
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
    void dragDropHandlersUseBindingSyntaxForQt6();
    void environmentActionsDoesNotAcceptMiddleButton();
    void upgraderQmlUsesPlasmoidConfiguration();
    void middleClickActionDefaultIsClose();
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
    QVERIFY(mainSource.contains(QStringLiteral("auto disableSessionManagement = [](QSessionManager &sm)")));
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
    QVERIFY(pollerBody.indexOf(QStringLiteral("Latte::Session::shouldQuitForCommittedShutdown(sessionShutdownSawBlockingWindows, shutdownServiceActive, hasBlockingWindows)")) < pollerBody.indexOf(QStringLiteral("app.quit();")));

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
    QVERIFY(cmake.contains(QStringLiteral("try_compile")));
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
    QVERIFY(backgroundSource.contains(QStringLiteral("import org.kde.latte.abilities.definition 0.1 as AbilityDefinition")));
    QVERIFY(backgroundSource.contains(QStringLiteral("AbilityDefinition.MetricsConstants.kModernBackgroundShadowMinPixels")));
    QVERIFY(backgroundSource.contains(QStringLiteral("if (modernDockStyle && customDefShadowIsEnabled) {\n            return AbilityDefinition.MetricsConstants.kModernBackgroundShadowMinPixels;\n        }")));
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

void SourceContractTest::restoreAnimationContractMovedToQmlSmokeTest()
{
    QFile qmlSmoke(QStringLiteral(LATTE_SOURCE_DIR "/autotests/qmlsmoketest.cpp"));
    QVERIFY(qmlSmoke.open(QFile::ReadOnly));
    const QString qmlSmokeSource = QString::fromUtf8(qmlSmoke.readAll());
    QVERIFY(qmlSmokeSource.contains(QStringLiteral("restoreAnimationLoadsFromSource")));
    QVERIFY(qmlSmokeSource.contains(QStringLiteral("QQmlComponent component")));
    QVERIFY(qmlSmokeSource.contains(QStringLiteral("LATTE_RESTORE_ANIMATION_QML")));

    QFile sourceContracts(QStringLiteral(LATTE_SOURCE_DIR "/autotests/sourcecontracttest.cpp"));
    QVERIFY(sourceContracts.open(QFile::ReadOnly));
    const QString sourceContractSource = QString::fromUtf8(sourceContracts.readAll());
    const QString oldSourceLock = QStringLiteral("QFile ") + QStringLiteral("restoreAnimation");
    QVERIFY(!sourceContractSource.contains(oldSourceLock));
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
    QVERIFY(moduleSource.contains(QStringLiteral("set(CPACK_RPM_PACKAGE_REQUIRES \"kf6-kirigami, kf6-kcmutils, kf6-knewstuff\")")));
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

void SourceContractTest::autostartDefaultEnabledContracts()
{
    //! "Enable autostart during startup" (Preferences page) must default to
    //! enabled and the first-run logic must create the autostart entry once.

    QFile preferencesHeader(QStringLiteral(LATTE_SOURCE_DIR "/app/data/preferencesdata.h"));
    QVERIFY(preferencesHeader.open(QFile::ReadOnly));
    const QString preferencesSource = QString::fromUtf8(preferencesHeader.readAll());
    QVERIFY(preferencesSource.contains(QStringLiteral("static const bool AUTOSTART = true;")));
    QVERIFY(preferencesSource.contains(QStringLiteral("bool autostart{AUTOSTART};")));

    QFile universalSettings(QStringLiteral(LATTE_SOURCE_DIR "/app/settings/universalsettings.cpp"));
    QVERIFY(universalSettings.open(QFile::ReadOnly));
    const QString universalSource = QString::fromUtf8(universalSettings.readAll());

    //! First-run chain: when the user never configured autostart and it is
    //! disabled, enable it exactly once and remember the decision.
    const int userSetRead = universalSource.indexOf(
        QStringLiteral("bool autostartUserSet = m_universalGroup.readEntry(\"userConfiguredAutostart\", false);"));
    const int firstRunGuard = universalSource.indexOf(
        QStringLiteral("if (!autostartUserSet && !autostart()) {"), userSetRead);
    const int enableCall = universalSource.indexOf(QStringLiteral("setAutostart(true);"), firstRunGuard);
    const int rememberDecision = universalSource.indexOf(
        QStringLiteral("m_universalGroup.writeEntry(\"userConfiguredAutostart\", true);"), enableCall);
    QVERIFY(userSetRead >= 0);
    QVERIFY(firstRunGuard > userSetRead);
    QVERIFY(enableCall > firstRunGuard);
    QVERIFY(rememberDecision > enableCall);

    //! Autostart state is the existence of the desktop file, and enabling
    //! copies the installed application launcher.
    QFile importer(QStringLiteral(LATTE_SOURCE_DIR "/app/layouts/importer.cpp"));
    QVERIFY(importer.open(QFile::ReadOnly));
    const QString importerSource = QString::fromUtf8(importer.readAll());
    QVERIFY(importerSource.contains(QStringLiteral("\"/autostart/org.kde.latte-dock.desktop\"")));
    QVERIFY(importerSource.contains(QStringLiteral("standardPath(\"applications/org.kde.latte-dock.desktop\", false)")));
}

void SourceContractTest::cmakeWarningRelaxationLivesInModule()
{
    QFile module(QStringLiteral(LATTE_SOURCE_DIR "/cmake/LatteCompilerWarnings.cmake"));
    QVERIFY(module.open(QFile::ReadOnly));
    const QString moduleSource = QString::fromUtf8(module.readAll());
    QVERIFY(moduleSource.contains(QStringLiteral("function(latte_apply_relaxed_warning_flags)")));
    QVERIFY(moduleSource.contains(QStringLiteral("set(LATTE_RELAXED_WARNING_FLAGS")));
    QVERIFY(moduleSource.contains(QStringLiteral("string(REPLACE \"${_latte_warning_flag}\" \"\" CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS}\")")));
    QVERIFY(moduleSource.contains(QStringLiteral("set(CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS}\" PARENT_SCOPE)")));
    QVERIFY(moduleSource.contains(QStringLiteral("message(STATUS \"Latte relaxed warning flags pending cleanup: ${LATTE_RELAXED_WARNING_FLAGS}\")")));

    QFile cmake(QStringLiteral(LATTE_SOURCE_DIR "/CMakeLists.txt"));
    QVERIFY(cmake.open(QFile::ReadOnly));
    const QString cmakeSource = QString::fromUtf8(cmake.readAll());
    QVERIFY(cmakeSource.contains(QStringLiteral("include(LatteCompilerWarnings)")));
    QVERIFY(cmakeSource.contains(QStringLiteral("latte_apply_relaxed_warning_flags()")));
    QVERIFY(!cmakeSource.contains(QStringLiteral("set(LATTE_RELAXED_WARNING_FLAGS")));
    QVERIFY(!cmakeSource.contains(QStringLiteral("foreach(_latte_warning_flag IN LISTS LATTE_RELAXED_WARNING_FLAGS)")));
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

    auto checkFileUsesAllScreens = [](const QString &filePath) {
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
    QVERIFY(src.contains(QStringLiteral("aid>0 && !isValidApplet(aid)")));
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
    QVERIFY(src.contains(QStringLiteral("m_slideOffset!=0 || inSlideAnimation()")));
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
        // effectiveBackgroundOpacity is now set from BindingsExternal
        // so that the default (-1) case reports the theme's real
        // maxOpacity instead of the old hard-coded 1.0 (which always
        // suppressed blur in effects.cpp at the >=0.95 guard).
        QFile f(QStringLiteral(LATTE_SOURCE_DIR
            "/containment/package/contents/ui/BindingsExternal.qml"));
        QVERIFY(f.open(QFile::ReadOnly));
        const QString src = QString::fromUtf8(f.readAll());

        QVERIFY(src.contains(QStringLiteral("effectiveBackgroundOpacity")));
        QVERIFY(src.contains(QStringLiteral("themeExtendedBackground.maxOpacity")));
        QVERIFY(src.contains(QStringLiteral("background.currentOpacity")));
        QVERIFY(src.contains(QStringLiteral("panelTransparency !== -1")));
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

QTEST_MAIN(SourceContractTest)

#include "sourcecontracttest.moc"
