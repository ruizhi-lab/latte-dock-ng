/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LATTE_LAYOUTMANAGERAPIDISPATCHER_H
#define LATTE_LAYOUTMANAGERAPIDISPATCHER_H

#include "../../containment/plugin/layoutmanagerapi.h"

#include <QMetaObject>
#include <QPointer>

namespace Latte::ViewPart {

//! Dispatches to the typed interface when present and preserves the older
//! QObject property/invokable contract for a QML plugin loaded from another
//! installation root. The guarded QObject is the lifetime authority; the
//! interface is resolved for each call so destruction or replacement cannot
//! leave a cached plugin pointer behind.
//!
//! This dispatcher is the only writer of the one-shot insertion protocol: the
//! typed manager keeps the token in private state and the legacy fallback uses
//! the older dynamic property names. No other application code sets those names.
class LayoutManagerApiDispatcher final
{
  public:
    void setObject(QObject *object) { m_object = object; }

    QObject *object() const { return m_object; }

    QQuickItem *rootItem() const
    {
        if (const auto *api = typedApi()) {
            return api->rootItem();
        }
        return m_object ? m_object->property("rootItem").value<QQuickItem *>() : nullptr;
    }

    QQuickItem *dndSpacer() const
    {
        if (const auto *api = typedApi()) {
            return api->dndSpacer();
        }
        return m_object ? m_object->property("dndSpacerItem").value<QQuickItem *>() : nullptr;
    }

    QList<int> order() const
    {
        if (const auto *api = typedApi()) {
            return api->order();
        }
        return m_object ? m_object->property("order").value<QList<int>>() : QList<int>{};
    }

    QList<int> lockedZoomApplets() const
    {
        if (const auto *api = typedApi()) {
            return api->lockedZoomApplets();
        }
        return m_object ? m_object->property("lockedZoomApplets").value<QList<int>>() : QList<int>{};
    }

    QList<int> userBlocksColorizingApplets() const
    {
        if (const auto *api = typedApi()) {
            return api->userBlocksColorizingApplets();
        }
        return m_object ? m_object->property("userBlocksColorizingApplets").value<QList<int>>() : QList<int>{};
    }

    void setOption(const int appletId, const QString &property, const QVariant &value) const
    {
        if (auto *api = typedApi()) {
            api->setOption(appletId, property, value);
        } else if (m_object) {
            QMetaObject::invokeMethod(m_object, "setOption", Qt::DirectConnection, Q_ARG(int, appletId), Q_ARG(QString, property), Q_ARG(QVariant, value));
        }
    }

    void addAppletItem(QObject *applet, const int index) const
    {
        if (auto *api = typedApi()) {
            api->addAppletItem(applet, index);
        } else if (m_object) {
            QMetaObject::invokeMethod(m_object, "addAppletItem", Qt::DirectConnection, Q_ARG(QObject*, applet), Q_ARG(int, index));
        }
    }

    void removeAppletItem(QObject *applet) const
    {
        if (auto *api = typedApi()) {
            api->removeAppletItem(applet);
        } else if (m_object) {
            QMetaObject::invokeMethod(m_object, "removeAppletItem", Qt::DirectConnection, Q_ARG(QObject*, applet));
        }
    }

    void hideAppletItem(QObject *applet) const
    {
        if (auto *api = typedApi()) {
            api->hideAppletItem(applet);
        } else if (m_object) {
            QMetaObject::invokeMethod(m_object, "hideAppletItem", Qt::DirectConnection, Q_ARG(QObject*, applet));
        }
    }

    void showAppletItem(QObject *applet) const
    {
        if (auto *api = typedApi()) {
            api->showAppletItem(applet);
        } else if (m_object) {
            QMetaObject::invokeMethod(m_object, "showAppletItem", Qt::DirectConnection, Q_ARG(QObject*, applet));
        }
    }

    void insertAtCoordinates(QQuickItem *item, const int x, const int y) const
    {
        if (auto *api = typedApi()) {
            api->insertAtCoordinates(item, x, y);
        } else if (m_object) {
            QMetaObject::invokeMethod(m_object, "insertAtCoordinates", Qt::DirectConnection, Q_ARG(QQuickItem*, item), Q_ARG(int, x), Q_ARG(int, y));
        }
    }

    int dndSpacerIndex() const
    {
        if (auto *api = typedApi()) {
            return api->dndSpacerIndex();
        }
        int index{ -1 };
        if (m_object) {
            QMetaObject::invokeMethod(m_object, "dndSpacerIndex", Qt::DirectConnection, Q_RETURN_ARG(int, index));
        }
        return index;
    }

    void requestAppletsOrder(const QList<int> &order) const
    {
        if (auto *api = typedApi()) {
            api->requestAppletsOrder(order);
        } else if (m_object) {
            QMetaObject::invokeMethod(m_object, "requestAppletsOrder", Qt::DirectConnection, Q_ARG(QList<int>, order));
        }
    }

    void requestAppletsInLockedZoom(const QList<int> &applets) const
    {
        if (auto *api = typedApi()) {
            api->requestAppletsInLockedZoom(applets);
        } else if (m_object) {
            QMetaObject::invokeMethod(m_object, "requestAppletsInLockedZoom", Qt::DirectConnection, Q_ARG(QList<int>, applets));
        }
    }

    void requestAppletsDisabledColoring(const QList<int> &applets) const
    {
        if (auto *api = typedApi()) {
            api->requestAppletsDisabledColoring(applets);
        } else if (m_object) {
            QMetaObject::invokeMethod(m_object, "requestAppletsDisabledColoring", Qt::DirectConnection, Q_ARG(QList<int>, applets));
        }
    }

    void setAppletInScheduledDestruction(const int id, const bool enabled) const
    {
        if (auto *api = typedApi()) {
            api->setAppletInScheduledDestruction(id, enabled);
        } else if (m_object) {
            QMetaObject::invokeMethod(m_object, "setAppletInScheduledDestruction", Qt::DirectConnection, Q_ARG(int, id), Q_ARG(bool, enabled));
        }
    }

    //! One-shot insertion protocol. A plugin without the typed interface still
    //! publishes the older dynamic-property channel, so the fallback keeps the
    //! previous behaviour instead of silently dropping the request. A negative
    //! index drops an unconsumed request.
    void expectAppletInsertionAt(const int index) const
    {
        if (auto *api = typedApi()) {
            api->expectAppletInsertionAt(index);
        } else if (m_object) {
            m_object->setProperty("_latte_pendingInsertionIndex", index >= 0 ? QVariant(index) : QVariant());
        }
    }

    int takeAppletInsertionIndex() const
    {
        if (auto *api = typedApi()) {
            return api->takeAppletInsertionIndex();
        }
        if (!m_object) {
            return -1;
        }
        const QVariant pendingIndex = m_object->property("_latte_pendingInsertionIndex");
        m_object->setProperty("_latte_pendingInsertionIndex", QVariant());
        return pendingIndex.isValid() ? pendingIndex.toInt() : -1;
    }

    void expectSkipSpacerReorderOnce() const
    {
        if (auto *api = typedApi()) {
            api->expectSkipSpacerReorderOnce();
        } else if (m_object) {
            m_object->setProperty("_latte_skipSpacerReorder", true);
        }
    }

  private:
    Containment::LayoutManagerApi *typedApi() const { return qobject_cast<Containment::LayoutManagerApi *>(m_object.data()); }

    QPointer<QObject> m_object;
};

}

#endif // LATTE_LAYOUTMANAGERAPIDISPATCHER_H
