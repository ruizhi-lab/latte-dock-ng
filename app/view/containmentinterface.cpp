/*
    SPDX-FileCopyrightText: 2019 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "containmentinterface.h"

// local
#include "pluginids.h"
#include "view.h"
#include "../lattecorona.h"
#include "../layout/genericlayout.h"
#include "../layouts/importer.h"
#include "../layouts/storage.h"
#include "../settings/universalsettings.h"

// Qt
#include <QJsonArray>
#include <QFileInfo>
#include <QDebug>
#include <QLatin1String>
#include <QSet>
#include <QStringList>

// Plasma
#include <Plasma/Applet>
#include <Plasma/Containment>
#include <PlasmaQuick/AppletQuickItem>

// KDE
#include <KConfigGroup>
#include <KDesktopFile>
#include <KLocalizedString>
#include <KPluginMetaData>
#include <KDeclarative/ConfigPropertyMap>

namespace {
constexpr const char kInternalViewSplitterPluginId[] = "org.kde.latte.splitter";
constexpr const char kLatteSeparatorPluginId[] = "org.kde.latte.separator";
constexpr const char kLegacySeparatorPluginId[] = "audoban.applet.separator";

inline bool isSeparatorPluginId(const QString &pluginId)
{
    return pluginId == QLatin1String(kLatteSeparatorPluginId)
            || pluginId == QLatin1String(kLegacySeparatorPluginId);
}

inline QString pluginIdFromMetaData(const KPluginMetaData &meta)
{
    QString pluginId = meta.pluginId();

    if (!pluginId.isEmpty()) {
        return pluginId;
    }

    const QJsonObject raw = meta.rawData();

    if (raw.contains(QStringLiteral("KPlugin")) && raw.value(QStringLiteral("KPlugin")).isObject()) {
        const QJsonObject kplugin = raw.value(QStringLiteral("KPlugin")).toObject();
        pluginId = kplugin.value(QStringLiteral("Id")).toString();

        if (!pluginId.isEmpty()) {
            return pluginId;
        }
    }

    pluginId = raw.value(QStringLiteral("Id")).toString();
    if (!pluginId.isEmpty()) {
        return pluginId;
    }

    // Some applets still expose legacy metadata keys.
    pluginId = raw.value(QStringLiteral("X-KDE-PluginInfo-Name")).toString();
    if (!pluginId.isEmpty()) {
        return pluginId;
    }

    const QString fileName = meta.fileName();
    if (!fileName.isEmpty()) {
        const QFileInfo fileInfo(fileName);
        const QString leaf = fileInfo.fileName();

        // If metadata lives under <plugin-id>/metadata.json (or metadata.desktop),
        // use the plugin directory name instead of the metadata file name.
        if (leaf == QLatin1String("metadata.json")
                || leaf == QLatin1String("metadata.desktop")) {
            const QString dirName = fileInfo.dir().dirName();
            if (!dirName.isEmpty()) {
                return dirName;
            }
        }

        return leaf;
    }

    return QString();
}

inline QStringList appletProvidesFromMetaData(const KPluginMetaData &meta)
{
    const auto value = meta.rawData().value(QStringLiteral("X-Plasma-Provides"));

    if (value.isArray()) {
        QStringList result;
        const auto array = value.toArray();
        for (const auto &entry : array) {
            result << entry.toString();
        }
        return result;
    }

    return value.toString().split(QLatin1Char(','), Qt::SkipEmptyParts);
}

inline QList<int> bestOrderForApplet(const QList<int> &appletOrder,
                                     const QList<int> &layoutOrder,
                                     const int appletId)
{
    if (!layoutOrder.isEmpty() && layoutOrder.contains(appletId)) {
        return layoutOrder;
    }

    if (!appletOrder.isEmpty() && appletOrder.contains(appletId)) {
        return appletOrder;
    }

    if (!layoutOrder.isEmpty()) {
        return layoutOrder;
    }

    return appletOrder;
}

inline QList<int> sanitizeSeparatorOrder(const QList<int> &order,
                                         const QHash<int, Latte::ViewPart::AppletInterfaceData> &appletData,
                                         const QHash<int, QString> &containmentPlugins)
{
    QList<int> sanitized;
    sanitized.reserve(order.size());

    auto hasAppletId = [&appletData, &containmentPlugins](const int appletId) -> bool {
        return appletData.contains(appletId) || containmentPlugins.contains(appletId);
    };

    auto pluginIdFor = [&appletData, &containmentPlugins](const int appletId) -> QString {
        auto it = appletData.constFind(appletId);
        if (it != appletData.constEnd()) {
            return it->plugin;
        }

        return containmentPlugins.value(appletId);
    };

    enum class ItemKind {
        Separator,
        Splitter,
        Content,
        Unknown
    };

    auto itemKind = [&pluginIdFor](const int appletId) -> ItemKind {
        const QString pluginId = pluginIdFor(appletId);

        if (pluginId.isEmpty()) {
            return ItemKind::Unknown;
        }

        if (pluginId == QLatin1String(kInternalViewSplitterPluginId)) {
            return ItemKind::Splitter;
        }

        if (isSeparatorPluginId(pluginId)) {
            return ItemKind::Separator;
        }

        return ItemKind::Content;
    };

    bool seenRenderableApplet{false};
    bool previousWasSeparator{false};

    for (int i = 0; i < order.count(); ++i) {
        const int appletId = order.at(i);
        if (!hasAppletId(appletId)) {
            // Drop stale ids only; keep existing applets even if plugin lookup is
            // temporarily unresolved to avoid boundary separator drift.
            continue;
        }

        const ItemKind kind = itemKind(appletId);

        if (kind == ItemKind::Separator) {
            if (!seenRenderableApplet || previousWasSeparator) {
                continue;
            }

            sanitized << appletId;
            previousWasSeparator = true;
            continue;
        }

        sanitized << appletId;
        seenRenderableApplet = true;
        previousWasSeparator = false;
    }

    // Never keep trailing separators at containment edges.
    while (!sanitized.isEmpty()) {
        const ItemKind tailKind = itemKind(sanitized.constLast());
        if (tailKind != ItemKind::Separator) {
            break;
        }

        sanitized.removeLast();
    }

    return sanitized;
}
}

namespace Latte {
namespace ViewPart {

ContainmentInterface::ContainmentInterface(Latte::View *parent)
    : QObject(parent),
      m_view(parent)
{
    m_corona = qobject_cast<Latte::Corona *>(m_view->corona());

    m_latteTasksModel = new TasksModel(this);
    m_plasmaTasksModel = new TasksModel(this);

    m_appletsExpandedConnectionsTimer.setInterval(2000);
    m_appletsExpandedConnectionsTimer.setSingleShot(true);

    m_appletDelayedConfigurationTimer.setInterval(1000);
    m_appletDelayedConfigurationTimer.setSingleShot(true);
    connect(&m_appletDelayedConfigurationTimer, &QTimer::timeout, this, &ContainmentInterface::updateAppletDelayedConfiguration);

    connect(&m_appletsExpandedConnectionsTimer, &QTimer::timeout, this, &ContainmentInterface::updateAppletsTracking);

    connect(m_view, &View::containmentChanged
            , this, [&]() {
        if (m_view->containment()) {
            connect(m_view->containment(), &Plasma::Containment::appletAdded, this, &ContainmentInterface::onAppletAdded);
            m_appletsExpandedConnectionsTimer.start();
        }
    });

    connect(m_latteTasksModel, &TasksModel::countChanged, this, &ContainmentInterface::onLatteTasksCountChanged);
    connect(m_plasmaTasksModel, &TasksModel::countChanged, this, &ContainmentInterface::onPlasmaTasksCountChanged);
}

ContainmentInterface::~ContainmentInterface()
{
}

void ContainmentInterface::identifyShortcutsHost()
{
    if (m_shortcutsHost) {
        return;
    }

    if (QQuickItem *graphicItem = m_view->containment()->property("_plasma_graphicObject").value<QQuickItem *>()) {
        const auto &childItems = graphicItem->childItems();

        for (QQuickItem *item : childItems) {
            if (item->objectName() == QLatin1String("containmentViewLayout")) {
                for (QQuickItem *subitem : item->childItems()) {
                    if (subitem->objectName() == QLatin1String("PositionShortcutsAbilityHost")) {
                        m_shortcutsHost = subitem;
                        identifyMethods();
                        return;
                    }
                }
            }
        }
    }
}

void ContainmentInterface::identifyMethods()
{
    int aeIndex = m_shortcutsHost->metaObject()->indexOfMethod("activateEntryAtIndex(QVariant)");
    int niIndex = m_shortcutsHost->metaObject()->indexOfMethod("newInstanceForEntryAtIndex(QVariant)");
    int sbIndex = m_shortcutsHost->metaObject()->indexOfMethod("setShowAppletShortcutBadges(QVariant,QVariant,QVariant,QVariant)");
    int afiIndex = m_shortcutsHost->metaObject()->indexOfMethod("appletIdForIndex(QVariant)");

    m_activateEntryMethod = m_shortcutsHost->metaObject()->method(aeIndex);
    m_appletIdForIndexMethod = m_shortcutsHost->metaObject()->method(afiIndex);
    m_newInstanceMethod = m_shortcutsHost->metaObject()->method(niIndex);
    m_showShortcutsMethod = m_shortcutsHost->metaObject()->method(sbIndex);
}

bool ContainmentInterface::applicationLauncherHasGlobalShortcut() const
{
    if (!containsApplicationLauncher()) {
        return false;
    }

    uint launcherAppletId = applicationLauncherId();

    const auto applets = m_view->containment()->applets();

    for (auto applet : applets) {
        if (applet->id() == launcherAppletId) {
            return !applet->globalShortcut().isEmpty();
        }
    }

    return false;
}

bool ContainmentInterface::applicationLauncherInPopup() const
{
    if (!containsApplicationLauncher()) {
        return false;
    }

    uint launcherAppletId = applicationLauncherId();
    const auto applets = m_view->containment()->applets();

    PlasmaQuick::AppletQuickItem *appLauncherItem{nullptr};

    for (auto applet : applets) {
        if (applet->id() == launcherAppletId) {
            appLauncherItem = applet->property("_plasma_graphicObject").value<PlasmaQuick::AppletQuickItem *>();
        }
    }

    return appLauncherItem && appletIsExpandable(appLauncherItem);
}

bool ContainmentInterface::containsApplicationLauncher() const
{
    return (applicationLauncherId() >= 0);
}

bool ContainmentInterface::isCapableToShowShortcutBadges()
{
    identifyShortcutsHost();

    if (!hasLatteTasks() && hasPlasmaTasks()) {
        return false;
    }

    return m_showShortcutsMethod.isValid();
}

bool ContainmentInterface::isApplication(const QUrl &url) const
{
    if (!url.isValid() || !url.isLocalFile()) {
        return false;
    }

    const QString &localPath = url.toLocalFile();

    if (!KDesktopFile::isDesktopFile(localPath)) {
        return false;
    }

    KDesktopFile desktopFile(localPath);
    return desktopFile.hasApplicationType();
}

int ContainmentInterface::applicationLauncherId() const
{
    const auto applets = m_view->containment()->applets();

    auto launcherId{-1};

    for (auto applet : applets) {
        const auto provides = applet->pluginMetaData().value(QStringLiteral("X-Plasma-Provides"));

        if (provides.contains(QLatin1String(Latte::PluginId::kLauncherMenu))) {
            if (!applet->globalShortcut().isEmpty()) {
                return applet->id();
            } else if (launcherId == -1) {
                launcherId = applet->id();
            }
        }
    }

    return launcherId;
}

bool ContainmentInterface::updateBadgeForLatteTask(const QString identifier, const QString value)
{
    if (!hasLatteTasks()) {
        return false;
    }

    const auto &applets = m_view->containment()->applets();

    for (auto *applet : applets) {
        KPluginMetaData meta = applet->pluginMetaData();

        if (meta.pluginId() == QLatin1String(Latte::PluginId::kPlasmoid)) {

            if (QQuickItem *appletInterface = applet->property("_plasma_graphicObject").value<QQuickItem *>()) {
                const auto &childItems = appletInterface->childItems();

                if (childItems.isEmpty()) {
                    continue;
                }

                for (QQuickItem *item : childItems) {
                    if (auto *metaObject = item->metaObject()) {
                        // not using QMetaObject::invokeMethod to avoid warnings when calling
                        // this on applets that don't have it or other child items since this
                        // is pretty much trial and error.
                        // Also, "var" arguments are treated as QVariant in QMetaObject

                        int methodIndex = metaObject->indexOfMethod("updateBadge(QVariant,QVariant)");

                        if (methodIndex == -1) {
                            continue;
                        }

                        QMetaMethod method = metaObject->method(methodIndex);

                        if (method.invoke(item, Q_ARG(QVariant, identifier), Q_ARG(QVariant, value))) {
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

bool ContainmentInterface::activatePlasmaTask(const int index)
{
    bool containsPlasmaTaskManager{hasPlasmaTasks() && !hasLatteTasks()};

    if (!containsPlasmaTaskManager) {
        return false;
    }

    const auto &applets = m_view->containment()->applets();

    for (auto *applet : applets) {
        const auto &provides = [&applet]() -> QStringList { const auto v = applet->pluginMetaData().rawData().value(QStringLiteral("X-Plasma-Provides")); if (v.isArray()) { QStringList r; for (const auto &e : v.toArray()) r << e.toString(); return r; } return v.toString().split(QLatin1Char(','), Qt::SkipEmptyParts); }();

        if (provides.contains(QLatin1String(Latte::PluginId::kMultiTasking))) {
            if (QQuickItem *appletInterface = applet->property("_plasma_graphicObject").value<QQuickItem *>()) {
                const auto &childItems = appletInterface->childItems();

                if (childItems.isEmpty()) {
                    continue;
                }

                KPluginMetaData meta = applet->pluginMetaData();

                for (QQuickItem *item : childItems) {
                    if (auto *metaObject = item->metaObject()) {
                        int methodIndex{metaObject->indexOfMethod("activateTaskAtIndex(QVariant)")};

                        if (methodIndex == -1) {
                            continue;
                        }

                        QMetaMethod method = metaObject->method(methodIndex);

                        if (method.invoke(item, Q_ARG(QVariant, index - 1))) {
                            showShortcutBadges(false, true);

                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

bool ContainmentInterface::newInstanceForPlasmaTask(const int index)
{
    bool containsPlasmaTaskManager{hasPlasmaTasks() && !hasLatteTasks()};

    if (!containsPlasmaTaskManager) {
        return false;
    }

    const auto &applets = m_view->containment()->applets();

    for (auto *applet : applets) {
        const auto &provides = [&applet]() -> QStringList { const auto v = applet->pluginMetaData().rawData().value(QStringLiteral("X-Plasma-Provides")); if (v.isArray()) { QStringList r; for (const auto &e : v.toArray()) r << e.toString(); return r; } return v.toString().split(QLatin1Char(','), Qt::SkipEmptyParts); }();

        if (provides.contains(QLatin1String(Latte::PluginId::kMultiTasking))) {
            if (QQuickItem *appletInterface = applet->property("_plasma_graphicObject").value<QQuickItem *>()) {
                const auto &childItems = appletInterface->childItems();

                if (childItems.isEmpty()) {
                    continue;
                }

                KPluginMetaData meta = applet->pluginMetaData();

                for (QQuickItem *item : childItems) {
                    if (auto *metaObject = item->metaObject()) {
                        int methodIndex{metaObject->indexOfMethod("newInstanceForTaskAtIndex(QVariant)")};

                        if (methodIndex == -1) {
                            continue;
                        }

                        QMetaMethod method = metaObject->method(methodIndex);

                        if (method.invoke(item, Q_ARG(QVariant, index - 1))) {
                            showShortcutBadges(false, true);

                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

bool ContainmentInterface::activateEntry(const int index)
{
    identifyShortcutsHost();

    if (!m_activateEntryMethod.isValid()) {
        return false;
    }

    return m_activateEntryMethod.invoke(m_shortcutsHost, Q_ARG(QVariant, index));
}

bool ContainmentInterface::newInstanceForEntry(const int index)
{
    identifyShortcutsHost();

    if (!m_newInstanceMethod.isValid()) {
        return false;
    }

    return m_newInstanceMethod.invoke(m_shortcutsHost, Q_ARG(QVariant, index));
}

bool ContainmentInterface::hideShortcutBadges()
{
    identifyShortcutsHost();

    if (!m_showShortcutsMethod.isValid()) {
        return false;
    }

    return m_showShortcutsMethod.invoke(m_shortcutsHost, Q_ARG(QVariant, false), Q_ARG(QVariant, false), Q_ARG(QVariant, false), Q_ARG(QVariant, -1));
}

bool ContainmentInterface::showOnlyMeta()
{
    // Meta forwarding to Latte via KWin is not supported on Wayland.
    return false;
}

bool ContainmentInterface::showShortcutBadges(const bool showLatteShortcuts, const bool showMeta)
{
    identifyShortcutsHost();

    if (!m_showShortcutsMethod.isValid() || !isCapableToShowShortcutBadges()) {
        return false;
    }

    int appLauncherId = -1;

    return m_showShortcutsMethod.invoke(m_shortcutsHost, Q_ARG(QVariant, showLatteShortcuts), Q_ARG(QVariant, true), Q_ARG(QVariant, showMeta), Q_ARG(QVariant, appLauncherId));
}

int ContainmentInterface::appletIdForVisualIndex(const int index)
{
    identifyShortcutsHost();

    if (!m_appletIdForIndexMethod.isValid()) {
        return -1;
    }

    QVariant appletId{-1};

    m_appletIdForIndexMethod.invoke(m_shortcutsHost, Q_RETURN_ARG(QVariant, appletId), Q_ARG(QVariant, index));

    return appletId.toInt();
}

int ContainmentInterface::appletIdForAppletIndex(const int appletIndex) const
{
    if (appletIndex < 0) {
        return -1;
    }

    if (appletIndex >= 0 && appletIndex < m_appletOrder.count()) {
        const int appletId = m_appletOrder.at(appletIndex);
        if (appletId > 0) {
            return appletId;
        }
    }

    for (auto it = m_appletData.constBegin(); it != m_appletData.constEnd(); ++it) {
        if (it.value().lastValidIndex == appletIndex && it.key() > 0) {
            return it.key();
        }
    }

    if (m_layoutManager) {
        const QList<int> layoutOrder = m_layoutManager->property("order").value<QList<int>>();
        if (appletIndex >= 0 && appletIndex < layoutOrder.count()) {
            const int appletId = layoutOrder.at(appletIndex);
            if (appletId > 0) {
                return appletId;
            }
        }
    }

    return -1;
}


void ContainmentInterface::deactivateApplets()
{
    if (!m_view->containment() || !m_view->inReadyState()) {
        return;
    }

    for (const auto applet : m_view->containment()->applets()) {
        PlasmaQuick::AppletQuickItem *ai = applet->property("_plasma_graphicObject").value<PlasmaQuick::AppletQuickItem *>();

        if (ai) {
            ai->setExpanded(false);
        }
    }
}

bool ContainmentInterface::appletIsExpandable(const int id) const
{
    if (!m_view->containment() || !m_view->inReadyState()) {
        return false;
    }

    for (const auto applet : m_view->containment()->applets()) {
        if (applet && applet->id() == (uint)id) {
            if (Layouts::Storage::self()->isSubContainment(m_view->corona(), applet)) {
                return true;
            }

            PlasmaQuick::AppletQuickItem *ai = applet->property("_plasma_graphicObject").value<PlasmaQuick::AppletQuickItem *>();

            if (ai) {
                return appletIsExpandable(ai);
            }
        }
    }

    return false;
}

bool ContainmentInterface::appletIsExpandable(PlasmaQuick::AppletQuickItem *appletQuickItem) const
{
    if (!appletQuickItem || !m_view->inReadyState()) {
        return false;
    }

    return ((appletQuickItem->fullRepresentation() != nullptr
            && appletQuickItem->preferredRepresentation() != appletQuickItem->fullRepresentation())
            || Latte::Layouts::Storage::self()->isSubContainment(m_view->corona(), appletQuickItem->applet()));
}

bool ContainmentInterface::appletIsActivationTogglesExpanded(const int id) const
{
    if (!m_view->containment() || !m_view->inReadyState()) {
        return false;
    }

    for (const auto applet : m_view->containment()->applets()) {
        if (applet && applet->id() == (uint)id) {
            if (Layouts::Storage::self()->isSubContainment(m_view->corona(), applet)) {
                return true;
            }

            PlasmaQuick::AppletQuickItem *ai = applet->property("_plasma_graphicObject").value<PlasmaQuick::AppletQuickItem *>();

            if (ai) {
                return ai->isActivationTogglesExpanded();
            }
        }
    }

    return false;
}

bool ContainmentInterface::hasExpandedApplet() const
{
    return m_expandedAppletIds.count() > 0;
}

bool ContainmentInterface::hasLatteTasks() const
{
    return (m_latteTasksModel->count() > 0);
}

bool ContainmentInterface::hasPlasmaTasks() const
{
    return (m_plasmaTasksModel->count() > 0);
}

bool ContainmentInterface::isInitialized() const
{
    return m_initializationCompleted;
}

int ContainmentInterface::indexOfApplet(const int &id)
{
    if (m_appletOrder.contains(id)) {
        return m_appletOrder.indexOf(id);
    } else if (m_appletData.contains(id)) {
        return m_appletData[id].lastValidIndex;
    }

    return -1;
}

ViewPart::AppletInterfaceData ContainmentInterface::appletDataAtIndex(const int &index)
{
    ViewPart::AppletInterfaceData data;

    if (index<0 || (index > (m_appletOrder.count()-1))) {
        return data;
    }

    return m_appletData[m_appletOrder[index]];
}

ViewPart::AppletInterfaceData ContainmentInterface::appletDataForId(const int &id)
{
    ViewPart::AppletInterfaceData data;

    if (!m_appletData.contains(id)) {
        return data;
    }

    return m_appletData[id];
}

QObject *ContainmentInterface::plasmoid() const
{
    return m_plasmoid;
}

void ContainmentInterface::setPlasmoid(QObject *plasmoid)
{
    if (m_plasmoid == plasmoid) {
        return;
    }

    m_plasmoid = plasmoid;

    if (m_plasmoid) {
        m_configuration = dynamic_cast<QQmlPropertyMap *>(m_plasmoid->property("configuration").value<QObject *>());

        if (m_configuration) {
            connect(m_configuration, &QQmlPropertyMap::valueChanged, this, &ContainmentInterface::containmentConfigPropertyChanged);
        }
    }

    Q_EMIT plasmoidChanged();
}

QObject *ContainmentInterface::layoutManager() const
{
    return m_layoutManager;
}

void ContainmentInterface::setLayoutManager(QObject *manager)
{
    if (m_layoutManager == manager) {
        return;
    }

    m_layoutManager = manager;

    if (!m_layoutManager) {
        Q_EMIT layoutManagerChanged();
        return;
    }

    // applets order
    int metaorderindex = m_layoutManager->metaObject()->indexOfProperty("order");
    if (metaorderindex >= 0) {
        QMetaProperty metaorder = m_layoutManager->metaObject()->property(metaorderindex);
        if (metaorder.hasNotifySignal()) {
            QMetaMethod metaorderchanged = metaorder.notifySignal();
            QMetaMethod metaupdateappletorder = this->metaObject()->method(this->metaObject()->indexOfSlot("updateAppletsOrder()"));
            connect(m_layoutManager, metaorderchanged, this, metaupdateappletorder);
            updateAppletsOrder();
        }
    }

    // applets in locked zoom
    metaorderindex = m_layoutManager->metaObject()->indexOfProperty("lockedZoomApplets");
    if (metaorderindex >= 0) {
        QMetaProperty metaorder = m_layoutManager->metaObject()->property(metaorderindex);
        if (metaorder.hasNotifySignal()) {
            QMetaMethod metaorderchanged = metaorder.notifySignal();
            QMetaMethod metaupdateapplets = this->metaObject()->method(this->metaObject()->indexOfSlot("updateAppletsInLockedZoom()"));
            connect(m_layoutManager, metaorderchanged, this, metaupdateapplets);
            updateAppletsInLockedZoom();
        }
    }

    // applets disabled their autocoloring
    metaorderindex = m_layoutManager->metaObject()->indexOfProperty("userBlocksColorizingApplets");
    if (metaorderindex >= 0) {
        QMetaProperty metaorder = m_layoutManager->metaObject()->property(metaorderindex);
        if (metaorder.hasNotifySignal()) {
            QMetaMethod metaorderchanged = metaorder.notifySignal();
            QMetaMethod metaupdateapplets = this->metaObject()->method(this->metaObject()->indexOfSlot("updateAppletsDisabledColoring()"));
            connect(m_layoutManager, metaorderchanged, this, metaupdateapplets);
            updateAppletsDisabledColoring();
        }
    }

    Q_EMIT layoutManagerChanged();
}

void ContainmentInterface::addApplet(const QString &pluginId)
{
    qDebug() << "org.kde.sync containment addApplet(plugin)"
             << "containment" << (m_view && m_view->containment() ? m_view->containment()->id() : 0)
             << "screen" << (m_view && m_view->screen() ? m_view->screen()->name() : QStringLiteral("<none>"))
             << "plugin" << pluginId;

    if (pluginId.isEmpty()) {
        return;
    }

    // Calculate the default insertion index and set it BEFORE creating the
    // applet so the QML handler's addAppletItem call places it correctly.
    if (m_layoutManager) {
        QList<int> order = m_layoutManager->property("order").value<QList<int>>();
        if (!order.isEmpty()) {
            int defaultIndex = calculateDefaultAppletInsertionIndex(order);
            m_layoutManager->setProperty("_latte_pendingInsertionIndex", defaultIndex);
        }
    }

    suppressNextAppletCreatedSignal();
    Plasma::Applet *createdApplet = m_view->containment()->createApplet(pluginId);
    qDebug() << "org.kde.sync containment createApplet(plugin) result"
             << "containment" << (m_view && m_view->containment() ? m_view->containment()->id() : 0)
             << "screen" << (m_view && m_view->screen() ? m_view->screen()->name() : QStringLiteral("<none>"))
             << "plugin" << pluginId
             << "created" << (createdApplet ? createdApplet->id() : 0)
             << "failed" << (createdApplet ? createdApplet->failedToLaunch() : true);

    if (!createdApplet && m_suppressedAppletCreations > 0) {
        --m_suppressedAppletCreations;
    }
}

void ContainmentInterface::suppressNextAppletCreatedSignal()
{
    ++m_suppressedAppletCreations;
}

void ContainmentInterface::addApplet(QObject *metadata, int x, int y)
{
    qDebug() << "org.kde.sync containment addApplet(drop)"
             << "containment" << (m_view && m_view->containment() ? m_view->containment()->id() : 0)
             << "screen" << (m_view && m_view->screen() ? m_view->screen()->name() : QStringLiteral("<none>"))
             << "coords" << QPoint(x, y);

    if (!m_plasmoid) {
        return;
    }

    int processmimedataindex = m_plasmoid->metaObject()->indexOfMethod("processMimeData(QObject*,int,int)");
    if (processmimedataindex < 0) {
        return;
    }

    QMetaMethod processmethod = m_plasmoid->metaObject()->method(processmimedataindex);
    suppressNextAppletCreatedSignal();
    const bool invoked = processmethod.invoke(m_plasmoid,
                                              Q_ARG(QObject *, metadata),
                                              Q_ARG(int, x),
                                              Q_ARG(int, y));
    if (!invoked && m_suppressedAppletCreations > 0) {
        --m_suppressedAppletCreations;
    }
}

bool ContainmentInterface::addInternalSeparatorBeforeApplet(const int appletId)
{
    if (!m_view || !m_view->containment() || appletId < 0) {
        return false;
    }

    QList<int> layoutOrder;
    if (m_layoutManager) {
        layoutOrder = m_layoutManager->property("order").value<QList<int>>();
    }
    const QList<int> effectiveOrder = bestOrderForApplet(m_appletOrder, layoutOrder, appletId);

    const int targetIndex = effectiveOrder.indexOf(appletId);
    if (targetIndex < 0) {
        return false;
    }

    QHash<int, QString> containmentPlugins;
    if (m_view && m_view->containment()) {
        const auto applets = m_view->containment()->applets();
        containmentPlugins.reserve(applets.count());

        for (const auto applet : applets) {
            if (!applet) {
                continue;
            }

            containmentPlugins.insert(static_cast<int>(applet->id()), pluginIdFromMetaData(applet->pluginMetaData()));
        }
    }

    auto pluginIdForApplet = [this, &containmentPlugins](const int id) -> QString {
        const auto it = m_appletData.constFind(id);
        if (it != m_appletData.constEnd()) {
            return it->plugin;
        }

        return containmentPlugins.value(id);
    };

    int leftNeighborId = -1;

    for (int i = targetIndex - 1; i >= 0; --i) {
        const int candidateId = effectiveOrder.at(i);
        const QString pluginId = pluginIdForApplet(candidateId);

        if (pluginId == QLatin1String(kInternalViewSplitterPluginId)) {
            continue;
        }

        leftNeighborId = candidateId;
        break;
    }

    // No real applet on the left means this would create a leading separator.
    if (leftNeighborId < 0) {
        return false;
    }

    if (isSeparatorPluginId(pluginIdForApplet(leftNeighborId))) {
        return true;
    }

    const QStringList separatorPlugins{QLatin1String(kLatteSeparatorPluginId)};

    Plasma::Applet *separatorApplet{nullptr};

    for (const QString &pluginId : separatorPlugins) {
        separatorApplet = m_view->containment()->createApplet(pluginId);
        if (separatorApplet && !separatorApplet->failedToLaunch()) {
            break;
        }

        if (separatorApplet) {
            separatorApplet->destroy();
            separatorApplet = nullptr;
        }
    }

    if (!separatorApplet) {
        return false;
    }

    QList<int> newOrder = effectiveOrder;
    const int separatorId = static_cast<int>(separatorApplet->id());

    if (!newOrder.contains(appletId)) {
        separatorApplet->destroy();
        return false;
    }

    if (!newOrder.contains(separatorId)) {
        newOrder << separatorId;
    }

    newOrder.removeAll(separatorId);
    newOrder.insert(targetIndex, separatorId);

    setAppletsOrder(newOrder);
    // Separator applets can be added asynchronously by Plasma. Re-apply the
    // same order shortly after creation so boundary separators stay anchored
    // to the requested applet side instead of falling back to default side.
    QTimer::singleShot(0, this, [this, newOrder]() {
        setAppletsOrder(newOrder);
    });
    QTimer::singleShot(150, this, [this, newOrder]() {
        setAppletsOrder(newOrder);
    });
    QTimer::singleShot(300, this, [this, newOrder]() {
        saveAppletsOrder(newOrder);
    });

    return true;
}

bool ContainmentInterface::addInternalSeparatorAtLeftBoundaryOfApplet(const int appletId)
{
    if (!m_view || !m_view->containment() || appletId < 0) {
        return false;
    }

    QList<int> layoutOrder;
    if (m_layoutManager) {
        layoutOrder = m_layoutManager->property("order").value<QList<int>>();
    }
    const QList<int> effectiveOrder = bestOrderForApplet(m_appletOrder, layoutOrder, appletId);

    QHash<int, QString> containmentPlugins;
    if (m_view && m_view->containment()) {
        const auto applets = m_view->containment()->applets();
        containmentPlugins.reserve(applets.count());

        for (const auto applet : applets) {
            if (!applet) {
                continue;
            }

            containmentPlugins.insert(static_cast<int>(applet->id()), pluginIdFromMetaData(applet->pluginMetaData()));
        }
    }

    auto pluginIdForApplet = [this, &containmentPlugins](const int id) -> QString {
        const auto it = m_appletData.constFind(id);
        if (it != m_appletData.constEnd()) {
            return it->plugin;
        }

        return containmentPlugins.value(id);
    };

    const int currentIndex = effectiveOrder.indexOf(appletId);
    if (currentIndex <= 0) {
        return false;
    }

    int leftNeighborId = -1;

    for (int i = currentIndex - 1; i >= 0; --i) {
        const int candidateId = effectiveOrder.at(i);

        if (candidateId == appletId) {
            continue;
        }

        const QString pluginId = pluginIdForApplet(candidateId);
        if (pluginId == QLatin1String(kInternalViewSplitterPluginId)) {
            continue;
        }

        if (isSeparatorPluginId(pluginId)) {
            return true;
        }

        leftNeighborId = candidateId;
        break;
    }

    if (leftNeighborId < 0) {
        return false;
    }

    if (isSeparatorPluginId(pluginIdForApplet(leftNeighborId))) {
        return false;
    }

    return addInternalSeparatorBeforeApplet(appletId);
}

bool ContainmentInterface::addInternalSeparatorAtRightBoundaryOfApplet(const int appletId)
{
    if (!m_view || !m_view->containment() || appletId < 0) {
        return false;
    }

    QList<int> layoutOrder;
    if (m_layoutManager) {
        layoutOrder = m_layoutManager->property("order").value<QList<int>>();
    }
    const QList<int> effectiveOrder = bestOrderForApplet(m_appletOrder, layoutOrder, appletId);

    QHash<int, QString> containmentPlugins;
    if (m_view && m_view->containment()) {
        const auto applets = m_view->containment()->applets();
        containmentPlugins.reserve(applets.count());

        for (const auto applet : applets) {
            if (!applet) {
                continue;
            }

            containmentPlugins.insert(static_cast<int>(applet->id()), pluginIdFromMetaData(applet->pluginMetaData()));
        }
    }

    auto pluginIdForApplet = [this, &containmentPlugins](const int id) -> QString {
        const auto it = m_appletData.constFind(id);
        if (it != m_appletData.constEnd()) {
            return it->plugin;
        }

        return containmentPlugins.value(id);
    };

    const int currentIndex = effectiveOrder.indexOf(appletId);
    if (currentIndex < 0 || currentIndex >= (effectiveOrder.count() - 1)) {
        return false;
    }

    int rightNeighborId = -1;

    for (int i = currentIndex + 1; i < effectiveOrder.count(); ++i) {
        const int candidateId = effectiveOrder.at(i);

        if (candidateId == appletId) {
            continue;
        }

        const QString pluginId = pluginIdForApplet(candidateId);
        if (pluginId == QLatin1String(kInternalViewSplitterPluginId)) {
            continue;
        }

        if (isSeparatorPluginId(pluginId)) {
            return true;
        }

        rightNeighborId = candidateId;
        break;
    }

    if (rightNeighborId < 0) {
        return false;
    }

    if (isSeparatorPluginId(pluginIdForApplet(rightNeighborId))) {
        return false;
    }

    return addInternalSeparatorBeforeApplet(rightNeighborId);
}

bool ContainmentInterface::removeInternalSeparatorAtLeftBoundaryOfApplet(const int appletId)
{
    if (!m_view || !m_view->containment() || appletId < 0) {
        return false;
    }

    QList<int> layoutOrder;
    if (m_layoutManager) {
        layoutOrder = m_layoutManager->property("order").value<QList<int>>();
    }
    const QList<int> effectiveOrder = bestOrderForApplet(m_appletOrder, layoutOrder, appletId);

    const int currentIndex = effectiveOrder.indexOf(appletId);
    if (currentIndex <= 0) {
        return false;
    }

    QHash<int, QString> containmentPlugins;
    QHash<int, Plasma::Applet *> containmentApplets;
    const auto applets = m_view->containment()->applets();
    containmentPlugins.reserve(applets.count());
    containmentApplets.reserve(applets.count());

    for (const auto applet : applets) {
        if (!applet) {
            continue;
        }

        const int id = static_cast<int>(applet->id());
        containmentPlugins.insert(id, pluginIdFromMetaData(applet->pluginMetaData()));
        containmentApplets.insert(id, applet);
    }

    auto pluginIdForApplet = [this, &containmentPlugins](const int id) -> QString {
        const auto it = m_appletData.constFind(id);
        if (it != m_appletData.constEnd()) {
            return it->plugin;
        }

        return containmentPlugins.value(id);
    };

    int separatorId = -1;

    for (int i = currentIndex - 1; i >= 0; --i) {
        const int candidateId = effectiveOrder.at(i);
        const QString pluginId = pluginIdForApplet(candidateId);

        if (pluginId == QLatin1String(kInternalViewSplitterPluginId)) {
            continue;
        }

        if (isSeparatorPluginId(pluginId)) {
            separatorId = candidateId;
        }
        break;
    }

    if (separatorId < 0) {
        return false;
    }

    QList<int> newOrder = effectiveOrder;
    newOrder.removeAll(separatorId);
    setAppletsOrder(newOrder);

    if (m_appletData.contains(separatorId)) {
        removeApplet(separatorId);
    } else if (containmentApplets.contains(separatorId) && containmentApplets[separatorId]) {
        containmentApplets[separatorId]->destroy();
    }

    QTimer::singleShot(0, this, [this, newOrder]() {
        setAppletsOrder(newOrder);
    });
    QTimer::singleShot(120, this, [this, newOrder]() {
        setAppletsOrder(newOrder);
    });
    QTimer::singleShot(250, this, [this, newOrder]() {
        saveAppletsOrder(newOrder);
    });

    return true;
}

bool ContainmentInterface::removeInternalSeparatorAtRightBoundaryOfApplet(const int appletId)
{
    if (!m_view || !m_view->containment() || appletId < 0) {
        return false;
    }

    QList<int> layoutOrder;
    if (m_layoutManager) {
        layoutOrder = m_layoutManager->property("order").value<QList<int>>();
    }
    const QList<int> effectiveOrder = bestOrderForApplet(m_appletOrder, layoutOrder, appletId);

    const int currentIndex = effectiveOrder.indexOf(appletId);
    if (currentIndex < 0 || currentIndex >= (effectiveOrder.count() - 1)) {
        return false;
    }

    QHash<int, QString> containmentPlugins;
    QHash<int, Plasma::Applet *> containmentApplets;
    const auto applets = m_view->containment()->applets();
    containmentPlugins.reserve(applets.count());
    containmentApplets.reserve(applets.count());

    for (const auto applet : applets) {
        if (!applet) {
            continue;
        }

        const int id = static_cast<int>(applet->id());
        containmentPlugins.insert(id, pluginIdFromMetaData(applet->pluginMetaData()));
        containmentApplets.insert(id, applet);
    }

    auto pluginIdForApplet = [this, &containmentPlugins](const int id) -> QString {
        const auto it = m_appletData.constFind(id);
        if (it != m_appletData.constEnd()) {
            return it->plugin;
        }

        return containmentPlugins.value(id);
    };

    int separatorId = -1;

    for (int i = currentIndex + 1; i < effectiveOrder.count(); ++i) {
        const int candidateId = effectiveOrder.at(i);
        const QString pluginId = pluginIdForApplet(candidateId);

        if (pluginId == QLatin1String(kInternalViewSplitterPluginId)) {
            continue;
        }

        if (isSeparatorPluginId(pluginId)) {
            separatorId = candidateId;
        }
        break;
    }

    if (separatorId < 0) {
        return false;
    }

    QList<int> newOrder = effectiveOrder;
    newOrder.removeAll(separatorId);
    setAppletsOrder(newOrder);

    if (m_appletData.contains(separatorId)) {
        removeApplet(separatorId);
    } else if (containmentApplets.contains(separatorId) && containmentApplets[separatorId]) {
        containmentApplets[separatorId]->destroy();
    }

    QTimer::singleShot(0, this, [this, newOrder]() {
        setAppletsOrder(newOrder);
    });
    QTimer::singleShot(120, this, [this, newOrder]() {
        setAppletsOrder(newOrder);
    });
    QTimer::singleShot(250, this, [this, newOrder]() {
        saveAppletsOrder(newOrder);
    });

    return true;
}

void ContainmentInterface::addExpandedApplet(PlasmaQuick::AppletQuickItem * appletQuickItem)
{
    if (appletQuickItem && m_expandedAppletIds.contains(appletQuickItem) && appletIsExpandable(appletQuickItem)) {
        return;
    }

    bool isExpanded = hasExpandedApplet();

    m_expandedAppletIds[appletQuickItem] = appletQuickItem->applet()->id();

    if (isExpanded != hasExpandedApplet()) {
        Q_EMIT hasExpandedAppletChanged();
    }

    Q_EMIT expandedAppletStateChanged();
}

void ContainmentInterface::removeExpandedApplet(PlasmaQuick::AppletQuickItem *appletQuickItem)
{
    if (!m_expandedAppletIds.contains(appletQuickItem)) {
        return;
    }

    bool isExpanded = hasExpandedApplet();

    m_expandedAppletIds.remove(appletQuickItem);

    if (isExpanded != hasExpandedApplet()) {
        Q_EMIT hasExpandedAppletChanged();
    }

    Q_EMIT expandedAppletStateChanged();
}

QAbstractListModel *ContainmentInterface::latteTasksModel() const
{
    return m_latteTasksModel;
}

QAbstractListModel *ContainmentInterface::plasmaTasksModel() const
{
    return m_plasmaTasksModel;
}

void ContainmentInterface::onAppletExpandedChanged()
{
    PlasmaQuick::AppletQuickItem *appletItem = static_cast<PlasmaQuick::AppletQuickItem *>(QObject::sender());

    if (appletItem) {
        bool added{false};

        if (appletItem->isExpanded()) {
            if (appletItem->switchWidth()>0 && appletItem->switchHeight()>0) {
                added = ((appletItem->width()<=appletItem->switchWidth())
                         && (appletItem->height()<=appletItem->switchHeight()));
            } else {
                added = true;
            }
        }

        if (added && appletIsExpandable(appletItem)) {
            addExpandedApplet(appletItem);
        } else {
            removeExpandedApplet(appletItem);
        }
    }
}

QList<int> ContainmentInterface::appletsOrder() const
{
    return m_appletOrder;
}

void ContainmentInterface::cleanupInvalidSeparatorApplets()
{
    if (m_cleaningSeparatorApplets || !m_layoutManager || !m_view || !m_view->containment()) {
        return;
    }

    const QList<int> currentOrder = m_layoutManager->property("order").value<QList<int>>();
    if (currentOrder.isEmpty()) {
        return;
    }

    QHash<int, QString> containmentPlugins;
    QHash<int, Plasma::Applet *> containmentApplets;
    const auto applets = m_view->containment()->applets();
    containmentPlugins.reserve(applets.count());
    containmentApplets.reserve(applets.count());

    for (auto *applet : applets) {
        if (!applet) {
            continue;
        }

        const int id = static_cast<int>(applet->id());
        containmentPlugins.insert(id, pluginIdFromMetaData(applet->pluginMetaData()));
        containmentApplets.insert(id, applet);
    }

    auto pluginIdForApplet = [this, &containmentPlugins](const int id) -> QString {
        const auto it = m_appletData.constFind(id);
        if (it != m_appletData.constEnd() && !it->plugin.isEmpty()) {
            return it->plugin;
        }

        return containmentPlugins.value(id);
    };

    const QList<int> sanitizedOrder = sanitizeSeparatorOrder(currentOrder, m_appletData, containmentPlugins);
    QSet<int> orderIds;
    QSet<int> separatorIdsToDestroy;

    for (const int id : currentOrder) {
        if (id > 0) {
            orderIds.insert(id);
        }

        if (!sanitizedOrder.contains(id) && isSeparatorPluginId(pluginIdForApplet(id))) {
            separatorIdsToDestroy.insert(id);
        }
    }

    for (auto it = containmentPlugins.constBegin(); it != containmentPlugins.constEnd(); ++it) {
        if (!orderIds.contains(it.key()) && isSeparatorPluginId(it.value())) {
            separatorIdsToDestroy.insert(it.key());
        }
    }

    if (sanitizedOrder == currentOrder && separatorIdsToDestroy.isEmpty()) {
        return;
    }

    m_cleaningSeparatorApplets = true;

    if (sanitizedOrder != currentOrder) {
        setAppletsOrder(sanitizedOrder);
    }

    for (const int id : separatorIdsToDestroy) {
        if (m_appletData.contains(id)) {
            removeApplet(id);
        } else if (containmentApplets.contains(id) && containmentApplets.value(id)) {
            containmentApplets.value(id)->destroy();
        }
    }

    m_cleaningSeparatorApplets = false;

    QTimer::singleShot(120, this, [this, sanitizedOrder]() {
        setAppletsOrder(sanitizedOrder);
    });
    QTimer::singleShot(300, this, [this, sanitizedOrder]() {
        saveAppletsOrder(sanitizedOrder);
    });
}

void ContainmentInterface::saveAppletsOrder(const QList<int> &order)
{
    if (!m_layoutManager || !m_view || !m_view->containment()) {
        return;
    }

    QHash<int, QString> containmentPlugins;
    const auto applets = m_view->containment()->applets();
    containmentPlugins.reserve(applets.count());

    for (const auto applet : applets) {
        if (!applet) {
            continue;
        }

        containmentPlugins.insert(static_cast<int>(applet->id()), pluginIdFromMetaData(applet->pluginMetaData()));
    }

    const QList<int> sanitizedOrder = sanitizeSeparatorOrder(order, m_appletData, containmentPlugins);
    setAppletsOrder(sanitizedOrder);

    QStringList serializedOrder;
    serializedOrder.reserve(sanitizedOrder.count());

    for (const int appletId : sanitizedOrder) {
        if (appletId > 0) {
            serializedOrder << QString::number(appletId);
        }
    }

    KConfigGroup generalConfig = m_view->containment()->config().group("General");
    generalConfig.writeEntry("appletOrder", serializedOrder.join(QLatin1Char(';')));
    generalConfig.sync();
}

void ContainmentInterface::updateAppletsOrder()
{
    if (!m_layoutManager) {
        return;
    }

    QList<int> neworder = m_layoutManager->property("order").value<QList<int>>();

    QHash<int, QString> containmentPlugins;
    if (m_view && m_view->containment()) {
        const auto applets = m_view->containment()->applets();
        containmentPlugins.reserve(applets.count());

        for (const auto applet : applets) {
            if (!applet) {
                continue;
            }

            containmentPlugins.insert(static_cast<int>(applet->id()), pluginIdFromMetaData(applet->pluginMetaData()));
        }
    }

    const QList<int> sanitizedOrder = sanitizeSeparatorOrder(neworder, m_appletData, containmentPlugins);
    const bool orderWasSanitized = sanitizedOrder != neworder;

    if (orderWasSanitized) {
        neworder = sanitizedOrder;
        setAppletsOrder(neworder);
        QTimer::singleShot(0, this, &ContainmentInterface::cleanupInvalidSeparatorApplets);
    }

    if (m_appletOrder == neworder) {
        return;
    }

    m_appletOrder = neworder;
    qDebug() << "org.kde.sync containment appletsOrderChanged"
             << "containment" << (m_view && m_view->containment() ? m_view->containment()->id() : 0)
             << "screen" << (m_view && m_view->screen() ? m_view->screen()->name() : QStringLiteral("<none>"))
             << "order" << m_appletOrder;

    //! update applets last recorded index, this is needed for example when an applet is removed
    //! to know in which index was located before the removal
    for(const auto &id: m_appletOrder) {
        if (m_appletData.contains(id)) {
            m_appletData[id].lastValidIndex = m_appletOrder.indexOf(id);
        }
    }

    Q_EMIT appletsOrderChanged();

    QTimer::singleShot(0, this, &ContainmentInterface::cleanupInvalidSeparatorApplets);
}

void ContainmentInterface::updateAppletsInLockedZoom()
{
    if (!m_layoutManager) {
        return;
    }

    QList<int> appletslockedzoom = m_layoutManager->property("lockedZoomApplets").value<QList<int>>();

    if (m_appletsInLockedZoom == appletslockedzoom) {
        return;
    }

    m_appletsInLockedZoom = appletslockedzoom;
    Q_EMIT appletsInLockedZoomChanged(m_appletsInLockedZoom);
}

void ContainmentInterface::updateAppletsDisabledColoring()
{
    if (!m_layoutManager) {
        return;
    }

    QList<int> appletsdisabledcoloring = m_layoutManager->property("userBlocksColorizingApplets").value<QList<int>>();

    if (m_appletsDisabledColoring == appletsdisabledcoloring) {
        return;
    }

    m_appletsDisabledColoring = appletsdisabledcoloring;
    Q_EMIT appletsDisabledColoringChanged(appletsdisabledcoloring);
}

void ContainmentInterface::onLatteTasksCountChanged()
{
    if ((m_hasLatteTasks && m_latteTasksModel->count()>0)
            || (!m_hasLatteTasks && m_latteTasksModel->count() == 0)) {
        return;
    }

    m_hasLatteTasks = (m_latteTasksModel->count() > 0);
    Q_EMIT hasLatteTasksChanged();
}

void ContainmentInterface::onPlasmaTasksCountChanged()
{
    if ((m_hasPlasmaTasks && m_plasmaTasksModel->count()>0)
            || (!m_hasPlasmaTasks && m_plasmaTasksModel->count() == 0)) {
        return;
    }

    m_hasPlasmaTasks = (m_plasmaTasksModel->count() > 0);
    Q_EMIT hasPlasmaTasksChanged();
}

bool ContainmentInterface::appletIsExpanded(const int id) const
{
    return m_expandedAppletIds.values().contains(id);
}

void ContainmentInterface::toggleAppletExpanded(const int id)
{
    if (!m_view->containment() || !m_view->inReadyState()) {
        return;
    }

    for (const auto applet : m_view->containment()->applets()) {
        if (applet->id() == (uint)id && !Layouts::Storage::self()->isSubContainment(m_view->corona(), applet)/*block for sub-containments*/) {
            PlasmaQuick::AppletQuickItem *ai = applet->property("_plasma_graphicObject").value<PlasmaQuick::AppletQuickItem *>();

            if (ai) {
                Q_EMIT applet->activated();
            }
        }
    }
}

