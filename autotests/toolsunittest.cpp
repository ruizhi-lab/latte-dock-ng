/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "generictools.h"
#include "genericviewtools.h"
#include "view/viewgeometryhelpers.h"

#include <QImage>
#include <QStyleOptionViewItem>
#include <QTest>

class ToolsUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void styleStatePredicatesReflectOptionState();
    void colorGroupFollowsEnabledActiveAndSelectedState();
    void horizontalAlignmentPrefersCenterThenRightThenLeft();
    void remainedRectHelpersReserveExpectedSpace();
    void subtractedKeepsOriginalOrder();
    void screenMaxLengthUsesOddAspectLength();
    void screenDrawingReturnsAvailableInnerRect();
    void viewDrawingPaintsHorizontalAndVerticalEdges();
    void horizontalDockScreenEdgeCornerPolicy();
    void verticalDockExternalPanelAvoidancePolicy();
    void verticalDockExternalPanelGeometryKeepsScreenThicknessAxis();
    void dockFormFactorFollowsEdgeLocation();
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

void ToolsUnitTest::horizontalDockScreenEdgeCornerPolicy()
{
    using Latte::ViewPart::horizontalDockTouchesLeftLengthEdge;
    using Latte::ViewPart::horizontalDockTouchesRightLengthEdge;

    QVERIFY(horizontalDockTouchesLeftLengthEdge(Latte::Types::Left, 0.5f, 0.0f));
    QVERIFY(!horizontalDockTouchesRightLengthEdge(Latte::Types::Left, 0.5f, 0.0f));
    QVERIFY(!horizontalDockTouchesLeftLengthEdge(Latte::Types::Right, 0.5f, 0.0f));
    QVERIFY(horizontalDockTouchesRightLengthEdge(Latte::Types::Right, 0.5f, 0.0f));

    QVERIFY(horizontalDockTouchesLeftLengthEdge(Latte::Types::Justify, 1.0f, 0.0f));
    QVERIFY(horizontalDockTouchesRightLengthEdge(Latte::Types::Justify, 1.0f, 0.0f));

    QVERIFY(!horizontalDockTouchesLeftLengthEdge(Latte::Types::Center, 0.5f, 0.0f));
    QVERIFY(!horizontalDockTouchesRightLengthEdge(Latte::Types::Center, 0.5f, 0.0f));
    QVERIFY(!horizontalDockTouchesLeftLengthEdge(Latte::Types::Left, 0.5f, 0.1f));
    QVERIFY(!horizontalDockTouchesRightLengthEdge(Latte::Types::Right, 0.5f, 0.1f));
    QVERIFY(!horizontalDockTouchesLeftLengthEdge(Latte::Types::Justify, 0.9f, 0.0f));
    QVERIFY(!horizontalDockTouchesRightLengthEdge(Latte::Types::Justify, 0.9f, 0.0f));
}

void ToolsUnitTest::verticalDockExternalPanelAvoidancePolicy()
{
    using Latte::ViewPart::shouldRespectExternalPanelsForVerticalDock;
    using Latte::ViewPart::verticalDockTouchesBottomLengthEdge;
    using Latte::ViewPart::verticalDockTouchesTopLengthEdge;

    QVERIFY(shouldRespectExternalPanelsForVerticalDock(Latte::Types::Top, 0.5f, 0.0f));
    QVERIFY(shouldRespectExternalPanelsForVerticalDock(Latte::Types::Bottom, 0.5f, 0.0f));
    QVERIFY(shouldRespectExternalPanelsForVerticalDock(Latte::Types::Justify, 1.0f, 0.0f));

    QVERIFY(!shouldRespectExternalPanelsForVerticalDock(Latte::Types::Center, 0.5f, 0.0f));
    QVERIFY(!shouldRespectExternalPanelsForVerticalDock(Latte::Types::Top, 0.5f, 0.1f));
    QVERIFY(!shouldRespectExternalPanelsForVerticalDock(Latte::Types::Justify, 0.9f, 0.0f));

    QVERIFY(verticalDockTouchesTopLengthEdge(Latte::Types::Top, 0.5f, 0.0f));
    QVERIFY(!verticalDockTouchesBottomLengthEdge(Latte::Types::Top, 0.5f, 0.0f));
    QVERIFY(!verticalDockTouchesTopLengthEdge(Latte::Types::Bottom, 0.5f, 0.0f));
    QVERIFY(verticalDockTouchesBottomLengthEdge(Latte::Types::Bottom, 0.5f, 0.0f));
    QVERIFY(verticalDockTouchesTopLengthEdge(Latte::Types::Justify, 1.0f, 0.0f));
    QVERIFY(verticalDockTouchesBottomLengthEdge(Latte::Types::Justify, 1.0f, 0.0f));
}

void ToolsUnitTest::verticalDockExternalPanelGeometryKeepsScreenThicknessAxis()
{
    const QRect screenGeometry(0, 0, 1000, 800);

    QCOMPARE(Latte::ViewPart::verticalDockExternalPanelGeometry(screenGeometry, QRect(90, 30, 910, 740)),
             QRect(0, 30, 1000, 740));
    QCOMPARE(Latte::ViewPart::verticalDockExternalPanelGeometry(screenGeometry, QRect(90, 0, 910, 800)),
             screenGeometry);

    const QList<QRect> panels{QRect(0, 0, 1000, 40), QRect(0, 0, 90, 800), QRect(0, 760, 1000, 40)};
    QCOMPARE(Latte::ViewPart::verticalDockExternalPanelGeometry(screenGeometry, panels),
             QRect(0, 40, 1000, 720));

    const QRect topPanelGeometry = Latte::ViewPart::screenEdgePanelGeometry(screenGeometry, Plasma::Types::TopEdge, 36);
    QCOMPARE(topPanelGeometry, QRect(0, 0, 1000, 36));
    QCOMPARE(Latte::ViewPart::verticalDockExternalPanelGeometry(screenGeometry, QList<QRect>{topPanelGeometry}),
             QRect(0, 36, 1000, 764));
}

void ToolsUnitTest::dockFormFactorFollowsEdgeLocation()
{
    using Latte::ViewPart::dockFormFactorForLocation;

    QCOMPARE(dockFormFactorForLocation(Plasma::Types::LeftEdge, Plasma::Types::Horizontal),
             Plasma::Types::Vertical);
    QCOMPARE(dockFormFactorForLocation(Plasma::Types::RightEdge, Plasma::Types::Horizontal),
             Plasma::Types::Vertical);

    QCOMPARE(dockFormFactorForLocation(Plasma::Types::TopEdge, Plasma::Types::Vertical),
             Plasma::Types::Horizontal);
    QCOMPARE(dockFormFactorForLocation(Plasma::Types::BottomEdge, Plasma::Types::Vertical),
             Plasma::Types::Horizontal);

    QCOMPARE(dockFormFactorForLocation(Plasma::Types::Desktop, Plasma::Types::Vertical),
             Plasma::Types::Vertical);
}

QTEST_MAIN(ToolsUnitTest)

#include "toolsunittest.moc"
