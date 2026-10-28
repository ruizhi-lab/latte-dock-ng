/*
    SPDX-FileCopyrightText: 2018 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <latte_debug.h>
#include "menu.h"
#include "layoutmenuitemwidget.h"
using Latte::LayoutMenuItemWidget;

// local
#include "contextmenudata.h"

// Qt
#include <QAction>
#include <QDebug>
#include <QFont>
#include <QMenu>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QTimer>
#include <QLatin1String>

// KDE
#include <KActionCollection>
#include <KLocalizedString>

// Plasma
#include <Plasma/Containment>
#include <Plasma/Corona>

constexpr int kDbusCallDelayMs = 400;

// Call a method on org.kde.lattedock without QDBusInterface (which internally
// connects to the deprecated serviceOwnerChanged signal in Qt 6.8+).
namespace {
QDBusMessage callLatte(const QString &method, const QVariantList &args = {})
{
    QDBusMessage msg = QDBusMessage::createMethodCall(
                           QStringLiteral("org.kde.lattedock"),
                           QStringLiteral("/Latte"),
                           QString(),
                           method);
    msg.setArguments(args);
    return QDBusConnection::sessionBus().call(msg);
}
} // anonymous namespace

const int MEMORYINDEX = 0;
const int ACTIVELAYOUTSINDEX = 1;
const int CURRENTLAYOUTSINDEX = 2;
const int ACTIONSALWAYSSHOWN = 3;
const int LAYOUTMENUINDEX = 4;
const int VIEWLAYOUTINDEX = 5;
const int VIEWTYPEINDEX = 6;

enum LayoutsMemoryUsage {
    SingleLayout = 0,
    MultipleLayouts
};

enum LatteConfigPage {
    LayoutPage = 0,
    PreferencesPage
};

template<typename T>
inline auto registerContainmentAction(T *containment, const QString &name, QAction *action, int)
-> decltype(containment->actions()->addAction(name, action), void())
{
    if (containment && action) {
        containment->actions()->addAction(name, action);
    }
}

template<typename T>
inline void registerContainmentAction(T *, const QString &, QAction *, long)
{
}

Menu::Menu(QObject *parent, const QVariantList &args)
    : Plasma::ContainmentActions(parent, args)
{
}

Menu::~Menu()
{
    //! sub-menus
    if (m_addViewMenu) {
        m_addViewMenu->deleteLater();
    }

    if (m_switchLayoutsMenu) {
        m_switchLayoutsMenu->deleteLater();
    }

    if (m_moveToLayoutMenu) {
        m_moveToLayoutMenu->deleteLater();
    }

    //! clear menu actions that have been created from submenus
    m_actions.remove(Latte::Data::ContextMenu::ADDVIEWACTION);
    m_actions.remove(Latte::Data::ContextMenu::LAYOUTSACTION);

    //! actions
    qDeleteAll(m_actions.values());
    m_actions.clear();
}

void Menu::restore(const KConfigGroup &)
{
    if (!m_actions.isEmpty()) {
        return;
    }

    m_actions[Latte::Data::ContextMenu::SECTIONACTION] = new QAction(this);
    m_actions[Latte::Data::ContextMenu::SECTIONACTION]->setSeparator(true);
    m_actions[Latte::Data::ContextMenu::SECTIONACTION]->setText(QStringLiteral("Latte"));

    m_actions[Latte::Data::ContextMenu::SEPARATOR1ACTION] = new QAction(this);
    m_actions[Latte::Data::ContextMenu::SEPARATOR1ACTION]->setSeparator(true);

    //! Print Message...
    m_actions[Latte::Data::ContextMenu::PRINTACTION] = new QAction(QIcon::fromTheme(QStringLiteral("edit")), QStringLiteral("Print Message..."), this);
    connect(m_actions[Latte::Data::ContextMenu::PRINTACTION], &QAction::triggered, this, [=]() {
        qCDebug(latteContainment) << "Action Triggered !!!";
    });

    //! Add Widgets...
    m_actions[Latte::Data::ContextMenu::ADDWIDGETSACTION] = new QAction(QIcon::fromTheme(QStringLiteral("list-add")), i18n("&Add Widgets..."), this);
    m_actions[Latte::Data::ContextMenu::ADDWIDGETSACTION]->setStatusTip(i18n("Show Widget Explorer"));
    connect(m_actions[Latte::Data::ContextMenu::ADDWIDGETSACTION], &QAction::triggered, this, &Menu::requestWidgetExplorer);
    registerContainmentAction(this->containment(), Latte::Data::ContextMenu::ADDWIDGETSACTION, m_actions[Latte::Data::ContextMenu::ADDWIDGETSACTION], 0);

    /*connect(m_addWidgetsAction, &QAction::triggered, [ = ]() {
        QDBusInterface iface("org.kde.plasmashell", "/PlasmaShell", "", QDBusConnection::sessionBus());

        if (iface.isValid()) {
            iface.call("toggleWidgetExplorer");
        }
    });*/

    //! Edit Dock...
    m_actions[Latte::Data::ContextMenu::EDITVIEWACTION] = new QAction(QIcon::fromTheme(QStringLiteral("document-edit")), QStringLiteral("Edit Dock..."), this);
    connect(m_actions[Latte::Data::ContextMenu::EDITVIEWACTION], &QAction::triggered, this, &Menu::requestConfiguration);
    registerContainmentAction(this->containment(), Latte::Data::ContextMenu::EDITVIEWACTION, m_actions[Latte::Data::ContextMenu::EDITVIEWACTION], 0);


    //! Quit Application
    m_actions[Latte::Data::ContextMenu::QUITLATTEACTION] = new QAction(QIcon::fromTheme(QStringLiteral("application-exit")), i18nc("quit application", "Quit &Latte"));
    connect(m_actions[Latte::Data::ContextMenu::QUITLATTEACTION], &QAction::triggered, this, &Menu::quitApplication);
    registerContainmentAction(this->containment(), Latte::Data::ContextMenu::QUITLATTEACTION, m_actions[Latte::Data::ContextMenu::QUITLATTEACTION], 0);

    //! Layouts submenu
    m_switchLayoutsMenu = new QMenu;
    m_actions[Latte::Data::ContextMenu::LAYOUTSACTION] = m_switchLayoutsMenu->menuAction();
    m_actions[Latte::Data::ContextMenu::LAYOUTSACTION]->setText(i18n("&Layouts"));
    m_actions[Latte::Data::ContextMenu::LAYOUTSACTION]->setIcon(QIcon::fromTheme(QStringLiteral("user-identity")));
    m_actions[Latte::Data::ContextMenu::LAYOUTSACTION]->setStatusTip(i18n("Switch to another layout"));
    registerContainmentAction(this->containment(), Latte::Data::ContextMenu::LAYOUTSACTION, m_actions[Latte::Data::ContextMenu::LAYOUTSACTION], 0);

    connect(m_switchLayoutsMenu, &QMenu::aboutToShow, this, &Menu::populateLayouts);
    connect(m_switchLayoutsMenu, &QMenu::triggered, this, &Menu::switchToLayout);

    //! Add View submenu
    m_addViewMenu = new QMenu;
    m_actions[Latte::Data::ContextMenu::ADDVIEWACTION] = m_addViewMenu->menuAction();
    m_actions[Latte::Data::ContextMenu::ADDVIEWACTION]->setText(i18n("&Add Dock"));
    m_actions[Latte::Data::ContextMenu::ADDVIEWACTION]->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    m_actions[Latte::Data::ContextMenu::ADDVIEWACTION]->setStatusTip(i18n("Add dock based on specific template"));
    registerContainmentAction(this->containment(), Latte::Data::ContextMenu::ADDVIEWACTION, m_actions[Latte::Data::ContextMenu::ADDVIEWACTION], 0);

    connect(m_addViewMenu, &QMenu::aboutToShow, this, &Menu::populateViewTemplates);
    connect(m_addViewMenu, &QMenu::triggered, this, &Menu::addView);

    //! Move submenu
    m_moveToLayoutMenu = new QMenu;
    m_actions[Latte::Data::ContextMenu::MOVEVIEWACTION] = m_moveToLayoutMenu->menuAction();
    m_actions[Latte::Data::ContextMenu::MOVEVIEWACTION]->setText(QStringLiteral("Move To Layout"));
    m_actions[Latte::Data::ContextMenu::MOVEVIEWACTION]->setIcon(QIcon::fromTheme(QStringLiteral("transform-move-horizontal")));
    m_actions[Latte::Data::ContextMenu::MOVEVIEWACTION]->setStatusTip(i18n("Move dock to different layout"));
    registerContainmentAction(this->containment(), Latte::Data::ContextMenu::MOVEVIEWACTION, m_actions[Latte::Data::ContextMenu::MOVEVIEWACTION], 0);

    connect(m_moveToLayoutMenu, &QMenu::aboutToShow, this, &Menu::populateMoveToLayouts);
    connect(m_moveToLayoutMenu, &QMenu::triggered, this, &Menu::moveToLayout);

    //! Configure Latte
    m_actions[Latte::Data::ContextMenu::PREFERENCESACTION] = new QAction(QIcon::fromTheme(QStringLiteral("configure")), i18nc("global settings window", "&Configure Latte..."), this);
    registerContainmentAction(this->containment(), Latte::Data::ContextMenu::PREFERENCESACTION, m_actions[Latte::Data::ContextMenu::PREFERENCESACTION], 0);
    connect(m_actions[Latte::Data::ContextMenu::PREFERENCESACTION], &QAction::triggered, this, [=]() {
        callLatte(QStringLiteral("showSettingsWindow"),
        {QVariant::fromValue((int)PreferencesPage)});
    });

    //! Duplicate Action
    m_actions[Latte::Data::ContextMenu::DUPLICATEVIEWACTION] = new QAction(QIcon::fromTheme(QStringLiteral("edit-copy")), QStringLiteral("Duplicate Dock as Template"), this);
    connect(m_actions[Latte::Data::ContextMenu::DUPLICATEVIEWACTION], &QAction::triggered, this, [=, this]() {
        callLatte(QStringLiteral("duplicateView"),
        {QVariant::fromValue(containment()->id())});
    });

    registerContainmentAction(this->containment(), Latte::Data::ContextMenu::DUPLICATEVIEWACTION, m_actions[Latte::Data::ContextMenu::DUPLICATEVIEWACTION], 0);

    //! Export View Template Action
    m_actions[Latte::Data::ContextMenu::EXPORTVIEWTEMPLATEACTION] = new QAction(QIcon::fromTheme(QStringLiteral("document-export")), QStringLiteral("Export as Template..."), this);
    connect(m_actions[Latte::Data::ContextMenu::EXPORTVIEWTEMPLATEACTION], &QAction::triggered, this, [=, this]() {
        callLatte(QStringLiteral("exportViewTemplate"),
        {QVariant::fromValue(containment()->id())});
    });

    registerContainmentAction(this->containment(), Latte::Data::ContextMenu::EXPORTVIEWTEMPLATEACTION, m_actions[Latte::Data::ContextMenu::EXPORTVIEWTEMPLATEACTION], 0);

    //! Remove Action
    m_actions[Latte::Data::ContextMenu::REMOVEVIEWACTION] = new QAction(QIcon::fromTheme(QStringLiteral("delete")), QStringLiteral("Remove Dock"), this);
    connect(m_actions[Latte::Data::ContextMenu::REMOVEVIEWACTION], &QAction::triggered, this, [=, this]() {
        callLatte(QStringLiteral("removeView"),
        {QVariant::fromValue(containment()->id())});
    });

    registerContainmentAction(this->containment(), Latte::Data::ContextMenu::REMOVEVIEWACTION, m_actions[Latte::Data::ContextMenu::REMOVEVIEWACTION], 0);

    //! Signals
    //! The containment outlives this actions plugin when it is reconfigured.
    connect(this->containment(), &Plasma::Containment::userConfiguringChanged, this, [=, this]() {
        updateVisibleActions();
    });
}

