/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "view/viewgeometryhelpers.h"

#include <QRect>
#include <QTest>
#include <Plasma/Plasma>

using namespace Latte::ViewPart;

class PositionerGeometryTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // verticalDockExternalPanelGeometry (QList overload)
    void emptyPanelListPreservesScreenGeometry();
    void topPanelReducesAvailableTopSpace();
    void bottomPanelReducesAvailableBottomSpace();
    void leftPanelReducesAvailableLeftSpace();
    void rightPanelReducesAvailableRightSpace();
    void topAndBottomPanelsShrinkBothSides();
    void leftAndRightPanelsShrinkBothSides();
    void fourSurroundingPanelsShrinkAllEdges();
    void overlappingPanelsUseMostRestrictiveBoundary();
    void nonIntersectingPanelLeavesGeometryUnchanged();
    void invalidScreenGeometryReturnsItUnchanged();
    void invalidOrEmptyPanelGeometryIsSkipped();

    // verticalDockExternalPanelGeometry (QRect overload)
    void singleAvailableRectKeepsScreenWidth();
    void fullScreenAvailableReturnsScreenGeometry();
    void availableRectTopOffsetPreservesBottom();

    // screenEdgePanelGeometry
    void floatingAndDesktopLocationReturnsNull();
    void zeroThicknessReturnsNullForAllEdges();
    void thicknessLargerThanScreenIsClamped();
    void negativeScreenOriginIsHandled();

    // dockAlignmentForLocation
    void leftAndRightAlignmentRemappedForVerticalEdges();
    void topAndBottomAlignmentRemappedForHorizontalEdges();
    void centerAndJustifyAlignmentPreservedOnAllEdges();
    void desktopEdgePreservesAllAlignments();

    // dockFormFactorForLocation
    void floatingAndDesktopAreFallbackFormFactor();
    void topAndBottomAreHorizontal();
    void leftAndRightAreVertical();

    // multi-panel scenarios from Positioner::immediateSyncGeometry
    void typicalPlasmaDesktopWithTopBar();
    void twoMonitorsWithDocksOnDifferentEdges();
    void panelCoveringEntireTopEdge();
    void narrowSidePanelBesideWideBottomPanel();
};

void PositionerGeometryTest::emptyPanelListPreservesScreenGeometry()
{
    const QRect screen(0, 0, 1920, 1080);
    QCOMPARE(verticalDockExternalPanelGeometry(screen, QList<QRect>{}), screen);
}

void PositionerGeometryTest::topPanelReducesAvailableTopSpace()
{
    const QRect screen(0, 0, 1920, 1080);
    const QList<QRect> panels{screenEdgePanelGeometry(screen, Plasma::Types::TopEdge, 40)};
    QCOMPARE(verticalDockExternalPanelGeometry(screen, panels), QRect(0, 40, 1920, 1040));
}

void PositionerGeometryTest::bottomPanelReducesAvailableBottomSpace()
{
    const QRect screen(0, 0, 1920, 1080);
    const QList<QRect> panels{screenEdgePanelGeometry(screen, Plasma::Types::BottomEdge, 32)};
    QCOMPARE(verticalDockExternalPanelGeometry(screen, panels), QRect(0, 0, 1920, 1048));
}

void PositionerGeometryTest::leftPanelReducesAvailableLeftSpace()
{
    const QRect screen(0, 0, 1920, 1080);
    const QList<QRect> panels{screenEdgePanelGeometry(screen, Plasma::Types::LeftEdge, 60)};
    QCOMPARE(verticalDockExternalPanelGeometry(screen, panels), QRect(60, 0, 1860, 1080));
}

void PositionerGeometryTest::rightPanelReducesAvailableRightSpace()
{
    const QRect screen(0, 0, 1920, 1080);
    const QList<QRect> panels{screenEdgePanelGeometry(screen, Plasma::Types::RightEdge, 50)};
    QCOMPARE(verticalDockExternalPanelGeometry(screen, panels), QRect(0, 0, 1870, 1080));
}

