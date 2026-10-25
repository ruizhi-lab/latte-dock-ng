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

namespace Latte::Session {

//! ksmserver can be busy while the session tears down. The 25 s default D-Bus
//! timeout would stall the event loop long enough for quit() and the view
//! teardown to miss the logout window, so every state query is bounded.
inline constexpr int KsmServerQueryTimeoutMs = 1000;

//! Returns ksmserver's isShuttingDown(); false on error or timeout.
bool isKsmServerShuttingDown(const QDBusConnection &connection, int timeoutMs = KsmServerQueryTimeoutMs);

//! Follows a confirmed KDE logout without polling for the whole session.
//!
//! Trigger: plasma-shutdown is D-Bus activated as org.kde.Shutdown only after
//! a logout, reboot or shutdown is confirmed, and it exits once ksmserver
//! reports the logout cancelled or complete.
//! Authority: ksmserver's isShuttingDown() and KWin's window list decide every
//! step. Neither publishes a change signal, so both are sampled every
//! PollIntervalMs while org.kde.Shutdown is owned, and afterwards only until a
//! logout already being sampled is resolved (a cancel restores the views, a
//! commit quits). The session-ending flag alone never starts sampling: a flag
//! set by SIGTERM must not be read back as a cancelled logout.
//! Failure prevented: sampling for the whole session issued two blocking
//! D-Bus calls every 500 ms and kept waking the idle dock.
class ShutdownMonitor : public QObject
{
    Q_OBJECT

public:
    //! Probes and side effects are injected so the state machine can be
    //! exercised without ksmserver, KWin or a Corona.
    struct Hooks {
        std::function<bool()> isSessionShuttingDown;
        std::function<bool()> hasBlockingWindows;
        std::function<bool()> isSessionEnding;
        std::function<void()> markSessionEnding;
        std::function<void()> clearSessionEnding;
        std::function<void()> hideViews;
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
    bool m_sawBlockingWindows{false};
};

}

#endif