void Menu::requestConfiguration()
{
    if (this->containment()) {
        Q_EMIT this->containment()->configureRequested(containment());
    }
}

void Menu::requestWidgetExplorer()
{
    if (this->containment()) {
        Q_EMIT this->containment()->showAddWidgetsInterface(QPointF());
    }
}

QList<QAction *> Menu::contextualActions()
{
    QList<QAction *> actions;

    actions << m_actions[Latte::Data::ContextMenu::SECTIONACTION];
    actions << m_actions[Latte::Data::ContextMenu::PRINTACTION];

    for (int i = 0; i < Latte::Data::ContextMenu::ACTIONSEDITORDER.count(); ++i) {
        actions << m_actions[Latte::Data::ContextMenu::ACTIONSEDITORDER[i]];
    }

    actions << m_actions[Latte::Data::ContextMenu::EDITVIEWACTION];

    m_data.clear();
    m_viewTemplates.clear();

    {
        QDBusMessage reply = callLatte(QStringLiteral("contextMenuData"),
        {QVariant::fromValue(containment()->id())});

        if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
            m_data = reply.arguments().constFirst().toStringList();
        }

        QDBusMessage tReply = callLatte(QStringLiteral("viewTemplatesData"));

        if (tReply.type() == QDBusMessage::ReplyMessage && !tReply.arguments().isEmpty()) {
            m_viewTemplates = tReply.arguments().constFirst().toStringList();
        }
    }

    m_actionsAlwaysShown = m_data.value(ACTIONSALWAYSSHOWN).split(QStringLiteral(";;"));

    updateViewData();

    QString configureActionText = i18n("&Edit Dock...");

    if (m_view.isCloned) {
        configureActionText = i18n("&Edit Original Dock...");
    }

    m_actions[Latte::Data::ContextMenu::EDITVIEWACTION]->setText(configureActionText);

    m_actions[Latte::Data::ContextMenu::DUPLICATEVIEWACTION]->setText(i18n("&Duplicate Dock"));

    m_actions[Latte::Data::ContextMenu::EXPORTVIEWTEMPLATEACTION]->setText(i18n("E&xport Dock as Template"));

    m_activeLayoutNames = m_data.value(ACTIVELAYOUTSINDEX).split(QStringLiteral(";;"));
    m_actions[Latte::Data::ContextMenu::MOVEVIEWACTION]->setText(i18n("&Move Dock To Layout"));

    m_actions[Latte::Data::ContextMenu::REMOVEVIEWACTION]->setText(i18n("&Remove Dock"));

    updateVisibleActions();

    return actions;
}