void PositionerGeometryTest::topAndBottomPanelsShrinkBothSides()
{
    const QRect screen(0, 0, 1920, 1080);
    const QList<QRect> panels{
        screenEdgePanelGeometry(screen, Plasma::Types::TopEdge, 40),
        screenEdgePanelGeometry(screen, Plasma::Types::BottomEdge, 30),
    };
    QCOMPARE(verticalDockExternalPanelGeometry(screen, panels), QRect(0, 40, 1920, 1010));
}

void PositionerGeometryTest::leftAndRightPanelsShrinkBothSides()
{
    const QRect screen(0, 0, 1920, 1080);
    const QList<QRect> panels{
        screenEdgePanelGeometry(screen, Plasma::Types::LeftEdge, 60),
        screenEdgePanelGeometry(screen, Plasma::Types::RightEdge, 50),
    };
    QCOMPARE(verticalDockExternalPanelGeometry(screen, panels), QRect(60, 0, 1810, 1080));
}

void PositionerGeometryTest::fourSurroundingPanelsShrinkAllEdges()
{
    const QRect screen(0, 0, 1920, 1080);
    const QList<QRect> panels{
        screenEdgePanelGeometry(screen, Plasma::Types::TopEdge, 30),
        screenEdgePanelGeometry(screen, Plasma::Types::BottomEdge, 30),
        screenEdgePanelGeometry(screen, Plasma::Types::LeftEdge, 60),
        screenEdgePanelGeometry(screen, Plasma::Types::RightEdge, 50),
    };
    QCOMPARE(verticalDockExternalPanelGeometry(screen, panels), QRect(60, 30, 1810, 1020));
}

void PositionerGeometryTest::overlappingPanelsUseMostRestrictiveBoundary()
{
    const QRect screen(0, 0, 1920, 1080);
    const QList<QRect> panels{QRect(0, 0, 1920, 30), QRect(0, 0, 1920, 50)};
    const QRect result = verticalDockExternalPanelGeometry(screen, panels);
    QVERIFY(result.top() >= 50);
}

void PositionerGeometryTest::nonIntersectingPanelLeavesGeometryUnchanged()
{
    const QRect screen(0, 0, 1920, 1080);
    const QList<QRect> panels{QRect(2000, 0, 500, 40)};
    QCOMPARE(verticalDockExternalPanelGeometry(screen, panels), screen);
}

void PositionerGeometryTest::invalidScreenGeometryReturnsItUnchanged()
{
    const QRect invalid;
    const QList<QRect> panels{screenEdgePanelGeometry(QRect(0, 0, 1920, 1080), Plasma::Types::TopEdge, 40)};
    QCOMPARE(verticalDockExternalPanelGeometry(invalid, panels), invalid);
}

void PositionerGeometryTest::invalidOrEmptyPanelGeometryIsSkipped()
{
    const QRect screen(0, 0, 1920, 1080);
    const QList<QRect> panels{QRect(), QRect(0, 0, 0, 0), screenEdgePanelGeometry(screen, Plasma::Types::LeftEdge, 100)};
    QCOMPARE(verticalDockExternalPanelGeometry(screen, panels), QRect(100, 0, 1820, 1080));
}

void PositionerGeometryTest::singleAvailableRectKeepsScreenWidth()
{
    const QRect screen(100, 200, 1600, 900);
    const QRect available(150, 250, 1500, 800);
    const QRect result = verticalDockExternalPanelGeometry(screen, available);
    QCOMPARE(result.x(), screen.x());
    QCOMPARE(result.width(), screen.width());
    QCOMPARE(result.y(), available.y());
    QCOMPARE(result.height(), available.height());
}

void PositionerGeometryTest::fullScreenAvailableReturnsScreenGeometry()
{
    const QRect screen(0, 0, 1920, 1080);
    QCOMPARE(verticalDockExternalPanelGeometry(screen, screen), screen);
}

void PositionerGeometryTest::availableRectTopOffsetPreservesBottom()
{
    const QRect screen(0, 0, 1920, 1080);
    const QRect available(0, 30, 1920, 1050);
    const QRect result = verticalDockExternalPanelGeometry(screen, available);
    QCOMPARE(result.y(), 30);
    QCOMPARE(result.bottom(), screen.bottom());
}

