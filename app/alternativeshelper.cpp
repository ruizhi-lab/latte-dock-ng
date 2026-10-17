/*
    SPDX-FileCopyrightText: 2014 Marco Hart <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "alternativeshelper.h"

// Qt
#include <QJsonArray>
#include <QQmlEngine>
#include <QQmlContext>

// KDE
#include <KPackage/Package>

// Plasma
#include <Plasma/Containment>
#include <Plasma/PluginLoader>
#include <PlasmaQuick/AppletQuickItem>

namespace Latte {

namespace {

//! Plasma 6 resolves applet items through AppletQuickItem::itemForApplet()
//! and no longer publishes the Plasma 5 "_plasma_graphicObject" property
//! (verified on libplasma 6.8); keep the property only as a fallback, like
//! launcherApiItemForApplet() in lattecorona.cpp.
PlasmaQuick::AppletQuickItem *itemFor(Plasma::Applet *applet)
{
    PlasmaQuick::AppletQuickItem *item = PlasmaQuick::AppletQuickItem::itemForApplet(applet);

    if (!item) {
        item = applet->property("_plasma_graphicObject").value<PlasmaQuick::AppletQuickItem *>();
    }

    return item;
}

}


AlternativesHelper::AlternativesHelper(Plasma::Applet *applet, QObject *parent)
    : QObject(parent),
      m_applet(applet)
{
}

AlternativesHelper::~AlternativesHelper()
{
}

QStringList AlternativesHelper::appletProvides() const
{
    const auto val = m_applet->pluginMetaData().rawData().value(QStringLiteral("X-Plasma-Provides"));

    if (val.isArray()) {
        QStringList result;

        for (const auto &v : val.toArray()) result << v.toString();

        return result;
    }

    return val.toString().split(QLatin1Char(','), Qt::SkipEmptyParts);
}

QString AlternativesHelper::currentPlugin() const
{
    return m_applet->pluginMetaData().pluginId();
}

QQuickItem *AlternativesHelper::applet() const
{
    //! A null item made the dialog open in a screen corner.
    return itemFor(m_applet);
}

void AlternativesHelper::loadAlternative(const QString &plugin)
{
    if (plugin == currentPlugin() || m_applet->isContainment()) {
        return;
    }

    Plasma::Containment *cont = m_applet->containment();

    if (!cont) {
        return;
    }

    //! Plasma 6's ContainmentItem::createApplet() takes a QRectF geometry hint;
    //! with the Plasma 5 QPoint call the old applet was destroyed without a
    //! replacement.
    PlasmaQuick::AppletQuickItem *appletItem = itemFor(m_applet);
    PlasmaQuick::AppletQuickItem *contItem = itemFor(cont);

    if (!appletItem || !contItem) {
        return;
    }

    // ensure the global shortcut is moved to the new applet
    const QKeySequence &shortcut = m_applet->globalShortcut();
    m_applet->setGlobalShortcut(QKeySequence()); // need to unmap the old one first

    const QPoint newPos = appletItem->mapToItem(contItem, QPointF(0, 0)).toPoint();

    m_applet->destroy();

    connect(m_applet, &QObject::destroyed, [=]() {
        Plasma::Applet *newApplet = nullptr;
        QMetaObject::invokeMethod(contItem, "createApplet", Q_RETURN_ARG(Plasma::Applet *, newApplet), Q_ARG(QString, plugin), Q_ARG(QVariantList, QVariantList()), Q_ARG(QRectF, QRectF(newPos, QSizeF())));

        if (newApplet) {
            newApplet->setGlobalShortcut(shortcut);
        }
    });
}

} // namespace Latte

#include "moc_alternativeshelper.cpp"
