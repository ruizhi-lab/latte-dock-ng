/*
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LATTE_APPLETINSERTION_H
#define LATTE_APPLETINSERTION_H

#include "../pluginids.h"

#include <QHash>
#include <QList>

namespace Latte::ViewPart {

inline int
widgetInsertionIndex(const QList<int> &order, const QHash<int, QString> &plugins, int requestedIndex)
{
    const auto isBoundary = [&plugins](int id) { return PluginId::isWidgetInsertionBoundary(plugins.value(id)); };

    int index = requestedIndex;
    if (index < 0) {
        index = 0;
        while (index < order.size() && !isBoundary(order.at(index))) {
            ++index;
        }
    }

    // Use the live order so a new widget cannot split an existing separator
    // from its task or tray boundary, on either side of the boundary applet.
    if (index > 0 && index < order.size()) {
        const int left = order.at(index - 1);
        const int right = order.at(index);
        if (isBoundary(right) && PluginId::isSeparatorPluginId(plugins.value(left))) {
            return index - 1;
        }
        if (isBoundary(left) && PluginId::isSeparatorPluginId(plugins.value(right))) {
            return index + 1;
        }
    }

    return index;
}

} // namespace Latte::ViewPart

#endif
