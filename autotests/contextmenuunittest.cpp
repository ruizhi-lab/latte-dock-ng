/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "layoutmenuitemwidget.h"

#include <QAction>
#include <QImage>
#include <QRadioButton>
#include <QTest>

namespace {
// Renders the item at the given size. Two renderings compared by the test use the same size and
// the same state, so their pixels differ only where the glyph weight of the name differs.
QImage renderItem(Latte::LayoutMenuItemWidget &widget, const QSize &size)
{
    widget.resize(size);
    return widget.grab().toImage();
}

// Counts the pixels whose colour differs between two renderings of the same size. The highlight
// background is not uniform, so comparing against one background pixel would count almost everything.
int differentPixelCount(const QImage &first, const QImage &second)
{
    int different = 0;

    for (int y = 0; y < first.height(); ++y) {
        for (int x = 0; x < first.width(); ++x) {
            if (first.pixel(x, y) != second.pixel(x, y)) {
                ++different;
            }
        }
    }

    return different;
}
} // namespace

class ContextMenuUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void layoutMenuItemReflectsCheckableActionState();
    void layoutMenuItemHidesRadioForNonCheckableAction();
    void currentLayoutItemIsDrawnBoldAndWider();
};

void ContextMenuUnitTest::layoutMenuItemReflectsCheckableActionState()
{
    QAction action(QStringLiteral("<b>Current Layout</b>"), this);
    action.setCheckable(true);
    action.setChecked(true);
    action.setVisible(true);

    Latte::LayoutMenuItemWidget widget(&action, nullptr);
    widget.setIcon(false, QStringLiteral("user-identity"));

    auto *radio = widget.findChild<QRadioButton *>();
    QVERIFY(radio);
    QVERIFY(radio->isVisibleTo(&widget));
    QVERIFY(radio->isChecked());
    QVERIFY(widget.sizeHint().width() > widget.fontMetrics().horizontalAdvance(action.text()));
    QVERIFY(widget.minimumSizeHint().isValid());
}

void ContextMenuUnitTest::layoutMenuItemHidesRadioForNonCheckableAction()
{
    QAction action(QStringLiteral("Plain Layout"), this);
    action.setCheckable(false);
    action.setVisible(true);

    Latte::LayoutMenuItemWidget widget(&action, nullptr);

    auto *radio = widget.findChild<QRadioButton *>();
    QVERIFY(radio);
    QVERIFY(!radio->isVisibleTo(&widget));
    QVERIFY(widget.sizeHint().isValid());
}

void ContextMenuUnitTest::currentLayoutItemIsDrawnBoldAndWider()
{
    QAction action(QStringLiteral("Current Layout Name"), this);
    action.setCheckable(true);
    action.setVisible(true);

    Latte::LayoutMenuItemWidget plain(&action, nullptr);
    Latte::LayoutMenuItemWidget current(&action, nullptr);
    current.setCurrent(true);

    QVERIFY(!plain.isCurrent());
    QVERIFY(current.isCurrent());

    // sizeHint() measures the same bold markup that paintEvent() draws, so the wider name is not clipped.
    QVERIFY(current.sizeHint().width() > plain.sizeHint().width());

    // Render both at one size: the glyph weight of the name is the only difference between them.
    const QSize size = current.sizeHint().expandedTo(plain.sizeHint());
    const QImage regular = renderItem(plain, size);
    const QImage bold = renderItem(current, size);
    QCOMPARE(bold.size(), regular.size());

    // Bold glyphs change about a thousand pixels of this name; without the markup nothing changes.
    QVERIFY(differentPixelCount(regular, bold) > 100);

    // Clearing the state returns the item to the regular measurement.
    current.setCurrent(false);
    QVERIFY(!current.isCurrent());
    QCOMPARE(current.sizeHint(), plain.sizeHint());
}

QTEST_MAIN(ContextMenuUnitTest)

#include "contextmenuunittest.moc"
