/*
    SPDX-FileCopyrightText: 2018 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.7
import QtQuick.Effects

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore

Item {
    MultiEffect {
        id: colorizer
        anchors.fill: parent
        colorization: 1.0
        colorizationColor: colorizerManager.applyColor
        source: wrapper
    }

    ///Shadow in applets
    Loader{
        id: colorizedAppletShadow
        anchors.fill: colorizer

        active: appletItem.environment.isGraphicsSystemAccelerated
                && plasmoid.configuration.appletShadowsEnabled
                && (appletColorizer.opacity>0)

        sourceComponent: MultiEffect{
            anchors.fill: parent
            shadowEnabled: true
            shadowColor: appletItem.myView.itemShadow.shadowColor
            source: colorizer
            shadowBlur: 0.5
            shadowVerticalOffset: forcedShadow ? 0 : 2

            readonly property bool forcedShadow: root.forceTransparentPanel
                                                 && plasmoid.configuration.appletShadowsEnabled
                                                 && !appletItem.communicator.indexerIsSupported ? true : false
        }
    }
}
