/*
    SPDX-FileCopyrightText: 2019 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0
import QtQuick.Layouts 1.3

import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.kirigami 2.0 as Kirigami

PlasmaComponents.Label {
    readonly property var units: Kirigami.Units

    Layout.fillWidth: true
    Layout.topMargin: isFirstSubCategory ? 0 : units.smallSpacing * 2
    Layout.bottomMargin: units.smallSpacing
    horizontalAlignment: Text.AlignHCenter
    opacity: 0.4

    property bool isFirstSubCategory: false
}
