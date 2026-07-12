/*
    SPDX-FileCopyrightText: 2026 Latte Dock Contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "windowscreenshotprovider.h"
#include "pipecaptureworker.h"

// Qt
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusObjectPath>
#include <QDBusPendingReply>
#include <QDBusUnixFileDescriptor>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QSocketNotifier>
#include <QStandardPaths>
#include <QTimer>

// POSIX
#include <sys/socket.h>
#include <unistd.h>

namespace Latte
{
namespace Tasks
{

WindowScreenshotProvider::WindowScreenshotProvider(QObject *parent)
    : QObject(parent)
{
    m_cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                 + QStringLiteral("/window-screenshots");
    QDir().mkpath(m_cacheDir);
}

WindowScreenshotProvider::~WindowScreenshotProvider()
{
    // Close all pending capture FDs
    for (auto it = m_pendingByFd.begin(); it != m_pendingByFd.end(); ++it) {
        delete it.value().notifier;
        ::close(it.key());
    }
    m_pendingByFd.clear();
}

void WindowScreenshotProvider::cacheFrame(const QString &windowUuid, const QVariant &image)
{
    if (windowUuid.isEmpty() || !image.isValid()) return;

    // Evict old entry
    if (m_cache.contains(windowUuid)) {
        QFile::remove(m_cache.value(windowUuid));
        m_lruOrder.removeAll(windowUuid);
    }

    while (m_cache.size() >= m_maxCacheSize) {
        evictLru();
    }

    QString filePath = m_cacheDir + QLatin1Char('/') + windowUuid + QLatin1String(".png");
    filePath.replace(QLatin1Char('/'), QLatin1Char('_'));

    QImage img = image.value<QImage>();
    if (img.isNull()) {
        qWarning() << "WindowScreenshotProvider: cacheFrame got null image for" << windowUuid;
        return;
    }

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        img.save(&file, "PNG");
        file.close();
        m_cache.insert(windowUuid, filePath);
        m_lruOrder.append(windowUuid);
        emit cacheSizeChanged();
        emit screenshotReady(windowUuid, filePath);
    }
}

bool WindowScreenshotProvider::hasPending(const QString &windowUuid) const
{
    return m_pendingUuids.contains(windowUuid);
}

void WindowScreenshotProvider::captureViaPipeWire(uint nodeId, const QString &windowUuid)
{
    if (windowUuid.isEmpty() || nodeId == 0) {
        emit screenshotFailed(windowUuid);
        return;
    }

    // Return cached entry immediately if we already have one
    if (m_cache.contains(windowUuid)) {
        m_lruOrder.removeAll(windowUuid);
        m_lruOrder.append(windowUuid);
        emit screenshotReady(windowUuid, m_cache.value(windowUuid));
        return;
    }

    if (m_pendingUuids.contains(windowUuid)) {
        return;
    }
    m_pendingUuids.insert(windowUuid);

    // Build output path
    QString filePath = m_cacheDir + QLatin1Char('/') + windowUuid + QLatin1String(".png");
    filePath.replace(QLatin1Char('/'), QLatin1Char('_'));

    auto *worker = new PipeWireCaptureWorker(this);
    QObject::connect(worker, &PipeWireCaptureWorker::captured,
                     this, [this](const QString &uuid, const QString &path) {
        m_pendingUuids.remove(uuid);
        // Evict old entry if it exists
        if (m_cache.contains(uuid)) {
            QFile::remove(m_cache.value(uuid));
            m_lruOrder.removeAll(uuid);
        }
        while (m_cache.size() >= m_maxCacheSize) {
            evictLru();
        }
        m_cache.insert(uuid, path);
        m_lruOrder.append(uuid);
        emit cacheSizeChanged();
        emit screenshotReady(uuid, path);
    });
    QObject::connect(worker, &PipeWireCaptureWorker::failed,
                     this, [this, worker](const QString &uuid) {
        m_pendingUuids.remove(uuid);
        emit screenshotFailed(uuid);
        worker->deleteLater();
    });
    QObject::connect(worker, &PipeWireCaptureWorker::captured,
                     worker, &QObject::deleteLater);

    worker->capture(nodeId, windowUuid, filePath);
}

void WindowScreenshotProvider::ping(const QString &msg)
{
    qDebug() << "WindowScreenshotProvider::ping:" << msg;
}

void WindowScreenshotProvider::captureWindow(const QString &windowUuid)
{
    if (windowUuid.isEmpty()) {
        return;
    }

    // Return cached entry immediately if we already have one
    if (m_cache.contains(windowUuid)) {
        // Update LRU: move to end
        m_lruOrder.removeAll(windowUuid);
        m_lruOrder.append(windowUuid);
        qDebug() << "WindowScreenshotProvider: cache hit for" << windowUuid;
        emit screenshotReady(windowUuid, m_cache.value(windowUuid));
        return;
    }

    // Don't capture the same window again while a capture is pending
    if (m_pendingUuids.contains(windowUuid)) {
        return;
    }

    qDebug() << "WindowScreenshotProvider: captureWindow called for" << windowUuid;
    doCaptureWindow(windowUuid);
}

QString WindowScreenshotProvider::cachedScreenshot(const QString &windowUuid) const
{
    return m_cache.value(windowUuid, QString());
}

bool WindowScreenshotProvider::hasScreenshot(const QString &windowUuid) const
{
    if (!m_cache.contains(windowUuid)) {
        return false;
    }
    return QFile::exists(m_cache.value(windowUuid));
}

void WindowScreenshotProvider::clearCache()
{
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        QFile::remove(it.value());
    }
    m_cache.clear();
    m_lruOrder.clear();
    emit cacheSizeChanged();
}

void WindowScreenshotProvider::evictWindow(const QString &windowUuid)
{
    if (m_cache.contains(windowUuid)) {
        QFile::remove(m_cache.value(windowUuid));
        m_cache.remove(windowUuid);
        m_lruOrder.removeAll(windowUuid);
        emit cacheSizeChanged();
    }
}

int WindowScreenshotProvider::cacheSize() const
{
    return m_cache.size();
}

int WindowScreenshotProvider::maxCacheSize() const
{
    return m_maxCacheSize;
}

void WindowScreenshotProvider::setMaxCacheSize(int size)
{
    if (m_maxCacheSize != size) {
        m_maxCacheSize = size;
        emit maxCacheSizeChanged();
        while (m_cache.size() > m_maxCacheSize) {
            evictLru();
        }
    }
}

void WindowScreenshotProvider::doCaptureWindow(const QString &windowUuid)
{
    // Use the XDG Desktop Portal Screenshot API instead of KWin ScreenShot2.
    // The portal runs in a separate process (xdg-desktop-portal-kde) and
    // handles all the PipeWire/Wayland complexity; latte-dock's UI thread
    // is never blocked.

    qDebug() << "WindowScreenshotProvider: portal capture for" << windowUuid;

    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.portal.Desktop"),
        QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.Screenshot"),
        QStringLiteral("Screenshot"));

    // parent_window: empty string (no parent)
    msg << QString();

    // options: interactive=false, modal=false
    QVariantMap options;
    options[QStringLiteral("interactive")] = false;
    options[QStringLiteral("modal")] = false;
    msg << options;

    auto pendingCall = QDBusConnection::sessionBus().asyncCall(msg);
    auto *watcher = new QDBusPendingCallWatcher(pendingCall, this);
    watcher->setProperty("screenshotUuid", windowUuid);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this,
                     [this](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<QDBusObjectPath> reply = *w;
        const QString uuid = w->property("screenshotUuid").toString();
        w->deleteLater();

        if (reply.isError()) {
            qWarning() << "WindowScreenshotProvider: portal call failed for" << uuid
                       << ":" << reply.error().message();
            m_pendingUuids.remove(uuid);
            emit screenshotFailed(uuid);
            return;
        }

        QDBusObjectPath requestPath = reply.value();
        qDebug() << "WindowScreenshotProvider: portal request path" << requestPath.path() << "for" << uuid;

        // Listen for the Response signal on the request object
        QDBusConnection::sessionBus().connect(
            QStringLiteral("org.freedesktop.portal.Desktop"),
            requestPath.path(),
            QStringLiteral("org.freedesktop.portal.Request"),
            QStringLiteral("Response"),
            this,
            SLOT(onPortalResponse(uint, QVariantMap)));

        // Store mapping from request path to uuid
        m_portalRequests.insert(requestPath.path(), uuid);
    });
}

void WindowScreenshotProvider::onReadData(int fd)
{
    auto it = m_pendingByFd.find(fd);
    if (it == m_pendingByFd.end()) {
        return;
    }

    PendingCapture &pending = it.value();

    char buf[65536];
    qint64 bytesRead = ::read(fd, buf, sizeof(buf));

    if (bytesRead > 0) {
        pending.buffer.append(buf, static_cast<int>(bytesRead));
    } else {
        // EOF (0) or error (<0): transfer complete or failed
        pending.notifier->setEnabled(false);

        if (bytesRead == 0 && !pending.buffer.isEmpty()) {
            qDebug() << "WindowScreenshotProvider: received" << pending.buffer.size() << "bytes for" << pending.uuid;
            finishCapture(pending.uuid, pending.buffer);
        } else {
            qWarning() << "WindowScreenshotProvider: capture failed for" << pending.uuid << "bytesRead:" << bytesRead;
            m_pendingUuids.remove(pending.uuid);
            emit screenshotFailed(pending.uuid);
        }

        delete pending.notifier;
        ::close(fd);
        m_pendingByFd.remove(fd);
    }
}

void WindowScreenshotProvider::onDbusReply(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;

    const QString uuid = watcher->property("screenshotUuid").toString();
    int readFd = watcher->property("screenshotReadFd").toInt();

    if (reply.isError()) {
        qWarning() << "WindowScreenshotProvider: D-Bus error for" << uuid << ":" << reply.error().message();
        auto it = m_pendingByFd.find(readFd);
        if (it != m_pendingByFd.end()) {
            delete it.value().notifier;
            ::close(readFd);
            m_pendingByFd.remove(readFd);
        }
        m_pendingUuids.remove(uuid);
        emit screenshotFailed(uuid);
    } else {
        qDebug() << "WindowScreenshotProvider: D-Bus call succeeded for" << uuid;
    }

    watcher->deleteLater();
}

void WindowScreenshotProvider::finishCapture(const QString &uuid, const QByteArray &imageData)
{
    m_pendingUuids.remove(uuid);

    // Evict old entry if it exists
    if (m_cache.contains(uuid)) {
        QFile::remove(m_cache.value(uuid));
        m_lruOrder.removeAll(uuid);
    }

    // Ensure we don't exceed max cache size
    while (m_cache.size() >= m_maxCacheSize) {
        evictLru();
    }

    // Write to a file in the cache directory
    QString filePath = m_cacheDir + QLatin1Char('/') + uuid + QLatin1String(".png");
    // Sanitize the filename: UUIDs from KWin may contain '/' characters
    filePath.replace(QLatin1Char('/'), QLatin1Char('_'));

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(imageData);
        file.close();

        m_cache.insert(uuid, filePath);
        m_lruOrder.append(uuid);
        emit cacheSizeChanged();
        emit screenshotReady(uuid, filePath);
    } else {
        emit screenshotFailed(uuid);
    }
}

void WindowScreenshotProvider::onPortalResponse(uint response, const QVariantMap &results)
{
    Q_UNUSED(response);
    Q_UNUSED(results);
    qDebug() << "WindowScreenshotProvider::onPortalResponse" << response << results;
}

void WindowScreenshotProvider::evictLru()
{
    if (m_lruOrder.isEmpty()) {
        return;
    }

    const QString oldest = m_lruOrder.takeFirst();
    if (m_cache.contains(oldest)) {
        QFile::remove(m_cache.value(oldest));
        m_cache.remove(oldest);
    }
}

} // namespace Tasks
} // namespace Latte
