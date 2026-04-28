/*
    SPDX-FileCopyrightText: 2019 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick.Controls 2.15 as QQC2

QQC2.CheckBox {
    id: root
    property int value: 0
    property alias partiallyCheckedEnabled: root.tristate
    property alias checkedState: root.checkState

    onValueChanged: {
        if (partiallyCheckedEnabled) {
            checkedState = value;
        } else {
            checked = !!value;
        }
    }
}
