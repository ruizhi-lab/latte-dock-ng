/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "session/shutdownmonitor.h"

#include <QDBusConnection>
#include <QDBusContext>
#include <QElapsedTimer>
#include <QTest>
#include <QThread>

#include <atomic>

using Latte::Session::ShutdownMonitor;

namespace {

//! Fake session state reported by the hooks, plus counters for side effects.
struct FakeSession {
    bool shuttingDown{false};
    bool sessionEnding{false};
    bool quitWithoutWaiting{true};
    int probes{0};
    int hides{0};
    int restores{0};
    int quits{0};

    ShutdownMonitor::Hooks hooks()
    {
        return {
            .isSessionShuttingDown = [this]() {
                ++probes;
                return shuttingDown;
            },
            .isSessionEnding = [this]() {
                return sessionEnding;
            },
            .quitWithoutWaiting = [this]() {
                return quitWithoutWaiting;
            },
            .markSessionEnding = [this]() {
                sessionEnding = true;
            },
            .clearSessionEnding = [this]() {
                sessionEnding = false;
            },
            .hideViews = [this]() {
                ++hides;
            },
            .restoreViews = [this]() {
                ++restores;
            },
            .quit = [this]() {
                ++quits;
            },
        };
    }
};

QDBusConnection disconnectedBus()
{
    return QDBusConnection(QStringLiteral("latte-shutdownmonitortest-no-bus"));
}

//! CTest runs this test under dbus-run-session. Bus tests must never claim
//! org.kde.Shutdown or org.kde.ksmserver on a real desktop bus, so each one
//! checks this with QVERIFY before touching the bus.
bool onPrivateBus()
{
    return qEnvironmentVariableIsSet("LATTE_PRIVATE_TEST_BUS") && QDBusConnection::sessionBus().isConnected();
}

//! Lives in a worker thread so the blocking query on the main thread can be
//! answered (or deliberately left unanswered).
class FakeKsmServer : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.KSMServerInterface")

public:
    std::atomic<bool> shuttingDown{false};
    std::atomic<bool> withholdReply{false};

public Q_SLOTS:
    bool isShuttingDown()
    {
        if (withholdReply) {
            // Never answer: the caller has to give up at its own timeout.
            setDelayedReply(true);
            return false;
        }

        return shuttingDown;
    }
};

}

class ShutdownMonitorTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void idleSessionDoesNotPoll();
    void shutdownServiceStartsAndStopsPolling();
    void confirmedLogoutHidesViewsOnce();
    void fastExitQuitsOnConfirmedLogout();
    void cancelledLogoutRestoresViewsWhenServiceLeaves();
    void completedLogoutQuitsWhenServiceLeaves();
    void fallbackQuitsWhenCompletionNeverArrives();
    void signalSetEndingFlagDoesNotStartPolling();
    void busOwnershipDrivesPolling();
    void existingOwnerIsDetectedAtStartup();
    void ksmServerQueryReportsState();
    void ksmServerQueryIsBounded();
};

void ShutdownMonitorTest::idleSessionDoesNotPoll()
{
    FakeSession session;
    ShutdownMonitor monitor(disconnectedBus(), session.hooks());

    QVERIFY(!monitor.isShutdownServiceRegistered());
    QVERIFY(!monitor.isPolling());

    QTest::qWait(ShutdownMonitor::PollIntervalMs + 200);
    QCOMPARE(session.probes, 0);
}

void ShutdownMonitorTest::shutdownServiceStartsAndStopsPolling()
{
    FakeSession session;
    ShutdownMonitor monitor(disconnectedBus(), session.hooks());

    monitor.setShutdownServiceRegistered(true);
    QVERIFY(monitor.isPolling());
    QTRY_VERIFY_WITH_TIMEOUT(session.probes > 0, ShutdownMonitor::PollIntervalMs * 4);

    // plasma-shutdown left without a confirmed logout: nothing to undo.
    monitor.setShutdownServiceRegistered(false);
    QVERIFY(!monitor.isPolling());
    QCOMPARE(session.hides, 0);
    QCOMPARE(session.restores, 0);
    QCOMPARE(session.quits, 0);
}

void ShutdownMonitorTest::confirmedLogoutHidesViewsOnce()
{
    FakeSession session;
    // Keep the dock alive after the confirmation so the hide side effect can be
    // observed more than once; the fast-exit path is covered separately.
    session.quitWithoutWaiting = false;
    ShutdownMonitor monitor(disconnectedBus(), session.hooks());
    monitor.setShutdownServiceRegistered(true);

    session.shuttingDown = true;
    monitor.evaluate();
    QVERIFY(session.sessionEnding);
    QCOMPARE(session.hides, 1);
    QCOMPARE(session.quits, 0);

    // No window list is consulted, so the decision never depends on other
    // clients: the fallback timer or the completion report ends the dock.
    monitor.evaluate();
    QCOMPARE(session.hides, 1);
    QCOMPARE(session.quits, 0);
}

