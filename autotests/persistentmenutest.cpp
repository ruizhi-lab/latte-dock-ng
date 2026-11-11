/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

// PersistentMenu is the popup of an activity cell in the Layouts editor. A click on a checkable
// item must keep the popup open, so that the user can tick more items and then press OK. A hide
// that does not come from that release (OK, Cancel, close()) must still close the popup.

#include "persistentmenu.h"

#include <QAction>
#include <QSignalSpy>
#include <QTest>

using Latte::Settings::Layout::Delegate::PersistentMenu;

class PersistentMenuTest : public QObject
{
    Q_OBJECT

  private Q_SLOTS:
    void checkableItemClickKeepsMenuOpen();
    void explicitHideCloses();
    void closeCloses();
};

void
PersistentMenuTest::checkableItemClickKeepsMenuOpen()
{
    PersistentMenu menu;
    QAction *work = menu.addAction(QStringLiteral("Work"));
    work->setCheckable(true);
    QAction *other = menu.addAction(QStringLiteral("Other"));
    other->setCheckable(true);

    menu.popup(QPoint(40, 40));
    QVERIFY(QTest::qWaitForWindowExposed(&menu));

    const QPoint item = menu.actionGeometry(work).center();
    QSignalSpy toggles(work, &QAction::toggled);

    // Pointer press and release on the item, as a user does it.
    QTest::mouseMove(&menu, item);
    QTest::mousePress(&menu, Qt::LeftButton, Qt::NoModifier, item);
    QTest::mouseRelease(&menu, Qt::LeftButton, Qt::NoModifier, item);
    QTest::qWait(50);

    QCOMPARE(toggles.count(), 1);
    QVERIFY(work->isChecked());
    QVERIFY(!other->isChecked());
    QVERIFY(menu.isVisible());

    menu.hide();
    QVERIFY(!menu.isVisible());
}

void
PersistentMenuTest::explicitHideCloses()
{
    PersistentMenu menu;
    QAction *work = menu.addAction(QStringLiteral("Work"));
    work->setCheckable(true);

    menu.popup(QPoint(40, 40));
    QVERIFY(QTest::qWaitForWindowExposed(&menu));

    menu.hide();
    QVERIFY(!menu.isVisible());
}

void
PersistentMenuTest::closeCloses()
{
    PersistentMenu menu;
    QAction *work = menu.addAction(QStringLiteral("Work"));
    work->setCheckable(true);

    menu.popup(QPoint(40, 40));
    QVERIFY(QTest::qWaitForWindowExposed(&menu));

    QVERIFY(menu.close());
    QVERIFY(!menu.isVisible());
}

QTEST_MAIN(PersistentMenuTest)

#include "persistentmenutest.moc"
