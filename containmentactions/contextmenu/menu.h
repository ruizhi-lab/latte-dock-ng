/*
    SPDX-FileCopyrightText: 2018 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef MENU_H
#define MENU_H

// Qt
#include <QHash>
#include <QObject>

// Plasma
#include <Plasma/ContainmentActions>

class QAction;
class QMenu;

namespace Latte {
namespace ContainmentActions {

struct LayoutInfo
{
    QString layoutName;
    bool isBackgroundFileIcon;
    QString iconName;
};

struct ViewData
{
    bool isCloned{true};
    int clonesCount{0};
};

// Decides which Move To Layout items are drawn bold. The names come from CURRENTLAYOUTSINDEX,
// which Corona::contextMenuData() fills from Synchronizer::currentLayoutsNames(); the Layouts
// menu checks the same list. An empty name is never marked: an empty list splits into one
// empty entry, and an incomplete DBus reply can yield an empty layout name.
bool isCurrentLayoutName(const QStringList &currentNames, const QString &layoutName);

} // namespace ContainmentActions
} // namespace Latte

using Latte::ContainmentActions::LayoutInfo;
using Latte::ContainmentActions::ViewData;
using Latte::ContainmentActions::isCurrentLayoutName;

class Menu : public Plasma::ContainmentActions
{
    Q_OBJECT

public:
    Menu(QObject *parent, const QVariantList &args);
    ~Menu() override;

    QList<QAction *> contextualActions() override;
    void restore(const KConfigGroup &) override;

    QAction *action(const QString &name);
private Q_SLOTS:
    void populateLayouts();
    void populateMoveToLayouts();
    void populateViewTemplates();
    void quitApplication();
    void requestConfiguration();
    void requestWidgetExplorer();
    void updateViewData();
    void updateVisibleActions();

    void addView(QAction *action);
    void moveToLayout(QAction *action);
    void switchToLayout(QAction *action);

private:
    QStringList m_data;
    QStringList m_viewTemplates;

    QStringList m_actionsAlwaysShown;
    QStringList m_activeLayoutNames;

    ViewData m_view;

    QHash<QString, QAction *> m_actions;

    QMenu *m_addViewMenu{nullptr};
    QMenu *m_switchLayoutsMenu{nullptr};
    QMenu *m_moveToLayoutMenu{nullptr};
};

#endif
