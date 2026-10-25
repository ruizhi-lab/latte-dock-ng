/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef UNIQUEIDINFODATA_H
#define UNIQUEIDINFODATA_H

//! Qt
#include <QMetaType>
#include <QString>

namespace Latte {
namespace Data {

class UniqueIdInfo
{
public:
    UniqueIdInfo();

    //! Unique Id nifo
    QString newId;
    QString newName;
    QString oldId;
    QString oldName;

    //! Operators
    bool operator==(const UniqueIdInfo &rhs) const;
    bool operator!=(const UniqueIdInfo &rhs) const;
};

}
}

Q_DECLARE_METATYPE(Latte::Data::UniqueIdInfo)

#endif
