/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QFile>
#include <QTest>

class SourceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pulseAudioBootstrapIsBounded();
    void compactAppletUsesLargerDefaultForVolumePopup();
    void compactAppletKeepsApplicationMenuPopupResizable();
    void applicationLauncherUsesFixedExternalSlot();
    void latteTasksExposesPlasmaLauncherApi();
    void latteDockDbusExportsLauncherApi();
    void containmentClearsParabolicStateWhenEdgeChanges();
    void taskWindowDoesNotKeepStaleFrozenZoom();
    void parabolicAnimationRecoveryKeepsZoomStateBounded();
    void launcherRestoreCoversGeometryTransitionSettling();
    void sessionShutdownHandlingMatchesStableWaylandPath();
    void itemsAlignmentIsSeparateAndJustifyOnly();
    void itemsAlignmentNormalizesDirectionsByFormFactor();
    void itemsAlignmentConfigDefaultsToCenter();
    void appearancePaletteExposesLayoutCustomColors();
    void layoutDetailsExposeCustomColorSchemeSelector();
    void autotestAggregateTargetDocumentsFullSuiteBuild();
};

void SourceContractTest::pulseAudioBootstrapIsBounded()
{
    QFile pulseAudio(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/PulseAudio.qml"));
    QVERIFY(pulseAudio.open(QFile::ReadOnly));

    // Regression lock: the PulseAudio refresh must be a bounded startup
    // bootstrap, not a long-running periodic fix timer.
    const QString source = QString::fromUtf8(pulseAudio.readAll());
    QVERIFY(source.contains(QStringLiteral("bootstrapAttempts")));
    QVERIFY(source.contains(QStringLiteral("bootstrapMaxAttempts")));
    QVERIFY(source.contains(QStringLiteral("paFixTimer.stop()")));
    QVERIFY(!source.contains(QStringLiteral("interval = 30000")));
}

void SourceContractTest::compactAppletUsesLargerDefaultForVolumePopup()
{
    QFile compactApplet(QStringLiteral(LATTE_SOURCE_DIR "/shell/package/contents/applet/CompactApplet.qml"));
    QVERIFY(compactApplet.open(QFile::ReadOnly));

    // Regression lock: the Plasma volume applet needs Latte's wrapper to keep
    // a Plasma-panel-like initial popup size instead of reusing stale tiny
    // persisted popup dimensions.
    const QString source = QString::fromUtf8(compactApplet.readAll());
    QVERIFY(source.contains(QStringLiteral("function popupPreferredWidth")));
    QVERIFY(source.contains(QStringLiteral("function popupPreferredHeight")));
    QVERIFY(source.contains(QStringLiteral("org.kde.plasma.volume")));
    QVERIFY(source.contains(QStringLiteral("Kirigami.Units.gridUnit * 27")));
}

void SourceContractTest::compactAppletKeepsApplicationMenuPopupResizable()
{
    QFile compactApplet(QStringLiteral(LATTE_SOURCE_DIR "/shell/package/contents/applet/CompactApplet.qml"));
    QVERIFY(compactApplet.open(QFile::ReadOnly));

    // Regression lock: application menu popups should keep their preferred
    // initial size while advertising a smaller minimum and unbounded maximum
    // so the user can resize both width and height.
    const QString source = QString::fromUtf8(compactApplet.readAll());
    QVERIFY(source.contains(QStringLiteral("function popupMenuMinimumWidth")));
    QVERIFY(source.contains(QStringLiteral("function popupMenuMinimumHeight")));
    QVERIFY(source.contains(QStringLiteral("function popupMaximumWidth")));
    QVERIFY(source.contains(QStringLiteral("function popupMaximumHeight")));
    QVERIFY(source.contains(QStringLiteral("org.kde.plasma.kicker")));
    QVERIFY(source.contains(QStringLiteral("isApplicationMenuApplet()")));
    QVERIFY(source.contains(QStringLiteral("Kirigami.Units.gridUnit * 18")));
    QVERIFY(source.contains(QStringLiteral("return Infinity;")));
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

void SourceContractTest::containmentClearsParabolicStateWhenEdgeChanges()
{
    QFile containment(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/main.qml"));
    QVERIFY(containment.open(QFile::ReadOnly));

    const QString source = QString::fromUtf8(containment.readAll());
    QVERIFY(source.contains(QStringLiteral("function onLocationChanged() {\n            root.resetModernParabolicOffsets();")));
    QVERIFY(source.contains(QStringLiteral("function onFormFactorChanged() {\n            root.resetModernParabolicOffsets();")));
    QVERIFY(source.contains(QStringLiteral("function onShowingAfterRelocationFinished() {\n            root.resetModernParabolicOffsets();")));
}

void SourceContractTest::taskWindowDoesNotKeepStaleFrozenZoom()
{
    QFile showWindowAnimation(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/task/animations/ShowWindowAnimation.qml"));
    QVERIFY(showWindowAnimation.open(QFile::ReadOnly));

    const QString source = QString::fromUtf8(showWindowAnimation.readAll());
    QVERIFY(source.contains(QStringLiteral("function keepFrozenZoomForCurrentTask()")));
    QVERIFY(source.contains(QStringLiteral("taskItem.parabolicAreaIsCurrent || taskItem.parabolicAreaContainsMouse")));
    QVERIFY(source.contains(QStringLiteral("taskItem.parabolicItem.zoom = keepFrozenZoomForCurrentTask() ? frozenTask.zoom : 1;")));
}

void SourceContractTest::parabolicAnimationRecoveryKeepsZoomStateBounded()
{
    QFile restoreAnimation(QStringLiteral(LATTE_SOURCE_DIR "/declarativeimports/abilities/items/basicitem/RestoreAnimation.qml"));
    QVERIFY(restoreAnimation.open(QFile::ReadOnly));

    const QString restoreSource = QString::fromUtf8(restoreAnimation.readAll());
    QVERIFY(restoreSource.contains(QStringLiteral("target: abilityItem.parabolicItem")));
    QVERIFY(restoreSource.contains(QStringLiteral("property: \"zoom\"")));
    QVERIFY(restoreSource.contains(QStringLiteral("to: 1")));

    QFile parabolicItem(QStringLiteral(LATTE_SOURCE_DIR "/declarativeimports/abilities/items/basicitem/ParabolicItem.qml"));
    QVERIFY(parabolicItem.open(QFile::ReadOnly));

    const QString parabolicSource = QString::fromUtf8(parabolicItem.readAll());
    QVERIFY(parabolicSource.contains(QStringLiteral("function onFormFactorChanged()")));
    QVERIFY(parabolicSource.contains(QStringLiteral("parabolicItem.zoom = 1.01")));
    QVERIFY(parabolicSource.contains(QStringLiteral("parabolicItem.zoom = 1")));
    QVERIFY(parabolicSource.contains(QStringLiteral("parabolicItem.sendEndOfNeedBothAxisAnimation();")));
}

void SourceContractTest::launcherRestoreCoversGeometryTransitionSettling()
{
    QFile launchers(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/abilities/Launchers.qml"));
    QVERIFY(launchers.open(QFile::ReadOnly));

    const QString source = QString::fromUtf8(launchers.readAll());
    QVERIFY(source.contains(QStringLiteral("function scheduleLaunchersRestore(reason)")));
    QVERIFY(source.contains(QStringLiteral("launchersRestoreTimer.restart()")));
    QVERIFY(source.contains(QStringLiteral("launchersRestoreFollowUpTimer.restart()")));
    QVERIFY(source.contains(QStringLiteral("launchersRestoreFinalTimer.restart()")));
    QVERIFY(source.contains(QStringLiteral("_launchers.restoreLaunchersFromConfig(_launchers._pendingRestoreReason + \":primary\")")));
    QVERIFY(source.contains(QStringLiteral("_launchers.restoreLaunchersFromConfig(_launchers._pendingRestoreReason + \":followup\")")));
    QVERIFY(source.contains(QStringLiteral("_launchers.restoreLaunchersFromConfig(_launchers._pendingRestoreReason + \":final\")")));
}

void SourceContractTest::sessionShutdownHandlingMatchesStableWaylandPath()
{
    QFile mainSourceFile(QStringLiteral(LATTE_SOURCE_DIR "/app/main.cpp"));
    QVERIFY(mainSourceFile.open(QFile::ReadOnly));
    const QString mainSource = QString::fromUtf8(mainSourceFile.readAll());

    QVERIFY(mainSource.contains(QStringLiteral("KSignalHandler::self()->watchSignal(SIGTERM);")));
    QVERIFY(mainSource.contains(QStringLiteral("KSignalHandler::self()->watchSignal(SIGINT);")));
    QVERIFY(mainSource.contains(QStringLiteral("KSignalHandler::self()->watchSignal(SIGHUP);")));
    QVERIFY(mainSource.contains(QStringLiteral("QCoreApplication::setQuitLockEnabled(false);")));
    QVERIFY(mainSource.contains(QStringLiteral("qputenv(\"LATTE_SESSION_ENDING\", \"1\");")));
    QVERIFY(mainSource.contains(QStringLiteral("app.setProperty(\"latte_session_ending\", true);")));
    QVERIFY(mainSource.contains(QStringLiteral("QObject::connect(&app, &QGuiApplication::commitDataRequest")));
    QVERIFY(mainSource.contains(QStringLiteral("sm.setRestartHint(QSessionManager::RestartNever);")));
    QVERIFY(mainSource.contains(QStringLiteral("QObject::connect(&app, &QGuiApplication::saveStateRequest")));
    QVERIFY(mainSource.contains(QStringLiteral("sessionShutdownPoll.setInterval(500);")));
    QVERIFY(mainSource.contains(QStringLiteral("flagSetTimer.hasExpired(5000)")));
    QVERIFY(mainSource.contains(QStringLiteral("qunsetenv(\"LATTE_SESSION_ENDING\");")));

    const int saveStateStart = mainSource.indexOf(QStringLiteral("QObject::connect(&app, &QGuiApplication::saveStateRequest"));
    const int pollerStart = mainSource.indexOf(QStringLiteral("QTimer sessionShutdownPoll"));
    QVERIFY(saveStateStart >= 0);
    QVERIFY(pollerStart > saveStateStart);
    const QString saveStateBlock = mainSource.mid(saveStateStart, pollerStart - saveStateStart);
    QVERIFY(saveStateBlock.contains(QStringLiteral("RestartNever")));
    QVERIFY(!saveStateBlock.contains(QStringLiteral("app.quit()")));

    QFile coronaSourceFile(QStringLiteral(LATTE_SOURCE_DIR "/app/lattecorona.cpp"));
    QVERIFY(coronaSourceFile.open(QFile::ReadOnly));
    const QString coronaSource = QString::fromUtf8(coronaSourceFile.readAll());
    QVERIFY(coronaSource.contains(QStringLiteral("qEnvironmentVariableIntValue(\"LATTE_SESSION_ENDING\") == 1")));
    QVERIFY(coronaSource.contains(QStringLiteral("qApp->property(\"latte_session_ending\").toBool()")));
    QVERIFY(coronaSource.contains(QStringLiteral("m_layoutsManager->synchronizer()->hideAllViews();")));
    QVERIFY(coronaSource.contains(QStringLiteral("fast shutdown path for session logout")));
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

QTEST_MAIN(SourceContractTest)

#include "sourcecontracttest.moc"
