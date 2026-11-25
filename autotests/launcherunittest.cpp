/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "syncedlaunchers.h"

#include <coretypes.h>

#include <QQuickItem>
#include <QTest>

class LauncherClient : public QQuickItem
{
    Q_OBJECT

public:
    QList<QString> calls;
    QList<QVariantList> arguments;

    Q_INVOKABLE void addSyncedLauncher(QVariant group, QVariant launcher) {
        calls << QStringLiteral("add");
        arguments << QVariantList{group, launcher};
    }

    Q_INVOKABLE void addSyncedLauncherAtPosition(QVariant group, QVariant launcher, QVariant position) {
        calls << QStringLiteral("addAtPosition");
        arguments << QVariantList{group, launcher, position};
    }

    Q_INVOKABLE void removeSyncedLauncher(QVariant group, QVariant launcher) {
        calls << QStringLiteral("remove");
        arguments << QVariantList{group, launcher};
    }

    Q_INVOKABLE void addSyncedLauncherToActivity(QVariant group, QVariant launcher, QVariant activity) {
        calls << QStringLiteral("addToActivity");
        arguments << QVariantList{group, launcher, activity};
    }

    Q_INVOKABLE void removeSyncedLauncherFromActivity(QVariant group, QVariant launcher, QVariant activity) {
        calls << QStringLiteral("removeFromActivity");
        arguments << QVariantList{group, launcher, activity};
    }

    Q_INVOKABLE void dropSyncedUrls(QVariant group, QVariant urls) {
        calls << QStringLiteral("drop");
        arguments << QVariantList{group, urls};
    }

    Q_INVOKABLE void validateSyncedLaunchersOrder(QVariant group, QVariant launchers) {
        calls << QStringLiteral("validate");
        arguments << QVariantList{group, launchers};
    }
};

class LauncherUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void uniqueLaunchersTargetOnlySenderWhenGroupIdIsEmpty();
    void positionedLaunchersDispatchPosition();
    void groupLaunchersTargetMatchingGroupClients();
    void layoutLaunchersRestrictClientsToMatchingLayout();
    void destroyedClientsAreCompactedBeforeDispatch();
    void validateLaunchersOrderSkipsSender();
};

void LauncherUnitTest::uniqueLaunchersTargetOnlySenderWhenGroupIdIsEmpty()
{
    Latte::Layouts::SyncedLaunchers launchers(nullptr);
    LauncherClient sender;
    LauncherClient other;
    sender.setProperty("clientId", 1);
    other.setProperty("clientId", 2);

    launchers.addAbilityClient(&sender);
    launchers.addAbilityClient(&sender);
    launchers.addAbilityClient(&other);
    launchers.addLauncher(QStringLiteral("Layout"), 1, Latte::Types::UniqueLaunchers, QString(), QStringLiteral("app.desktop"));

    QCOMPARE(sender.calls, QList<QString> {QStringLiteral("add")});
    QCOMPARE(sender.arguments.constFirst().at(1).toString(), QStringLiteral("app.desktop"));
    QVERIFY(other.calls.isEmpty());
}

void LauncherUnitTest::positionedLaunchersDispatchPosition()
{
    Latte::Layouts::SyncedLaunchers launchers(nullptr);
    LauncherClient sender;
    LauncherClient receiver;
    sender.setProperty("clientId", 1);
    sender.setProperty("syncedGroupId", QStringLiteral("group"));
    receiver.setProperty("clientId", 2);
    receiver.setProperty("syncedGroupId", QStringLiteral("group"));

    launchers.addAbilityClient(&sender);
    launchers.addAbilityClient(&receiver);
    launchers.addLauncherAtPosition(QString(), 1, Latte::Types::UniqueLaunchers,
                                    QStringLiteral("group"), QStringLiteral("file:///latte-separator1.desktop"), 3);

    QCOMPARE(sender.calls, QList<QString>{QStringLiteral("addAtPosition")});
    QCOMPARE(receiver.calls, QList<QString>{QStringLiteral("addAtPosition")});
    QCOMPARE(sender.arguments.constFirst().at(2).toInt(), 3);
    QCOMPARE(receiver.arguments.constFirst().at(2).toInt(), 3);
}

