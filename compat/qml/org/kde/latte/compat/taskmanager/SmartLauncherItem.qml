/*
    SPDX-FileCopyrightText: 2026 Latte Dock Contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0

QtObject {
    // Lightweight fallback implementation used when Plasma's private
    // taskmanager module is not shipped by the distribution.
    property string launcherUrl: ""
    property bool countVisible: false
    property int count: 0
    property bool progressVisible: false
    property real progress: 0
}
