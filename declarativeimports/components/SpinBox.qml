/*
    SPDX-FileCopyrightText: 2019 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.0 as Kirigami

QQC2.SpinBox {
    Kirigami.Theme.inherit: true

    implicitWidth: Kirigami.Units.gridUnit * 10
}
