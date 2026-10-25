/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LAYOUTSTABLEDATA_H
#define LAYOUTSTABLEDATA_H

// local
#include "generictable.h"
#include "layoutdata.h"

// Qt
#include <QList>

namespace Latte {
namespace Data {

class LayoutsTable : public GenericTable<Layout>
{

public:
    LayoutsTable();

    //! Operators
    LayoutsTable subtracted(const LayoutsTable &rhs) const;

    void setLayoutForFreeActivities(const QString &id);
};

}
}

Q_DECLARE_METATYPE(Latte::Data::LayoutsTable)

#endif
