/*
    SPDX-FileCopyrightText: 2019 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "persistentmenu.h"

namespace Latte {
namespace Settings {
namespace Layout {
namespace Delegate {

PersistentMenu::PersistentMenu(QWidget *parent)
  : QMenu(parent)
{
}

void PersistentMenu::setVisible(bool visible)
{
    // The close that QMenu starts for the release is stopped in closeEvent(). A direct hide()
    // during the release is refused here too.
    if (!visible && m_keepOpenForRelease) {
        return;
    }

    QMenu::setVisible(visible);
}

void
PersistentMenu::closeEvent(QCloseEvent *e)
{
    // QMenuPrivate::hideMenu() closes the popup with QWidget::close(). Since Qt 6.2 a native popup
    // closes through QWindow::close() (checked in the Qt 6.2.4 and later sources; Qt 5.15 hides the
    // widget directly). The close event reaches the widget first. If it is accepted, Qt calls
    // setVisible(false), and once the window's close is accepted it hides the widget itself, so
    // a refused setVisible(false) does not keep the popup open. Ignoring the close event stops
    // the close before that point.
    if (m_keepOpenForRelease) {
        e->ignore();
        return;
    }

    QMenu::closeEvent(e);
}

int PersistentMenu::masterIndex() const
{
    return m_masterIndex;
}

void PersistentMenu::setMasterIndex(const int &index)
{
    if (m_masterIndex == index) {
        return;
    }

    m_masterIndex = index;
    Q_EMIT masterIndexChanged(index);
}

void PersistentMenu::mouseReleaseEvent(QMouseEvent *e)
{
    // QMenu::mouseReleaseEvent() closes the popup and then triggers the item, as on a normal menu.
    // This menu stays open after a click on an item, so the close of this release is vetoed
    // (closeEvent(), and setVisible() for a direct hide). The veto covers this call only: OK,
    // Cancel and a click outside the menu are not affected, and the flag cannot outlive the
    // release even when QMenu does not close the popup at all.
    // Failure mode without the veto: the popup closes at once, the editor commits without OK,
    // and the tick is lost (Qt 6.11.3 reproduction in docs/modernization-results/R7j.md).
    const QAction *action = actionAt(e->pos());
    m_keepOpenForRelease = (action != nullptr);

    QMenu::mouseReleaseEvent(e);

    m_keepOpenForRelease = false;
}

}
}
}
}
