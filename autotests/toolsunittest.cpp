/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "actionlistwidgetitem.h"
#include "generictools.h"
#include "genericviewtools.h"
#include "layoutscombobox.h"
#include "schemescombobox.h"

#include <QImage>
#include <QStyleOptionViewItem>
#include <QTest>

class ToolsUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void actionListWidgetItemStoresIdAndSortOrder();
    void customComboBoxesStoreDecorationState();
    void styleStatePredicatesReflectOptionState();
    void colorGroupFollowsEnabledActiveAndSelectedState();
    void horizontalAlignmentPrefersCenterThenRightThenLeft();
    void remainedRectHelpersReserveExpectedSpace();
    void subtractedKeepsOriginalOrder();
    void screenMaxLengthUsesOddAspectLength();
    void screenDrawingReturnsAvailableInnerRect();
    void viewDrawingPaintsHorizontalAndVerticalEdges();
};

namespace {

bool hasPaintedPixel(const QImage &image)
{
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y)) > 0) {
                return true;
            }
        }
    }

    return false;
}

}

void ToolsUnitTest::actionListWidgetItemStoresIdAndSortOrder()
{
    using Latte::Settings::ActionsDialog::ActionListWidgetItem;

    ActionListWidgetItem first(QIcon(), QStringLiteral("First"), 20, QStringLiteral("first"));
    ActionListWidgetItem second(QIcon(), QStringLiteral("Second"), 10, QStringLiteral("second"));

    QCOMPARE(first.data(ActionListWidgetItem::IDROLE).toString(), QStringLiteral("first"));
    QCOMPARE(first.data(ActionListWidgetItem::ORDERROLE).toInt(), 20);
    QVERIFY(second < first);
    QVERIFY(!(first < second));
}

void ToolsUnitTest::customComboBoxesStoreDecorationState()
{
    Latte::Settings::LayoutsComboBox layoutsCombo;
    Latte::Data::LayoutIcon icon;
    icon.name = QStringLiteral("layout-icon");
    icon.isBackgroundFile = true;

    layoutsCombo.setLayoutIcon(icon);
    QCOMPARE(layoutsCombo.layoutIcon().name, QStringLiteral("layout-icon"));
    QCOMPARE(layoutsCombo.layoutIcon().isBackgroundFile, true);

    Latte::Settings::SchemesComboBox schemesCombo;
    schemesCombo.setBackgroundColor(Qt::black);
    schemesCombo.setTextColor(Qt::white);
    QCOMPARE(schemesCombo.backgroundColor(), QColor(Qt::black));
    QCOMPARE(schemesCombo.textColor(), QColor(Qt::white));
}

void ToolsUnitTest::styleStatePredicatesReflectOptionState()
{
    QStyleOption option;
    option.state = QStyle::State_Enabled | QStyle::State_Active | QStyle::State_Selected
            | QStyle::State_HasFocus | QStyle::State_MouseOver;

    QVERIFY(Latte::isEnabled(option));
    QVERIFY(Latte::isActive(option));
    QVERIFY(Latte::isSelected(option));
    QVERIFY(Latte::isFocused(option));
    QVERIFY(Latte::isHovered(option));

    option.state = QStyle::State_None;
    QVERIFY(!Latte::isEnabled(option));
    QVERIFY(!Latte::isActive(option));
    QVERIFY(!Latte::isSelected(option));
    QVERIFY(!Latte::isFocused(option));
    QVERIFY(!Latte::isHovered(option));

    QStyleOptionViewItem viewOption;
    viewOption.displayAlignment = Qt::AlignHCenter | Qt::AlignVCenter;
    QVERIFY(Latte::isTextCentered(viewOption));
    viewOption.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    QVERIFY(!Latte::isTextCentered(viewOption));
}

void ToolsUnitTest::colorGroupFollowsEnabledActiveAndSelectedState()
{
    QStyleOption option;

    option.state = QStyle::State_None;
    QCOMPARE(Latte::colorGroup(option), QPalette::Disabled);

    option.state = QStyle::State_Enabled | QStyle::State_Active;
    QCOMPARE(Latte::colorGroup(option), QPalette::Active);

    option.state = QStyle::State_Enabled | QStyle::State_HasFocus;
    QCOMPARE(Latte::colorGroup(option), QPalette::Active);

    option.state = QStyle::State_Enabled | QStyle::State_Selected;
    QCOMPARE(Latte::colorGroup(option), QPalette::Inactive);

    option.state = QStyle::State_Enabled;
    QCOMPARE(Latte::colorGroup(option), QPalette::Normal);
}

