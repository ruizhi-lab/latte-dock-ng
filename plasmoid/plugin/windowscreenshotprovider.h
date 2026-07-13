/*
    SPDX-FileCopyrightText: 2026 Latte Dock Contributors
    SPDX-License-Identifier: GPL-2.0-or-later

    On-demand window screenshots via KWin's ScreenShot2 D-Bus API.

    KWin renders the window internally and returns the image as a Unix
    file descriptor — zero PipeWire, zero worker threads, zero jank.
    The result is cached as a PNG and delivered via screenshotReady().
*/

#ifndef WINDOWSCREENSHOTPROVIDER_H
#define WINDOWSCREENSHOTPROVIDER_H

#include <QObject>
#include <QMap>
#include <QSet>
#include <QString>

class QSocketNotifier;
class QDBusPendingCallWatcher;

namespace Latte {
namespace Tasks {

class WindowScreenshotProvider : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int cacheSize READ cacheSize NOTIFY cacheSizeChanged)
    Q_PROPERTY(int maxCacheSize READ maxCacheSize WRITE setMaxCacheSize NOTIFY maxCacheSizeChanged)

public:
    explicit WindowScreenshotProvider(QObject *parent = nullptr);
    ~WindowScreenshotProvider() override;

public Q_SLOTS:
    /// Request a screenshot from KWin for the given window UUID.
    /// Result is delivered asynchronously via screenshotReady().
    Q_INVOKABLE void captureWindow(const QString &windowUuid);

    /// Returns the cached screenshot file path for a window, or "".
    Q_INVOKABLE QString cachedScreenshot(const QString &windowUuid) const;

    /// Returns true if a cached screenshot exists for this window.
    Q_INVOKABLE bool hasScreenshot(const QString &windowUuid) const;

    /// Returns true if a capture is currently in progress.
    Q_INVOKABLE bool hasPending(const QString &windowUuid) const;

    /// Diagnostic
    Q_INVOKABLE void ping(const QString &msg);

    /// Clear all cached screenshots.
    Q_INVOKABLE void clearCache();

    /// Remove screenshot for a specific window.
    Q_INVOKABLE void evictWindow(const QString &windowUuid);

    int cacheSize() const;
    int maxCacheSize() const;
    void setMaxCacheSize(int size);

Q_SIGNALS:
    void screenshotReady(const QString &windowUuid, const QString &filePath);
    void screenshotFailed(const QString &windowUuid);
    void cacheSizeChanged();
    void maxCacheSizeChanged();

private Q_SLOTS:
    void onReadData(int fd);

private:
    struct PendingCapture {
        QString uuid;
        int readFd = -1;
        QSocketNotifier *notifier = nullptr;
        QByteArray buffer;
    };

    void doCaptureWindow(const QString &windowUuid);
    void finishCapture(const QString &uuid, const QByteArray &imageData);
    void evictLru();

    QString m_cacheDir;
    QMap<QString, QString> m_cache;        // uuid → file path
    QSet<QString> m_pendingUuids;           // uuids being captured
    QMap<int, PendingCapture> m_pendingByFd; // fd → capture
    QStringList m_lruOrder;
    int m_maxCacheSize = 20;
};

} // namespace Tasks
} // namespace Latte

#endif // WINDOWSCREENSHOTPROVIDER_H