void ContainmentInterface::removeApplet(const int &id)
{
    qDebug() << "org.kde.sync containment removeApplet"
             << "containment" << (m_view && m_view->containment() ? m_view->containment()->id() : 0)
             << "screen" << (m_view && m_view->screen() ? m_view->screen()->name() : QStringLiteral("<none>"))
             << "id" << id
             << "known" << m_appletData.contains(id);

    if (!m_appletData.contains(id)) {
        return;
    }

    auto applet = m_appletData[id].applet;
    Q_EMIT applet->appletDeleted(applet); //! this signal should be part of Plasma Frameworks AppletPrivate::destroy() function...
    applet->destroy();
}


void ContainmentInterface::setAppletsOrder(const QList<int> &order)
{
    QHash<int, QString> containmentPlugins;
    if (m_view && m_view->containment()) {
        const auto applets = m_view->containment()->applets();
        containmentPlugins.reserve(applets.count());

        for (const auto applet : applets) {
            if (!applet) {
                continue;
            }

            containmentPlugins.insert(static_cast<int>(applet->id()), pluginIdFromMetaData(applet->pluginMetaData()));
        }
    }

    const QList<int> sanitizedOrder = sanitizeSeparatorOrder(order, m_appletData, containmentPlugins);

    QMetaObject::invokeMethod(m_layoutManager,
                              "requestAppletsOrder",
                              Qt::DirectConnection,
                              Q_ARG(QList<int>, sanitizedOrder));

}

