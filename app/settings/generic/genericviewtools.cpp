/*
    SPDX-FileCopyrightText: 2021 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "genericviewtools.h"

// local
#include "generictools.h"

// Qt
#include <QApplication>
#include <QDebug>
#include <QStyle>
#include <QTextDocument>

namespace Latte {

// Drawing constants (kIconMargin, kMargin, etc.) are defined in generictools.h

constexpr int kViewLineThickness = 4;
constexpr double kNonJustifyLengthRatio = 0.5;

void drawView(QPainter *painter, const QStyleOption &option, const Latte::Data::View &view, const QRect &availableScreenRect, const float brushOpacity)
{
    int thick = kViewLineThickness;
    painter->save();

    bool selected = Latte::isSelected(option);
    QPalette::ColorRole viewColorRole = !selected ? QPalette::Highlight : QPalette::Text;
    QPen pen; pen.setWidth(thick);
    QColor pencolor = option.palette.color(Latte::colorGroup(option), viewColorRole);
    pencolor.setAlphaF(brushOpacity);
    pen.setColor(pencolor);
    painter->setPen(pen);

    int x = availableScreenRect.x();
    int y = availableScreenRect.y();

    int length = view.isVertical() ? availableScreenRect.height() - thick + 1 : availableScreenRect.width() - thick + 1;
    int max_length = length;

    if (view.alignment != Latte::Types::Justify) {
        length = static_cast<int>(kNonJustifyLengthRatio * length);
    }

    //! provide even screen length
    if (length % 2 == 1) {
        length = qMin(max_length, length + 1);
    }

    int screen_edge = (view.screenEdgeMargin > 0) ? kMargin : 0;

    if (view.edge == Plasma::Types::TopEdge) {
        y = availableScreenRect.y() + thick / 2 + screen_edge;
    } else if (view.edge == Plasma::Types::BottomEdge) {
        y = availableScreenRect.y() + availableScreenRect.height() - 1 - screen_edge;
    } else if (view.edge == Plasma::Types::LeftEdge) {
        x = availableScreenRect.x() + thick / 2 + screen_edge;
    } else if (view.edge == Plasma::Types::RightEdge) {
        x = availableScreenRect.x() + availableScreenRect.width() - 1 - screen_edge;
    }

    if (view.isHorizontal()) {
        // Justify keeps the full length computed above but shares Left's start.
        if (view.alignment == Latte::Types::Left || view.alignment == Latte::Types::Justify) {
            x = availableScreenRect.x() + thick / 2;
        } else if (view.alignment == Latte::Types::Right) {
            x = availableScreenRect.x() + availableScreenRect.width() - length - 1;
        } else if (view.alignment == Latte::Types::Center) {
            int leftmargin = (availableScreenRect.width() / 2) - (length / 2);
            x = availableScreenRect.x() + leftmargin + 1;
        }
    } else if (view.isVertical()) {
        // Justify keeps the full length computed above but shares Top's start.
        if (view.alignment == Latte::Types::Top || view.alignment == Latte::Types::Justify) {
            y = availableScreenRect.y() + thick / 2;
        } else if (view.alignment == Latte::Types::Bottom) {
            y = availableScreenRect.y() + availableScreenRect.height() - length - 1;
        } else if (view.alignment == Latte::Types::Center) {
            int topmargin = (availableScreenRect.height() / 2) - (length / 2);
            y = availableScreenRect.y() + topmargin + 1;
        }
    }

    if (view.isHorizontal()) {
        painter->drawLine(x, y, x + length, y);
    } else if (view.isVertical()) {
        painter->drawLine(x, y, x, y + length);
    }

    painter->restore();
}

}
