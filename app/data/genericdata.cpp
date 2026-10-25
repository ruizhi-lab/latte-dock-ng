/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "genericdata.h"

namespace Latte {
namespace Data {

Generic::Generic(const QString &newid, const QString &newname)
    : id(newid),
      name(newname)
{
}

bool Generic::operator==(const Generic &rhs) const
{
    return (id == rhs.id)
           && (name == rhs.name);
}

bool Generic::operator!=(const Generic &rhs) const
{
    return !(*this == rhs);
}

}
}
