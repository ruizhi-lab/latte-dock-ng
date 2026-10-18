/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>
#include <QSignalSpy>
#include <QWheelEvent>
#include <QtQml/qqml.h>

#include <QColor>
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
    void containerWheelYieldsToTaskOverflowScrolling();
    void containmentWheelScrollsTaskViewport();
    void edgeHoverScrollsWithStationaryPointer_data();
    void edgeHoverScrollsWithStationaryPointer();
    void scrollingTasksCanRequestShorterViewports();
    void scrollingTasksKeepFillRequestsInModernDocks();
    void scrollingTasksShareRemainingModernDockLength();
    void latteCoreQmlPluginLoadsFromBuildTree();
    void layoutInfoWindowHasAValidSizeFromSource();
    void appletAlternativesDialogResolvesOutsidePlasmashell();
    void showWindowAnimationFrozenZoomDecisionLoadsFromSource();
    void parabolicItemZoomRecoveryLoadsFromSource();
    void taskRemovalDefersGeometryFeedback_data();
    void taskRemovalDefersGeometryFeedback();
    void compactAppletPopupSizingLoadsFromSource();
    void globalMenuBackendStatusTransitionsLoadFromSource();
    void globalMenuBackendStatusTransitionsLoadFromSource_data();
    void globalMenuNativeLengthUpdatesWithoutClassifier();
    void globalMenuReflowsNeighboursSynchronously();
    void launchersGeometryRestoreSchedulingLoadsFromSource();
    void plasmaVolumeBootstrapLoadsFromSource();
    void taskMouseAreaAllClickActionsPresent();
    void taskPresentationFiltersPhantoms();
    void taskMouseAreaRegressionGuardsPresent();
    void taskMouseAreaStructuralGuardsPresent();
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

    Q_INVOKABLE void syncLaunchers() {
        ++m_syncCount;
    }

Q_SIGNALS:
    void launcherListChanged();

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
    property var filters
    property var sourceModel
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
    const QVariantMap types{ { QStringLiteral("ScrollNone"), 0 },
                             { QStringLiteral("ScrollDesktops"), 1 },
                             { QStringLiteral("ScrollActivities"), 2 },
                             { QStringLiteral("ScrollToggleMinimized"), 4 },
                             { QStringLiteral("ManualScrollOnlyParallel"), 1 } };
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
    property bool manualScrollTasksEnabled: true
    property int manualScrollTasksType: 2
    property int scrollAction: 3
    property int windowActions: 0
    property bool audioHandled: false
    property var appletAbilities: ({myView: {isShownFully: true}})
    property var plasmoid: ({applets: [{pluginName: "org.kde.latte.plasmoid"}]})
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
        readonly property bool contentsExceed: true
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
    auto sendWheel = [&](QPoint delta) {
        const QPoint position(120, 120);
        QWheelEvent wheel(QPointF(position), QPointF(view.mapToGlobal(position)), QPoint(), delta, Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
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
    QVERIFY(object->setProperty("manualScrollTasksEnabled", false));
    sendWheel(QPoint(0, -120));
    QCOMPARE(object->property("windowActions").toInt(), 2);
}

void
QmlSmokeTest::containerWheelYieldsToTaskOverflowScrolling()
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
    QCOMPARE(object->property("receivedTaskWheels").toInt(), 1);
    QCOMPARE(object->property("containerDelta").toInt(), 0);
    // Without manual overflow scrolling, preserve the global window action.
    QVERIFY(object->setProperty("manualScrollTasksEnabled", false));
    sendWheel();
    QCOMPARE(object->property("receivedTaskWheels").toInt(), 1);
    QCOMPARE(object->property("containerDelta").toInt(), 12345);
    QVERIFY(object->setProperty("containerDelta", 0));
    QVERIFY(object->setProperty("manualScrollTasksEnabled", true));
    QVERIFY(object->setProperty("overflow", false));
    sendWheel();
    QCOMPARE(object->property("receivedTaskWheels").toInt(), 1);
    QCOMPARE(object->property("containerDelta").toInt(), 12345);
    QVERIFY(object->setProperty("overflow", true));
    sendWheel();
    QCOMPARE(object->property("receivedTaskWheels").toInt(), 2);
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
        property bool isModernDockStyle: true
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
    // Since libplasma 6.8 Plasma's AppletAlternatives.qml requires the
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
    std::unique_ptr<QObject> object(component.create(&context));
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
        QStringLiteral("applications:org.kde.dolphin.desktop"),
        QStringLiteral("applications:org.kde.konsole.desktop"),
    };
    const QStringList normalizedLaunchers{
        QStringLiteral("preferred://filemanager"),
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
    QVERIFY(object->setProperty("tasksModel", QVariant::fromValue(static_cast<QObject *>(&tasksModel))));

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

QTEST_MAIN(QmlSmokeTest)
#include "qmlsmoketest.moc"
