/*
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LATTEPLASMAAPICOMPAT_H
#define LATTEPLASMAAPICOMPAT_H

// Qt
#include <QtGlobal>

// C++
#include <limits>

// Plasma
#include <Plasma/Containment>
#include <Plasma/version.h>

//! libplasma 6.8 (SONAME libPlasma.so.8) is one coordinated API/ABI break:
//! screen ids became unsigned, Corona::screenForContainment() and
//! Containment::lastScreen()/reactToScreenChange() were removed, and
//! Containment::screen() became a stored value that the owner of the view
//! must publish through Containment::setScreen(). Before 6.8 Plasma pulled the
//! screen from Corona::screenForContainment() whenever it needed it. A
//! version check is used because these changes only exist together; remove
//! the pre-6.8 branches once PLASMA_MIN_VERSION reaches 6.8.
#if PLASMA_VERSION >= QT_VERSION_CHECK(6, 8, 0)
#define LATTE_PLASMA_STORED_CONTAINMENT_SCREEN
#endif

namespace Latte {

//! Screen id type of the Plasma::Corona virtual screen API.
#ifdef LATTE_PLASMA_STORED_CONTAINMENT_SCREEN
using PlasmaScreenId = uint;
#else
using PlasmaScreenId = int;
#endif

//! Last screen Plasma persisted for a containment, or -1 when there is none.
//! Plasma 6.8 merged screen() and lastScreen() into one stored value.
inline int containmentLastScreen(const Plasma::Containment *containment)
{
#ifdef LATTE_PLASMA_STORED_CONTAINMENT_SCREEN
    const uint screen = containment->screen();
    return screen <= static_cast<uint>(std::numeric_limits<int>::max()) ? static_cast<int>(screen) : -1;
#else
    return containment->lastScreen();
#endif
}

}

#endif
