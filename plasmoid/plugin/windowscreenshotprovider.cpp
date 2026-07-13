/*
    SPDX-FileCopyrightText: 2026 Latte Dock Contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "windowscreenshotprovider.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusUnixFileDescriptor>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QSocketNotifier>
#include <QStandardPaths>

#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>

namespace Latte {
namespace Tasks {

WindowScreenshotProvider::WindowScreenshotProvider(QObject *parent)
    : QObject(parent)
{
    m_cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                 + QStringLiteral("/window-screenshots");
    QDir().mkpath(m_cacheDir);
}

WindowScreenshotProvider::~WindowScreenshotProvider()
{
    for (auto it = m_pendingByFd.begin(); it != m_pendingByFd.end(); ++it) {
        delete it.value().notifier;
        ::close(it.key());
    }
    m_pendingByFd.clear();
}

void WindowScreenshotProvider::captureWindow(const QString &windowUuid)
{
    if (windowUuid.isEmpty())
        return;

    // Fast path: cached
    if (m_cache.contains(windowUuid)) {
        m_lruOrder.removeAll(windowUuid);
        m_lruOrder.append(windowUuid);
        emit screenshotReady(windowUuid, m_cache.value(windowUuid));
        return;
    }

    // Don't duplicate
    if (m_pendingUuids.contains(windowUuid))
        return;

    m_pendingUuids.insert(windowUuid);
    doCaptureWindow(windowUuid);
}

QString WindowScreenshotProvider::cachedScreenshot(const QString &windowUuid) const
{
    return m_cache.value(windowUuid, QString());
}

bool WindowScreenshotProvider::hasScreenshot(const QString &windowUuid) const
{
    return m_cache.contains(windowUuid) && QFile::exists(m_cache.value(windowUuid));
}

bool WindowScreenshotProvider::hasPending(const QString &windowUuid) const
{
    return m_pendingUuids.contains(windowUuid);
}

void WindowScreenshotProvider::ping(const QString &msg)
{
    qDebug() << "WindowScreenshotProvider::ping:" << msg;
}

void WindowScreenshotProvider::clearCache()
{
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it)
        QFile::remove(it.value());
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
        while (m_cache.size() > m_maxCacheSize)
            evictLru();
    }
}

// ---------------------------------------------------------------------------
// KWin ScreenShot2 D-Bus capture
// ---------------------------------------------------------------------------

void WindowScreenshotProvider::doCaptureWindow(const QString &windowUuid)
{
    // Use KWin's ScreenShot2 D-Bus API for fast, per-window screenshots.
    // The caller provides a pipe; KWin writes the image data into it.
    //
    // D-Bus signature:
    //   CaptureWindow(s handle, a{sv} options, h pipe) → a{sv} results

    // Use socketpair instead of pipe — larger buffer, no blocking issues
    int fds[2];
    if (::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0, fds) < 0) {
        qWarning() << "WindowScreenshotProvider: socketpair failed for" << windowUuid;
        m_pendingUuids.remove(windowUuid);
        emit screenshotFailed(windowUuid);
        return;
    }

    int readFd = fds[0];
    int writeFd = fds[1];

    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.kde.KWin"),
        QStringLiteral("/org/kde/KWin/ScreenShot2"),
        QStringLiteral("org.kde.KWin.ScreenShot2"),
        QStringLiteral("CaptureWindow"));

    QVariantMap options;
    options[QStringLiteral("include-cursor")] = false;
    options[QStringLiteral("include-decoration")] = true;

    // QDBusUnixFileDescriptor duplicates writeFd during D-Bus marshalling.
    // Keep writeFd alive until the reply confirms KWin has its copy.
    msg << windowUuid << options;
    msg << QVariant::fromValue(QDBusUnixFileDescriptor(writeFd));

    auto *pending = new QDBusPendingCallWatcher(
        QDBusConnection::sessionBus().asyncCall(msg), this);
    pending->setProperty("screenshotUuid", windowUuid);
    pending->setProperty("screenshotReadFd", readFd);
    pending->setProperty("screenshotWriteFd", writeFd);

    QObject::connect(pending, &QDBusPendingCallWatcher::finished,
                     this, [this](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<QVariantMap> reply = *w;
        const QString uuid = w->property("screenshotUuid").toString();
        int rfd = w->property("screenshotReadFd").toInt();
        int wfd = w->property("screenshotWriteFd").toInt();
        w->deleteLater();

        // Close our write end — KWin has its own dup.  This lets the
        // read end see EOF once KWin finishes writing.
        ::close(wfd);

        if (reply.isError()) {
            qWarning() << "KWin ScreenShot2 D-Bus error for" << uuid
                       << ":" << reply.error().message();
            m_pendingUuids.remove(uuid);
            ::close(rfd);
            emit screenshotFailed(uuid);
            return;
        }

        // KWin has written the screenshot into our socket; start reading
        PendingCapture pc;
        pc.uuid = uuid;
        pc.readFd = rfd;
        pc.notifier = new QSocketNotifier(rfd, QSocketNotifier::Read, this);
        QObject::connect(pc.notifier, &QSocketNotifier::activated,
                         this, [this](int f) { onReadData(f); });

        m_pendingByFd.insert(rfd, pc);

        qDebug() << "WindowScreenshotProvider: KWin capture started for" << uuid;
    });
}

void WindowScreenshotProvider::onReadData(int fd)
{
    auto it = m_pendingByFd.find(fd);
    if (it == m_pendingByFd.end())
        return;

    PendingCapture &pc = it.value();
    char buf[65536];
    qint64 n = ::read(fd, buf, sizeof(buf));

    if (n > 0) {
        pc.buffer.append(buf, static_cast<int>(n));
    } else {
        // EOF or error
        pc.notifier->setEnabled(false);

        if (n == 0 && !pc.buffer.isEmpty()) {
            qDebug() << "WindowScreenshotProvider: received"
                     << pc.buffer.size() << "bytes for" << pc.uuid;
            finishCapture(pc.uuid, pc.buffer);
        } else {
            qWarning() << "WindowScreenshotProvider: read failed for"
                       << pc.uuid << "ret:" << n;
            m_pendingUuids.remove(pc.uuid);
            emit screenshotFailed(pc.uuid);
        }

        delete pc.notifier;
        ::close(fd);
        m_pendingByFd.erase(it);
    }
}

void WindowScreenshotProvider::finishCapture(const QString &uuid,
                                              const QByteArray &imageData)
{
    m_pendingUuids.remove(uuid);

    // Evict old
    if (m_cache.contains(uuid)) {
        QFile::remove(m_cache.value(uuid));
        m_lruOrder.removeAll(uuid);
    }
    while (m_cache.size() >= m_maxCacheSize)
        evictLru();

    // Write PNG to cache
    QString filePath = m_cacheDir + QLatin1Char('/') + uuid + QLatin1String(".png");
    filePath.replace(QLatin1Char('/'), QLatin1Char('_'));

    // Try to decode + re-encode as PNG (KWin may send raw pixel data or a
    // complete image format — try QImage loading first).
    QImage img = QImage::fromData(imageData);
    if (img.isNull()) {
        // If it's raw BGRA, try to interpret it as a common resolution
        qWarning() << "WindowScreenshotProvider: QImage::fromData failed for"
                   << uuid << "size:" << imageData.size();
        emit screenshotFailed(uuid);
        return;
    }

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        img.save(&file, "PNG");
        file.close();
        m_cache.insert(uuid, filePath);
        m_lruOrder.append(uuid);
        emit cacheSizeChanged();
        emit screenshotReady(uuid, filePath);
    } else {
        emit screenshotFailed(uuid);
    }
}

void WindowScreenshotProvider::evictLru()
{
    if (m_lruOrder.isEmpty())
        return;
    const QString oldest = m_lruOrder.takeFirst();
    if (m_cache.contains(oldest)) {
        QFile::remove(m_cache.value(oldest));
        m_cache.remove(oldest);
    }
}

} // namespace Tasks
} // namespace Latte
