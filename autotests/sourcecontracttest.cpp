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
    void cmakeWarningRelaxationLivesInModule();
    void cmakeFindsQtCoreToolsBeforeKdeInstallDirs();
    void qtQuickGpuPreferenceKeepsSoftwareFallbackAvailable();
    void knsCompatImportsAreAvailableForSystemInstall();
    void taskIconsRefreshAfterIconThemeChanges();
    void taskAudioBadgesScaleWithParabolicZoom();
    void widgetExplorerLaunchesKnsDialogOutOfProcess();
    void widgetExplorerUsesPlasmaTranslationContexts();
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
    QVERIFY(mainSource.contains(QStringLiteral("KSignalHandler::self()->watchSignal(SIGINT);")));
    QVERIFY(mainSource.contains(QStringLiteral("KSignalHandler::self()->watchSignal(SIGHUP);")));
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

    const int sharedEngineDetach = mainSource.indexOf(QStringLiteral("s_sharedEngine->setParent(nullptr);"));
    const int postExecDeferredDeleteFlush = mainSource.indexOf(QStringLiteral("QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);"), sharedEngineDetach);
    const int sharedEngineReset = mainSource.indexOf(QStringLiteral("s_sharedEngine.reset();"), sharedEngineDetach);
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

    QVERIFY(mainSource.contains(QStringLiteral("inline void ensureKnsCompatQmlImportPaths();")));
    QVERIFY(mainSource.contains(QStringLiteral("void ensureKnsCompatQmlImportPaths()")));
    QVERIFY(mainSource.contains(QStringLiteral("knsCompatUserQmlRoot()")));
    QVERIFY(mainSource.contains(QStringLiteral("prependEnvironmentPath(\"QML2_IMPORT_PATH\"")));
    QVERIFY(mainSource.contains(QStringLiteral("prependEnvironmentPath(\"QML_IMPORT_PATH\"")));
    QVERIFY(mainSource.contains(QStringLiteral("prependEnvironmentPath(\"QT_QML_IMPORT_PATH\"")));

    const int appCreation = mainSource.indexOf(QStringLiteral("QApplication app(argc, argv);"));
    QVERIFY(appCreation >= 0);
    const int ensureCompatCall = mainSource.indexOf(QStringLiteral("ensureKnsCompat();"), appCreation);
    const int ensureImportPathCall = mainSource.indexOf(QStringLiteral("ensureKnsCompatQmlImportPaths();"), appCreation);
    const int sharedEngineCreation = mainSource.indexOf(QStringLiteral("std::make_shared<PlasmaQuick::SharedQmlEngine>(&app);"), appCreation);
    QVERIFY(ensureCompatCall > appCreation);
    QVERIFY(ensureImportPathCall > ensureCompatCall);
    QVERIFY(sharedEngineCreation > ensureImportPathCall);

    QFile compatHeader(QStringLiteral(LATTE_SOURCE_DIR "/app/knscompat.h"));
    QVERIFY(compatHeader.open(QFile::ReadOnly));
    const QString compatHeaderSource = QString::fromUtf8(compatHeader.readAll());
    QVERIFY(compatHeaderSource.contains(QStringLiteral("QString knsCompatUserQmlRoot();")));
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
    QVERIFY(source.contains(QStringLiteral("readonly property real parabolicZoom: taskItem.parabolicItem.zoom")));
    QVERIFY(source.contains(QStringLiteral("readonly property real maximumBadgeSize: Kirigami.Units.iconSizes.smallMedium * parabolicZoom")));
    QVERIFY(source.contains(QStringLiteral("compactBadgeSize: Math.min(iconBoxSize * 0.4, maximumBadgeSize)")));
    QVERIFY(source.contains(QStringLiteral("Math.min(parent.height * audioStreamIconBox.indicatorScale, audioStreamIconBox.maximumBadgeSize)")));

    const int zoomProperty = source.indexOf(QStringLiteral("readonly property real parabolicZoom"));
    const int maximumSize = source.indexOf(QStringLiteral("readonly property real maximumBadgeSize"), zoomProperty);
    const int compactSize = source.indexOf(QStringLiteral("compactBadgeSize: Math.min(iconBoxSize * 0.4, maximumBadgeSize)"), maximumSize);
    QVERIFY(zoomProperty >= 0);
    QVERIFY(maximumSize > zoomProperty);
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
    QVERIFY(cmakeSource.contains(QStringLiteral("set_tests_properties(${_test_name} PROPERTIES ENVIRONMENT \"QT_QPA_PLATFORM=offscreen\")")));
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

QTEST_MAIN(SourceContractTest)

#include "sourcecontracttest.moc"
