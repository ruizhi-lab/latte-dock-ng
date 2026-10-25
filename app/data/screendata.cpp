/*
    SPDX-FileCopyrightText: 2021 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "screendata.h"

#include "../tools/commontools.h"

namespace Latte {
namespace Data {

Screen::Screen()
    : Generic(),
      hasExplicitViews(false),
      isActive(false),
      isRemovable(false),
      isSelected(false),
      geometry(QRect(0, 0, 1920, 1080))
{
}

Screen::Screen(const QString &screenId, const QString &serialized)
    : Screen()
{
    init(screenId, serialized);
}

bool Screen::operator==(const Screen &rhs) const
{
    return (id == rhs.id)
           && (name == rhs.name)
           && (hasExplicitViews == rhs.hasExplicitViews)
           //&& (isActive == rhs.isActive) /*Disabled because this is not a data but a screen state*/
           //&& (isRemovable == rhs.isRemovable) /*Disabled because this is not a data but a screen state*/
           && (isSelected == rhs.isSelected)
           && (geometry == rhs.geometry);
}

bool Screen::operator!=(const Screen &rhs) const
{
    return !(*this == rhs);
}

void Screen::init(const QString &screenId, const QString &serialized)
{
    QStringList parts = serialized.split(SERIALIZESPLITTER);

    id = screenId;
    name = parts[0];
    isActive = false;
    isRemovable = false;
    isSelected = false;

    if (parts.count() > 1) {
        geometry = Latte::stringToRect(parts[1]);
    }
}

bool Screen::isScreensGroup() const
{
    int sid = id.toInt();
    return ((sid == ONALLSCREENSID) || (sid == ONALLSECONDARYSCREENSID));
}

QString Screen::serialize() const
{
    QStringList result;
    result << name;
    result << Latte::rectToString(geometry);

    return result.join(SERIALIZESPLITTER);
}

}
}
