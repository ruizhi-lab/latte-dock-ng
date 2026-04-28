/*
    SPDX-FileCopyrightText: 2016 Smith AR <audoban@openmailbox.org>
    SPDX-FileCopyrightText: 2016 Michail Vourlakos <mvourlakos@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "quickwindowsystem.h"

// Qt
#include <QDebug>

namespace Latte {

QuickWindowSystem::QuickWindowSystem(QObject *parent)
    : QObject(parent)
{
    // Wayland-only build keeps compositing enabled by contract.
    m_compositing = true;
}

QuickWindowSystem::~QuickWindowSystem()
{
    qDebug() << staticMetaObject.className() << "destructed";
}

bool QuickWindowSystem::compositingActive() const
{
    return m_compositing;
}

bool QuickWindowSystem::isPlatformWayland() const
{
    return true;
}

bool QuickWindowSystem::isPlatformX11() const
{
    return false;
}

} //end of namespace