void ShutdownMonitorTest::fastExitQuitsOnConfirmedLogout()
{
    FakeSession session;
    session.quitWithoutWaiting = true;
    ShutdownMonitor monitor(disconnectedBus(), session.hooks());
    monitor.setShutdownServiceRegistered(true);

    session.shuttingDown = true;
    monitor.evaluate();

    // Preferences > Behavior: leave in the same sample, without waiting for
    // any other client's window and without the fallback timer.
    QVERIFY(session.sessionEnding);
    QCOMPARE(session.hides, 1);
    QCOMPARE(session.quits, 1);
    QCOMPARE(session.restores, 0);
}

void ShutdownMonitorTest::cancelledLogoutRestoresViewsWhenServiceLeaves()
{
    FakeSession session;
    // What is tested here is the cancel branch, so the dock stays alive long
    // enough to observe it instead of using the fast-exit path.
    session.quitWithoutWaiting = false;
    ShutdownMonitor monitor(disconnectedBus(), session.hooks());
    monitor.setShutdownServiceRegistered(true);

    session.shuttingDown = true;
    monitor.evaluate();
    QCOMPARE(session.hides, 1);

    // ksmserver resets its state before plasma-shutdown exits; the exit
    // triggers an immediate sample instead of waiting another interval.
    session.shuttingDown = false;
    monitor.setShutdownServiceRegistered(false);
    QVERIFY(!session.sessionEnding);
    QCOMPARE(session.restores, 1);
    QCOMPARE(session.quits, 0);
    QVERIFY(!monitor.isPolling());
}

void ShutdownMonitorTest::completedLogoutQuitsWhenServiceLeaves()
{
    FakeSession session;
    session.quitWithoutWaiting = false;
    ShutdownMonitor monitor(disconnectedBus(), session.hooks());
    monitor.setShutdownServiceRegistered(true);

    session.shuttingDown = true;
    monitor.evaluate();
    QCOMPARE(session.quits, 0);

    // plasma-shutdown leaving while ksmserver still reports a logout is the
    // completion report: the dock leaves without inspecting any window list.
    monitor.setShutdownServiceRegistered(false);
    QTRY_COMPARE_WITH_TIMEOUT(session.quits, 1, ShutdownMonitor::PollIntervalMs * 4);
    QCOMPARE(session.restores, 0);
}

void ShutdownMonitorTest::fallbackQuitsWhenCompletionNeverArrives()
{
    FakeSession session;
    session.quitWithoutWaiting = false;
    ShutdownMonitor monitor(disconnectedBus(), session.hooks());
    monitor.setShutdownServiceRegistered(true);

    session.shuttingDown = true;
    monitor.evaluate();
    QCOMPARE(session.quits, 0);

    // Neither a cancel nor a completion arrives: the hard bound must end the
    // dock before KWin reports its windows as unclosed (10 s).
    QVERIFY(ShutdownMonitor::QuitFallbackMs < 10000);
    QTRY_COMPARE_WITH_TIMEOUT(session.quits, 1, ShutdownMonitor::QuitFallbackMs + 3000);
    QCOMPARE(session.restores, 0);
}

void ShutdownMonitorTest::signalSetEndingFlagDoesNotStartPolling()
{
    FakeSession session;
    ShutdownMonitor monitor(disconnectedBus(), session.hooks());

    // SIGTERM marks the session as ending without a logout. Sampling it would
    // read "not shutting down" as a cancelled logout and re-show the views.
    session.sessionEnding = true;
    monitor.setShutdownServiceRegistered(false);
    QVERIFY(!monitor.isPolling());

    QTest::qWait(ShutdownMonitor::PollIntervalMs + 200);
    QCOMPARE(session.probes, 0);
    QCOMPARE(session.restores, 0);
    QVERIFY(session.sessionEnding);
}

void ShutdownMonitorTest::busOwnershipDrivesPolling()
{
    QVERIFY(onPrivateBus());

    FakeSession session;
    ShutdownMonitor monitor(QDBusConnection::sessionBus(), session.hooks());

    const QString ownerName = QStringLiteral("latte-shutdown-service-owner");
    QDBusConnection owner = QDBusConnection::connectToBus(QDBusConnection::SessionBus, ownerName);
    QVERIFY(owner.isConnected());

    QVERIFY(owner.registerService(ShutdownMonitor::shutdownServiceName()));
    QTRY_VERIFY(monitor.isShutdownServiceRegistered());
    QVERIFY(monitor.isPolling());

    QVERIFY(owner.unregisterService(ShutdownMonitor::shutdownServiceName()));
    QTRY_VERIFY(!monitor.isShutdownServiceRegistered());
    QVERIFY(!monitor.isPolling());

    QDBusConnection::disconnectFromBus(ownerName);
}