void ContainmentInterface::setAppletsInLockedZoom(const QList<int> &applets)
{
    QMetaObject::invokeMethod(m_layoutManager,
                              "requestAppletsInLockedZoom",
                              Qt::DirectConnection,
                              Q_ARG(QList<int>, applets));
}

void ContainmentInterface::setAppletsDisabledColoring(const QList<int> &applets)
{
    QMetaObject::invokeMethod(m_layoutManager,
                              "requestAppletsDisabledColoring",
                              Qt::DirectConnection,
                              Q_ARG(QList<int>, applets));
}

void ContainmentInterface::setAppletInScheduledDestruction(const int &id, const bool &enabled)
{
    QMetaObject::invokeMethod(m_layoutManager,
                              "setAppletInScheduledDestruction",
                              Qt::DirectConnection,
                              Q_ARG(int, id),
                              Q_ARG(bool, enabled));
}

void ContainmentInterface::updateContainmentConfigProperty(const QString &key, const QVariant &value)
{
    if (!m_configuration) {
        return;
    }

    // Check if the key already exists with the same value
    if (m_configuration->keys().contains(key) && (*m_configuration)[key] == value) {
        return;
    }

    m_configuration->insert(key, value);
    Q_EMIT m_configuration->valueChanged(key, value);
}

