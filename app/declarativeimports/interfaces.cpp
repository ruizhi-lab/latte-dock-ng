/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "interfaces.h"

#include <QDynamicPropertyChangeEvent>

#include <PlasmaQuick/AppletQuickItem>

namespace Latte{

Interfaces::Interfaces(QObject *parent)
    : QObject(parent)
{
    m_viewSyncTimer.setInterval(100);
    m_viewSyncTimer.setSingleShot(false);

    connect(&m_viewSyncTimer, &QTimer::timeout, this, [this]() {
        if (!m_plasmoid) {
            m_viewSyncTimer.stop();
            return;
        }

        updateView();

        if (m_view || ++m_viewSyncAttempts >= 50) {
            m_viewSyncTimer.stop();
        }
    });
}

QObject *Interfaces::globalShortcuts() const
{
    return m_globalShortcuts;
}

void Interfaces::setGlobalShortcuts(QObject *shortcuts)
{
    if (m_globalShortcuts == shortcuts) {
        return;
    }

    m_globalShortcuts = shortcuts;

    if (m_globalShortcuts) {
        connect(m_globalShortcuts, &QObject::destroyed, this, [&]() {
            setGlobalShortcuts(nullptr);
        });
    }

    Q_EMIT globalShortcutsChanged();
}

QObject *Interfaces::layoutsManager() const
{
    return m_layoutsManager;
}

void Interfaces::setLayoutsManager(QObject *manager)
{
    if (m_layoutsManager == manager) {
        return;
    }

    m_layoutsManager = manager;

    if (m_layoutsManager) {
        connect(m_layoutsManager, &QObject::destroyed, this, [&]() {
            setLayoutsManager(nullptr);
        });
    }

    Q_EMIT layoutsManagerChanged();
}

QObject *Interfaces::themeExtended() const
{
    return m_themeExtended;
}

void Interfaces::setThemeExtended(QObject *theme)
{
    if (m_themeExtended == theme) {
        return;
    }

    m_themeExtended = theme;

    if (m_themeExtended) {
        connect(m_themeExtended, &QObject::destroyed, this, [&]() {
            setThemeExtended(nullptr);
        });
    }

    Q_EMIT themeExtendedChanged();
}

QObject *Interfaces::universalSettings() const
{
    return m_universalSettings;
}

void Interfaces::setUniversalSettings(QObject *settings)
{
    if (m_universalSettings == settings) {
        return;
    }

    m_universalSettings = settings;

    if (m_universalSettings) {
        connect(m_universalSettings, &QObject::destroyed, this, [&]() {
            setUniversalSettings(nullptr);
        });
    }

    Q_EMIT universalSettingsChanged();
}

void Interfaces::updateView()
{
    QObject *source = activePropertySource();

    if (!source) {
        return;
    }

    QObject *resolvedView = source->property("_latte_view_object").value<QObject *>();

    if (!resolvedView && source != m_plasmoidInterface && m_plasmoidInterface) {
        resolvedView = m_plasmoidInterface->property("_latte_view_object").value<QObject *>();
    }

    setView(resolvedView);

    if (m_view) {
        m_viewSyncTimer.stop();
        m_viewSyncAttempts = 0;
    }
}

QObject *Interfaces::view() const
{
    return m_view;
}

void Interfaces::setView(QObject *view)
{
    if (m_view == view) {
        return;
    }

    m_view = view;

    if (m_view) {
        connect(m_view, &QObject::destroyed, this, [&]() {
            setView(nullptr);
        });
    }

    Q_EMIT viewChanged();
}

QObject *Interfaces::plasmoidInterface() const
{
    return m_plasmoid;
}

void Interfaces::setPlasmoidInterface(QObject *interface)
{
    if (m_plasmoidInterface == interface) {
        syncPlasmoidReference();
        syncPlasmoidObjects();
        return;
    }

    if (m_plasmoidInterface && m_plasmoidInterface != m_plasmoid) {
        m_plasmoidInterface->removeEventFilter(this);
    }
    if (m_plasmoid) {
        m_plasmoid->removeEventFilter(this);
    }

    m_plasmoidInterface = interface;

    if (m_plasmoidInterface) {
        m_plasmoidInterface->installEventFilter(this);

        connect(m_plasmoidInterface, &QObject::destroyed, this, [this]() {
            if (m_plasmoid && m_plasmoid != m_plasmoidInterface) {
                m_plasmoid->removeEventFilter(this);
            }

            m_plasmoidInterface = nullptr;
            m_plasmoid = nullptr;
            m_viewSyncTimer.stop();
            m_viewSyncAttempts = 0;
            setView(nullptr);
            setGlobalShortcuts(nullptr);
            setLayoutsManager(nullptr);
            setThemeExtended(nullptr);
            setUniversalSettings(nullptr);
        });
    }

    syncPlasmoidReference();

    syncPlasmoidObjects();
    Q_EMIT interfaceChanged();
}

bool Interfaces::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() != QEvent::DynamicPropertyChange) {
        return QObject::eventFilter(watched, event);
    }

    auto *changeEvent = static_cast<QDynamicPropertyChangeEvent *>(event);
    const QByteArray propName = changeEvent->propertyName();

    if (watched == m_plasmoidInterface && propName == "_plasma_graphicObject") {
        syncPlasmoidReference();
        syncPlasmoidObjects();
    } else if (watched == m_plasmoid) {
        if (propName == "_latte_view_object"
                || propName == "_latte_globalShortcuts_object"
                || propName == "_latte_layoutsManager_object"
                || propName == "_latte_themeExtended_object"
                || propName == "_latte_universalSettings_object") {
            syncPlasmoidObjects();
        }
    } else if (watched == m_plasmoidInterface) {
        if (propName == "_latte_view_object"
                || propName == "_latte_globalShortcuts_object"
                || propName == "_latte_layoutsManager_object"
                || propName == "_latte_themeExtended_object"
                || propName == "_latte_universalSettings_object") {
            syncPlasmoidObjects();
        }
    }

    return QObject::eventFilter(watched, event);
}

QObject *Interfaces::activePropertySource() const
{
    if (m_plasmoid) {
        return m_plasmoid;
    }

    return m_plasmoidInterface;
}

void Interfaces::syncPlasmoidReference()
{
    PlasmaQuick::AppletQuickItem *plasmoid = qobject_cast<PlasmaQuick::AppletQuickItem *>(m_plasmoidInterface);

    if (!plasmoid && m_plasmoidInterface) {
        QObject *graphicObject = m_plasmoidInterface->property("_plasma_graphicObject").value<QObject *>();
        plasmoid = qobject_cast<PlasmaQuick::AppletQuickItem *>(graphicObject);
    }

    if (m_plasmoid == plasmoid) {
        return;
    }

    if (m_plasmoid) {
        m_plasmoid->removeEventFilter(this);
    }

    m_plasmoid = plasmoid;

    if (m_plasmoid) {
        m_plasmoid->installEventFilter(this);
    }
}

void Interfaces::syncPlasmoidObjects()
{
    QObject *source = activePropertySource();

    if (!source) {
        return;
    }

    setGlobalShortcuts(source->property("_latte_globalShortcuts_object").value<QObject *>());
    setLayoutsManager(source->property("_latte_layoutsManager_object").value<QObject *>());
    setThemeExtended(source->property("_latte_themeExtended_object").value<QObject *>());
    setUniversalSettings(source->property("_latte_universalSettings_object").value<QObject *>());
    updateView();

    if (!m_view && !m_viewSyncTimer.isActive()) {
        m_viewSyncAttempts = 0;
        m_viewSyncTimer.start();
    }
}

}
