/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#include <QtGlobal>

namespace Latte::Core::Geometry {

// Keep the QSize half-size integral before combining it with QQuickItem's qreal
// extent. Odd popup sizes use integer division, then the final qreal-to-int
// conversion truncates toward zero, including at negative global coordinates;
// promoting the popup size before division can move the popup by one pixel.
inline int centeredPopupOrigin(int parentOrigin, qreal parentExtent, int popupExtent)
{
    const int popupHalfExtent = popupExtent / 2;
    return static_cast<int>(parentOrigin + (parentExtent / 2) - popupHalfExtent);
}

} // namespace Latte::Core::Geometry