void ContainmentInterface::emitContainmentConfigProperties()
{
    if (!m_configuration) {
        return;
    }

    const QStringList keys = m_configuration->keys();
    for (const QString &key : keys) {
        Q_EMIT containmentConfigPropertyChanged(key, (*m_configuration)[key]);
    }
}

void ContainmentInterface::updateAppletConfigProperty(const int &id, const QString &key, const QVariant &value)
{
    if (!m_appletData.contains(id) || !m_appletData[id].configuration || !m_appletData[id].configuration->keys().contains(key)) {
        return;
    }

    if (m_appletData[id].configuration->keys().contains(key)
            && (*m_appletData[id].configuration)[key] != value) {
        m_appletData[id].configuration->insert(key, value);
        Q_EMIT m_appletData[id].configuration->valueChanged(key, value);
    }
}

void ContainmentInterface::updateAppletsTracking()
{
    if (!m_view->containment()) {
        return;
    }

    for (const auto applet : m_view->containment()->applets()) {
        onAppletAdded(applet);
    }

    m_initializationCompleted = true;
    qDebug() << "org.kde.sync containment initializationCompleted"
             << "containment" << (m_view && m_view->containment() ? m_view->containment()->id() : 0)
             << "screen" << (m_view && m_view->screen() ? m_view->screen()->name() : QStringLiteral("<none>"))
             << "order" << m_appletOrder
             << "knownApplets" << m_appletData.keys();
    Q_EMIT initializationCompleted();
    QTimer::singleShot(0, this, &ContainmentInterface::cleanupInvalidSeparatorApplets);
    QTimer::singleShot(250, this, &ContainmentInterface::cleanupInvalidSeparatorApplets);
}

