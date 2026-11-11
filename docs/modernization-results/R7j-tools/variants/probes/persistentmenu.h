/*
    R7j scratch copy of app/settings/generic/persistentmenu.h (not part of the repository).
    The only addition is the hideEvent()/showEvent() overrides, which override virtuals that
    QWidget already has, so the class layout and the vtable slots stay as the repository's.
*/

#ifndef PERSISTENTMENU_H
#define PERSISTENTMENU_H

//Qt
#include <QMenu>
#include <QMouseEvent>

namespace Latte {
namespace Settings {
namespace Layout {
namespace Delegate {

class PersistentMenu : public QMenu
{
    Q_OBJECT
public:
    PersistentMenu(QWidget *parent = nullptr);

    int masterIndex() const;
    void setMasterIndex(const int &index);

protected:
    void setVisible(bool visible) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void hideEvent(QHideEvent *e) override;
    void showEvent(QShowEvent *e) override;

Q_SIGNALS:
    void masterIndexChanged(const int &masterRow);

private:
    bool m_blockHide{false};

    int m_masterIndex{-1};

};

}
}
}
}

#endif
