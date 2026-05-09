/*
    SPDX-FileCopyrightText: 2019 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0
import QtQuick.Layouts 1.3

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.kirigami 2.0 as Kirigami

PlasmaComponents.Label {
    readonly property var theme: Kirigami.Theme
    readonly property var units: Kirigami.Units

    Layout.alignment: Qt.AlignLeft
    Layout.topMargin: units.smallSpacing
    Layout.bottomMargin: units.smallSpacing
    color: theme.textColor
    font.weight: Font.DemiBold
    font.letterSpacing: 1.05
    font.pixelSize: 1.2 * units.gridUnit
}
