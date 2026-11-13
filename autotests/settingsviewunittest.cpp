/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "layoutstableview.h"
#include "viewstableview.h"

#include <QDragEnterEvent>
#include <QLabel>
#include <QMimeData>
#include <QPointF>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QTest>
#include <QtTest/qtestaccessible.h>

#include <algorithm>
#include <type_traits>

namespace {

template<typename Event>
Event
makeDragEnterEvent(const QPointF &position, const QMimeData *mimeData)
{
    // Qt 6.8 exposes the QPoint constructor; newer Qt releases deprecate it in favor of QPointF.
    // Probe the constructor so this test uses the available API without pinning a Qt version.
    if constexpr (std::is_constructible_v<Event, QPointF, Qt::DropActions, const QMimeData *, Qt::MouseButtons, Qt::KeyboardModifiers>) {
        return Event(position, Qt::CopyAction, mimeData, Qt::LeftButton, Qt::NoModifier);
    } else {
        return Event(position.toPoint(), Qt::CopyAction, mimeData, Qt::LeftButton, Qt::NoModifier);
    }
}

#if QT_CONFIG(accessibility)
bool
containsAccessibilityEvent(QAccessible::Event type)
{
    // events() is available across supported Qt versions; filtering retains exact event-type checks.
    const auto events = QTestAccessibility::events();
    return std::any_of(events.cbegin(), events.cend(), [type](const QAccessibleEvent *event) { return event && event->type() == type; });
}
#endif

} // namespace

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
    auto urlEvent = makeDragEnterEvent<QDragEnterEvent>(QPointF(20, 20), &urlMime);
    table.dragEntered(&urlEvent);
    QVERIFY(overlay->isVisibleTo(&table));
    QVERIFY(overlay->text().contains(QStringLiteral("layout files")));

    table.dragLeft();
    QVERIFY(!overlay->isVisible());

    QMimeData textMime;
    textMime.setText(QStringLiteral("[LayoutSettings]"));
    auto textEvent = makeDragEnterEvent<QDragEnterEvent>(QPointF(20, 20), &textMime);
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
    // Mapping the window can change platform accessibility state, so activate it at the point
    // where selection events are generated and verify that the platform accepted the request.
    QAccessible::setActive(true);
    QVERIFY(QAccessible::isActive());

    QTestAccessibility::clearEvents();
    table.selectRow(1);
    QVERIFY(containsAccessibilityEvent(QAccessible::SelectionAdd));

    QTestAccessibility::clearEvents();
    table.selectRow(2);
    QVERIFY(containsAccessibilityEvent(QAccessible::SelectionRemove));
    QVERIFY(containsAccessibilityEvent(QAccessible::SelectionAdd));

    QAccessible::setActive(false);
#else
    QSKIP("Qt was built without accessibility support");
#endif
}

QTEST_MAIN(SettingsViewUnitTest)

#include "settingsviewunittest.moc"
