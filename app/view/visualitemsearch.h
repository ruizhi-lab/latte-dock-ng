/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef VISUALITEMSEARCH_H
#define VISUALITEMSEARCH_H

#include <QList>
#include <QObject>
#include <QQuickItem>
#include <QSet>
#include <QString>

namespace Latte {
namespace ViewPart {

//! Find a QQuickItem anywhere in the QObject/QQuickItem descendant tree.
inline QQuickItem *findQuickItemByObjectName(QObject *root, const QString &objectName)
{
    if (!root) {
        return nullptr;
    }

    QList<QObject *> pending{root};
    QSet<QObject *> visited;

    while (!pending.isEmpty()) {
        QObject *object = pending.takeLast();

        if (!object || visited.contains(object)) {
            continue;
        }

        visited.insert(object);
        pending.append(object->children());

        auto *item = qobject_cast<QQuickItem *>(object);

        if (!item) {
            continue;
        }

        if (item->objectName() == objectName) {
            return item;
        }

        for (const auto childItems = item->childItems(); QQuickItem *childItem : childItems) {
            pending.append(childItem);
        }
    }

    return nullptr;
}

}
}

#endif
