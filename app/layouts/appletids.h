/*
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LATTEAPPLETIDS_H
#define LATTEAPPLETIDS_H

// Qt
#include <QList>
#include <QSet>
#include <QStringList>

// KDE
#include <KConfigGroup>

namespace Latte {
namespace Layouts {
namespace AppletIds {

//! Lowest id Latte hands out to applets (containments start at 12).
constexpr uint APPLETIDBASE = 40;

//! Entries of a containment's [General] group that list applet ids of that
//! containment, separated by ';'.
QStringList appletIdListEntries();

//! Every applet id below an "Applets" config group, including the applets an
//! applet hosts in its own "Applets" group. libplasma 6.8 stores the system
//! tray items that way instead of in a separate tray containment.
QStringList collect(const KConfigGroup &appletsGroup);

//! Renames the config groups of imported applets whose ids are already taken,
//! so that every applet in the corona keeps a unique id: Plasma allocates ids
//! from one counter for containments, applets and nested applets, and an
//! applet sharing an id with another one conflicts in id based lookups such as
//! the per-widget global shortcut ("activate widget <id>").
//!
//! containments: groups of the containments being imported, each with its
//! "Applets" group. usedIds: ids already taken outside of them. Top-level
//! applets are resolved before nested ones so that existing top-level ids and
//! the [General] id lists that reference them survive whenever possible.
//! Applet groups that are already unique are left untouched.
void makeUnique(const QList<KConfigGroup> &containments, QSet<uint> usedIds, uint base = APPLETIDBASE);

}
}
}

#endif
