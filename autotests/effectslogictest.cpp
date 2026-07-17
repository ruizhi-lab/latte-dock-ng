/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "view/viewgeometryhelpers.h"

#include <QRegion>
#include <QTest>
#include <Plasma/Plasma>

using namespace Latte::ViewPart;

// FrameSvg::EnabledBorders bitmask constants (matches Plasma::FrameSvg)
constexpr int B_TOP    = 1;
constexpr int B_BOTTOM = 2;
constexpr int B_LEFT   = 4;
constexpr int B_RIGHT  = 8;
constexpr int B_ALL    = 15;

namespace EffectsLogic {

int computeEnabledBorders(
    Plasma::Types::Location location,
    bool screenEdgeMarginEnabled,
    Latte::Types::Alignment alignment,
    float maxLength,
    float offset,
    bool forceTopBorder,
    bool forceBottomBorder,
    bool backgroundAllCorners)
{
    int borders = B_ALL;

    if (!screenEdgeMarginEnabled) {
        switch (location) {
        case Plasma::Types::TopEdge:    borders &= ~B_TOP; break;
        case Plasma::Types::LeftEdge:   borders &= ~B_LEFT; break;
        case Plasma::Types::RightEdge:  borders &= ~B_RIGHT; break;
        case Plasma::Types::BottomEdge: borders &= ~B_BOTTOM; break;
        default: break;
        }
    }

    if (location == Plasma::Types::LeftEdge || location == Plasma::Types::RightEdge) {
        if (verticalDockTouchesTopLengthEdge(alignment, maxLength, offset) && !forceTopBorder)
            borders &= ~B_TOP;
        if (verticalDockTouchesBottomLengthEdge(alignment, maxLength, offset) && !forceBottomBorder)
            borders &= ~B_BOTTOM;
    }

    if (location == Plasma::Types::TopEdge || location == Plasma::Types::BottomEdge) {
        if (horizontalDockTouchesLeftLengthEdge(alignment, maxLength, offset))
            borders &= ~B_LEFT;
        if (horizontalDockTouchesRightLengthEdge(alignment, maxLength, offset))
            borders &= ~B_RIGHT;
    }

    if (!backgroundAllCorners) {
        if (location == Plasma::Types::TopEdge || location == Plasma::Types::BottomEdge) {
            if (qFuzzyCompare(maxLength, 1.0f) && alignment == Latte::Types::Justify) {
                borders &= ~B_LEFT;
                borders &= ~B_RIGHT;
            }
        }
    }

    return borders;
}

QRegion combinedMask(const QRect &baseMask,
                      const QHash<QString, QRegion> &subtracted,
                      const QHash<QString, QRegion> &united)
{
    QRegion region(baseMask);
    for (auto &r : subtracted) region = region.subtracted(r);
    for (auto &r : united)     region = region.united(r);
    return region;
}

bool shouldApplyBlur(float backgroundOpacity, const QRect &rect, bool drawEffects)
{
    if (!drawEffects) return false;
    if (rect.isNull() || rect.isEmpty()) return false;
    // Default (-1, theme-controlled) passes; custom values >= 0.95 are skipped
    if (backgroundOpacity >= 0.95f) return false;
    return true;
}

} // namespace EffectsLogic

class EffectsLogicTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void screenEdgeMarginEnabledKeepsAllBorders();
    void screenEdgeMarginDisabledRemovesAttachedEdge();
    void forceTopBorderPreventsRemoval();
    void forceBottomBorderPreventsRemoval();
    void centerAlignmentKeepsAllBordersVertical();
    void justifyFullLengthRemovesBothEndBorders();
    void leftAlignOnHorizontalDockKeepsRightBorder();
    void rightAlignOnHorizontalDockKeepsLeftBorder();
    void allCornersFalseWithJustifyRemovesSideBorders();

    void baseMaskWithoutSubtractionsIsIdentity();
    void subtractionsReduceMask();
    void unionsExpandMask();
    void emptySubtractionIsNoOp();

    void blurDisabledAboveThreshold();
    void blurEnabledBelowThreshold();
    void blurEnabledAtDefaultOpacity();
    void blurDisabledWhenDrawEffectsFalse();
    void blurDisabledWhenRectInvalid();
};

void EffectsLogicTest::screenEdgeMarginEnabledKeepsAllBorders()
{
    for (auto loc : {Plasma::Types::TopEdge, Plasma::Types::BottomEdge, Plasma::Types::LeftEdge, Plasma::Types::RightEdge})
        QCOMPARE(EffectsLogic::computeEnabledBorders(loc, true, Latte::Types::Center, 0.5f, 0.f, false, false, true), B_ALL);
}

void EffectsLogicTest::screenEdgeMarginDisabledRemovesAttachedEdge()
{
    QCOMPARE(EffectsLogic::computeEnabledBorders(Plasma::Types::TopEdge, false, Latte::Types::Center, 0.5f, 0.f, false, false, true), B_ALL & ~B_TOP);
    QCOMPARE(EffectsLogic::computeEnabledBorders(Plasma::Types::BottomEdge, false, Latte::Types::Center, 0.5f, 0.f, false, false, true), B_ALL & ~B_BOTTOM);
    QCOMPARE(EffectsLogic::computeEnabledBorders(Plasma::Types::LeftEdge, false, Latte::Types::Center, 0.5f, 0.f, false, false, true), B_ALL & ~B_LEFT);
    QCOMPARE(EffectsLogic::computeEnabledBorders(Plasma::Types::RightEdge, false, Latte::Types::Center, 0.5f, 0.f, false, false, true), B_ALL & ~B_RIGHT);
}

