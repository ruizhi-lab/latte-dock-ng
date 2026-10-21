/*
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "../app/view/appletpreferences.h"
#include <QTest>

class ClonedViewPolicyTest : public QObject
{
    Q_OBJECT
  private Q_SLOTS:
    void partialMappingPreservesPreferences();
    void emptySourceClearsOnlyMappedPreferences();
    void lateMappingReplacesProvisionalPreferences();
};

void
ClonedViewPolicyTest::partialMappingPreservesPreferences()
{
    const QHash<int, int> mapping{ { 3, 45 } };
    QCOMPARE(Latte::ViewPart::syncAppletPreferences({ 3, 9 }, { 77 }, mapping, true), (QList<int>{ 45, 77 }));
    QCOMPARE(Latte::ViewPart::syncAppletPreferences({ 45, 77 }, { 9 }, mapping, false), (QList<int>{ 3, 9 }));
    QCOMPARE(Latte::ViewPart::syncAppletPreferences({ 3 }, { 77 }, {}, true), (QList<int>{ 77 }));
}

void
ClonedViewPolicyTest::emptySourceClearsOnlyMappedPreferences()
{
    const QHash<int, int> mapping{ { 3, 45 } };
    QCOMPARE(Latte::ViewPart::syncAppletPreferences({}, { 45, 77 }, mapping, true), (QList<int>{ 77 }));
    QCOMPARE(Latte::ViewPart::syncAppletPreferences({}, { 3, 9 }, mapping, false), (QList<int>{ 9 }));
    QVERIFY(Latte::ViewPart::syncAppletPreferences({}, { 45 }, mapping, true).isEmpty());
}

void
ClonedViewPolicyTest::lateMappingReplacesProvisionalPreferences()
{
    const QHash<int, int> partial{ { 3, 45 } };
    const QHash<int, int> complete{ { 3, 45 }, { 9, 77 } };
    const auto provisional = Latte::ViewPart::syncAppletPreferences({ 3 }, { 77 }, partial, true);
    QCOMPARE(Latte::ViewPart::syncAppletPreferences({ 3 }, provisional, complete, true), (QList<int>{ 45 }));
    QCOMPARE(Latte::ViewPart::syncAppletPreferences({ 3, 9 }, provisional, complete, true), (QList<int>{ 45, 77 }));
    QCOMPARE(Latte::ViewPart::syncAppletPreferences({ 3, 3 }, { 45, 45 }, complete, true), (QList<int>{ 45 }));
}

QTEST_GUILESS_MAIN(ClonedViewPolicyTest)
#include "clonedviewpolicytest.moc"
