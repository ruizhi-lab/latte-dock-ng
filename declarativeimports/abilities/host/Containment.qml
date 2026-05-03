/*
    SPDX-FileCopyrightText: 2021 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.7

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore

import org.kde.latte.core 0.2 as LatteCore
import org.kde.latte.abilities.definition 0.1 as AbilityDefinition

AbilityDefinition.Containment {
    id: apis

    property int appletIndex: -1
    property Item myView: null

    alignment: {
        if (!myView) {
            return LatteCore.types.Center;
        }

        if (myView.alignment === LatteCore.types.Justify) {
            if (appletIndex>=0 && appletIndex<100) {
                return plasmoid.formFactor === PlasmaCore.Types.Horizontal ? LatteCore.types.Left : LatteCore.types.Top;
            } else if (appletIndex>=100 && appletIndex<200) {
                return LatteCore.types.Center;
            } else if (appletIndex>=200) {
                return plasmoid.formFactor === PlasmaCore.Types.Horizontal ? LatteCore.types.Right : LatteCore.types.Bottom;
            }

            return LatteCore.types.Center;
        }

        return myView.alignment;
    }

    readonly property Item publicApi: Item {
        readonly property alias isFirstAppletInContainment: apis.isFirstAppletInContainment
        readonly property alias isLastAppletInContainment: apis.isLastAppletInContainment

        readonly property alias alignment: apis.alignment
    }
}
