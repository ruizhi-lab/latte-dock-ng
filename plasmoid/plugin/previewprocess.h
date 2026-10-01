/* SPDX-FileCopyrightText: 2026 Latte Dock Contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QRect>
#include <QString>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
namespace Latte::Tasks {
class PreviewProcess : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PreviewProcess)
    Q_PROPERTY(bool enabled READ enabled NOTIFY stateChanged)
    Q_PROPERTY(bool visible READ visible NOTIFY stateChanged)
    Q_PROPERTY(bool hovered READ hovered NOTIFY stateChanged)
public:
    explicit PreviewProcess(QObject *parent = nullptr);
    // Dependency injection keeps protocol tests deterministic: production
    // still resolves the installed sibling helper, while autotests use a
    // small process that speaks the same bounded JSON-lines protocol.
    PreviewProcess(const QString &executable, bool enabled, QObject *parent = nullptr);
    ~PreviewProcess() override;
    bool enabled() const { return m_enabled; }
    bool visible() const { return m_visible; }
    bool hovered() const { return m_hovered; }
    Q_INVOKABLE void show(const QVariantList &windows, const QRect &anchor, int edge);
    // Geometry-only update used every animation frame while the preview is
    // visible. Keeps the surface glued to the icon during parabolic zoom
    // without re-serializing the (unchanged) window list on every frame.
    Q_INVOKABLE void move(const QRect &anchor, int edge);
    Q_INVOKABLE void hide();
Q_SIGNALS:
    void stateChanged();
    void activateRequested(const QString &uuid);
    void closeRequested(const QString &uuid);
private:
    void start();
    void send();
    void read();
    void fail();
    void onFinished();
    void discardProcess();
    void shutdownProcess();
    bool m_enabled{false};
    // Cleared at the start of every hover: a single helper failure hides the
    // preview for the current hover but must not permanently disable the
    // feature if the helper was only restarted or killed by the user.
    bool m_hoverBlocked{false};
    // True while a reply for the current generation is outstanding; used to
    // distinguish an idle-lease exit from a crash during active use.
    bool m_pending{false};
    bool m_closing{false};
    int m_failures{0};
    bool m_visible{false};
    bool m_hovered{false};
    QString m_executable;
    QProcess *m_process{nullptr};
    QTimer m_watchdog;
    QByteArray m_input;
    QByteArray m_request;
    QByteArray m_sent;
    QVariantList m_windows;
    int m_serial{0};
};
}