void LauncherUnitTest::groupLaunchersTargetMatchingGroupClients()
{
    Latte::Layouts::SyncedLaunchers launchers(nullptr);
    LauncherClient first;
    LauncherClient second;
    LauncherClient unrelated;

    first.setProperty("clientId", 1);
    first.setProperty("layoutName", QStringLiteral("LayoutA"));
    first.setProperty("syncedGroupId", QStringLiteral("group"));
    second.setProperty("clientId", 2);
    second.setProperty("layoutName", QStringLiteral("LayoutB"));
    second.setProperty("syncedGroupId", QStringLiteral("group"));
    unrelated.setProperty("clientId", 3);
    unrelated.setProperty("syncedGroupId", QStringLiteral("other"));

    launchers.addAbilityClient(&first);
    launchers.addAbilityClient(&second);
    launchers.addAbilityClient(&unrelated);
    launchers.removeLauncher(QString(), 1, Latte::Types::UniqueLaunchers, QStringLiteral("group"), QStringLiteral("app.desktop"));

    QCOMPARE(first.calls, QList<QString> {QStringLiteral("remove")});
    QCOMPARE(second.calls, QList<QString> {QStringLiteral("remove")});
    QVERIFY(unrelated.calls.isEmpty());
}

void LauncherUnitTest::layoutLaunchersRestrictClientsToMatchingLayout()
{
    Latte::Layouts::SyncedLaunchers launchers(nullptr);
    LauncherClient sameLayout;
    LauncherClient otherLayout;
    sameLayout.setProperty("clientId", 1);
    sameLayout.setProperty("layoutName", QStringLiteral("LayoutA"));
    sameLayout.setProperty("syncedGroupId", QStringLiteral("shared"));
    otherLayout.setProperty("clientId", 2);
    otherLayout.setProperty("layoutName", QStringLiteral("LayoutB"));
    otherLayout.setProperty("syncedGroupId", QStringLiteral("shared"));

    launchers.addAbilityClient(&sameLayout);
    launchers.addAbilityClient(&otherLayout);
    launchers.addLauncherToActivity(QStringLiteral("LayoutA"),
                                    1,
                                    Latte::Types::LayoutLaunchers,
                                    QStringLiteral("shared"),
                                    QStringLiteral("app.desktop"),
                                    QStringLiteral("activity"));

    QCOMPARE(sameLayout.calls, QList<QString> {QStringLiteral("addToActivity")});
    QCOMPARE(sameLayout.arguments.constFirst().at(2).toString(), QStringLiteral("activity"));
    QVERIFY(otherLayout.calls.isEmpty());
}

void LauncherUnitTest::destroyedClientsAreCompactedBeforeDispatch()
{
    Latte::Layouts::SyncedLaunchers launchers(nullptr);
    auto *destroyed = new LauncherClient;
    LauncherClient alive;
    destroyed->setProperty("clientId", 1);
    destroyed->setProperty("syncedGroupId", QStringLiteral("group"));
    alive.setProperty("clientId", 2);
    alive.setProperty("syncedGroupId", QStringLiteral("group"));

    launchers.addAbilityClient(destroyed);
    launchers.addAbilityClient(&alive);
    delete destroyed;

    launchers.urlsDropped(QString(), 2, Latte::Types::UniqueLaunchers, QStringLiteral("group"), {QStringLiteral("file:///tmp/app.desktop")});

    QCOMPARE(alive.calls, QList<QString> {QStringLiteral("drop")});
    QCOMPARE(alive.arguments.constFirst().at(1).toStringList(), QStringList{QStringLiteral("file:///tmp/app.desktop")});
}

void LauncherUnitTest::validateLaunchersOrderSkipsSender()
{
    Latte::Layouts::SyncedLaunchers launchers(nullptr);
    LauncherClient sender;
    LauncherClient receiver;
    sender.setProperty("clientId", 1);
    sender.setProperty("syncedGroupId", QStringLiteral("group"));
    receiver.setProperty("clientId", 2);
    receiver.setProperty("syncedGroupId", QStringLiteral("group"));

    launchers.addAbilityClient(&sender);
    launchers.addAbilityClient(&receiver);
    launchers.validateLaunchersOrder(QString(), 1, Latte::Types::UniqueLaunchers, QStringLiteral("group"), {QStringLiteral("a"), QStringLiteral("b")});

    QVERIFY(sender.calls.isEmpty());
    QCOMPARE(receiver.calls, QList<QString> {QStringLiteral("validate")});
    QCOMPARE(receiver.arguments.constFirst().at(1).toStringList(), QStringList({QStringLiteral("a"), QStringLiteral("b")}));
}

QTEST_MAIN(LauncherUnitTest)

#include "launcherunittest.moc"
