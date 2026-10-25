/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "uniqueidinfo.h"

namespace Latte {
namespace Data {

UniqueIdInfo::UniqueIdInfo()
{
}

bool UniqueIdInfo::operator==(const UniqueIdInfo &rhs) const
{
    return (newId == rhs.newId)
           && (newName == rhs.newName)
           && (oldId == rhs.oldId)
           && (oldName == rhs.oldName);
}

bool UniqueIdInfo::operator!=(const UniqueIdInfo &rhs) const
{
    return !(*this == rhs);
}

}
}