void PositionerGeometryTest::floatingAndDesktopLocationReturnsNull()
{
    QVERIFY(screenEdgePanelGeometry(QRect(0, 0, 1920, 1080), Plasma::Types::Floating, 36).isNull());
    QVERIFY(screenEdgePanelGeometry(QRect(0, 0, 1920, 1080), Plasma::Types::Desktop, 36).isNull());
}

void PositionerGeometryTest::zeroThicknessReturnsNullForAllEdges()
{
    const QRect screen(0, 0, 1000, 800);
    QVERIFY(screenEdgePanelGeometry(screen, Plasma::Types::TopEdge, 0).isNull());
    QVERIFY(screenEdgePanelGeometry(screen, Plasma::Types::BottomEdge, 0).isNull());
    QVERIFY(screenEdgePanelGeometry(screen, Plasma::Types::LeftEdge, 0).isNull());
    QVERIFY(screenEdgePanelGeometry(screen, Plasma::Types::RightEdge, 0).isNull());
}

void PositionerGeometryTest::thicknessLargerThanScreenIsClamped()
{
    const QRect screen(10, 20, 800, 600);
    QCOMPARE(screenEdgePanelGeometry(screen, Plasma::Types::TopEdge, 999), QRect(10, 20, 800, 600));
    QCOMPARE(screenEdgePanelGeometry(screen, Plasma::Types::LeftEdge, 999), QRect(10, 20, 800, 600));
}

void PositionerGeometryTest::negativeScreenOriginIsHandled()
{
    const QRect screen(-1920, 0, 1920, 1080);
    QCOMPARE(screenEdgePanelGeometry(screen, Plasma::Types::TopEdge, 36), QRect(-1920, 0, 1920, 36));
    QCOMPARE(screenEdgePanelGeometry(screen, Plasma::Types::LeftEdge, 60), QRect(-1920, 0, 60, 1080));
}

void PositionerGeometryTest::leftAndRightAlignmentRemappedForVerticalEdges()
{
    QCOMPARE(static_cast<int>(dockAlignmentForLocation(Plasma::Types::LeftEdge, Latte::Types::Left)), static_cast<int>(Latte::Types::Top));
    QCOMPARE(static_cast<int>(dockAlignmentForLocation(Plasma::Types::LeftEdge, Latte::Types::Right)), static_cast<int>(Latte::Types::Bottom));
    QCOMPARE(static_cast<int>(dockAlignmentForLocation(Plasma::Types::RightEdge, Latte::Types::Left)), static_cast<int>(Latte::Types::Top));
    QCOMPARE(static_cast<int>(dockAlignmentForLocation(Plasma::Types::RightEdge, Latte::Types::Right)), static_cast<int>(Latte::Types::Bottom));
}

void PositionerGeometryTest::topAndBottomAlignmentRemappedForHorizontalEdges()
{
    QCOMPARE(static_cast<int>(dockAlignmentForLocation(Plasma::Types::TopEdge, Latte::Types::Top)), static_cast<int>(Latte::Types::Left));
    QCOMPARE(static_cast<int>(dockAlignmentForLocation(Plasma::Types::TopEdge, Latte::Types::Bottom)), static_cast<int>(Latte::Types::Right));
    QCOMPARE(static_cast<int>(dockAlignmentForLocation(Plasma::Types::BottomEdge, Latte::Types::Top)), static_cast<int>(Latte::Types::Left));
    QCOMPARE(static_cast<int>(dockAlignmentForLocation(Plasma::Types::BottomEdge, Latte::Types::Bottom)), static_cast<int>(Latte::Types::Right));
}

void PositionerGeometryTest::centerAndJustifyAlignmentPreservedOnAllEdges()
{
    for (auto edge : {Plasma::Types::TopEdge, Plasma::Types::BottomEdge, Plasma::Types::LeftEdge, Plasma::Types::RightEdge}) {
        QCOMPARE(static_cast<int>(dockAlignmentForLocation(edge, Latte::Types::Center)), static_cast<int>(Latte::Types::Center));
        QCOMPARE(static_cast<int>(dockAlignmentForLocation(edge, Latte::Types::Justify)), static_cast<int>(Latte::Types::Justify));
    }
}

