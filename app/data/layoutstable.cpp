/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "layoutstable.h"

#include <QDebug>

namespace Latte {
namespace Data {

LayoutsTable::LayoutsTable()
    : GenericTable<Layout>()
{
}

//! Operators
LayoutsTable LayoutsTable::subtracted(const LayoutsTable &rhs) const
{
    LayoutsTable subtract;

    if ((*this) == rhs) {
        return subtract;
    }

    for (int i = 0; i < m_list.count(); ++i) {
        if (!rhs.containsId(m_list[i].id)) {
            subtract << m_list[i];
        }
    }

    return subtract;
}

LayoutsTable LayoutsTable::withRenamedIds(const QHash<QString, QString> &newIdForOldId) const
{
    LayoutsTable renamed(*this);

    //! The lookup key is the id in this table (before the call), and the result goes to
    //! renamed: a row written earlier in the loop is never matched again by a later row.
    for (int i = 0; i < m_list.count(); ++i) {
        const auto found = newIdForOldId.constFind(m_list[i].id);

        if (found != newIdForOldId.constEnd()) {
            renamed.m_list[i].id = found.value();
        }
    }

    return renamed;
}

void LayoutsTable::setLayoutForFreeActivities(const QString &id)
{
    int row = indexOf(id);


    if (row >= 0) {
        m_list[row].activities = QStringList(Data::Layout::FREEACTIVITIESID);
    }
}

}
}
