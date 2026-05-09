/*
    SPDX-FileCopyrightText: 2019 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.0 as Kirigami

QQC2.CheckBox {
    id: root
    Kirigami.Theme.inherit: true
    Kirigami.Theme.colorSet: Kirigami.Theme.Window

    property int value: 0
    property alias partiallyCheckedEnabled: root.tristate
    property alias checkedState: root.checkState

    // Custom string tooltip — required because many config pages do
    //   LatteComponents.CheckBox { tooltip: i18n("...") }
    // and Plasma 6 made "Cannot assign to non-existent property" fatal.
    property string tooltip: ""

    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
    QQC2.ToolTip.visible: hovered && tooltip !== ""
    QQC2.ToolTip.text: tooltip

    onValueChanged: {
        if (partiallyCheckedEnabled) {
            checkedState = value;
        } else {
            checked = !!value;
        }
    }
}
