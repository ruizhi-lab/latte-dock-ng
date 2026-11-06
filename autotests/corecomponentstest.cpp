/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "apptypes.h"
#include "plasma/extended/backgroundcache.h"
#include "plasma/extended/backgroundtracker.h"
#include "plasma/extended/screenpool.h"
#include "primaryoutputwatcher.h"
#include "screenpool.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QScreen>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

class TestPrimaryOutputWatcher : public Latte::PrimaryOutputWatcher
{
public:
    explicit TestPrimaryOutputWatcher(QObject *parent = nullptr)
        : Latte::PrimaryOutputWatcher(parent) {
    }

    using Latte::PrimaryOutputWatcher::setPrimaryOutputName;
};

class TestScreenPool : public Latte::ScreenPool
{
public:
    explicit TestScreenPool(KSharedConfig::Ptr config)
        : Latte::ScreenPool(config) {
    }

    using Latte::ScreenPool::firstAvailableId;
};

class CoreComponentsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void appTypesMatchOnlyKnownLatteApplicationIds();
    void primaryOutputWatcherFindsScreensAndSignalsRealChanges();
    void screenPoolAllocatesStableIdsAndRejectsInvalidConnectors();
    void screenPoolLoadsStoredConnectorsAndIgnoresInvalidIds();
    void plasmaExtendedScreenPoolRejectsInvalidConnectors();
    void screenPoolIsScreenActiveRequiresAttachedScreen();
    void backgroundCacheHandlesBroadcastedSmallImagesAtEveryEdge();
    void backgroundTrackerEmitsOnlyForChangedHints();
};

void CoreComponentsTest::appTypesMatchOnlyKnownLatteApplicationIds()
{
    QCOMPARE(Latte::App::preferredWaylandAppId(), QStringLiteral("latte-dock-ng"));
    QVERIFY(Latte::App::matchesSelfAppId(QStringLiteral("latte-dock-ng")));
    QVERIFY(Latte::App::matchesSelfAppId(QStringLiteral("latte-dock")));
    QVERIFY(Latte::App::matchesSelfAppId(QStringLiteral("org.kde.latte-dock")));
    QVERIFY(!Latte::App::matchesSelfAppId(QStringLiteral("org.kde.plasmashell")));
    QVERIFY(!Latte::App::matchesSelfAppId(QStringLiteral("")));
}

void CoreComponentsTest::primaryOutputWatcherFindsScreensAndSignalsRealChanges()
{
    TestPrimaryOutputWatcher watcher;
    QScreen *primary = qGuiApp->primaryScreen();
    QVERIFY(primary);

    QCOMPARE(watcher.screenForName(primary->name()), primary);
    QCOMPARE(watcher.screenForName(QStringLiteral("missing-output")), nullptr);

    QSignalSpy changedSpy(&watcher, &Latte::PrimaryOutputWatcher::primaryOutputNameChanged);
    watcher.setPrimaryOutputName(primary->name());
    QCOMPARE(changedSpy.count(), 0);

    watcher.setPrimaryOutputName(QStringLiteral("missing-output"));
    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(changedSpy.first().at(1).toString(), QStringLiteral("missing-output"));
    QCOMPARE(watcher.primaryScreen(), primary);
}

void CoreComponentsTest::screenPoolAllocatesStableIdsAndRejectsInvalidConnectors()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString configPath = dir.path() + QStringLiteral("/lattedockrc");
    KSharedConfig::Ptr config = KSharedConfig::openConfig(configPath);

    TestScreenPool pool(config);
    QCOMPARE(pool.firstAvailableId(), Latte::ScreenPool::FIRSTSCREENID);
    QCOMPARE(pool.id(QStringLiteral("HDMI-A-1")), Latte::ScreenPool::NOSCREENID);

    pool.insertScreenMapping(QStringLiteral(":0.0"));
    QCOMPARE(pool.screensTable().rowCount(), 0);

    pool.insertScreenMapping(QStringLiteral("HDMI-A-1"));
    QCOMPARE(pool.id(QStringLiteral("HDMI-A-1")), Latte::ScreenPool::FIRSTSCREENID);
    QCOMPARE(pool.connector(Latte::ScreenPool::FIRSTSCREENID), QStringLiteral("HDMI-A-1"));
    QCOMPARE(pool.firstAvailableId(), Latte::ScreenPool::FIRSTSCREENID + 1);

    pool.insertScreenMapping(QStringLiteral("HDMI-A-1"));
    QCOMPARE(pool.screensTable().rowCount(), 1);
}

