/*
    SPDX-FileCopyrightText: 2026 Latte Dock Contributors
    SPDX-License-Identifier: GPL-2.0-or-later

    Background PipeWire frame capture.

    Runs the entire PipeWire stream lifecycle in a dedicated QThread so
    that stream creation, Wayland DMA-BUF negotiation, and format setup
    never block the UI.  Once a frame is captured it is saved as a PNG
    and the caller is notified via the captured() signal.
*/

#ifndef PIPEWIRECAPTUREWORKER_H
#define PIPEWIRECAPTUREWORKER_H

#include <QObject>
#include <QThread>
#include <QString>

namespace Latte {
namespace Tasks {

class PipeWireCaptureWorker : public QObject
{
    Q_OBJECT
public:
    explicit PipeWireCaptureWorker(QObject *parent = nullptr);
    ~PipeWireCaptureWorker() override;

    /// Request a single-frame capture from the given PipeWire node.
    /// Runs asynchronously; captured() or failed() is emitted on completion.
    void capture(uint nodeId, const QString &windowUuid,
                 const QString &outputPath);

signals:
    void captured(const QString &windowUuid, const QString &outputPath);
    void failed(const QString &windowUuid);

private:
    QThread m_thread;
};

} // namespace Tasks
} // namespace Latte

#endif // PIPEWIRECAPTUREWORKER_H
