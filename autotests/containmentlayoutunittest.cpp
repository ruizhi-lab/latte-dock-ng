/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "layoutmanager.h"

#include <QQuickItem>
#include <QQmlPropertyMap>
#include <QSignalSpy>
#include <QTest>

class TestConfig : public QQmlPropertyMap
{
  public:
    TestConfig()
      : QQmlPropertyMap(this, nullptr)
    {
    }
};

// Exercise the real restore/save path without a Plasma shell or user config.
class RestoreRoot : public QQuickItem
{
    Q_OBJECT
  public:
    Q_INVOKABLE QVariant createAppletItem(QVariant applet)
    {
        auto *item = new QQuickItem(this);
        item->setProperty("backendAppletRef", applet);
        return QVariant::fromValue(item);
    }
};

class ContainmentLayoutUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void tracksRequestedAppletOptionLists();
    void independentCopyWaitsForLateApplet_data();
    void independentCopyWaitsForLateApplet();
    void tracksScheduledDestructionIds();
    void restoresScheduledDestructionAppletAtOriginalIndex();
    void roundTripsMasqueradedIndexes();
    void scheduledDestructionHidesAndShowsAppletItem();
    void scheduledDestructionToleratesMissingLayouts();
};

void
ContainmentLayoutUnitTest::independentCopyWaitsForLateApplet_data()
{
    QTest::addColumn<bool>("originalColors");
    QTest::newRow("copied-trash-original-colors") << true;
    QTest::newRow("copied-trash-explicit-colorization") << false;
}

void
ContainmentLayoutUnitTest::independentCopyWaitsForLateApplet()
{
    QFETCH(bool, originalColors);
    const QString savedColors = originalColors ? QStringLiteral("45") : QString();
    TestConfig config;
    config.insert(QStringLiteral("appletOrder"), QStringLiteral("44;46;45"));
    config.insert(QStringLiteral("lockedZoomApplets"), QStringLiteral("45"));
    config.insert(QStringLiteral("userBlocksColorizingApplets"), savedColors);
    config.insert(QStringLiteral("trashKeepOriginalColorsDefaulted"), true);
    QObject plasmoid;
    plasmoid.setProperty("configuration", QVariant::fromValue(static_cast<QObject *>(&config)));
    QQuickItem launcher;
    launcher.setProperty("id", 44);
    launcher.setProperty("_plasma_graphicObject", QVariant::fromValue(static_cast<QObject *>(&launcher)));
    QQuickItem tasks;
    tasks.setProperty("id", 46);
    tasks.setProperty("_plasma_graphicObject", QVariant::fromValue(static_cast<QObject *>(&tasks)));
    QQuickItem trash;
    trash.setProperty("id", 45);
    trash.setProperty("_plasma_graphicObject", QVariant::fromValue(static_cast<QObject *>(&trash)));
    plasmoid.setProperty("applets", QVariant::fromValue(QList<QObject *>{ &launcher, &tasks }));
    RestoreRoot root;
    QQuickItem start;
    QQuickItem main;
    QQuickItem end;
    Latte::Containment::LayoutManager manager;
    manager.setRootItem(&root);
    manager.setStartLayout(&start);
    manager.setMainLayout(&main);
    manager.setEndLayout(&end);
    manager.setPlasmoid(&plasmoid);
    manager.restore();
    QCOMPARE(config.value(QStringLiteral("appletOrder")).toString(), QStringLiteral("44;46;45"));
    QCOMPARE(config.value(QStringLiteral("userBlocksColorizingApplets")).toString(), savedColors);
    QVERIFY(!manager.hasRestoredApplets());

    plasmoid.setProperty("applets", QVariant::fromValue(QList<QObject *>{ &launcher, &tasks, &trash }));
    manager.restore();
    QCOMPARE(manager.appletOrder(), (QList<int>{ 44, 46, 45 }));
    QCOMPARE(manager.lockedZoomApplets(), (QList<int>{ 45 }));
    QCOMPARE(manager.userBlocksColorizingApplets(), originalColors ? QList<int>{ 45 } : QList<int>());
    manager.saveOptions();
    QCOMPARE(config.value(QStringLiteral("lockedZoomApplets")).toString(), QStringLiteral("45"));
    QCOMPARE(config.value(QStringLiteral("userBlocksColorizingApplets")).toString(), savedColors);
}

void ContainmentLayoutUnitTest::tracksRequestedAppletOptionLists()
{
    Latte::Containment::LayoutManager manager;
    QSignalSpy lockedSpy(&manager, &Latte::Containment::LayoutManager::lockedZoomAppletsChanged);
    QSignalSpy coloringSpy(&manager, &Latte::Containment::LayoutManager::userBlocksColorizingAppletsChanged);

    manager.requestAppletsInLockedZoom(QList<int> {3, 5});
    manager.requestAppletsInLockedZoom(QList<int> {3, 5});
    QCOMPARE(manager.lockedZoomApplets(), (QList<int> {3, 5}));
    QCOMPARE(lockedSpy.count(), 1);

    manager.requestAppletsDisabledColoring(QList<int> {7});
    manager.requestAppletsDisabledColoring(QList<int> {7});
    QCOMPARE(manager.userBlocksColorizingApplets(), (QList<int> {7}));
    QCOMPARE(coloringSpy.count(), 1);
}