void CoreComponentsTest::screenPoolLoadsStoredConnectorsAndIgnoresInvalidIds()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString configPath = dir.path() + QStringLiteral("/lattedockrc");
    KSharedConfig::Ptr config = KSharedConfig::openConfig(configPath);
    KConfigGroup group(config, QStringLiteral("ScreenConnectors"));
    Latte::Data::Screen hdmi;
    hdmi.name = QStringLiteral("HDMI-A-1");
    hdmi.geometry = QRect(0, 0, 1920, 1080);
    Latte::Data::Screen displayPort;
    displayPort.name = QStringLiteral("DP-1");
    displayPort.geometry = QRect(1920, 0, 1920, 1080);

    group.writeEntry(QStringLiteral("0"), QStringLiteral("ignored-primary"));
    group.writeEntry(QStringLiteral("-1"), QStringLiteral("ignored-negative"));
    group.writeEntry(QStringLiteral("10"), hdmi.serialize());
    group.writeEntry(QStringLiteral("11"), displayPort.serialize());
    group.sync();

    TestScreenPool pool(config);
    pool.load();

    QCOMPARE(pool.id(QStringLiteral("HDMI-A-1")), 10);
    QCOMPARE(pool.id(QStringLiteral("DP-1")), 11);
    QCOMPARE(pool.connector(10), QStringLiteral("HDMI-A-1"));
    QCOMPARE(pool.connector(11), QStringLiteral("DP-1"));
    QVERIFY(!pool.hasScreenId(0));
    QVERIFY(!pool.hasScreenId(-1));
    QCOMPARE(pool.firstAvailableId(), 12);
}

void CoreComponentsTest::plasmaExtendedScreenPoolRejectsInvalidConnectors()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    qputenv("XDG_CONFIG_HOME", dir.path().toUtf8());

    Latte::PlasmaExtended::ScreenPool pool;
    QVERIFY(QMetaObject::invokeMethod(&pool, "insertScreenMapping",
                                      Q_ARG(int, 1),
                                      Q_ARG(QString, QString())));
    QVERIFY(QMetaObject::invokeMethod(&pool, "insertScreenMapping",
                                      Q_ARG(int, 2),
                                      Q_ARG(QString, QStringLiteral(":0.0"))));
    QVERIFY(QMetaObject::invokeMethod(&pool, "insertScreenMapping",
                                      Q_ARG(int, 3),
                                      Q_ARG(QString, QStringLiteral("HDMI-A-1"))));

    QCOMPARE(pool.connector(1), QString());
    QCOMPARE(pool.connector(2), QString());
    QCOMPARE(pool.connector(3), QStringLiteral("HDMI-A-1"));
    QCOMPARE(pool.id(QStringLiteral("HDMI-A-1")), 3);
}

void CoreComponentsTest::screenPoolIsScreenActiveRequiresAttachedScreen()
{
    // ScreenPool::isScreenActive() is true only for a mapped id whose connector
    // names a screen that is attached now. A mapping for a detached monitor
    // stays in the table, so the check must not count it as active; otherwise
    // views would be placed on a screen that no longer exists.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    KSharedConfig::Ptr config = KSharedConfig::openConfig(dir.path() + QStringLiteral("/lattedockrc"));

    TestScreenPool pool(config);
    pool.insertScreenMapping(QStringLiteral("LATTE-TEST-DETACHED-1"));
    const int detachedId = pool.id(QStringLiteral("LATTE-TEST-DETACHED-1"));
    QVERIFY(pool.hasScreenId(detachedId));
    QVERIFY(!pool.isScreenActive(detachedId));

    QVERIFY(!pool.isScreenActive(Latte::ScreenPool::NOSCREENID));
    QVERIFY(!pool.isScreenActive(Latte::ScreenPool::FIRSTSCREENID + 50));
    // The offscreen platform gives its only screen an empty name, which
    // insertScreenMapping() refuses, so the attached branch is not exercised here.
}

