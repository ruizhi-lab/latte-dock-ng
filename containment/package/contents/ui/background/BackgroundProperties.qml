/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0

import "./types" as BackgroundTypes
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore

Item{
    property bool isShown: false

    property bool hasAllBorders: false
    property bool hasLeftBorder: false
    property bool hasRightBorder: false
    property bool hasTopBorder: false
    property bool hasBottomBorder: false

    readonly property bool edgeIsVertical: plasmoid.location === PlasmaCore.Types.LeftEdge
                                           || plasmoid.location === PlasmaCore.Types.RightEdge
    readonly property bool edgeIsHorizontal: plasmoid.location === PlasmaCore.Types.TopEdge
                                             || plasmoid.location === PlasmaCore.Types.BottomEdge
    readonly property bool isVertical: edgeIsVertical ? true
                                    : (edgeIsHorizontal ? false : (plasmoid.formFactor === PlasmaCore.Types.Vertical))

    readonly property bool hasBothLengthShadows:{
        if (isVertical) {
            return (hasTopBorder && hasBottomBorder);
        }

        return (hasLeftBorder && hasRightBorder);
    }

    property real offset: 0
    property real length: 0
    property int thickness: 0
    property int screenEdgeMargin: 0
    property real currentOpacity: 0

    readonly property BackgroundTypes.Shadows shadows: BackgroundTypes.Shadows{}
    readonly property BackgroundTypes.Paddings paddings: BackgroundTypes.Paddings{}
    readonly property BackgroundTypes.Totals totals: BackgroundTypes.Totals{}
}