void ToolsUnitTest::horizontalAlignmentPrefersCenterThenRightThenLeft()
{
    QCOMPARE(Latte::horizontalAlignment(Qt::AlignHCenter | Qt::AlignRight), Qt::AlignHCenter);
    QCOMPARE(Latte::horizontalAlignment(Qt::AlignRight | Qt::AlignVCenter), Qt::AlignRight);
    QCOMPARE(Latte::horizontalAlignment(Qt::AlignLeft | Qt::AlignVCenter), Qt::AlignLeft);
}

void ToolsUnitTest::remainedRectHelpersReserveExpectedSpace()
{
    QStyleOption option;
    option.rect = QRect(0, 0, 100, 20);

    QCOMPARE(Latte::remainedFromIcon(option, Qt::AlignLeft, 2, 1), QRect(22, 0, 78, 20));
    QCOMPARE(Latte::remainedFromIcon(option, Qt::AlignRight, 2, 1), QRect(0, 0, 78, 20));
    QCOMPARE(Latte::remainedFromLayoutIcon(option, Qt::AlignHCenter, 2, 1), option.rect);
    QCOMPARE(Latte::remainedFromColorSchemeIcon(option, Qt::AlignLeft, 2, 1), QRect(22, 0, 78, 20));

    QStyleOptionButton buttonOption;
    buttonOption.rect = option.rect;
    const QRect checkBoxRemained = Latte::remainedFromCheckBox(buttonOption, Qt::AlignLeft);
    QVERIFY(checkBoxRemained.x() > option.rect.x());
    QVERIFY(checkBoxRemained.width() < option.rect.width());

    QCOMPARE(Latte::remainedFromFormattedText(option, QStringLiteral("Latte"), Qt::AlignHCenter), option.rect);
    const QRect textRemained = Latte::remainedFromFormattedText(option, QStringLiteral("Latte"), Qt::AlignLeft);
    QVERIFY(textRemained.x() > option.rect.x());
    QVERIFY(textRemained.width() < option.rect.width());
}

void ToolsUnitTest::subtractedKeepsOriginalOrder()
{
    const QStringList original{QStringLiteral("one"), QStringLiteral("two"), QStringLiteral("three"), QStringLiteral("two")};
    const QStringList current{QStringLiteral("two")};

    QCOMPARE(Latte::subtracted(original, current), QStringList({QStringLiteral("one"), QStringLiteral("three")}));
}

void ToolsUnitTest::screenMaxLengthUsesOddAspectLength()
{
    QStyleOption option;
    option.rect = QRect(0, 0, 200, 20);

    QCOMPARE(Latte::screenMaxLength(option), 33);
    QCOMPARE(Latte::screenMaxLength(option, 10), 17);
}

void ToolsUnitTest::screenDrawingReturnsAvailableInnerRect()
{
    QImage image(QSize(160, 80), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);

    QStyleOption option;
    option.rect = QRect(0, 0, 160, 40);
    option.state = QStyle::State_Enabled;

    const QRect available = Latte::drawScreen(&painter, option, false, QRect(0, 0, 1920, 1080), 32);

    QVERIFY(available.isValid());
    QVERIFY(option.rect.contains(available.topLeft()));
    QVERIFY(option.rect.contains(available.bottomRight()));
    QVERIFY(available.width() > 0);
    QVERIFY(available.height() > 0);
}

void ToolsUnitTest::viewDrawingPaintsHorizontalAndVerticalEdges()
{
    QStyleOption option;
    option.state = QStyle::State_Enabled;
    option.palette = QPalette();

    Latte::Data::View horizontalView;
    horizontalView.edge = Plasma::Types::BottomEdge;
    horizontalView.alignment = Latte::Types::Center;

    QImage horizontalImage(QSize(120, 80), QImage::Format_ARGB32_Premultiplied);
    horizontalImage.fill(Qt::transparent);
    QPainter horizontalPainter(&horizontalImage);
    Latte::drawView(&horizontalPainter, option, horizontalView, QRect(10, 10, 100, 50));
    horizontalPainter.end();
    QVERIFY(hasPaintedPixel(horizontalImage));

    Latte::Data::View verticalView;
    verticalView.edge = Plasma::Types::LeftEdge;
    verticalView.alignment = Latte::Types::Center;

    QImage verticalImage(QSize(120, 80), QImage::Format_ARGB32_Premultiplied);
    verticalImage.fill(Qt::transparent);
    QPainter verticalPainter(&verticalImage);
    Latte::drawView(&verticalPainter, option, verticalView, QRect(10, 10, 100, 50));
    verticalPainter.end();
    QVERIFY(hasPaintedPixel(verticalImage));
}

QTEST_MAIN(ToolsUnitTest)

#include "toolsunittest.moc"
