/*
    SPDX-FileCopyrightText: 2021 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <latte_debug.h>
#include "syncedlaunchers.h"

// local

// Qt
#include <QMetaMethod>
#include <QQuickItem>
#include <utility>

// Plasma
#include <Plasma/Applet>
#include <Plasma/Containment>


namespace Latte {
namespace Layouts {

SyncedLaunchers::SyncedLaunchers(QObject *parent)
    : QObject(parent)
{
    Q_UNUSED(parent)
}

SyncedLaunchers::~SyncedLaunchers()
{
}

void SyncedLaunchers::addAbilityClient(QQuickItem *client)
{
    if (!client) {
        return;
    }

    compactClients();

    for (const auto &clientPtr : std::as_const(m_clients)) {
        if (clientPtr == client) {
            return;
        }
    }

    m_clients << QPointer<QQuickItem>(client);

    connect(client, &QObject::destroyed, this, &SyncedLaunchers::removeClientObject);
}

void SyncedLaunchers::removeAbilityClient(QQuickItem *client)
{
    if (!client) {
        return;
    }

    for (int i = m_clients.count() - 1; i >= 0; --i) {
        if (!m_clients[i] || m_clients[i] == client) {
            m_clients.removeAt(i);
        }
    }

    disconnect(client, &QObject::destroyed, this, &SyncedLaunchers::removeClientObject);
}

void SyncedLaunchers::removeClientObject(QObject *obj)
{
    QQuickItem *item = qobject_cast<QQuickItem *>(obj);

    if (item) {
        removeAbilityClient(item);
    }
}

QQuickItem *SyncedLaunchers::client(const int &id)
{
    if (id <= 0) {
        return nullptr;
    }

    compactClients();

    for (const auto &clientPtr : std::as_const(m_clients)) {
        QQuickItem *client = clientPtr.data();

        if (!client) {
            continue;
        }

        int clientid = client->property("clientId").toInt();

        if (clientid == id) {
            return client;
        }
    }

    return nullptr;
}

QList<QQuickItem *> SyncedLaunchers::clients(QString layoutName, QString groupId)
{
    QList<QQuickItem *> items;

    compactClients();

    for (const auto &clientPtr : std::as_const(m_clients)) {
        QQuickItem *client = clientPtr.data();

        if (!client) {
            continue;
        }

        QString cLayoutName = layoutName.isEmpty() ? QString() : client->property("layoutName").toString();
        QString gid = client->property("syncedGroupId").toString();

        if (cLayoutName == layoutName && gid == groupId) {
            items << client;
        }
    }

    return items;
}

void SyncedLaunchers::compactClients()
{
    for (int i = m_clients.count() - 1; i >= 0; --i) {
        if (!m_clients[i]) {
            m_clients.removeAt(i);
        }
    }
}

QList<QQuickItem *> SyncedLaunchers::clients(QString layoutName, uint senderId, Latte::Types::LaunchersGroup launcherGroup, QString launcherGroupId)
{
    QList<QQuickItem *> temclients;

    if (launcherGroup == Types::UniqueLaunchers && launcherGroupId.isEmpty()) {
        //! on its own, single taskmanager
        auto c = client(senderId);

        if (c) {
            temclients << client(senderId);
        }
    } else {
        temclients << clients(layoutName, launcherGroupId);
    }

    return temclients;
}

void SyncedLaunchers::addLauncher(QString layoutName, uint senderId, int launcherGroup, QString launcherGroupId, QString launcher)
{
    Types::LaunchersGroup group = static_cast<Types::LaunchersGroup>(launcherGroup);
    QString lName = (group == Types::LayoutLaunchers) ? layoutName : QString();

    for (const auto matchingClients = clients(lName, senderId, group, launcherGroupId); const auto c : matchingClients) {
        if (auto *metaObject = c->metaObject()) {
            int methodIndex = metaObject->indexOfMethod("addSyncedLauncher(QVariant,QVariant)");

            if (methodIndex == -1) {
                qCDebug(latteLayout) << "Launchers Syncer Ability: addSyncedLauncher(QVariant,QVariant) was NOT found...";
                continue;
            }

            QMetaMethod method = metaObject->method(methodIndex);
            method.invoke(c, Q_ARG(QVariant, launcherGroup), Q_ARG(QVariant, launcher));
        }
    }
}

void SyncedLaunchers::removeLauncher(QString layoutName, uint senderId, int launcherGroup, QString launcherGroupId, QString launcher)
{
    Types::LaunchersGroup group = static_cast<Types::LaunchersGroup>(launcherGroup);
    QString lName = (group == Types::LayoutLaunchers) ? layoutName : QString();

    for (const auto matchingClients = clients(lName, senderId, group, launcherGroupId); const auto c : matchingClients) {
        if (auto *metaObject = c->metaObject()) {
            int methodIndex = metaObject->indexOfMethod("removeSyncedLauncher(QVariant,QVariant)");

            if (methodIndex == -1) {
                qCDebug(latteLayout) << "Launchers Syncer Ability: removeSyncedLauncher(QVariant,QVariant) was NOT found...";
                continue;
            }

            QMetaMethod method = metaObject->method(methodIndex);
            method.invoke(c, Q_ARG(QVariant, launcherGroup), Q_ARG(QVariant, launcher));
        }
    }
}

void SyncedLaunchers::addLauncherToActivity(QString layoutName, uint senderId, int launcherGroup, QString launcherGroupId, QString launcher, QString activity)
{
    Types::LaunchersGroup group = static_cast<Types::LaunchersGroup>(launcherGroup);
    QString lName = (group == Types::LayoutLaunchers) ? layoutName : QString();

    for (const auto matchingClients = clients(lName, senderId, group, launcherGroupId); const auto c : matchingClients) {
        if (auto *metaObject = c->metaObject()) {
            int methodIndex = metaObject->indexOfMethod("addSyncedLauncherToActivity(QVariant,QVariant,QVariant)");

            if (methodIndex == -1) {
                qCDebug(latteLayout) << "Launchers Syncer Ability: addSyncedLauncherToActivity(QVariant,QVariant,QVariant) was NOT found...";
                continue;
            }

            QMetaMethod method = metaObject->method(methodIndex);
            method.invoke(c, Q_ARG(QVariant, launcherGroup), Q_ARG(QVariant, launcher), Q_ARG(QVariant, activity));
        }
    }
}

void SyncedLaunchers::removeLauncherFromActivity(QString layoutName, uint senderId, int launcherGroup, QString launcherGroupId, QString launcher, QString activity)
{
    Types::LaunchersGroup group = static_cast<Types::LaunchersGroup>(launcherGroup);
    QString lName = (group == Types::LayoutLaunchers) ? layoutName : QString();

    for (const auto matchingClients = clients(lName, senderId, group, launcherGroupId); const auto c : matchingClients) {
        if (auto *metaObject = c->metaObject()) {
            int methodIndex = metaObject->indexOfMethod("removeSyncedLauncherFromActivity(QVariant,QVariant,QVariant)");

            if (methodIndex == -1) {
                qCDebug(latteLayout) << "Launchers Syncer Ability: removeSyncedLauncherFromActivity(QVariant,QVariant,QVariant) was NOT found...";
                continue;
            }

            QMetaMethod method = metaObject->method(methodIndex);
            method.invoke(c, Q_ARG(QVariant, launcherGroup), Q_ARG(QVariant, launcher), Q_ARG(QVariant, activity));
        }
    }
}

void SyncedLaunchers::urlsDropped(QString layoutName, uint senderId, int launcherGroup, QString launcherGroupId, QStringList urls)
{
    Types::LaunchersGroup group = static_cast<Types::LaunchersGroup>(launcherGroup);
    QString lName = (group == Types::LayoutLaunchers) ? layoutName : QString();

    for (const auto matchingClients = clients(lName, senderId, group, launcherGroupId); const auto c : matchingClients) {
        if (auto *metaObject = c->metaObject()) {
            int methodIndex = metaObject->indexOfMethod("dropSyncedUrls(QVariant,QVariant)");

            if (methodIndex == -1) {
                qCDebug(latteLayout) << "Launchers Syncer Ability: dropSyncedUrls(QVariant,QVariant) was NOT found...";
                continue;
            }

            QMetaMethod method = metaObject->method(methodIndex);
            method.invoke(c, Q_ARG(QVariant, launcherGroup), Q_ARG(QVariant, urls));
        }
    }
}

void SyncedLaunchers::validateLaunchersOrder(QString layoutName, uint senderId, int launcherGroup, QString launcherGroupId, QStringList launchers)
{
    Types::LaunchersGroup group = static_cast<Types::LaunchersGroup>(launcherGroup);
    QString lName = (group == Types::LayoutLaunchers) ? layoutName : QString();

    for (const auto matchingClients = clients(lName, senderId, group, launcherGroupId); const auto c : matchingClients) {
        auto tc = client(senderId);

        if (c != tc) {
            if (auto *metaObject = c->metaObject()) {
                int methodIndex = metaObject->indexOfMethod("validateSyncedLaunchersOrder(QVariant,QVariant)");

                if (methodIndex == -1) {
                    qCDebug(latteLayout) << "Launchers Syncer Ability: validateSyncedLaunchersOrder(QVariant,QVariant) was NOT found...";
                    continue;
                }

                QMetaMethod method = metaObject->method(methodIndex);
                method.invoke(c, Q_ARG(QVariant, launcherGroup), Q_ARG(QVariant, launchers));
            }
        }
    }
}

}
} //end of namespace
