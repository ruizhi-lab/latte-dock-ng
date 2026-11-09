/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "layoutstableview.h"
#include "viewstableview.h"

#include <QDragEnterEvent>
#include <QLabel>
#include <QMimeData>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QTest>
#include <QtTest/qtestaccessible.h>

class SettingsViewUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void layoutsTableShowsDropOverlayForSupportedMimeTypes();
    void viewsTableClearsSelectionWhenClickingEmptyArea();
    void viewsTableEmitsSelectionAccessibilityEvents();
};


void SettingsViewUnitTest::layoutsTableShowsDropOverlayForSupportedMimeTypes()
{
    Latte::Settings::View::LayoutsTableView table;
    table.resize(300, 160);

    auto *overlay = table.findChild<QLabel *>();
    QVERIFY(overlay);
    QVERIFY(!overlay->isVisible());

    QMimeData urlMime;
    urlMime.setUrls({QUrl::fromLocalFile(QStringLiteral("/tmp/sample.layout.latte"))});
    QDragEnterEvent urlEvent(QPoint(20, 20), Qt::CopyAction, &urlMime, Qt::LeftButton, Qt::NoModifier);
    table.dragEntered(&urlEvent);
    QVERIFY(overlay->isVisibleTo(&table));
    QVERIFY(overlay->text().contains(QStringLiteral("layout files")));

    table.dragLeft();
    QVERIFY(!overlay->isVisible());

    QMimeData textMime;
    textMime.setText(QStringLiteral("[LayoutSettings]"));
    QDragEnterEvent textEvent(QPoint(20, 20), Qt::CopyAction, &textMime, Qt::LeftButton, Qt::NoModifier);
    table.dragEntered(&textEvent);
    QVERIFY(overlay->isVisibleTo(&table));
    QVERIFY(overlay->text().contains(QStringLiteral("raw layout text")));
}

void SettingsViewUnitTest::viewsTableClearsSelectionWhenClickingEmptyArea()
{
    Latte::Settings::View::ViewsTableView table;
    QStandardItemModel model(1, 1);
    table.setModel(&model);
    table.resize(240, 120);
    table.selectRow(0);
    QVERIFY(table.selectionModel()->hasSelection());

    QSignalSpy selectionSpy(&table, &Latte::Settings::View::ViewsTableView::selectionsChanged);
    const QPointF clickPoint(table.width() - 2, table.height() - 2);
    QMouseEvent press(QEvent::MouseButtonPress, clickPoint, clickPoint, clickPoint,
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(table.viewport(), &press);

    QVERIFY(!table.selectionModel()->hasSelection());
    QVERIFY(selectionSpy.count() >= 1);
}

void SettingsViewUnitTest::viewsTableEmitsSelectionAccessibilityEvents()
{
#if QT_CONFIG(accessibility)
    // QTableView::selectionChanged() sends the SelectionAdd/SelectionRemove events,
    // and only while accessibility is active. ViewsTableView must reach that override
    // instead of calling QAbstractItemView's directly, which sends none of them.
    QTestAccessibility::initialize();
    QAccessible::setActive(true);

    Latte::Settings::View::ViewsTableView table;
    QStandardItemModel model(3, 1);
    table.setModel(&model);
    table.resize(240, 160);
    table.show();
    QVERIFY(QTest::qWaitForWindowExposed(&table));

    QTestAccessibility::clearEvents();
    table.selectRow(1);
    QVERIFY(QTestAccessibility::containsEventOfType(QAccessible::SelectionAdd));

    QTestAccessibility::clearEvents();
    table.selectRow(2);
    QVERIFY(QTestAccessibility::containsEventOfType(QAccessible::SelectionRemove));
    QVERIFY(QTestAccessibility::containsEventOfType(QAccessible::SelectionAdd));

    QAccessible::setActive(false);
#else
    QSKIP("Qt was built without accessibility support");
#endif
}

QTEST_MAIN(SettingsViewUnitTest)

#include "settingsviewunittest.moc"