void PositionerGeometryTest::desktopEdgePreservesAllAlignments()
{
    QCOMPARE(static_cast<int>(dockAlignmentForLocation(Plasma::Types::Desktop, Latte::Types::Left)), static_cast<int>(Latte::Types::Left));
    QCOMPARE(static_cast<int>(dockAlignmentForLocation(Plasma::Types::Desktop, Latte::Types::Top)), static_cast<int>(Latte::Types::Top));
    QCOMPARE(static_cast<int>(dockAlignmentForLocation(Plasma::Types::Desktop, Latte::Types::Center)), static_cast<int>(Latte::Types::Center));
}

void PositionerGeometryTest::floatingAndDesktopAreFallbackFormFactor()
{
    QCOMPARE(dockFormFactorForLocation(Plasma::Types::Desktop, Plasma::Types::Horizontal), Plasma::Types::Horizontal);
    QCOMPARE(dockFormFactorForLocation(Plasma::Types::Floating, Plasma::Types::Horizontal), Plasma::Types::Horizontal);
}

void PositionerGeometryTest::topAndBottomAreHorizontal()
{
    QCOMPARE(dockFormFactorForLocation(Plasma::Types::TopEdge, Plasma::Types::Vertical), Plasma::Types::Horizontal);
    QCOMPARE(dockFormFactorForLocation(Plasma::Types::BottomEdge, Plasma::Types::Vertical), Plasma::Types::Horizontal);
}

void PositionerGeometryTest::leftAndRightAreVertical()
{
    QCOMPARE(dockFormFactorForLocation(Plasma::Types::LeftEdge, Plasma::Types::Horizontal), Plasma::Types::Vertical);
    QCOMPARE(dockFormFactorForLocation(Plasma::Types::RightEdge, Plasma::Types::Horizontal), Plasma::Types::Vertical);
}

void PositionerGeometryTest::typicalPlasmaDesktopWithTopBar()
{
    const QRect screen(0, 0, 1920, 1080);
    const QList<QRect> panels{screenEdgePanelGeometry(screen, Plasma::Types::TopEdge, 36)};
    const QRect available = verticalDockExternalPanelGeometry(screen, panels);
    QCOMPARE(available, QRect(0, 36, 1920, 1044));
    QVERIFY(available.height() == 1044);
}

void PositionerGeometryTest::twoMonitorsWithDocksOnDifferentEdges()
{
    // Left monitor (positive origin) with left-edge dock
    const QRect leftScreen(0, 0, 1920, 1080);
    const QList<QRect> leftPanels{screenEdgePanelGeometry(leftScreen, Plasma::Types::LeftEdge, 60)};
    QCOMPARE(verticalDockExternalPanelGeometry(leftScreen, leftPanels), QRect(60, 0, 1860, 1080));

    // Right monitor with bottom panel
    const QRect rightScreen(1920, 0, 1920, 1080);
    const QList<QRect> rightPanels{screenEdgePanelGeometry(rightScreen, Plasma::Types::BottomEdge, 40)};
    QCOMPARE(verticalDockExternalPanelGeometry(rightScreen, rightPanels), QRect(1920, 0, 1920, 1040));
}

void PositionerGeometryTest::panelCoveringEntireTopEdge()
{
    const QRect screen(0, 0, 2560, 1440);
    const QList<QRect> panels{screenEdgePanelGeometry(screen, Plasma::Types::TopEdge, 40)};
    QCOMPARE(verticalDockExternalPanelGeometry(screen, panels), QRect(0, 40, 2560, 1400));
}

void PositionerGeometryTest::narrowSidePanelBesideWideBottomPanel()
{
    const QRect screen(0, 0, 1920, 1080);
    const QList<QRect> panels{QRect(0, 0, 48, 1080), QRect(0, 1040, 1920, 40)};
    const QRect available = verticalDockExternalPanelGeometry(screen, panels);
    QCOMPARE(available.left(), 48);
    QCOMPARE(available.bottom(), 1039);
}

QTEST_APPLESS_MAIN(PositionerGeometryTest)
#include "positionergeometrytest.moc"
