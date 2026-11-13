/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "../app/layout/containmentscreenresolver.h"

#include <QTest>

class ContainmentScreenResolverTest : public QObject
{
    Q_OBJECT

  private Q_SLOTS:
    void liveViewMappingReturnsValidIdWithoutFallback_data()
    {
        QTest::addColumn<int>("screenId");
        QTest::newRow("negative unknown/disconnected sentinel") << -5;
        QTest::newRow("zero connector ID") << 0;
        QTest::newRow("positive connector ID") << 17;
    }

    void pendingScreenWinsWithoutReadingLiveState()
    {
        int primaryReads = 0;
        int viewReads = 0;
        int fallbackReads = 0;

        const int result = Latte::Layout::ContainmentScreenResolver::resolve(
          Latte::Layout::PendingContainmentScreen{ false, -7 },
          true,
          [&] {
              ++primaryReads;
              return 4;
          },
          [&]() -> std::optional<int> {
              ++viewReads;
              return 8;
          },
          [&] {
              ++fallbackReads;
              return 9;
          });

        QCOMPARE(result, -7);
        QCOMPARE(primaryReads, 0);
        QCOMPARE(viewReads, 0);
        QCOMPARE(fallbackReads, 0);
    }

    void onPrimaryUsesActualPrimaryIdOnlyWhenCoronaExists()
    {
        int primaryReads = 0;
        int viewReads = 0;

        const int result = Latte::Layout::ContainmentScreenResolver::resolve(
          Latte::Layout::PendingContainmentScreen{ true, 12 },
          true,
          [&] {
              ++primaryReads;
              return 6;
          },
          [&]() -> std::optional<int> {
              ++viewReads;
              return 8;
          },
          [] { return 9; });

        QCOMPARE(result, 6);
        QCOMPARE(primaryReads, 1);
        QCOMPARE(viewReads, 0);

        const int withoutCorona = Latte::Layout::ContainmentScreenResolver::resolve(
          Latte::Layout::PendingContainmentScreen{ true, -2 },
          false,
          [&] {
              ++primaryReads;
              return 6;
          },
          [&]() -> std::optional<int> {
              ++viewReads;
              return 8;
          },
          [] { return 9; });

        QCOMPARE(withoutCorona, -2);
        QCOMPARE(primaryReads, 1);
        QCOMPARE(viewReads, 0);
    }

    void liveViewMappingReturnsValidIdWithoutFallback()
    {
        QFETCH(int, screenId);
        int primaryReads = 0;
        int viewReads = 0;
        int fallbackReads = 0;

        const int result = Latte::Layout::ContainmentScreenResolver::resolve(
          std::nullopt,
          true,
          [&] {
              ++primaryReads;
              return 3;
          },
          [&]() -> std::optional<int> {
              ++viewReads;
              return screenId;
          },
          [&] {
              ++fallbackReads;
              return 10;
          });

        QCOMPARE(result, screenId);
        QCOMPARE(primaryReads, 0);
        QCOMPARE(viewReads, 1);
        QCOMPARE(fallbackReads, 0);
    }

    void missingLiveMappingUsesPersistedScreen()
    {
        int viewReads = 0;
        int fallbackReads = 0;

        const int result = Latte::Layout::ContainmentScreenResolver::resolve(
          std::nullopt,
          false,
          [] { return 3; },
          [&]() -> std::optional<int> {
              ++viewReads;
              return std::nullopt;
          },
          [&] {
              ++fallbackReads;
              return -4;
          });

        QCOMPARE(result, -4);
        QCOMPARE(viewReads, 1);
        QCOMPARE(fallbackReads, 1);
    }
};

QTEST_GUILESS_MAIN(ContainmentScreenResolverTest)

#include "containmentscreenresolvertest.moc"
