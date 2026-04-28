/*
    SPDX-FileCopyrightText: 2019 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "currentscreentracker.h"

// local
#include "../view.h"
#include "../../wm/schemecolors.h"
#include "../../wm/tracker/lastactivewindow.h"
#include "../../wm/tracker/windowstracker.h"

namespace Latte {
namespace ViewPart {
namespace TrackerPart {

CurrentScreenTracker::CurrentScreenTracker(WindowsTracker *parent)
    : QObject(parent),
      m_latteView(parent->view()),
      m_wm(parent->wm())
{
    init();
}

CurrentScreenTracker::~CurrentScreenTracker()
{
    m_wm->windowsTracker()->removeView(m_latteView);
}

void  CurrentScreenTracker::init()
{
    if (lastActiveWindow()) {
        initSignalsForInformation();
    }

    connect(m_latteView, &Latte::View::layoutChanged, this, [&]() {
        if (m_latteView->layout()) {
            initSignalsForInformation();
        }
    });

    connect(m_wm->windowsTracker(), &WindowSystem::Tracker::Windows::informationAnnounced, this, [&](const Latte::View *view) {
        if (m_latteView == view) {
            initSignalsForInformation();
        }
    });

    connect(m_wm->windowsTracker(), &WindowSystem::Tracker::Windows::activeWindowMaximizedChanged, this, [&](const Latte::View *view) {
        if (m_latteView == view) {
            Q_EMIT activeWindowMaximizedChanged();
        }
    });

    connect(m_wm->windowsTracker(), &WindowSystem::Tracker::Windows::activeWindowTouchingChanged, this, [&](const Latte::View *view) {
        if (m_latteView == view) {
            Q_EMIT activeWindowTouchingChanged();
        }
    });

    connect(m_wm->windowsTracker(), &WindowSystem::Tracker::Windows::activeWindowTouchingEdgeChanged, this, [&](const Latte::View *view) {
        if (m_latteView == view) {
            Q_EMIT activeWindowTouchingEdgeChanged();
        }
    });

    connect(m_wm->windowsTracker(), &WindowSystem::Tracker::Windows::existsWindowActiveChanged, this, [&](const Latte::View *view) {
        if (m_latteView == view) {
            Q_EMIT existsWindowActiveChanged();
        }
    });

    connect(m_wm->windowsTracker(), &WindowSystem::Tracker::Windows::existsWindowMaximizedChanged, this, [&](const Latte::View *view) {
        if (m_latteView == view) {
            Q_EMIT existsWindowMaximizedChanged();
        }
    });

    connect(m_wm->windowsTracker(), &WindowSystem::Tracker::Windows::existsWindowTouchingChanged, this, [&](const Latte::View *view) {
        if (m_latteView == view) {
            Q_EMIT existsWindowTouchingChanged();
        }
    });

    connect(m_wm->windowsTracker(), &WindowSystem::Tracker::Windows::existsWindowTouchingEdgeChanged, this, [&](const Latte::View *view) {
        if (m_latteView == view) {
            Q_EMIT existsWindowTouchingEdgeChanged();
        }
    });

    connect(m_wm->windowsTracker(), &WindowSystem::Tracker::Windows::isTouchingBusyVerticalViewChanged, this, [&](const Latte::View *view) {
        if (m_latteView == view) {
            Q_EMIT isTouchingBusyVerticalViewChanged();
        }
    });

    connect(m_wm->windowsTracker(), &WindowSystem::Tracker::Windows::activeWindowSchemeChanged, this, [&](const Latte::View *view) {
        if (m_latteView == view) {
            Q_EMIT activeWindowSchemeChanged();
        }
    });

    connect(m_wm->windowsTracker(), &WindowSystem::Tracker::Windows::touchingWindowSchemeChanged, this, [&](const Latte::View *view) {
        if (m_latteView == view) {
            Q_EMIT touchingWindowSchemeChanged();
        }
    });
}

void CurrentScreenTracker::initSignalsForInformation()
{
    Q_EMIT lastActiveWindowChanged();
    Q_EMIT activeWindowMaximizedChanged();
    Q_EMIT activeWindowTouchingChanged();
    Q_EMIT activeWindowTouchingEdgeChanged();
    Q_EMIT existsWindowActiveChanged();
    Q_EMIT existsWindowMaximizedChanged();
    Q_EMIT existsWindowTouchingChanged();
    Q_EMIT existsWindowTouchingEdgeChanged();
    Q_EMIT activeWindowSchemeChanged();
    Q_EMIT touchingWindowSchemeChanged();
}

bool CurrentScreenTracker::activeWindowMaximized() const
{
    return m_wm->windowsTracker()->activeWindowMaximized(m_latteView);
}

bool CurrentScreenTracker::activeWindowTouching() const
{
    return m_wm->windowsTracker()->activeWindowTouching(m_latteView);
}

bool CurrentScreenTracker::activeWindowTouchingEdge() const
{
    return m_wm->windowsTracker()->activeWindowTouchingEdge(m_latteView);
}

bool CurrentScreenTracker::existsWindowActive() const
{
    return m_wm->windowsTracker()->existsWindowActive(m_latteView);
}

bool CurrentScreenTracker::existsWindowMaximized() const
{
    return m_wm->windowsTracker()->existsWindowMaximized(m_latteView);
}

bool CurrentScreenTracker::existsWindowTouching() const
{
    return m_wm->windowsTracker()->existsWindowTouching(m_latteView);
}

bool CurrentScreenTracker::existsWindowTouchingEdge() const
{
    return m_wm->windowsTracker()->existsWindowTouchingEdge(m_latteView);
}

bool CurrentScreenTracker::isTouchingBusyVerticalView() const
{
    return m_wm->windowsTracker()->isTouchingBusyVerticalView(m_latteView);
}

WindowSystem::SchemeColors *CurrentScreenTracker::activeWindowScheme() const
{
    return m_wm->windowsTracker()->activeWindowScheme(m_latteView);
}

WindowSystem::SchemeColors *CurrentScreenTracker::touchingWindowScheme() const
{
    return m_wm->windowsTracker()->touchingWindowScheme(m_latteView);
}

WindowSystem::Tracker::LastActiveWindow *CurrentScreenTracker::lastActiveWindow()
{
    return m_wm->windowsTracker()->lastActiveWindow(m_latteView);
}


//! Window Functions
void CurrentScreenTracker::requestMoveLastWindow(int localX, int localY)
{
    m_wm->windowsTracker()->lastActiveWindow(m_latteView)->requestMove(m_latteView, localX, localY);
}

}
}
}
