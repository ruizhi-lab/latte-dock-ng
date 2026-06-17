/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "dialog.h"
#include "iconitem.h"

#include <QSignalSpy>
#include <QTest>

class DeclarativeCoreUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void iconItemEmitsOnlyForChangedState();
    void dialogTracksMouseAndEdgeChanges();
};

void DeclarativeCoreUnitTest::iconItemEmitsOnlyForChangedState()
{
    Latte::IconItem item;

    QVERIFY(item.smooth());
    QVERIFY(!item.isActive());
    QVERIFY(!item.providesColors());
    QVERIFY(!item.usesPlasmaTheme());
    QVERIFY(item.overlays().isEmpty());

    QSignalSpy activeSpy(&item, &Latte::IconItem::activeChanged);
    QSignalSpy overlaysSpy(&item, &Latte::IconItem::overlaysChanged);
    QSignalSpy providesSpy(&item, &Latte::IconItem::providesColorsChanged);
    QSignalSpy plasmaThemeSpy(&item, &Latte::IconItem::usesPlasmaThemeChanged);

    item.setActive(true);
    item.setActive(true);
    QCOMPARE(activeSpy.count(), 1);
    QVERIFY(item.isActive());

    item.setOverlays(QStringList{QStringLiteral("emblem-favorite")});
    item.setOverlays(QStringList{QStringLiteral("emblem-favorite")});
    QCOMPARE(overlaysSpy.count(), 1);

    item.setProvidesColors(true);
    item.setProvidesColors(true);
    QCOMPARE(providesSpy.count(), 1);

    item.setUsesPlasmaTheme(true);
    item.setUsesPlasmaTheme(true);
    QCOMPARE(plasmaThemeSpy.count(), 1);
}

void DeclarativeCoreUnitTest::dialogTracksMouseAndEdgeChanges()
{
    Latte::Quick::Dialog dialog;
    QCOMPARE(dialog.edge(), Plasma::Types::BottomEdge);
    QVERIFY(!dialog.containsMouse());

    QSignalSpy edgeSpy(&dialog, &Latte::Quick::Dialog::edgeChanged);
    QSignalSpy containsSpy(&dialog, &Latte::Quick::Dialog::containsMouseChanged);

    dialog.setEdge(Plasma::Types::TopEdge);
    dialog.setEdge(Plasma::Types::TopEdge);
    QCOMPARE(dialog.edge(), Plasma::Types::TopEdge);
    QCOMPARE(edgeSpy.count(), 1);

    QEnterEvent enterEvent{QPointF(), QPointF(), QPointF()};
    QCoreApplication::sendEvent(&dialog, &enterEvent);
    QVERIFY(dialog.containsMouse());
    QCOMPARE(containsSpy.count(), 1);

    QEvent leaveEvent(QEvent::Leave);
    QCoreApplication::sendEvent(&dialog, &leaveEvent);
    QVERIFY(!dialog.containsMouse());
    QCOMPARE(containsSpy.count(), 2);

    QEvent hideEvent(QEvent::Hide);
    QCoreApplication::sendEvent(&dialog, &hideEvent);
    QCOMPARE(containsSpy.count(), 2);
}

QTEST_MAIN(DeclarativeCoreUnitTest)

#include "declarativecoreunittest.moc"