void ContainmentInterface::updateAppletDelayedConfiguration()
{
    for (const auto id : m_appletData.keys()) {
        if (!m_appletData[id].configuration) {
            m_appletData[id].configuration = appletConfiguration(m_appletData[id].applet);

            if (m_appletData[id].configuration) {
                qDebug() << "org.kde.sync delayed applet configuration was successful for : " << id;
                initAppletConfigurationSignals(id, m_appletData[id].configuration);
            }
        }
    }
}

void ContainmentInterface::initAppletConfigurationSignals(const int &id, QQmlPropertyMap *configuration)
{
    if (!configuration) {
        return;
    }

    connect(configuration, &QQmlPropertyMap::valueChanged,
            this, [&, id](const QString &key, const QVariant &value) {
        //qDebug() << "org.kde.sync applet property changed : " << currentAppletId << " __ " << m_appletData[currentAppletId].plugin << " __ " << key << " __ " << value;
        Q_EMIT appletConfigPropertyChanged(id, key, value);
    });
}

QQmlPropertyMap *ContainmentInterface::appletConfiguration(const Plasma::Applet *applet)
{
    if (!m_view->containment() || !applet) {
        return nullptr;
    }

    PlasmaQuick::AppletQuickItem *ai = applet->property("_plasma_graphicObject").value<PlasmaQuick::AppletQuickItem *>();
    bool isSubContainment = Layouts::Storage::self()->isSubContainment(m_view->corona(), applet); //we use corona() to make sure that returns true even when it is first created from user
    int currentAppletId = applet->id();
    QQmlPropertyMap *configuration{nullptr};

    if (!ai) {
        return nullptr;
    }

    //! set configuration object properly for applets and subcontainments
    if (!isSubContainment) {
        int metaconfigindex = ai->metaObject()->indexOfProperty("configuration");
        if (metaconfigindex >=0 ){
            configuration = dynamic_cast<QQmlPropertyMap *>((ai->property("configuration")).value<QObject *>());
        }
    } else {
        Plasma::Containment *subcontainment = Layouts::Storage::self()->subContainmentOf(m_view->corona(), applet);
        if (subcontainment) {
            PlasmaQuick::AppletQuickItem *subcai = subcontainment->property("_plasma_graphicObject").value<PlasmaQuick::AppletQuickItem *>();

            if (subcai) {
                int metaconfigindex = subcai->metaObject()->indexOfProperty("configuration");
                if (metaconfigindex >=0 ){
                    configuration = dynamic_cast<QQmlPropertyMap *>((subcai->property("configuration")).value<QObject *>());
                }
            }
        }
    }

    return configuration;
}

