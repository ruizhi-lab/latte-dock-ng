/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QHash>
#include <QStringList>
#include <QTest>

// Replicates core logic from app/layouts/synchronizer.cpp

struct LayoutInfo {
    QString id; QString name; bool isActive{false}; bool isShownInMenu{true}; int errors{0}; int warnings{0};
};
struct LayoutsTable {
    QList<LayoutInfo> rows;
    bool containsName(const QString &n) const { for (auto &r:rows) if(r.name==n) return true; return false; }
    QString idForName(const QString &n) const { for (auto &r:rows) if(r.name==n) return r.id; return {}; }
    LayoutInfo operator[](const QString &id) const { for (auto &r:rows) if(r.id==id) return r; return {}; }
    int rowCount() const { return rows.size(); }
    LayoutInfo at(int i) const { return rows[i]; }
};
using AssignedHash = QHash<QString, QStringList>;

namespace SyncLogic {
inline int centralLayoutPos(const QStringList &names, const QString &t) {
    for (int i=0; i<names.size(); i++) if(names.at(i)==t) return i;
    return -1;
}
inline bool isAssigned(const AssignedHash &h, const QString &n) {
    for (auto &k:h.keys()) if(h[k].contains(n)) return true;
    return false;
}
inline QStringList freeActivities(const QStringList &all, const AssignedHash &h) {
    QStringList f=all; for (auto &k:h.keys()) f.removeAll(k); return f;
}
inline QStringList freeRunningActivities(const QStringList &run, const AssignedHash &h) {
    QStringList r; for (auto &a:run) if(!h.contains(a)) r<<a; return r;
}
inline QStringList validActivities(const QStringList &all, const QStringList &cand) {
    QStringList v; for (auto &a:cand) if(all.contains(a)) v<<a; return v;
}
inline QStringList centralLayoutNames(bool single, const QStringList &n) {
    if(single) return n.isEmpty()?QStringList():QStringList{n.at(0)};
    return n;
}
inline QStringList menuLayouts(const LayoutsTable &t, const QStringList &active) {
    QStringList r;
    for(int i=0;i<t.rowCount();i++) if(t.at(i).isShownInMenu) r<<t.at(i).name;
    for(auto &n:active) if(!r.contains(n)) r.prepend(n);
    r.sort(Qt::CaseInsensitive); return r;
}
inline bool lookupData(const LayoutsTable &t, const QString &n, LayoutInfo &o) {
    if(!t.containsName(n)) return false; o=t[t.idForName(n)]; return true;
}
inline bool hasIdConflict(const QStringList &c, const QStringList &a) {
    for(auto &x:c) if(a.contains(x)) return true;
    return false;
}
}

class SynchronizerLogicTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void findLayoutPosition();
    void assignedAndUnassigned();
    void freeActivitiesLogic();
    void freeRunningActivitiesLogic();
    void validActivitiesIntersection();
    void singleVsMultipleMode();
    void menuLayoutsSortedAndActivePrepended();
    void menuExcludesHiddenLayouts();
    void lookupExistingAndNonExisting();
    void idConflictDetection();
    void typicalTwoActivitySetup();
    void sharedLayoutAcrossActivities();
};

LayoutsTable sampleTable() {
    LayoutsTable t;
    t.rows<<LayoutInfo{QStringLiteral("/dock"),QStringLiteral("MyDock"),true,true,0,0};
    t.rows<<LayoutInfo{QStringLiteral("/panel"),QStringLiteral("TopPanel"),false,true,1,0};
    t.rows<<LayoutInfo{QStringLiteral("/hidden"),QStringLiteral("HiddenLayout"),false,false,0,0};
    return t;
}

void SynchronizerLogicTest::findLayoutPosition() {
    QStringList n{QStringLiteral("D"),QStringLiteral("P")};
    QCOMPARE(SyncLogic::centralLayoutPos(n,QStringLiteral("D")),0);
    QCOMPARE(SyncLogic::centralLayoutPos(n,QStringLiteral("P")),1);
    QCOMPARE(SyncLogic::centralLayoutPos(n,QStringLiteral("X")),-1);
    QCOMPARE(SyncLogic::centralLayoutPos({},QStringLiteral("X")),-1);
}

void SynchronizerLogicTest::assignedAndUnassigned() {
    AssignedHash h; h[QStringLiteral("a1")]={QStringLiteral("Dock")};
    QVERIFY(SyncLogic::isAssigned(h,QStringLiteral("Dock")));
    QVERIFY(!SyncLogic::isAssigned(h,QStringLiteral("Panel")));
    QVERIFY(!SyncLogic::isAssigned({},QStringLiteral("Dock")));

    h[QStringLiteral("a2")]={QStringLiteral("Panel")};
    QVERIFY(SyncLogic::isAssigned(h,QStringLiteral("Dock")));
    QVERIFY(SyncLogic::isAssigned(h,QStringLiteral("Panel")));
}

