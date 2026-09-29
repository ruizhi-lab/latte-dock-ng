/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "../app/view/blockhidingevents.h"

#include <QTest>

using Latte::ViewPart::BlockHidingEvents;

class BlockHidingEventsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void startsUnblocked();
    void firstEventChangesBlockedState();
    void duplicateEventIsIgnored();
    void emptyEventIsIgnored();
    void unknownRemovalIsIgnored();
    void overlappingEventsRemainBlockedUntilLastRemoval();
};

void BlockHidingEventsTest::startsUnblocked()
{
    const BlockHidingEvents events;

    QVERIFY(!events.isBlocked());
    QVERIFY(!events.hasEvent(QStringLiteral("menu")));
    QCOMPARE(events.count(), qsizetype(0));
}

void BlockHidingEventsTest::firstEventChangesBlockedState()
{
    BlockHidingEvents events;

    QVERIFY(events.addEvent(QStringLiteral("menu")));
    QVERIFY(events.isBlocked());
    QVERIFY(events.hasEvent(QStringLiteral("menu")));
    QCOMPARE(events.count(), qsizetype(1));
}

void BlockHidingEventsTest::duplicateEventIsIgnored()
{
    BlockHidingEvents events;

    QVERIFY(events.addEvent(QStringLiteral("menu")));
    QVERIFY(!events.addEvent(QStringLiteral("menu")));
    QVERIFY(events.isBlocked());
    QCOMPARE(events.count(), qsizetype(1));
}

void BlockHidingEventsTest::emptyEventIsIgnored()
{
    BlockHidingEvents events;

    QVERIFY(!events.addEvent(QString{}));
    QVERIFY(!events.removeEvent(QString{}));
    QVERIFY(!events.hasEvent(QString{}));
    QVERIFY(!events.isBlocked());
}

void BlockHidingEventsTest::unknownRemovalIsIgnored()
{
    BlockHidingEvents events;
    QVERIFY(events.addEvent(QStringLiteral("menu")));

    QVERIFY(!events.removeEvent(QStringLiteral("drag")));
    QVERIFY(events.isBlocked());
    QVERIFY(events.hasEvent(QStringLiteral("menu")));
    QCOMPARE(events.count(), qsizetype(1));
}

void BlockHidingEventsTest::overlappingEventsRemainBlockedUntilLastRemoval()
{
    BlockHidingEvents events;

    QVERIFY(events.addEvent(QStringLiteral("menu")));
    QVERIFY(!events.addEvent(QStringLiteral("drag")));
    QVERIFY(events.isBlocked());

    QVERIFY(!events.removeEvent(QStringLiteral("menu")));
    QVERIFY(events.isBlocked());
    QVERIFY(events.hasEvent(QStringLiteral("drag")));

    QVERIFY(events.removeEvent(QStringLiteral("drag")));
    QVERIFY(!events.isBlocked());
    QCOMPARE(events.count(), qsizetype(0));
}

QTEST_APPLESS_MAIN(BlockHidingEventsTest)

#include "blockhidingeventstest.moc"
