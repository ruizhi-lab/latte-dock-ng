/*
    SPDX-FileCopyrightText: 2026 Latte Dock Contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "pipecaptureworker.h"

// KPipeWire
#include <KPipeWire/pipewiresourcestream.h>

// Qt
#include <QDebug>
#include <QFile>
#include <QImage>
#include <QTimer>

#include <pipewire/pipewire.h>

namespace Latte {
namespace Tasks {

// Lives on the worker thread. Owns the PipeWireSourceStream and
// drives the capture lifecycle.
class CaptureDriver : public QObject
{
    Q_OBJECT
public:
    uint nodeId = 0;
    QString uuid;
    QString outputPath;

public Q_SLOTS:
    void start();
    void onFrame(const PipeWireFrame &frame);

signals:
    void finished(const QString &uuid, const QString &path);
    void errored(const QString &uuid);
};

void CaptureDriver::start()
{
    qDebug() << "CaptureDriver::start nodeId=" << nodeId << "uuid=" << uuid;

    auto *stream = new PipeWireSourceStream(this);
    stream->setAllowDmaBuf(false);

    QObject::connect(stream, &PipeWireSourceStream::frameReceived,
                     this, &CaptureDriver::onFrame);

    if (!stream->createStream(nodeId, 0)) {
        qWarning() << "CaptureDriver: createStream failed for" << uuid;
        emit errored(uuid);
        return;
    }

    stream->setActive(true);

    // Safety timeout
    QTimer::singleShot(5000, this, [this, stream]() {
        qWarning() << "CaptureDriver: timeout for" << uuid;
        stream->setActive(false);
        emit errored(uuid);
    });
}

void CaptureDriver::onFrame(const PipeWireFrame &frame)
{
    // Sender is the PipeWireSourceStream
    auto *stream = qobject_cast<PipeWireSourceStream *>(sender());
    if (!stream || !frame.dataFrame) {
        emit errored(uuid);
        return;
    }

    stream->setActive(false);

    QImage img = frame.dataFrame->toImage();
    if (img.isNull()) {
        qWarning() << "CaptureDriver: null image for" << uuid;
        emit errored(uuid);
        return;
    }

    QFile f(outputPath);
    if (!f.open(QIODevice::WriteOnly) || !img.save(&f, "PNG")) {
        qWarning() << "CaptureDriver: cannot write" << outputPath;
        emit errored(uuid);
        return;
    }

    qDebug() << "CaptureDriver: saved frame" << img.size() << "to" << outputPath;
    emit finished(uuid, outputPath);
}

// ---- PipeWireCaptureWorker ----------------------------------------

PipeWireCaptureWorker::PipeWireCaptureWorker(QObject *parent)
    : QObject(parent)
{
    pw_init(nullptr, nullptr);
    m_thread.setObjectName(QStringLiteral("pw-capture"));
}

PipeWireCaptureWorker::~PipeWireCaptureWorker()
{
    m_thread.quit();
    m_thread.wait(5000);
}

void PipeWireCaptureWorker::capture(uint nodeId, const QString &windowUuid,
                                     const QString &outputPath)
{
    if (nodeId == 0 || windowUuid.isEmpty()) {
        emit failed(windowUuid);
        return;
    }

    if (!m_thread.isRunning())
        m_thread.start();

    auto *driver = new CaptureDriver();
    driver->nodeId = nodeId;
    driver->uuid = windowUuid;
    driver->outputPath = outputPath;
    driver->moveToThread(&m_thread);

    // Forward signals from worker thread → main thread
    QObject::connect(driver, &CaptureDriver::finished,
                     this, &PipeWireCaptureWorker::captured,
                     Qt::QueuedConnection);
    QObject::connect(driver, &CaptureDriver::errored,
                     this, &PipeWireCaptureWorker::failed,
                     Qt::QueuedConnection);

    // Auto-cleanup
    QObject::connect(driver, &CaptureDriver::finished,
                     driver, &QObject::deleteLater);
    QObject::connect(driver, &CaptureDriver::errored,
                     driver, &QObject::deleteLater);

    // Kick off the capture on the worker thread
    QMetaObject::invokeMethod(driver, "start", Qt::QueuedConnection);
}

} // namespace Tasks
} // namespace Latte

#include "pipecaptureworker.moc"
