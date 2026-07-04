/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>

#include <QColor>
#include <QDir>
#include <QFile>
#include <QMetaObject>
#include <QTemporaryDir>
#include <QTest>

#include <cmath>
#include <memory>
#include <utility>

class QmlSmokeTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void latteCoreQmlPluginLoadsFromBuildTree();
    void restoreAnimationLoadsFromSource();
    void showWindowAnimationFrozenZoomDecisionLoadsFromSource();
    void parabolicItemZoomRecoveryLoadsFromSource();
    void compactAppletPopupSizingLoadsFromSource();
    void launchersGeometryRestoreSchedulingLoadsFromSource();
    void plasmaVolumeBootstrapLoadsFromSource();
    void taskMouseAreaAllClickActionsPresent();
    void taskMouseAreaRegressionGuardsPresent();
    void tasksConfigComboModelsComplete();
    void tasksConfigCfgPropertyAndNoStaleTasksConfig();
    void configInteractionHoverActionNotHardcoded();
};

class ParabolicTargetStub : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)

public:
    qreal zoom() const
    {
        return m_zoom;
    }

    void setZoom(qreal zoom)
    {
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
    Q_INVOKABLE void addEvent(const QString &)
    {
        ++m_addCount;
    }

    Q_INVOKABLE void removeEvent(const QString &)
    {
        ++m_removeCount;
    }

    int addCount() const
    {
        return m_addCount;
    }

    int removeCount() const
    {
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
    QVariantMap factor() const
    {
        return QVariantMap{
            {QStringLiteral("zoom"), 1.6},
            {QStringLiteral("marginThicknessZoomInPercentage"), 0.0},
        };
    }

    bool directRenderingEnabled() const
    {
        return m_directRenderingEnabled;
    }

    void setDirectRenderingEnabled(bool enabled)
    {
        if (m_directRenderingEnabled == enabled) {
            return;
        }

        m_directRenderingEnabled = enabled;
        Q_EMIT directRenderingEnabledChanged();
    }

    bool isEnabled() const
    {
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
    explicit AbilityItemStub(QObject *parabolicItem, QObject *parent = nullptr)
        : QObject(parent)
        , m_parabolicItem(parabolicItem)
    {
    }

    QObject *parabolicItem() const
    {
        return m_parabolicItem;
    }

    QVariantMap abilities() const
    {
        return QVariantMap{
            {QStringLiteral("metrics"),
             QVariantMap{
                 {QStringLiteral("iconSize"), 48},
                 {QStringLiteral("mask"),
                  QVariantMap{{QStringLiteral("thickness"), QVariantMap{{QStringLiteral("normalForItems"), 48}, {QStringLiteral("zoomedForItems"), 64}}}}},
                 {QStringLiteral("margin"), QVariantMap{{QStringLiteral("screenEdge"), 0}, {QStringLiteral("tailThickness"), 0}}},
                 {QStringLiteral("marginsArea"), QVariantMap{{QStringLiteral("iconSize"), 48}, {QStringLiteral("tailThickness"), 0}}},
                 {QStringLiteral("totals"),
                  QVariantMap{{QStringLiteral("length"), 48}, {QStringLiteral("thicknessEdges"), 0}, {QStringLiteral("lengthPaddings"), 0}}},
             }},
            {QStringLiteral("parabolic"), QVariant::fromValue(static_cast<QObject *>(const_cast<ParabolicAbilityStub *>(&m_parabolic)))},
            {QStringLiteral("animations"),
             QVariantMap{{QStringLiteral("needBothAxis"), QVariant::fromValue(static_cast<QObject *>(const_cast<EventSinkStub *>(&m_needBothAxis)))}}},
            {QStringLiteral("myView"),
             QVariantMap{{QStringLiteral("itemShadow"), QVariantMap{{QStringLiteral("isEnabled"), false}, {QStringLiteral("shadowColor"), QColor(Qt::black)}}},
                         {QStringLiteral("badgesIn3DStyle"), false}}},
            {QStringLiteral("environment"), QVariantMap{{QStringLiteral("isGraphicsSystemAccelerated"), false}}},
            {QStringLiteral("indexer"), QVariantMap{{QStringLiteral("inMarginsArea"), false}}},
            {QStringLiteral("debug"), QVariantMap{{QStringLiteral("graphicsEnabled"), false}}},
            {QStringLiteral("shortcuts"),
             QVariantMap{{QStringLiteral("showPositionShortcutBadges"), false},
                         {QStringLiteral("isEnabled"), false},
                         {QStringLiteral("badges"), QStringList{}},
                         {QStringLiteral("shortcutIndex"), QVariant::fromValue(static_cast<QObject *>(const_cast<AbilityItemStub *>(this)))}}},
        };
    }

    int animationTime() const
    {
        return 10;
    }

    bool isHorizontal() const
    {
        return true;
    }

    bool isVertical() const
    {
        return false;
    }

    bool isVisible() const
    {
        return m_visible;
    }

    void setVisible(bool visible)
    {
        if (m_visible == visible) {
            return;
        }

        m_visible = visible;
        Q_EMIT visibleChanged();
    }

    bool isSeparator() const
    {
        return false;
    }

    bool isHidden() const
    {
        return false;
    }

    int location() const
    {
        return 4;
    }

    qreal iconOffsetX() const
    {
        return 0;
    }

    qreal iconOffsetY() const
    {
        return 0;
    }

    int iconTransformOrigin() const
    {
        return 4;
    }

    qreal iconOpacity() const
    {
        return 1.0;
    }

    qreal iconRotation() const
    {
        return 0;
    }

    qreal iconScale() const
    {
        return 1.0;
    }

    QObject *contentItem() const
    {
        return nullptr;
    }

    bool isMonochromaticForcedContentItem() const
    {
        return false;
    }

    QObject *monochromizedItem() const
    {
        return nullptr;
    }

    int itemIndex() const
    {
        return 1;
    }

    bool parabolicAreaContainsMouse() const
    {
        return false;
    }

    Q_INVOKABLE int shortcutIndex(int) const
    {
        return -1;
    }

    int needBothAxisAddCount() const
    {
        return m_needBothAxis.addCount();
    }

    int needBothAxisRemoveCount() const
    {
        return m_needBothAxis.removeCount();
    }

Q_SIGNALS:
    void visibleChanged();
    void contentItemChanged();
    void parabolicAreaLastMousePosChanged();
    void itemIndexChanged();

private:
    QObject *m_parabolicItem{nullptr};
    bool m_visible{true};
    ParabolicAbilityStub m_parabolic;
    EventSinkStub m_needBothAxis;
};

class PlasmoidStub : public QObject
{
    Q_OBJECT

public:
    void emitFormFactorChanged()
    {
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
        , m_launchers(std::move(launchers))
    {
    }

    QStringList launchers59() const
    {
        return m_launchers;
    }

    void setLaunchers59(const QStringList &launchers)
    {
        m_launchers = launchers;
        Q_EMIT launchers59Changed();
    }

    bool userConfiguring() const
    {
        return m_userConfiguring;
    }

    void setUserConfiguring(bool userConfiguring)
    {
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
        , m_configuration(std::move(launchers), this)
    {
    }

    int id() const
    {
        return 1;
    }

    QObject *configuration()
    {
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
    bool isReady() const
    {
        return m_ready;
    }

    void setReady(bool ready)
    {
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
    QObject *myView()
    {
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
        , m_parabolicItem(parabolicItem)
    {
    }

    QObject *parabolicItem() const
    {
        return m_parabolicItem;
    }

    QVariantMap abilities() const
    {
        return QVariantMap{
            {QStringLiteral("animations"),
             QVariantMap{
                 {QStringLiteral("speedFactor"), QVariantMap{{QStringLiteral("normal"), 1}, {QStringLiteral("current"), 1}}},
                 {QStringLiteral("duration"), QVariantMap{{QStringLiteral("large"), 100}}},
                 {QStringLiteral("needLength"), QVariant::fromValue(static_cast<QObject *>(const_cast<TaskItemStub *>(this)))},
             }},
            {QStringLiteral("metrics"), QVariantMap{{QStringLiteral("iconSize"), 48}}},
            {QStringLiteral("launchers"), QVariant::fromValue(static_cast<QObject *>(const_cast<TaskItemStub *>(this)))},
        };
    }

    bool parabolicAreaIsCurrent() const
    {
        return m_parabolicAreaIsCurrent;
    }

    void setParabolicAreaIsCurrent(bool current)
    {
        m_parabolicAreaIsCurrent = current;
        Q_EMIT parabolicAreaChanged();
    }

    bool parabolicAreaContainsMouse() const
    {
        return m_parabolicAreaContainsMouse;
    }

    void setParabolicAreaContainsMouse(bool containsMouse)
    {
        m_parabolicAreaContainsMouse = containsMouse;
        Q_EMIT parabolicAreaChanged();
    }

    bool isVertical() const
    {
        return false;
    }

    bool isWindow() const
    {
        return true;
    }

    bool isStartup() const
    {
        return false;
    }

    bool isLauncher() const
    {
        return false;
    }

    bool isSeparator() const
    {
        return false;
    }

    QString launcherUrl() const
    {
        return QStringLiteral("applications:test.desktop");
    }

    QString launcherUrlWithIcon() const
    {
        return launcherUrl();
    }

    Q_INVOKABLE void addEvent(const QString &)
    {
    }

    Q_INVOKABLE void removeEvent(const QString &)
    {
    }

    Q_INVOKABLE bool inCurrentActivity(const QString &) const
    {
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
    QStringList launcherList() const
    {
        return m_launcherList;
    }

    void setLauncherList(const QStringList &launcherList)
    {
        m_launcherList = launcherList;
        Q_EMIT launcherListChanged();
    }

    int count() const
    {
        return m_launcherList.count();
    }

    int syncCount() const
    {
        return m_syncCount;
    }

    Q_INVOKABLE int launcherPosition(const QString &launcher) const
    {
        return m_launcherList.indexOf(launcher);
    }

    Q_INVOKABLE QStringList launcherActivities(const QString &) const
    {
        return {};
    }

    Q_INVOKABLE void syncLaunchers()
    {
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

void QmlSmokeTest::restoreAnimationLoadsFromSource()
{
    QQmlEngine engine;
    QQmlContext context(engine.rootContext());
    ParabolicTargetStub parabolicTarget;
    AbilityItemStub abilityItem(&parabolicTarget);
    context.setContextProperty(QStringLiteral("abilityItem"), &abilityItem);

    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(LATTE_RESTORE_ANIMATION_QML)));
    std::unique_ptr<QObject> object(component.create(&context));
    if (!object) {
        qWarning() << component.errors();
    }

    QVERIFY(object);
    const auto animations = object->findChildren<QObject *>();
    QObject *zoomAnimation = nullptr;
    for (QObject *animation : animations) {
        if (animation->property("property").toString() == QStringLiteral("zoom")) {
            zoomAnimation = animation;
            break;
        }
    }

    QVERIFY(zoomAnimation);
    QCOMPARE(zoomAnimation->property("target").value<QObject *>(), &parabolicTarget);
    QCOMPARE(zoomAnimation->property("to").toReal(), 1.0);
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

    QQmlContext context(engine.rootContext());
    ParabolicTargetStub parabolicTarget;
    AbilityItemStub abilityItem(&parabolicTarget);
    PlasmoidStub plasmoid;
    QVariantMap restoreAnimation{{QStringLiteral("running"), false}};
    QVariantMap theme{
        {QStringLiteral("textColor"), QColor(Qt::white)},
        {QStringLiteral("backgroundColor"), QColor(Qt::black)},
    };

    context.setContextProperty(QStringLiteral("abilityItem"), &abilityItem);
    context.setContextProperty(QStringLiteral("plasmoid"), &plasmoid);
    context.setContextProperty(QStringLiteral("restoreAnimation"), restoreAnimation);
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

void QmlSmokeTest::compactAppletPopupSizingLoadsFromSource()
{
    QQmlEngine engine;
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

    // canPresentWindowsIsSupported guard must still exist for PresentWindows fallback
    QVERIFY(source.contains(QStringLiteral("canPresentWindowsIsSupported")));

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

QTEST_MAIN(QmlSmokeTest)
#include "qmlsmoketest.moc"
