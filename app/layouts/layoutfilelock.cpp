/*
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "layoutfilelock.h"

// Qt
#include <QFile>
#include <QFileDevice>
#include <QFileInfo>

namespace Latte {
namespace Layouts {
namespace LayoutFileLock {

void lock(const QString &layoutFile)
{
    QFileInfo layoutFileInfo(layoutFile);

    if (layoutFileInfo.exists() && layoutFileInfo.isWritable()) {
        QFile(layoutFile).setPermissions(QFileDevice::ReadUser | QFileDevice::ReadGroup | QFileDevice::ReadOther);
    }
}

void unlock(const QString &layoutFile)
{
    QFileInfo layoutFileInfo(layoutFile);

    if (layoutFileInfo.exists() && !layoutFileInfo.isWritable()) {
        QFile(layoutFile).setPermissions(QFileDevice::ReadUser | QFileDevice::WriteUser | QFileDevice::ReadGroup | QFileDevice::ReadOther);
    }
}

}
}
}
