/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "layoutdata.h"

// Qt
#include <QDir>

namespace Latte {
namespace Data {

Layout::Layout()
    : Generic()
{
}

Layout::Layout(Layout &&o)
    : Generic(o),
      icon(o.icon),
      textColor(o.textColor),
      lastUsedActivity(o.lastUsedActivity),
      isActive(o.isActive),
      isConsideredActive(o.isConsideredActive),
      isLocked(o.isLocked),
      isShownInMenu(o.isShownInMenu),
      isTemplate(o.isTemplate),
      popUpMargin(o.popUpMargin),
      schemeFile(o.schemeFile),
      activities(o.activities),
      errors(o.errors),
      warnings(o.warnings),
      views(o.views)
{
}

Layout::Layout(const Layout &o)
    : Generic(o),
      icon(o.icon),
      textColor(o.textColor),
      lastUsedActivity(o.lastUsedActivity),
      isActive(o.isActive),
      isConsideredActive(o.isConsideredActive),
      isLocked(o.isLocked),
      isShownInMenu(o.isShownInMenu),
      isTemplate(o.isTemplate),
      popUpMargin(o.popUpMargin),
      schemeFile(o.schemeFile),
      activities(o.activities),
      errors(o.errors),
      warnings(o.warnings),
      views(o.views)
{
}

Layout &Layout::operator=(Layout &&rhs)
{
    id = rhs.id;
    name = rhs.name;
    icon = rhs.icon;
    textColor = rhs.textColor;
    lastUsedActivity = rhs.lastUsedActivity;
    isActive = rhs.isActive;
    isConsideredActive = rhs.isConsideredActive;
    isLocked = rhs.isLocked;
    isShownInMenu = rhs.isShownInMenu;
    isTemplate = rhs.isTemplate;
    popUpMargin = rhs.popUpMargin;
    schemeFile = rhs.schemeFile;
    activities = rhs.activities;
    errors = rhs.errors;
    warnings = rhs.warnings;
    views = rhs.views;

    return (*this);
}

Layout &Layout::operator=(const Layout &rhs)
{
    id = rhs.id;
    name = rhs.name;
    icon = rhs.icon;
    textColor = rhs.textColor;
    lastUsedActivity = rhs.lastUsedActivity;
    isActive = rhs.isActive;
    isConsideredActive = rhs.isConsideredActive;
    isLocked = rhs.isLocked;
    isShownInMenu = rhs.isShownInMenu;
    isTemplate = rhs.isTemplate;
    popUpMargin = rhs.popUpMargin;
    schemeFile = rhs.schemeFile;
    activities = rhs.activities;
    errors = rhs.errors;
    warnings = rhs.warnings;
    views = rhs.views;

    return (*this);
}

bool Layout::operator==(const Layout &rhs) const
{
    return (id == rhs.id)
            && (name == rhs.name)
            && (icon == rhs.icon)
            && (textColor == rhs.textColor)
            //&& (lastUsedActivity == rhs.lastUsedActivity) /*Disabled because this is not needed in order to track layout changes for saving*/
            //&& (isActive == rhs.isActive) /*Disabled because this is not needed in order to track layout changes for saving*/
            //&& (isConsideredActive == rhs.isConsideredActive) /*Disabled because this is not needed in order to track layout changes for saving*/
            && (isLocked == rhs.isLocked)
            && (isShownInMenu == rhs.isShownInMenu)
            && (isTemplate == rhs.isTemplate)
            && (popUpMargin == rhs.popUpMargin)
            && (schemeFile == rhs.schemeFile)
            && (activities == rhs.activities)
            //&& (errors == rhs.errors) /*Disabled because this is not needed in order to track layout changes for saving*/
            //&& (warnings == rhs.warnings) /*Disabled because this is not needed in order to track layout changes for saving*/
            && (views == rhs.views);
}

bool Layout::operator!=(const Layout &rhs) const
{
    return !(*this == rhs);
}

bool Layout::isOnAllActivities() const
{
    return ((activities.count() == 1) && (activities[0] == ALLACTIVITIESID));
}

bool Layout::isForFreeActivities() const
{
    return ((activities.count() == 1) && (activities[0] == FREEACTIVITIESID));
}

bool Layout::isTemporary() const
{
    return id.startsWith("/tmp");
}

bool Layout::isEmpty() const
{
    return isNull();
}

bool Layout::isNull() const
{
    return (id.isEmpty() && name.isEmpty());
}

bool Layout::isSystemTemplate() const
{
    return isTemplate && !id.startsWith(QDir::tempPath()) && !id.startsWith(QDir::homePath());
}

bool Layout::hasErrors() const
{
    return errors > 0;
}

bool Layout::hasWarnings() const
{
    return warnings > 0;
}

}
}