QAction *Menu::action(const QString &name)
{
    if (m_actions.contains(name)) {
        return m_actions[name];
    }

    return nullptr;
}

void Menu::updateVisibleActions()
{
    if (!m_actions.contains(Latte::Data::ContextMenu::EDITVIEWACTION)
        || !m_actions.contains(Latte::Data::ContextMenu::REMOVEVIEWACTION)) {
        return;
    }

    bool configuring = containment()->isUserConfiguring();

    // normal actions that the user can specify their visibility
    for (const auto &[actionName, action] : std::as_const(m_actions).asKeyValueRange()) {
        if (Latte::Data::ContextMenu::ACTIONSSPECIAL.contains(actionName)) {
            continue;
        } else if (Latte::Data::ContextMenu::ACTIONSALWAYSHIDDEN.contains(actionName)) {
            action->setVisible(false);
            continue;
        }

        bool isvisible = m_actionsAlwaysShown.contains(actionName) || configuring;
        action->setVisible(isvisible);
    }

    // normal actions with more criteria
    bool isshown = (m_actions[Latte::Data::ContextMenu::MOVEVIEWACTION]->isVisible() && m_activeLayoutNames.count() > 1);
    m_actions[Latte::Data::ContextMenu::MOVEVIEWACTION]->setVisible(isshown);

    // special actions
    m_actions[Latte::Data::ContextMenu::EDITVIEWACTION]->setVisible(!configuring);
    m_actions[Latte::Data::ContextMenu::SECTIONACTION]->setVisible(true);

    if (m_view.isCloned) {
        m_actions[Latte::Data::ContextMenu::DUPLICATEVIEWACTION]->setVisible(false);
        m_actions[Latte::Data::ContextMenu::EXPORTVIEWTEMPLATEACTION]->setVisible(false);
        m_actions[Latte::Data::ContextMenu::MOVEVIEWACTION]->setVisible(false);
        m_actions[Latte::Data::ContextMenu::REMOVEVIEWACTION]->setVisible(false);
    }

    // because sometimes they are disabled unexpectedly, we should reenable them
    for (QAction *action : std::as_const(m_actions)) {
        action->setEnabled(true);
    }
}


