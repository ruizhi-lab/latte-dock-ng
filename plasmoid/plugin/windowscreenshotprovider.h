/*
    SPDX-FileCopyrightText: 2026 Latte Dock Contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef WINDOWSCREENSHOTPROVIDER_H
#define WINDOWSCREENSHOTPROVIDER_H

// Qt
#include <QObject>
#include <QMap>
#include <QSet>
#include <QString>

class QSocketNotifier;
class QDBusPendingCallWatcher;

namespace Latte
{
namespace Tasks
{

/**
 * @brief Provides on-demand window screenshots via KWin's ScreenShot2 D-Bus API.
 *
 * Screenshots are taken asynchronously and cached to temporary files.
 * When a screenshot is ready, the screenshotReady() signal is emitted.
 *
 * This avoids the performance issues of PipeWire live streaming for
 * window previews — each hover triggers a lightweight D-Bus screenshot
 * instead of a continuous PipeWire connection.
 */
class WindowScreenshotProvider : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int cacheSize READ cacheSize NOTIFY cacheSizeChanged)
    Q_PROPERTY(int maxCacheSize READ maxCacheSize WRITE setMaxCacheSize NOTIFY maxCacheSizeChanged)

public:
    explicit WindowScreenshotProvider(QObject *parent = nullptr);
    ~WindowScreenshotProvider() override;

public Q_SLOTS:
    /// Called when the XDG Desktop Portal responds to a Screenshot request.
    void onPortalResponse(uint response, const QVariantMap &results);

    /**
     * Request a screenshot for the given window UUID.
     * The result is delivered asynchronously via screenshotReady() signal.
     */
    Q_INVOKABLE void captureWindow(const QString &windowUuid);

    /**
     * Request a screenshot via PipeWire (worker thread).
     * PipeWire stream creation runs in a background thread so the UI
     * never blocks.
     */
    Q_INVOKABLE void captureViaPipeWire(uint nodeId, const QString &windowUuid);

    /**
     * Cache a frame image for a window UUID.
     * The image is saved as PNG and can be retrieved via cachedScreenshot().
     */
    Q_INVOKABLE void cacheFrame(const QString &windowUuid, const QVariant &image);

    /**
     * Returns the cached screenshot file path for a window, or empty string.
     */
    Q_INVOKABLE QString cachedScreenshot(const QString &windowUuid) const;

    /**
     * Returns true if a cached screenshot exists for this window.
     */
    Q_INVOKABLE bool hasScreenshot(const QString &windowUuid) const;

    /// Returns true if a capture is currently in progress for this window.
    Q_INVOKABLE bool hasPending(const QString &windowUuid) const;

    /// Diagnostic
    Q_INVOKABLE void ping(const QString &msg);

    /**
     * Clear all cached screenshots.
     */
    Q_INVOKABLE void clearCache();

    /**
     * Remove screenshot for a specific window.
     */
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
    void onDbusReply(QDBusPendingCallWatcher *watcher);

private:
    struct PendingCapture {
        QString uuid;
        int readFd = -1;
        QSocketNotifier *notifier = nullptr;
        QByteArray buffer;
    };

    void doCaptureWindow(const QString &windowUuid);
    void onReadData(int fd);
    void finishCapture(const QString &uuid, const QByteArray &imageData);
    void evictLru();

    QString m_cacheDir;
    QMap<QString, QString> m_cache;       // uuid → file path
    QSet<QString> m_pendingUuids;          // uuids currently being captured
    QMap<int, PendingCapture> m_pendingByFd; // fd → pending capture
    QStringList m_lruOrder;                // LRU eviction order
    int m_maxCacheSize = 20;
    QMap<QString, QString> m_portalRequests; // request path → uuid
};

} // namespace Tasks
} // namespace Latte

#endif // WINDOWSCREENSHOTPROVIDER_H
