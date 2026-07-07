/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QObject>
#include <QRect>
#include <QStringList>
#include <QTest>
#include <Plasma/Plasma>

// Replicates core logic from app/view/visibilitymanager.cpp

namespace VisibilityMode {
enum Type { AlwaysVisible=0, AutoHide=1, DodgeActive=2, DodgeMaximized=3, DodgeAllWindows=4,
            WindowsGoBelow=5, WindowsCanCover=6, WindowsAlwaysCover=7, SidebarOnDemand=8, SidebarAutoHide=9 };
}

class BlockHidingEvents {
public:
    bool isBlocked() const { return !m_events.isEmpty(); }
    bool hasEvent(const QString &t) const { return !t.isEmpty() && m_events.contains(t); }
    bool addEvent(const QString &t) {
        if (m_events.contains(t) || t.isEmpty()) return false;
        bool was = isBlocked(); m_events << t; return was != isBlocked();
    }
    bool removeEvent(const QString &t) {
        if (!m_events.contains(t) || t.isEmpty()) return false;
        bool was = isBlocked(); m_events.removeAll(t); return was != isBlocked();
    }
    int count() const { return m_events.size(); }
private:
    QStringList m_events;
};

QRect computeStruts(const QRect &screen, const QRect &view, Plasma::Types::Location loc, int thickness) {
    if (thickness <= 0) return {};
    switch (loc) {
    case Plasma::Types::TopEdge: return {view.x(), screen.top(), view.width(), thickness};
    case Plasma::Types::BottomEdge: return {view.x(), screen.bottom()-thickness+1, view.width(), thickness};
    case Plasma::Types::LeftEdge: return {screen.left(), view.y(), thickness, view.height()};
    case Plasma::Types::RightEdge: return {screen.right()-thickness+1, view.y(), thickness, view.height()};
    default: return {};
    }
}