void EffectsLogicTest::forceTopBorderPreventsRemoval()
{
    QCOMPARE(EffectsLogic::computeEnabledBorders(Plasma::Types::LeftEdge, false, Latte::Types::Top, 0.5f, 0.f, true, false, true), B_ALL & ~B_LEFT);
}

void EffectsLogicTest::forceBottomBorderPreventsRemoval()
{
    QCOMPARE(EffectsLogic::computeEnabledBorders(Plasma::Types::LeftEdge, false, Latte::Types::Bottom, 0.5f, 0.f, false, true, true), B_ALL & ~B_LEFT);
}

void EffectsLogicTest::centerAlignmentKeepsAllBordersVertical()
{
    QCOMPARE(EffectsLogic::computeEnabledBorders(Plasma::Types::LeftEdge, false, Latte::Types::Center, 0.5f, 0.f, false, false, true), B_ALL & ~B_LEFT);
}

void EffectsLogicTest::justifyFullLengthRemovesBothEndBorders()
{
    QCOMPARE(EffectsLogic::computeEnabledBorders(Plasma::Types::LeftEdge, false, Latte::Types::Justify, 1.0f, 0.f, false, false, true), B_ALL & ~B_LEFT & ~B_TOP & ~B_BOTTOM);
}

void EffectsLogicTest::leftAlignOnHorizontalDockKeepsRightBorder()
{
    int b = EffectsLogic::computeEnabledBorders(Plasma::Types::BottomEdge, false, Latte::Types::Left, 0.5f, 0.f, false, false, true);
    QVERIFY(b & B_RIGHT);
    QVERIFY(!(b & B_LEFT));
}

void EffectsLogicTest::rightAlignOnHorizontalDockKeepsLeftBorder()
{
    int b = EffectsLogic::computeEnabledBorders(Plasma::Types::TopEdge, false, Latte::Types::Right, 0.5f, 0.f, false, false, true);
    QVERIFY(b & B_LEFT);
    QVERIFY(!(b & B_RIGHT));
}

void EffectsLogicTest::allCornersFalseWithJustifyRemovesSideBorders()
{
    int b = EffectsLogic::computeEnabledBorders(Plasma::Types::BottomEdge, false, Latte::Types::Justify, 1.0f, 0.f, false, false, false);
    QVERIFY(!(b & B_LEFT));
    QVERIFY(!(b & B_RIGHT));
    QVERIFY(!(b & B_BOTTOM)); // screen edge margin disabled
}

void EffectsLogicTest::baseMaskWithoutSubtractionsIsIdentity()
{
    QCOMPARE(EffectsLogic::combinedMask(QRect(0,0,100,50), {}, {}), QRegion(QRect(0,0,100,50)));
}

void EffectsLogicTest::subtractionsReduceMask()
{
    QHash<QString, QRegion> sub; sub[QStringLiteral("tl")] = QRegion(0,0,10,10);
    QVERIFY(!EffectsLogic::combinedMask(QRect(0,0,100,50), sub, {}).contains(QPoint(5,5)));
}

void EffectsLogicTest::unionsExpandMask()
{
    QHash<QString, QRegion> uni; uni[QStringLiteral("x")] = QRegion(100,0,20,50);
    QVERIFY(EffectsLogic::combinedMask(QRect(0,0,100,50), {}, uni).boundingRect().width() == 120);
}

void EffectsLogicTest::emptySubtractionIsNoOp()
{
    QHash<QString, QRegion> sub; sub[QStringLiteral("e")] = QRegion();
    QCOMPARE(EffectsLogic::combinedMask(QRect(0,0,100,50), sub, {}), QRegion(QRect(0,0,100,50)));
}

void EffectsLogicTest::blurDisabledAboveThreshold()
{
    QVERIFY(!EffectsLogic::shouldApplyBlur(0.95f, QRect(0,0,100,50), true));
    QVERIFY(!EffectsLogic::shouldApplyBlur(1.0f, QRect(0,0,100,50), true));
}

void EffectsLogicTest::blurEnabledBelowThreshold()
{
    QVERIFY(EffectsLogic::shouldApplyBlur(0.94f, QRect(0,0,100,50), true));
}

void EffectsLogicTest::blurEnabledAtDefaultOpacity()
{
    // -1 is the default (theme-controlled transparency), should allow blur
    QVERIFY(EffectsLogic::shouldApplyBlur(-1.0f, QRect(0,0,100,50), true));
}

void EffectsLogicTest::blurDisabledWhenDrawEffectsFalse()
{
    QVERIFY(!EffectsLogic::shouldApplyBlur(0.5f, QRect(0,0,100,50), false));
}

void EffectsLogicTest::blurDisabledWhenRectInvalid()
{
    QVERIFY(!EffectsLogic::shouldApplyBlur(0.5f, QRect(), true));
    QVERIFY(!EffectsLogic::shouldApplyBlur(0.5f, QRect(0,0,0,0), true));
}

QTEST_APPLESS_MAIN(EffectsLogicTest)
#include "effectslogictest.moc"