void Menu::populateLayouts()
{
    m_switchLayoutsMenu->clear();

    LayoutsMemoryUsage memoryUsage = static_cast<LayoutsMemoryUsage>(m_data.value(MEMORYINDEX).toInt());
    QStringList activeNames = m_data.value(ACTIVELAYOUTSINDEX).split(QStringLiteral(";;"));
    QStringList currentNames = m_data.value(CURRENTLAYOUTSINDEX).split(QStringLiteral(";;"));

    QList<LayoutInfo> layoutsmenulist;

    QStringList layoutsdata = m_data.value(LAYOUTMENUINDEX).split(QStringLiteral(";;"));

    for (int i = 0; i < layoutsdata.count(); ++i) {
        QStringList cdata = layoutsdata[i].split(QStringLiteral("**"));

        LayoutInfo info;
        info.layoutName = cdata.value(0);
        info.isBackgroundFileIcon = cdata.value(1).toInt();
        info.iconName = cdata.value(2);

        layoutsmenulist << info;
    }

    for (int i = 0; i < layoutsmenulist.count(); ++i) {
        bool isActive = activeNames.contains(layoutsmenulist[i].layoutName);

        bool isCurrent = ((memoryUsage == SingleLayout && isActive)
                          || (memoryUsage == MultipleLayouts && currentNames.contains(layoutsmenulist[i].layoutName)));


        QWidgetAction *action = new QWidgetAction(m_switchLayoutsMenu);
        action->setText(layoutsmenulist[i].layoutName);
        action->setCheckable(true);
        action->setChecked(isCurrent);
        action->setData(layoutsmenulist[i].layoutName);

        LayoutMenuItemWidget *menuitem = new LayoutMenuItemWidget(action, m_switchLayoutsMenu);
        menuitem->setIcon(layoutsmenulist[i].isBackgroundFileIcon, layoutsmenulist[i].iconName);
        action->setDefaultWidget(menuitem);
        m_switchLayoutsMenu->addAction(action);
    }

    m_switchLayoutsMenu->addSeparator();

    QWidgetAction *editaction = new QWidgetAction(m_switchLayoutsMenu);
    editaction->setText(i18n("Edit &Layouts..."));
    editaction->setCheckable(false);
    editaction->setData(QStringLiteral(" _show_latte_settings_dialog_"));
    editaction->setVisible(false);

    LayoutMenuItemWidget *editmenuitem = new LayoutMenuItemWidget(editaction, m_switchLayoutsMenu);
    editmenuitem->setIcon(false, QStringLiteral("document-edit"));
    editaction->setDefaultWidget(editmenuitem);
    m_switchLayoutsMenu->addAction(editaction);
}

