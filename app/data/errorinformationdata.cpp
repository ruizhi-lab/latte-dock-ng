/*
    SPDX-FileCopyrightText: 2021 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "errorinformationdata.h"

namespace Latte {
namespace Data {

ErrorInformation::ErrorInformation()
    : Generic()
{
}

bool ErrorInformation::operator==(const ErrorInformation &rhs) const
{
    return (id == rhs.id)
           && (name == rhs.name)
           && (containment == rhs.containment)
           && (applet == rhs.applet);
}

bool ErrorInformation::operator!=(const ErrorInformation &rhs) const
{
    return !(*this == rhs);
}

bool ErrorInformation::isValid() const
{
    return containment.isValid() || applet.isValid();
}

}
}
