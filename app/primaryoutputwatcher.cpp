/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "primaryoutputwatcher.h"

#include <QDebug>
#include <QGuiApplication>
#include <QScreen>

#include "qwayland-kde-primary-output-v1.h"
#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/registry.h>

class WaylandPrimaryOutput : public QObject, public QtWayland::kde_primary_output_v1
{
    Q_OBJECT
public:
    WaylandPrimaryOutput(struct ::wl_registry *registry, int id, int version, QObject *parent)
        : QObject(parent)
        , QtWayland::kde_primary_output_v1(registry, id, version)
    {
    }

    void kde_primary_output_v1_primary_output(const QString &outputName) override
    {
        Q_EMIT primaryOutputChanged(outputName);
    }

Q_SIGNALS:
    void primaryOutputChanged(const QString &outputName);
};

PrimaryOutputWatcher::PrimaryOutputWatcher(QObject *parent)
    : QObject(parent)
{
    if (qGuiApp->primaryScreen()) {
        m_primaryOutputName = qGuiApp->primaryScreen()->name();
    }

    connect(qGuiApp, &QGuiApplication::primaryScreenChanged, this, [this](QScreen *newPrimary) {
        if (newPrimary) {
            setPrimaryOutputName(newPrimary->name());
        }
    });

    setupRegistry();
}

void PrimaryOutputWatcher::setPrimaryOutputName(const QString &newOutputName)
{
    if (newOutputName != m_primaryOutputName) {
        const QString oldOutputName = m_primaryOutputName;
        m_primaryOutputName = newOutputName;
        Q_EMIT primaryOutputNameChanged(oldOutputName, newOutputName);
    }
}

void PrimaryOutputWatcher::setupRegistry()
{
    auto m_connection = KWayland::Client::ConnectionThread::fromApplication(this);
    if (!m_connection) {
        return;
    }

    // Asking for primaryOutputName() before this happened, will return qGuiApp->primaryScreen()->name() anyways, so set it so the primaryOutputNameChange will
    // have parameters that are coherent
    m_primaryOutputName = qGuiApp->primaryScreen()->name();
    m_registry = new KWayland::Client::Registry(this);
    connect(m_registry, &KWayland::Client::Registry::interfaceAnnounced, this, [this](const QByteArray &interface, quint32 name, quint32 version) {
        if (interface == WaylandPrimaryOutput::interface()->name) {
            auto m_outputManagement = new WaylandPrimaryOutput(m_registry->registry(), name, version, this);
            connect(m_outputManagement, &WaylandPrimaryOutput::primaryOutputChanged, this, [this](const QString &outputName) {
                m_primaryOutputWayland = outputName;
                // Only set the outputName when there's a QScreen attached to it
                if (screenForName(outputName)) {
                    setPrimaryOutputName(outputName);
                }
            });
        }
    });

    // In case the outputName was received before Qt reported the screen
    connect(qGuiApp, &QGuiApplication::screenAdded, this, [this](QScreen *screen) {
        if (screen->name() == m_primaryOutputWayland) {
            setPrimaryOutputName(m_primaryOutputWayland);
        }
    });

    m_registry->create(m_connection);
    m_registry->setup();
}

QScreen *PrimaryOutputWatcher::screenForName(const QString &outputName) const
{
    const auto screens = qGuiApp->screens();
    for (auto screen : screens) {
        if (screen->name() == outputName) {
            return screen;
        }
    }
    return nullptr;
}

QScreen *PrimaryOutputWatcher::primaryScreen() const
{
    auto screen = screenForName(m_primaryOutputName);
    if (!screen) {
        qDebug() << "PrimaryOutputWatcher: Could not find primary screen:" << m_primaryOutputName;
        return qGuiApp->primaryScreen();
    }
    return screen;
}

#include "primaryoutputwatcher.moc"
