/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "shutdownmonitor.h"

// Qt
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDebug>

namespace Latte::Session {

std::optional<bool> ksmServerShutdownState(const QDBusConnection &connection, int timeoutMs)
{
    const QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.ksmserver"),
                                                                QStringLiteral("/KSMServer"),
                                                                QStringLiteral("org.kde.KSMServerInterface"),
                                                                QStringLiteral("isShuttingDown"));
    const QDBusMessage reply = connection.call(message, QDBus::Block, timeoutMs);

    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty()) {
        return std::nullopt;
    }

    return reply.arguments().constFirst().toBool();
}

bool isKsmServerShuttingDown(const QDBusConnection &connection, int timeoutMs)
{
    return ksmServerShutdownState(connection, timeoutMs).value_or(false);
}

QString ShutdownMonitor::shutdownServiceName()
{
    return QStringLiteral("org.kde.Shutdown");
}

ShutdownMonitor::ShutdownMonitor(const QDBusConnection &connection, Hooks hooks, QObject *parent)
    : QObject(parent),
      m_hooks(std::move(hooks)),
      m_serviceWatcher(shutdownServiceName(), connection, QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration)
{
    m_pollTimer.setInterval(PollIntervalMs);
    connect(&m_pollTimer, &QTimer::timeout, this, &ShutdownMonitor::evaluate);

    connect(&m_serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, [this]() {
        setShutdownServiceRegistered(true);
    });
    connect(&m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this]() {
        setShutdownServiceRegistered(false);
    });

    //! The watcher reports later ownership changes only. Ask once for the
    //! current owner so a dock started during a logout still follows it. The
    //! bus answers in order with the ownership signals, so the reply never
    //! overrides a newer change; it is asynchronous so startup never waits.
    if (QDBusConnectionInterface *bus = connection.interface()) {
        auto *pending = new QDBusPendingCallWatcher(bus->asyncCall(QStringLiteral("NameHasOwner"), shutdownServiceName()), this);
        connect(pending, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *call) {
            const QDBusPendingReply<bool> reply = *call;

            if (reply.isValid()) {
                setShutdownServiceRegistered(reply.value());
            }

            call->deleteLater();
        });
    }
}

bool ShutdownMonitor::isPolling() const
{
    return m_pollTimer.isActive();
}

bool ShutdownMonitor::isShutdownServiceRegistered() const
{
    return m_shutdownServiceRegistered;
}

void ShutdownMonitor::setShutdownServiceRegistered(bool registered)
{
    if (m_shutdownServiceRegistered == registered) {
        return;
    }

    m_shutdownServiceRegistered = registered;
    qInfo() << "[shutdown]" << shutdownServiceName() << (registered ? "registered; sampling session state." : "unregistered.");

    if (!registered) {
        //! plasma-shutdown exits right after logoutCancelled/logoutComplete.
        //! Sample now so a cancelled logout restores the views at once.
        evaluate();
    }

    updatePolling();
}

void ShutdownMonitor::evaluate()
{
    const std::optional<bool> shuttingDown = m_hooks.isSessionShuttingDown();

    //! ksmserver did not answer: it is busy (the query is bounded by
    //! KsmServerQueryTimeoutMs) or already gone at the end of the session.
    //! That is neither a cancel nor a completion. Reading it as "not shutting
    //! down" restored the hidden dock in the middle of a logout, so keep the
    //! current state and decide on the next sample.
    if (!shuttingDown.has_value()) {
        qInfo() << "[shutdown] ksmserver did not answer isShuttingDown(); keeping the current state.";
        return;
    }

    if (m_hooks.isSessionEnding()) {
        if (!*shuttingDown) {
            // User cancelled the logout confirmation.
            qInfo() << "[shutdown] logout cancelled; clearing session-ending flag.";
            m_hooks.clearSessionEnding();

            //! Restore the views that were hidden when the logout was announced.
            m_hooks.restoreViews();
            updatePolling();
            return;
        }

        //! plasma-shutdown left while ksmserver still reports the logout: the
        //! logout completed. While plasma-shutdown runs, the logout can still
        //! be cancelled from KWin's notification, so the dock never quits
        //! earlier and never decides from other clients' windows.
        if (!m_shutdownServiceRegistered) {
            qInfo() << "[shutdown] logout completed; quitting.";
            m_hooks.quit();
        }

        return;
    }

    if (*shuttingDown) {
        // Phase 1: logout confirmed. ksmserver's isShuttingDown only turns
        // true after the user confirms (performLogout), not while the
        // confirmation dialog shows. Set the flag but do NOT quit: the logout
        // can still be cancelled through KWin's "Cancel Logout" notification
        // while it closes the windows.
        qInfo() << "[shutdown] isShuttingDown() = true; setting flag (not quitting yet).";
        m_hooks.markSessionEnding();

        //! KWin's session shutdown waits for every xdg_toplevel window to
        //! close (org.kde.KWin.Session.closeWaylandWindows): it closes the
        //! windows, waits 10s, shows a "Cancel Logout / Log Out Anyway"
        //! notification, then waits indefinitely. It never sends close to a
        //! panel-role (dock) window, and the dock views and their helper
        //! windows are panel-role xdg_toplevels, so hide all of them as soon
        //! as the logout is announced; otherwise the whole shutdown blocks on
        //! the dock. The cancel branch above shows them again.
        m_hooks.hideViews();
    }
}

void ShutdownMonitor::updatePolling()
{
    if (m_shutdownServiceRegistered) {
        if (!m_pollTimer.isActive()) {
            m_pollTimer.start();
        }

        return;
    }

    //! plasma-shutdown has left: keep sampling only while a logout that was
    //! already being sampled is unresolved. Never start from the flag alone.
    if (m_pollTimer.isActive() && !m_hooks.isSessionEnding()) {
        m_pollTimer.stop();
    }
}

}
