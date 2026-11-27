/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QQmlComponent>
#include <QQmlContext>
#include <QJSValue>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QWheelEvent>
#include <QtQml/qqml.h>

#include <QColor>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMetaObject>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTest>

#include <cmath>
#include <memory>
#include <utility>

//! Plasma 6.5+ ships org.kde.plasma.plasmoid as a filesystem QML module.
//! On Plasma 6.3 the module only exists inside a shell context, registered
//! lazily by PlasmaQuick::AppletQuickItem, so a bare engine used by these
//! smoke tests cannot resolve the import. Register an empty module so the
//! import parses; the QML under test reads the "plasmoid" object from its
//! context property, so no types from the module are required. The
//! registration is skipped whenever the real module resolves, keeping the
//! Plasma 6.5+ behavior untouched.
//!
//! Do not "improve" this by registering a stub Plasmoid attached type via
//! qmlRegisterExtendedType: it breaks "Behavior" type resolution in the
//! Qt 6.8 QML compiler (ParabolicItem.qml fails with "Behavior cannot
//! operate on zoom") while the context property already covers the
//! plasmoid lookups.
static void ensurePlasmaPlasmoidModuleAvailable()
{
    static bool checked{false};

    if (checked) {
        return;
    }

    checked = true;

    //! Probe whether a bare engine can resolve the real module.
    QQmlEngine probeEngine;
    QQmlComponent probe(&probeEngine);
    probe.setData(QByteArrayLiteral("import org.kde.plasma.plasmoid 2.0\nimport QtQml 2.15\nQtObject {}"),
                  QUrl(QStringLiteral("probe.qml")));

    if (probe.status() == QQmlComponent::Ready) {
        return;
    }

    qmlRegisterModule("org.kde.plasma.plasmoid", 2, 0);
}

class QmlSmokeTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void colorButtonSeparatesSwatchAndText_data();
    void colorButtonSeparatesSwatchAndText();
    void backgroundColorScheduleFollowsLocalTime();
    void indicatorShapeFollowsSavedOption_data();
    void indicatorShapeFollowsSavedOption();
    void taskIconRasterSizeStaysStableDuringZoom();
    void minimizedSectionTracksGroupedWindows();
    void containerWheelPreservesTaskCyclingWithOverflow();
    void containmentWheelScrollsTaskViewport();
    void editModeBlockerLeavesRightClicksToContextMenu();
    void activeTaskScrollsIntoView();
    void edgeHoverScrollsWithStationaryPointer_data();
    void edgeHoverScrollsWithStationaryPointer();
    void scrollingTasksCanRequestShorterViewports();
    void scrollingTasksKeepFillRequestsInModernDocks();
    void scrollingTasksShareRemainingModernDockLength();
    void appletIndexFollowsItsLayoutWithoutTransientInvalidation();
    void latteCoreQmlPluginLoadsFromBuildTree();
    void layoutInfoWindowHasAValidSizeFromSource();
    void appletAlternativesDialogResolvesOutsidePlasmashell();
    void showWindowAnimationFrozenZoomDecisionLoadsFromSource();
    void newTaskPublishesSlotAfterEntranceSetup();
    void launcherPositionPendingCoversDelayedMove();
    void positionedSyncedLauncherRequestsDelayedMove();
    void parabolicItemZoomRecoveryLoadsFromSource();
    void hiddenSpacerRetainsSlotWhileSeparatorStateSettles();
    void taskRemovalDefersGeometryFeedback_data();
    void taskRemovalDefersGeometryFeedback();
    void taskRowReplacementHandoffUsesOutgoingGeometry();
    void taskRowReplacementHandoffPreservesListViewSlot();
    void compactAppletPopupSizingLoadsFromSource();
    void globalMenuBackendStatusTransitionsLoadFromSource();
    void globalMenuBackendStatusTransitionsLoadFromSource_data();
    void globalMenuNativeLengthUpdatesWithoutClassifier();
    void globalMenuReflowsNeighboursSynchronously();
    void launchersGeometryRestoreSchedulingLoadsFromSource();
    void peerLauncherOrderStartsValidationWithoutRetryDelay();
    void plasmaVolumeBootstrapLoadsFromSource();
    void audioStreamsRequireUnambiguousOwnership();
    void audioBadgesTrackPauseRatherThanConnectionLifetime();
    void audioOwnershipTracksModelAndMetadataChanges();
    void taskMouseAreaAllClickActionsPresent();
    void taskPresentationFiltersPhantoms();
    void taskMouseAreaRegressionGuardsPresent();
    void taskMouseAreaStructuralGuardsPresent();
    void taskRightClickReachesDockMenuInEditMode();
    void tasksConfigComboModelsComplete();
    void tasksConfigCfgPropertyAndNoStaleTasksConfig();
    void tasksConfigClickActionModelOrderMatchesEnum();
    void configInteractionHoverActionNotHardcoded();
    void typesTaskActionEnumCompleteness();
    void parabolicAreaMouseAreaStructuralGuardsPresent();
    void zoomRestoreDelegatedToBehaviorAnimation();
};

class ParabolicTargetStub : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)

public:
    qreal zoom() const {
        return m_zoom;
    }

    void setZoom(qreal zoom) {
        if (qFuzzyCompare(m_zoom, zoom)) {
            return;
        }

        m_zoom = zoom;
        Q_EMIT zoomChanged();
    }

Q_SIGNALS:
    void zoomChanged();

private:
    qreal m_zoom{1.5};
};

class EventSinkStub : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE void addEvent(const QString &) {
        ++m_addCount;
    }

    Q_INVOKABLE void removeEvent(const QString &) {
        ++m_removeCount;
    }

    int addCount() const {
        return m_addCount;
    }

    int removeCount() const {
        return m_removeCount;
    }

private:
    int m_addCount{0};
    int m_removeCount{0};
};

class TaskRowLauncherSignalsStub : public QObject
{
    Q_OBJECT

Q_SIGNALS:
    void launcherInRemoving(const QString &launcherUrl);
    void launcherInAdding(const QString &launcherUrl);
    void launcherInMoving(const QString &launcherUrl, int position);
};

class TaskRowAppletAbilitiesStub : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *launchers READ launchers CONSTANT)

public:
    QObject *launchers() {
        return &m_launchers;
    }

private:
    TaskRowLauncherSignalsStub m_launchers;
};

class ParabolicAbilityStub : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap factor READ factor CONSTANT)
    Q_PROPERTY(bool directRenderingEnabled READ directRenderingEnabled WRITE setDirectRenderingEnabled NOTIFY directRenderingEnabledChanged)
    Q_PROPERTY(bool isEnabled READ isEnabled CONSTANT)

public:
    QVariantMap factor() const {
        return QVariantMap{
            {QStringLiteral("zoom"), 1.6},
            {QStringLiteral("marginThicknessZoomInPercentage"), 0.0},
        };
    }

    bool directRenderingEnabled() const {
        return m_directRenderingEnabled;
    }

    void setDirectRenderingEnabled(bool enabled) {
        if (m_directRenderingEnabled == enabled) {
            return;
        }

        m_directRenderingEnabled = enabled;
        Q_EMIT directRenderingEnabledChanged();
    }

    bool isEnabled() const {
        return true;
    }

Q_SIGNALS:
    void directRenderingEnabledChanged();

private:
    bool m_directRenderingEnabled{true};
};

class AbilityItemStub : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *parabolicItem READ parabolicItem CONSTANT)
    Q_PROPERTY(QVariantMap abilities READ abilities CONSTANT)
    Q_PROPERTY(int animationTime READ animationTime CONSTANT)
    Q_PROPERTY(bool isHorizontal READ isHorizontal CONSTANT)
    Q_PROPERTY(bool isVertical READ isVertical CONSTANT)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(bool isSeparator READ isSeparator CONSTANT)
    Q_PROPERTY(bool isHidden READ isHidden CONSTANT)
    Q_PROPERTY(int location READ location CONSTANT)
    Q_PROPERTY(qreal iconOffsetX READ iconOffsetX CONSTANT)
    Q_PROPERTY(qreal iconOffsetY READ iconOffsetY CONSTANT)
    Q_PROPERTY(int iconTransformOrigin READ iconTransformOrigin CONSTANT)
    Q_PROPERTY(qreal iconOpacity READ iconOpacity CONSTANT)
    Q_PROPERTY(qreal iconRotation READ iconRotation CONSTANT)
    Q_PROPERTY(qreal iconScale READ iconScale CONSTANT)
    Q_PROPERTY(QObject *contentItem READ contentItem NOTIFY contentItemChanged)
    Q_PROPERTY(bool isMonochromaticForcedContentItem READ isMonochromaticForcedContentItem CONSTANT)
    Q_PROPERTY(QObject *monochromizedItem READ monochromizedItem CONSTANT)
    Q_PROPERTY(int itemIndex READ itemIndex CONSTANT)
    Q_PROPERTY(bool parabolicAreaContainsMouse READ parabolicAreaContainsMouse CONSTANT)

public:
    explicit AbilityItemStub(QObject *parabolicItem, QObject *parent = nullptr, bool horizontal = true, bool separator = false)
        : QObject(parent)
        , m_parabolicItem(parabolicItem)
        , m_horizontal(horizontal)
        , m_separator(separator) {
    }

    QObject *parabolicItem() const {
        return m_parabolicItem;
    }

    QVariantMap abilities() const {
        return QVariantMap{
            {
                QStringLiteral("metrics"),
                QVariantMap{
                    {QStringLiteral("iconSize"), 48},
                    {
                        QStringLiteral("mask"),
                        QVariantMap{{QStringLiteral("thickness"), QVariantMap{{QStringLiteral("normalForItems"), 48}, {QStringLiteral("zoomedForItems"), 64}}}}
                    },
                    {QStringLiteral("margin"), QVariantMap{{QStringLiteral("screenEdge"), 0}, {QStringLiteral("tailThickness"), 0}}},
                    {QStringLiteral("marginsArea"), QVariantMap{{QStringLiteral("iconSize"), 48}, {QStringLiteral("tailThickness"), 0}}},
                    {
                        QStringLiteral("totals"),
                        QVariantMap{{QStringLiteral("length"), 48}, {QStringLiteral("thicknessEdges"), 0}, {QStringLiteral("lengthPaddings"), 0}}
                    },
                }
            },
            {QStringLiteral("parabolic"), QVariant::fromValue(static_cast<QObject *>(const_cast<ParabolicAbilityStub *>(&m_parabolic)))},
            {
                QStringLiteral("animations"),
                QVariantMap{{QStringLiteral("needBothAxis"), QVariant::fromValue(static_cast<QObject *>(const_cast<EventSinkStub *>(&m_needBothAxis)))}}
            },
            {
                QStringLiteral("myView"),
                QVariantMap{{QStringLiteral("itemShadow"), QVariantMap{{QStringLiteral("isEnabled"), false}, {QStringLiteral("shadowColor"), QColor(Qt::black)}}},
                    {QStringLiteral("badgesIn3DStyle"), false}}
            },
            {QStringLiteral("environment"), QVariantMap{{QStringLiteral("isGraphicsSystemAccelerated"), false}}},
            {QStringLiteral("indexer"), QVariantMap{{QStringLiteral("inMarginsArea"), false}}},
            {QStringLiteral("debug"), QVariantMap{{QStringLiteral("graphicsEnabled"), false}}},
            {
                QStringLiteral("shortcuts"),
                QVariantMap{{QStringLiteral("showPositionShortcutBadges"), false},
                    {QStringLiteral("isEnabled"), false},
                    {QStringLiteral("badges"), QStringList{}},
                    {QStringLiteral("shortcutIndex"), QVariant::fromValue(static_cast<QObject *>(const_cast<AbilityItemStub *>(this)))}}
            },
        };
    }

    int animationTime() const {
        return 10;
    }

    bool isHorizontal() const {
        return m_horizontal;
    }

    bool isVertical() const {
        return !m_horizontal;
    }

    bool isVisible() const {
        return m_visible;
    }

    void setVisible(bool visible) {
        if (m_visible == visible) {
            return;
        }

        m_visible = visible;
        Q_EMIT visibleChanged();
    }

    bool isSeparator() const {
        return m_separator;
    }

    bool isHidden() const {
        return false;
    }

    int location() const {
        return 4;
    }

    qreal iconOffsetX() const {
        return 0;
    }

    qreal iconOffsetY() const {
        return 0;
    }

    int iconTransformOrigin() const {
        return 4;
    }

    qreal iconOpacity() const {
        return 1.0;
    }

    qreal iconRotation() const {
        return 0;
    }

    qreal iconScale() const {
        return 1.0;
    }

    QObject *contentItem() const {
        return nullptr;
    }

    bool isMonochromaticForcedContentItem() const {
        return false;
    }

    QObject *monochromizedItem() const {
        return nullptr;
    }

    int itemIndex() const {
        return 1;
    }

    bool parabolicAreaContainsMouse() const {
        return false;
    }

    Q_INVOKABLE int shortcutIndex(int) const {
        return -1;
    }

    int needBothAxisAddCount() const {
        return m_needBothAxis.addCount();
    }

    int needBothAxisRemoveCount() const {
        return m_needBothAxis.removeCount();
    }

Q_SIGNALS:
    void visibleChanged();
    void contentItemChanged();
    void parabolicAreaLastMousePosChanged();
    void itemIndexChanged();

private:
    QObject *m_parabolicItem{nullptr};
    bool m_horizontal{true};
    bool m_separator{false};
    bool m_visible{true};
    ParabolicAbilityStub m_parabolic;
    EventSinkStub m_needBothAxis;
};

class PlasmoidStub : public QObject
{
    Q_OBJECT

public:
    void emitFormFactorChanged() {
        Q_EMIT formFactorChanged();
    }

Q_SIGNALS:
    void formFactorChanged();
};

class LaunchersConfigurationStub : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList launchers59 READ launchers59 WRITE setLaunchers59 NOTIFY launchers59Changed)
    Q_PROPERTY(bool userConfiguring READ userConfiguring WRITE setUserConfiguring NOTIFY userConfiguringChanged)

public:
    explicit LaunchersConfigurationStub(QStringList launchers, QObject *parent = nullptr)
        : QObject(parent)
        , m_launchers(std::move(launchers)) {
    }

    QStringList launchers59() const {
        return m_launchers;
    }

    void setLaunchers59(const QStringList &launchers) {
        m_launchers = launchers;
        Q_EMIT launchers59Changed();
    }

    bool userConfiguring() const {
        return m_userConfiguring;
    }

    void setUserConfiguring(bool userConfiguring) {
        if (m_userConfiguring == userConfiguring) {
            return;
        }

        m_userConfiguring = userConfiguring;
        Q_EMIT userConfiguringChanged();
    }

Q_SIGNALS:
    void launchers59Changed();
    void userConfiguringChanged();

private:
    QStringList m_launchers;
    bool m_userConfiguring{false};
};

class LaunchersPlasmoidStub : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int id READ id CONSTANT)
    Q_PROPERTY(QObject *configuration READ configuration CONSTANT)

public:
    explicit LaunchersPlasmoidStub(QStringList launchers, QObject *parent = nullptr)
        : QObject(parent)
        , m_configuration(std::move(launchers), this) {
    }

    int id() const {
        return 1;
    }

    QObject *configuration() {
        return &m_configuration;
    }

Q_SIGNALS:
    void locationChanged();
    void formFactorChanged();
    void userConfiguringChanged();

private:
    LaunchersConfigurationStub m_configuration;
};

class MyViewReadyStub : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isReady READ isReady WRITE setReady NOTIFY isReadyChanged)

public:
    bool isReady() const {
        return m_ready;
    }

    void setReady(bool ready) {
        if (m_ready == ready) {
            return;
        }

        m_ready = ready;
        Q_EMIT isReadyChanged();
    }

Q_SIGNALS:
    void isReadyChanged();

private:
    bool m_ready{true};
};

class AppletAbilitiesStub : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *myView READ myView CONSTANT)

public:
    QObject *myView() {
        return &m_myView;
    }

private:
    MyViewReadyStub m_myView;
};

class TaskItemStub : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *parabolicItem READ parabolicItem CONSTANT)
    Q_PROPERTY(QVariantMap abilities READ abilities CONSTANT)
    Q_PROPERTY(bool parabolicAreaIsCurrent READ parabolicAreaIsCurrent WRITE setParabolicAreaIsCurrent NOTIFY parabolicAreaChanged)
    Q_PROPERTY(bool parabolicAreaContainsMouse READ parabolicAreaContainsMouse WRITE setParabolicAreaContainsMouse NOTIFY parabolicAreaChanged)
    Q_PROPERTY(bool isVertical READ isVertical CONSTANT)
    Q_PROPERTY(bool isWindow READ isWindow CONSTANT)
    Q_PROPERTY(bool isStartup READ isStartup CONSTANT)
    Q_PROPERTY(bool isLauncher READ isLauncher CONSTANT)
    Q_PROPERTY(bool isSeparator READ isSeparator CONSTANT)
    Q_PROPERTY(QString launcherUrl READ launcherUrl CONSTANT)
    Q_PROPERTY(QString launcherUrlWithIcon READ launcherUrlWithIcon CONSTANT)
    Q_PROPERTY(qreal iconAnimatedOffsetX MEMBER m_iconAnimatedOffsetX)
    Q_PROPERTY(qreal iconAnimatedOffsetY MEMBER m_iconAnimatedOffsetY)
    Q_PROPERTY(bool inAddRemoveAnimation MEMBER m_inAddRemoveAnimation)
    Q_PROPERTY(bool inAnimation MEMBER m_inAnimation)

public:
    explicit TaskItemStub(QObject *parabolicItem, QObject *parent = nullptr)
        : QObject(parent)
        , m_parabolicItem(parabolicItem) {
    }

    QObject *parabolicItem() const {
        return m_parabolicItem;
    }

    QVariantMap abilities() const {
        return QVariantMap{
            {
                QStringLiteral("animations"),
                QVariantMap{
                    {QStringLiteral("speedFactor"), QVariantMap{{QStringLiteral("normal"), 1}, {QStringLiteral("current"), 1}}},
                    {QStringLiteral("duration"), QVariantMap{{QStringLiteral("large"), 100}}},
                    {QStringLiteral("needLength"), QVariant::fromValue(static_cast<QObject *>(const_cast<TaskItemStub *>(this)))},
                }
            },
            {QStringLiteral("metrics"), QVariantMap{{QStringLiteral("iconSize"), 48}}},
            {QStringLiteral("launchers"), QVariant::fromValue(static_cast<QObject *>(const_cast<TaskItemStub *>(this)))},
        };
    }

    bool parabolicAreaIsCurrent() const {
        return m_parabolicAreaIsCurrent;
    }

    void setParabolicAreaIsCurrent(bool current) {
        m_parabolicAreaIsCurrent = current;
        Q_EMIT parabolicAreaChanged();
    }

    bool parabolicAreaContainsMouse() const {
        return m_parabolicAreaContainsMouse;
    }

    void setParabolicAreaContainsMouse(bool containsMouse) {
        m_parabolicAreaContainsMouse = containsMouse;
        Q_EMIT parabolicAreaChanged();
    }

    bool isVertical() const {
        return false;
    }

    bool isWindow() const {
        return true;
    }

    bool isStartup() const {
        return false;
    }

    bool isLauncher() const {
        return false;
    }

    bool isSeparator() const {
        return false;
    }

    QString launcherUrl() const {
        return QStringLiteral("applications:test.desktop");
    }

    QString launcherUrlWithIcon() const {
        return launcherUrl();
    }

    Q_INVOKABLE void addEvent(const QString &) {
    }

    Q_INVOKABLE void removeEvent(const QString &) {
    }

    Q_INVOKABLE bool inCurrentActivity(const QString &) const {
        return true;
    }

Q_SIGNALS:
    void parabolicAreaChanged();

private:
    QObject *m_parabolicItem{nullptr};
    bool m_parabolicAreaIsCurrent{false};
    bool m_parabolicAreaContainsMouse{false};
    qreal m_iconAnimatedOffsetX{0};
    qreal m_iconAnimatedOffsetY{0};
    bool m_inAddRemoveAnimation{false};
    bool m_inAnimation{false};
};

class RootStub : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool newWindowSlidingEnabled READ newWindowSlidingEnabled CONSTANT)
    Q_PROPERTY(bool vertical READ vertical CONSTANT)
    Q_PROPERTY(bool inActivityChange READ inActivityChange CONSTANT)
    Q_PROPERTY(bool inDraggingPhase READ inDraggingPhase CONSTANT)
    Q_PROPERTY(bool showWindowsOnlyFromLaunchers READ showWindowsOnlyFromLaunchers CONSTANT)
    Q_PROPERTY(bool disableAllWindowsFunctionality READ disableAllWindowsFunctionality CONSTANT)

public:
    bool newWindowSlidingEnabled() const { return true; }

    bool vertical() const { return false; }

    bool inActivityChange() const { return false; }

    bool inDraggingPhase() const { return false; }

    bool showWindowsOnlyFromLaunchers() const { return false; }

    bool disableAllWindowsFunctionality() const { return false; }
};

class TasksExtendedManagerStub : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE bool toBeAddedLauncherExists(const QString &) const { return false; }

    Q_INVOKABLE void removeToBeAddedLauncher(const QString &) {}

    Q_INVOKABLE bool immediateLauncherExists(const QString &) const { return false; }

    Q_INVOKABLE void removeImmediateLauncher(const QString &) {}

    Q_INVOKABLE QVariantMap getFrozenTask(const QString &) const { return QVariantMap{{QStringLiteral("zoom"), 1.5}}; }

    Q_INVOKABLE void removeFrozenTask(const QString &) {}
};

class TasksModelStub : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE int launcherPosition(const QString &) const { return -1; }

    Q_INVOKABLE QStringList launcherActivities(const QString &) const { return {}; }
};

class TasksModelForLaunchersStub : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList launcherList READ launcherList WRITE setLauncherList NOTIFY launcherListChanged)
    Q_PROPERTY(int count READ count CONSTANT)

public:
    QStringList launcherList() const {
        return m_launcherList;
    }

    void setLauncherList(const QStringList &launcherList) {
        m_launcherList = launcherList;
        Q_EMIT launcherListChanged();
    }

    int count() const {
        return m_launcherList.count();
    }

    int syncCount() const {
        return m_syncCount;
    }

    Q_INVOKABLE int launcherPosition(const QString &launcher) const {
        return m_launcherList.indexOf(launcher);
    }

    Q_INVOKABLE QStringList launcherActivities(const QString &) const {
        return {};
    }

    Q_INVOKABLE QString launcherIdentity(const QString &url) const
    {
        if (url == QStringLiteral("preferred://filemanager")) {
            return QStringLiteral("applications:test-file-manager.desktop");
        }
        if (url == QStringLiteral("preferred://browser")) {
            return QStringLiteral("applications:test-browser.desktop");
        }
        return url;
    }

    Q_INVOKABLE bool requestAddLauncher(const QString &launcher)
    {
        m_launcherList.append(launcher);
        return true;
    }

    Q_INVOKABLE void syncLaunchers() {
        ++m_syncCount;
        Q_EMIT launcherOrderSyncInvoked();
    }

Q_SIGNALS:
    void launcherListChanged();
    void launcherOrderSyncInvoked();

private:
    QStringList m_launcherList;
    int m_syncCount{0};
};

static void addLatteCoreImport(QQmlEngine &engine, QTemporaryDir &importRoot)
{
    QVERIFY(importRoot.isValid());

    const QString modulePath = importRoot.path() + QStringLiteral("/org/kde/latte/core");
    QVERIFY(QDir().mkpath(modulePath));
    QVERIFY(QFile::copy(QStringLiteral(LATTE_CORE_QMLDIR), modulePath + QStringLiteral("/qmldir")));
    QVERIFY(QFile::copy(QStringLiteral(LATTE_CORE_PLUGIN), modulePath + QStringLiteral("/liblattecoreplugin.so")));

    engine.addImportPath(importRoot.path());
}

static void addLatteComponentsImport(QQmlEngine &engine, QTemporaryDir &importRoot)
{
    QVERIFY(importRoot.isValid());

    const QString modulePath = importRoot.path() + QStringLiteral("/org/kde/latte/components");
    QVERIFY(QDir().mkpath(modulePath));
    QVERIFY(QFile::copy(QStringLiteral(LATTE_SOURCE_DIR "/declarativeimports/components/qmldir"), modulePath + QStringLiteral("/qmldir")));
    QVERIFY(QFile::copy(QStringLiteral(LATTE_SOURCE_DIR "/declarativeimports/components/BadgeText.qml"), modulePath + QStringLiteral("/BadgeText.qml")));

    engine.addImportPath(importRoot.path());
}

static std::unique_ptr<QObject> createQmlObject(QQmlEngine &engine, const QByteArray &source, const QUrl &url)
{
    QQmlComponent component(&engine);
    component.setData(source, url);
    std::unique_ptr<QObject> object(component.create());

    if (!object) {
        qWarning() << component.errors();
    }

    return object;
}

static bool writeTextFile(const QString &path, const QByteArray &contents)
{
    QFile file(path);

    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        return false;
    }

    return file.write(contents) == contents.size();
}