void SynchronizerLogicTest::freeActivitiesLogic() {
    QStringList all{QStringLiteral("a"),QStringLiteral("b"),QStringLiteral("c")};
    QCOMPARE(SyncLogic::freeActivities(all,{}),all);
    AssignedHash h; h[QStringLiteral("a")]={QStringLiteral("L1")};
    QCOMPARE(SyncLogic::freeActivities(all,h),QStringList({QStringLiteral("b"),QStringLiteral("c")}));
}

void SynchronizerLogicTest::freeRunningActivitiesLogic() {
    QStringList run{QStringLiteral("a"),QStringLiteral("b")};
    AssignedHash h; h[QStringLiteral("a")]={QStringLiteral("L1")};
    QCOMPARE(SyncLogic::freeRunningActivities(run,h),QStringList{QStringLiteral("b")});
    QVERIFY(SyncLogic::freeRunningActivities({QStringLiteral("a")},h).isEmpty());
}

void SynchronizerLogicTest::validActivitiesIntersection() {
    QStringList all{QStringLiteral("a"),QStringLiteral("b"),QStringLiteral("c")};
    auto v=SyncLogic::validActivities(all,{QStringLiteral("a"),QStringLiteral("c"),QStringLiteral("d")});
    QCOMPARE(v.size(),2); QVERIFY(v.contains(QStringLiteral("a"))&&v.contains(QStringLiteral("c")));
    QVERIFY(SyncLogic::validActivities({QStringLiteral("x")},{QStringLiteral("y")}).isEmpty());
}

void SynchronizerLogicTest::singleVsMultipleMode() {
    QStringList n{QStringLiteral("D"),QStringLiteral("P"),QStringLiteral("S")};
    auto s=SyncLogic::centralLayoutNames(true,n); QCOMPARE(s.size(),1); QCOMPARE(s.at(0),QStringLiteral("D"));
    QCOMPARE(SyncLogic::centralLayoutNames(false,n),n);
}

void SynchronizerLogicTest::menuLayoutsSortedAndActivePrepended() {
    auto t=sampleTable();
    auto m=SyncLogic::menuLayouts(t,{QStringLiteral("NewActive")});
    QVERIFY(m.contains(QStringLiteral("NewActive")));
    QCOMPARE(m.size(),3);
    QVERIFY(m.at(0)<m.at(1)); // sorted
}

void SynchronizerLogicTest::menuExcludesHiddenLayouts() {
    auto m=SyncLogic::menuLayouts(sampleTable(),{QStringLiteral("MyDock")});
    QVERIFY(!m.contains(QStringLiteral("HiddenLayout")));
    QCOMPARE(m.count(QStringLiteral("MyDock")),1); // not duplicated
}

void SynchronizerLogicTest::lookupExistingAndNonExisting() {
    LayoutInfo o;
    QVERIFY(SyncLogic::lookupData(sampleTable(),QStringLiteral("MyDock"),o));
    QCOMPARE(o.name,QStringLiteral("MyDock"));
    QVERIFY(!SyncLogic::lookupData(sampleTable(),QStringLiteral("Nope"),o));
}

void SynchronizerLogicTest::idConflictDetection() {
    QVERIFY(SyncLogic::hasIdConflict({QStringLiteral("1"),QStringLiteral("5")},{QStringLiteral("3"),QStringLiteral("5")}));
    QVERIFY(!SyncLogic::hasIdConflict({QStringLiteral("1")},{QStringLiteral("2")}));
}

void SynchronizerLogicTest::typicalTwoActivitySetup() {
    QStringList acts{QStringLiteral("Desktop"),QStringLiteral("Coding")};
    AssignedHash h; h[QStringLiteral("Desktop")]={QStringLiteral("Dock")}; h[QStringLiteral("Coding")]={QStringLiteral("Sidebar")};
    QVERIFY(SyncLogic::isAssigned(h,QStringLiteral("Dock")));
    QVERIFY(SyncLogic::freeActivities(acts,h).isEmpty());
}

void SynchronizerLogicTest::sharedLayoutAcrossActivities() {
    QStringList acts{QStringLiteral("Work"),QStringLiteral("Play"),QStringLiteral("Sleep")};
    AssignedHash h; h[QStringLiteral("Work")]={QStringLiteral("Main")}; h[QStringLiteral("Play")]={QStringLiteral("Main")};
    QVERIFY(SyncLogic::isAssigned(h,QStringLiteral("Main")));
    QCOMPARE(SyncLogic::freeActivities(acts,h),QStringList{QStringLiteral("Sleep")});
    QCOMPARE(SyncLogic::freeRunningActivities(acts,h),QStringList{QStringLiteral("Sleep")});
}

QTEST_APPLESS_MAIN(SynchronizerLogicTest)
#include "synchronizerlogictest.moc"
