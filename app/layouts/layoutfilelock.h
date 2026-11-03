/*
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LATTELAYOUTFILELOCK_H
#define LATTELAYOUTFILELOCK_H

// Qt
#include <QString>

namespace Latte {
namespace Layouts {
namespace LayoutFileLock {

//! Read-only state of a layout file, addressed by its path. The file permissions
//! are the authoritative lock state (Storage::isWritable reads them too).
//! Paths are used instead of layout objects because a layout object keeps the path
//! it was loaded from: after a rename that path is gone, and a lock issued through
//! the object would silently do nothing. A missing file is left alone.
void lock(const QString &layoutFile);
void unlock(const QString &layoutFile);

}
}
}

#endif
