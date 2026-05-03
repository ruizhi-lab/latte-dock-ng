/*
    SPDX-FileCopyrightText: 2021 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.7

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore

import org.kde.latte.abilities.items 0.1 as AbilityItem

AbilityItem.IndicatorLevel {
    id: indicatorLevel
    anchors.bottom: (abilityItem.location === PlasmaCore.Types.BottomEdge) ? parent.bottom : undefined
    anchors.top: (abilityItem.location === PlasmaCore.Types.TopEdge) ? parent.top : undefined
    anchors.left: (abilityItem.location === PlasmaCore.Types.LeftEdge) ? parent.left : undefined
    anchors.right: (abilityItem.location === PlasmaCore.Types.RightEdge) ? parent.right : undefined

    anchors.horizontalCenter: abilityItem.isHorizontal ? parent.horizontalCenter : undefined
    anchors.verticalCenter: abilityItem.isHorizontal ? undefined : parent.verticalCenter

    width: abilityItem.isHorizontal ? length : thickness
    height: abilityItem.isHorizontal ? thickness : length

    level.isDrawn: level.indicator && level.indicator.host && level.indicator.host.isEnabled && !abilityItem.isSeparator && !abilityItem.isHidden

    readonly property var _metrics: (abilityItem.abilities && abilityItem.abilities.metrics) ? abilityItem.abilities.metrics : null
    readonly property real _metricsIconSize: (_metrics && _metrics.iconSize !== undefined) ? _metrics.iconSize : 0
    readonly property real _metricsLengthPaddings: (_metrics && _metrics.totals && _metrics.totals.lengthPaddings !== undefined) ? _metrics.totals.lengthPaddings : 0
    readonly property real _metricsThickness: (_metrics && _metrics.thickness !== undefined) ? _metrics.thickness : 1

    readonly property real length: abilityItem.preserveIndicatorInInitialPosition ?
                                       _metricsIconSize + _metricsLengthPaddings :
                                       ((abilityItem.parabolicItem && abilityItem.parabolicItem.length > 0)
                                            ? (abilityItem.parabolicItem.length - 2 * abilityItem.parabolicItem.zoom * ((_metrics && _metrics.margin && _metrics.margin.length !== undefined) ? _metrics.margin.length : 0))
                                            : (_metricsIconSize + _metricsLengthPaddings))
    readonly property real thickness: abilityItem.preserveIndicatorInInitialPosition ?
                                          _metricsThickness :
                                          ((abilityItem.parabolicItem && abilityItem.parabolicItem.thickness > 0)
                                               ? abilityItem.parabolicItem.thickness
                                               : _metricsThickness)

    Connections {
        target: abilityItem
        enabled: indicatorLevel.level.indicator.host ? indicatorLevel.level.indicator.host.info.needsMouseEventCoordinates : false
        function onMousePressed(x, y, button) {
            var fixedPos = indicatorLevel.mapFromItem(abilityItem, x, y);
            level.mousePressed(Math.round(fixedPos.x), Math.round(fixedPos.y), button);
        }
        function onMouseReleased(x, y, button) {
            var fixedPos = indicatorLevel.mapFromItem(abilityItem, x, y);
            level.mouseReleased(Math.round(fixedPos.x), Math.round(fixedPos.y), button);
        }
    }

    Connections {
        target: abilityItem
        function onTaskLauncherActivated() { level.taskLauncherActivated(); }
        function onTaskGroupedWindowAdded() { level.taskGroupedWindowAdded(); }
        function onTaskGroupedWindowRemoved() { level.taskGroupedWindowRemoved(); }
    }
}
