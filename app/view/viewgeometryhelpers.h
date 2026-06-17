/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef VIEWGEOMETRYHELPERS_H
#define VIEWGEOMETRYHELPERS_H

// local
#include <coretypes.h>

// Qt
#include <QList>
#include <QRect>

// Plasma
#include <Plasma/Plasma>

namespace Latte {
namespace ViewPart {

inline bool horizontalDockTouchesLeftLengthEdge(Types::Alignment alignment, float maxLength, float offset)
{
    if (alignment == Types::Justify) {
        return qFuzzyCompare(maxLength, 1.0f);
    }

    return alignment == Types::Left && qFuzzyIsNull(offset);
}

inline bool horizontalDockTouchesRightLengthEdge(Types::Alignment alignment, float maxLength, float offset)
{
    if (alignment == Types::Justify) {
        return qFuzzyCompare(maxLength, 1.0f);
    }

    return alignment == Types::Right && qFuzzyIsNull(offset);
}

inline bool verticalDockTouchesTopLengthEdge(Types::Alignment alignment, float maxLength, float offset)
{
    if (alignment == Types::Justify) {
        return qFuzzyCompare(maxLength, 1.0f);
    }

    return alignment == Types::Top && qFuzzyIsNull(offset);
}

inline bool verticalDockTouchesBottomLengthEdge(Types::Alignment alignment, float maxLength, float offset)
{
    if (alignment == Types::Justify) {
        return qFuzzyCompare(maxLength, 1.0f);
    }

    return alignment == Types::Bottom && qFuzzyIsNull(offset);
}

inline bool shouldRespectExternalPanelsForVerticalDock(Types::Alignment alignment, float maxLength, float offset)
{
    return verticalDockTouchesTopLengthEdge(alignment, maxLength, offset)
            || verticalDockTouchesBottomLengthEdge(alignment, maxLength, offset);
}

inline Plasma::Types::FormFactor dockFormFactorForLocation(Plasma::Types::Location location,
                                                           Plasma::Types::FormFactor fallback)
{
    switch (location) {
    case Plasma::Types::LeftEdge:
    case Plasma::Types::RightEdge:
        return Plasma::Types::Vertical;

    case Plasma::Types::TopEdge:
    case Plasma::Types::BottomEdge:
        return Plasma::Types::Horizontal;

    default:
        return fallback;
    }
}

inline QRect verticalDockExternalPanelGeometry(const QRect &screenGeometry, const QRect &availableGeometry)
{
    if (!screenGeometry.isValid() || !availableGeometry.isValid()) {
        return screenGeometry;
    }

    return QRect(screenGeometry.x(), availableGeometry.y(), screenGeometry.width(), availableGeometry.height());
}

inline QRect verticalDockExternalPanelGeometry(const QRect &screenGeometry, const QList<QRect> &panelGeometries)
{
    if (!screenGeometry.isValid()) {
        return screenGeometry;
    }

    QRect result = screenGeometry;

    for (const QRect &panelGeometry : panelGeometries) {
        if (!panelGeometry.isValid() || !screenGeometry.intersects(panelGeometry)) {
            continue;
        }

        const bool horizontalPanel = panelGeometry.width() > panelGeometry.height();
        if (!horizontalPanel) {
            continue;
        }

        if (panelGeometry.top() <= screenGeometry.top()) {
            result.setTop(qMax(result.top(), panelGeometry.bottom() + 1));
        }

        if (panelGeometry.bottom() >= screenGeometry.bottom()) {
            result.setBottom(qMin(result.bottom(), panelGeometry.top() - 1));
        }
    }

    return result;
}

inline QRect screenEdgePanelGeometry(const QRect &screenGeometry, Plasma::Types::Location location, int thickness)
{
    if (!screenGeometry.isValid() || thickness <= 0) {
        return QRect();
    }

    switch (location) {
    case Plasma::Types::TopEdge:
        return QRect(screenGeometry.x(), screenGeometry.y(), screenGeometry.width(), qMin(thickness, screenGeometry.height()));

    case Plasma::Types::BottomEdge:
        thickness = qMin(thickness, screenGeometry.height());
        return QRect(screenGeometry.x(), screenGeometry.bottom() - thickness + 1, screenGeometry.width(), thickness);

    case Plasma::Types::LeftEdge:
        return QRect(screenGeometry.x(), screenGeometry.y(), qMin(thickness, screenGeometry.width()), screenGeometry.height());

    case Plasma::Types::RightEdge:
        thickness = qMin(thickness, screenGeometry.width());
        return QRect(screenGeometry.right() - thickness + 1, screenGeometry.y(), thickness, screenGeometry.height());

    default:
        return QRect();
    }
}

}
}

#endif
