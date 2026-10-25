/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "activitydata.h"

namespace Latte {
namespace Data {

Activity::Activity()
    : Generic()
{
}

bool Activity::isValid() const
{
    return (state != Activity::Invalid);
}

bool Activity::isRunning() const
{
    return ((state == Activity::Running) || (state == Activity::Starting));
}

}
}
