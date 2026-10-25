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

void LayoutsTable::setLayoutForFreeActivities(const QString &id)
{
    int row = indexOf(id);


    if (row >= 0) {
        m_list[row].activities = QStringList(Data::Layout::FREEACTIVITIESID);
    }
}

}
}
