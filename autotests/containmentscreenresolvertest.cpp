/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "../app/layout/containmentscreenresolver.h"
#include "../app/layout/initialviewscreenresolver.h"

#include <QTest>

class ContainmentScreenResolverTest : public QObject
{
    Q_OBJECT

  private Q_SLOTS:
    void primaryScreenIdIsNotAssumedToBeZero();
    void nestedContainmentWalksToTheOutermostOwner();
    void nestingWalkStopsAtTheDepthGuard();
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

    void startupFallbackPreservesExplicitScreenForReconnect()
    {
        constexpr int preferredScreenId = 11;
        constexpr int primaryScreenId = 12;

        const auto startup = Latte::Layout::InitialViewScreenResolver::resolve(preferredScreenId, false, primaryScreenId);
        QCOMPARE(startup.runtimeScreenId, primaryScreenId);
        QCOMPARE(startup.preferredScreenId, preferredScreenId);
        QVERIFY(startup.temporaryPrimaryFallback);

        const auto reconnected = Latte::Layout::InitialViewScreenResolver::resolve(startup.preferredScreenId, true, primaryScreenId);
        QCOMPARE(reconnected.runtimeScreenId, preferredScreenId);
        QCOMPARE(reconnected.preferredScreenId, preferredScreenId);
        QVERIFY(!reconnected.temporaryPrimaryFallback);
    }

};

//! A4's remaining primary-not-zero case. The populated fixture uses connector 0,
//! so a resolver that hard-codes a zero primary would look correct there. Here the
//! primary connector is 3 and the pending update must follow it, not 0.
void ContainmentScreenResolverTest::primaryScreenIdIsNotAssumedToBeZero()
{
    constexpr int primaryConnector = 3;
    int primaryReads{0};

    const int resolved = Latte::Layout::ContainmentScreenResolver::resolve(
        Latte::Layout::PendingContainmentScreen{ true, /*screen*/ 0 },
        true,
        [&primaryReads] {
            ++primaryReads;
            return primaryConnector;
        },
        [] () -> std::optional<int> {
            return std::optional<int>{ 0 };
        },
        [] { return 0; });

    QCOMPARE(resolved, primaryConnector);
    QCOMPARE(primaryReads, 1);

    // Without a Corona the pending screen is used as given, even when it is 0.
    const int withoutCorona = Latte::Layout::ContainmentScreenResolver::resolve(
        Latte::Layout::PendingContainmentScreen{ true, /*screen*/ 0 },
        false,
        [] { return 9; },
        [] () -> std::optional<int> { return std::nullopt; },
        [] { return -1; });
    QCOMPARE(withoutCorona, 0);
}

//! A4's remaining nested-containment case. A containment inside an applet must
//! resolve the outermost containment that owns a view; the walk must report the
//! top of a multi-level chain and stop at the first level without a parent.
void ContainmentScreenResolverTest::nestedContainmentWalksToTheOutermostOwner()
{
    // Stand-in graph: each node points at its parent, nullptr ends the chain.
    struct Node {
        int id;
        Node *parent;
    };

    Node main{ 1, nullptr };
    Node appletLevel{ 2, &main };
    Node nested{ 3, &appletLevel };

    auto parentOf = [](Node *node) -> Node * {
        return node ? node->parent : nullptr;
    };

    QCOMPARE(Latte::Layout::NestingContainmentResolver::outermost(&nested, parentOf), &main);
    QCOMPARE(Latte::Layout::NestingContainmentResolver::outermost(&appletLevel, parentOf), &main);
    QCOMPARE(Latte::Layout::NestingContainmentResolver::outermost(&main, parentOf), &main);
    QCOMPARE(Latte::Layout::NestingContainmentResolver::outermost(static_cast<Node *>(nullptr), parentOf), nullptr);
}

//! A malformed or cyclic parent graph must not recurse without bound. The walk is
//! capped, and a chain longer than the cap reports the node it stopped at rather
//! than looping; Corona turns that into -1.
void ContainmentScreenResolverTest::nestingWalkStopsAtTheDepthGuard()
{
    struct Node {
        int id;
        Node *parent;
    };

    // A cycle of two nodes.
    Node a{ 1, nullptr };
    Node b{ 2, &a };
    a.parent = &b;

    int visited{0};
    auto parentOf = [&visited](Node *node) -> Node * {
        ++visited;
        return node ? node->parent : nullptr;
    };

    Latte::Layout::NestingContainmentResolver::outermost(&a, parentOf);

    // Pinned to the literal cap, not to the constant: asserting
    // `visited == MAXDEPTH` would pass for any cap, including one so large that
    // the guard no longer protects the session.
    QCOMPARE(Latte::Layout::NestingContainmentResolver::MAXDEPTH, 16);
    QCOMPARE(visited, 16);

    // A chain shorter than the cap terminates on its own root.
    Node root{ 1, nullptr };
    Node child{ 2, &root };
    int shortVisits{0};
    auto shortParentOf = [&shortVisits](Node *node) -> Node * {
        ++shortVisits;
        return node ? node->parent : nullptr;
    };
    QCOMPARE(Latte::Layout::NestingContainmentResolver::outermost(&child, shortParentOf), &root);
    QVERIFY(!Latte::Layout::NestingContainmentResolver::exceedsDepth(&child, shortParentOf));
    QVERIFY(Latte::Layout::NestingContainmentResolver::exceedsDepth(&a, parentOf));
}

QTEST_GUILESS_MAIN(ContainmentScreenResolverTest)

#include "containmentscreenresolvertest.moc"
