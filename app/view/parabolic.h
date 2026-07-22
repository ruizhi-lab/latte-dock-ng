/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef VIEWPARABOLIC_H
#define VIEWPARABOLIC_H

// Qt
#include <QElapsedTimer>
#include <QEvent>
#include <QObject>
#include <QQuickItem>
#include <QPointer>
#include <QPointF>
#include <QTimer>

namespace Latte {
class View;
}

namespace Latte {
namespace ViewPart {

class Parabolic: public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQuickItem *currentItem READ currentParabolicItem WRITE setCurrentParabolicItem NOTIFY currentParabolicItemChanged)

public:
    Parabolic(Latte::View *parent);
    ~Parabolic() override;

    QQuickItem *currentParabolicItem() const;
    void setCurrentParabolicItem(QQuickItem *item);

Q_SIGNALS:
    void currentParabolicItemChanged();

private Q_SLOTS:
    void onCurrentParabolicItemChanged();
    void onEvent(QEvent *e);

private:
    //! Minimum interval between parabolic item switches to prevent
    //! rapid oscillation when the cursor is positioned between two icons.
    static constexpr int MIN_SWITCH_INTERVAL_MS{150};

    QPointer<Latte::View> m_view;
    QPointer<QQuickItem> m_currentParabolicItem;

    //! Cache drag tree-walk result to avoid O(tree_depth) metaObject
    //! lookups per DragMove frame.
    QPointF m_cachedDragPos;
    QPointer<QQuickItem> m_cachedDragParabolicItem;

    QPointF m_lastOrphanParabolicMove;

    QTimer m_parabolicItemNullifier;
    QElapsedTimer m_lastSwitchTimer;
    bool m_nullifierJustFired{false};
};

}
}

#endif
