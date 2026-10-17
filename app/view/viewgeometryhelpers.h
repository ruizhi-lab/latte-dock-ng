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
#include <QMargins>
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
    if (qFuzzyCompare(maxLength, 1.0f)) {
        return true;
    }

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

inline Types::Alignment dockAlignmentForLocation(Plasma::Types::Location location, Types::Alignment alignment)
{
    switch (location) {
        case Plasma::Types::LeftEdge:
        case Plasma::Types::RightEdge:
            if (alignment == Types::Left) {
                return Types::Top;
            }

            if (alignment == Types::Right) {
                return Types::Bottom;
            }

            return alignment;

        case Plasma::Types::TopEdge:
        case Plasma::Types::BottomEdge:
            if (alignment == Types::Top) {
                return Types::Left;
            }

            if (alignment == Types::Bottom) {
                return Types::Right;
            }

            return alignment;

        default:
            return alignment;
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

        if (horizontalPanel) {
            // Horizontal panels (top/bottom edges) reduce available vertical space.
            if (panelGeometry.top() <= screenGeometry.top()) {
                result.setTop(qMax(result.top(), panelGeometry.bottom() + 1));
            }

            if (panelGeometry.bottom() >= screenGeometry.bottom()) {
                result.setBottom(qMin(result.bottom(), panelGeometry.top() - 1));
            }
        } else {
            // Vertical panels (left/right edges) reduce available horizontal space.
            if (panelGeometry.left() <= screenGeometry.left()) {
                result.setLeft(qMax(result.left(), panelGeometry.right() + 1));
            }

            if (panelGeometry.right() >= screenGeometry.right()) {
                result.setRight(qMin(result.right(), panelGeometry.left() - 1));
            }
        }
    }

    return result;
}

// Returns content-only margins for horizontal Plasma panels. The Latte
// window remains full-screen so its background continues beneath the panel.
inline QMargins verticalDockExternalPanelMargins(const QRect &screenGeometry, const QList<QRect> &panelGeometries)
{
    if (!screenGeometry.isValid()) {
        return {};
    }

    int top = 0;
    int bottom = 0;

    for (const QRect &panelGeometry : panelGeometries) {
        if (!panelGeometry.isValid() || !screenGeometry.intersects(panelGeometry)
            || panelGeometry.width() <= panelGeometry.height()) {
            continue;
        }

        // Classify floating panels by the nearest screen edge and retain their
        // edge gap in the content margin.
        if (panelGeometry.center().y() <= screenGeometry.center().y()) {
            top = qMax(top, qMax(0, panelGeometry.bottom() - screenGeometry.top() + 1));
        } else {
            bottom = qMax(bottom, qMax(0, screenGeometry.bottom() - panelGeometry.top() + 1));
        }
    }

    return QMargins(0, top, 0, bottom);
}

inline QRect
configViewAvailableGeometry(const QRect &screen, const QRect &available, const QList<QRect> &panels, const QRect &canvas, Plasma::Types::Location edge)
{
    if (!screen.isValid() || !available.isValid()) {
        return {};
    }

    // KWin's live panel rectangles include floating gaps; QScreen's work area
    // alone may cover the entire output on Wayland. Reserve these edges and
    // the edited dock's canvas, rather than letting its settings cover them.
    const auto transpose = [](const QRect &rect) { return QRect(rect.y(), rect.x(), rect.height(), rect.width()); };
    QList<QRect> ownedPanels;
    QList<QRect> transposedPanels;
    for (const QRect &panel : panels) {
        if (panel.isValid() && screen.contains(panel.center())) {
            ownedPanels.append(panel);
            transposedPanels.append(transpose(panel));
        }
    }
    const QMargins vertical = verticalDockExternalPanelMargins(screen, ownedPanels);
    const QMargins horizontal = verticalDockExternalPanelMargins(transpose(screen), transposedPanels);
    QRect result = available.intersected(screen.marginsRemoved(QMargins(horizontal.top(), vertical.top(), horizontal.bottom(), vertical.bottom())));

    if (canvas.isValid() && screen.intersects(canvas)) {
        switch (edge) {
            case Plasma::Types::TopEdge:
                result.setTop(qMax(result.top(), canvas.bottom() + 1));
                break;
            case Plasma::Types::BottomEdge:
                result.setBottom(qMin(result.bottom(), canvas.top() - 1));
                break;
            case Plasma::Types::LeftEdge:
                result.setLeft(qMax(result.left(), canvas.right() + 1));
                break;
            case Plasma::Types::RightEdge:
                result.setRight(qMin(result.right(), canvas.left() - 1));
                break;
            default:
                break;
        }
    }
    return result.isValid() ? result : QRect();
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

inline QMargins externalPanelContentMargins(const QRect &screen, const QRect &dockStrip,
        const QList<QRect> &panels, bool vertical)
{
    auto transpose = [](const QRect &r) { return QRect(r.y(), r.x(), r.height(), r.width()); };
    QList<QRect> overlaps;
    for (const QRect &panel : panels) {
        // A neighbouring output's panel, or a short floating panel that does
        // not reach this dock, must not move this view's applets.
        if (panel.isValid() && screen.contains(panel.center()) && dockStrip.intersects(panel)) {
            overlaps.append(vertical ? panel : transpose(panel));
        }
    }
    const QMargins margins = verticalDockExternalPanelMargins(vertical ? screen : transpose(screen), overlaps);
    return vertical ? margins : QMargins(margins.top(), 0, margins.bottom(), 0);
}

}
}

#endif
