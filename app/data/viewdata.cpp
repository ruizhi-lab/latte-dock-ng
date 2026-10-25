/*
    SPDX-FileCopyrightText: 2021 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "viewdata.h"

namespace Latte {
namespace Data {

const int View::ISCLONEDNULL = -1;

View::View()
    : Generic()
{
}

View::View(const QString &newid, const QString &newname)
    : Generic(newid, newname)
{
}

bool View::operator==(const View &rhs) const
{
    return (id == rhs.id)
           && (name == rhs.name)
           //&& (isActive == rhs.isActive) /*Disabled because this is not needed in order to track view changes for saving*/
           //&& (isMoveOrigin == rhs.isMoveOrigin) /*Disabled because this is not needed in order to track view changes for saving*/
           //&& (isMoveDestination == rhs.isMoveDestination) /*Disabled because this is not needed in order to track view changes for saving*/
           && (onPrimary == rhs.onPrimary)
           && (isClonedFrom == rhs.isClonedFrom)
           && (screen == rhs.screen)
           && (screenEdgeMargin == rhs.screenEdgeMargin)
           && (screensGroup == rhs.screensGroup)
           && (maxLength == rhs.maxLength)
           && (edge == rhs.edge)
           && (alignment == rhs.alignment)
           && (m_state == rhs.m_state)
           && (m_originFile == rhs.m_originFile)
           && (m_originLayout == rhs.m_originLayout)
           && (m_originView == rhs.m_originView)
           //&& (errors == rhs.errors) /*Disabled because this is not needed in order to track view changes for saving*/
           //&& (warnings == rhs.warnings) /*Disabled because this is not needed in order to track view changes for saving*/
           && (subcontainments == rhs.subcontainments);
}

bool View::operator!=(const View &rhs) const
{
    return !(*this == rhs);
}

View::operator QString() const
{
    QString result;

    result += id;
    result += QLatin1String(" : ");
    result += isActive ? QStringLiteral("Active") : QStringLiteral("Inactive");
    result += QLatin1String(" : ");

    if (m_state == OriginFromLayout && isMoveOrigin && isMoveDestination) {
        result += QStringLiteral(" ↑↓ ");
    } else if (m_state == OriginFromLayout && isMoveOrigin) {
        result += QStringLiteral(" ↑ ");
    } else if (m_state == OriginFromLayout && isMoveDestination) {
        result += QStringLiteral(" ↓ ");
    } else {
        result += QLatin1String(" - ");
    }

    result += QLatin1String(" : ");

    if (m_state == IsInvalid) {
        result += QLatin1String("IsInvalid");
    } else if (m_state == IsCreated) {
        result += QLatin1String("IsCreated");
    } else if (m_state == OriginFromViewTemplate) {
        result += QLatin1String("OriginFromViewTemplate");
    } else if (m_state == OriginFromLayout) {
        result += QLatin1String("OriginFromLayout");
    }

    result += QLatin1String(" : ");

    if (isCloned()) {
        result += QLatin1String("Cloned from:") + QString::number(isClonedFrom);
    } else {
        result += QLatin1String("Original");
    }

    result += QLatin1String(" : ");

    if (screensGroup == Latte::Types::SingleScreenGroup) {
        result += onPrimary ? QStringLiteral("Primary") : QStringLiteral("Explicit");
    } else if (screensGroup == Latte::Types::AllScreensGroup) {
        result += QLatin1String("All Screens");
    } else if (screensGroup == Latte::Types::AllSecondaryScreensGroup) {
        result += QLatin1String("All Secondary Screens");
    }

    result += onPrimary ? QStringLiteral("Primary") : QStringLiteral("Explicit");
    result += QLatin1String(" : ");
    result += QString::number(screen);
    result += QLatin1String(" : ");

    if (edge == Plasma::Types::BottomEdge) {
        result += QLatin1String("BottomEdge");
    } else if (edge == Plasma::Types::TopEdge) {
        result += QLatin1String("TopEdge");
    } else if (edge == Plasma::Types::LeftEdge) {
        result += QLatin1String("LeftEdge");
    } else if (edge == Plasma::Types::RightEdge) {
        result += QLatin1String("RightEdge");
    }

    result += QLatin1String(" : ");

    if (alignment == Latte::Types::Center) {
        result += QLatin1String("CenterAlignment");
    } else if (alignment == Latte::Types::Left) {
        result += QLatin1String("LeftAlignment");
    } else if (alignment == Latte::Types::Right) {
        result += QLatin1String("RightAlignment");
    } else if (alignment == Latte::Types::Top) {
        result += QLatin1String("TopAlignment");
    } else if (alignment == Latte::Types::Bottom) {
        result += QLatin1String("BottomAlignment");
    } else if (alignment == Latte::Types::Justify) {
        result += QLatin1String("JustifyAlignment");
    }

    result += QLatin1String(" : ");
    result += QString::number(maxLength) + QLatin1Char('%');

    result += QLatin1String(" || ");
    result += QStringLiteral("{") + QString(subcontainments) + QStringLiteral("}");

    return result;
}

bool View::isCreated() const
{
    return m_state == IsCreated;
}

bool View::isOriginal() const
{
    return !isCloned();
}

bool View::isCloned() const
{
    return isClonedFrom != ISCLONEDNULL;
}

bool View::isValid() const
{
    return m_state != IsInvalid;
}

bool View::isHorizontal() const
{
    return !isVertical();
}

bool View::isVertical() const
{
    return (edge == Plasma::Types::LeftEdge || edge == Plasma::Types::RightEdge);
}

bool View::hasViewTemplateOrigin() const
{
    return m_state == OriginFromViewTemplate;
}

bool View::hasLayoutOrigin() const
{
    return m_state == OriginFromLayout;
}

bool View::hasSubContainment(const QString &subId) const
{
    return subcontainments.containsId(subId);
}

bool View::hasErrors() const
{
    return errors > 0;
}

bool View::hasWarnings() const
{
    return warnings > 0;
}

QString View::originFile() const
{
    return m_originFile;
}

QString View::originLayout() const
{
    return m_originLayout;
}

QString View::originView() const
{
    return m_originView;
}

View::State View::state() const
{
    return m_state;
}

void View::setState(View::State state, QString file, QString layout, QString view)
{
    m_state = state;
    m_originFile = file;
    m_originLayout = layout;
    m_originView = view;
}

}
}