void CoreComponentsTest::backgroundCacheHandlesBroadcastedSmallImagesAtEveryEdge()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    qputenv("XDG_CONFIG_HOME", dir.path().toUtf8());
    qputenv("XDG_DATA_HOME", dir.path().toUtf8());

    const QString imagePath = dir.path() + QStringLiteral("/small-wallpaper.png");
    QImage image(QSize(8, 8), QImage::Format_ARGB32);
    image.fill(QColor(240, 240, 240));
    QVERIFY(image.save(imagePath));

    auto *cache = Latte::PlasmaExtended::BackgroundCache::self();
    QSignalSpy changedSpy(cache, &Latte::PlasmaExtended::BackgroundCache::backgroundChanged);
    cache->setBackgroundFromBroadcast(QStringLiteral("activity-a"), QStringLiteral("screen-a"), imagePath);

    QCOMPARE(cache->background(QStringLiteral("activity-a"), QStringLiteral("screen-a")), imagePath);
    QVERIFY(changedSpy.count() >= 1);

    const QList<Plasma::Types::Location> edges{
        Plasma::Types::TopEdge,
        Plasma::Types::BottomEdge,
        Plasma::Types::LeftEdge,
        Plasma::Types::RightEdge
    };

    for (const auto edge : edges) {
        const float brightness = cache->brightnessFor(QStringLiteral("activity-a"), QStringLiteral("screen-a"), edge);
        QVERIFY2(brightness >= 0.0f && brightness <= 255.0f, qPrintable(QString::number(brightness)));
        QCOMPARE(cache->busyFor(QStringLiteral("activity-a"), QStringLiteral("screen-a"), edge), false);
    }
}

void CoreComponentsTest::backgroundTrackerEmitsOnlyForChangedHints()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    qputenv("XDG_CONFIG_HOME", dir.path().toUtf8());
    qputenv("XDG_DATA_HOME", dir.path().toUtf8());

    const QString imagePath = dir.path() + QStringLiteral("/stable-wallpaper.png");
    QImage image(QSize(8, 8), QImage::Format_ARGB32);
    image.fill(QColor(24, 24, 24));
    QVERIFY(image.save(imagePath));

    auto *cache = Latte::PlasmaExtended::BackgroundCache::self();
    cache->setBackgroundFromBroadcast(QStringLiteral("activity-tracker"), QStringLiteral("screen-tracker"), imagePath);

    Latte::BackgroundTracker tracker;
    QSignalSpy brightnessSpy(&tracker, &Latte::BackgroundTracker::currentBrightnessChanged);
    QSignalSpy busySpy(&tracker, &Latte::BackgroundTracker::isBusyChanged);

    tracker.setActivity(QStringLiteral("activity-tracker"));
    tracker.setScreenName(QStringLiteral("screen-tracker"));

    QCOMPARE(brightnessSpy.count(), 1);
    QCOMPARE(busySpy.count(), 0);
    QVERIFY(tracker.currentBrightness() >= 0.0f);
    QCOMPARE(tracker.isBusy(), false);

    cache->setBackgroundFromBroadcast(QStringLiteral("activity-tracker"), QStringLiteral("screen-tracker"), imagePath);

    QCOMPARE(brightnessSpy.count(), 1);
    QCOMPARE(busySpy.count(), 0);
}

QTEST_MAIN(CoreComponentsTest)

#include "corecomponentstest.moc"
