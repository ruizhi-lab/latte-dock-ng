/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "plasmapaneltracker.h"
#include <latte_debug.h>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScreen>
#include <QUuid>

namespace Latte::WindowSystem {
namespace {
//! Function-local statics, not namespace-scope QStrings: see app/data/contextmenudata.h.
const QString &service()
{
    static const QString value = QStringLiteral("org.kde.KWin");
    return value;
}

const QString &path()
{
    static const QString value = QStringLiteral("/Latte/PanelTracker");
    return value;
}

QDBusMessage
scriptingCall(const QString &method)
{ return QDBusMessage::createMethodCall(service(), QStringLiteral("/Scripting"), QStringLiteral("org.kde.kwin.Scripting"), method); }
}

PlasmaPanelTracker::PlasmaPanelTracker(QObject *parent)
  : QObject(parent)
  , m_pluginName(QStringLiteral("latte-panel-tracker-") + QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    if (!QDBusConnection::sessionBus().registerObject(path(), this, QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportAllSlots)) {
        qCWarning(latteWm) << "Cannot register Plasma panel geometry bridge";
        return;
    }
    m_timeout.setSingleShot(true);
    // Qt and KWin announce output changes independently. Revalidate the last
    // authoritative payload when Qt catches up, rather than losing hotplug data.
    const auto watchScreen = [this](QScreen *screen) {
        connect(screen, &QScreen::geometryChanged, this, &PlasmaPanelTracker::refreshGeometries);
        refreshGeometries();
    };
    for (const auto screens = qGuiApp->screens(); QScreen *screen : screens) {
        watchScreen(screen);
    }
    connect(qGuiApp, &QGuiApplication::screenAdded, this, watchScreen);
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, &PlasmaPanelTracker::refreshGeometries);
    m_timeout.setInterval(5000);
    connect(&m_timeout, &QTimer::timeout, this, [] { qCWarning(latteWm) << "KWin panel tracker did not report geometry; panel overlap is not protected"; });
    auto watcher = new QDBusServiceWatcher(service(), QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange, this);
    connect(watcher, &QDBusServiceWatcher::serviceOwnerChanged, this, [this](const QString &, const QString &, const QString &) {
        clear();
        start();
    });
    QTimer::singleShot(0, this, &PlasmaPanelTracker::start);
}

PlasmaPanelTracker::~PlasmaPanelTracker()
{
    // No nested event loop at shutdown. Constraints disappear when our views
    // disappear, and unloading disconnects the script's workspace observers.
    auto message = scriptingCall(QStringLiteral("unloadScript"));
    message << m_pluginName;
    QDBusConnection::sessionBus().asyncCall(message, 2000);
    QDBusConnection::sessionBus().unregisterObject(path());
}

void
PlasmaPanelTracker::clear()
{
    ++m_generation;
    m_owner.clear();
    m_panels = {};
    m_timeout.stop();
    if (!m_geometries.isEmpty()) {
        m_geometries.clear();
        Q_EMIT changed();
    }
}

void
PlasmaPanelTracker::start()
{
    const int generation = ++m_generation;
    auto ownerCall =
      QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.DBus"), QStringLiteral("/org/freedesktop/DBus"), QStringLiteral("org.freedesktop.DBus"), QStringLiteral("GetNameOwner"));
    ownerCall << service();
    auto ownerWatcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(ownerCall, 2000), this);
    connect(ownerWatcher, &QDBusPendingCallWatcher::finished, this, [this, generation](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<QString> reply = *watcher;
        watcher->deleteLater();
        if (generation != m_generation || reply.isError()) {
            return;
        }
        m_owner = reply.value();
        QFile resource(QStringLiteral(":/latte/wm/plasmapaneltracker.js"));
        if (!resource.open(QIODevice::ReadOnly) || (!m_script.isOpen() && !m_script.open())) {
            qCWarning(latteWm) << "Cannot prepare KWin panel tracker script";
            return;
        }
        QByteArray script = resource.readAll();
        script.replace("@LATTE_BUS@", QDBusConnection::sessionBus().baseService().toUtf8());
        if (!m_script.resize(0) || !m_script.seek(0) || m_script.write(script) != script.size() || !m_script.flush()) {
            qCWarning(latteWm) << "Cannot write KWin panel tracker script";
            return;
        }
        auto message = scriptingCall(QStringLiteral("loadScript"));
        message << m_script.fileName() << m_pluginName;
        auto loadWatcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(message, 2000), this);
        connect(loadWatcher, &QDBusPendingCallWatcher::finished, this, [this, generation](QDBusPendingCallWatcher *pending) {
            QDBusPendingReply<int> loaded = *pending;
            pending->deleteLater();
            if (generation != m_generation) {
                return;
            }
            if (loaded.isError() || loaded.value() < 0) {
                qCWarning(latteWm) << "Cannot load KWin panel tracker:" << loaded.error().message();
                return;
            }
            // Start only our script; Scripting.start() would start unrelated scripts too.
            auto run = QDBusMessage::createMethodCall(service(), QStringLiteral("/Scripting/Script%1").arg(loaded.value()), QStringLiteral("org.kde.kwin.Script"), QStringLiteral("run"));
            auto runWatcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(run, 2000), this);
            connect(runWatcher, &QDBusPendingCallWatcher::finished, this, [this, generation](QDBusPendingCallWatcher *pendingRun) {
                QDBusPendingReply<> running = *pendingRun;
                pendingRun->deleteLater();
                if (generation == m_generation && running.isError()) {
                    qCWarning(latteWm) << "Cannot run KWin panel tracker:" << running.error().message();
                }
            });
            m_timeout.start();
        });
    });
}

QList<QRect>
PlasmaPanelTracker::geometries() const
{ return m_geometries; }

void
PlasmaPanelTracker::updatePanels(const QString &payload)
{
    // Reject stale KWin instances and bound IPC input before JSON parsing.
    if (!calledFromDBus() || message().service() != m_owner || payload.size() > 65536) {
        return;
    }
    const QJsonDocument document = QJsonDocument::fromJson(payload.toUtf8());
    if (!document.isArray() || document.array().size() > 128) {
        return;
    }
    m_panels = document.array();
    m_timeout.stop();
    refreshGeometries();
}

void
PlasmaPanelTracker::refreshGeometries()
{
    QList<QRect> next;
    for (const auto value : std::as_const(m_panels)) {
        const auto panel = value.toObject();
        const QRect rect(
          panel.value(QStringLiteral("x")).toInt(), panel.value(QStringLiteral("y")).toInt(), panel.value(QStringLiteral("width")).toInt(), panel.value(QStringLiteral("height")).toInt());
        for (const auto screens = qGuiApp->screens(); const QScreen *screen : screens) {
            // Output names, not Qt screen order or the desktop's lastScreen id,
            // identify the owner on staggered/negative-coordinate multi-monitor setups.
            if (screen->name() == panel.value(QStringLiteral("output")).toString() && rect.isValid() && screen->geometry().contains(rect.center())) {
                next.append(rect.intersected(screen->geometry()));
                break;
            }
        }
    }
    if (next != m_geometries) {
        m_geometries = next;
        qCDebug(latteWm) << "KWin Plasma panel geometries:" << next;
        Q_EMIT changed();
    }
}
}
