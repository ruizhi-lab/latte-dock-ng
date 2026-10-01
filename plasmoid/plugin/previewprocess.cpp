/* SPDX-FileCopyrightText: 2026 Latte Dock Contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */
#include "previewprocess.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <utility>
namespace Latte::Tasks {
namespace {
// Bounded JSON-lines protocol. A frame larger than this is treated as
// malformed so a stuck or compromised helper cannot grow dock memory.
constexpr int MaxProtocolBytes = 64 * 1024;
// The helper renders at most nine capture cards; the dock refuses anything
// larger before it reaches the wire.
constexpr int MaxPreviews = 9;
// Startup and heartbeat are both covered by one watchdog: the helper emits a
// heartbeat every 250 ms, so 2.5 s of silence means it is gone or wedged.
constexpr int WatchdogMs = 2500;
// A single crash keeps previews available for later hovers, but three
// consecutive failures mean the helper cannot run here and the session gate
// closes to prevent a restart loop.
constexpr int MaxConsecutiveFailures = 3;
const QString TypeShow = QStringLiteral("show");
const QString TypeMove = QStringLiteral("move");
const QString TypeHide = QStringLiteral("hide");
const QString TypeHeartbeat = QStringLiteral("heartbeat");
const QString TypeActivate = QStringLiteral("activate");
const QString TypeClose = QStringLiteral("close");
const QString TypeClosed = QStringLiteral("closed");
// Validate a window descriptor before it crosses the process boundary. Only
// identity metadata is allowed; image data must never travel through the pipe.
bool isValidWindow(const QVariant &window)
{
    const QVariantMap map = window.toMap();
    return map.value(QStringLiteral("uuid")).canConvert<QString>()
        && !QUuid(map.value(QStringLiteral("uuid")).toString()).isNull()
        && map.value(QStringLiteral("title")).canConvert<QString>()
        && map.value(QStringLiteral("appName")).canConvert<QString>()
        && map.value(QStringLiteral("launcherUrl")).canConvert<QString>()
        && map.value(QStringLiteral("appPid")).canConvert<int>();
}
}
PreviewProcess::PreviewProcess(QObject *parent)
    : PreviewProcess(QCoreApplication::applicationDirPath()
                         + QStringLiteral("/latte-dock-ng-preview"),
                     true,
                     parent)
{
}
PreviewProcess::PreviewProcess(const QString &executable, bool enabled, QObject *parent)
    : QObject(parent), m_enabled(enabled), m_executable(executable)
{
    // The saved hover action is authoritative in QML. This process-side gate
    // only closes after repeated helper failures; otherwise the visible Dock
    // setting would claim previews are enabled while silently doing nothing.
    m_watchdog.setInterval(WatchdogMs);
    m_watchdog.setSingleShot(true);
    connect(&m_watchdog, &QTimer::timeout, this, &PreviewProcess::fail);
}
PreviewProcess::~PreviewProcess()
{
    shutdownProcess();
}
void PreviewProcess::show(const QVariantList &windows, const QRect &anchor, int edge)
{
    // A blocked hover stays hidden without touching the process/pipe so the
    // title tooltip path owns the interaction until the pointer leaves.
    if (!m_enabled) {
        return;
    }
    if (windows.isEmpty() || windows.size() > MaxPreviews || !anchor.isValid()) {
        hide();
        return;
    }
    for (const auto &window : windows) {
        if (!isValidWindow(window)) {
            hide();
            return;
        }
    }
    // Generations reject delayed replies belonging to a previous task
    // selection while the helper is still finishing the previous capture. A
    // fresh selection is also a new hover boundary: a failure that hid the
    // previous task must not keep the new task blocked.
    if (windows != m_windows) {
        ++m_serial;
        m_windows = windows;
        m_hoverBlocked = false;
    }
    if (m_hoverBlocked) {
        return;
    }
    m_pending = true;
    m_request = QJsonDocument(QJsonObject{{QStringLiteral("type"), TypeShow},
        {QStringLiteral("serial"), m_serial},
        {QStringLiteral("windows"), QJsonArray::fromVariantList(windows)},
        {QStringLiteral("x"), anchor.x()}, {QStringLiteral("y"), anchor.y()},
        {QStringLiteral("width"), anchor.width()}, {QStringLiteral("height"), anchor.height()},
        {QStringLiteral("edge"), edge}}).toJson(QJsonDocument::Compact) + '\n';
    if (!m_process) {
        start();
    } else {
        send();
    }
}
void PreviewProcess::start()
{
    if (!QFileInfo(m_executable).isExecutable()) {
        fail();
        return;
    }
    m_process = new QProcess(this);
    // A new process has seen none of the previous traffic; force the pending
    // request to be written even if it is byte-identical to the last one.
    m_sent.clear();
    // The helper's stderr is forwarded for diagnostics; its stdout carries the
    // protocol and must stay a separate channel.
    m_process->setProcessChannelMode(QProcess::ForwardedErrorChannel);
    connect(m_process, &QProcess::started, this, &PreviewProcess::send);
    connect(m_process, &QProcess::bytesWritten, this, &PreviewProcess::send);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &PreviewProcess::read);
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) { fail(); });
    connect(m_process, &QProcess::finished, this, &PreviewProcess::onFinished);
    m_watchdog.start();
    m_process->start(m_executable, {});
}
void PreviewProcess::send()
{
    if (!m_process || m_process->state() != QProcess::Running) {
        return;
    }
    // Never wait for IPC. Coalesce to the latest hover position when busy and
    // fail closed if the peer stops draining the pipe.
    if (m_process->bytesToWrite() > MaxProtocolBytes || m_request.size() > MaxProtocolBytes) {
        fail();
    } else if (m_process->bytesToWrite() == 0 && m_sent != m_request) {
        m_sent = m_request;
        m_process->write(m_request);
    }
}
void PreviewProcess::move(const QRect &anchor, int edge)
{
    if (!m_enabled || m_hoverBlocked || m_windows.isEmpty() || !m_process || !anchor.isValid()) {
        return;
    }
    // A show request must be on the wire before a geometry-only update makes
    // sense. Flush it first; if it is still queued, skip this frame rather
    // than overwriting the selection with a move the helper cannot use yet.
    if (m_sent != m_request) {
        send();
        if (m_sent != m_request) {
            return;
        }
    }
    m_request = QJsonDocument(QJsonObject{{QStringLiteral("type"), TypeMove},
        {QStringLiteral("serial"), m_serial},
        {QStringLiteral("x"), anchor.x()}, {QStringLiteral("y"), anchor.y()},
        {QStringLiteral("width"), anchor.width()}, {QStringLiteral("height"), anchor.height()},
        {QStringLiteral("edge"), edge}}).toJson(QJsonDocument::Compact) + '\n';
    send();
}
void PreviewProcess::hide()
{
    // Leaving a task is a new failure boundary: the next hover may retry.
    m_hoverBlocked = false;
    m_pending = false;
    m_watchdog.stop();
    m_windows.clear();
    ++m_serial;
    m_request = QJsonDocument(QJsonObject{{QStringLiteral("type"), TypeHide},
        {QStringLiteral("serial"), m_serial}}).toJson(QJsonDocument::Compact) + '\n';
    m_visible = false;
    m_hovered = false;
    send();
    Q_EMIT stateChanged();
}
void PreviewProcess::read()
{
    if (!m_process) {
        return;
    }
    m_input += m_process->readAllStandardOutput();
    if (m_input.size() > MaxProtocolBytes) {
        fail();
        return;
    }
    while (m_input.contains('\n')) {
        const auto end = m_input.indexOf('\n');
        const auto line = m_input.left(end);
        m_input.remove(0, end + 1);
        // Malformed lines are ignored, never fatal to the dock.
        const auto reply = QJsonDocument::fromJson(line).object();
        if (!reply.contains(QStringLiteral("serial"))
                || reply.value(QStringLiteral("serial")).toInt() != m_serial) {
            continue;
        }
        const QString type = reply.value(QStringLiteral("type")).toString();
        if (type == TypeClosed) {
            // Clean shutdown announced before exit; do not count it as a crash.
            m_closing = true;
            m_watchdog.stop();
            continue;
        }
        if (type == TypeHeartbeat) {
            m_pending = false;
            // A live reply breaks any run of failures, so only genuinely
            // consecutive crashes can close the session gate.
            m_failures = 0;
            m_watchdog.start();
            const bool visible = reply.value(QStringLiteral("visible")).toBool();
            const bool hovered = reply.value(QStringLiteral("hovered")).toBool();
            if (visible != m_visible || hovered != m_hovered) {
                m_visible = visible;
                m_hovered = hovered;
                Q_EMIT stateChanged();
            }
            continue;
        }
        if (type == TypeActivate || type == TypeClose) {
            // The UUID is re-validated against the windows the dock most
            // recently sent, so a stale or forged click cannot activate an
            // arbitrary window.
            const QString uuid = reply.value(QStringLiteral("uuid")).toString();
            for (const auto &window : std::as_const(m_windows)) {
                if (!uuid.isEmpty() && window.toMap().value(QStringLiteral("uuid")).toString() == uuid) {
                    if (type == TypeActivate) {
                        Q_EMIT activateRequested(uuid);
                        hide();
                    } else {
                        Q_EMIT closeRequested(uuid);
                    }
                    break;
                }
            }
        }
    }
}
void PreviewProcess::onFinished()
{
    QProcess *process = m_process;
    m_process = nullptr;
    if (process) {
        process->deleteLater();
    }
    if (m_pending && !m_closing) {
        fail();
        return;
    }
    // Idle-lease expiry or an announced close is normal: keep the feature
    // enabled so the next hover can start a fresh helper.
    m_closing = false;
    m_pending = false;
    m_watchdog.stop();
    if (m_visible || m_hovered) {
        m_visible = false;
        m_hovered = false;
        Q_EMIT stateChanged();
    }
}
void PreviewProcess::discardProcess()
{
    if (!m_process) {
        return;
    }
    QProcess *process = m_process;
    m_process = nullptr;
    // Detach first: a subsequent finished() must not double-handle teardown.
    process->disconnect(this);
    if (process->state() == QProcess::NotRunning) {
        process->deleteLater();
    } else {
        connect(process, &QProcess::finished, process, &QObject::deleteLater);
        process->kill();
    }
}

