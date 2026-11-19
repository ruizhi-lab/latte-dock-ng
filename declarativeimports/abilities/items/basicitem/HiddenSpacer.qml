/*
    SPDX-FileCopyrightText: 2016 Smith AR <audoban@openmailbox.org>
    SPDX-FileCopyrightText: 2016 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick

import org.kde.plasma.core as PlasmaCore
import org.kde.latte.core as LatteCore

Item{
    id: hiddenSpacer
    width: plasmoid.formFactor === PlasmaCore.Types.Vertical ? abilityItem.parabolicItem.width : nHiddenSize
    height: plasmoid.formFactor === PlasmaCore.Types.Vertical ? nHiddenSize : abilityItem.parabolicItem.height

    visible: (rightSpacer ? index === abilityItem.abilities.indexer.lastVisibleItemIndex : index === abilityItem.abilities.indexer.firstVisibleItemIndex)
             || (separatorSpace > 0)
             || abilityItem.isHiddenSpacerForcedShow

    property bool neighbourSeparator: rightSpacer ? abilityItem.headItemIsVisibleSeparator : abilityItem.tailItemIsVisibleSeparator
    property bool rightSpacer: false

    property int separatorSpace: neighbourSeparator && !abilityItem.isSeparator && abilityItem.abilities.parabolic.isEnabled
                                 && !(abilityItem.abilities.indexer.separators.length>0 && abilityItem.isSeparatorInRealLength) ?
                                     (LatteCore.Environment.separatorLength/2) : 0

    property real nScale: 0
    property real nHiddenSize: 0

    readonly property int maxSize: Math.max(0,Math.ceil(0.55*abilityItem.abilities.metrics.iconSize) - abilityItem.abilities.metrics.totals.lengthEdges)

    property real settledSeparatorSpace: 0
    readonly property bool retainingSeparatorSpace: itemIndex < 0 && abilityItem.keepSeparatorSpacingOnRemoval

    Binding {
        target: hiddenSpacer
        property: "settledSeparatorSpace"
        // Indexer publishes the neighbouring indexes in stages. Keep the
        // previous separator space through settling and through a row handoff.
        when: !retainingSeparatorSpace && (itemIndex < 0 || !hiddenSizeDelayer.running)
        restoreMode: Binding.RestoreNone
        value: itemIndex < 0 ? 0 : separatorSpace
    }

    Binding {
        target: hiddenSpacer
        property: "nHiddenSize"
        value: {
            if (abilityItem.isHidden || (itemIndex < 0 && !retainingSeparatorSpace)) {
                return 0;
            }
            if (itemIndex < 0) {
                return settledSeparatorSpace;
            }
            const scaleLength = abilityItem.parabolicItem.isUpdatingOnlySpacers
                              ? abilityItem.abilities.metrics.iconSize : maxSize;
            return settledSeparatorSpace + Math.max(0, nScale) * scaleLength;
        }
    }

    function inheritSeparatorSpace(previousSpacer) {
        hiddenSizeDelayer.restart();
        settledSeparatorSpace = previousSpacer.settledSeparatorSpace;
    }

    Connections{
        target: abilityItem
        function onContainsMouseChanged() {
            if (!abilityItem.containsMouse && !abilityItem.isHiddenSpacerForcedShow) {
                hiddenSpacer.nScale = 0;
            }
        }
    }

    Behavior on nHiddenSize {
        NumberAnimation{
            duration: abilityItem.isHiddenSpacerAnimated ? abilityItem.animationTime : 0
        }
    }

    Loader{
        active: abilityItem.abilities.debug.spacersEnabled

        sourceComponent: Rectangle{
            width: abilityItem.isHorizontal ? hiddenSpacer.width : 1
            height: abilityItem.isHorizontal ? 1 : hiddenSpacer.height
            x: abilityItem.isHorizontal ? 0 : hiddenSpacer.width/2
            y: abilityItem.isHorizontal ? hiddenSpacer.height/2 : 0

            border.width: 1
            border.color: "red"
            color: "transparent"
        }
    }

    //! Delayer — fire on the next idle frame (≈16ms) instead of the old 400ms
    //! to break binding loops without a visible position lag after drag reorder.
    onSeparatorSpaceChanged: {
        if (!hiddenSizeDelayer.running) {
            hiddenSizeDelayer.start();
        }
    }

    //! BUGLOCALREF: #1
    //! Timer that helps to avoid binding loops from head/tailItemIsVisibleSeparator by delaying to update the spacer size
    //! This solution needs confirmation. That specific binding loop is very nasty as it breaks the tasks model consistency
    //! somehow when switching between different activities and the model contains separators that become shown and hidden
    //! during the activity change
    Timer {
        id: hiddenSizeDelayer
        interval: 32
    }
}
