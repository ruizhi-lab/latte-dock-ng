/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LATTE_SESSION_SHUTDOWNMONITOR_H
#define LATTE_SESSION_SHUTDOWNMONITOR_H

// Qt
#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QObject>
#include <QString>
#include <QTimer>

// C++
#include <functional>
#include <optional>

namespace Latte::Session {

//! ksmserver can be busy while the session tears down. The 25 s default D-Bus
//! timeout would stall the event loop long enough for quit() and the view
//! teardown to miss the logout window, so every state query is bounded.
inline constexpr int KsmServerQueryTimeoutMs = 1000;

//! Returns ksmserver's isShuttingDown(), or no value when ksmserver did not
//! answer (no owner, error or timeout).
std::optional<bool> ksmServerShutdownState(const QDBusConnection &connection, int timeoutMs = KsmServerQueryTimeoutMs);

//! Returns ksmserver's isShuttingDown(); false on error or timeout.
bool isKsmServerShuttingDown(const QDBusConnection &connection, int timeoutMs = KsmServerQueryTimeoutMs);

//! Follows a confirmed KDE logout without polling for the whole session.
//!
//! Trigger: plasma-shutdown is D-Bus activated as org.kde.Shutdown only after
//! a logout, reboot or shutdown is confirmed, and it exits once the logout is
//! cancelled or complete.
//! Authority: ksmserver's isShuttingDown() and the ownership of
//! org.kde.Shutdown. A confirmed logout hides the dock; isShuttingDown()
//! turning false again is a cancel and restores it; plasma-shutdown leaving
//! while isShuttingDown() is still true is the completion and quits. Other
//! clients' windows are never consulted: on Wayland that list keeps windows a
//! logout does not wait for (tray-only and minimized clients, plasmashell's
//! desktop surfaces), so it cannot tell when the logout ends.
//! Lifetime: the dock stays until the logout completes or is cancelled, so
//! KWin's "Cancel Logout" brings back a working dock. It does not hold the
//! logout up because the hidden dock keeps no window that KWin waits for.
//! Sampling: isShuttingDown() has no change signal, so it is sampled every
//! PollIntervalMs while org.kde.Shutdown is owned, and afterwards only until a
//! logout already being sampled is resolved. The session-ending flag alone
//! never starts sampling: a flag set by SIGTERM must not be read back as a
//! cancelled logout.
//! Failure prevented: sampling for the whole session issued two blocking
//! D-Bus calls every 500 ms and kept waking the idle dock.
class ShutdownMonitor : public QObject
{
    Q_OBJECT

public:
    //! Probes and side effects are injected so the state machine can be
    //! exercised without ksmserver, KWin or a Corona.
    struct Hooks {
        //! ksmserver's isShuttingDown(); no value when ksmserver did not answer.
        std::function<std::optional<bool>()> isSessionShuttingDown;
        std::function<bool()> isSessionEnding;
        std::function<void()> markSessionEnding;
        std::function<void()> clearSessionEnding;
        //! Hides the views and suspends their helper windows.
        std::function<void()> hideViews;
        //! Shows the views again and recreates their helper windows.
        std::function<void()> restoreViews;
        std::function<void()> quit;
    };

    static constexpr int PollIntervalMs = 500;
    static QString shutdownServiceName();

    ShutdownMonitor(const QDBusConnection &connection, Hooks hooks, QObject *parent = nullptr);

    bool isPolling() const;
    bool isShutdownServiceRegistered() const;

    //! Applies an org.kde.Shutdown ownership change reported by the bus.
    void setShutdownServiceRegistered(bool registered);

    //! Samples the session once; the poll timer drives it while needed.
    void evaluate();

private:
    void updatePolling();

    Hooks m_hooks;
    QDBusServiceWatcher m_serviceWatcher;
    QTimer m_pollTimer;
    bool m_shutdownServiceRegistered{false};
};

}

#endif
