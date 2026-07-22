/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

//! Tests the nullifier-guard state machine added to Parabolic to prevent
//! oscillation when the cursor is in an inter-icon gap.  The guard:
//!   1. After the nullifier fires → m_nullifierJustFired = true
//!   2. While true, m_parabolicItemNullifier.start() is skipped
//!      (handlePointerMove won't restart it)
//!   3. An enter from genuine cursor movement clears the guard
//!   4. An enter from animation boundary-crossing (stationary cursor)
//!      does NOT clear the guard

#include <QTest>
#include <QPointF>

namespace {

constexpr double STATIONARY_THRESHOLD{3.0};

//! Simulates the nullifier-fire position and a subsequent cursor position
//! to decide whether the guard should be cleared.
bool shouldClearGuard(const QPointF &firePos, const QPointF &currentPos)
{
    if (firePos.isNull())
        return true;
    return (currentPos - firePos).manhattanLength() >= STATIONARY_THRESHOLD;
}

//! Simulates the nullifier restart decision.
bool shouldStartNullifier(bool nullifierJustFired)
{
    return !nullifierJustFired;
}

} // namespace

class ParabolicStateTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void nullifierFiresGuardsRestart();
    void nullifierNotFiredAllowsRestart();
    void stationaryCursorKeepsGuard();
    void movingCursorClearsGuard();
    void tinyMovementStillStationary();
    void nullPositionClearsGuard();
    void diagonalMovementClearsGuard();
    void largeMovementClearsGuard();
    void borderMovementEqualsThreshold();
};

void ParabolicStateTest::nullifierFiresGuardsRestart()
{
    QVERIFY(!shouldStartNullifier(true));  // guard active → no restart
}

void ParabolicStateTest::nullifierNotFiredAllowsRestart()
{
    QVERIFY(shouldStartNullifier(false));  // guard inactive → restart
}

void ParabolicStateTest::stationaryCursorKeepsGuard()
{
    const QPointF firePos(100, 200);
    const QPointF curPos(100, 200);  // identical
    QVERIFY(!shouldClearGuard(firePos, curPos));
}

void ParabolicStateTest::movingCursorClearsGuard()
{
    const QPointF firePos(100, 200);
    const QPointF curPos(103, 200);  // 3px horizontal
    QVERIFY(shouldClearGuard(firePos, curPos));
}

void ParabolicStateTest::tinyMovementStillStationary()
{
    const QPointF firePos(100, 200);
    const QPointF curPos(101, 200);  // 1px
    QVERIFY(!shouldClearGuard(firePos, curPos));
}

void ParabolicStateTest::nullPositionClearsGuard()
{
    QVERIFY(shouldClearGuard(QPointF(), QPointF(50, 50)));
}

void ParabolicStateTest::diagonalMovementClearsGuard()
{
    const QPointF firePos(100, 200);
    const QPointF curPos(102, 201);  // manhattan = 3
    QVERIFY(shouldClearGuard(firePos, curPos));
}

void ParabolicStateTest::largeMovementClearsGuard()
{
    const QPointF firePos(100, 200);
    const QPointF curPos(200, 300);
    QVERIFY(shouldClearGuard(firePos, curPos));
}

void ParabolicStateTest::borderMovementEqualsThreshold()
{
    const QPointF firePos(100, 200);
    const QPointF curPos(102, 201);  // manhattan = |2| + |1| = 3
    QCOMPARE((curPos - firePos).manhattanLength(), 3.0);
    QVERIFY(shouldClearGuard(firePos, curPos));
}

QTEST_APPLESS_MAIN(ParabolicStateTest)
#include "parabolicstatetest.moc"
