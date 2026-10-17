/*
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "appletids.h"

// C++
#include <utility>

// Qt
#include <QHash>

namespace Latte {
namespace Layouts {
namespace AppletIds {

namespace {

const QString APPLETSGROUP = QStringLiteral("Applets");
const QString GENERALGROUP = QStringLiteral("General");

class IdAllocator
{
public:
    IdAllocator(QSet<uint> reserved, uint base)
        : m_reserved(std::move(reserved))
        , m_next(base)
    {
    }

    QString take()
    {
        while (m_reserved.contains(m_next)) {
            ++m_next;
        }

        m_reserved.insert(m_next);
        return QString::number(m_next);
    }

private:
    QSet<uint> m_reserved;
    uint m_next;
};

bool toId(const QString &groupName, uint &id)
{
    bool ok{false};
    id = groupName.toUInt(&ok);
    return ok;
}

void renameGroup(KConfigGroup &parent, const QString &from, const QString &to)
{
    KConfigGroup source = parent.group(from);
    KConfigGroup destination = parent.group(to);
    source.copyTo(&destination);
    source.deleteGroup();
}

void updateIdLists(KConfigGroup host, const QHash<QString, QString> &renamed)
{
    if (renamed.isEmpty()) {
        return;
    }

    KConfigGroup general = host.group(GENERALGROUP);

    for (const auto &entry : appletIdListEntries()) {
        const QString value = general.readEntry(entry, QString());

        if (value.isEmpty()) {
            continue;
        }

        QStringList ids = value.split(QLatin1Char(';'));
        bool changed{false};

        for (auto &id : ids) {
            if (renamed.contains(id)) {
                id = renamed.value(id);
                changed = true;
            }
        }

        if (changed) {
            general.writeEntry(entry, ids.join(QLatin1Char(';')));
        }
    }
}

//! Renames the applets of host's "Applets" group that are taken and records
//! the final ids. Returns the renamed ones, old id to new id.
QHash<QString, QString> resolveApplets(KConfigGroup host, QSet<uint> &usedIds, IdAllocator &allocator)
{
    KConfigGroup applets = host.group(APPLETSGROUP);
    QHash<QString, QString> renamed;

    for (const auto &appletId : applets.groupList()) {
        uint id{0};

        if (!toId(appletId, id)) {
            continue;
        }

        if (usedIds.contains(id)) {
            const QString newId = allocator.take();
            renameGroup(applets, appletId, newId);
            renamed[appletId] = newId;
            id = newId.toUInt();
        }

        usedIds.insert(id);
    }

    updateIdLists(host, renamed);
    return renamed;
}

void resolveNestedApplets(KConfigGroup host, QSet<uint> &usedIds, IdAllocator &allocator)
{
    KConfigGroup applets = host.group(APPLETSGROUP);

    for (const auto &appletId : applets.groupList()) {
        KConfigGroup applet = applets.group(appletId);

        if (!applet.hasGroup(APPLETSGROUP)) {
            continue;
        }

        resolveApplets(applet, usedIds, allocator);
        resolveNestedApplets(applet, usedIds, allocator);
    }
}

}

QStringList appletIdListEntries()
{
    return {QStringLiteral("appletOrder"), QStringLiteral("lockedZoomApplets"), QStringLiteral("userBlocksColorizingApplets")};
}

QStringList collect(const KConfigGroup &appletsGroup)
{
    QStringList ids;

    for (const auto &appletId : appletsGroup.groupList()) {
        ids << appletId;
        ids << collect(appletsGroup.group(appletId).group(APPLETSGROUP));
    }

    return ids;
}

void makeUnique(const QList<KConfigGroup> &containments, QSet<uint> usedIds, uint base)
{
    //! A renamed applet must not land on an id that a not yet visited applet
    //! of the import already has, or copyTo() would merge the two groups.
    QSet<uint> reserved = usedIds;

    for (const auto &containment : containments) {
        for (const auto &appletId : collect(containment.group(APPLETSGROUP))) {
            uint id{0};

            if (toId(appletId, id)) {
                reserved.insert(id);
            }
        }
    }

    IdAllocator allocator(reserved, base);

    for (const auto &containment : containments) {
        resolveApplets(containment, usedIds, allocator);
    }

    for (const auto &containment : containments) {
        resolveNestedApplets(containment, usedIds, allocator);
    }
}

}
}
}
