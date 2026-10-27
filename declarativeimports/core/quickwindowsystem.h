/*
    SPDX-FileCopyrightText: 2016 Smith AR <audoban@openmailbox.org>
    SPDX-FileCopyrightText: 2016 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef QUICKWINDOWSYSTEM_H
#define QUICKWINDOWSYSTEM_H

// Qt
#include <QObject>
#include <QtQml/qqmlregistration.h>

namespace Latte {

/**
 * @brief The QuickWindowSystem class,
 * is a tiny class that provide basic information of WindowSystem
 */
class QuickWindowSystem final : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(WindowSystem)
    QML_SINGLETON

    Q_PROPERTY(bool compositingActive READ compositingActive NOTIFY compositingChanged FINAL)
    Q_PROPERTY(bool isPlatformWayland READ isPlatformWayland NOTIFY isPlatformWaylandChanged FINAL)

public:
    explicit QuickWindowSystem(QObject *parent = nullptr);
    ~QuickWindowSystem() override;

    bool compositingActive() const;
    bool isPlatformWayland() const;

Q_SIGNALS:
    void compositingChanged();
    void isPlatformWaylandChanged();

private:
    bool m_compositing{true};
};

}

#endif // QUICKWINDOWSYSTEM_H