void ContainmentInterface::onAppletAdded(Plasma::Applet *applet)
{
    if (!m_view->containment() || !applet) {
        return;
    }

    // Check for a pending insertion index set by View::handlePlasmoidDrop.
    // The QML Containment.onAppletAdded handler cannot match the Plasma 6
    // signal signature (appletAdded(Plasma::Applet*, const QRectF&) vs the
    // expected (applet, int, int)), so we handle position-aware insertion
    // here in C++ instead.
    if (m_layoutManager) {
        QVariant pendingIndex = m_layoutManager->property("_latte_pendingInsertionIndex");
        if (pendingIndex.isValid()) {
            int index = pendingIndex.toInt();
            m_layoutManager->setProperty("_latte_pendingInsertionIndex", QVariant()); // clear

            // Call addAppletItem via invokeMethod because the LayoutManager
            // type is defined in the containment plugin, not linked directly.
            QMetaObject::invokeMethod(m_layoutManager,
                                      "addAppletItem",
                                      Q_ARG(QObject *, applet),
                                      Q_ARG(int, index));
        } else {
            // No drag-drop index. The QML handler never fires in Plasma 6,
            // so we must create the container ourselves here. Place new
            // applets at the end of left-side widgets (before system tray).
            QList<int> order = m_layoutManager->property("order").value<QList<int>>();
            if (!order.isEmpty() && !order.contains(applet->id())) {
                const QString pluginId = pluginIdFromMetaData(applet->pluginMetaData());
                if (pluginId != QLatin1String(kLatteSeparatorPluginId)
                    && pluginId != QLatin1String(kLegacySeparatorPluginId)
                    && pluginId != QLatin1String(kInternalViewSplitterPluginId)) {
                    int defaultIndex = calculateDefaultAppletInsertionIndex(order);
                    QMetaObject::invokeMethod(m_layoutManager,
                                              "addAppletItem",
                                              Q_ARG(QObject *, applet),
                                              Q_ARG(int, defaultIndex));
                }
            }
        }
    }

    PlasmaQuick::AppletQuickItem *ai = PlasmaQuick::AppletQuickItem::itemForApplet(applet);
    bool isSubContainment = Layouts::Storage::self()->isSubContainment(m_view->corona(), applet); //we use corona() to make sure that returns true even when it is first created from user
    int currentAppletId = applet->id();
    KPluginMetaData appletMeta = applet->pluginMetaData();
    const QString currentPluginId = pluginIdFromMetaData(appletMeta);
    const QStringList currentProvides = appletProvidesFromMetaData(appletMeta);
    bool initializing{!m_appletData.contains(currentAppletId)};

    if (initializing
            && m_initializationCompleted
            && !m_cleaningSeparatorApplets
            && !m_handledRuntimeAppletCreations.contains(currentAppletId)) {
        m_handledRuntimeAppletCreations.insert(currentAppletId);

        if (m_suppressedAppletCreations > 0) {
            qDebug() << "org.kde.sync containment appletCreated suppressed"
                     << "containment" << m_view->containment()->id()
                     << "screen" << (m_view->screen() ? m_view->screen()->name() : QStringLiteral("<none>"))
                     << "id" << currentAppletId
                     << "plugin" << currentPluginId
                     << "suppressed" << m_suppressedAppletCreations;
            --m_suppressedAppletCreations;
        } else {
            qDebug() << "org.kde.sync containment appletCreated"
                     << "containment" << m_view->containment()->id()
                     << "screen" << (m_view->screen() ? m_view->screen()->name() : QStringLiteral("<none>"))
                     << "id" << currentAppletId
                     << "plugin" << currentPluginId;
            Q_EMIT appletCreated(currentPluginId);
        }
    }

    //! Track expanded/able applets and Tasks applets
    if (isSubContainment) {
        //! internal containment case
        Plasma::Containment *subContainment = Layouts::Storage::self()->subContainmentOf(m_view->corona(), applet);
        PlasmaQuick::AppletQuickItem *contAi = ai;

        if (contAi && !m_appletsExpandedConnections.contains(contAi)) {
            m_appletsExpandedConnections[contAi] = connect(contAi, &PlasmaQuick::AppletQuickItem::expandedChanged, this, &ContainmentInterface::onAppletExpandedChanged);

            connect(contAi, &QObject::destroyed, this, [&, contAi](){
                m_appletsExpandedConnections.remove(contAi);
                removeExpandedApplet(contAi);
            });
        }

        for (const auto internalApplet : subContainment->applets()) {
            PlasmaQuick::AppletQuickItem *ai = internalApplet->property("_plasma_graphicObject").value<PlasmaQuick::AppletQuickItem *>();

            if (ai && !m_appletsExpandedConnections.contains(ai) ){
                m_appletsExpandedConnections[ai] = connect(ai, &PlasmaQuick::AppletQuickItem::expandedChanged, this, &ContainmentInterface::onAppletExpandedChanged);

                connect(ai, &QObject::destroyed, this, [&, ai](){
                    m_appletsExpandedConnections.remove(ai);
                    removeExpandedApplet(ai);
                });
            }
        }
    } else if (ai) {
        if (appletMeta.pluginId() == QLatin1String(Latte::PluginId::kPlasmoid)) {
            //! populate latte tasks applet
            m_latteTasksModel->addTask(ai);
        } else if (currentProvides.contains(QLatin1String(Latte::PluginId::kMultiTasking))) {
            //! populate plasma tasks applet
            m_plasmaTasksModel->addTask(ai);
        } else if (!m_appletsExpandedConnections.contains(ai)) {
            m_appletsExpandedConnections[ai] = connect(ai, &PlasmaQuick::AppletQuickItem::expandedChanged, this, &ContainmentInterface::onAppletExpandedChanged);

            connect(ai, &QObject::destroyed, this, [&, ai](){
                m_appletsExpandedConnections.remove(ai);
                removeExpandedApplet(ai);
            });
        }
    }

    //! Track all applets, for example to support syncing between different docks.
    //! The PlasmaQuick item can be missing during startup, but structural sync
    //! still needs applet id/plugin metadata to map originals to clones.
    {
        ViewPart::AppletInterfaceData data;
        data.id = currentAppletId;
        data.plugin = currentPluginId;
        data.provides = currentProvides;
        data.applet = applet;
        data.plasmoid = ai;
        data.lastValidIndex = m_appletOrder.indexOf(data.id);
        //! set configuration object properly for applets and subcontainments
        data.configuration = appletConfiguration(applet);

        //! track property changes in applets
        if (data.configuration) {
            initAppletConfigurationSignals(data.id, data.configuration);
        } else {
            qDebug() << "org.kde.sync Unfortunately configuration syncing for :: " << currentAppletId << " was not established, configuration object was missing!";
            m_appletDelayedConfigurationTimer.start();
        }

        if (initializing) {
            //! track applet destroyed flag
            connect(applet, &Plasma::Applet::destroyedChanged, this, [&, currentAppletId, applet](bool destroyed) {
                Q_EMIT appletInScheduledDestructionChanged(currentAppletId, destroyed);
                if (!destroyed && m_layoutManager) {
                    // Undo: re-show the item that was hidden by the
                    // context menu's early hideAppletItem.
                    QMetaObject::invokeMethod(m_layoutManager,
                                              "showAppletItem",
                                              Qt::DirectConnection,
                                              Q_ARG(QObject *, applet));
                }
            });

            //! remove on applet destruction
            connect(applet, &QObject::destroyed, this, [&, data](){
                qDebug() << "org.kde.sync containment appletRemoved"
                         << "containment" << (m_view && m_view->containment() ? m_view->containment()->id() : 0)
                         << "screen" << (m_view && m_view->screen() ? m_view->screen()->name() : QStringLiteral("<none>"))
                         << "id" << data.id
                         << "plugin" << data.plugin;
                Q_EMIT appletRemoved(data.id);
                //qDebug() << "org.kde.sync: removing applet ::: " << data.id << " __ " << data.plugin << " remained : " << m_appletData.keys();
                m_appletData.remove(data.id);
            });
        }

        m_appletData[data.id] = data;
        Q_EMIT appletDataCreated(data.id);
    }

    QTimer::singleShot(0, this, &ContainmentInterface::cleanupInvalidSeparatorApplets);
    QTimer::singleShot(250, this, &ContainmentInterface::cleanupInvalidSeparatorApplets);
}

QList<int> ContainmentInterface::toIntList(const QVariantList &list)
{
    QList<int> converted;

    for(const QVariant &item: list) {
        converted << item.toInt();
    }

    return converted;
}

int ContainmentInterface::calculateDefaultAppletInsertionIndex(const QList<int> &order)
{
    // Identify boundary applets (system tray, task manager).  New
    // applets go just before the first boundary encountered when
    // scanning left-to-right.  If none found, append to the end.

    QSet<int> boundaryIds;

    if (m_view && m_view->containment()) {
        const auto applets = m_view->containment()->applets();
        for (const auto *applet : applets) {
            if (!applet) {
                continue;
            }
            const QString pluginId = pluginIdFromMetaData(applet->pluginMetaData());
            if (pluginId == QLatin1String(Latte::PluginId::kSystemTray)
                || pluginId == QLatin1String("org.nomad.systemtray")
                || pluginId == QLatin1String(Latte::PluginId::kPlasmoid)) {
                boundaryIds.insert(static_cast<int>(applet->id()));
            }
        }
    }

    for (int i = 0; i < order.count(); ++i) {
        if (boundaryIds.contains(order[i])) {
            return i; // Insert just before the boundary
        }
    }

    return order.count();
}

}
}
