/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LATTEPLASMAQUICKCONTAINMENTVIEW_H
#define LATTEPLASMAQUICKCONTAINMENTVIEW_H

//! PlasmaQuick::ContainmentView is implemented by the system libPlasmaQuick,
//! so its declaration must come from the matching installed header. A local
//! copy of the class silently mismatches the library ABI whenever libplasma
//! changes the class (libplasma 6.8 rebased it from QuickViewSharedEngine onto
//! QQuickWindow and dropped config(), which broke the build and would crash a
//! binary compiled against the stale copy). The header is public since 6.0.
#include <PlasmaQuick/ContainmentView>

#include "../plasmaapicompat.h"

#ifdef LATTE_PLASMA_STORED_CONTAINMENT_SCREEN
#include <PlasmaQuick/PlasmaQuick>
#endif

#include <QQmlEngine>

#include <memory>

namespace Latte {

//! QML engine that loads the view's QML. libplasma 6.8 ContainmentView no
//! longer exposes engine(); it always uses PlasmaQuick::globalEngine(), the
//! engine shared with applets.
inline std::shared_ptr<QQmlEngine> containmentViewEngine([[maybe_unused]] PlasmaQuick::ContainmentView *view)
{
#ifdef LATTE_PLASMA_STORED_CONTAINMENT_SCREEN
    return PlasmaQuick::globalEngine();
#else
    return view->engine();
#endif
}

}

#endif