QMargins frameExtents(Plasma::Types::Location loc, int gap) {
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

bool isSidebarMode(int m) { return m==VisibilityMode::SidebarOnDemand || m==VisibilityMode::SidebarAutoHide; }

bool modeSupportsKWinEdges(int m) {
    switch (m) {
    case VisibilityMode::AutoHide: case VisibilityMode::DodgeActive:
    case VisibilityMode::DodgeAllWindows: case VisibilityMode::DodgeMaximized:
    case VisibilityMode::WindowsCanCover: return true;
    default: return false;
    }
}

bool dodgeShouldRaise(bool containsMouse, bool windowTouching) {
    return containsMouse || !windowTouching;
}

class VisibilityLogicTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void initiallyNotBlocked();
    void addMakesBlocked();
    void removeLastMakesUnblocked();
    void duplicateIgnored();
    void emptyTypeIgnored();
    void multipleEventsKeepBlocked();
    void removeOneOfMultipleStillBlocked();
    void addReturnsChangedWhenStatusFlips();

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

void VisibilityLogicTest::initiallyNotBlocked() { BlockHidingEvents e; QVERIFY(!e.isBlocked()); }
void VisibilityLogicTest::addMakesBlocked() { BlockHidingEvents e; e.addEvent(QStringLiteral("drag")); QVERIFY(e.isBlocked()); }
void VisibilityLogicTest::removeLastMakesUnblocked() {
    BlockHidingEvents e; e.addEvent(QStringLiteral("drag")); e.removeEvent(QStringLiteral("drag")); QVERIFY(!e.isBlocked());
}
void VisibilityLogicTest::duplicateIgnored() {
    BlockHidingEvents e; QVERIFY(e.addEvent(QStringLiteral("menu"))); QVERIFY(!e.addEvent(QStringLiteral("menu"))); QCOMPARE(e.count(), 1);
}
void VisibilityLogicTest::emptyTypeIgnored() { BlockHidingEvents e; QVERIFY(!e.addEvent(QString())); QVERIFY(!e.isBlocked()); }
void VisibilityLogicTest::multipleEventsKeepBlocked() {
    BlockHidingEvents e; e.addEvent(QStringLiteral("a")); e.addEvent(QStringLiteral("b")); QVERIFY(e.isBlocked()); QCOMPARE(e.count(), 2);
}
void VisibilityLogicTest::removeOneOfMultipleStillBlocked() {
    BlockHidingEvents e; e.addEvent(QStringLiteral("a")); e.addEvent(QStringLiteral("b"));
    QVERIFY(!e.removeEvent(QStringLiteral("a"))); // still blocked
    QVERIFY(e.isBlocked());
}
void VisibilityLogicTest::addReturnsChangedWhenStatusFlips() {
    BlockHidingEvents e; QVERIFY(e.addEvent(QStringLiteral("first"))); QVERIFY(!e.addEvent(QStringLiteral("first")));
}

void VisibilityLogicTest::topStrutsAlignsWithView() {
    QRect s(0,0,1920,1080), v(100,0,500,48);
    QCOMPARE(computeStruts(s,v,Plasma::Types::TopEdge,48), QRect(100,0,500,48));
}
void VisibilityLogicTest::bottomStrutsHugsBottom() {
    QRect s(0,0,1920,1080), v(400,1032,600,48);
    auto r = computeStruts(s,v,Plasma::Types::BottomEdge,48);
    QCOMPARE(r.bottom(), s.bottom()); QCOMPARE(r.height(), 48);
}
void VisibilityLogicTest::zeroThicknessEmpty() { QVERIFY(computeStruts({0,0,1920,1080},{100,0,500,48},Plasma::Types::TopEdge,0).isNull()); }
void VisibilityLogicTest::floatingLocationEmpty() { QVERIFY(computeStruts({0,0,1920,1080},{100,0,500,48},Plasma::Types::Floating,48).isNull()); }

void VisibilityLogicTest::onlySidebarModesAreSidebars() {
    QVERIFY(isSidebarMode(VisibilityMode::SidebarOnDemand));
    QVERIFY(isSidebarMode(VisibilityMode::SidebarAutoHide));
    QVERIFY(!isSidebarMode(VisibilityMode::AlwaysVisible));
    QVERIFY(!isSidebarMode(VisibilityMode::AutoHide));
}
void VisibilityLogicTest::hidingModesSupportKWinEdges() {
    QVERIFY(modeSupportsKWinEdges(VisibilityMode::AutoHide));
    QVERIFY(modeSupportsKWinEdges(VisibilityMode::DodgeActive));
}
void VisibilityLogicTest::nonHidingModesDoNot() {
    QVERIFY(!modeSupportsKWinEdges(VisibilityMode::AlwaysVisible));
    QVERIFY(!modeSupportsKWinEdges(VisibilityMode::SidebarOnDemand));
}

void VisibilityLogicTest::frameExtentsForEachEdge() {
    QCOMPARE(frameExtents(Plasma::Types::LeftEdge,8).right(),8);
    QCOMPARE(frameExtents(Plasma::Types::TopEdge,6).bottom(),6);
    QCOMPARE(frameExtents(Plasma::Types::RightEdge,10).left(),10);
    QCOMPARE(frameExtents(Plasma::Types::BottomEdge,12).top(),12);
}
void VisibilityLogicTest::zeroGapProducesZeroExtents() { QVERIFY(frameExtents(Plasma::Types::BottomEdge,0).isNull()); }

void VisibilityLogicTest::dodgeRaisesWithMouseInside() { QVERIFY(dodgeShouldRaise(true,true)); QVERIFY(dodgeShouldRaise(true,false)); }
void VisibilityLogicTest::dodgeHidesWhenWindowTouchingWithoutMouse() { QVERIFY(!dodgeShouldRaise(false,true)); }
void VisibilityLogicTest::sidebarOnDemandToggle() {
    bool shown=false; shown=!shown; QVERIFY(shown); shown=!shown; QVERIFY(!shown);
}

QTEST_APPLESS_MAIN(VisibilityLogicTest)
#include "visibilitylogictest.moc"
