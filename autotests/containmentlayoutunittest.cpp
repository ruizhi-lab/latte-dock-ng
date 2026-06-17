/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "layoutmanager.h"

#include <QSignalSpy>
#include <QTest>

class ContainmentLayoutUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void tracksRequestedAppletOptionLists();
    void tracksScheduledDestructionIds();
    void restoresScheduledDestructionAppletAtOriginalIndex();
    void roundTripsMasqueradedIndexes();
};

void ContainmentLayoutUnitTest::tracksRequestedAppletOptionLists()
{
    Latte::Containment::LayoutManager manager;
    QSignalSpy lockedSpy(&manager, &Latte::Containment::LayoutManager::lockedZoomAppletsChanged);
    QSignalSpy coloringSpy(&manager, &Latte::Containment::LayoutManager::userBlocksColorizingAppletsChanged);

    manager.requestAppletsInLockedZoom(QList<int>{3, 5});
    manager.requestAppletsInLockedZoom(QList<int>{3, 5});
    QCOMPARE(manager.lockedZoomApplets(), (QList<int>{3, 5}));
    QCOMPARE(lockedSpy.count(), 1);

    manager.requestAppletsDisabledColoring(QList<int>{7});
    manager.requestAppletsDisabledColoring(QList<int>{7});
    QCOMPARE(manager.userBlocksColorizingApplets(), (QList<int>{7}));
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

    manager.requestAppletsOrder(QList<int>{1, 2, 3, 4});

    QVERIFY(QMetaObject::invokeMethod(&manager,
                                      "rememberAppletRemovalIndex",
                                      Qt::DirectConnection,
                                      Q_ARG(int, 2)));

    manager.requestAppletsOrder(QList<int>{1, 3, 4});

    QVERIFY(QMetaObject::invokeMethod(&manager,
                                      "restoreAppletOrderForApplet",
                                      Qt::DirectConnection,
                                      Q_ARG(int, 2)));

    QCOMPARE(manager.appletOrder(), (QList<int>{1, 2, 3, 4}));
}

void ContainmentLayoutUnitTest::roundTripsMasqueradedIndexes()
{
    Latte::Containment::LayoutManager manager;

    const QPoint encoded = manager.indexToMasquearadedPoint(12);
    QVERIFY(manager.isMasqueradedIndex(encoded.x(), encoded.y()));
    QCOMPARE(manager.masquearadedIndex(encoded.x(), encoded.y()), 12);
    QVERIFY(!manager.isMasqueradedIndex(encoded.x(), encoded.y() + 1));
}



QTEST_MAIN(ContainmentLayoutUnitTest)

#include "containmentlayoutunittest.moc"
