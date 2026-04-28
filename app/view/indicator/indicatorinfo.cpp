/*
    SPDX-FileCopyrightText: 2019 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "indicatorinfo.h"

// Qt
#include <QDebug>

// Plasma
#include <Plasma/Svg>

namespace Latte {
namespace ViewPart {
namespace IndicatorPart {

Info::Info(QObject *parent) :
    QObject(parent)
{
}

Info::~Info()
{
}

bool Info::needsIconColors() const
{
    return m_needsIconColors;
}

void Info::setNeedsIconColors(bool needs)
{
    if (m_needsIconColors == needs) {
        return;
    }

    m_needsIconColors = needs;
    Q_EMIT needsIconColorsChanged();
}

bool Info::needsMouseEventCoordinates() const
{
    return m_needsMouseEventCoordinates;
}

void Info::setNeedsMouseEventCoordinates(bool needs)
{
    if (m_needsMouseEventCoordinates == needs) {
        return;
    }

    m_needsMouseEventCoordinates = needs;
    Q_EMIT needsMouseEventCoordinatesChanged();
}

bool Info::providesClickedAnimation() const
{
    return m_providesClickedAnimation;
}

void Info::setProvidesClickedAnimation(bool provides)
{
    if (m_providesClickedAnimation == provides) {
        return;
    }

    m_providesClickedAnimation = provides;
    Q_EMIT providesClickedAnimationChanged();
}

bool Info::providesHoveredAnimation() const
{
    return m_providesHoveredAnimation;
}

void Info::setProvidesHoveredAnimation(bool provides)
{
    if (m_providesHoveredAnimation == provides) {
        return;
    }

    m_providesHoveredAnimation = provides;
    Q_EMIT providesHoveredAnimationChanged();
}

bool Info::providesInAttentionAnimation() const
{
    return m_providesInAttentionAnimation;
}

void Info::setProvidesInAttentionAnimation(bool provides)
{
    if (m_providesInAttentionAnimation == provides) {
        return;
    }

    m_providesInAttentionAnimation = provides;
    Q_EMIT providesInAttentionAnimationChanged();
}

bool Info::providesTaskLauncherAnimation() const
{
    return m_providesTaskLauncherAnimation;
}

void Info::setProvidesTaskLauncherAnimation(bool provides)
{
    if (m_providesTaskLauncherAnimation == provides) {
        return;
    }

    m_providesTaskLauncherAnimation = provides;
    Q_EMIT providesTaskLauncherAnimationChanged();
}

bool Info::providesGroupedWindowAddedAnimation() const
{
    return m_providesGroupedWindowAddedAnimation;
}

void Info::setProvidesGroupedWindowAddedAnimation(bool provides)
{
    if (m_providesGroupedWindowAddedAnimation == provides) {
        return;
    }

    m_providesGroupedWindowAddedAnimation = provides;
    Q_EMIT providesGroupedWindowAddedAnimationChanged();
}

bool Info::providesGroupedWindowRemovedAnimation() const
{
    return m_providesGroupedWindowRemovedAnimation;
}

void Info::setProvidesGroupedWindowRemovedAnimation(bool provides)
{
    if (m_providesGroupedWindowRemovedAnimation == provides) {
        return;
    }

    m_providesGroupedWindowRemovedAnimation = provides;
    Q_EMIT providesGroupedWindowRemovedAnimationChanged();
}

bool Info::providesFrontLayer() const
{
    return m_providesFrontLayer;
}

void Info::setProvidesFrontLayer(bool front)
{
    if (m_providesFrontLayer == front) {
        return;
    }

    m_providesFrontLayer = front;
    Q_EMIT providesFrontLayerChanged();
}

int Info::extraMaskThickness() const
{
    return m_extraMaskThickness;
}

void Info::setExtraMaskThickness(int thick)
{
    if (m_extraMaskThickness == thick) {
        return;
    }

    m_extraMaskThickness = thick;
    Q_EMIT extraMaskThicknessChanged();
}

float Info::minLengthPadding() const
{
    return m_minLengthPadding;
}

void Info::setMinLengthPadding(float padding)
{
    if (m_minLengthPadding == padding) {
        return;
    }

    m_minLengthPadding = padding;
    Q_EMIT minLengthPaddingChanged();
}

float Info::minThicknessPadding() const
{
    return m_minThicknessPadding;
}

void Info::setMinThicknessPadding(float padding)
{
    if (m_minThicknessPadding == padding) {
        return;
    }

    m_minThicknessPadding = padding;
    Q_EMIT minThicknessPaddingChanged();
}

}
}
}
