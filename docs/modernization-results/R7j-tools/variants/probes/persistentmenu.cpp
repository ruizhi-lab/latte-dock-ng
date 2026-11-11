/*
    R7j scratch copy of app/settings/generic/persistentmenu.cpp (not part of the repository).
    The production logic is unchanged; qInfo() probes record every setVisible() call, the
    m_blockHide state around mouseReleaseEvent(), and every real show and hide.
*/

#include "persistentmenu.h"

#include <QApplication>
#include <QDebug>

namespace Latte {
namespace Settings {
namespace Layout {
namespace Delegate {

PersistentMenu::PersistentMenu(QWidget *parent)
    : QMenu(parent),
      m_blockHide(false)
{
}

void PersistentMenu::setVisible(bool visible)
{
    qInfo().noquote() << "R7J setVisible(" << visible << ") blockHide=" << m_blockHide
                      << "isVisible=" << isVisible() << "activePopup=" << (QApplication::activePopupWidget() == this);

    if (m_blockHide) {
        m_blockHide = false;
        qInfo().noquote() << "R7J   -> blocked, blockHide cleared";
        return;
    }

    QMenu::setVisible(visible);
}

void PersistentMenu::showEvent(QShowEvent *e)
{
    qInfo().noquote() << "R7J showEvent";
    QMenu::showEvent(e);
}

void PersistentMenu::hideEvent(QHideEvent *e)
{
    qInfo().noquote() << "R7J hideEvent activePopup=" << (QApplication::activePopupWidget() == this);
    QMenu::hideEvent(e);
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
    const QAction *action = actionAt(e->pos());
    qInfo().noquote() << "R7J mouseRelease pos=" << e->pos() << "actionAt="
                      << (action ? action->text() : QStringLiteral("(none)"))
                      << "activeAction=" << (activeAction() ? activeAction()->text() : QStringLiteral("(none)"));

    if (action) {
        m_blockHide = true;
    }

    QMenu::mouseReleaseEvent(e);
    qInfo().noquote() << "R7J after QMenu::mouseReleaseEvent blockHide=" << m_blockHide << "isVisible=" << isVisible();
}

}
}
}
}
