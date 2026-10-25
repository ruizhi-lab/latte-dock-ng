/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "layoutcolordata.h"

namespace Latte {
namespace Data {

LayoutColor::LayoutColor()
    : Generic()
{
}

bool LayoutColor::operator==(const LayoutColor &rhs) const
{
    return (id == rhs.id)
           && (name == rhs.name)
           && (path == rhs.path)
           && (textColor == rhs.textColor);
}

bool LayoutColor::operator!=(const LayoutColor &rhs) const
{
    return !(*this == rhs);
}

void LayoutColor::setData(const QString &newid, const QString &newname, const QString &newpath, const QString &newtextcolor)
{
    id = newid;
    name = newname;
    path = newpath;
    textColor = newtextcolor;
}

}
}