void ShutdownMonitorTest::existingOwnerIsDetectedAtStartup()
{
    QVERIFY(onPrivateBus());

    const QString ownerName = QStringLiteral("latte-shutdown-early-owner");
    QDBusConnection owner = QDBusConnection::connectToBus(QDBusConnection::SessionBus, ownerName);
    QVERIFY(owner.isConnected());
    QVERIFY(owner.registerService(ShutdownMonitor::shutdownServiceName()));

    // The ownership signal predates the monitor; only the startup query can
    // report it.
    FakeSession session;
    ShutdownMonitor monitor(QDBusConnection::sessionBus(), session.hooks());
    QTRY_VERIFY(monitor.isShutdownServiceRegistered());
    QVERIFY(monitor.isPolling());

    QVERIFY(owner.unregisterService(ShutdownMonitor::shutdownServiceName()));
    QDBusConnection::disconnectFromBus(ownerName);
    QTRY_VERIFY(!monitor.isPolling());
}

void ShutdownMonitorTest::ksmServerQueryReportsState()
{
    QVERIFY(onPrivateBus());

    QThread serviceThread;
    auto *ksmserver = new FakeKsmServer;
    ksmserver->moveToThread(&serviceThread);
    connect(&serviceThread, &QThread::finished, ksmserver, &QObject::deleteLater);
    serviceThread.start();

    const QString ownerName = QStringLiteral("latte-ksmserver-owner");
    QDBusConnection owner = QDBusConnection::connectToBus(QDBusConnection::SessionBus, ownerName);
    QVERIFY(owner.registerObject(QStringLiteral("/KSMServer"), ksmserver, QDBusConnection::ExportAllSlots));
    QVERIFY(owner.registerService(QStringLiteral("org.kde.ksmserver")));

    ksmserver->shuttingDown = true;
    QVERIFY(Latte::Session::isKsmServerShuttingDown(QDBusConnection::sessionBus()));
    ksmserver->shuttingDown = false;
    QVERIFY(!Latte::Session::isKsmServerShuttingDown(QDBusConnection::sessionBus()));

    owner.unregisterObject(QStringLiteral("/KSMServer"));
    QVERIFY(owner.unregisterService(QStringLiteral("org.kde.ksmserver")));
    QDBusConnection::disconnectFromBus(ownerName);
    serviceThread.quit();
    QVERIFY(serviceThread.wait(5000));
}

void ShutdownMonitorTest::ksmServerQueryIsBounded()
{
    QVERIFY(onPrivateBus());

    QThread serviceThread;
    auto *ksmserver = new FakeKsmServer;
    ksmserver->moveToThread(&serviceThread);
    connect(&serviceThread, &QThread::finished, ksmserver, &QObject::deleteLater);
    serviceThread.start();

    const QString ownerName = QStringLiteral("latte-ksmserver-silent-owner");
    QDBusConnection owner = QDBusConnection::connectToBus(QDBusConnection::SessionBus, ownerName);
    QVERIFY(owner.registerObject(QStringLiteral("/KSMServer"), ksmserver, QDBusConnection::ExportAllSlots));
    QVERIFY(owner.registerService(QStringLiteral("org.kde.ksmserver")));

    // An unresponsive ksmserver must not stall the caller for the 25 s
    // default D-Bus timeout; the query reports "not shutting down" instead.
    ksmserver->withholdReply = true;
    QElapsedTimer elapsed;
    elapsed.start();
    QVERIFY(!Latte::Session::isKsmServerShuttingDown(QDBusConnection::sessionBus(), 200));
    // The lower bound proves the call reached the silent service and waited
    // for its timeout instead of failing fast for an unrelated bus error.
    const qint64 waitedMs = elapsed.elapsed();
    QVERIFY2(waitedMs >= 100 && waitedMs < 5000, qPrintable(QStringLiteral("query took %1 ms").arg(waitedMs)));

    owner.unregisterObject(QStringLiteral("/KSMServer"));
    QVERIFY(owner.unregisterService(QStringLiteral("org.kde.ksmserver")));
    QDBusConnection::disconnectFromBus(ownerName);
    serviceThread.quit();
    QVERIFY(serviceThread.wait(5000));
}

QTEST_GUILESS_MAIN(ShutdownMonitorTest)

#include "shutdownmonitortest.moc"
