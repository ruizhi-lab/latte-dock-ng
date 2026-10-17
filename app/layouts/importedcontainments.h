/*
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LATTEIMPORTEDCONTAINMENTS_H
#define LATTEIMPORTEDCONTAINMENTS_H

// Qt
#include <QList>
#include <QPointer>

namespace Plasma {
class Containment;
class Corona;
}

namespace Latte {
namespace Layouts {
namespace ImportedContainments {

//! Initializes and restores, in the given order, containments that were
//! created delayed for an import and already have their configuration in the
//! corona, then returns the ones that still exist.
//!
//! Restoring one containment can delete another one of the same import:
//! since libplasma 6.8 the system tray applet of a restored dock migrates its
//! legacy tray containment and deletes it synchronously
//! (SystemTray::migrateFromSystrayContainer()). The import order is the string
//! order of the stored ids, so a tray "100" is restored before its dock "19"
//! and deleted afterwards, while a tray "20" is deleted before its own turn.
//! The QPointers are the authoritative record of which containments survive;
//! the result is built from them only after the last restore, because a raw
//! pointer collected earlier dangles and callers dereference every returned
//! containment.
QList<Plasma::Containment *> restore(Plasma::Corona *corona, const QList<QPointer<Plasma::Containment>> &containments);

}
}
}

#endif
