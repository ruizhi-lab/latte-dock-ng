/*
    SPDX-FileCopyrightText: 2019 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
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
    void closeEvent(QCloseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

Q_SIGNALS:
    void masterIndexChanged(const int &masterRow);

private:
  //! Set only while mouseReleaseEvent() passes a release on one of our items to QMenu. It must
  //! not outlive that call: the earlier one-shot flag stayed set when QMenu did not close the
  //! popup, and then swallowed the hide of OK. See mouseReleaseEvent() for the Qt behavior.
  bool m_keepOpenForRelease{ false };

  int m_masterIndex{ -1 };
};

}
}
}
}

#endif
