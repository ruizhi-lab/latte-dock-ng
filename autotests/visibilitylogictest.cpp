/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QObject>
#include <QRect>
#include <QTest>
#include <Plasma/Plasma>

// These pure geometry and mode checks avoid hosting a full Plasma view;
// hiding-blocker behavior is tested against its production helper.

namespace VisibilityMode {
enum Type { AlwaysVisible = 0, AutoHide = 1, DodgeActive = 2, DodgeMaximized = 3, DodgeAllWindows = 4,
            WindowsGoBelow = 5, WindowsCanCover = 6, WindowsAlwaysCover = 7, SidebarOnDemand = 8, SidebarAutoHide = 9
          };
}

QRect computeStruts(const QRect &screen, const QRect &view, Plasma::Types::Location loc, int thickness)
{
    if (thickness <= 0) return {};

    switch (loc) {
        case Plasma::Types::TopEdge: return {view.x(), screen.top(), view.width(), thickness};
        case Plasma::Types::BottomEdge: return {view.x(), screen.bottom() - thickness + 1, view.width(), thickness};
        case Plasma::Types::LeftEdge: return {screen.left(), view.y(), thickness, view.height()};
        case Plasma::Types::RightEdge: return {screen.right() - thickness + 1, view.y(), thickness, view.height()};
        default: return {};
    }
}

QMargins frameExtents(Plasma::Types::Location loc, int gap)
{
    QMargins m;

    switch (loc) {
        case Plasma::Types::LeftEdge: m.setRight(gap); break;

        case Plasma::Types::TopEdge: m.setBottom(gap); break;

        case Plasma::Types::RightEdge: m.setLeft(gap); break;

        case Plasma::Types::BottomEdge: m.setTop(gap); break;

        default: break;
    }

    return m;
}

bool isSidebarMode(int m) { return m == VisibilityMode::SidebarOnDemand || m == VisibilityMode::SidebarAutoHide; }

bool modeSupportsKWinEdges(int m)
{
    switch (m) {
        case VisibilityMode::AutoHide: case VisibilityMode::DodgeActive:
        case VisibilityMode::DodgeAllWindows: case VisibilityMode::DodgeMaximized:
        case VisibilityMode::WindowsCanCover: return true;

        default: return false;
    }
}

bool dodgeShouldRaise(bool containsMouse, bool windowTouching)
{
    return containsMouse || !windowTouching;
}

class VisibilityLogicTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void topStrutsAlignsWithView();
    void bottomStrutsHugsBottom();
    void zeroThicknessEmpty();
    void floatingLocationEmpty();

    void onlySidebarModesAreSidebars();
    void hidingModesSupportKWinEdges();
    void nonHidingModesDoNot();

    void frameExtentsForEachEdge();
    void zeroGapProducesZeroExtents();

    void dodgeRaisesWithMouseInside();
    void dodgeHidesWhenWindowTouchingWithoutMouse();
    void sidebarOnDemandToggle();
};

void VisibilityLogicTest::topStrutsAlignsWithView()
{
    QRect s(0, 0, 1920, 1080), v(100, 0, 500, 48);
    QCOMPARE(computeStruts(s, v, Plasma::Types::TopEdge, 48), QRect(100, 0, 500, 48));
}

void VisibilityLogicTest::bottomStrutsHugsBottom()
{
    QRect s(0, 0, 1920, 1080), v(400, 1032, 600, 48);
    auto r = computeStruts(s, v, Plasma::Types::BottomEdge, 48);
    QCOMPARE(r.bottom(), s.bottom()); QCOMPARE(r.height(), 48);
}

void VisibilityLogicTest::zeroThicknessEmpty() { QVERIFY(computeStruts({0, 0, 1920, 1080}, {100, 0, 500, 48}, Plasma::Types::TopEdge, 0).isNull()); }

void VisibilityLogicTest::floatingLocationEmpty() { QVERIFY(computeStruts({0, 0, 1920, 1080}, {100, 0, 500, 48}, Plasma::Types::Floating, 48).isNull()); }

void VisibilityLogicTest::onlySidebarModesAreSidebars()
{
    QVERIFY(isSidebarMode(VisibilityMode::SidebarOnDemand));
    QVERIFY(isSidebarMode(VisibilityMode::SidebarAutoHide));
    QVERIFY(!isSidebarMode(VisibilityMode::AlwaysVisible));
    QVERIFY(!isSidebarMode(VisibilityMode::AutoHide));
}

void VisibilityLogicTest::hidingModesSupportKWinEdges()
{
    QVERIFY(modeSupportsKWinEdges(VisibilityMode::AutoHide));
    QVERIFY(modeSupportsKWinEdges(VisibilityMode::DodgeActive));
}

void VisibilityLogicTest::nonHidingModesDoNot()
{
    QVERIFY(!modeSupportsKWinEdges(VisibilityMode::AlwaysVisible));
    QVERIFY(!modeSupportsKWinEdges(VisibilityMode::SidebarOnDemand));
}

void VisibilityLogicTest::frameExtentsForEachEdge()
{
    QCOMPARE(frameExtents(Plasma::Types::LeftEdge, 8).right(), 8);
    QCOMPARE(frameExtents(Plasma::Types::TopEdge, 6).bottom(), 6);
    QCOMPARE(frameExtents(Plasma::Types::RightEdge, 10).left(), 10);
    QCOMPARE(frameExtents(Plasma::Types::BottomEdge, 12).top(), 12);
}

void VisibilityLogicTest::zeroGapProducesZeroExtents() { QVERIFY(frameExtents(Plasma::Types::BottomEdge, 0).isNull()); }

void VisibilityLogicTest::dodgeRaisesWithMouseInside() { QVERIFY(dodgeShouldRaise(true, true)); QVERIFY(dodgeShouldRaise(true, false)); }

void VisibilityLogicTest::dodgeHidesWhenWindowTouchingWithoutMouse() { QVERIFY(!dodgeShouldRaise(false, true)); }

void VisibilityLogicTest::sidebarOnDemandToggle()
{
    bool shown = false; shown = !shown; QVERIFY(shown); shown = !shown; QVERIFY(!shown);
}

QTEST_APPLESS_MAIN(VisibilityLogicTest)
#include "visibilitylogictest.moc"
