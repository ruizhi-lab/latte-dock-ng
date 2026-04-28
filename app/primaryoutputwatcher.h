/*
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PRIMARYOUTPUTWATCHER_H
#define PRIMARYOUTPUTWATCHER_H

#include <QObject>

namespace KWayland
{
namespace Client
{
class Registry;
class ConnectionThread;
}
}

class QScreen;

class PrimaryOutputWatcher : public QObject
{
    Q_OBJECT
public:
    PrimaryOutputWatcher(QObject *parent);
    QScreen *primaryScreen() const;
    QScreen *screenForName(const QString &outputName) const;

Q_SIGNALS:
    void primaryOutputNameChanged(const QString &oldOutputName, const QString &newOutputName);

protected:
    friend class WaylandOutputDevice;
    void setPrimaryOutputName(const QString &outputName);

private:
    void setupRegistry();

    // All
    QString m_primaryOutputName;

    // Wayland
    KWayland::Client::Registry *m_registry = nullptr;
    QString m_primaryOutputWayland;
};

#endif // PRIMARYOUTPUTWATCHER_H