void ContainmentLayoutUnitTest::tracksScheduledDestructionIds()
{
    Latte::Containment::LayoutManager manager;
    QSignalSpy scheduledSpy(&manager, &Latte::Containment::LayoutManager::appletsInScheduledDestructionChanged);

    manager.setAppletInScheduledDestruction(9, true);
    manager.setAppletInScheduledDestruction(9, true);
    QVERIFY(manager.appletsInScheduledDestruction().contains(9));
    QCOMPARE(scheduledSpy.count(), 1);

    manager.setAppletInScheduledDestruction(9, false);
    manager.setAppletInScheduledDestruction(9, false);
    QVERIFY(!manager.appletsInScheduledDestruction().contains(9));
    QCOMPARE(scheduledSpy.count(), 2);
}

void ContainmentLayoutUnitTest::restoresScheduledDestructionAppletAtOriginalIndex()
{
    Latte::Containment::LayoutManager manager;

    manager.requestAppletsOrder(QList<int> {1, 2, 3, 4});

    QVERIFY(QMetaObject::invokeMethod(&manager,
                                      "rememberAppletRemovalIndex",
                                      Qt::DirectConnection,
                                      Q_ARG(int, 2)));

    manager.requestAppletsOrder(QList<int> {1, 3, 4});

    QVERIFY(QMetaObject::invokeMethod(&manager,
                                      "restoreAppletOrderForApplet",
                                      Qt::DirectConnection,
                                      Q_ARG(int, 2)));

    QCOMPARE(manager.appletOrder(), (QList<int> {1, 2, 3, 4}));
}

void ContainmentLayoutUnitTest::roundTripsMasqueradedIndexes()
{
    Latte::Containment::LayoutManager manager;

    const QPoint encoded = manager.indexToMasquearadedPoint(12);
    QVERIFY(manager.isMasqueradedIndex(encoded.x(), encoded.y()));
    QCOMPARE(manager.masquearadedIndex(encoded.x(), encoded.y()), 12);
    QVERIFY(!manager.isMasqueradedIndex(encoded.x(), encoded.y() + 1));
}

void ContainmentLayoutUnitTest::scheduledDestructionHidesAndShowsAppletItem()
{
    Latte::Containment::LayoutManager manager;

    // Stack-allocated layout and applet item — QQuickItem can be
    // stack-allocated when no QObject parent is needed.
    QQuickItem mainLayout;
    manager.setMainLayout(&mainLayout);

    // Mock backend object: a plain QObject whose "id" property is the
    // target applet id (appletId() reads candidate->property("id")).
    constexpr int testId{42};
    QObject backend;
    backend.setProperty("id", testId);

    // Applet item — the item whose visibility we want to verify.
    // The visual parent (mainLayout) is also the QObject parent, so
    // appletItem appears in mainLayout.childItems().
    QQuickItem appletItem(&mainLayout);
    appletItem.setProperty("applet", QVariant::fromValue(static_cast<QObject *>(&backend)));
    appletItem.setVisible(true);
    QVERIFY(appletItem.isVisible());

    QSignalSpy scheduledSpy(&manager, &Latte::Containment::LayoutManager::appletsInScheduledDestructionChanged);

    // Schedule destruction — the item should be hidden immediately.
    manager.setAppletInScheduledDestruction(testId, true);
    QVERIFY(manager.appletsInScheduledDestruction().contains(testId));
    QVERIFY(!appletItem.isVisible());
    QCOMPARE(scheduledSpy.count(), 1);

    // Redundant enable — no change, no extra signal.
    manager.setAppletInScheduledDestruction(testId, true);
    QVERIFY(manager.appletsInScheduledDestruction().contains(testId));
    QCOMPARE(scheduledSpy.count(), 1);

    // Undo — the item should become visible again.
    manager.setAppletInScheduledDestruction(testId, false);
    QVERIFY(!manager.appletsInScheduledDestruction().contains(testId));
    QVERIFY(appletItem.isVisible());
    QCOMPARE(scheduledSpy.count(), 2);

    // Redundant disable — no change, no extra signal.
    manager.setAppletInScheduledDestruction(testId, false);
    QVERIFY(!manager.appletsInScheduledDestruction().contains(testId));
    QCOMPARE(scheduledSpy.count(), 2);
}

void ContainmentLayoutUnitTest::scheduledDestructionToleratesMissingLayouts()
{
    Latte::Containment::LayoutManager manager;
    QSignalSpy scheduledSpy(&manager, &Latte::Containment::LayoutManager::appletsInScheduledDestructionChanged);

    // No layouts set — appletItem() returns nullptr.
    // The function must not crash and must still track IDs correctly.
    manager.setAppletInScheduledDestruction(7, true);
    QVERIFY(manager.appletsInScheduledDestruction().contains(7));
    QCOMPARE(scheduledSpy.count(), 1);

    manager.setAppletInScheduledDestruction(7, false);
    QVERIFY(!manager.appletsInScheduledDestruction().contains(7));
    QCOMPARE(scheduledSpy.count(), 2);
}



QTEST_MAIN(ContainmentLayoutUnitTest)

#include "containmentlayoutunittest.moc"