static void addFakePlasmaVolumeImport(QQmlEngine &engine, QTemporaryDir &importRoot)
{
    QVERIFY(importRoot.isValid());

    const QString modulePath = importRoot.path() + QStringLiteral("/org/kde/plasma/private/volume");
    QVERIFY(QDir().mkpath(modulePath));
    QVERIFY(writeTextFile(modulePath + QStringLiteral("/qmldir"), R"(
module org.kde.plasma.private.volume
singleton PulseAudio 0.1 PulseAudio.qml
singleton Server 0.1 Server.qml
singleton PreferredDevice 0.1 PreferredDevice.qml
SinkModel 0.1 SinkModel.qml
SinkInputModel 0.1 SinkInputModel.qml
PulseObjectFilterModel 0.1 PulseObjectFilterModel.qml
VolumeMonitor 0.1 VolumeMonitor.qml
)"));
    QVERIFY(writeTextFile(modulePath + QStringLiteral("/PulseAudio.qml"), R"(
pragma Singleton
import QtQml 2.15
QtObject {
    enum Volume {
        MinimalVolume = 0,
        NormalVolume = 65536
    }
}
)"));
    QVERIFY(writeTextFile(modulePath + QStringLiteral("/Server.qml"), R"(
pragma Singleton
import QtQml 2.15
QtObject {
    property var defaultSink: ({})
}
)"));
    QVERIFY(writeTextFile(modulePath + QStringLiteral("/PreferredDevice.qml"), R"(
pragma Singleton
import QtQml 2.15
QtObject {}
)"));
    QVERIFY(writeTextFile(modulePath + QStringLiteral("/SinkModel.qml"), R"(
import QtQml.Models 2.15
ListModel {}
)"));
    QVERIFY(writeTextFile(modulePath + QStringLiteral("/SinkInputModel.qml"), R"(
import QtQml.Models 2.15
ListModel {}
)"));
    QVERIFY(writeTextFile(modulePath + QStringLiteral("/PulseObjectFilterModel.qml"), R"(
import QtQml.Models 2.15
ListModel {
    dynamicRoles: true
    property var filters
    property var sourceModel
}
)"));

    QVERIFY(writeTextFile(modulePath + QStringLiteral("/VolumeMonitor.qml"), R"(
import QtQml
QtObject {
    property var target: null
    property real volume: 0
    property bool monitorAvailable: true
    readonly property bool available: target !== null && monitorAvailable
}
)"));
    engine.addImportPath(importRoot.path());
}

static void addFakePlasmaCoreImport(QQmlEngine &engine, QTemporaryDir &importRoot)
{
    QVERIFY(importRoot.isValid());

    const QString modulePath = importRoot.path() + QStringLiteral("/org/kde/plasma/core");
    QVERIFY(QDir().mkpath(modulePath));
    QVERIFY(writeTextFile(modulePath + QStringLiteral("/qmldir"), R"(
module org.kde.plasma.core
singleton Types 2.0 Types.qml
)"));
    QVERIFY(writeTextFile(modulePath + QStringLiteral("/Types.qml"), R"(
pragma Singleton
import QtQml 2.15
QtObject {
    enum Types { TopEdge, LeftEdge, BottomEdge, RightEdge, Horizontal, Vertical, HiddenStatus, ActiveStatus, NeedsAttentionStatus }
}
)"));

    engine.addImportPath(importRoot.path());
}

static void addFakePlasmaCoreComponents(QQmlEngine &engine, QTemporaryDir &importRoot)
{
    addFakePlasmaCoreImport(engine, importRoot);

    const QString modulePath = importRoot.path() + QStringLiteral("/org/kde/plasma/core");
    QVERIFY(writeTextFile(modulePath + QStringLiteral("/qmldir"), R"(
module org.kde.plasma.core
singleton Types 2.0 Types.qml
singleton Units 2.0 Units.qml
ToolTipArea 2.0 ToolTipArea.qml
AppletPopup 2.0 AppletPopup.qml
FrameSvgItem 2.0 FrameSvgItem.qml
)"));
    QVERIFY(writeTextFile(modulePath + QStringLiteral("/Units.qml"), R"(
pragma Singleton
import QtQml 2.15
QtObject {
    readonly property int shortDuration: 40
    readonly property int longDuration: 240
}
)"));
    QVERIFY(writeTextFile(modulePath + QStringLiteral("/ToolTipArea.qml"), R"(
import QtQuick 2.15
Item {
    property string mainText: ""
    property string subText: ""
    property int location: 0
    property bool active: false
    property int textFormat: Text.PlainText
    property Item mainItem: null
}
)"));
    QVERIFY(writeTextFile(modulePath + QStringLiteral("/AppletPopup.qml"), R"(
import QtQuick 2.15
Item {
    enum Edge { AtScreenEdges, AtPanelEdges, SolidBackground, StandardBackground }
    property bool hideOnWindowDeactivate: false
    property bool hideOnDeactivate: false
    property bool floating: false
    property int location: 0
    property int removeBorderStrategy: 0
    property int backgroundHints: 0
    property int margin: 0
    property int popupDirection: 0
    property Item visualParent: null
    property Item contentItem: null
    property Item mainItem: null
    property var appletInterface: null
    property int type: 0
    function open() {}
    function close() {}
}
)"));
    QVERIFY(writeTextFile(modulePath + QStringLiteral("/FrameSvgItem.qml"), R"(
import QtQuick 2.15
Item {}
)"));

    engine.addImportPath(importRoot.path());
}

void
QmlSmokeTest::activeTaskScrollsIntoView()
{
    QFile viewportFile(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/taskslayout/ScrollableList.qml"));
    QFile delegateFile(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/task/TaskItem.qml"));
    QVERIFY(viewportFile.open(QFile::ReadOnly));
    QVERIFY(delegateFile.open(QFile::ReadOnly));
    const QString viewport = QString::fromUtf8(viewportFile.readAll());
    const QString delegate = QString::fromUtf8(delegateFile.readAll());
    const int functionsStart = viewport.indexOf(QStringLiteral("    function increasePosWithStep("));
    const int functionsEnd = viewport.indexOf(QStringLiteral("    function autoScrollFor("), functionsStart);
    const int handlerStart = delegate.indexOf(QStringLiteral("    onIsActiveChanged: {"));
    const int handlerEnd = delegate.indexOf(QStringLiteral("    onIsSeparatorChanged:"), handlerStart);
    QVERIFY(functionsStart >= 0 && functionsEnd > functionsStart);
    QVERIFY(handlerStart >= 0 && handlerEnd > handlerStart);
    QQmlEngine engine;
    QQmlComponent component(&engine);
    const QByteArray harness = QByteArrayLiteral(R"(
import QtQuick
Item {
    id: root
    property bool vertical: false
    property var appletAbilities: ({metrics: {iconSize: 40}})
    property alias viewport: scrollableList
    property alias task: taskItem
    Flickable {
        id: scrollableList
        width: 160; height: 160
        contentWidth: 800; contentHeight: 800
        property bool contentsExceed: true
        property real scrollFirstPos: 0
        property real scrollLastPos: 640
)") + viewport.mid(functionsStart, functionsEnd - functionsStart).toUtf8() +
                               QByteArrayLiteral(R"(
        Item {
            id: taskItem
            x: 600; y: 600; width: 40; height: 40
            property bool isActive: false
            function checkWindowsStates() {}
)") + delegate.mid(handlerStart, handlerEnd - handlerStart).toUtf8() +
                               QByteArrayLiteral("\n}\n}\n}");
    component.setData(harness, QUrl(QStringLiteral("qrc:/active-task-scroll.qml")));
    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    QObject *list = qvariant_cast<QObject *>(object->property("viewport"));
    QObject *task = qvariant_cast<QObject *>(object->property("task"));
    QVERIFY(list && task);
    for (bool vertical : { false, true }) {
        QVERIFY(object->setProperty("vertical", vertical));
        const char *offset = vertical ? "contentY" : "contentX";
        const char *coordinate = vertical ? "y" : "x";
        QVERIFY(list->setProperty(offset, 0));
        QVERIFY(task->setProperty(coordinate, 600));
        QVERIFY(task->setProperty("isActive", true));
        QCOMPARE(list->property(offset).toInt(), 520);
        // Visible tasks do not move the list, while wrapping to its start does.
        QVERIFY(task->setProperty("isActive", false));
        QVERIFY(task->setProperty("isActive", true));
        QCOMPARE(list->property(offset).toInt(), 520);
        QVERIFY(task->setProperty("isActive", false));
        QVERIFY(task->setProperty(coordinate, 0));
        QVERIFY(task->setProperty("isActive", true));
        QCOMPARE(list->property(offset).toInt(), 0);
        QVERIFY(task->setProperty("isActive", false));
        QVERIFY(list->setProperty("contentsExceed", false));
        QVERIFY(task->setProperty(coordinate, 600));
        QVERIFY(task->setProperty("isActive", true));
        QCOMPARE(list->property(offset).toInt(), 0);
        QVERIFY(task->setProperty("isActive", false));
        QVERIFY(list->setProperty("contentsExceed", true));
    }
}

void
QmlSmokeTest::containmentWheelScrollsTaskViewport()
{
    QFile containment(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/main.qml"));
    QFile tasks(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/main.qml"));
    QVERIFY(containment.open(QFile::ReadOnly));
    QVERIFY(tasks.open(QFile::ReadOnly));
    const QString outer = QString::fromUtf8(containment.readAll());
    const QString inner = QString::fromUtf8(tasks.readAll());
    const int bridgeStart = outer.indexOf(QStringLiteral("    // Bridge to receive wheel"));
    const int bridgeEnd = outer.indexOf(QStringLiteral("    // In edit mode, block"), bridgeStart);
    const int helperStart = inner.indexOf(QStringLiteral("    function scrollTaskViewport("));
    const int helperEnd = inner.indexOf(QStringLiteral("    // Called from containment's wheel bridge with raw"), helperStart);
    QVERIFY(bridgeStart >= 0 && bridgeEnd > bridgeStart);
    QVERIFY(helperStart >= 0 && helperEnd > helperStart);
    QQuickView view;
    const QVariantMap types{ { QStringLiteral("ScrollNone"), 0 },       { QStringLiteral("ScrollDesktops"), 1 },
                             { QStringLiteral("ScrollActivities"), 2 }, { QStringLiteral("ScrollToggleMinimized"), 4 },
                             { QStringLiteral("ScrollTasks"), 3 },      { QStringLiteral("ManualScrollOnlyParallel"), 1 } };
    const QVariantMap module{ { QStringLiteral("types"), types } };
    view.rootContext()->setContextProperty(QStringLiteral("LatteContainment"), module);
    view.rootContext()->setContextProperty(QStringLiteral("LatteTasks"), module);
    QQmlComponent component(view.engine());
    const QByteArray harness = QByteArrayLiteral(R"(
import QtQuick
Item {
    id: root
    width: 240; height: 240
    property bool editMode: false
    property bool inEditMode: false
    property bool inDraggingPhase: false
    property bool vertical: false
    property bool scrollingEnabled: true
    property bool overflow: true
    readonly property bool manualScrollTasksEnabled: scrollingEnabled && manualScrollTasksType !== 0
    property int manualScrollTasksType: 2
    property int scrollAction: 0
    property int windowActions: 0
    property bool audioHandled: false
    property int minimized: 0
    property int maximized: 0
    property var latteView: ({windowsTracker: {allScreens: {lastActiveWindow: activeWindow}}})
    QtObject {
        id: activeWindow
        property bool isValid: true
        property bool isMinimized: false
        property bool isMaximized: false
        function requestToggleMinimized() { root.minimized++; }
        function requestToggleMaximized() { root.maximized++; }
    }
    property var appletAbilities: ({myView: {isShownFully: true}})
    property var plasmoid: ({applets: [{pluginName: "org.kde.latte.plasmoid"}]})
    property var plasmoidReference: plasmoid
    property var fastLayoutManager: ({resolveAppletQuickItem: function(applet) { return root; }})
    property alias position: scrollableList.contentX
    function activateWheelTask(next) { windowActions++; }
    function adjustVolumeForAudioBadge(x, y, delta, item) { return audioHandled; }
    function emptyAreasWheel(wheel) { windowActions++; }
    Item { id: layoutsContainer; anchors.fill: parent }
    Flickable {
        id: scrollableList
        x: 40; y: 40; width: 160; height: 160
        contentWidth: 800; contentHeight: 160
        interactive: false
        readonly property bool contentsExceed: root.scrollingEnabled && root.overflow
        function decreasePos() { contentX = Math.max(0, contentX - 80); }
        function increasePos() { contentX = Math.min(640, contentX + 80); }
    }
)") + inner.mid(helperStart, helperEnd - helperStart).toUtf8() +
                               outer.mid(bridgeStart, bridgeEnd - bridgeStart).toUtf8() + QByteArrayLiteral("\n}");
    const QUrl url(QStringLiteral("qrc:/containment-viewport-wheel.qml"));
    component.setData(harness, url);
    QObject *object = component.create();
    QVERIFY2(object, qPrintable(component.errorString()));
    view.setContent(url, &component, object);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    auto sendWheel = [&](QPoint delta, Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        const QPoint position(120, 120);
        QWheelEvent wheel(QPointF(position), QPointF(view.mapToGlobal(position)), QPoint(), delta, Qt::NoButton, modifiers, Qt::NoScrollPhase, false);
        QCoreApplication::sendEvent(&view, &wheel);
    };
    sendWheel(QPoint(0, -120));
    QCOMPARE(object->property("position").toInt(), 80);
    QCOMPARE(object->property("windowActions").toInt(), 0);
    sendWheel(QPoint(0, -120));
    QCOMPARE(object->property("position").toInt(), 80);
    QCOMPARE(object->property("windowActions").toInt(), 0);
    QTest::qWait(420);
    QVERIFY(object->setProperty("audioHandled", true));
    sendWheel(QPoint(0, -120));
    QCOMPARE(object->property("position").toInt(), 80);
    QVERIFY(object->setProperty("audioHandled", false));
    sendWheel(QPoint(0, 120));
    QCOMPARE(object->property("position").toInt(), 0);
    QVERIFY(object->setProperty("manualScrollTasksType", 1));
    sendWheel(QPoint(0, -120));
    QCOMPARE(object->property("windowActions").toInt(), 1);
    QTest::qWait(420);
    sendWheel(QPoint(-120, 0));
    QCOMPARE(object->property("position").toInt(), 80);
    QTest::qWait(420);
    QVERIFY(object->setProperty("scrollAction", 3));
    sendWheel(QPoint(0, -120));
    QCOMPARE(object->property("position").toInt(), 80);
    QCOMPARE(object->property("windowActions").toInt(), 2);
    QTest::qWait(220);
    QVERIFY(object->setProperty("scrollAction", 4));
    sendWheel(QPoint(0, 120));
    QCOMPARE(object->property("position").toInt(), 80);
    QCOMPARE(object->property("windowActions").toInt(), 3);
    QTest::qWait(220);
    QVERIFY(object->setProperty("scrollAction", 0));
    QVERIFY(object->setProperty("manualScrollTasksType", 0));
    sendWheel(QPoint(0, -120));
    QCOMPARE(object->property("windowActions").toInt(), 4);

    // Explicit cycling must survive every overflow/manual-scroll combination.
    // Downward toggle-minimize and Ctrl+wheel keep their existing semantics.
    for (int action : { 3, 4 }) {
        QVERIFY(object->setProperty("scrollAction", action));
        for (bool scrolling : { false, true }) {
            QVERIFY(object->setProperty("scrollingEnabled", scrolling));
            for (bool overflow : { false, true }) {
                QVERIFY(object->setProperty("overflow", overflow));
                for (int manual : { 0, 1, 2 }) {
                    QVERIFY(object->setProperty("manualScrollTasksType", manual));
                    QTest::qWait(220);
                    const int before = object->property(action == 3 ? "windowActions" : "minimized").toInt();
                    sendWheel(QPoint(0, -120));
                    QCOMPARE(object->property(action == 3 ? "windowActions" : "minimized").toInt(), before + 1);
                    QCOMPARE(object->property("position").toInt(), 80);
                }
            }
        }
    }
    QTest::qWait(220);
    sendWheel(QPoint(0, 120), Qt::ControlModifier);
    QCOMPARE(object->property("maximized").toInt(), 1);
    QCOMPARE(object->property("position").toInt(), 80);
    QTest::qWait(220);
    const int beforeAudio = object->property("minimized").toInt();
    QVERIFY(object->setProperty("audioHandled", true));
    sendWheel(QPoint(0, -120));
    QCOMPARE(object->property("minimized").toInt(), beforeAudio);
}

void
QmlSmokeTest::editModeBlockerLeavesRightClicksToContextMenu()
{
    // Run the production edit-mode blocker above stand-ins for the applet
    // layouts and the context menu layer that lies beneath them. Only that
    // layer offers "Show Alternatives..." and the edit-only dock actions, so
    // the blocker must leave right-clicks to it while keeping every other
    // applet interaction blocked and applet sort-drag working.
    QFile containment(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/main.qml"));
    QVERIFY(containment.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(containment.readAll());
    const int blockerStart = source.indexOf(QStringLiteral("    // In edit mode, block"));
    const int blockerEnd = source.indexOf(QStringLiteral("    Colorizer.Manager {"), blockerStart);
    QVERIFY(blockerStart >= 0 && blockerEnd > blockerStart);
    QQuickView view;
    QQmlComponent component(view.engine());
    const QByteArray harness = QByteArrayLiteral(R"(
import QtQuick
Item {
    id: root
    width: 300; height: 100
    property bool editMode: true
    property int menuPresses: 0
    property int menuButton: Qt.NoButton
    property int appletClicks: 0
    property int appletHolds: 0
    property int appletWheels: 0
    property int taskPresses: 0
    readonly property bool appletDragActive: appletDrag.active
    Item {
        id: layoutsContainer
        anchors.fill: parent
        readonly property alias startLayout: _startLayout
        readonly property alias mainLayout: _mainLayout
        readonly property alias endLayout: _endLayout
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            onPressed: (mouse) => {
                root.menuPresses++;
                root.menuButton = mouse.button;
            }
        }
        Item { id: _startLayout }
        Item {
            id: _mainLayout
            anchors.fill: parent
            Item {
                width: 100; height: 100
                readonly property bool indexerIsSupported: false
                readonly property bool isHidden: false
                MouseArea {
                    anchors.fill: parent
                    pressAndHoldInterval: 50
                    onClicked: root.appletClicks++
                    onPressAndHold: root.appletHolds++
                    onWheel: (wheel) => { root.appletWheels++; }
                }
                DragHandler {
                    id: appletDrag
                    target: null
                    dragThreshold: 10
                }
            }
            Item {
                x: 100; width: 100; height: 100
                readonly property bool indexerIsSupported: true
                readonly property bool isHidden: false
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onPressed: root.taskPresses++
                }
            }
        }
        Item { id: _endLayout }
    }
)") + source.mid(blockerStart, blockerEnd - blockerStart).toUtf8() + QByteArrayLiteral("\n}");
    const QUrl url(QStringLiteral("qrc:/containment-edit-mode-blocker.qml"));
    component.setData(harness, url);
    QObject *object = component.create();
    QVERIFY2(object, qPrintable(component.errorString()));
    view.setContent(url, &component, object);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    const QPoint applet(50, 50);
    const QPoint task(150, 50);
    const QPoint emptyArea(250, 50);

    // Right-clicks on an external applet and on the empty dock area reach the
    // context menu layer, which builds the applet and dock menus.
    QTest::mouseClick(&view, Qt::RightButton, Qt::NoModifier, applet);
    QCOMPARE(object->property("menuPresses").toInt(), 1);
    QCOMPARE(object->property("menuButton").toInt(), int(Qt::RightButton));
    QTest::mouseClick(&view, Qt::RightButton, Qt::NoModifier, emptyArea);
    QCOMPARE(object->property("menuPresses").toInt(), 2);

    // Left clicks, press-and-hold and wheel never reach the external applet.
    QTest::mouseClick(&view, Qt::LeftButton, Qt::NoModifier, applet);
    QTest::mousePress(&view, Qt::LeftButton, Qt::NoModifier, applet);
    QTest::qWait(150);
    QTest::mouseRelease(&view, Qt::LeftButton, Qt::NoModifier, applet);
    QWheelEvent wheel(QPointF(applet), QPointF(view.mapToGlobal(applet)), QPoint(), QPoint(0, -120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QCoreApplication::sendEvent(&view, &wheel);
    QCOMPARE(object->property("appletClicks").toInt(), 0);
    QCOMPARE(object->property("appletHolds").toInt(), 0);
    QCOMPARE(object->property("appletWheels").toInt(), 0);
    QCOMPARE(object->property("menuPresses").toInt(), 2);

    // The applet's sort-drag handler takes over the blocked left press once
    // the pointer passes its drag threshold.
    QTest::mousePress(&view, Qt::LeftButton, Qt::NoModifier, applet);
    for (int step = 1; step <= 4; ++step) {
        QTest::mouseMove(&view, applet + QPoint(step * 8, 0));
    }
    QVERIFY(object->property("appletDragActive").toBool());
    QTest::mouseRelease(&view, Qt::LeftButton, Qt::NoModifier, applet + QPoint(32, 0));
    QVERIFY(!object->property("appletDragActive").toBool());
    QCOMPARE(object->property("appletClicks").toInt(), 0);

    // Every button over the tasks plasmoid passes through; TaskMouseArea
    // applies the task-side edit-mode rules.
    QTest::mouseClick(&view, Qt::LeftButton, Qt::NoModifier, task);
    QTest::mouseClick(&view, Qt::RightButton, Qt::NoModifier, task);
    QCOMPARE(object->property("taskPresses").toInt(), 2);
    QCOMPARE(object->property("menuPresses").toInt(), 2);

    // Outside edit mode the blocker is inert.
    QVERIFY(object->setProperty("editMode", false));
    QTest::mouseClick(&view, Qt::LeftButton, Qt::NoModifier, applet);
    QCOMPARE(object->property("appletClicks").toInt(), 1);
    QTest::mouseClick(&view, Qt::RightButton, Qt::NoModifier, applet);
    QCOMPARE(object->property("menuPresses").toInt(), 3);
}

void
QmlSmokeTest::containerWheelPreservesTaskCyclingWithOverflow()
{
    QFile file(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/taskslayout/MouseHandler.qml"));
    QVERIFY(file.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(file.readAll());
    const int start = source.lastIndexOf(QStringLiteral("    MouseArea {"));
    const int end = source.lastIndexOf(QStringLiteral("\n}"));
    QVERIFY(start >= 0 && end > start);
    QTemporaryDir scripts;
    QVERIFY(scripts.isValid());
    QVERIFY(writeTextFile(scripts.path() + QStringLiteral("/tools.js"), QByteArrayLiteral("function wheelActivateNextPrevTask(delta, eventDelta) { return 12345; }")));
    QQuickView view;
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    QQmlComponent component(view.engine());
    const QByteArray harness = QByteArrayLiteral("import QtQuick\nimport \"") + QUrl::fromLocalFile(scripts.path() + QStringLiteral("/tools.js")).toEncoded() + QByteArrayLiteral(R"(" as TaskTools
Item {
    id: root
    width: 240; height: 240
    property bool manualScrollTasksEnabled: true
    property bool overflow: true
    property int receivedTaskWheels: 0
    property alias containerDelta: wheelHandler.wheelDelta
    property var plasmoid: ({containment: {configuration: {scrollAction: 4}}})
    property var scrollableList: ({contentsExceed: overflow})
    MouseArea {
        anchors.fill: parent
        onWheel: root.receivedTaskWheels++
    }
)") + source.mid(start, end - start).toUtf8() +
                               QByteArrayLiteral("\n}");
    const QUrl url(QStringLiteral("qrc:/container-wheel-routing.qml"));
    component.setData(harness, url);
    QObject *object = component.create();
    QVERIFY2(object, qPrintable(component.errorString()));
    view.setContent(url, &component, object);
    view.resize(240, 240);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    const QPoint position(120, 120);
    auto sendWheel = [&]() {
        QWheelEvent wheel(QPointF(position), QPointF(view.mapToGlobal(position)), QPoint(), QPoint(0, -120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QCoreApplication::sendEvent(&view, &wheel);
    };
    sendWheel();
    QCOMPARE(object->property("receivedTaskWheels").toInt(), 0);
    QCOMPARE(object->property("containerDelta").toInt(), 12345);
    // Without manual overflow scrolling, preserve the global window action.
    QVERIFY(object->setProperty("manualScrollTasksEnabled", false));
    sendWheel();
    QCOMPARE(object->property("receivedTaskWheels").toInt(), 0);
    QCOMPARE(object->property("containerDelta").toInt(), 12345);
    QVERIFY(object->setProperty("containerDelta", 0));
    QVERIFY(object->setProperty("manualScrollTasksEnabled", true));
    QVERIFY(object->setProperty("overflow", false));
    sendWheel();
    QCOMPARE(object->property("receivedTaskWheels").toInt(), 0);
    QCOMPARE(object->property("containerDelta").toInt(), 12345);
    QVERIFY(object->setProperty("overflow", true));
    sendWheel();
    QCOMPARE(object->property("receivedTaskWheels").toInt(), 0);
}

void
QmlSmokeTest::edgeHoverScrollsWithStationaryPointer_data()
{
    QTest::addColumn<bool>("vertical");
    QTest::addColumn<QPoint>("position");
    QTest::addColumn<int>("direction");
    QTest::newRow("left") << false << QPoint(5, 120) << -1;
    QTest::newRow("right") << false << QPoint(235, 120) << 1;
    QTest::newRow("top") << true << QPoint(120, 5) << -1;
    QTest::newRow("bottom") << true << QPoint(120, 235) << 1;
}

void
QmlSmokeTest::edgeHoverScrollsWithStationaryPointer()
{
    QFETCH(bool, vertical);
    QFETCH(QPoint, position);
    QFETCH(int, direction);
    QQuickView view;
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    QQmlComponent component(view.engine());
    const QByteArray source = QByteArrayLiteral(R"(
import QtQuick
import ")") + QUrl::fromLocalFile(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/taskslayout")).toEncoded() +
                              QByteArrayLiteral(R"(" as TasksLayout
Item {
    id: inputRoot
    width: 240; height: 240
    property alias detector: detector
    property int clicks: 0
    property int wheels: 0
    property int parabolicMoves: 0
    property alias viewport: testViewport
    Flickable {
        id: testViewport
        anchors.fill: parent
        contentWidth: 800
        contentHeight: 800
        interactive: false
        Item {
            width: 800; height: 800
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onClicked: inputRoot.clicks++
                onWheel: inputRoot.wheels++
            }
            // Match the separate, button-transparent hover area above each task.
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.NoButton
                hoverEnabled: true
                onPositionChanged: inputRoot.parabolicMoves++
            }
        }
    }
    TasksLayout.ScrollEdgeHover {
        id: detector
        parent: testViewport
        viewport: testViewport
        edgeLength: 40
        canScrollBackward: true
        canScrollForward: true
        repeatInterval: 30
    }
}
)");
    const QUrl url(QStringLiteral("qrc:/edge-hover-events.qml"));
    component.setData(source, url);
    QObject *object = component.create();
    QVERIFY2(object, qPrintable(component.errorString()));
    view.setContent(url, &component, object);
    view.resize(240, 240);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QObject *detector = object->property("detector").value<QObject *>();
    QVERIFY(detector);
    QObject *viewport = object->property("viewport").value<QObject *>();
    QVERIFY(viewport);
    QCOMPARE(detector->property("viewport").value<QObject *>(), viewport);
    // Exercise viewport coordinates after content has moved, not just at zero.
    QVERIFY(viewport->setProperty("contentX", 64.0));
    QVERIFY(viewport->setProperty("contentY", 64.0));
    QVERIFY(detector->setProperty("vertical", vertical));
    QSignalSpy requests(detector, SIGNAL(scrollRequested(int)));
    QVERIFY(requests.isValid());

    // A real mouse event over a hover-enabled MouseArea must reach the passive
    // detector too; remaining still must produce more than one scroll step.
    const QPoint center(120, 120);
    QTest::mouseMove(&view, center);
    QTest::mouseMove(&view, position);
    QTRY_VERIFY_WITH_TIMEOUT(requests.count() >= 2, 1000);
    QCOMPARE(requests.last().at(0).toInt(), direction);
    QVERIFY(object->property("parabolicMoves").toInt() > 0);

    QVERIFY(detector->setProperty("scrollingInProgress", true));
    int count = requests.count();
    QTest::qWait(90);
    QCOMPARE(requests.count(), count);
    QVERIFY(detector->setProperty("scrollingInProgress", false));
    QTRY_VERIFY_WITH_TIMEOUT(requests.count() > count, 1000);

    QVERIFY(detector->setProperty(direction < 0 ? "canScrollBackward" : "canScrollForward", false));
    count = requests.count();
    QTest::qWait(90);
    QCOMPARE(requests.count(), count);
    QVERIFY(detector->setProperty(direction < 0 ? "canScrollBackward" : "canScrollForward", true));
    QTRY_VERIFY_WITH_TIMEOUT(requests.count() > count, 1000);

    QVERIFY(detector->setProperty("enabled", false));
    count = requests.count();
    QTest::qWait(90);
    QCOMPARE(requests.count(), count);
    QVERIFY(detector->setProperty("enabled", true));
    QTest::mouseMove(&view, center);
    count = requests.count();
    QTest::qWait(90);
    QCOMPARE(requests.count(), count);
    QTest::mouseClick(&view, Qt::LeftButton, Qt::NoModifier, center);
    QCOMPARE(object->property("clicks").toInt(), 1);
    QWheelEvent wheel(QPointF(center), QPointF(view.mapToGlobal(center)), QPoint(), QPoint(0, 120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QCoreApplication::sendEvent(&view, &wheel);
    QCOMPARE(object->property("wheels").toInt(), 1);
    QTest::mouseMove(&view, position);
    QTRY_VERIFY_WITH_TIMEOUT(requests.count() > count, 1000);
    object->setProperty("visible", false);
    count = requests.count();
    QTest::qWait(90);
    QCOMPARE(requests.count(), count);
}

void
QmlSmokeTest::scrollingTasksCanRequestShorterViewports()
{
    QFile file(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/main.qml"));
    QVERIFY(file.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(file.readAll());
    const int start = source.indexOf(QStringLiteral("    readonly property int minimumLength:"));
    const int end = source.indexOf(QStringLiteral("    property real textColorBrightness:"), start);
    QVERIFY(start >= 0 && end > start);
    QQmlEngine engine;
    auto object = createQmlObject(engine,
                                  QByteArrayLiteral(R"(
import QtQuick
Item {
    id: root
    property bool scrollingEnabled: false
    property real taskLayoutLengthHint: 592
    property var appletAbilities: ({metrics: {totals: {length: 74}}})
)") + source.mid(start, end - start).toUtf8() +
                                    QByteArrayLiteral("\n}"),
                                  QUrl(QStringLiteral("qrc:/scroll-minimum.qml")));
    QVERIFY(object);
    QCOMPARE(object->property("minimumLength").toInt(), 592);
    QVERIFY(object->setProperty("scrollingEnabled", true));
    QCOMPARE(object->property("minimumLength").toInt(), 222);
    QVERIFY(object->setProperty("taskLayoutLengthHint", 148.0));
    QCOMPARE(object->property("minimumLength").toInt(), 148);
    QVERIFY(object->setProperty("scrollingEnabled", false));
    QCOMPARE(object->property("minimumLength").toInt(), 148);
}

void
QmlSmokeTest::scrollingTasksKeepFillRequestsInModernDocks()
{
    QFile file(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/applet/AppletItem.qml"));
    QVERIFY(file.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(file.readAll());
    const int start = source.indexOf(QStringLiteral("    property bool isRequestingFill:"));
    const int end = source.indexOf(QStringLiteral("    property int maxAutoFillLength:"), start);
    QVERIFY(start >= 0 && end > start);
    QQmlEngine engine;
    auto object = createQmlObject(engine,
                                  QByteArrayLiteral(R"(
import QtQuick
import QtQuick.Layouts
Item {
    property QtObject root: QtObject {
        property bool singleRowLayoutEnabled: true
        property bool isHorizontal: true
        property bool isVertical: !isHorizontal
    }
    property bool indexerIsSupported: true
    property bool isGlobalMenuApplet: false
    property bool isHidden: false
    property Item applet: Item {
        property bool scrollingEnabled: false
        Layout.fillWidth: scrollingEnabled
        Layout.fillHeight: scrollingEnabled
    }
)") + source.mid(start, end - start).toUtf8() +
                                    QByteArrayLiteral("\n}"),
                                  QUrl(QStringLiteral("qrc:/scroll-fill.qml")));
    QVERIFY(object);
    QObject *applet = object->property("applet").value<QObject *>();
    QObject *root = object->property("root").value<QObject *>();
    QVERIFY(applet && root);
    QCOMPARE(object->property("isRequestingFill").toBool(), false);
    QVERIFY(applet->setProperty("scrollingEnabled", true));
    QCOMPARE(object->property("isRequestingFill").toBool(), true);
    QVERIFY(root->setProperty("isHorizontal", false));
    QCOMPARE(object->property("isRequestingFill").toBool(), true);
    QVERIFY(object->setProperty("isHidden", true));
    QCOMPARE(object->property("isRequestingFill").toBool(), false);
    QVERIFY(object->setProperty("isHidden", false));
    QVERIFY(applet->setProperty("scrollingEnabled", false));
    QCOMPARE(object->property("isRequestingFill").toBool(), false);
}

void QmlSmokeTest::appletIndexFollowsItsLayoutWithoutTransientInvalidation()
{
    QFile file(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/applet/AppletItem.qml"));
    QVERIFY(file.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(file.readAll());
    const int start = source.indexOf(QStringLiteral("    function checkIndex("));
    const int end = source.indexOf(QStringLiteral("    function sltClearZoom("), start);
    QVERIFY(start >= 0 && end > start);

    QQmlEngine engine;
    auto object = createQmlObject(engine,
                                  QByteArrayLiteral(R"(
import QtQuick
Item {
    id: fixture
    property alias applet: appletItem
    property alias main: mainLayout
    property alias end: endLayout
    property alias splitter: splitterItem
    Item {
        id: startLayout
        readonly property int count: children.length
        property int beginIndex: 0
        Item { property bool isParabolicEdgeSpacer: true }
        Item { id: splitterItem; property bool isInternalViewSplitter: true }
        Item {
            id: appletItem
            property int index: -1
            property bool isParabolicEdgeSpacer: false
            property bool isInternalViewSplitter: false
            readonly property var layoutsContainerReference: ({
                startLayout: startLayout, mainLayout: mainLayout, endLayout: endLayout
            })
            readonly property var layoutsContainer: layoutsContainerReference
            readonly property var layouter: layoutsContainerReference
)") + source.mid(start, end - start).toUtf8() + QByteArrayLiteral(R"(
        }
    }
    Item {
        id: mainLayout
        readonly property int count: children.length
        property int beginIndex: 100
        Item {}
    }
    Item {
        id: endLayout
        readonly property int count: children.length
        property int beginIndex: 200
    }
}
)"),
                                  QUrl(QStringLiteral("qrc:/applet-index.qml")));
    QVERIFY(object);
    auto *applet = object->property("applet").value<QQuickItem *>();
    auto *main = object->property("main").value<QQuickItem *>();
    auto *endLayout = object->property("end").value<QQuickItem *>();
    auto *splitter = object->property("splitter").value<QQuickItem *>();
    QVERIFY(applet && main && endLayout && splitter);
    QSignalSpy changes(applet, SIGNAL(indexChanged()));
    QVERIFY(changes.isValid());

    QVERIFY(QMetaObject::invokeMethod(applet, "checkIndex"));
    QCOMPARE(applet->property("index").toInt(), 0);
    QCOMPARE(changes.count(), 1);

    QVERIFY(QMetaObject::invokeMethod(applet, "checkIndex"));
    QCOMPARE(changes.count(), 1);

    applet->setParentItem(main);
    QVERIFY(QMetaObject::invokeMethod(applet, "checkIndex"));
    QCOMPARE(applet->property("index").toInt(), 101);
    QCOMPARE(changes.count(), 2);

    applet->stackBefore(main->childItems().constFirst());
    QVERIFY(QMetaObject::invokeMethod(applet, "checkIndex"));
    QCOMPARE(applet->property("index").toInt(), 100);
    QCOMPARE(changes.count(), 3);

    applet->setParentItem(endLayout);
    QVERIFY(QMetaObject::invokeMethod(applet, "checkIndex"));
    QCOMPARE(applet->property("index").toInt(), 200);
    QCOMPARE(changes.count(), 4);

    QVERIFY(applet->setProperty("isInternalViewSplitter", true));
    QVERIFY(QMetaObject::invokeMethod(applet, "checkIndex"));
    QCOMPARE(applet->property("index").toInt(), -1);
    QCOMPARE(changes.count(), 5);

    applet->setParentItem(splitter);
    QVERIFY(applet->setProperty("isInternalViewSplitter", false));
    QVERIFY(QMetaObject::invokeMethod(applet, "checkIndex"));
    QCOMPARE(applet->property("index").toInt(), -1);
    QCOMPARE(changes.count(), 5);
}

void
QmlSmokeTest::scrollingTasksShareRemainingModernDockLength()
{
    QFile file(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/abilities/privates/LayouterPrivate.qml"));
    QVERIFY(file.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(file.readAll());
    const int start = source.indexOf(QStringLiteral("    function normalizedModernFillLength("));
    const int end = source.indexOf(QStringLiteral("    function _updateSizeForAppletsInFill()"), start);
    QVERIFY(start >= 0 && end > start);
    QQmlEngine engine;
    auto object = createQmlObject(engine,
                                  QByteArrayLiteral(R"(
import QtQuick
Item {
    property var metrics: ({iconSize: 64})
    property var metricsReference: ({iconSize: 64})
    property var root: ({isHorizontal: true})
    property real contentsMaxLength: 1920
    property real sizeWithNoFillApplets: 1500
    property bool useSecondTask: false
    property var firstTask: ({isAutoFillApplet: true, isHidden: false,
        indexerIsSupported: true, applet: {scrollingEnabled: true},
        appletMinimumLength: 222, appletPreferredLength: 592, appletMaximumLength: -1})
    property var secondTask: ({isAutoFillApplet: true, isHidden: false,
        indexerIsSupported: true, applet: {scrollingEnabled: true},
        appletMinimumLength: 148, appletPreferredLength: 148, appletMaximumLength: -1})
    property var clock: ({isAutoFillApplet: true, isHidden: false,
        indexerIsSupported: false, appletMinimumLength: 100,
        appletPreferredLength: 100, appletMaximumLength: -1})
    property var startLayout: ({grid: {children: []}})
    property var mainLayout: ({grid: {children: useSecondTask ? [firstTask, clock, secondTask] : [firstTask, clock]}})
    property var endLayout: ({grid: {children: []}})
    property real firstLength: -1
    property real secondLength: -1
    property real clockLength: -1
    function apply() {
        updateModernFillApplets();
        firstLength = firstTask.maxAutoFillLength;
        secondLength = secondTask.maxAutoFillLength === undefined ? -1 : secondTask.maxAutoFillLength;
        clockLength = clock.maxAutoFillLength;
    }
)") + source.mid(start, end - start).toUtf8() +
                                    QByteArrayLiteral("\n}"),
                                  QUrl(QStringLiteral("qrc:/scroll-allocation.qml")));
    QVERIFY(object);
    QVERIFY(QMetaObject::invokeMethod(object.get(), "apply"));
    QCOMPARE(object->property("firstLength").toReal(), 320.0);
    QCOMPARE(object->property("clockLength").toReal(), 100.0);
    // Spare space must not restore the oversized task slots fixed for normal docks.
    QVERIFY(object->setProperty("contentsMaxLength", 2400.0));
    QVERIFY(QMetaObject::invokeMethod(object.get(), "apply"));
    QCOMPARE(object->property("firstLength").toReal(), 592.0);
    // The short second list keeps its natural length and leaves the rest to the longer list.
    QVERIFY(object->setProperty("contentsMaxLength", 2100.0));
    QVERIFY(object->setProperty("useSecondTask", true));
    QVERIFY(QMetaObject::invokeMethod(object.get(), "apply"));
    QCOMPARE(object->property("firstLength").toReal(), 352.0);
    QCOMPARE(object->property("secondLength").toReal(), 148.0);
    QCOMPARE(object->property("clockLength").toReal(), 100.0);
    // If fixed widgets alone exhaust the dock, retain the minimum reachable task slots.
    QVERIFY(object->setProperty("contentsMaxLength", 1500.0));
    QVERIFY(QMetaObject::invokeMethod(object.get(), "apply"));
    QCOMPARE(object->property("firstLength").toReal(), 222.0);
    QCOMPARE(object->property("secondLength").toReal(), 148.0);
}

void QmlSmokeTest::initTestCase()
{
    ensurePlasmaPlasmoidModuleAvailable();
}

void
QmlSmokeTest::colorButtonSeparatesSwatchAndText_data()
{
    QTest::addColumn<int>("pointSize");
    QTest::newRow("normal-text") << 10;
    QTest::newRow("large-text") << 20;
}

void
QmlSmokeTest::colorButtonSeparatesSwatchAndText()
{
    QFETCH(int, pointSize);
    QQuickView view;
    QQmlComponent component(view.engine());
    component.setData(R"(
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import "../shell/package/contents/controls" as Controls
GridLayout {
    width: 480
    columns: 2
    property int pointSize: 10
    QQC2.Label { text: "Day color" }
    Controls.ColorButton {
        objectName: "testColorButton"
        selectedColor: "#133557"
        Layout.fillWidth: true
        font.pointSize: parent.pointSize
    }
}
)",
                      QUrl::fromLocalFile(QStringLiteral(LATTE_SOURCE_DIR "/autotests/color-button-test.qml")));
    std::unique_ptr<QObject> object(component.createWithInitialProperties({ { QStringLiteral("pointSize"), pointSize } }));
    QVERIFY2(object, qPrintable(component.errorString()));
    auto *layout = qobject_cast<QQuickItem *>(object.get());
    QVERIFY(layout);
    layout->setParentItem(view.contentItem());
    auto *button = layout->findChild<QQuickItem *>(QStringLiteral("testColorButton"));
    QVERIFY(button);
    view.resize(480, 120);
    view.show();
    auto *swatch = button->findChild<QQuickItem *>(QStringLiteral("colorSwatch"));
    auto *label = button->findChild<QQuickItem *>(QStringLiteral("colorLabel"));
    QVERIFY(swatch);
    QVERIFY(label);
    // The native style must have no text to paint a second time. The single
    // content label and swatch must retain separate geometry at larger fonts.
    QCOMPARE(button->property("text").toString(), QString());
    QCOMPARE(label->property("text").toString(), QStringLiteral("#133557"));
    QTRY_VERIFY(swatch->width() > 0);
    QTRY_VERIFY(label->x() >= swatch->x() + swatch->width());
    QTRY_VERIFY(label->width() + 1 >= label->implicitWidth());
    QTRY_VERIFY(swatch->mapToItem(button, QPointF()).x() >= button->property("leftPadding").toReal() - 1);
    QTRY_VERIFY(label->mapToItem(button, QPointF(label->width(), 0)).x() <= button->width() - button->property("rightPadding").toReal() + 1);
    QVERIFY(button->setProperty("selectedColor", QColor(QStringLiteral("#ff2020"))));
    QCOMPARE(label->property("text").toString(), QStringLiteral("#ff2020"));
}

void
QmlSmokeTest::backgroundColorScheduleFollowsLocalTime()
{
    QQmlEngine engine;
    auto config = createQmlObject(engine,
                                  R"(
import QtQuick
QtObject {
    property bool customBackgroundColorEnabled: true
    property bool backgroundColorScheduleEnabled: false
    property color customBackgroundColor: "#fafafa"
    property color customNightBackgroundColor: "#202020"
    property int backgroundDayStart: 420
    property int backgroundNightStart: 1140
}
)",
                                  QUrl(QStringLiteral("background-config.qml")));
    QVERIFY(config);
    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/colorizer/BackgroundColorPolicy.qml")));
    std::unique_ptr<QObject> policy(component.createWithInitialProperties({ { QStringLiteral("configuration"), QVariant::fromValue(config.get()) } }));
    QVERIFY2(policy, qPrintable(component.errorString()));
    const auto setTime = [&policy](int hour, int minute) { return policy->setProperty("currentTime", QDateTime(QDate(2026, 11, 13), QTime(hour, minute))); };
    const auto color = [&policy]() { return policy->property("backgroundColor").value<QColor>(); };
    QVERIFY(setTime(23, 0));
    QCOMPARE(color(), QColor(QStringLiteral("#fafafa"))); // Disabled schedule keeps the day color.
    QVERIFY(config->setProperty("backgroundColorScheduleEnabled", true));
    QVERIFY(setTime(6, 59));
    QCOMPARE(color(), QColor(QStringLiteral("#202020")));
    QCOMPARE(policy->property("textColor").value<QColor>(), QColor(Qt::white));
    QVERIFY(setTime(7, 0));
    QCOMPARE(color(), QColor(QStringLiteral("#fafafa")));
    QVERIFY(setTime(18, 59));
    QCOMPARE(color(), QColor(QStringLiteral("#fafafa")));
    QVERIFY(setTime(19, 0));
    QCOMPARE(color(), QColor(QStringLiteral("#202020")));
    // Reversed boundaries represent a daytime interval that crosses midnight.
    QVERIFY(config->setProperty("backgroundDayStart", 1140));
    QVERIFY(config->setProperty("backgroundNightStart", 420));
    QVERIFY(setTime(23, 0));
    QCOMPARE(color(), QColor(QStringLiteral("#fafafa")));
    QVERIFY(setTime(7, 0));
    QCOMPARE(color(), QColor(QStringLiteral("#202020")));
    QVERIFY(config->setProperty("backgroundNightStart", 1140));
    QCOMPARE(color(), QColor(QStringLiteral("#fafafa"))); // Equal boundaries mean all day.
    QVERIFY(config->setProperty("customBackgroundColor", QColor(QStringLiteral("#ffcc00"))));
    QCOMPARE(color(), QColor(QStringLiteral("#ffcc00")));
    QVERIFY(config->setProperty("backgroundColorScheduleEnabled", false));
    QVERIFY(setTime(2, 0));
    QCOMPARE(color(), QColor(QStringLiteral("#ffcc00")));
}

void
QmlSmokeTest::indicatorShapeFollowsSavedOption_data()
{
    QTest::addColumn<int>("dockStyle");
    QTest::addColumn<bool>("vertical");
    QTest::newRow("classic-horizontal") << 0 << false;
    QTest::newRow("modern-horizontal") << 1 << false;
    QTest::newRow("classic-vertical") << 0 << true;
    QTest::newRow("modern-vertical") << 1 << true;
}

void
QmlSmokeTest::indicatorShapeFollowsSavedOption()
{
    QFETCH(int, dockStyle);
    QFETCH(bool, vertical);
    QTemporaryDir importRoot;
    QQmlEngine engine;
    addLatteCoreImport(engine, importRoot);
    addLatteComponentsImport(engine, importRoot);
    for (const QString &name : { QStringLiteral("IndicatorItem.qml"), QStringLiteral("GlowPoint.qml") }) {
        QVERIFY(QFile::copy(QStringLiteral(LATTE_SOURCE_DIR "/declarativeimports/components/") + name, importRoot.path() + QStringLiteral("/org/kde/latte/components/") + name));
    }

    // Load the production renderer with an active task's public API. Shape must
    // follow configuration changes without recreating the task or switching dock style.
    auto host = createQmlObject(engine,
                                R"(
import QtQuick
import org.kde.plasma.core as PlasmaCore
Item {
    id: host
    width: 64; height: 64
    property int dockStyle: 0
    property int compactIndicators: -1
    property bool vertical: false
    property QtObject plasmoid: QtObject {
        property var configuration: ({dockStyle: host.dockStyle})
        property int location: host.vertical ? PlasmaCore.Types.LeftEdge : PlasmaCore.Types.BottomEdge
        property int formFactor: host.vertical ? PlasmaCore.Types.Vertical : PlasmaCore.Types.Horizontal
    }
    property QtObject config: QtObject {
        property int activeStyle: 0
        property bool modernDotPreset: true
        property bool useIconColor: false
        property real size: 0.1
        property real thickMargin: 0
        property real lengthPadding: 0.08
        property real backgroundCornerMargin: 1
        property bool reversed: false
        property bool extraDotOnActive: false
        property bool dynamicWindowDots: false
        property bool minimizedTaskColoredDifferently: false
        property bool glowEnabled: false
        property bool glow3D: false
        property int glowApplyTo: 0
        property real glowOpacity: 1
        property bool enabledForApplets: true
    }
    property Item level: Item {
        property var requested: ({iconOffsetX: 0, iconOffsetY: 0})
        property Item bridge: Item {
            property QtObject configuration: host.config
            property bool isModernDockStyle: host.compactIndicators === -1
                                             ? host.dockStyle === 1 : host.compactIndicators === 1
            property int currentIconSize: 48
            property int maxIconSize: 48
            property int screenEdgeMargin: 0
            property int tailThickness: 8
            property var palette: ({textColor: "white", backgroundColor: "black", negativeTextColor: "red"})
            property bool isMinimized: false
            property bool isActive: true
            property bool hasActive: true
            property bool isGroup: false
            property bool isTask: true
            property bool isApplet: false
            property bool isEmptySpace: false
            property bool isLauncher: false
            property bool isWindow: true
            property bool inRemoving: false
            property bool inAttention: false
            property real durationTime: 0
            property real scaleFactor: 1
            property color iconBackgroundColor: "#62dd85"
            property color shadowColor: "black"
        }
    }
}
)",
                                QUrl(QStringLiteral("indicator-host.qml")));
    QVERIFY(host);
    host->setProperty("dockStyle", dockStyle);
    host->setProperty("vertical", vertical);
    engine.rootContext()->setContextProperty(QStringLiteral("plasmoid"), host->property("plasmoid").value<QObject *>());
    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(LATTE_SOURCE_DIR "/indicators/default/package/ui/main.qml")));
    const QVariantMap initial{ { QStringLiteral("parent"), QVariant::fromValue(qobject_cast<QQuickItem *>(host.get())) }, { QStringLiteral("width"), 64 }, { QStringLiteral("height"), 64 } };
    std::unique_ptr<QObject> renderer(component.createWithInitialProperties(initial));
    QVERIFY2(renderer, qPrintable(component.errorString()));
    QObject *point = nullptr;
    for (auto *child : renderer->findChildren<QObject *>()) {
        if (child->metaObject()->indexOfProperty("stateWidth") >= 0) {
            point = child;
            break;
        }
    }
    QVERIFY(point);
    auto *config = host->property("config").value<QObject *>();
    QVERIFY(config);
    const char *length = vertical ? "stateHeight" : "stateWidth";
    QCOMPARE(renderer->property("effectiveActiveStyle").toInt(), dockStyle == 1 ? 1 : 0);
    if (dockStyle == 1) {
        QCOMPARE(point->property(length).toInt(), renderer->property("size").toInt());
    }
    QVERIFY(host->setProperty("compactIndicators", 1));
    QCOMPARE(renderer->property("effectiveActiveStyle").toInt(), 1);
    QVERIFY(host->setProperty("compactIndicators", 0));
    QCOMPARE(renderer->property("effectiveActiveStyle").toInt(), 0);
    QVERIFY(host->setProperty("compactIndicators", -1));
    QVERIFY(config->setProperty("modernDotPreset", false));
    QCOMPARE(renderer->property("effectiveActiveStyle").toInt(), 0);
    QVERIFY(point->property(length).toInt() > renderer->property("size").toInt());
    QVERIFY(config->setProperty("activeStyle", 1));
    QCOMPARE(renderer->property("effectiveActiveStyle").toInt(), 1);
    QCOMPARE(point->property(length).toInt(), renderer->property("size").toInt());
    QVERIFY(config->setProperty("activeStyle", 0));
    QVERIFY(point->property(length).toInt() > renderer->property("size").toInt());
    QCOMPARE(renderer->property("needsIconColors").toBool(), false);
    QVERIFY(config->setProperty("useIconColor", true));
    QCOMPARE(renderer->property("needsIconColors").toBool(), true);
    QCOMPARE(renderer->property("isActiveColor").value<QColor>(), QColor(QStringLiteral("#62dd85")));
    QVERIFY(config->setProperty("useIconColor", false));
    QCOMPARE(renderer->property("isActiveColor").value<QColor>(), QColor(Qt::white));
}

void QmlSmokeTest::latteCoreQmlPluginLoadsFromBuildTree()
{
    QTemporaryDir importRoot;
    QQmlEngine engine;
    addLatteCoreImport(engine, importRoot);

    QQmlComponent component(&engine);
    component.setData(R"(
import QtQml 2.15
import org.kde.latte.core 0.2

QtObject {
    property int separator: Environment.separatorLength
    property int version: Environment.makeVersion(1, 2, 3)
    property real brightness: Tools.colorBrightness("#ffffff")
    property real lumina: Tools.colorLumina("#000000")
    property bool hasCompositingProperty: WindowSystem.compositingActive === true || WindowSystem.compositingActive === false
}
)",
                      QUrl(QStringLiteral("qrc:/lattecore-smoke.qml")));

    std::unique_ptr<QObject> object(component.create());
    if (!object) {
        qWarning() << component.errors();
    }

    QVERIFY(object);
    QCOMPARE(object->property("separator").toInt(), 5);
    QCOMPARE(object->property("version").toInt(), 0x010203);
    QVERIFY(object->property("brightness").toReal() > 0.99);
    QCOMPARE(object->property("lumina").toReal(), 0.0);
    QCOMPARE(object->property("hasCompositingProperty").toBool(), true);
}

void QmlSmokeTest::layoutInfoWindowHasAValidSizeFromSource()
{
    // InfoView is the window shown while layouts switch. Its size becomes the
    // window geometry; a zero dimension is a Wayland protocol error that makes
    // KWin disconnect Latte, so it must evaluate to a real size without QML
    // errors (it used the Plasma 5 global `units`, undefined in Plasma 6).
    QTemporaryDir importRoot;
    QQmlEngine engine;
    addLatteCoreImport(engine, importRoot);
    QStringList warnings;
    connect(&engine, &QQmlEngine::warnings, &engine, [&warnings](const QList<QQmlError> &errors) {
        for (const QQmlError &error : errors) {
            warnings.append(error.toString());
        }
    });

    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(LATTE_INFO_VIEW_QML)));
    std::unique_ptr<QObject> object(component.create());
    if (!object) {
        qWarning() << component.errors();
    }

    QVERIFY(object);
    object->setProperty("message", QStringLiteral("Switching to layout Default"));
    QCoreApplication::processEvents();

    QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join(QLatin1Char('\n'))));
    QVERIFY(object->property("width").toReal() > 0);
    QVERIFY(object->property("height").toReal() > 0);
}

void QmlSmokeTest::appletAlternativesDialogResolvesOutsidePlasmashell()
{
    // Since Plasma 6.5 Plasma's AppletAlternatives.qml requires the
    // AlternativesHelper type, which is compiled into plasmashell; in Latte it
    // was "not a type" and the dialog never opened. Latte's own copy must
    // resolve every import and type in a process that is not plasmashell.
    QQmlEngine engine;

    // The dialog lists alternatives with WidgetExplorer from the plasma-workspace
    // runtime package, like Latte's widget explorer; build-only environments
    // may ship just the -dev package.
    QQmlComponent probe(&engine);
    probe.setData(QByteArrayLiteral("import org.kde.plasma.private.shell\nimport QtQml\nQtObject {}"), QUrl(QStringLiteral("probe.qml")));

    if (probe.status() != QQmlComponent::Ready) {
        QSKIP("org.kde.plasma.private.shell (plasma-workspace runtime) is not installed");
    }

    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(LATTE_APPLET_ALTERNATIVES_QML)));

    if (!component.isReady()) {
        qWarning() << component.errors();
    }

    QVERIFY(component.isReady());
}

void QmlSmokeTest::showWindowAnimationFrozenZoomDecisionLoadsFromSource()
{
    QTemporaryDir importRoot;
    QQmlEngine engine;
    addLatteCoreImport(engine, importRoot);

    QQmlContext context(engine.rootContext());
    ParabolicTargetStub parabolicTarget;
    TaskItemStub taskItem(&parabolicTarget);
    RootStub root;
    TasksExtendedManagerStub tasksExtendedManager;
    TasksModelStub tasksModel;
    QObject publishGeometryTimer;
    QVariantMap activityInfo{
        {QStringLiteral("currentActivity"), QStringLiteral("current")},
        {QStringLiteral("previousActivity"), QStringLiteral("previous")},
    };

    context.setContextProperty(QStringLiteral("taskItem"), &taskItem);
    context.setContextProperty(QStringLiteral("root"), &root);
    context.setContextProperty(QStringLiteral("tasksExtendedManager"), &tasksExtendedManager);
    context.setContextProperty(QStringLiteral("tasksModel"), &tasksModel);
    context.setContextProperty(QStringLiteral("activityInfo"), activityInfo);
    context.setContextProperty(QStringLiteral("publishGeometryTimer"), &publishGeometryTimer);
    context.setContextProperty(QStringLiteral("isWindow"), true);
    context.setContextProperty(QStringLiteral("isLauncher"), false);
    context.setContextProperty(QStringLiteral("isForcedHidden"), false);
    context.setContextProperty(QStringLiteral("index"), QVariant::fromValue(0));
    context.setContextProperty(QStringLiteral("icList"), QVariantMap{});

    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(LATTE_SHOW_WINDOW_ANIMATION_QML)));
    const QVariantMap initialProperties{{QStringLiteral("syncingLauncherOrder"), false}};
    std::unique_ptr<QObject> object(component.createWithInitialProperties(initialProperties, &context));
    if (!object) {
        qWarning() << component.errors();
    }

    QVERIFY(object);
    QVariant keepFrozenZoom;
    QVERIFY(QMetaObject::invokeMethod(object.get(), "keepFrozenZoomForCurrentTask", Q_RETURN_ARG(QVariant, keepFrozenZoom)));
    QCOMPARE(keepFrozenZoom.toBool(), false);

    taskItem.setParabolicAreaContainsMouse(true);
    QVERIFY(QMetaObject::invokeMethod(object.get(), "keepFrozenZoomForCurrentTask", Q_RETURN_ARG(QVariant, keepFrozenZoom)));
    QCOMPARE(keepFrozenZoom.toBool(), true);

    taskItem.setParabolicAreaContainsMouse(false);
    taskItem.setParabolicAreaIsCurrent(true);
    QVERIFY(QMetaObject::invokeMethod(object.get(), "keepFrozenZoomForCurrentTask", Q_RETURN_ARG(QVariant, keepFrozenZoom)));
    QCOMPARE(keepFrozenZoom.toBool(), true);
}

void QmlSmokeTest::newTaskPublishesSlotAfterEntranceSetup()
{
    QFile source(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/task/TaskItem.qml"));
    QVERIFY(source.open(QFile::ReadOnly));
    const QByteArray contents = source.readAll();
    const QByteArray bindingStart("occupiesLayoutSlot:");
    const qsizetype start = contents.indexOf(bindingStart);
    QVERIFY(start >= 0);
    const QByteArray slotBinding = contents.mid(start, contents.indexOf('\n', start) - start);

    QQmlEngine engine;
    auto host = createQmlObject(engine, QByteArrayLiteral(R"(
import QtQuick
Item {
    id: host
    property alias task: taskItem
    property real followingX: 100 + taskItem.width
    Item {
        id: taskItem
        visible: false
        property bool initialTaskGeometryReady: false
        property bool movedToMinimizedSection: false
        property bool waitingForTaskRowReplacement: false
        property bool checkingTaskRowReplacement: false
        property bool waitingForLauncherPosition: false
        property bool )") + slotBinding + QByteArrayLiteral(R"(
        property real zoomLength: 1
        width: visible && occupiesLayoutSlot ? 48 * zoomLength : 0
    }
}
)"), QUrl(QStringLiteral("qrc:/new-task-slot.qml")));
    QVERIFY(host);
    auto *task = host->property("task").value<QQuickItem *>();
    QVERIFY(task);

    QSignalSpy positionChanges(host.get(), SIGNAL(followingXChanged()));
    QVERIFY(task->setProperty("visible", true));
    QCOMPARE(host->property("followingX").toReal(), 100.0);
    QVERIFY(task->setProperty("zoomLength", 0.0));
    QVERIFY(task->setProperty("initialTaskGeometryReady", true));
    QCOMPARE(positionChanges.count(), 0);

    QVERIFY(task->setProperty("zoomLength", 0.5));
    QCOMPARE(host->property("followingX").toReal(), 124.0);
    QVERIFY(task->setProperty("zoomLength", 1.0));
    QCOMPARE(host->property("followingX").toReal(), 148.0);
    QCOMPARE(positionChanges.count(), 2);

    QVERIFY(task->setProperty("waitingForLauncherPosition", true));
    QCOMPARE(host->property("followingX").toReal(), 100.0);
    QVERIFY(task->setProperty("waitingForLauncherPosition", false));
    QCOMPARE(host->property("followingX").toReal(), 148.0);
}

void QmlSmokeTest::launcherPositionPendingCoversDelayedMove()
{
    QQmlEngine engine;
    auto host = createQmlObject(engine, QByteArrayLiteral(R"(
import QtQuick
import "plasmoid/package/contents/ui" as Tasks
Item {
    id: host
    property alias manager: manager
    property int moves: 0
    property bool firstPending: manager.launcherPositionIsPending("file:///latte-separator1.desktop")
    property bool pendingDuringMove: false
    property string moveOrder: ""
    property QtObject appletAbilities: QtObject {
        property QtObject launchers: QtObject {
            signal launcherInRemoving(string launcherUrl)
            signal launcherInAdding(string launcherUrl)
            signal launcherInMoving(string launcherUrl, int pos)
            function syncLauncherOrder() {}
            function validateSyncedLaunchersOrder() {}
        }
    }
    property QtObject tasksModel: QtObject {
        function move(from, to) {
            if (host.moves === 0) {
                host.pendingDuringMove = host.firstPending;
            }
            ++host.moves;
            host.moveOrder += from + ":" + to + ";";
        }
    }
    Tasks.TasksExtendedManager { id: manager }
}
)"), QUrl::fromLocalFile(QStringLiteral(LATTE_SOURCE_DIR "/launcher-position-pending.qml")));
    QVERIFY(host);
    auto *manager = host->property("manager").value<QObject *>();
    QVERIFY(manager);

    const QString launcher = QStringLiteral("file:///latte-separator1.desktop");
    QVERIFY(QMetaObject::invokeMethod(manager, "addLauncherToBeMoved", Q_ARG(QVariant, launcher), Q_ARG(QVariant, 3)));
    QVERIFY(host->property("firstPending").toBool());
    QVariant pending;
    QVERIFY(QMetaObject::invokeMethod(manager, "launcherPositionIsPending", Q_RETURN_ARG(QVariant, pending), Q_ARG(QVariant, launcher)));
    QVERIFY(pending.toBool());
    QVERIFY(QMetaObject::invokeMethod(manager, "moveLauncherToCorrectPos", Q_ARG(QVariant, launcher), Q_ARG(QVariant, 8)));
    QVERIFY(QMetaObject::invokeMethod(manager, "launcherPositionIsPending", Q_RETURN_ARG(QVariant, pending), Q_ARG(QVariant, launcher)));
    QVERIFY(pending.toBool());
    QTRY_VERIFY((QMetaObject::invokeMethod(manager, "launcherPositionIsPending", Q_RETURN_ARG(QVariant, pending), Q_ARG(QVariant, launcher)), !pending.toBool()));
    QCOMPARE(host->property("moves").toInt(), 1);
    QVERIFY(host->property("pendingDuringMove").toBool());
    QVERIFY(!host->property("firstPending").toBool());

    const QString secondLauncher = QStringLiteral("file:///latte-separator2.desktop");
    const QString thirdLauncher = QStringLiteral("file:///latte-separator3.desktop");
    QVERIFY(QMetaObject::invokeMethod(manager, "addLauncherToBeMoved", Q_ARG(QVariant, secondLauncher), Q_ARG(QVariant, 4)));
    QVERIFY(QMetaObject::invokeMethod(manager, "addLauncherToBeMoved", Q_ARG(QVariant, thirdLauncher), Q_ARG(QVariant, 5)));
    QVERIFY(QMetaObject::invokeMethod(manager, "moveLauncherToCorrectPos", Q_ARG(QVariant, secondLauncher), Q_ARG(QVariant, 9)));
    QVERIFY(QMetaObject::invokeMethod(manager, "moveLauncherToCorrectPos", Q_ARG(QVariant, thirdLauncher), Q_ARG(QVariant, 10)));
    QVERIFY(QMetaObject::invokeMethod(manager, "launcherPositionIsPending", Q_RETURN_ARG(QVariant, pending), Q_ARG(QVariant, secondLauncher)));
    QVERIFY(pending.toBool());
    QVERIFY(QMetaObject::invokeMethod(manager, "launcherPositionIsPending", Q_RETURN_ARG(QVariant, pending), Q_ARG(QVariant, thirdLauncher)));
    QVERIFY(pending.toBool());
    QTRY_COMPARE(host->property("moves").toInt(), 3);
    QCOMPARE(host->property("moveOrder").toString(), QStringLiteral("8:3;9:4;10:5;"));
    QVERIFY(QMetaObject::invokeMethod(manager, "launcherPositionIsPending", Q_RETURN_ARG(QVariant, pending), Q_ARG(QVariant, secondLauncher)));
    QVERIFY(!pending.toBool());
    QVERIFY(QMetaObject::invokeMethod(manager, "launcherPositionIsPending", Q_RETURN_ARG(QVariant, pending), Q_ARG(QVariant, thirdLauncher)));
    QVERIFY(!pending.toBool());
}

void QmlSmokeTest::positionedSyncedLauncherRequestsDelayedMove()
{
    QQmlEngine engine;
    auto launchers = createQmlObject(engine, QByteArrayLiteral(R"(
import QtQuick
Item {
    property int group: 1
    property string groupId: "group"
    property int added: 0
    property int syncs: 0
    property int movingPosition: -1
    signal launcherInMoving(string launcherUrl, int pos)
    signal launcherChanged(string launcherUrl)
    onLauncherInMoving: function(launcherUrl, pos) { movingPosition = pos; }
    function addLauncherToModel(launcher) { ++added; return true; }
    function syncLaunchers() { ++syncs; }
}
)"), QUrl(QStringLiteral("qrc:/positioned-syncer-launchers.qml")));
    QVERIFY(launchers);
    auto plasmoid = createQmlObject(engine, QByteArrayLiteral("import QtQml\nQtObject { property int id: 42 }"), QUrl(QStringLiteral("qrc:/syncer-plasmoid.qml")));
    QVERIFY(plasmoid);
    QQmlContext context(engine.rootContext());
    context.setContextProperty(QStringLiteral("_launchers"), launchers.get());
    context.setContextProperty(QStringLiteral("plasmoid"), plasmoid.get());
    context.setContextProperty(QStringLiteral("bridge"), QVariant::fromValue(static_cast<QObject *>(nullptr)));
    context.setContextProperty(QStringLiteral("tasksModel"), launchers.get());

    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(
        LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/abilities/launchers/Syncer.qml")));
    std::unique_ptr<QObject> syncer(component.create(&context));
    QVERIFY(syncer);
    QVERIFY(QMetaObject::invokeMethod(syncer.get(), "addSyncedLauncherAtPosition",
                                      Q_ARG(QVariant, 1),
                                      Q_ARG(QVariant, QStringLiteral("file:///latte-separator1.desktop")),
                                      Q_ARG(QVariant, 3)));
    QCOMPARE(launchers->property("movingPosition").toInt(), 3);
    QCOMPARE(launchers->property("added").toInt(), 1);
    QCOMPARE(launchers->property("syncs").toInt(), 0);
}

void QmlSmokeTest::parabolicItemZoomRecoveryLoadsFromSource()
{
    QTemporaryDir importRoot;
    QQmlEngine engine;
    addLatteCoreImport(engine, importRoot);
    addLatteComponentsImport(engine, importRoot);
    addFakePlasmaCoreImport(engine, importRoot);

    QQmlContext context(engine.rootContext());
    ParabolicTargetStub parabolicTarget;
    AbilityItemStub abilityItem(&parabolicTarget);
    PlasmoidStub plasmoid;
    QVariantMap theme{
        {QStringLiteral("textColor"), QColor(Qt::white)},
        {QStringLiteral("backgroundColor"), QColor(Qt::black)},
    };

    context.setContextProperty(QStringLiteral("abilityItem"), &abilityItem);
    context.setContextProperty(QStringLiteral("plasmoid"), &plasmoid);
    context.setContextProperty(QStringLiteral("theme"), theme);
    context.setContextProperty(QStringLiteral("latteBridge"), QVariant());

    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(LATTE_PARABOLIC_ITEM_QML)));
    std::unique_ptr<QObject> object(component.create(&context));
    if (!object) {
        qWarning() << component.errors();
    }

    QVERIFY(object);
    QVERIFY(object->setProperty("zoom", 1.6));
    QVERIFY(object->setProperty("zoomLength", 1.6));
    QVERIFY(object->setProperty("zoomThickness", 1.6));

    plasmoid.emitFormFactorChanged();
    QCOMPARE(object->property("zoom").toReal(), 1.0);
    QCOMPARE(object->property("zoomLength").toReal(), 1.0);
    QCOMPARE(object->property("zoomThickness").toReal(), 1.0);

    QVERIFY(object->setProperty("zoom", 1.4));
    QVERIFY(object->property("isZoomed").toBool());
    const int removedBefore = abilityItem.needBothAxisRemoveCount();
    QVariant ignoredReturn;
    QVERIFY(QMetaObject::invokeMethod(object.get(), "sendEndOfNeedBothAxisAnimation", Q_RETURN_ARG(QVariant, ignoredReturn)));
    QVERIFY(!object->property("isZoomed").toBool());
    QCOMPARE(abilityItem.needBothAxisRemoveCount(), removedBefore + 1);
    QVERIFY(abilityItem.needBothAxisAddCount() > 0);
}

void QmlSmokeTest::hiddenSpacerRetainsSlotWhileSeparatorStateSettles()
{
    QTemporaryDir importRoot;
    QQmlEngine engine;
    addLatteCoreImport(engine, importRoot);
    addFakePlasmaCoreImport(engine, importRoot);

    auto host = createQmlObject(engine, R"(
import QtQuick
import org.kde.plasma.core as PlasmaCore
Item {
    property QtObject abilityItem: QtObject {
        property QtObject parabolicItem: QtObject {
            property int width: 48
            property bool isUpdatingOnlySpacers: false
        }
        property QtObject abilities: QtObject {
            property QtObject parabolic: QtObject { property bool isEnabled: true }
            property QtObject metrics: QtObject {
                property int iconSize: 48
                property QtObject totals: QtObject { property int lengthEdges: 0 }
            }
            property QtObject indexer: QtObject {
                property int lastVisibleItemIndex: 1
                property var separators: []
            }
            property QtObject debug: QtObject { property bool spacersEnabled: false }
        }
        property bool tailItemIsVisibleSeparator: true
        property bool headItemIsVisibleSeparator: false
        property bool isSeparator: false
        property bool isHidden: false
        property bool keepSeparatorSpacingOnRemoval: false
        property bool isHiddenSpacerForcedShow: false
        property bool containsMouse: false
        property bool isHorizontal: true
        property bool isHiddenSpacerAnimated: false
        property int animationTime: 0
    }
    property QtObject plasmoid: QtObject { property int formFactor: PlasmaCore.Types.Horizontal }
    property int itemIndex: 1
    property int index: 1
}
)", QUrl(QStringLiteral("qrc:/hidden-spacer-host.qml")));
    QVERIFY(host);

    QQmlContext context(engine.rootContext());
    context.setContextObject(host.get());
    QQmlComponent spacerComponent(&engine, QUrl::fromLocalFile(QStringLiteral(LATTE_HIDDEN_SPACER_QML)));
    std::unique_ptr<QObject> spacer(spacerComponent.create(&context));
    if (!spacer) {
        qWarning() << spacerComponent.errors();
    }
    QVERIFY(spacer);

    auto *abilityItem = host->property("abilityItem").value<QObject *>();
    QVERIFY(abilityItem);
    QTRY_COMPARE(spacer->property("nHiddenSize").toReal(), 2.0);
    QTRY_COMPARE(spacer->property("width").toReal(), 2.0);

    // Indexer publishes separator/hidden changes separately. During the
    // 32ms loop-breaking pause, the slot must retain its last settled width.
    QVERIFY(abilityItem->setProperty("tailItemIsVisibleSeparator", false));
    QCOMPARE(spacer->property("nHiddenSize").toReal(), 2.0);
    QCOMPARE(spacer->property("width").toReal(), 2.0);
    QTRY_COMPARE_WITH_TIMEOUT(spacer->property("nHiddenSize").toReal(), 0.0, 250);
    QTRY_COMPARE(spacer->property("width").toReal(), 0.0);

    QVERIFY(abilityItem->setProperty("tailItemIsVisibleSeparator", true));
    QCOMPARE(spacer->property("nHiddenSize").toReal(), 0.0);
    QCOMPARE(spacer->property("width").toReal(), 0.0);
    QTRY_COMPARE_WITH_TIMEOUT(spacer->property("nHiddenSize").toReal(), 2.0, 250);
    QTRY_COMPARE(spacer->property("width").toReal(), 2.0);

    // A pinned task replacement retains the outgoing gap while its index
    // is invalid, then seeds the incoming gap until that row's indexes settle.
    QVERIFY(abilityItem->setProperty("keepSeparatorSpacingOnRemoval", true));
    QVERIFY(host->setProperty("itemIndex", -1));
    QVERIFY(abilityItem->setProperty("tailItemIsVisibleSeparator", false));
    QCOMPARE(spacer->property("nHiddenSize").toReal(), 2.0);
    std::unique_ptr<QObject> incomingSpacer(spacerComponent.create(&context));
    QVERIFY(incomingSpacer);
    QVERIFY(QMetaObject::invokeMethod(incomingSpacer.get(), "inheritSeparatorSpace", Q_ARG(QVariant, QVariant::fromValue(spacer.get()))));
    QCOMPARE(incomingSpacer->property("settledSeparatorSpace").toReal(), 2.0);
    QVERIFY(host->setProperty("itemIndex", 1));
    QCOMPARE(incomingSpacer->property("nHiddenSize").toReal(), 2.0);
    QTRY_COMPARE_WITH_TIMEOUT(incomingSpacer->property("nHiddenSize").toReal(), 0.0, 250);
    QTRY_COMPARE_WITH_TIMEOUT(spacer->property("nHiddenSize").toReal(), 0.0, 250);
    QVERIFY(abilityItem->setProperty("tailItemIsVisibleSeparator", true));
    QTRY_COMPARE_WITH_TIMEOUT(spacer->property("nHiddenSize").toReal(), 2.0, 250);
    QVERIFY(abilityItem->setProperty("keepSeparatorSpacingOnRemoval", false));

    // Launcher-order commits can destroy every old delegate at once.
    // The replacement initializes from plain gap values captured beforehand.
    const QVariant savedSpacer = QVariantMap{{QStringLiteral("settledSeparatorSpace"), 2.0}};
    QVERIFY(abilityItem->setProperty("tailItemIsVisibleSeparator", false));
    std::unique_ptr<QObject> resetSpacer(spacerComponent.create(&context));
    QVERIFY(resetSpacer);
    QVERIFY(QMetaObject::invokeMethod(resetSpacer.get(), "inheritSeparatorSpace", Q_ARG(QVariant, savedSpacer)));
    QCOMPARE(resetSpacer->property("width").toReal(), 2.0);
    QTRY_COMPARE_WITH_TIMEOUT(resetSpacer->property("width").toReal(), 0.0, 250);
    QVERIFY(abilityItem->setProperty("tailItemIsVisibleSeparator", true));
    QTRY_COMPARE_WITH_TIMEOUT(spacer->property("width").toReal(), 2.0, 250);

    // An invalid delegate index releases its slot even if the settle timer is
    // running, matching the prior immediate departure behavior.
    QVERIFY(abilityItem->setProperty("tailItemIsVisibleSeparator", false));
    QCOMPARE(spacer->property("nHiddenSize").toReal(), 2.0);
    QVERIFY(host->setProperty("itemIndex", -1));
    QCOMPARE(spacer->property("nHiddenSize").toReal(), 0.0);
    QCOMPARE(spacer->property("width").toReal(), 0.0);
}

void
QmlSmokeTest::taskRemovalDefersGeometryFeedback_data()
{
    QTest::addColumn<bool>("vertical");
    QTest::addColumn<int>("duration");
    QTest::addColumn<bool>("destroyPending");
    QTest::addColumn<bool>("separator");
    for (bool vertical : { false, true }) {
        for (int duration : { 0, 40 }) {
            for (bool destroyPending : { false, true }) {
                const QByteArray name = QByteArray::number(vertical) + '-' + QByteArray::number(duration) + '-' + QByteArray::number(destroyPending);
                QTest::newRow(name.constData()) << vertical << duration << destroyPending << false;
            }
        }
        const QByteArray name = QByteArray("separator-") + QByteArray::number(vertical);
        QTest::newRow(name.constData()) << vertical << 0 << false << true;
    }
}

void
QmlSmokeTest::taskRemovalDefersGeometryFeedback()
{
    QFETCH(bool, vertical);
    QFETCH(int, duration);
    QFETCH(bool, destroyPending);
    QFETCH(bool, separator);
    QTemporaryDir importRoot;
    QQmlEngine engine;
    addLatteCoreImport(engine, importRoot);
    addLatteComponentsImport(engine, importRoot);
    addFakePlasmaCoreImport(engine, importRoot);
    QStringList warnings;
    connect(&engine, &QQmlEngine::warnings, &engine, [&warnings](const QList<QQmlError> &errors) {
        for (const QQmlError &error : errors) {
            warnings.append(error.toString());
        }
    });
    ParabolicTargetStub target;
    AbilityItemStub abilityItem(&target, nullptr, !vertical, separator);
    PlasmoidStub plasmoid;
    EventSinkStub events;
    engine.rootContext()->setContextProperty(QStringLiteral("abilityItem"), &abilityItem);
    engine.rootContext()->setContextProperty(QStringLiteral("plasmoid"), &plasmoid);
    engine.rootContext()->setContextProperty(QStringLiteral("events"), &events);
    engine.rootContext()->setContextProperty(QStringLiteral("animationDuration"), duration);
    engine.rootContext()->setContextProperty(QStringLiteral("theme"), QVariantMap{ { QStringLiteral("textColor"), QColor(Qt::white) } });
    engine.rootContext()->setContextProperty(QStringLiteral("latteBridge"), QVariant());
    const QByteArray source = QStringLiteral(R"(
import QtQuick
import "%1" as Items
import "%2" as Animations
ListView {
    id: view
    width: 400; height: 400
    property var delegateObject: null
    property bool armed: false
    property int starts: 0
    property int finishes: 0
    property int removals: 0
    property int destructions: 0
    model: ListModel { id: entries; ListElement { name: "task" } }
    delegate: Item {
        id: task
        width: parabolic.width
        height: parabolic.height
        property alias parabolic: parabolic
        property bool held: ListView.delayRemove
        function geometryChanged() {
            if (view.armed) {
                view.armed = false;
                entries.remove(0);
                view.forceLayout();
            }
        }
        onWidthChanged: geometryChanged()
        onHeightChanged: geometryChanged()
        Component.onCompleted: view.delegateObject = task
        Component.onDestruction: view.destructions++
        Items.ParabolicItem { id: parabolic }
        Animations.RemovalScheduler { id: scheduler; task: task; animation: removal }
        ListView.onRemove: {
            view.removals++;
            scheduler.schedule();
            scheduler.schedule(); // Repeated notifications must coalesce.
        }
        SequentialAnimation {
            id: removal
            PropertyAction { target: task; property: "ListView.delayRemove"; value: !abilityItem.isSeparator }
            ScriptAction { script: { view.starts++; events.addEvent("removal"); } }
            PropertyAction { target: parabolic; property: "zoomThickness"; value: 0 }
            NumberAnimation { target: parabolic; property: "zoomLength"; to: 0; duration: animationDuration }
            ScriptAction { script: view.finishes++ }
            PropertyAction { target: task; property: "ListView.delayRemove"; value: false }
        }
    }
    function exercise() {
        if (abilityItem.isSeparator) {
            entries.remove(0);
            forceLayout();
            return;
        }
        armed = true;
        delegateObject.parabolic.zoomThickness = 0.5;
    }
}
)")
                                .arg(QUrl::fromLocalFile(QStringLiteral(LATTE_SOURCE_DIR "/declarativeimports/abilities/items/basicitem")).toString(),
                                     QUrl::fromLocalFile(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/task/animations")).toString())
                                .toUtf8();
    auto object = createQmlObject(engine, source, QUrl(QStringLiteral("qrc:/removal-feedback.qml")));
    QVERIFY(object);
    QTRY_VERIFY(object->property("delegateObject").value<QObject *>());
    QObject *delegate = object->property("delegateObject").value<QObject *>();
    QVERIFY(QMetaObject::invokeMethod(object.get(), "exercise"));
    QCOMPARE(object->property("removals").toInt(), 1);
    QCOMPARE(object->property("starts").toInt(), 0);
    QVERIFY(delegate->property("held").toBool());
    if (destroyPending) {
        object.reset();
        QTest::qWait(80);
        QCOMPARE(events.addCount(), 0);
    } else {
        QTRY_COMPARE(object->property("finishes").toInt(), 1);
        QTRY_COMPARE(object->property("destructions").toInt(), 1);
        QCOMPARE(object->property("starts").toInt(), 1);
        QCOMPARE(events.addCount(), 1);
    }
    QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join(QLatin1Char('\n'))));
}

void QmlSmokeTest::taskRowReplacementHandoffUsesOutgoingGeometry()
{
    QFile mainQml(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/main.qml"));
    QVERIFY(mainQml.open(QFile::ReadOnly));
    const QString mainSource = QString::fromUtf8(mainQml.readAll());
    const int replacementResolver = mainSource.indexOf(QStringLiteral("function outgoingTaskRowReplacement("));
    QVERIFY(replacementResolver >= 0);
    const int replacementResolverEnd = mainSource.indexOf(QStringLiteral("\n    function "), replacementResolver + 1);
    QVERIFY(replacementResolverEnd > replacementResolver);
    const QString resolverSource = mainSource.mid(replacementResolver, replacementResolverEnd - replacementResolver);
    QVERIFY(resolverSource.contains(QStringLiteral("findAdjacentTaskRowReplacement")));
    QVERIFY(!resolverSource.contains(QStringLiteral("launcherExists")));

    QQmlEngine engine;
    TaskRowAppletAbilitiesStub appletAbilities;
    engine.rootContext()->setContextProperty(QStringLiteral("appletAbilities"), &appletAbilities);
    QStringList warnings;
    connect(&engine, &QQmlEngine::warnings, &engine, [&warnings](const QList<QQmlError> &errors) {
        for (const QQmlError &error : errors) {
            warnings.append(error.toString());
        }
    });

    const QByteArray source = QStringLiteral(R"(
import QtQuick
import "%1" as Tasks
Item {
    id: testRoot
    width: 400; height: 100
    property alias incoming: incoming
    property alias outgoing: outgoing
    property alias wrongOutgoing: wrongOutgoing
    property var serializedLauncherList: []
    Tasks.TasksExtendedManager { id: manager }
    Item {
        id: incoming
        property string launcherUrl: "applications:ghostty.desktop"
        property string launcherUrlWithIcon: "applications:ghostty.desktop"
        property bool isLauncher: false
        property bool isWindow: true
        property bool isStartup: false
        property bool isGroupParent: false
        property bool isSeparator: false
        property int windowsCount: 1
        property int itemIndex: 4
        property int previousModelIndex: 4
        property Item pendingTaskRowReplacementOutgoing: null
        property bool waitingForTaskRowReplacement: false
        property int scheduledResumes: 0
        property int synchronousResumes: 0
        property QtObject parabolicItem: QtObject {
            property real zoomLength: 1
            property real zoomThickness: 1
            property real opacity: 1
        }
        property real rowLength: 74 * parabolicItem.zoomLength
        function scheduleTaskRowReplacementAnimation() {
            scheduledResumes++;
            showWindowAnimation.showWindow();
        }
        function resumeTaskRowReplacementAnimation() {
            synchronousResumes++;
            showWindowAnimation.showWindow();
        }
    }
    Item {
        id: outgoing
        objectName: "TaskItem"
        property string launcherUrl: "applications:ghostty.desktop"
        property string launcherUrlWithIcon: "applications:ghostty.desktop"
        property bool isLauncher: true
        property bool isWindow: false
        property bool isStartup: false
        property bool isGroupParent: false
        property bool isSeparator: false
        property int windowsCount: 1
        property int itemIndex: -1
        property int previousModelIndex: 4
        property Item taskRowReplacementSuccessor: null
    }
    Item { id: wrongOutgoing }
    QtObject {
        id: showWindowAnimation
        function showWindow() {
            incoming.parabolicItem.zoomLength = 1;
            incoming.parabolicItem.zoomThickness = 1;
            incoming.parabolicItem.opacity = 1;
        }
    }
    function findOutgoing() {
        return manager.findAdjacentTaskRowReplacement(incoming, [incoming, outgoing]);
    }
    function beginHandoff() {
        var candidate = findOutgoing();
        return candidate && manager.holdTaskRowForReplacement(incoming, candidate);
    }
    function beginCloseHandoff() {
        serializedLauncherList = undefined;
        incoming.isLauncher = true;
        incoming.isWindow = false;
        outgoing.isLauncher = false;
        outgoing.isWindow = true;
        var candidate = findOutgoing();
        return candidate && manager.holdTaskRowForReplacement(incoming, candidate);
    }
    function restoreOpenHandoff() {
        incoming.isLauncher = false;
        incoming.isWindow = true;
        outgoing.isLauncher = true;
        outgoing.isWindow = false;
    }
    function releaseHandoff() {
        return manager.releaseTaskRowReplacement(incoming, outgoing, true);
    }
    function releaseWrongHandoff() {
        return manager.releaseTaskRowReplacement(incoming, wrongOutgoing);
    }
}
)").arg(QUrl::fromLocalFile(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui")).toString()).toUtf8();

    auto object = createQmlObject(engine, source, QUrl(QStringLiteral("qrc:/task-row-handoff.qml")));
    QVERIFY(object);
    QObject *incoming = object->property("incoming").value<QObject *>();
    QObject *outgoing = object->property("outgoing").value<QObject *>();
    QObject *wrongOutgoing = object->property("wrongOutgoing").value<QObject *>();
    QVERIFY(incoming);
    QVERIFY(outgoing);
    QVERIFY(wrongOutgoing);
    QCOMPARE(incoming->property("rowLength").toReal(), 74.0);
    QCOMPARE(object->property("serializedLauncherList").toStringList().size(), 0);

    QVariant accepted;
    QVERIFY(QMetaObject::invokeMethod(object.get(), "beginHandoff", Q_RETURN_ARG(QVariant, accepted)));
    QVERIFY(accepted.toBool());
    QCOMPARE(incoming->property("rowLength").toReal(), 0.0);
    QObject *parabolicItem = incoming->property("parabolicItem").value<QObject *>();
    QVERIFY(parabolicItem);
    QCOMPARE(parabolicItem->property("opacity").toReal(), 0.0);

    QVERIFY(QMetaObject::invokeMethod(object.get(), "releaseWrongHandoff", Q_RETURN_ARG(QVariant, accepted)));
    QVERIFY(!accepted.toBool());
    QCOMPARE(incoming->property("rowLength").toReal(), 0.0);

    QVERIFY(QMetaObject::invokeMethod(object.get(), "releaseHandoff", Q_RETURN_ARG(QVariant, accepted)));
    QVERIFY(accepted.toBool());
    QCOMPARE(incoming->property("rowLength").toReal(), 74.0);
    QCOMPARE(parabolicItem->property("opacity").toReal(), 1.0);
    QCOMPARE(incoming->property("synchronousResumes").toInt(), 1);
    QCOMPARE(incoming->property("scheduledResumes").toInt(), 0);
    QVERIFY(!incoming->property("waitingForTaskRowReplacement").toBool());

    QVERIFY(QMetaObject::invokeMethod(object.get(), "beginCloseHandoff", Q_RETURN_ARG(QVariant, accepted)));
    QVERIFY(accepted.toBool());
    QVERIFY(!object->property("serializedLauncherList").isValid());
    QCOMPARE(incoming->property("rowLength").toReal(), 0.0);
    QVERIFY(QMetaObject::invokeMethod(object.get(), "releaseHandoff", Q_RETURN_ARG(QVariant, accepted)));
    QVERIFY(accepted.toBool());
    QCOMPARE(incoming->property("rowLength").toReal(), 74.0);
    QVERIFY(QMetaObject::invokeMethod(object.get(), "restoreOpenHandoff"));

    QVERIFY(outgoing->setProperty("isGroupParent", true));
    QVERIFY(QMetaObject::invokeMethod(object.get(), "beginHandoff", Q_RETURN_ARG(QVariant, accepted)));
    QVERIFY(!accepted.toBool());
    QCOMPARE(incoming->property("rowLength").toReal(), 74.0);

    QVERIFY(outgoing->setProperty("isGroupParent", false));
    QVERIFY(outgoing->setProperty("previousModelIndex", 0));
    QVERIFY(QMetaObject::invokeMethod(object.get(), "beginHandoff", Q_RETURN_ARG(QVariant, accepted)));
    QVERIFY(!accepted.toBool());
    QVERIFY(outgoing->setProperty("previousModelIndex", 4));

    QVERIFY(outgoing->setProperty("launcherUrl", QStringLiteral("applications:other.desktop")));
    QVERIFY(outgoing->setProperty("launcherUrlWithIcon", QStringLiteral("applications:other.desktop")));
    QVERIFY(QMetaObject::invokeMethod(object.get(), "beginHandoff", Q_RETURN_ARG(QVariant, accepted)));
    QVERIFY(!accepted.toBool());
    QVERIFY(outgoing->setProperty("launcherUrl", QStringLiteral("applications:ghostty.desktop")));
    QVERIFY(outgoing->setProperty("launcherUrlWithIcon", QStringLiteral("applications:ghostty.desktop")));

    QVERIFY(outgoing->setProperty("isSeparator", true));
    QVERIFY(QMetaObject::invokeMethod(object.get(), "beginHandoff", Q_RETURN_ARG(QVariant, accepted)));
    QVERIFY(!accepted.toBool());
    QVERIFY(outgoing->setProperty("isSeparator", false));

    QVERIFY(QMetaObject::invokeMethod(object.get(), "beginHandoff", Q_RETURN_ARG(QVariant, accepted)));
    QVERIFY(accepted.toBool());
    QVERIFY(incoming->setProperty("windowsCount", 2));
    QVERIFY(QMetaObject::invokeMethod(object.get(), "releaseHandoff", Q_RETURN_ARG(QVariant, accepted)));
    QVERIFY(accepted.toBool());
    QVERIFY(QMetaObject::invokeMethod(object.get(), "beginHandoff", Q_RETURN_ARG(QVariant, accepted)));
    QVERIFY(!accepted.toBool());
    QCOMPARE(incoming->property("rowLength").toReal(), 74.0);
    QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join(QLatin1Char('\n'))));
}

void QmlSmokeTest::taskRowReplacementHandoffPreservesListViewSlot()
{
    QQuickView quickView;
    quickView.resize(600, 100);
    QStringList warnings;
    connect(quickView.engine(), &QQmlEngine::warnings, &quickView, [&warnings](const QList<QQmlError> &errors) {
        for (const QQmlError &error : errors) {
            warnings.append(error.toString());
        }
    });

    const QByteArray source = QStringLiteral(R"(
import QtQuick
import "%1" as Tasks
import "%2" as Animations
Item {
    id: root
    width: 600; height: 100
    property int openCalls: 0
    property alias view: view
    property alias handoffs: view.handoffs
    property QtObject appletAbilities: QtObject {
        property QtObject launchers: QtObject {
            signal launcherInRemoving(string launcherUrl)
            signal launcherInAdding(string launcherUrl)
            signal launcherInMoving(string launcherUrl, int position)
        }
    }
    Timer {
        id: insertRow
        interval: 20
        onTriggered: {
            view.model.insert(1, { launcherUrl: "applications:ghostty.desktop", isLauncher: false, isWindow: true, isStartup: false });
        }
    }
    Tasks.TasksExtendedManager { id: manager }
    ListView {
        id: view
        width: 600; height: 80
        orientation: ListView.Horizontal
        property int handoffs: 0
        property int replacementMatches: 0
        property int replacementHolds: 0
        property int removalEvents: 0
        property int initialGhostReady: 0
        property real handoffContentWidth: 0
        function ghostRowWidths() {
            const widths = [];
            const delegates = contentItem.children;
            for (let i = 0; i < delegates.length; ++i) {
                if (delegates[i].launcherUrl === "applications:ghostty.desktop") {
                    widths.push(delegates[i].width);
                }
            }
            return widths;
        }
        model: ListModel {
            id: entries
            ListElement { launcherUrl: "applications:first.desktop"; isLauncher: true; isWindow: false; isStartup: false }
            ListElement { launcherUrl: "applications:ghostty.desktop"; isLauncher: true; isWindow: false; isStartup: false }
            ListElement { launcherUrl: "applications:last.desktop"; isLauncher: true; isWindow: false; isStartup: false }
        }
        delegate: Item {
            id: row
            objectName: "TaskItem"
            property string launcherUrl: ""
            property string launcherUrlWithIcon: launcherUrl
            property bool isLauncher: false
            property bool isWindow: false
            property bool isStartup: false
            property bool isGroupParent: false
            property bool isSeparator: false
            property int windowsCount: 1
            property int itemIndex: index
            property int previousModelIndex: -1
            onItemIndexChanged: {
                if (itemIndex >= 0) {
                    previousModelIndex = itemIndex;
                }
            }
            property Item pendingTaskRowReplacementOutgoing: null
            property bool waitingForTaskRowReplacement: false
            property Item taskRowReplacementSuccessor: null
            property int showCalls: 0
            property QtObject parabolicItem: QtObject {
                property real zoomLength: 1
                property real zoomThickness: 1
                property real opacity: 1
            }
            property bool checkingReplacement: false
            // Model an icon with separator spacing independent of icon zoom.
            width: waitingForTaskRowReplacement || checkingReplacement ? 0 : 70 * parabolicItem.zoomLength + 4
            height: 60
            function scheduleTaskRowReplacementAnimation() {
                if (!waitingForTaskRowReplacement) {
                    parabolicItem.zoomLength = 1;
                    parabolicItem.zoomThickness = 1;
                    parabolicItem.opacity = 1;
                    showCalls++;
                }
            }
            function resumeTaskRowReplacementAnimation() {
                if (!waitingForTaskRowReplacement) {
                    parabolicItem.zoomLength = 1;
                    parabolicItem.zoomThickness = 1;
                    parabolicItem.opacity = 1;
                    showCalls++;
                }
            }
            function findReplacement() {
                const candidate = manager.findAdjacentTaskRowReplacement(row, view.contentItem.children);
                if (candidate) {
                    view.replacementMatches++;
                    if (manager.holdTaskRowForReplacement(row, candidate)) {
                        view.replacementHolds++;
                    }
                }
            }
            Timer {
                id: retryReplacement
                interval: 0
                onTriggered: {
                    row.findReplacement();
                    row.checkingReplacement = false;
                }
            }
            Component.onCompleted: {
                launcherUrl = model.launcherUrl;
                isLauncher = model.isLauncher;
                isWindow = model.isWindow;
                isStartup = model.isStartup;
                if (isLauncher && launcherUrl === "applications:ghostty.desktop") {
                    view.initialGhostReady++;
                }
                previousModelIndex = itemIndex;
                row.findReplacement();
                if (!waitingForTaskRowReplacement
                        && manager.findAdjacentTaskRowReplacement(row, view.contentItem.children, true)) {
                    checkingReplacement = true;
                    retryReplacement.start();
                }
            }
            Animations.RemovalScheduler { id: scheduler; task: row; animation: removal }
            SequentialAnimation {
                id: removal
                // Hold the old slot until the replacement is ready, then
                // model the production instant replacement removal.
                PauseAnimation { duration: 100 }
                PropertyAction { target: row.parabolicItem; property: "zoomLength"; value: 0 }
                ScriptAction {
                    script: {
                        if (row.taskRowReplacementSuccessor) {
                            manager.releaseTaskRowReplacement(row.taskRowReplacementSuccessor, row, true);
                            view.handoffs++;
                            view.handoffContentWidth = view.contentWidth;
                        }
                    }
                }
                PropertyAction { target: row; property: "ListView.delayRemove"; value: false }
            }
            ListView.onRemove: {
                view.removalEvents++;
                scheduler.schedule();
            }
        }
    }
    function openWindow() {
        root.openCalls++;
        view.model.remove(1);
        insertRow.start();
    }
    function closeWindow() {
        view.model.insert(1, { launcherUrl: "applications:ghostty.desktop", isLauncher: true, isWindow: false, isStartup: false });
        view.forceLayout();
        view.model.remove(2);
    }
}
)").arg(QUrl::fromLocalFile(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui")).toString(),
         QUrl::fromLocalFile(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/task/animations")).toString()).toUtf8();

    QTemporaryDir fixtureDirectory;
    QVERIFY(fixtureDirectory.isValid());
    QFile fixtureFile(fixtureDirectory.filePath(QStringLiteral("task-row-listview-handoff.qml")));
    QVERIFY(fixtureFile.open(QIODevice::WriteOnly));
    QCOMPARE(fixtureFile.write(source), source.size());
    fixtureFile.close();
    quickView.setSource(QUrl::fromLocalFile(fixtureFile.fileName()));
    QCOMPARE(quickView.status(), QQuickView::Ready);
    quickView.show();
    QTRY_VERIFY(quickView.rootObject());
    QObject *rootObject = quickView.rootObject();
    QObject *view = rootObject->property("view").value<QObject *>();
    QVERIFY(view);
    QTRY_COMPARE(view->property("initialGhostReady").toInt(), 1);
    QCOMPARE(view->property("contentWidth").toReal(), 222.0);

    QVector<qreal> frameContentWidths;
    QVector<QVariantList> frameGhostWidths;
    connect(&quickView, &QQuickWindow::frameSwapped, &quickView, [&] {
        frameContentWidths.append(view->property("contentWidth").toReal());
        QVariant widths;
        if (QMetaObject::invokeMethod(view, "ghostRowWidths", Q_RETURN_ARG(QVariant, widths))) {
            frameGhostWidths.append(widths.toList());
        }
    });

    QVERIFY(QMetaObject::invokeMethod(rootObject, "openWindow"));
    QTRY_COMPARE(view->property("handoffs").toInt(), 1);
    QCOMPARE(view->property("removalEvents").toInt(), 1);
    QCOMPARE(view->property("replacementMatches").toInt(), 1);
    QCOMPARE(view->property("replacementHolds").toInt(), 1);
    QCOMPARE(view->property("handoffs").toInt(), 1);
    QTRY_COMPARE(view->property("contentWidth").toReal(), 222.0);

    QVERIFY(QMetaObject::invokeMethod(rootObject, "closeWindow"));
    QTRY_COMPARE(view->property("handoffs").toInt(), 2);
    QTRY_COMPARE(view->property("contentWidth").toReal(), 222.0);
    QTest::qWait(50);
    QVERIFY(frameContentWidths.size() > 4);
    QCOMPARE(frameGhostWidths.size(), frameContentWidths.size());
    for (qsizetype frame = 0; frame < frameContentWidths.size(); ++frame) {
        QCOMPARE(frameContentWidths.at(frame), 222.0);
        int positiveGhostRows = 0;
        for (const QVariant &width : frameGhostWidths.at(frame)) {
            positiveGhostRows += width.toReal() > 0.5 ? 1 : 0;
        }
        QCOMPARE(positiveGhostRows, 1);
    }
    QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join(QLatin1Char('\n'))));
}

void QmlSmokeTest::globalMenuBackendStatusTransitionsLoadFromSource_data()
{
    QTest::addColumn<bool>("wrappedBackend");
    QTest::newRow("direct-backend") << false;
    QTest::newRow("plasmoid-backend") << true;
}

void QmlSmokeTest::globalMenuBackendStatusTransitionsLoadFromSource()
{
    QFETCH(bool, wrappedBackend);
    QFile file(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/applet/AppletItem.qml"));
    QVERIFY(file.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(file.readAll());
    const int dynamicPropertyStart = source.indexOf(QStringLiteral("    function readDynamicProperty(object, propertyName)"));
    const int dynamicPropertyEnd = source.indexOf(QStringLiteral("    // Plasma 6 passes"), dynamicPropertyStart);
    QVERIFY(dynamicPropertyStart >= 0 && dynamicPropertyEnd > dynamicPropertyStart);
    QString bindings;
    // Execute the production bindings with a Plasma 6-style visual item that
    // has no status property. The backend owns the notifying status instead.
    for (const QString &prefix : {QStringLiteral("width:"), QStringLiteral("height:"), QStringLiteral("enabled:"),
                                  QStringLiteral("readonly property bool isGlobalMenuApplet:"),
                                  QStringLiteral("readonly property bool keepVisibleOnHiddenStatus:"),
                                  QStringLiteral("property bool isHidden:"), QStringLiteral("readonly property QtObject statusSource:"),
                                  QStringLiteral("readonly property int status:")}) {
        const int start = source.indexOf(QStringLiteral("\n    ") + prefix);
        QVERIFY(start >= 0);
        if (prefix != QStringLiteral("property bool isHidden:") && prefix != QStringLiteral("readonly property int status:")
            && prefix != QStringLiteral("readonly property QtObject statusSource:")) {
            const int end = source.indexOf(QLatin1Char('\n'), start + 1);
            QVERIFY(end > start);
            bindings += source.mid(start, end - start);
            continue;
        }
        const QRegularExpression nextDeclaration(QStringLiteral("\n    (?:readonly )?property |\n\n"));
        const auto end = nextDeclaration.match(source, start + 1);
        QVERIFY(end.hasMatch());
        bindings += source.mid(start, end.capturedStart() - start);
    }

    QTemporaryDir importRoot;
    QQmlEngine engine;
    addFakePlasmaCoreImport(engine, importRoot);
    const QByteArray backend = wrappedBackend
        ? QByteArrayLiteral("property QtObject applet: QtObject { property QtObject plasmoid: nativeBackend }\nproperty QtObject backendAppletRef: applet\n")
        : QByteArrayLiteral("property QtObject applet: QtObject {}\nproperty QtObject backendAppletRef: nativeBackend\n");
    const QByteArray harness = QByteArrayLiteral(R"(
import QtQuick
import org.kde.plasma.core 2.0 as PlasmaCore
Item {
    property string pluginName: "org.kde.plasma.appmenu"
    property bool externalAppletDrawsAboveTasks: true
    property bool isInternalViewSplitter: false
    property bool isScheduledForDestruction: false
    property real computeWidth: 360
    property real computeHeight: 48
    property QtObject nativeBackend: QtObject {
        property int status: PlasmaCore.Types.ActiveStatus
    }
    function hideMenu() { nativeBackend.status = PlasmaCore.Types.HiddenStatus; }
    function showMenu() { nativeBackend.status = PlasmaCore.Types.ActiveStatus; }
    function openMenu() { nativeBackend.status = PlasmaCore.Types.NeedsAttentionStatus; }
)") + source.mid(dynamicPropertyStart, dynamicPropertyEnd - dynamicPropertyStart).toUtf8() + backend
                               + bindings.toUtf8() + QByteArrayLiteral("\n}");
    auto object = createQmlObject(engine, harness, QUrl(QStringLiteral("qrc:/global-menu-status.qml")));
    QVERIFY(object);
    QCOMPARE(object->property("isHidden").toBool(), false);
    QCOMPARE(object->property("width").toReal(), 360.0);
    QVERIFY(QMetaObject::invokeMethod(object.get(), "hideMenu"));
    QCOMPARE(object->property("isHidden").toBool(), true);
    QCOMPARE(object->property("width").toReal(), 0.0);
    QCOMPARE(object->property("height").toReal(), 0.0);
    QCOMPARE(object->property("enabled").toBool(), false);
    QVERIFY(QMetaObject::invokeMethod(object.get(), "showMenu"));
    QCOMPARE(object->property("isHidden").toBool(), false);
    QCOMPARE(object->property("width").toReal(), 360.0);
    QCOMPARE(object->property("height").toReal(), 48.0);
    QCOMPARE(object->property("enabled").toBool(), true);
    QVERIFY(QMetaObject::invokeMethod(object.get(), "openMenu"));
    QCOMPARE(object->property("isHidden").toBool(), false);
    QVERIFY(object->setProperty("pluginName", QStringLiteral("org.kde.plasma.kickoff")));
    QVERIFY(QMetaObject::invokeMethod(object.get(), "hideMenu"));
    QCOMPARE(object->property("isHidden").toBool(), false);
}

void
QmlSmokeTest::globalMenuNativeLengthUpdatesWithoutClassifier()
{
    QFile file(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/applet/ItemWrapper.qml"));
    QVERIFY(file.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(file.readAll());
    const int lengthProperty = source.indexOf(QStringLiteral("property: \"layoutLength\""));
    QVERIFY(lengthProperty >= 0);
    const int start = source.lastIndexOf(QStringLiteral("    Binding {"), lengthProperty);
    const int end = source.indexOf(QStringLiteral("    Binding {"), lengthProperty);
    QVERIFY(start >= 0);
    QVERIFY(end > start);
    const QByteArray harness = QByteArrayLiteral(R"(
import QtQuick
Item {
    id: wrapper
    property bool latteView: true
    property real zoomScale: 1.5
    property real layoutLength: 0
    property real appletMinimumLength: 24
    property real appletPreferredLength: -1
    property real appletMaximumLength: Infinity
    property bool isSeparator: false
    property var applet: ({})
    property QtObject appletItem: QtObject {
        property bool isGlobalMenuApplet: true
        property bool isAutoFillApplet: false
        property bool externalAppletUsesFixedSlotSizing: false
        property bool originalAppletBehavior: false
        property QtObject metrics: QtObject { property int iconSize: 48 }
    }
)") + source.mid(start, end - start).toUtf8() +
                               QByteArrayLiteral("\n}");
    QQmlEngine engine;
    auto object = createQmlObject(engine, harness, QUrl(QStringLiteral("qrc:/global-menu-length.qml")));
    QVERIFY(object);
    // Even with an unsettled zoom scale, short and long native menus update
    // synchronously without the 100ms icon-size classification timer.
    QCOMPARE(object->property("layoutLength").toReal(), 24.0);
    QVERIFY(object->setProperty("appletMinimumLength", 420.0));
    QCOMPARE(object->property("layoutLength").toReal(), 420.0);
    QVERIFY(object->setProperty("appletMinimumLength", 30.0));
    QCOMPARE(object->property("layoutLength").toReal(), 30.0);
    QVERIFY(object->setProperty("appletPreferredLength", 80.0));
    QCOMPARE(object->property("layoutLength").toReal(), 80.0);
    QVERIFY(object->setProperty("appletPreferredLength", -1.0));
    QVERIFY(object->setProperty("appletMinimumLength", 0.0));
    // Before AppletQuickItem creates its representation, it has no native
    // size hints. Keep non-zero bootstrap geometry so initialization can run.
    QCOMPARE(object->property("layoutLength").toReal(), 48.0);
}

void
QmlSmokeTest::globalMenuReflowsNeighboursSynchronously()
{
    QFile file(QStringLiteral(LATTE_SOURCE_DIR "/containment/package/contents/ui/applet/AppletItem.qml"));
    QVERIFY(file.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(file.readAll());
    const int fillStart = source.indexOf(QStringLiteral("    property bool isRequestingFill:"));
    const int fillEnd = source.indexOf(QStringLiteral("    property int maxAutoFillLength:"), fillStart);
    const int dynamicPropertyStart = source.indexOf(QStringLiteral("    function readDynamicProperty(object, propertyName)"));
    const int dynamicPropertyEnd = source.indexOf(QStringLiteral("    // Plasma 6 passes"), dynamicPropertyStart);
    const int layoutStart = source.indexOf(QStringLiteral("    property bool globalMenuLayoutUpdating:"));
    const int layoutEnd = source.indexOf(QStringLiteral("    //when the applet moves"), layoutStart);
    QVERIFY(fillStart >= 0 && fillEnd > fillStart);
    QVERIFY(dynamicPropertyStart >= 0 && dynamicPropertyEnd > dynamicPropertyStart);
    QVERIFY(layoutStart >= 0 && layoutEnd > layoutStart);
    const QByteArray harness = QByteArrayLiteral(R"(
import QtQuick
import QtQuick.Layouts
Grid {
    id: container
    columns: 99999
    rows: 99999
    flow: root.isHorizontal ? Grid.LeftToRight : Grid.TopToBottom
    property QtObject root: QtObject {
        property bool isHorizontal: true
        property bool isVertical: !isHorizontal
        property bool isModernDockStyle: false
    }
    property alias menu: menuItem
    property alias neighbour: nextItem
    Item {
        id: menuItem
        width: 80
        height: 40
        property QtObject root: container.root
        property bool isGlobalMenuApplet: true
        property bool indexerIsSupported: false
        property bool isHidden: false
        property Item applet: Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
)") + source.mid(fillStart, fillEnd - fillStart).toUtf8() +
                               source.mid(dynamicPropertyStart, dynamicPropertyEnd - dynamicPropertyStart).toUtf8() +
                               source.mid(layoutStart, layoutEnd - layoutStart).toUtf8() + QByteArrayLiteral("\n}\nItem { id: nextItem; width: 48; height: 48 }\n}");
    QQmlEngine engine;
    auto object = createQmlObject(engine, harness, QUrl(QStringLiteral("qrc:/global-menu-reflow.qml")));
    QVERIFY(object);
    QObject *menu = object->property("menu").value<QObject *>();
    QObject *neighbour = object->property("neighbour").value<QObject *>();
    QVERIFY(menu && neighbour);
    QVERIFY(QMetaObject::invokeMethod(object.get(), "forceLayout"));
    QCOMPARE(menu->property("isRequestingFill").toBool(), false);
    QVERIFY(menu->setProperty("width", 500.0));
    QCOMPARE(neighbour->property("x").toReal(), 500.0);
    QVERIFY(menu->setProperty("width", 0.0));
    QCOMPARE(neighbour->property("x").toReal(), 0.0);
    QVERIFY(menu->setProperty("width", 150.0));
    QCOMPARE(neighbour->property("x").toReal(), 150.0);
    QObject *root = object->property("root").value<QObject *>();
    QVERIFY(root);
    QVERIFY(root->setProperty("isHorizontal", false));
    QVERIFY(menu->setProperty("height", 300.0));
    QCOMPARE(neighbour->property("y").toReal(), 300.0);
    QVERIFY(menu->setProperty("height", 0.0));
    QCOMPARE(neighbour->property("y").toReal(), 0.0);
}

void
QmlSmokeTest::compactAppletPopupSizingLoadsFromSource()
{
    QTemporaryDir importRoot;
    QQmlEngine engine;
    addFakePlasmaCoreComponents(engine, importRoot);
    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(LATTE_COMPACT_APPLET_QML)));
    std::unique_ptr<QObject> object(component.create());
    if (!object) {
        qWarning() << component.errors();
    }

    QVERIFY(object);

    std::unique_ptr<QObject> fullRepresentation = createQmlObject(
            engine,
            R"(
import QtQuick 2.15
import QtQuick.Layouts 1.15

Item {
    implicitWidth: 1
    implicitHeight: 1
    Layout.minimumWidth: 12
    Layout.minimumHeight: 14
    Layout.preferredWidth: 0
    Layout.preferredHeight: 0
    Layout.maximumWidth: 400
    Layout.maximumHeight: 300
}
)",
            QUrl(QStringLiteral("qrc:/compact-full-representation.qml")));
    QVERIFY(fullRepresentation);
    QVERIFY(object->setProperty("fullRepresentation", QVariant::fromValue(fullRepresentation.get())));

    std::unique_ptr<QObject> appletItem = createQmlObject(
            engine,
            R"(
import QtQuick 2.15

Item {
    property string pluginName: ""
}
)",
            QUrl(QStringLiteral("qrc:/compact-applet-item.qml")));
    QVERIFY(appletItem);
    QVERIFY(object->setProperty("appletItem", QVariant::fromValue(appletItem.get())));

    QVariant volumeMinimumWidth;
    QVariant volumeMinimumHeight;
    appletItem->setProperty("pluginName", QStringLiteral("org.kde.plasma.volume"));
    QVERIFY(QMetaObject::invokeMethod(object.get(), "popupMinimumWidth", Q_RETURN_ARG(QVariant, volumeMinimumWidth)));
    QVERIFY(QMetaObject::invokeMethod(object.get(), "popupMinimumHeight", Q_RETURN_ARG(QVariant, volumeMinimumHeight)));
    QVERIFY(volumeMinimumWidth.toReal() > 12);
    QVERIFY(volumeMinimumHeight.toReal() > 14);

    QVariant preferredWidth;
    QVariant preferredHeight;
    QVERIFY(QMetaObject::invokeMethod(object.get(), "popupPreferredWidth", Q_RETURN_ARG(QVariant, preferredWidth)));
    QVERIFY(QMetaObject::invokeMethod(object.get(), "popupPreferredHeight", Q_RETURN_ARG(QVariant, preferredHeight)));
    QCOMPARE(preferredWidth.toReal(), volumeMinimumWidth.toReal());
    QCOMPARE(preferredHeight.toReal(), volumeMinimumHeight.toReal());

    appletItem->setProperty("pluginName", QStringLiteral("org.kde.plasma.kicker"));
    QVariant menuMaximumWidth;
    QVariant menuMaximumHeight;
    QVERIFY(QMetaObject::invokeMethod(object.get(), "popupMaximumWidth", Q_RETURN_ARG(QVariant, menuMaximumWidth)));
    QVERIFY(QMetaObject::invokeMethod(object.get(), "popupMaximumHeight", Q_RETURN_ARG(QVariant, menuMaximumHeight)));
    QVERIFY(std::isinf(menuMaximumWidth.toDouble()));
    QVERIFY(std::isinf(menuMaximumHeight.toDouble()));
}

void QmlSmokeTest::launchersGeometryRestoreSchedulingLoadsFromSource()
{
    QTemporaryDir importRoot;
    QQmlEngine engine;
    addLatteCoreImport(engine, importRoot);

    QQmlContext context(engine.rootContext());
    TasksModelForLaunchersStub tasksModel;
    const QStringList storedLaunchers{
        QStringLiteral("preferred://filemanager"),
        QStringLiteral("applications:test-file-manager.desktop"),
        QStringLiteral("preferred://browser"),
        QStringLiteral("applications:test-browser.desktop"),
        QStringLiteral("applications:org.kde.konsole.desktop"),
    };
    const QStringList normalizedLaunchers{
        QStringLiteral("preferred://filemanager"),
        QStringLiteral("preferred://browser"),
        QStringLiteral("applications:org.kde.konsole.desktop"),
    };
    LaunchersPlasmoidStub plasmoid(storedLaunchers);
    AppletAbilitiesStub appletAbilities;
    QVariantMap activityInfo{{QStringLiteral("currentActivity"), QStringLiteral("current")}};

    context.setContextProperty(QStringLiteral("plasmoid"), &plasmoid);
    context.setContextProperty(QStringLiteral("appletAbilities"), &appletAbilities);
    context.setContextProperty(QStringLiteral("activityInfo"), activityInfo);
    context.setContextProperty(QStringLiteral("inDraggingPhase"), false);

    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(LATTE_LAUNCHERS_QML)));
    std::unique_ptr<QObject> object(component.create(&context));
    if (!object) {
        qWarning() << component.errors();
    }

    QVERIFY(object);
    context.setContextProperty(QStringLiteral("launchers"), object.get());
    QVERIFY(object->setProperty("tasksModel", QVariant::fromValue(static_cast<QObject *>(&tasksModel))));

    const QJSValue identityFactory = engine.evaluate(QStringLiteral("(backend) => (url) => backend.launcherIdentity(url)"));
    const QJSValue resolver = identityFactory.call({engine.newQObject(&tasksModel)});
    QVERIFY(resolver.isCallable());
    QVERIFY(object->setProperty("resolveLauncherIdentity", QVariant::fromValue(resolver)));

    QVariant ignoredReturn;
    QVERIFY(QMetaObject::invokeMethod(object.get(), "scheduleLaunchersRestore",
                                      Q_RETURN_ARG(QVariant, ignoredReturn),
                                      Q_ARG(QVariant, QStringLiteral("formFactorChanged"))));
    QCOMPARE(object->property("_pendingRestoreReason").toString(), QStringLiteral("formFactorChanged"));
    QCOMPARE(object->property("_geometryTransitionInProgress").toBool(), true);

    const auto children = object->findChildren<QObject *>();
    int runningRestoreTimers = 0;
    bool launchersRestoreFinalTimerWasStarted = false;
    for (QObject *child : children) {
        if (child->property("running").toBool()) {
            const int interval = child->property("interval").toInt();
            if (interval == 450 || interval == 1200 || interval == 2200) {
                ++runningRestoreTimers;
            }
            if (interval == 2200) {
                launchersRestoreFinalTimerWasStarted = true;
            }
        }
    }

    QCOMPARE(runningRestoreTimers, 3);
    QVERIFY(launchersRestoreFinalTimerWasStarted);

    QVERIFY(QMetaObject::invokeMethod(object.get(), "restoreLaunchersFromConfig",
                                      Q_RETURN_ARG(QVariant, ignoredReturn),
                                      Q_ARG(QVariant, QStringLiteral("manual"))));
    QCOMPARE(tasksModel.launcherList(), normalizedLaunchers);
    QCOMPARE(tasksModel.syncCount(), 1);

    QVariant added;
    QVERIFY(QMetaObject::invokeMethod(object.get(), "addLauncherToModel",
                                      Q_RETURN_ARG(QVariant, added),
                                      Q_ARG(QVariant, QStringLiteral("applications:test-file-manager.desktop")),
                                      Q_ARG(QVariant, false)));
    QCOMPARE(added.toBool(), false);
    QCOMPARE(tasksModel.launcherList(), normalizedLaunchers);

    QVERIFY(QMetaObject::invokeMethod(object.get(), "addDroppedLauncher",
                                      Q_RETURN_ARG(QVariant, ignoredReturn),
                                      Q_ARG(QVariant, QStringLiteral("applications:test-browser.desktop?iconData=test"))));
    QCOMPARE(tasksModel.launcherList(), normalizedLaunchers);
    QCOMPARE(tasksModel.syncCount(), 1);

    QVERIFY(QMetaObject::invokeMethod(object.get(), "addLauncherToModel",
                                      Q_RETURN_ARG(QVariant, added),
                                      Q_ARG(QVariant, QStringLiteral("applications:org.kde.kate.desktop")),
                                      Q_ARG(QVariant, false)));
    QCOMPARE(added.toBool(), true);
    QCOMPARE(tasksModel.launcherList().constLast(), QStringLiteral("applications:org.kde.kate.desktop"));

    const QStringList regularLaunchers = tasksModel.launcherList();
    const QString interiorSeparator = QStringLiteral("file:///latte-separator1.desktop");
    const QString trailingSeparator = QStringLiteral("file:///latte-separator2.desktop");
    const QString anotherSeparator = QStringLiteral("file:///latte-separator3.desktop");
    const QString newLauncher = QStringLiteral("applications:test-menu-launcher.desktop");
    tasksModel.setLauncherList({regularLaunchers.first(), interiorSeparator,
                                regularLaunchers.last(), trailingSeparator, anotherSeparator});

    // The application-menu action enters through addLauncher. The launcher
    // model is authoritative even when there are no task delegates.
    QVERIFY(QMetaObject::invokeMethod(object.get(), "addLauncher", Q_ARG(QVariant, newLauncher)));
    QCOMPARE(tasksModel.launcherList(),
             (QStringList{regularLaunchers.first(), interiorSeparator, regularLaunchers.last(),
                          newLauncher, trailingSeparator, anotherSeparator}));

    auto taskRows = createQmlObject(engine, QByteArrayLiteral(R"(
import QtQuick
Item {
    Item { property string launcherUrl: "file:///latte-separator2.desktop"; property int itemIndex: 6 }
    Item { property string launcherUrl: "file:///latte-separator3.desktop"; property int itemIndex: 7 }
}
)"), QUrl(QStringLiteral("qrc:/trailing-separator-task-rows.qml")));
    QVERIFY(taskRows);
    QVERIFY(object->setProperty("layout", QVariant::fromValue(static_cast<QObject *>(taskRows.get()))));
    const QJSValue launcherObject = engine.newQObject(object.get());
    const QJSValue isTrailingSeparator = launcherObject.property(QStringLiteral("isTrailingSeparator"));
    QVERIFY(isTrailingSeparator.isCallable());
    QVERIFY(isTrailingSeparator.callWithInstance(launcherObject, {trailingSeparator}).toBool());
    QVERIFY(!isTrailingSeparator.callWithInstance(launcherObject, {interiorSeparator}).toBool());
    const QJSValue taskMoveIndex = launcherObject.property(QStringLiteral("taskMoveIndex"));
    QVERIFY(taskMoveIndex.isCallable());
    QCOMPARE(taskMoveIndex.callWithInstance(launcherObject, {1, 9, newLauncher, true}).toInt(), 5);
    QCOMPARE(taskMoveIndex.callWithInstance(launcherObject, {9, 3, QStringLiteral("applications:running.desktop"), false}).toInt(), 8);
    QCOMPARE(taskMoveIndex.callWithInstance(launcherObject, {6, 3, trailingSeparator, true}).toInt(), 6);
    QCOMPARE(taskMoveIndex.callWithInstance(launcherObject, {1, 3, newLauncher, true}).toInt(), 3);
    QVERIFY(object->setProperty("layout", QVariant::fromValue(static_cast<QQuickItem *>(nullptr))));
    tasksModel.setLauncherList(regularLaunchers);

    const QMetaMethod orderCommit = object->metaObject()->method(object->metaObject()->indexOfSignal("launcherOrderCommitRequested()"));
    const QMetaMethod syncOrder = tasksModel.metaObject()->method(tasksModel.metaObject()->indexOfMethod("syncLaunchers()"));
    QVERIFY(QObject::connect(object.get(), orderCommit, &tasksModel, syncOrder));
    bool syncingDuringModelCall = false;
    connect(&tasksModel, &TasksModelForLaunchersStub::launcherOrderSyncInvoked, object.get(), [&]() {
        syncingDuringModelCall = object->property("syncingLauncherOrder").toBool();
    });
    QVERIFY(QMetaObject::invokeMethod(object.get(), "syncLauncherOrder"));
    QCOMPARE(tasksModel.syncCount(), 2);
    QVERIFY(syncingDuringModelCall);
    QVERIFY(!object->property("syncingLauncherOrder").toBool());

    auto bridge = createQmlObject(engine, QByteArrayLiteral(R"(
import QtQuick
Item {
    id: bridge
    property var indexer: null
    property var receivedOrder: []
    property int receivedPosition: -1
    property string receivedSeparator: ""
    property QtObject launchers: QtObject {
        property Item client
        property QtObject host: QtObject {
            property bool isReady: true
            function addAbilityClient(client) {}
            function removeAbilityClient(client) {}
            function validateSyncedLaunchersOrder(sender, group, groupId, order) {
                bridge.receivedOrder = order
            }
            function addSyncedLauncher(sender, group, groupId, launcher) {
                bridge.receivedSeparator = launcher
            }
            function addSyncedLauncherAtPosition(sender, group, groupId, launcher, position) {
                bridge.receivedSeparator = launcher
                bridge.receivedPosition = position
            }
        }
    }
}
)"), QUrl(QStringLiteral("qrc:/launcher-order-bridge.qml")));
    QVERIFY(bridge);
    QVERIFY(object->setProperty("bridge", QVariant::fromValue(bridge.get())));

    // A launcher-list notification can arrive before the delegate layout is
    // available. The broadcast must still reflect the committed model order.
    const QStringList modelOrder{
        QStringLiteral("applications:second.desktop"),
        QStringLiteral("[other]\napplications:hidden.desktop"),
        QStringLiteral("[current]\napplications:first.desktop"),
    };
    QTRY_VERIFY(!object->property("_suppressLauncherListWriteBack").toBool());
    context.setContextProperty(QStringLiteral("inDraggingPhase"), true);
    tasksModel.setLauncherList(modelOrder);
    const QStringList expectedOrder{
        QStringLiteral("applications:second.desktop"),
        QStringLiteral("applications:first.desktop"),
    };
    QCOMPARE(bridge->property("receivedOrder").value<QJSValue>().toVariant().toStringList(), expectedOrder);
    QVariant separatorAdded;
    QVERIFY(QMetaObject::invokeMethod(object.get(), "addInternalSeparatorAtPos",
                                      Q_RETURN_ARG(QVariant, separatorAdded), Q_ARG(QVariant, 3)));
    QVERIFY(separatorAdded.toBool());
    QCOMPARE(bridge->property("receivedSeparator").toString(), QStringLiteral("file:///latte-separator1.desktop"));
    QCOMPARE(bridge->property("receivedPosition").toInt(), 3);
    object.reset();
}

void QmlSmokeTest::peerLauncherOrderStartsValidationWithoutRetryDelay()
{
    QQmlEngine engine;
    auto launchers = createQmlObject(engine,
                                     QByteArrayLiteral(R"(
import QtQuick
QtObject {
    id: launchers
    property var order: ["first", "second"]
    property int moves: 0
    property int commits: 0
    readonly property QtObject tasksModel: QtObject {
        function move(from, to) {
            const updated = launchers.order.slice();
            const moved = updated.splice(from, 1)[0];
            updated.splice(to, 0, moved);
            launchers.order = updated;
            ++launchers.moves;
        }
    }
    function currentShownLauncherList() { return order; }
    function launcherIdentity(url) { return url === "preferred://browser" ? "second" : url; }
    function indexOfLayoutLauncher(url) { return order.indexOf(url); }
    function syncLauncherOrder() { ++commits; }
}
)"), QUrl(QStringLiteral("qrc:/peer-launcher-order.qml")));
    QVERIFY(launchers);
    QQmlContext context(engine.rootContext());
    context.setContextProperty(QStringLiteral("_launchers"), launchers.get());

    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(
        LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/abilities/launchers/Validator.qml")));
    std::unique_ptr<QObject> validator(component.create(&context));
    QVERIFY(validator);

    const QVariant targetOrder = QStringList{QStringLiteral("preferred://browser"), QStringLiteral("first")};
    QVERIFY(QMetaObject::invokeMethod(validator.get(), "validateOrder", Q_ARG(QVariant, targetOrder)));
    QCOMPARE(validator->property("interval").toInt(), 0);
    QCOMPARE(launchers->property("moves").toInt(), 0);

    QTRY_COMPARE(launchers->property("moves").toInt(), 1);
    QCOMPARE(validator->property("interval").toInt(), 400);
    QTRY_COMPARE(launchers->property("commits").toInt(), 1);
    QVERIFY(!validator->property("running").toBool());

    QVERIFY(launchers->setProperty("order", QStringList{}));
    QVERIFY(QMetaObject::invokeMethod(validator.get(), "validateOrder", Q_ARG(QVariant, targetOrder)));
    QTRY_COMPARE(validator->property("interval").toInt(), 400);
    QVERIFY(validator->property("running").toBool());
    QCOMPARE(launchers->property("moves").toInt(), 1);

    QVERIFY(launchers->setProperty("order", QStringList{QStringLiteral("first"), QStringLiteral("second")}));
    QTRY_COMPARE(launchers->property("moves").toInt(), 2);
    QTRY_COMPARE(launchers->property("commits").toInt(), 2);
    QVERIFY(!validator->property("running").toBool());
}

void QmlSmokeTest::plasmaVolumeBootstrapLoadsFromSource()
{
    QTemporaryDir importRoot;
    QQmlEngine engine;
    addFakePlasmaVolumeImport(engine, importRoot);

    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(LATTE_PULSEAUDIO_QML)));
    std::unique_ptr<QObject> object(component.create());
    if (!object) {
        qWarning() << component.errors();
    }

    QVERIFY(object);
    QCOMPARE(object->property("bootstrapAttempts").toInt(), 0);
    QCOMPARE(object->property("bootstrapMaxAttempts").toInt(), 5);

    QObject *paFixTimer = object->property("paFixTimer").value<QObject *>();
    QVERIFY(paFixTimer);
    QCOMPARE(paFixTimer->property("interval").toInt(), 1000);
    QCOMPARE(paFixTimer->property("repeat").toBool(), true);
    QCOMPARE(paFixTimer->property("running").toBool(), true);
}

void QmlSmokeTest::audioBadgesTrackPauseRatherThanConnectionLifetime()
{
    QQmlEngine engine;
    auto stream = createQmlObject(engine, R"(
import QtQml
QtObject {
    property bool corked: false
    property bool muted: false
    property int volume: 100
    property bool activityAvailable: true
    property real peak: 0.2
    property bool recentlyAudible: true
}
)", QUrl(QStringLiteral("playback-stream.qml")));
    QVERIFY(stream);
    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/task/AudioPlaybackState.qml")));
    std::unique_ptr<QObject> state(component.createWithInitialProperties({
        {QStringLiteral("streams"), QVariantList{QVariant::fromValue(stream.get())}}
    }));
    QVERIFY2(state, qPrintable(component.errorString()));
    QCOMPARE(state->property("playing").toBool(), true);
    state->setProperty("playerPaused", true);
    QCOMPARE(state->property("playing").toBool(), true); // Other actual audio can continue in a browser.
    stream->setProperty("peak", 0.0);
    QCOMPARE(state->property("playing").toBool(), false); // Pause bypasses the silence grace.
    state->setProperty("playerPaused", false);
    QCOMPARE(state->property("playing").toBool(), true); // Unknown player gets a short silence grace.
    stream->setProperty("recentlyAudible", false);
    QCOMPARE(state->property("playing").toBool(), false);
    state->setProperty("playerPlaying", true);
    QCOMPARE(state->property("playing").toBool(), true); // A playing session can contain quiet passages.
    stream->setProperty("corked", true);
    QCOMPARE(state->property("playing").toBool(), false);
    stream->setProperty("corked", false);
    stream->setProperty("muted", true);
    QCOMPARE(state->property("playing").toBool(), true);
    state->setProperty("playerPlaying", false);
    state->setProperty("playerPaused", true);
    QCOMPARE(state->property("playing").toBool(), false); // A retained muted stream is not playback.
    stream->setProperty("muted", false);
    stream->setProperty("activityAvailable", false);
    QCOMPARE(state->property("playing").toBool(), false);
    state->setProperty("playerPaused", false);
    QCOMPARE(state->property("playing").toBool(), true); // Corked fallback if monitoring is unavailable.
    state->setProperty("enabled", false);
    QCOMPARE(state->property("playing").toBool(), false);
    state->setProperty("enabled", true);
    state->setProperty("streams", QVariantList{});
    QCOMPARE(state->property("playing").toBool(), false);
}

void
QmlSmokeTest::audioStreamsRequireUnambiguousOwnership()
{
    QFile policy(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/code/AudioStreamMatch.js"));
    QVERIFY(policy.open(QFile::ReadOnly));
    QString source = QString::fromUtf8(policy.readAll());
    source.remove(QStringLiteral(".pragma library"));
    QQmlEngine engine;
    auto result = engine.evaluate(source);
    QVERIFY2(!result.isError(), qPrintable(result.toString()));
    result = engine.evaluate(QStringLiteral(R"(
(function() {
    var chrome = {pid: 100, appId: "google-chrome", launcherName: "applications:google-chrome.desktop", appName: "Google Chrome"};
    var music = {pid: 100, appId: "google-chrome", launcherName: "chrome-music123-Default.desktop", appName: "Google Chrome"};
    var mail = {pid: 100, appId: "google-chrome", launcherName: "chrome-mail456-Default.desktop", appName: "Google Chrome"};
    var peers = [chrome, music, mail];
    var stream = {pid: 201, appId: "", appName: "Google Chrome", binaryName: "chrome"};
    var calls = 0;
    function parent(pid) { ++calls; return pid === 201 ? 200 : pid === 200 ? 100 : 0; }
    function check(condition, message) { if (!condition) { throw new Error(message); } }
    function count(map, task) { return (map[token(task)] || []).length; }
    var map = assign(peers, [stream], parent);
    check(count(map, chrome) === 1 && count(map, music) === 0 && count(map, mail) === 0, "generic browser stream leaked to a PWA");
    check(count(assign([mail, music, chrome], [stream], parent), chrome) === 1, "task order affected ownership");
    stream.appId = "com.google.Chrome";
    check(count(assign(peers, [stream], parent), chrome) === 1, "reverse-DNS browser alias lost its badge");
    stream.desktopId = music.launcherName;
    map = assign(peers, [stream], parent);
    check(count(map, chrome) === 0 && count(map, music) === 1 && count(map, mail) === 0, "explicit PWA desktop ID did not win");
    stream.desktopId = "";
    stream.portalAppId = mail.launcherName;
    map = assign(peers, [stream], parent);
    check(count(map, mail) === 1 && count(map, chrome) === 0, "portal ID did not win");
    stream.portalAppId = "unrelated.desktop";
    check(Object.keys(assign(peers, [stream], parent)).length === 0, "conflicting explicit identity fell back to PID");
    stream = {pid: 201, appName: "Google Chrome", binaryName: "chrome"};
    check(Object.keys(assign([music, mail], [stream], parent)).length === 0, "shared browser PID selected an arbitrary PWA");
    check(Object.keys(assign([music], [stream], parent)).length === 0, "filtered-out browser left its stream on a lone PWA");
    var pwaWithOwnAppId = {pid:100, appId:"chrome-owned-Default", launcherName:"chrome-owned-Default.desktop", appName:"Music"};
    check(Object.keys(assign([pwaWithOwnAppId], [stream], parent)).length === 0, "PWA-specific AppId still inherited the shared browser process");
    var standalone = {pid: 300, appId: "yesplaymusic", launcherName: "yesplaymusic.desktop", appName: "YesPlayMusic"};
    map = assign([chrome, standalone], [stream], parent);
    check(count(map, chrome) === 1 && count(map, standalone) === 0, "idle standalone music app got a browser badge");
    var nativeStream = {pid: 301, binaryName: "electron", appName: "YesPlayMusic"};
    function nativeParent(pid) { return pid === 301 ? 300 : 0; }
    map = assign([chrome, standalone], [nativeStream], nativeParent);
    check(count(map, standalone) === 1 && count(map, chrome) === 0, "native child process failed to match");
    map = assign([chrome, standalone], [stream, nativeStream], function(pid) { return pid === 201 ? 100 : pid === 301 ? 300 : 0; });
    check(count(map, chrome) === 1 && map[token(chrome)][0] === stream, "browser controls target the wrong stream");
    check(count(map, standalone) === 1 && map[token(standalone)][0] === nativeStream, "music controls target the wrong stream");
    var genericElectron = {pid: 300, appId: "google-chrome", launcherName: "yesplaymusic.desktop", appName: "Google Chrome"};
    var genericElectronStream = {pid: 301, appName: "Google Chrome", binaryName: "chrome"};
    map = assign([chrome, genericElectron], [stream, genericElectronStream], function(pid) { return pid === 201 ? 100 : pid === 301 ? 300 : 0; });
    check(count(map, genericElectron) === 1 && map[token(genericElectron)][0] === genericElectronStream, "Electron's generic Chromium identity lost its own PID match");
    check(count(map, chrome) === 1 && map[token(chrome)][0] === stream, "Electron audio leaked back to Chrome");
    genericElectronStream.appId = "com.google.Chrome";
    map = assign([chrome, genericElectron], [genericElectronStream], nativeParent);
    check(count(map, genericElectron) === 1 && count(map, chrome) === 0, "generic Chromium stream appId overrode Electron's independent process");
    check(count(assign([genericElectron], [stream], parent), genericElectron) === 0, "filtered-out Chrome leaked through Electron's generic name");
    var other = {pid: 400, appId: "yesplaymusic", launcherName: "yesplaymusic.desktop", appName: "YesPlayMusic"};
    nativeStream.appId = "yesplaymusic";
    map = assign([standalone, other], [nativeStream], nativeParent);
    check(count(map, standalone) === 1 && count(map, other) === 0, "another process of the same application got the stream");
    check(!sameId("chrome-music123-Default", "chrome"), "PWA treated as a browser suffix variant");
    check(sameId("org.mozilla.firefox", "firefox-esr"), "distribution suffix compatibility regressed");
    check(!sameId("org.first.Player", "org.second.Player"), "distinct reverse-DNS IDs collapsed");
    var a = {pid: 0, launcherName: "one.desktop", appName: "Shared"};
    var b = {pid: 0, launcherName: "two.desktop", appName: "Shared"};
    check(Object.keys(assign([a, b], [{appName: "Shared"}], parent)).length === 0, "ambiguous name fallback leaked");
    check(count(assign([a], [{appName: "Shared"}], parent), a) === 1, "unique legacy name fallback regressed");
    check(count(assign([a], [], parent), a) === 0, "removed stream remained assigned");
    calls = 0;
    assign([{pid: 999, launcherName: "app"}], [{pid: 800}], function(pid) { ++calls; return pid + 1; });
    check(calls <= 16, "ancestor traversal was not bounded");
    check(ancestry(800, function(pid) { return pid; }, Object.create(null)).length === 1, "cyclic parent chain was not bounded");
    return true;
})()
)"));
    QVERIFY2(!result.isError(), qPrintable(result.toString()));
    QVERIFY(result.toBool());
}

void
QmlSmokeTest::audioOwnershipTracksModelAndMetadataChanges()
{
    QTest::failOnWarning(QRegularExpression(QStringLiteral(".*(TypeError|ReferenceError|Cannot read property).*")));
    QTemporaryDir imports;
    QQmlEngine engine;
    addFakePlasmaVolumeImport(engine, imports);
    auto backend = createQmlObject(engine,
                                   R"(
import QtQml
QtObject { function parentPid(pid) { return pid === 201 ? 100 : 0; } }
)",
                                   QUrl(QStringLiteral("audio-backend.qml")));
    QVERIFY(backend);
    engine.rootContext()->setContextProperty(QStringLiteral("backend"), backend.get());
    auto tasks = createQmlObject(engine,
                                 R"(
import QtQml.Models
ListModel { dynamicRoles: true }
)",
                                 QUrl(QStringLiteral("audio-tasks.qml")));
    QVERIFY(tasks);
    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(LATTE_PULSEAUDIO_QML)));
    std::unique_ptr<QObject> audio(component.create());
    QVERIFY2(audio, qPrintable(component.errorString()));
    QVERIFY(audio->setProperty("taskModel", QVariant::fromValue(tasks.get())));
    auto *streams = audio->property("instantiator").value<QObject *>();
    QVERIFY(streams);
    auto *model = streams->property("model").value<QObject *>();
    QVERIFY(model);
    audio->property("paFixTimer").value<QObject *>()->setProperty("running", false);
    engine.globalObject().setProperty(QStringLiteral("audio"), engine.newQObject(audio.get()));
    engine.globalObject().setProperty(QStringLiteral("tasks"), engine.newQObject(tasks.get()));
    engine.globalObject().setProperty(QStringLiteral("streams"), engine.newQObject(model));
    const auto setup = engine.evaluate(QStringLiteral(R"(
        tasks.append({AppPid:100, AppId:"google-chrome", AppName:"Google Chrome", LauncherUrlWithoutIcon:"google-chrome.desktop", IsWindow:true, IsGroupParent:false});
        tasks.append({AppPid:100, AppId:"google-chrome", AppName:"Google Chrome", LauncherUrlWithoutIcon:"chrome-music-Default.desktop", IsWindow:true, IsGroupParent:false});
        streams.append({Client:{properties:{"application.process.id":"201", "application.name":"Google Chrome", "application.process.binary":"chrome"}},
            Properties:{}, PulseObject:{index:1}, Muted:false, Corked:false, Volume:65536});
    )"));
    QVERIFY2(!setup.isError(), qPrintable(setup.toString()));
    QCoreApplication::processEvents();
    auto count = [&engine](const QString &launcher) { return engine.evaluate(QStringLiteral("audio.streamsForTask('google-chrome', '%1', 'Google Chrome', 100).length").arg(launcher)).toInt(); };
    QTRY_COMPARE(count(QStringLiteral("google-chrome")), 1);
    QCOMPARE(count(QStringLiteral("chrome-music-Default")), 0);
    auto activity = engine.evaluate(QStringLiteral("var info = audio.instantiator.objectAt(0); info.activityMonitor.volume = 0.2; info.recentlyAudible"));
    QVERIFY2(!activity.isError(), qPrintable(activity.toString()));
    QVERIFY(activity.toBool());
    activity = engine.evaluate(QStringLiteral("info.activityMonitor.volume = 0; info.silenceTimer.running"));
    QVERIFY(activity.toBool());
    QTRY_VERIFY(!engine.evaluate(QStringLiteral("info.recentlyAudible")).toBool());
    activity = engine.evaluate(QStringLiteral("info.activityMonitor.volume = 0.3; streams.setProperty(0, 'Corked', true)"));
    QVERIFY2(!activity.isError(), qPrintable(activity.toString()));
    QCoreApplication::processEvents();
    QVERIFY(engine.evaluate(QStringLiteral("info.activityMonitor.target === null && !info.recentlyAudible && !info.silenceTimer.running")).toBool());
    activity = engine.evaluate(QStringLiteral("streams.setProperty(0, 'Corked', false)"));
    QVERIFY2(!activity.isError(), qPrintable(activity.toString()));
    QCoreApplication::processEvents();
    QVERIFY(engine.evaluate(QStringLiteral("!info.recentlyAudible && info.activityMonitor.volume === 0")).toBool());
    auto update = engine.evaluate(QStringLiteral("streams.setProperty(0, 'Properties', {'application.desktop':'chrome-music-Default.desktop'})"));
    QVERIFY2(!update.isError(), qPrintable(update.toString()));
    QCoreApplication::processEvents();
    QTRY_COMPARE(count(QStringLiteral("chrome-music-Default")), 1);
    QCOMPARE(count(QStringLiteral("google-chrome")), 0);
    update = engine.evaluate(QStringLiteral(R"(
        streams.append({Client:{properties:{"application.process.id":"201", "application.name":"Google Chrome", "application.process.binary":"chrome"}},
            Properties:{}, Muted:false, Corked:false, Volume:65536});
    )"));
    QVERIFY2(!update.isError(), qPrintable(update.toString()));
    QCoreApplication::processEvents();
    QCOMPARE(count(QStringLiteral("google-chrome")), 1);
    QCOMPARE(count(QStringLiteral("chrome-music-Default")), 1);
    update = engine.evaluate(QStringLiteral("audio.streamsForTask('google-chrome', 'chrome-music-Default', 'Google Chrome', 100)[0].mute(); streams.get(0).Muted && !streams.get(1).Muted"));
    QVERIFY2(!update.isError(), qPrintable(update.toString()));
    QVERIFY(update.toBool());
    update = engine.evaluate(
      QStringLiteral("audio.streamsForTask('google-chrome', 'google-chrome', 'Google Chrome', 100)[0].increaseVolume(); streams.get(1).Volume > 65536 && streams.get(0).Volume === 65536"));
    QVERIFY2(!update.isError(), qPrintable(update.toString()));
    QVERIFY(update.toBool());
    update = engine.evaluate(QStringLiteral("streams.setProperty(0, 'Properties', {})"));
    QVERIFY2(!update.isError(), qPrintable(update.toString()));
    QCoreApplication::processEvents();
    QCOMPARE(count(QStringLiteral("chrome-music-Default")), 0);
    QCOMPARE(count(QStringLiteral("google-chrome")), 2);
    update = engine.evaluate(QStringLiteral("streams.clear()"));
    QVERIFY2(!update.isError(), qPrintable(update.toString()));
    QCoreApplication::processEvents();
    QCOMPARE(count(QStringLiteral("chrome-music-Default")), 0);
    update = engine.evaluate(QStringLiteral("tasks.clear()"));
    QVERIFY2(!update.isError(), qPrintable(update.toString()));
    QCoreApplication::processEvents();
    QCOMPARE(count(QStringLiteral("google-chrome")), 0);
    audio.reset();
    QCoreApplication::processEvents();
}

void QmlSmokeTest::taskPresentationFiltersPhantoms()
{
    QFile file(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/task/SubWindows.qml"));
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString source = QString::fromUtf8(file.readAll());
    // Execute the production JavaScript helpers with just the task-model
    // boundary mocked, without constructing the dock's entire scene graph.
    QQmlEngine engine;
    for (const auto &name : {"isActivatableChild", "presentableWindowIds"}) {
        const int start = source.indexOf(QStringLiteral("    function %1(").arg(QLatin1String(name)));
        QVERIFY(start >= 0);
        const int end = source.indexOf(QStringLiteral("\n    }"), start);
        QVERIFY(end > start);
        const auto result = engine.evaluate(source.mid(start, end + 6 - start));
        QVERIFY2(!result.isError(), qPrintable(result.toString()));
    }
    const auto setup = engine.evaluate(QStringLiteral(R"(
        var children = [
            {model: {WinIdList: ["visible"], IsHidden: false}},
            {model: {WinIdList: ["minimized"], IsHidden: true, IsMinimized: true}},
            {model: {WinIdList: ["phantom"], IsHidden: true, IsMinimized: false}},
            {model: {WinIdList: []}},
            {model: {WinIdList: ["visible"]}},
            null
        ];
        var taskItem = {modelIndex: function() { return 42; }};
        var windowsLocalModel = {rootIndex: -1, items: {
            count: children.length, get: function(i) { return children[i]; }
        }};
    )"));
    QVERIFY2(!setup.isError(), qPrintable(setup.toString()));
    const auto ids = engine.evaluate(QStringLiteral("presentableWindowIds()"));
    QVERIFY2(!ids.isError(), qPrintable(ids.toString()));
    QCOMPARE(ids.toVariant().toList(), QVariantList({QStringLiteral("visible"), QStringLiteral("minimized")}));
    QCOMPARE(engine.evaluate(QStringLiteral("windowsLocalModel.rootIndex")).toInt(), 42);

    const auto single = engine.evaluate(QStringLiteral(
        "children = [children[0], children[2]]; windowsLocalModel.items.count = children.length; presentableWindowIds()"));
    QCOMPARE(single.toVariant().toList(), QVariantList({QStringLiteral("visible")}));
}

void QmlSmokeTest::taskMouseAreaAllClickActionsPresent()
{
    // Verify TaskMouseArea.qml handles all 9 TaskAction enum values for
    // left-click, middle-click, and modifier-click (including the new
    // PresentWindows, PreviewWindows, HighlightWindows,
    // PreviewAndHighlightWindows handlers that were added alongside the
    // original Close / NewInstance / ToggleMinimized / CycleThroughTasks /
    // ToggleGrouping / NoneAction).
    QFile file(QStringLiteral(LATTE_TASKMOUSEAREA_QML));
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString source = QString::fromUtf8(file.readAll());

    const QStringList requiredActions{
        QStringLiteral("Close"),
        QStringLiteral("NewInstance"),
        QStringLiteral("ToggleMinimized"),
        QStringLiteral("CycleThroughTasks"),
        QStringLiteral("ToggleGrouping"),
        QStringLiteral("PresentWindows"),
        QStringLiteral("PreviewWindows"),
        QStringLiteral("HighlightWindows"),
        QStringLiteral("PreviewAndHighlightWindows"),
    };

    // Every required action must be referenced for left-click
    for (const auto &action : requiredActions) {
        const QString leftPattern = QStringLiteral("leftClickAction === LatteTasks.types.%1").arg(action);
        QVERIFY2(source.contains(leftPattern),
                 qPrintable(QStringLiteral("TaskMouseArea.qml missing left-click handler for %1").arg(action)));
    }

    // Every required action must be referenced for middle-click
    for (const auto &action : requiredActions) {
        const QString middlePattern = QStringLiteral("middleClickAction == LatteTasks.types.%1").arg(action);
        QVERIFY2(source.contains(middlePattern),
                 qPrintable(QStringLiteral("TaskMouseArea.qml missing middle-click handler for %1").arg(action)));
    }

    // Every required action must be referenced for modifier-click
    for (const auto &action : requiredActions) {
        const QString modPattern = QStringLiteral("modifierClickAction == LatteTasks.types.%1").arg(action);
        QVERIFY2(source.contains(modPattern),
                 qPrintable(QStringLiteral("TaskMouseArea.qml missing modifier-click handler for %1").arg(action)));
    }

    // Mentioning an enum is insufficient: the old Present Windows branches
    // existed but only cycled tasks through a permanently unavailable backend.
    for (const auto &click : {"left", "middle", "modifier"}) {
        const QRegularExpression presentCall(QStringLiteral(
            "%1ClickAction ==+ LatteTasks\\.types\\.PresentWindows\\)\\s*\\{\\s*taskItem\\.presentWindows\\(\\);")
            .arg(QLatin1String(click)));
        QVERIFY2(presentCall.match(source).hasMatch(), click);
    }

    // NoneAction must be acknowledged (comment is fine)
    QVERIFY(source.contains(QStringLiteral("NoneAction")));
}

void QmlSmokeTest::tasksConfigComboModelsComplete()
{
    // Verify the leftClickAction, middleClickAction, and modifierClickAction
    // combo boxes in TasksConfig.qml expose all 10 items (None + 9 actions).
    QFile file(QStringLiteral(LATTE_TASKSCONFIG_QML));
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString source = QString::fromUtf8(file.readAll());

    // leftClickAction combo must list all 10 choices
    const QStringList leftClickItems{
        QStringLiteral("None"),
        QStringLiteral("Close Window or Group"),
        QStringLiteral("New Instance"),
        QStringLiteral("Minimize/Restore"),
        QStringLiteral("Cycle Through Tasks"),
        QStringLiteral("Toggle Task Grouping"),
        QStringLiteral("Present Windows"),
        QStringLiteral("Preview Windows"),
        QStringLiteral("Highlight Windows"),
        QStringLiteral("Preview and Highlight Windows"),
    };
    for (const auto &item : leftClickItems) {
        QVERIFY2(source.contains(item),
                 qPrintable(QStringLiteral("TasksConfig.qml leftClickAction combo missing: %1").arg(item)));
    }

    // middleClickAction combo must list all 10 choices
    // The combo model is shared; verify it includes PresentWindows onwards
    // (items that were added by the fix)
    QVERIFY(source.contains(QStringLiteral("Present Windows")));
    QVERIFY(source.contains(QStringLiteral("Preview and Highlight Windows")));

    // modifierClickAction combo must list all 10 choices
    // Its model is in the same file; verify the extended items are present
    const int presentWindowsCount = source.count(QStringLiteral("Present Windows"));
    QVERIFY2(presentWindowsCount >= 3,
             qPrintable(QStringLiteral("TasksConfig.qml should have Present Windows in all 3 click-action combos, found %1").arg(presentWindowsCount)));
}

void QmlSmokeTest::taskMouseAreaRegressionGuardsPresent()
{
    // Verify that existing behaviour (right-click context menu, wheel
    // handler, highlight cancellation) was not accidentally removed.
    QFile file(QStringLiteral(LATTE_TASKMOUSEAREA_QML));
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString source = QString::fromUtf8(file.readAll());

    // Right-click must still show context menu (not replaced by an action)
    QVERIFY(source.contains(QStringLiteral("RightButton")));
    QVERIFY(source.contains(QStringLiteral("showContextMenu")));

    // Wheel handler must still check taskScrollAction and ScrollToggleMinimized
    QVERIFY(source.contains(QStringLiteral("taskScrollAction")));
    QVERIFY(source.contains(QStringLiteral("ScrollToggleMinimized")));

    // Highlight cancellation must still run after all click handlers
    QVERIFY(source.contains(QStringLiteral("cancelHighlightWindows")));

    // The former capability guard always evaluated false in our compat module.
    // Fallback now follows the asynchronous effect reply in TaskItem instead.
    QVERIFY(!source.contains(QStringLiteral("canPresentWindowsIsSupported")));

    // preventStealing must still be set (Qt6 drag regression guard)
    QVERIFY(source.contains(QStringLiteral("preventStealing: true")));
}

void QmlSmokeTest::tasksConfigCfgPropertyAndNoStaleTasksConfig()
{
    // The cfg property (latteView.extendedInterface.configurationForAppletVisualIndex)
    // must be the config source.  No stale tasks.configuration references
    // should remain.
    QFile file(QStringLiteral(LATTE_TASKSCONFIG_QML));
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString source = QString::fromUtf8(file.readAll());

    // cfg property must exist
    QVERIFY(source.contains(QStringLiteral("configurationForAppletVisualIndex")));

    // No stale tasks.configuration references (all replaced with cfg)
    QVERIFY2(!source.contains(QStringLiteral("tasks.configuration")),
             "TasksConfig.qml still contains stale tasks.configuration references — replace with cfg");
}

void QmlSmokeTest::configInteractionHoverActionNotHardcoded()
{
    // cfg_hoverAction must read from plasmoid.configuration.hoverAction,
    // not be hardcoded to LatteTasks.types.NoneAction.
    QFile file(QStringLiteral(LATTE_CONFIG_INTERACTION_QML));
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString source = QString::fromUtf8(file.readAll());

    // Must read from real config (not hardcoded)
    QVERIFY(source.contains(QStringLiteral("plasmoid.configuration.hoverAction")));

    // The old hardcoded value must be gone
    QVERIFY2(!source.contains(QStringLiteral("cfg_hoverAction: LatteTasks.types.NoneAction")),
             "ConfigInteraction.qml cfg_hoverAction is still hardcoded to NoneAction");
}

void QmlSmokeTest::taskMouseAreaStructuralGuardsPresent()
{
    // Verify structural guards that prevent accidental event handling
    // in edit mode, during drag, or when all-windows functionality is
    // disabled.
    QFile file(QStringLiteral(LATTE_TASKMOUSEAREA_QML));
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString source = QString::fromUtf8(file.readAll());

    // Edit-mode guard must be checked before any click action
    QVERIFY(source.contains(QStringLiteral("isContainmentEditing()")));
    QVERIFY(source.contains(QStringLiteral("!isSeparator")));
    QVERIFY(source.contains(QStringLiteral("!taskItem.isDragged")));

    // modifierAccepted function must exist (used by modifier-click path)
    QVERIFY(source.contains(QStringLiteral("modifierAccepted")));

    // disableAllWindowsFunctionality guard must be present on modifier
    // and middle-click paths
    QVERIFY(source.contains(QStringLiteral("!root.disableAllWindowsFunctionality")));

    // LeftClick must be checked by mouse.button
    QVERIFY(source.contains(QStringLiteral("Qt.LeftButton")));
    QVERIFY(source.contains(QStringLiteral("Qt.MiddleButton")));

    // highlightedWindows signal integration
    QVERIFY(source.contains(QStringLiteral("windowsHovered")));
}

void QmlSmokeTest::taskRightClickReachesDockMenuInEditMode()
{
    // Run the production press handler above a stand-in for the containment
    // context menu layer. In edit mode a right press must reach that layer,
    // which offers the edit-only dock actions; the task menu stays for
    // normal mode and left presses stay with the task for drag-to-reorder.
    QFile file(QStringLiteral(LATTE_TASKMOUSEAREA_QML));
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString source = QString::fromUtf8(file.readAll());
    const int pressedStart = source.indexOf(QStringLiteral("    onPressed: (mouse) => {"));
    const int pressedEnd = source.indexOf(QStringLiteral("    onReleased: (mouse) => {"), pressedStart);
    QVERIFY(pressedStart >= 0 && pressedEnd > pressedStart);
    QQuickView view;
    QQmlComponent component(view.engine());
    const QByteArray harness = QByteArrayLiteral(R"(
import QtQuick
Item {
    id: root
    width: 100; height: 100
    property bool inEditMode: true
    property int menuPresses: 0
    property int taskMenus: 0
    readonly property bool taskPressed: taskMouseArea.pressed
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        onPressed: root.menuPresses++
    }
    MouseArea {
        id: taskMouseArea
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
        property bool pressed: false
        property bool dragReady: false
        property int lastButtonClicked: Qt.NoButton
        property int pressX: -1
        property int pressY: -1
        readonly property bool isSeparator: false
        readonly property var model: ({IsLauncher: false})
        Timer { id: _resistanerTimer; interval: 1000 }
        function slotPublishGeometries() {}
        function modifierAccepted(mouse) { return false; }
        function isContainmentEditing() { return root.inEditMode; }
        function showContextMenu(args) { root.taskMenus++; }
)") + source.mid(pressedStart, pressedEnd - pressedStart).toUtf8() + QByteArrayLiteral("    }\n}");
    const QUrl url(QStringLiteral("qrc:/task-edit-mode-right-press.qml"));
    component.setData(harness, url);
    QObject *object = component.create();
    QVERIFY2(object, qPrintable(component.errorString()));
    view.setContent(url, &component, object);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    const QPoint task(50, 50);

    QTest::mouseClick(&view, Qt::RightButton, Qt::NoModifier, task);
    QCOMPARE(object->property("menuPresses").toInt(), 1);
    QCOMPARE(object->property("taskMenus").toInt(), 0);

    QTest::mouseClick(&view, Qt::LeftButton, Qt::NoModifier, task);
    QVERIFY(object->property("taskPressed").toBool());
    QCOMPARE(object->property("menuPresses").toInt(), 1);

    QVERIFY(object->setProperty("inEditMode", false));
    QTest::mouseClick(&view, Qt::RightButton, Qt::NoModifier, task);
    QCOMPARE(object->property("taskMenus").toInt(), 1);
    QCOMPARE(object->property("menuPresses").toInt(), 1);
}

void QmlSmokeTest::tasksConfigClickActionModelOrderMatchesEnum()
{
    // The combo box models for click actions use direct index-to-enum
    // mapping (currentIndex ↔ plasmoid.configuration value).  Verify
    // the model array order matches the TaskAction enum order defined
    // in types.h:
    //   0=NoneAction  1=Close  2=NewInstance  3=ToggleMinimized
    //   4=CycleThroughTasks  5=ToggleGrouping  6=PresentWindows
    //   7=PreviewWindows  8=HighlightWindows  9=PreviewAndHighlightWindows
    QFile file(QStringLiteral(LATTE_TASKSCONFIG_QML));
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString source = QString::fromUtf8(file.readAll());

    // Find the leftClickAction model block
    // Items must appear in enum order: None(0), Close(1), NewInstance(2),
    // Minimize/Restore(3), CycleThroughTasks(4), ToggleGrouping(5),
    // PresentWindows(6), PreviewWindows(7), HighlightWindows(8),
    // PreviewAndHighlightWindows(9)
    const QStringList orderedActions{
        QStringLiteral("None"),
        QStringLiteral("Close Window or Group"),
        QStringLiteral("New Instance"),
        QStringLiteral("Minimize/Restore Window or Group"),
        QStringLiteral("Cycle Through Tasks"),
        QStringLiteral("Toggle Task Grouping"),
        QStringLiteral("Present Windows"),
        QStringLiteral("Preview Windows"),
        QStringLiteral("Highlight Windows"),
        QStringLiteral("Preview and Highlight Windows"),
    };

    // Extract the leftClickAction model section
    int leftClickStart = source.indexOf(QStringLiteral("id: leftClickAction"));
    QVERIFY(leftClickStart > 0);

    // Find the model array within the leftClickAction block
    int modelStart = source.indexOf(QStringLiteral("model:"), leftClickStart);
    QVERIFY(modelStart > 0);
    int modelBracket = source.indexOf(QStringLiteral("["), modelStart);
    QVERIFY(modelBracket > 0);
    int modelEnd = source.indexOf(QStringLiteral("]"), modelBracket);
    QVERIFY(modelEnd > 0);

    // Verify items appear in the correct order within the model array
    int lastPos = modelBracket;

    for (const auto &action : orderedActions) {
        int pos = source.indexOf(action, lastPos);
        QVERIFY2(pos > 0 && pos < modelEnd,
                 qPrintable(QStringLiteral("leftClickAction model: '%1' missing or out of order").arg(action)));
        lastPos = pos;
    }

    // Verify direct index mapping is used (no switch-case conversion)
    int afterModel = source.indexOf(QStringLiteral("currentIndex:"), modelEnd);
    QVERIFY(afterModel > 0);
    QVERIFY(source.indexOf(QStringLiteral("cfg.leftClickAction"), afterModel) > 0
            || source.indexOf(QStringLiteral("tasks.configuration.leftClickAction"), afterModel) > 0);
}

void QmlSmokeTest::typesTaskActionEnumCompleteness()
{
    // Verify the TaskAction enum in types.h has all 10 values (0-9).
    QFile file(QStringLiteral(LATTE_TYPES_H));
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString source = QString::fromUtf8(file.readAll());

    const QStringList requiredEnumValues{
        QStringLiteral("NoneAction"),
        QStringLiteral("Close"),
        QStringLiteral("NewInstance"),
        QStringLiteral("ToggleMinimized"),
        QStringLiteral("CycleThroughTasks"),
        QStringLiteral("ToggleGrouping"),
        QStringLiteral("PresentWindows"),
        QStringLiteral("PreviewWindows"),
        QStringLiteral("HighlightWindows"),
        QStringLiteral("PreviewAndHighlightWindows"),
    };

    // All enum values must be present in the TaskAction enum block
    for (const auto &value : requiredEnumValues) {
        QVERIFY2(source.contains(value),
                 qPrintable(QStringLiteral("types.h TaskAction enum missing: %1").arg(value)));
    }

    // TaskScrollAction must include ScrollNone, ScrollTasks, ScrollToggleMinimized
    QVERIFY(source.contains(QStringLiteral("ScrollNone")));
    QVERIFY(source.contains(QStringLiteral("ScrollTasks")));
    QVERIFY(source.contains(QStringLiteral("ScrollToggleMinimized")));
}

void QmlSmokeTest::parabolicAreaMouseAreaStructuralGuardsPresent()
{
    // Verify the ParabolicArea.qml MouseArea has the guards that keep the
    // wave (parabolic) animation smooth and clicks working for widget applets:
    //   1. acceptedButtons: Qt.NoButton — clicks pass through to the applet
    //   2. onPositionChanged handler — synchronous local position tracking
    //   3. onExited handler — immediate cleanup, no 80 ms C++ nullifier delay
    //   4. Visible condition does NOT hide when item is current (no longer
    //      depends on currentParabolicItem !== _parabolicArea)
    QFile file(QStringLiteral(LATTE_PARABOLICAREA_QML));
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString source = QString::fromUtf8(file.readAll());

    // Must not consume button presses — clicks reach the underlying applet
    QVERIFY(source.contains(QStringLiteral("acceptedButtons: Qt.NoButton")));

    // Must have local onPositionChanged for synchronous parabolic updates
    QVERIFY(source.contains(QStringLiteral("onPositionChanged:")));

    // Must have local onExited for immediate cleanup
    QVERIFY(source.contains(QStringLiteral("onExited:")));

    // MouseArea must be always visible when parabolic is supported.
    // The old guard "currentParabolicItem !== _parabolicArea" that hid the
    // MouseArea when the item was the current parabolic target must be gone
    // from the visible: binding specifically.
    //
    // Find the MouseArea visible: line and verify it does not include the
    // old currentParabolicItem guard.
    int mouseAreaIndex = source.indexOf(QStringLiteral("id: parabolicMouseArea"));
    QVERIFY(mouseAreaIndex > 0);
    int visibleLine = source.indexOf(QStringLiteral("\n        visible:"), mouseAreaIndex);
    QVERIFY(visibleLine > 0);
    int visibleEnd = source.indexOf(QLatin1Char('\n'), visibleLine + 1);
    QVERIFY(visibleEnd > 0);
    const QString visibleBinding = source.mid(visibleLine, visibleEnd - visibleLine);
    QVERIFY2(!visibleBinding.contains(QStringLiteral("currentParabolicItem")),
             "MouseArea visible: must not hide when item is current. "
             "visible binding should only gate on parabolicEffectIsSupported "
             "and !indexerIsSupported.");

    // Visible must still gate on parabolic support (not indexer-supported aka tasks)
    QVERIFY(source.contains(QStringLiteral("parabolicEffectIsSupported")));
    QVERIFY(source.contains(QStringLiteral("!communicator.indexerIsSupported")));
}

void QmlSmokeTest::zoomRestoreDelegatedToBehaviorAnimation()
{
    // Commit baff151b7 reworked zoom recovery: instead of the old
    // onParabolicExited / onStopped guards that force zoom = 1 directly
    // (which caused jarring visual jumps and interrupted in-flight
    // animations), the restore is now delegated to the Behavior on zoom
    // inside ParabolicItem. Any code that needs to clear the zoom simply
    // sets the target value (parabolicItem.zoom = 1) and BehaviorAnimation
    // animates toward it smoothly when direct rendering is disabled.
    //
    // Verify the new restore path end to end:
    //   1. BasicItem.slotClearZoom — sets the zoom target, delegating the
    //      actual restore animation to the Behavior below
    //   2. ParabolicItem — "Behavior on zoom" + NumberAnimation is the real
    //      animator that restores the zoom smoothly (3 * animationTime when
    //      direct rendering is disabled, instant otherwise)
    //   3. ParabolicEventsArea — onParabolicExited keeps the tooltip cleanup;
    //      the only zoom gating left is restoreZoomIsBlocked inside
    //      calculateParabolicScales (contextual blocking, not a restore guard)
    //   4. LauncherAnimation.onStopped — must always re-enable direct
    //      rendering; the Behavior reads directRenderingEnabled to decide
    //      between animated and instant restore
    QFile basicItem(QStringLiteral(LATTE_SOURCE_DIR "/declarativeimports/abilities/items/BasicItem.qml"));
    QVERIFY(basicItem.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString basicItemSource = QString::fromUtf8(basicItem.readAll());

    const int slotClearZoomPos = basicItemSource.indexOf(QStringLiteral("function slotClearZoom()"));
    QVERIFY(slotClearZoomPos >= 0);
    QVERIFY(basicItemSource.mid(slotClearZoomPos).contains(QStringLiteral("parabolicItem.zoom = 1")));

    QFile parabolicItem(QStringLiteral(LATTE_PARABOLIC_ITEM_QML));
    QVERIFY(parabolicItem.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString parabolicItemSource = QString::fromUtf8(parabolicItem.readAll());

    QVERIFY(parabolicItemSource.contains(QStringLiteral("Behavior on zoom")));
    QVERIFY(parabolicItemSource.contains(QStringLiteral("NumberAnimation")));

    QFile eventsArea(QStringLiteral(LATTE_PARABOLIC_EVENTS_AREA_QML));
    QVERIFY(eventsArea.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString eventsAreaSource = QString::fromUtf8(eventsArea.readAll());

    QVERIFY(eventsAreaSource.contains(QStringLiteral("onParabolicExited:")));
    QVERIFY(eventsAreaSource.contains(QStringLiteral("restoreZoomIsBlocked")));

    QFile launcherAnimation(QStringLiteral(LATTE_LAUNCHER_ANIMATION_QML));
    QVERIFY(launcherAnimation.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString launcherAnimationSource = QString::fromUtf8(launcherAnimation.readAll());

    QVERIFY(launcherAnimationSource.contains(QStringLiteral("function onStopped()")));
    QVERIFY(launcherAnimationSource.contains(QStringLiteral("setDirectRenderingEnabled")));

    //! The old force-reset guards must not silently return: the restore is
    //! delegated to the Behavior on zoom, not to exit-path zoom assignments.
    QVERIFY(!eventsAreaSource.contains(QStringLiteral("parabolicItem.zoom = 1")));
    QVERIFY(!launcherAnimationSource.contains(QStringLiteral("parabolicAreaContainsMouse")));
}

void QmlSmokeTest::taskIconRasterSizeStaysStableDuringZoom()
{
    QFile file(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/task/TaskIcon.qml"));
    QVERIFY(file.open(QFile::ReadOnly));
    const QString source = QString::fromUtf8(file.readAll());
    const int start = source.indexOf(QStringLiteral("    Item {\n        id: taskIconItem"));
    const int end = source.indexOf(QStringLiteral("        ///states for launcher animation"), start);
    QVERIFY(start >= 0 && end > start);
    // Exercise the production wrapper with real Kirigami sizing. Animation
    // anchors and the platform icon resolver are outside this geometry test.
    QString wrapper = source.mid(start, end - start) + QStringLiteral("\n}");
    wrapper.replace(QStringLiteral("LatteCore.Environment.iconSourceForTheme(decoration)"), QStringLiteral("\"application-x-executable\""));
    const QString harness = QStringLiteral(R"(
import QtQuick
import org.kde.kirigami as Kirigami
Item {
    id: taskIconContainer
    width: 48; height: 48
    property real iconSize: 48
    property real zoom: 1.6
    property real maxZoom: 1.8
    property alias raster: taskIconRaster
    property alias icon: taskIconItem
    QtObject {
        id: taskItem
        property var abilities: ({metrics: {iconSize: 48}, parabolic: {factor: {zoom: 1.6, maxZoom: 1.8}}})
    }
)") + wrapper + QStringLiteral("\n}");
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(harness.toUtf8(), QUrl(QStringLiteral("task-icon-raster.qml")));
    QTRY_VERIFY_WITH_TIMEOUT(component.status() != QQmlComponent::Loading, 5000);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    QVERIFY(object);
    auto *slot = qobject_cast<QQuickItem *>(object.get());
    auto *raster = qvariant_cast<QQuickItem *>(object->property("raster"));
    auto *icon = qvariant_cast<QQuickItem *>(object->property("icon"));
    QVERIFY(slot && raster && icon);
    QCOMPARE(raster->width(), 48.0);
    QCOMPARE(raster->scale(), 1.0);
    for (int length = 49; length <= 86; ++length) {
        slot->setSize(QSizeF(length, length));
        QCOMPARE(raster->width(), 87.0);
        QCOMPARE(raster->height(), 87.0);
        QVERIFY(std::abs(raster->width() * raster->scale() - length) < 0.001);
        // Qt centers anchors on whole pixels, allowing a half-pixel offset
        // when the stable raster and slot have different parity.
        QVERIFY(std::abs(raster->x() + raster->width() / 2 - length / 2.0) <= 0.5);
    }
    // Returning to rest restores native pixels, and the public aliases still
    // reach the raster after a source refresh or active-state change.
    slot->setSize(QSizeF(48, 48));
    QCOMPARE(raster->width(), 48.0);
    QCOMPARE(raster->scale(), 1.0);
    QVERIFY(icon->setProperty("source", QStringLiteral("folder")));
    QCOMPARE(raster->property("source").toString(), QStringLiteral("folder"));
    QVERIFY(icon->setProperty("active", true));
    QCOMPARE(raster->property("active").toBool(), true);
}

class MinimizedSectionTestModel : public QStandardItemModel
{
    Q_OBJECT
    Q_PROPERTY(QStringList launcherList READ launcherList CONSTANT)

  public:
    enum Role
    {
        Window = Qt::UserRole + 1,
        Minimized,
        Group,
        WindowIds,
        LauncherUrl,
        ApplicationId,
        ApplicationName
    };

    QHash<int, QByteArray> roleNames() const override
    {
        auto roles = QStandardItemModel::roleNames();
        roles.insert(Window, "IsWindow");
        roles.insert(Minimized, "IsMinimized");
        roles.insert(Group, "IsGroupParent");
        roles.insert(WindowIds, "WinIdList");
        roles.insert(LauncherUrl, "LauncherUrlWithoutIcon");
        roles.insert(ApplicationId, "AppId");
        roles.insert(ApplicationName, "AppName");
        return roles;
    }

    QStringList launcherList() const { return { QStringLiteral("pinned") }; }
    Q_INVOKABLE int launcherPosition(const QString &url) const { return launcherList().indexOf(url); }
    Q_INVOKABLE bool inCurrentActivity(const QString &url) const { return launcherList().contains(url); }
    Q_INVOKABLE QModelIndex makeModelIndex(int row, int child = -1) const
    {
        const auto parent = index(row, 0);
        return child < 0 ? parent : index(child, 0, parent);
    }
    Q_INVOKABLE void requestToggleMinimized(const QModelIndex &index) { setData(index, !index.data(Minimized).toBool(), Minimized); }
    Q_INVOKABLE void requestActivate(const QModelIndex &index) { activatedWindow = index.data(WindowIds).toList().value(0).toString(); }

    QString activatedWindow;
};

void
QmlSmokeTest::minimizedSectionTracksGroupedWindows()
{
    MinimizedSectionTestModel model;
    const auto addWindow = [](QStandardItem *parent, const QString &id, bool minimized) {
        auto *window = new QStandardItem(id);
        window->setData(true, MinimizedSectionTestModel::Window);
        window->setData(minimized, MinimizedSectionTestModel::Minimized);
        window->setData(QVariantList{ id }, MinimizedSectionTestModel::WindowIds);
        window->setData(parent->data(MinimizedSectionTestModel::ApplicationId), MinimizedSectionTestModel::ApplicationId);
        window->setData(parent->data(MinimizedSectionTestModel::ApplicationName), MinimizedSectionTestModel::ApplicationName);
        parent->appendRow(window);
        return window;
    };

    auto *group = new QStandardItem(QStringLiteral("Pinned application"));
    group->setData(true, MinimizedSectionTestModel::Group);
    group->setData(QStringLiteral("pinned"), MinimizedSectionTestModel::LauncherUrl);
    group->setData(QStringLiteral("pinned.application"), MinimizedSectionTestModel::ApplicationId);
    group->setData(QStringLiteral("Pinned application"), MinimizedSectionTestModel::ApplicationName);
    model.appendRow(group);
    auto *ordinary = addWindow(group, QStringLiteral("ordinary"), false);
    addWindow(group, QStringLiteral("minimized"), true);
    addWindow(model.invisibleRootItem(), QStringLiteral("unpinned"), true);

    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(LATTE_SOURCE_DIR "/plasmoid/package/contents/ui/task/MinimizedWindows.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    const QVariant source = QVariant::fromValue(static_cast<QObject *>(&model));
    std::unique_ptr<QObject> section(component.createWithInitialProperties({
      { QStringLiteral("sourceModel"), source },
      { QStringLiteral("launcherAbility"), source },
      { QStringLiteral("activity"), QStringLiteral("activity") },
      { QStringLiteral("onlyFromLaunchers"), false },
    }));
    QVERIFY2(section, qPrintable(component.errorString()));
    // The active ordinary window follows a minimized sibling in source order.
    // Its only visible dot must receive index zero.
    int activeDot = -1;
    QVERIFY(QMetaObject::invokeMethod(section.get(), "ordinaryActiveWindowIndex", Q_RETURN_ARG(int, activeDot),
                                      Q_ARG(int, 1), Q_ARG(QVariant, QVariant::fromValue(QVariantList{true, false}))));
    QCOMPARE(activeDot, 0);
    const QVariant mixedWindows = QVariantList{true, false, false};
    QVERIFY(QMetaObject::invokeMethod(section.get(), "ordinaryActiveWindowIndex", Q_RETURN_ARG(int, activeDot),
                                      Q_ARG(int, 2), Q_ARG(QVariant, mixedWindows)));
    QCOMPARE(activeDot, 1);

    QObject *entries = section->property("orderedWindows").value<QObject *>();
    QVERIFY(entries);
    QTRY_COMPARE(entries->property("count").toInt(), 2);

    section->setProperty("onlyFromLaunchers", true);
    QTRY_COMPARE(entries->property("count").toInt(), 1);
    ordinary->setData(true, MinimizedSectionTestModel::Minimized);
    QTRY_COMPARE(entries->property("count").toInt(), 2);

    const QJSValue entriesObject = engine.newQObject(entries);
    const QJSValue firstEntry = entriesObject.property(QStringLiteral("get")).callWithInstance(entriesObject, QJSValueList{ 0 });
    QVERIFY(!firstEntry.isError());
    QCOMPARE(firstEntry.property(QStringLiteral("applicationOrdinal")).toInt(), 1);
    QCOMPARE(firstEntry.property(QStringLiteral("applicationCount")).toInt(), 2);
    QCOMPARE(firstEntry.property(QStringLiteral("applicationBoundary")).toBool(), true);
    const QJSValue secondEntry = entriesObject.property(QStringLiteral("get")).callWithInstance(entriesObject, QJSValueList{ 1 });
    QVERIFY(!secondEntry.isError());
    QCOMPARE(secondEntry.property(QStringLiteral("applicationKey")).toString(), QStringLiteral("pinned.application"));
    QCOMPARE(secondEntry.property(QStringLiteral("applicationOrdinal")).toInt(), 2);
    QCOMPARE(secondEntry.property(QStringLiteral("applicationCount")).toInt(), 2);
    QCOMPARE(secondEntry.property(QStringLiteral("applicationBoundary")).toBool(), false);

    QVERIFY(QMetaObject::invokeMethod(section.get(), "activate", Q_ARG(QVariant, QStringLiteral("minimized"))));
    QCOMPARE(model.activatedWindow, QStringLiteral("minimized"));
    QVERIFY(ordinary->data(MinimizedSectionTestModel::Minimized).toBool());
    QTRY_COMPARE(entries->property("count").toInt(), 1);
}

QTEST_MAIN(QmlSmokeTest)
#include "qmlsmoketest.moc"