void PreviewProcess::shutdownProcess()
{
    if (!m_process) {
        return;
    }

    QProcess *process = m_process;
    m_process = nullptr;
    process->disconnect(this);

    if (process->state() != QProcess::NotRunning) {
        // QObject teardown is about to stop the event loop. EOF lets the helper
        // close cleanly; reap it synchronously so its child QProcess is never
        // destroyed while live. Force-kill only if graceful exit exceeds the
        // short shutdown bound, then wait once more before deleting the owner.
        if (process->state() == QProcess::Running) {
            process->closeWriteChannel();
        }
        if (!process->waitForFinished(250) && process->state() != QProcess::NotRunning) {
            process->kill();
            process->waitForFinished(1000);
        }
    }

    delete process;
}

void PreviewProcess::fail()
{
    discardProcess();
    m_watchdog.stop();
    m_pending = false;
    m_closing = false;
    m_visible = false;
    m_hovered = false;
    // Fail closed for this hover; the title tooltip takes over immediately.
    m_hoverBlocked = true;
    if (++m_failures >= MaxConsecutiveFailures) {
        m_enabled = false;
        qWarning("Isolated preview helper failed repeatedly; disabled for this session");
    } else {
        qWarning("Isolated preview helper failed or stopped responding; falling back to the title tooltip");
    }
    Q_EMIT stateChanged();
}
}
