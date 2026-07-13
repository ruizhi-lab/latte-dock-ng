/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "parabolic.h"

// local
#include "view.h"

// Qt
#include <QDragMoveEvent>
#include <QHoverEvent>
#include <QMetaObject>

namespace Latte {
namespace ViewPart {

Parabolic::Parabolic(Latte::View *parent)
    : QObject(parent),
      m_view(parent)
{
    //! Debounce interval for nullifying the current parabolic item.
    //! When the cursor is positioned exactly between two icons, a too-short
    //! interval (e.g. 1ms) causes rapid oscillation between the two items
    //! as the nullifier fires before the next hover-move event can settle on
    //! the correct target. A longer interval acts as hysteresis, preventing
    //! the magnification effect from jittering between adjacent icons.
    m_parabolicItemNullifier.setInterval(80);
    m_parabolicItemNullifier.setSingleShot(true);
    connect(&m_parabolicItemNullifier, &QTimer::timeout, this, [this]() {
        setCurrentParabolicItem(nullptr);
    });

    connect(this, &Parabolic::currentParabolicItemChanged, this, &Parabolic::onCurrentParabolicItemChanged);

    connect(m_view, &View::eventTriggered, this, &Parabolic::onEvent);
}

Parabolic::~Parabolic()
{
}

QQuickItem *Parabolic::currentParabolicItem() const
{
    return m_currentParabolicItem;
}

void Parabolic::setCurrentParabolicItem(QQuickItem *item)
{
    if (m_currentParabolicItem == item) {
        return;
    }

    //! Prevent rapid oscillation between items when the cursor is positioned
    //! exactly between two icons. A minimum lock interval (150ms) is enforced
    //! when switching from one zoomed item to another — clearing to null on
    //! exit is always allowed to avoid a stale zoomed state.
    if (m_currentParabolicItem && item) {
        if (m_lastSwitchTimer.isValid() && m_lastSwitchTimer.elapsed() < MIN_SWITCH_INTERVAL_MS) {
            return;
        }
        m_lastSwitchTimer.start();
    }

    if (m_currentParabolicItem) {
        QMetaObject::invokeMethod(m_currentParabolicItem, "parabolicExited", Qt::QueuedConnection);
    }

    m_currentParabolicItem = item;
    Q_EMIT currentParabolicItemChanged();
}

void Parabolic::onEvent(QEvent *e)
{
    if (!e) {
        return;
    }

    auto handlePointerMove = [&](const QPointF &windowPos) {
        if (m_currentParabolicItem) {
            QPointF internal = m_currentParabolicItem->mapFromItem(m_view->contentItem(), windowPos);

            if (m_currentParabolicItem->contains(internal)) {
                m_parabolicItemNullifier.stop();
                //! sending move event to parabolic item.
                //! Use DirectConnection since the QML item is in the same
                //! thread — avoids event queue overhead per mouse move frame.
                QMetaObject::invokeMethod(m_currentParabolicItem,
                                          "parabolicMove",
                                          Qt::DirectConnection,
                                          Q_ARG(qreal, internal.x()),
                                          Q_ARG(qreal, internal.y()));
            } else {
                m_lastOrphanParabolicMove = windowPos;
                //! clearing parabolic item
                m_parabolicItemNullifier.start();
            }
        } else {
            m_lastOrphanParabolicMove = windowPos;
        }
    };

    switch (e->type()) {

    case QEvent::Leave:
    case QEvent::DragLeave:
        setCurrentParabolicItem(nullptr);
        m_cachedDragParabolicItem = nullptr;
        break;
    case QEvent::MouseMove:
        if (auto me = dynamic_cast<QMouseEvent *>(e)) {
            handlePointerMove(me->position());
        }
        break;
    case QEvent::HoverMove:
        if (auto he = dynamic_cast<QHoverEvent *>(e)) {
            handlePointerMove(he->position());
        }
        break;
    case QEvent::DragMove:
        if (auto de = dynamic_cast<QDragMoveEvent *>(e)) {
            // During drag, QML MouseArea hover events are suppressed, so no
            // ParabolicArea item is set as current.  Walk the visual item tree
            // from the cursor position to find an item that accepts parabolic
            // enter/move signals (identified by the "parabolicEntered" method).
            // Cache the result per drag position to avoid redundant tree walks.
            if (!m_currentParabolicItem && m_view) {
                QPointF pos = de->position();
                if (m_cachedDragPos != pos || !m_cachedDragParabolicItem) {
                    m_cachedDragPos = pos;
                    QQuickItem *child = m_view->contentItem()->childAt(pos.x(), pos.y());
                    while (child) {
                        int enterIdx = child->metaObject()->indexOfMethod("parabolicEntered(real,real)");
                        if (enterIdx >= 0) {
                            m_cachedDragParabolicItem = child;
                            break;
                        }
                        child = child->parentItem();
                    }
                }
                if (m_cachedDragParabolicItem) {
                    setCurrentParabolicItem(m_cachedDragParabolicItem);
                }
            }
            handlePointerMove(de->position());
        }
        break;
    default:
        break;
    }

}

void Parabolic::onCurrentParabolicItemChanged()
{
    m_parabolicItemNullifier.stop();

    if (m_currentParabolicItem) {
        QPointF internal = m_currentParabolicItem->mapFromItem(m_view->contentItem(), m_lastOrphanParabolicMove);

        if (m_currentParabolicItem->contains(internal)) {
            //! sending enter event to parabolic item.
            //! Use DirectConnection since the QML item is in the same
            //! thread — avoids event queue overhead.
            QMetaObject::invokeMethod(m_currentParabolicItem,
                                      "parabolicEntered",
                                      Qt::DirectConnection,
                                      Q_ARG(qreal, internal.x()),
                                      Q_ARG(qreal, internal.y()));
        }
    }
}

}
}
