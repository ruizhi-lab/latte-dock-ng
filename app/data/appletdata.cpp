/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "appletdata.h"

namespace Latte {
namespace Data {

Applet::Applet()
    : Generic()
{
}

bool Applet::operator==(const Applet &rhs) const
{
    return (id == rhs.id)
           && (name == rhs.name)
           && (description == rhs.description)
           && (icon == rhs.icon)
           && (isSelected == rhs.isSelected)
           && (storageId == rhs.storageId)
           && (subcontainmentId == rhs.subcontainmentId);
}

bool  Applet::operator!=(const Applet &rhs) const
{
    return !(*this == rhs);
}

bool Applet::isInstalled() const
{
    return isValid() && id != name;
}

bool Applet::isValid() const
{
    return !id.isEmpty();
}

QString Applet::visibleName() const
{
    return name.isEmpty() ? id : name;
}

}
}
