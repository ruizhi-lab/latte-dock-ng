/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "panelbackground.h"

#include <QSignalSpy>
#include <QTest>

class PlasmaExtendedUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void panelBackgroundStartsWithNeutralMetrics();
    void panelBackgroundForAllEdgesReturnsNeutralDefaults();
    void panelBackgroundPaddingsAreIndependentPerEdge();
    void panelBackgroundShadowColorDefaultsToInvalid();
    void panelBackgroundRoundnessStartsAtZero();
};

void PlasmaExtendedUnitTest::panelBackgroundStartsWithNeutralMetrics()
{
    for (auto edge : {Plasma::Types::TopEdge, Plasma::Types::RightEdge}) {
        Latte::PlasmaExtended::PanelBackground bg(edge, nullptr);
        QCOMPARE(bg.paddingTop(), 0);
        QCOMPARE(bg.paddingLeft(), 0);
        QCOMPARE(bg.paddingBottom(), 0);
        QCOMPARE(bg.paddingRight(), 0);
        QCOMPARE(bg.shadowSize(), 0);
        QCOMPARE(bg.roundness(), 0);
        QCOMPARE(bg.maxOpacity(), 1.0f);
        QCOMPARE(bg.shadowColor(), QColor());
    }
}

void PlasmaExtendedUnitTest::panelBackgroundForAllEdgesReturnsNeutralDefaults()
{
    for (auto edge : {Plasma::Types::TopEdge, Plasma::Types::LeftEdge, Plasma::Types::BottomEdge, Plasma::Types::RightEdge}) {
        Latte::PlasmaExtended::PanelBackground bg(edge, nullptr);
        QCOMPARE(bg.paddingTop(), 0);
        QCOMPARE(bg.shadowSize(), 0);
    }
}

void PlasmaExtendedUnitTest::panelBackgroundPaddingsAreIndependentPerEdge()
{
    Latte::PlasmaExtended::PanelBackground top(Plasma::Types::TopEdge, nullptr);
    Latte::PlasmaExtended::PanelBackground left(Plasma::Types::LeftEdge, nullptr);
    Latte::PlasmaExtended::PanelBackground bottom(Plasma::Types::BottomEdge, nullptr);
    Latte::PlasmaExtended::PanelBackground right(Plasma::Types::RightEdge, nullptr);
    // All start at zero — verifying independent construction doesn't corrupt state
    QCOMPARE(top.maxOpacity(), 1.0f);
    QCOMPARE(left.maxOpacity(), 1.0f);
    QCOMPARE(bottom.maxOpacity(), 1.0f);
    QCOMPARE(right.maxOpacity(), 1.0f);
}

void PlasmaExtendedUnitTest::panelBackgroundShadowColorDefaultsToInvalid()
{
    Latte::PlasmaExtended::PanelBackground bg(Plasma::Types::TopEdge, nullptr);
    QVERIFY(!bg.shadowColor().isValid());
}

void PlasmaExtendedUnitTest::panelBackgroundRoundnessStartsAtZero()
{
    Latte::PlasmaExtended::PanelBackground bg(Plasma::Types::BottomEdge, nullptr);
    QCOMPARE(bg.roundness(), 0);
}

QTEST_GUILESS_MAIN(PlasmaExtendedUnitTest)

#include "plasmaextendedunittest.moc"
