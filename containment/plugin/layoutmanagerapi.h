/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef CONTAINMENTLAYOUTMANAGERAPI_H
#define CONTAINMENTLAYOUTMANAGERAPI_H

#include <QList>
#include <QObject>
#include <QQuickItem>
#include <QString>
#include <QVariant>

namespace Latte::Containment {

//! Link-free C++ contract between the application and its QML containment plugin.
//! The object is still owned and guarded as a QObject; callers must not retain this
//! interface pointer beyond the guarded object's lifetime.
class LayoutManagerApi
{
  public:
    virtual ~LayoutManagerApi() = default;

    virtual QQuickItem *rootItem() const = 0;
    virtual QQuickItem *dndSpacer() const = 0;
    virtual QList<int> order() const = 0;
    virtual QList<int> lockedZoomApplets() const = 0;
    virtual QList<int> userBlocksColorizingApplets() const = 0;

    virtual void setOption(const int &appletId, const QString &property, const QVariant &value) = 0;
    virtual void addAppletItem(QObject *applet, int index) = 0;
    virtual void removeAppletItem(QObject *applet) = 0;
    virtual void hideAppletItem(QObject *applet) = 0;
    virtual void showAppletItem(QObject *applet) = 0;
    virtual void insertAtCoordinates(QQuickItem *item, const int &x, const int &y) = 0;
    virtual int dndSpacerIndex() = 0;
    virtual void requestAppletsOrder(const QList<int> &order) = 0;
    virtual void requestAppletsInLockedZoom(const QList<int> &applets) = 0;
    virtual void requestAppletsDisabledColoring(const QList<int> &applets) = 0;
    virtual void setAppletInScheduledDestruction(const int &id, const bool &enabled) = 0;
};

}

#define LatteContainmentLayoutManagerApi_iid "org.kde.latte.containment.LayoutManagerApi/1.0"
Q_DECLARE_INTERFACE(Latte::Containment::LayoutManagerApi, LatteContainmentLayoutManagerApi_iid)

#endif // CONTAINMENTLAYOUTMANAGERAPI_H
