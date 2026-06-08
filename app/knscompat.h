/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KNSCOMPAT_H
#define KNSCOMPAT_H

//! Set up user-local QML module overrides to fix the KNS download dialog.
//! Qt 6.10.3 removed DragHandler.xAxis.onActiveValueChanged, which breaks
//! Kirigami's DrawerHandle.qml.  The system qmldir files use `prefer` to
//! load incompatible AOT-compiled versions.  We create user-local module
//! overrides without `prefer` and a patched DrawerHandle.qml so that the
//! KNSWidgets::Dialog (opened by "Download New Plasma Widgets") renders
//! correctly, matching the official Plasma panel behavior.
void ensureKnsCompat();

#endif // KNSCOMPAT_H
