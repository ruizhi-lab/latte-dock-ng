/*
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "layoutfilelock.h"

// Qt
#include <QFile>
#include <QFileDevice>
#include <QFileInfo>

#include <sys/stat.h>

namespace Latte {
namespace Layouts {
namespace LayoutFileLock {

namespace {

//! The owner write bit is the authoritative lock state. QFileInfo::isWritable()
//! uses access(2), which reports a read-only file as writable to root, and Qt
//! 6.11 also reports effective permissions in permissions(), so neither can
//! drive lock()/unlock() when Latte runs as root. Read the mode bits instead.
bool hasOwnerWriteBit(const QString &layoutFile)
{
    struct stat info {};
    return ::stat(QFile::encodeName(layoutFile).constData(), &info) == 0 && (info.st_mode & S_IWUSR) != 0;
}

}

void lock(const QString &layoutFile)
{
    QFileInfo layoutFileInfo(layoutFile);

    if (layoutFileInfo.exists() && hasOwnerWriteBit(layoutFile)) {
        QFile(layoutFile).setPermissions(QFileDevice::ReadUser | QFileDevice::ReadGroup | QFileDevice::ReadOther);
    }
}

void unlock(const QString &layoutFile)
{
    QFileInfo layoutFileInfo(layoutFile);

    if (layoutFileInfo.exists() && !hasOwnerWriteBit(layoutFile)) {
        QFile(layoutFile).setPermissions(QFileDevice::ReadUser | QFileDevice::WriteUser | QFileDevice::ReadGroup | QFileDevice::ReadOther);
    }
}

}
}
}
