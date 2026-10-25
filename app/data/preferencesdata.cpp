/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "preferencesdata.h"

namespace Latte {
namespace Data {

const bool Preferences::BADGE3DSTYLE;
const bool Preferences::LAYOUTSINFORMATIONWINDOW;
const bool Preferences::AUTOSTART;
const bool Preferences::ISAVAILABLEGEOMETRYBROADCASTEDTOPLASMA;
const bool Preferences::METAHOLDFORBADGES;
const int Preferences::PARABOLICSPREAD;
const int Preferences::SCREENSDELAY;
const float Preferences::THICKNESSMARGININFLUENCE = 1.0f;

Preferences::Preferences()
{
}

bool Preferences::operator==(const Preferences &rhs) const
{
    return (badgeStyle3D == rhs.badgeStyle3D)
           && (layoutsInformationWindow == rhs.layoutsInformationWindow)
           && (autostart == rhs.autostart)
           && (contextMenuAlwaysActions == rhs.contextMenuAlwaysActions)
           && (isAvailableGeometryBroadcastedToPlasma == rhs.isAvailableGeometryBroadcastedToPlasma)
           && (metaHoldForBadges == rhs.metaHoldForBadges)
           && (parabolicSpread == rhs.parabolicSpread)
           && (thicknessMarginInfluence == rhs.thicknessMarginInfluence)
           && (screensDelay == rhs.screensDelay);
}

bool Preferences::operator!=(const Preferences &rhs) const
{
    return !(*this == rhs);
}

bool Preferences::inDefaultValues() const
{
    return (badgeStyle3D == BADGE3DSTYLE)
           && (layoutsInformationWindow == LAYOUTSINFORMATIONWINDOW)
           && (autostart == AUTOSTART)
           && (contextMenuAlwaysActions == Data::ContextMenu::ACTIONSALWAYSVISIBLE)
           && (isAvailableGeometryBroadcastedToPlasma == ISAVAILABLEGEOMETRYBROADCASTEDTOPLASMA)
           && (metaHoldForBadges == METAHOLDFORBADGES)
           && (parabolicSpread == PARABOLICSPREAD)
           && (thicknessMarginInfluence == THICKNESSMARGININFLUENCE)
           && (screensDelay == SCREENSDELAY);
}

void Preferences::setToDefaults()
{
    badgeStyle3D = BADGE3DSTYLE;
    layoutsInformationWindow = LAYOUTSINFORMATIONWINDOW;
    autostart = AUTOSTART;
    contextMenuAlwaysActions = Data::ContextMenu::ACTIONSALWAYSVISIBLE;
    isAvailableGeometryBroadcastedToPlasma = ISAVAILABLEGEOMETRYBROADCASTEDTOPLASMA;
    metaHoldForBadges = METAHOLDFORBADGES;
    parabolicSpread = PARABOLICSPREAD;
    thicknessMarginInfluence = THICKNESSMARGININFLUENCE;
    screensDelay = SCREENSDELAY;
}


}
}
