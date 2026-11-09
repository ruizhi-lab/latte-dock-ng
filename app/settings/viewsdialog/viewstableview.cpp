/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "viewstableview.h"

//! Qt
#include <QDebug>
#include <QModelIndex>


namespace Latte {
namespace Settings {
namespace View {

ViewsTableView::ViewsTableView(QWidget *parent)
    : QTableView(parent)
{
}

void ViewsTableView::mousePressEvent(QMouseEvent *event)
{
    QModelIndex eventIndex = indexAt(event->pos());

    if (!eventIndex.isValid()) {
        clearSelection();
    }

    QTableView::mousePressEvent(event);
}

void ViewsTableView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    // Call QTableView's override, not QAbstractItemView's: QTableView sends the
    // accessibility events for the selection, and skipping it hides selection changes
    // from assistive technology.
    QTableView::selectionChanged(selected, deselected);
    Q_EMIT selectionsChanged();
}

}
}
}

