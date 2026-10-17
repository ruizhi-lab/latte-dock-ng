/*
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "importedcontainments.h"

// KDE
#include <KConfigGroup>

// Plasma
#include <Plasma/Applet>
#include <Plasma/Containment>
#include <Plasma/Corona>

namespace Latte {
namespace Layouts {
namespace ImportedContainments {

QList<Plasma::Containment *> restore(Plasma::Corona *corona, const QList<QPointer<Plasma::Containment>> &containments)
{
    QList<Plasma::Containment *> restored;

    if (!corona) {
        return restored;
    }

    for (const auto &entry : containments) {
        Plasma::Containment *containment = entry;

        //! Deleted by the restore of a containment before it in this loop.
        if (!containment) {
            continue;
        }

        containment->init();
        KConfigGroup cg = containment->config();
        containment->restore(cg);
        containment->updateConstraints(Plasma::Applet::StartupCompletedConstraint);
        containment->save(cg);
        corona->requestConfigSync();
        containment->flushPendingConstraintsEvents();
        Q_EMIT corona->containmentAdded(containment);
    }

    //! Collected only now: a containment restored above can still be deleted
    //! by the restore of a later one, see the header.
    for (const auto &entry : containments) {
        if (entry) {
            restored << entry.data();
        }
    }

    return restored;
}

}
}
}
