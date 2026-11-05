/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "menu.h"

#include <QAction>
#include <QTest>

class ContainmentActionMenuUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void menuCanBeDestroyedBeforeRestore();
    void actionLookupIsEmptyBeforeRestore();
    void onlyListedCurrentLayoutsAreMarked();
    void emptyCurrentListMarksNothing();
};

void ContainmentActionMenuUnitTest::menuCanBeDestroyedBeforeRestore()
{
    auto *menu = new Menu(nullptr, QVariantList());
    delete menu;
}

void ContainmentActionMenuUnitTest::actionLookupIsEmptyBeforeRestore()
{
    Menu menu(nullptr, QVariantList());
    QVERIFY(menu.action(QStringLiteral("missing")) == nullptr);
}

void ContainmentActionMenuUnitTest::onlyListedCurrentLayoutsAreMarked()
{
    const QStringList current = QStringLiteral("Alpha;;Beta Gamma").split(QStringLiteral(";;"));

    QVERIFY(isCurrentLayoutName(current, QStringLiteral("Alpha")));
    QVERIFY(isCurrentLayoutName(current, QStringLiteral("Beta Gamma")));
    QVERIFY(!isCurrentLayoutName(current, QStringLiteral("Delta")));

    // Names match exactly: a prefix or another letter case is a different layout.
    QVERIFY(!isCurrentLayoutName(current, QStringLiteral("Beta")));
    QVERIFY(!isCurrentLayoutName(current, QStringLiteral("alpha")));
}

void ContainmentActionMenuUnitTest::emptyCurrentListMarksNothing()
{
    // An empty CURRENTLAYOUTSINDEX splits into one empty entry, and an incomplete DBus reply
    // can produce an empty layout name: neither may be marked.
    const QStringList none = QString().split(QStringLiteral(";;"));

    QVERIFY(!isCurrentLayoutName(none, QString()));
    QVERIFY(!isCurrentLayoutName(none, QStringLiteral("Alpha")));

    const QStringList withEmptyEntry = QStringLiteral("Alpha;;;;Beta").split(QStringLiteral(";;"));
    QVERIFY(!isCurrentLayoutName(withEmptyEntry, QString()));
}

QTEST_MAIN(ContainmentActionMenuUnitTest)

#include "containmentactionmenuunittest.moc"