void Menu::populateMoveToLayouts()
{
    m_moveToLayoutMenu->clear();

    LayoutsMemoryUsage memoryUsage = static_cast<LayoutsMemoryUsage>(m_data.value(MEMORYINDEX).toInt());

    if (memoryUsage == LayoutsMemoryUsage::MultipleLayouts) {
        QString viewLayoutName = m_data.value(VIEWLAYOUTINDEX);

        QList<LayoutInfo> layoutsmenulist;

        QStringList layoutsdata = m_data.value(LAYOUTMENUINDEX).split(QStringLiteral(";;"));

        for (int i = 0; i < layoutsdata.count(); ++i) {
            QStringList cdata = layoutsdata[i].split(QStringLiteral("**"));

            LayoutInfo info;
            info.layoutName = cdata.value(0);
            info.isBackgroundFileIcon = cdata.value(1).toInt();
            info.iconName = cdata.value(2);

            layoutsmenulist << info;
        }

        for (int i = 0; i < layoutsmenulist.count(); ++i) {
            bool isViewCurrentLayout = layoutsmenulist[i].layoutName == viewLayoutName;

            QWidgetAction *action = new QWidgetAction(m_moveToLayoutMenu);
            action->setText(layoutsmenulist[i].layoutName);
            action->setCheckable(true);
            action->setChecked(isViewCurrentLayout);
            action->setData(isViewCurrentLayout ? QString() : layoutsmenulist[i].layoutName);

            LayoutMenuItemWidget *menuitem = new LayoutMenuItemWidget(action, m_moveToLayoutMenu);
            menuitem->setIcon(layoutsmenulist[i].isBackgroundFileIcon, layoutsmenulist[i].iconName);
            action->setDefaultWidget(menuitem);
            m_moveToLayoutMenu->addAction(action);
        }
    }
}

