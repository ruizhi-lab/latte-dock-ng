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
#include <QHash>
#include <QList>

namespace Latte {
namespace Data {

class LayoutsTable : public GenericTable<Layout>
{

public:
    LayoutsTable();

    //! Operators
    LayoutsTable subtracted(const LayoutsTable &rhs) const;

    //! Copy of the table in which each row whose id is a key of newIdForOldId gets the mapped id.
    //! Rows are matched by the ids they have before the call. Matching against ids that earlier
    //! renames already wrote would rewrite a row twice on a name chain (A->B, B->C) or a swap
    //! (A<->B), so all renames of one Apply must be applied from one snapshot like this.
    LayoutsTable withRenamedIds(const QHash<QString, QString> &newIdForOldId) const;

    void setLayoutForFreeActivities(const QString &id);
};

}
}

Q_DECLARE_METATYPE(Latte::Data::LayoutsTable)

#endif
