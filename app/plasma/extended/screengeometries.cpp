/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <latte_debug.h>
#include "screengeometries.h"

//!local
#include "../../lattecorona.h"
#include "../../screenpool.h"
#include "../../view/view.h"
#include "../../layout/genericlayout.h"
#include "../../layouts/manager.h"
#include "../../settings/universalsettings.h"

// Qt
#include <QDebug>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusPendingCallWatcher>
#include <QDBusServiceWatcher>


#define LATTESERVICE QStringLiteral("org.kde.lattedock")
#define PLASMASERVICE QStringLiteral("org.kde.plasmashell")
#define PLASMASTRUTNAMESPACE QStringLiteral("org.kde.PlasmaShell.StrutManager")

#define PUBLISHINTERVAL 1000

namespace Latte {
namespace PlasmaExtended {

ScreenGeometries::ScreenGeometries(Latte::Corona *parent)
    : QObject(parent),
      m_corona(parent),
      m_plasmaServiceWatcher(new QDBusServiceWatcher(this))
{
    qDBusRegisterMetaType<QList<QRect>>();

    m_startupInitTimer.setInterval(2500);
    m_startupInitTimer.setSingleShot(true);
    connect(&m_startupInitTimer, &QTimer::timeout, this, &ScreenGeometries::init);

    m_publishTimer.setInterval(PUBLISHINTERVAL);
    m_publishTimer.setSingleShot(true);
    connect(&m_publishTimer, &QTimer::timeout, this, &ScreenGeometries::updateGeometries);

    m_startupInitTimer.start();

    m_plasmaServiceWatcher->setConnection(QDBusConnection::sessionBus());
    m_plasmaServiceWatcher->setWatchMode(QDBusServiceWatcher::WatchForRegistration);
    m_plasmaServiceWatcher->setWatchedServices(QStringList({PLASMASERVICE}));
    connect(m_plasmaServiceWatcher, &QDBusServiceWatcher::serviceRegistered, this, [this](const QString & serviceName) {
        if (serviceName == PLASMASERVICE && !m_plasmaInterfaceAvailable) {
            init();
        }
    });
}

ScreenGeometries::~ScreenGeometries()
{
    qCDebug(lattePlasma) << "Plasma Extended Screen Geometries :: Deleted...";
}

void ScreenGeometries::init()
{
    qCDebug(lattePlasma) << " PLASMA STRUTS MANAGER :: checking availability....";

    // Use asyncCall() to avoid blocking the event loop.
    QDBusMessage ping = QDBusMessage::createMethodCall(
                            QStringLiteral("org.freedesktop.DBus"),
                            QStringLiteral("/"),
                            QStringLiteral("org.freedesktop.DBus"),
                            QStringLiteral("NameHasOwner"));
    ping.setArguments({PLASMASERVICE});

    auto *watcher = new QDBusPendingCallWatcher(
        QDBusConnection::sessionBus().asyncCall(ping), this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this,
    [this](QDBusPendingCallWatcher *w) {
        w->deleteLater();
        QDBusMessage reply = w->reply();
        bool serviceavailable = (reply.type() == QDBusMessage::ReplyMessage
                                 && !reply.arguments().isEmpty()
                                 && reply.arguments().constFirst().toBool());

        qCDebug(lattePlasma) << "PLASMA STRUTS MANAGER :: interface availability :: " << serviceavailable;

        connect(m_corona->universalSettings(),
                &Latte::UniversalSettings::isAvailableGeometryBroadcastedToPlasmaChanged,
                this, &ScreenGeometries::onBroadcastToPlasmaChanged);

        if (serviceavailable) {
            m_plasmaInterfaceAvailable = true;
            onPlasmaInterfaceAvailable();
        }
    });
}

void ScreenGeometries::onPlasmaInterfaceAvailable()
{
    qCDebug(lattePlasma) << " PLASMA STRUTS MANAGER :: is available...";

    connect(m_corona, &Latte::Corona::availableScreenRectChangedFrom, this, &ScreenGeometries::availableScreenGeometryChangedFrom);
    connect(m_corona, &Latte::Corona::availableScreenRegionChangedFrom, this, &ScreenGeometries::availableScreenGeometryChangedFrom);

    connect(m_corona->layoutsManager()->synchronizer(), &Latte::Layouts::Synchronizer::centralLayoutsChanged, this, [this]() {
        m_publishTimer.start();
    });

    connect(m_corona->activitiesConsumer(), &KActivities::Consumer::currentActivityChanged, this, [this]() {
        if (m_corona->universalSettings()->isAvailableGeometryBroadcastedToPlasma()) {
            m_publishTimer.start();
        }
    });

    if (m_corona->universalSettings()->isAvailableGeometryBroadcastedToPlasma()) {
        m_publishTimer.start();
    }
}

bool ScreenGeometries::screenIsActive(const QString &screenName) const {
    for (QScreen *screen : qGuiApp->screens())
    {
        if (screen->name() == screenName) {
            return true;
        }
    }

    return false;
}

void ScreenGeometries::onBroadcastToPlasmaChanged()
{
    if (m_corona->universalSettings()->isAvailableGeometryBroadcastedToPlasma()) {
        m_publishTimer.start();
    } else {

        clearGeometries();
    }
}

void ScreenGeometries::setPlasmaAvailableScreenRect(const QString &screenName, const QRect &rect)
{
    QDBusMessage message = QDBusMessage::createMethodCall(PLASMASERVICE,
                           QStringLiteral("/StrutManager"),
                           PLASMASTRUTNAMESPACE,
                           QStringLiteral("setAvailableScreenRect"));
    QVariantList args;

    args << LATTESERVICE
         << screenName
         << rect;

    message.setArguments(args);
    QDBusConnection::sessionBus().call(message, QDBus::NoBlock);
}

void ScreenGeometries::setPlasmaAvailableScreenRegion(const QString &screenName, const QRegion &region)
{
    QDBusMessage message = QDBusMessage::createMethodCall(PLASMASERVICE,
                           QStringLiteral("/StrutManager"),
                           PLASMASTRUTNAMESPACE,
                           QStringLiteral("setAvailableScreenRegion"));

    QVariant regionvariant;

    QList<QRect> rects;

    if (!region.isNull()) {
        //! transform QRegion to QList<QRect> in order to be sent through dbus
        for (const auto &rect : region) {
            rects << rect;
        }
    } else {
        rects << QRect();
    }

    regionvariant = QVariant::fromValue(rects);

    QVariantList args;

    args << LATTESERVICE
         << screenName
         << regionvariant;

    message.setArguments(args);
    QDBusConnection::sessionBus().call(message, QDBus::NoBlock);
}

void ScreenGeometries::clearGeometries()
{
    if (!m_plasmaInterfaceAvailable) {
        return;
    }

    for (QScreen *screen : qGuiApp->screens()) {
        QString scrName = screen->name();
        int scrId = m_corona->screenPool()->id(screen->name());

        if (m_corona->screenPool()->hasScreenId(scrId)) {
            setPlasmaAvailableScreenRect(scrName, QRect());
            setPlasmaAvailableScreenRegion(scrName, QRegion());
        }
    }

    m_lastAvailableRect.clear();
    m_lastAvailableRegion.clear();
}

void ScreenGeometries::updateGeometries()
{
    if (!m_plasmaInterfaceAvailable || !m_corona->universalSettings()->isAvailableGeometryBroadcastedToPlasma()) {
        return;
    }

    QStringList availableScreenNames;

    qCDebug(lattePlasma) << " PLASMA SCREEN GEOMETRIES, LAST AVAILABLE SCREEN RECTS :: " << m_lastAvailableRect;

    QStringList clearedScreenNames;

    //! check for available geometries changes
    for (QScreen *screen : qGuiApp->screens()) {
        QString scrName = screen->name();
        int scrId = m_corona->screenPool()->id(screen->name());

        qCDebug(lattePlasma) << " PLASMA SCREEN GEOMETRIES, SCREEN :: " << scrId << " - " << scrName;

        if (m_corona->screenPool()->hasScreenId(scrId)) {
            QRect availableRect = m_corona->availableScreenRectWithCriteria(scrId,
                                  QString(),
                                  m_ignoreModes,
                                  QList<Plasma::Types::Location>(),
                                  true,
                                  true);

            QRegion availableRegion = m_corona->availableScreenRegionWithCriteria(scrId,
                                      QString(),
                                      m_ignoreModes,
                                      QList<Plasma::Types::Location>(),
                                      true,
                                      true);

            bool clearedScreen = (availableRect == screen->geometry());

            if (!clearedScreen) {
                //! Disable checks because of the workaround concerning plasma desktop behavior
                if (!m_lastAvailableRect.contains(scrName) || m_lastAvailableRect[scrName] != availableRect) {
                    m_lastAvailableRect[scrName] = availableRect;
                    setPlasmaAvailableScreenRect(scrName, availableRect);
                    qCDebug(lattePlasma) << " PLASMA SCREEN GEOMETRIES, AVAILABLE RECT :: " << screen->name() << " : " << availableRect;
                }

                if (!m_lastAvailableRegion.contains(scrName) || m_lastAvailableRegion[scrName] != availableRegion) {
                    m_lastAvailableRegion[scrName] = availableRegion;
                    setPlasmaAvailableScreenRegion(scrName, availableRegion);
                    qCDebug(lattePlasma) << " PLASMA SCREEN GEOMETRIES, AVAILABLE REGION :: " << screen->name() << " : " << availableRegion;
                }
            } else {
                clearedScreenNames << scrName;
            }
        }

        availableScreenNames << scrName;
    }

    //! check for inactive screens that were published previously
    for (QString &lastScrName : m_lastScreenNames) {
        bool scractive = screenIsActive(lastScrName);

        if (!scractive || clearedScreenNames.contains(lastScrName)) {
            //! screen became inactive and its geometries could be unpublished
            setPlasmaAvailableScreenRect(lastScrName, QRect());
            setPlasmaAvailableScreenRegion(lastScrName, QRegion());

            m_lastAvailableRect.remove(lastScrName);
            m_lastAvailableRegion.remove(lastScrName);
        }

        if (!scractive) {
            qCDebug(lattePlasma) << " PLASMA SCREEN GEOMETRIES, INACTIVE SCREEN :: " << lastScrName;
        } else if (clearedScreenNames.contains(lastScrName)) {
            qCDebug(lattePlasma) << " PLASMA SCREEN GEOMETRIES, CLEARED SCREEN :: " << lastScrName;
        }
    }

    m_lastScreenNames = availableScreenNames;
}

void ScreenGeometries::availableScreenGeometryChangedFrom(Latte::View *origin)
{
    if (m_corona->universalSettings()->isAvailableGeometryBroadcastedToPlasma() &&  origin && origin->layout() && origin->layout()->isCurrent()) {
        m_publishTimer.start();
    }
}

}
}