void Menu::updateViewData()
{
    QStringList vdata = m_data.value(VIEWTYPEINDEX).split(QStringLiteral(";;"));
    m_view.isCloned = vdata.value(1).toInt();
    m_view.clonesCount = vdata.value(2).toInt();
}

void Menu::populateViewTemplates()
{
    m_addViewMenu->clear();

    for (int i = 0; i < m_viewTemplates.count(); ++i) {
        if (i % 2 == 1) {
            //! even records are the templates ids and they have already been registered
            continue;
        }

        QAction *templateAction = m_addViewMenu->addAction(m_viewTemplates[i]);
        templateAction->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
        templateAction->setData(m_viewTemplates[i + 1]);
    }

    m_addViewMenu->addSeparator();
    QAction *duplicateAction = m_addViewMenu->addAction(m_actions[Latte::Data::ContextMenu::DUPLICATEVIEWACTION]->text());
    duplicateAction->setToolTip(m_actions[Latte::Data::ContextMenu::DUPLICATEVIEWACTION]->toolTip());
    duplicateAction->setIcon(m_actions[Latte::Data::ContextMenu::DUPLICATEVIEWACTION]->icon());
    connect(duplicateAction, &QAction::triggered, m_actions[Latte::Data::ContextMenu::DUPLICATEVIEWACTION], &QAction::triggered);
}

void Menu::addView(QAction *action)
{
    const QString templateId = action->data().toString();

    QTimer::singleShot(kDbusCallDelayMs, [this, templateId]() {
        callLatte(QStringLiteral("addView"),
        {QVariant::fromValue(containment()->id()), QVariant::fromValue(templateId)});
    });
}

void Menu::moveToLayout(QAction *action)
{
    const QString layoutName = action->data().toString();

    QTimer::singleShot(kDbusCallDelayMs, [this, layoutName]() {
        callLatte(QStringLiteral("moveViewToLayout"),
        {QVariant::fromValue(containment()->id()), QVariant::fromValue(layoutName)});
    });
}

void Menu::switchToLayout(QAction *action)
{
    const QString layout = action->data().toString();

    if (layout == QLatin1String(" _show_latte_settings_dialog_")) {
        QTimer::singleShot(kDbusCallDelayMs, []() {
            callLatte(QStringLiteral("showSettingsWindow"),
            {QVariant::fromValue((int)LayoutPage)});
        });
    } else {
        QTimer::singleShot(kDbusCallDelayMs, [layout]() {
            callLatte(QStringLiteral("switchToLayout"),
            {QVariant::fromValue(layout)});
        });
    }
}

void Menu::quitApplication()
{
    callLatte(QStringLiteral("quitApplication"));
}

K_PLUGIN_CLASS_WITH_JSON(Menu, "plasma-containmentactions-lattecontextmenu.json")

#include "menu.moc"
