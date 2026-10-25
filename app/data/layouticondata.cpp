/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "layouticondata.h"


namespace Latte {
namespace Data {

LayoutIcon::LayoutIcon()
    : Generic()
{
}

bool LayoutIcon::isEmpty() const
{
    return (id.isEmpty() && name.isEmpty());
}

bool LayoutIcon::operator==(const LayoutIcon &rhs) const
{
    return (id == rhs.id)
           && (name == rhs.name)
           && (isBackgroundFile == rhs.isBackgroundFile);
}

bool LayoutIcon::operator!=(const LayoutIcon &rhs) const
{
    return !(*this == rhs);
}

}
}

