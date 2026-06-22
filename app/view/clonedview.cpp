/*
    SPDX-FileCopyrightText: 2021 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "clonedview.h"
#include "containmentinterface.h"
#include "visibilitymanager.h"
#include "../data/viewdata.h"
#include "../layouts/storage.h"
#include "../pluginids.h"

// Qt
#include <QDebug>

namespace {
QString appletSyncKey(const Latte::ViewPart::AppletInterfaceData &data)
{
    if (data.provides.contains(QLatin1String(Latte::PluginId::kLauncherMenu))) {
        return QString::fromLatin1(Latte::PluginId::kLauncherMenu);
    }

    return data.plugin;
}
}

namespace Latte {

const int ClonedView::ERRORAPPLETID;

QStringList ClonedView::CONTAINMENTMANUALSYNCEDPROPERTIES = QStringList()
        << QString("appletOrder")
        << QString("lockedZoomApplets")
        << QString("userBlocksColorizingApplets");  

ClonedView::ClonedView(Plasma::Corona *corona, Latte::OriginalView *originalView, QScreen *targetScreen)
    : View(corona, targetScreen),
      m_originalView(originalView)
{
    m_originalView->addClone(this);
    initSync();
}

ClonedView::~ClonedView()
{
}

void ClonedView::initSync()
{
    connect(m_originalView, &View::containmentChanged, this, &View::groupIdChanged);

    m_originalInitialized = m_originalView
            && m_originalView->extendedInterface()
            && m_originalView->extendedInterface()->isInitialized();
    m_cloneInitialized = extendedInterface() && extendedInterface()->isInitialized();

    //! Update Visibility From Original
    connect(m_originalView->visibility(), &Latte::ViewPart::VisibilityManager::modeChanged, this, [this]() {
        visibility()->setMode(m_originalView->visibility()->mode());
    });

    connect(m_originalView->visibility(), &Latte::ViewPart::VisibilityManager::raiseOnDesktopChanged, this, [this]() {
        visibility()->setRaiseOnDesktop(m_originalView->visibility()->raiseOnDesktop());
    });

    connect(m_originalView->visibility(), &Latte::ViewPart::VisibilityManager::raiseOnActivityChanged, this, [this]() {
        visibility()->setRaiseOnActivity(m_originalView->visibility()->raiseOnActivity());
    });

    connect(m_originalView->visibility(), &Latte::ViewPart::VisibilityManager::enableKWinEdgesChanged, this, [this]() {
        visibility()->setEnableKWinEdges(m_originalView->visibility()->enableKWinEdges());
    });

    connect(m_originalView->visibility(), &Latte::ViewPart::VisibilityManager::timerShowChanged, this, [this]() {
        visibility()->setTimerShow(m_originalView->visibility()->timerShow());
    });

    connect(m_originalView->visibility(), &Latte::ViewPart::VisibilityManager::timerHideChanged, this, [this]() {
        visibility()->setTimerHide(m_originalView->visibility()->timerHide());
    });


    //! Update Applets from Clone -> OriginalView
    connect(extendedInterface(), &Latte::ViewPart::ContainmentInterface::appletConfigPropertyChanged, this, &ClonedView::updateOriginalAppletConfigProperty);
    connect(extendedInterface(), &Latte::ViewPart::ContainmentInterface::initializationCompleted, this, [this]() {
        m_cloneInitialized = true;
        updateAppletIdsHash();
        debugSyncState(QStringLiteral("clone initialization completed"));
    });
    connect(extendedInterface(), &Latte::ViewPart::ContainmentInterface::appletsOrderChanged, this, &ClonedView::updateAppletIdsHash);
    connect(extendedInterface(), &Latte::ViewPart::ContainmentInterface::appletInScheduledDestructionChanged, this, &ClonedView::onCloneAppletInScheduledDestructionChanged);
    connect(extendedInterface(), &Latte::ViewPart::ContainmentInterface::appletRemoved, this, &ClonedView::onCloneAppletRemoved);
    connect(extendedInterface(), &Latte::ViewPart::ContainmentInterface::appletsOrderChanged, this, &ClonedView::onCloneAppletsOrderChanged);
    connect(extendedInterface(), &Latte::ViewPart::ContainmentInterface::appletsInLockedZoomChanged, this, &ClonedView::onCloneAppletsInLockedZoomChanged);
    connect(extendedInterface(), &Latte::ViewPart::ContainmentInterface::appletsDisabledColoringChanged, this, &ClonedView::onCloneAppletsDisabledColoringChanged);
    connect(extendedInterface(), &Latte::ViewPart::ContainmentInterface::appletDataCreated, this, [this]() {
        if (refreshAppletIdsHash()) {
            if (m_syncingFromOriginal) {
                onOriginalAppletsOrderChanged();
            } else {
                onCloneAppletsOrderChanged();
            }
        }
    });
    connect(extendedInterface(), &Latte::ViewPart::ContainmentInterface::appletCreated, this, [this](const QString &pluginId) {
        debugSyncState(QStringLiteral("clone appletCreated %1").arg(pluginId));
        if (m_syncingFromOriginal) {
            return;
        }

        m_originalView->addApplet(pluginId, containment()->id());
    });

    connect(extendedInterface(), &Latte::ViewPart::ContainmentInterface::appletDropped, this, [this](QObject *data, int x, int y) {
        debugSyncState(QStringLiteral("clone appletDropped %1,%2").arg(x).arg(y));
        if (m_syncingFromOriginal) {
            return;
        }

        m_originalView->addApplet(data, x, y, containment()->id());
        onCloneAppletsOrderChanged();
    });

    //! When clone QML finishes initializing, re-sync all containment config
    //! from the original to ensure consistency (catch up on changes that
    //! arrived before the clone's m_configuration was available).
    connect(extendedInterface(), &Latte::ViewPart::ContainmentInterface::initializationCompleted, this, [this]() {
        if (m_originalView && m_originalView->extendedInterface()) {
            m_originalView->extendedInterface()->emitContainmentConfigProperties();
        }
    });

    //! Update Applets and Containment from OrigalView -> Clone
    connect(m_originalView->extendedInterface(), &Latte::ViewPart::ContainmentInterface::containmentConfigPropertyChanged, this, &ClonedView::updateContainmentConfigProperty);
    connect(m_originalView->extendedInterface(), &Latte::ViewPart::ContainmentInterface::appletConfigPropertyChanged, this, &ClonedView::onOriginalAppletConfigPropertyChanged);
    connect(m_originalView->extendedInterface(), &Latte::ViewPart::ContainmentInterface::appletInScheduledDestructionChanged, this, &ClonedView::onOriginalAppletInScheduledDestructionChanged);
    connect(m_originalView->extendedInterface(), &Latte::ViewPart::ContainmentInterface::appletRemoved, this, &ClonedView::onOriginalAppletRemoved);
    connect(m_originalView->extendedInterface(), &Latte::ViewPart::ContainmentInterface::appletsOrderChanged, this, &ClonedView::onOriginalAppletsOrderChanged);
    connect(m_originalView->extendedInterface(), &Latte::ViewPart::ContainmentInterface::appletsInLockedZoomChanged, this, &ClonedView::onOriginalAppletsInLockedZoomChanged);
    connect(m_originalView->extendedInterface(), &Latte::ViewPart::ContainmentInterface::appletsDisabledColoringChanged, this, &ClonedView::onOriginalAppletsDisabledColoringChanged);
    connect(m_originalView->extendedInterface(), &Latte::ViewPart::ContainmentInterface::initializationCompleted, this, [this]() {
        m_originalInitialized = true;
        updateAppletIdsHash();
        debugSyncState(QStringLiteral("original initialization completed"));
    });
    connect(m_originalView->extendedInterface(), &Latte::ViewPart::ContainmentInterface::appletDataCreated, this, [this]() {
        if (refreshAppletIdsHash()) {
            onOriginalAppletsOrderChanged();
        }
    });
    connect(m_originalView->extendedInterface(), &Latte::ViewPart::ContainmentInterface::appletCreated, this->extendedInterface(), [this](const QString &pluginId) {
        debugSyncState(QStringLiteral("original appletCreated %1").arg(pluginId));
        m_syncingFromOriginal = true;
        extendedInterface()->addApplet(pluginId);
        m_syncingFromOriginal = false;
        onOriginalAppletsOrderChanged();
    });
    connect(m_originalView->extendedInterface(), &Latte::ViewPart::ContainmentInterface::appletDropped, this->extendedInterface(), [this](QObject *data, int x, int y) {
        debugSyncState(QStringLiteral("original appletDropped %1,%2").arg(x).arg(y));
        m_syncingFromOriginal = true;
        extendedInterface()->addApplet(data, x, y);
        m_syncingFromOriginal = false;
        onOriginalAppletsOrderChanged();
    });

    //! Indicator
    connect(m_originalView, &Latte::View::indicatorChanged, this, &ClonedView::indicatorChanged);
}

bool ClonedView::isSingle() const
{
    return false;
}

bool ClonedView::isOriginal() const
{
    return false;
}

bool ClonedView::isCloned() const
{
    return true;
}

bool ClonedView::isPreferredForShortcuts() const
{
    return false;
}

int ClonedView::groupId() const
{
    if (!m_originalView->containment()) {
        return -1;
    }

    return m_originalView->containment()->id();
}

Latte::Types::ScreensGroup ClonedView::screensGroup() const
{
    return Latte::Types::SingleScreenGroup;
}

ViewPart::Indicator *ClonedView::indicator() const
{
    return m_originalView->indicator();
}


bool ClonedView::hasOriginalAppletId(const int &clonedid)
{
    if (clonedid < 0) {
        return false;
    }

    QHash<int, int>::const_iterator i = m_currentAppletIds.constBegin();
    while (i != m_currentAppletIds.constEnd()) {
        if (i.value() == clonedid) {
            return true;
        }

        ++i;
    }

    return false;
}

int ClonedView::originalAppletId(const int &clonedid)
{
    if (clonedid < 0) {
        return -1;
    }

    QHash<int, int>::const_iterator i = m_currentAppletIds.constBegin();
    while (i != m_currentAppletIds.constEnd()) {
        if (i.value() == clonedid) {
            return i.key();
        }

        ++i;
    }

    return -1;
}


bool ClonedView::isTranslatableToClonesOrder(const QList<int> &originalOrder)
{
    for(int i=0; i<originalOrder.count(); ++i) {
        int oid = originalOrder[i];
        if (oid < 0 ) {
            continue;
        }

        if (!m_currentAppletIds.contains(oid)) {
            return false;
        }
    }

    return true;
}

bool ClonedView::isTranslatableToOriginalsOrder(const QList<int> &clonedOrder)
{
    for(int i=0; i<clonedOrder.count(); ++i) {
        int cid = clonedOrder[i];
        if (cid < 0 ) {
            continue;
        }

        if (!hasOriginalAppletId(cid)) {
            return false;
        }
    }

    return true;
}

Latte::Data::View ClonedView::data() const
{
    Latte::Data::View vdata = View::data();
    vdata.isClonedFrom = m_originalView->containment()->id();
    return vdata;
}

void ClonedView::updateAppletIdsHash()
{
    refreshAppletIdsHash();
}

bool ClonedView::refreshAppletIdsHash()
{
    QList<int> originalids = m_originalView->extendedInterface()->appletsOrder();
    QList<int> clonedids = extendedInterface()->appletsOrder();
    const QHash<int, int> previousAppletIds = m_currentAppletIds;
    QHash<int, int> nextAppletIds;
    QSet<int> usedClonedIds;

    for (auto it = m_currentAppletIds.constBegin(); it != m_currentAppletIds.constEnd(); ++it) {
        ViewPart::AppletInterfaceData originalapplet = m_originalView->extendedInterface()->appletDataForId(it.key());
        ViewPart::AppletInterfaceData clonedapplet = extendedInterface()->appletDataForId(it.value());

        if (originalapplet.id > 0
                && clonedapplet.id > 0
                && appletSyncKey(originalapplet) == appletSyncKey(clonedapplet)) {
            nextAppletIds[it.key()] = it.value();
            usedClonedIds.insert(it.value());
        }
    }

    for (const int oid : originalids) {
        if (oid < 0 || !m_currentAppletIds.contains(oid) || nextAppletIds.contains(oid)) {
            continue;
        }

        const int cid = m_currentAppletIds.value(oid);
        ViewPart::AppletInterfaceData originalapplet = m_originalView->extendedInterface()->appletDataForId(oid);
        ViewPart::AppletInterfaceData clonedapplet = extendedInterface()->appletDataForId(cid);

        if (originalapplet.id > 0
                && clonedapplet.id > 0
                && appletSyncKey(originalapplet) == appletSyncKey(clonedapplet)
                && !usedClonedIds.contains(cid)) {
            nextAppletIds[oid] = cid;
            usedClonedIds.insert(cid);
        }
    }

    for (const int oid : originalids) {
        if (oid < 0 || nextAppletIds.contains(oid)) {
            continue;
        }

        ViewPart::AppletInterfaceData originalapplet = m_originalView->extendedInterface()->appletDataForId(oid);
        const QString originalKey = appletSyncKey(originalapplet);
        if (originalapplet.id <= 0 || originalKey.isEmpty()) {
            continue;
        }

        for (const int cid : clonedids) {
            if (cid < 0 || usedClonedIds.contains(cid)) {
                continue;
            }

            ViewPart::AppletInterfaceData clonedapplet = extendedInterface()->appletDataForId(cid);
            if (clonedapplet.id > 0 && originalKey == appletSyncKey(clonedapplet)) {
                nextAppletIds[originalapplet.id] = clonedapplet.id;
                usedClonedIds.insert(clonedapplet.id);
                break;
            }
        }
    }

    m_currentAppletIds = nextAppletIds;
    debugSyncState(QStringLiteral("updateAppletIdsHash"));
    return m_currentAppletIds != previousAppletIds;
}

QList<int> ClonedView::translateToClonesOrder(const QList<int> &originalIds)
{
    QList<int> ids;

    for (int i=0; i<originalIds.count(); ++i) {
        int originalid = originalIds[i];
        if (originalid < 0 ) {
            ids << originalid;
            continue;
        }

        if (m_currentAppletIds.contains(originalid)) {
            ids << m_currentAppletIds[originalid];
        }
    }

    return ids;
}

QList<int> ClonedView::translateToOriginalsOrder(const QList<int> &clonedIds)
{
    QList<int> ids;

    for (int i=0; i<clonedIds.count(); ++i) {
        int clonedid = clonedIds[i];
        if (clonedid < 0 ) {
            ids << clonedid;
            continue;
        }

        int originalid = originalAppletId(clonedid);
        if (originalid > 0) {
            ids << originalid;
        }
    }

    return ids;
}

QList<int> ClonedView::orderWithUnmappedAppletsPreserved(const QList<int> &sourceOrder, const QList<int> &targetOrder, const bool toClones)
{
    const QList<int> translated = toClones ? translateToClonesOrder(sourceOrder) : translateToOriginalsOrder(sourceOrder);

    if (translated.isEmpty()) {
        return {};
    }

    QList<int> result = targetOrder;
    int translatedIndex{0};

    for (int i=0; i<result.count() && translatedIndex<translated.count(); ++i) {
        const int targetId = result[i];
        const bool mappedTarget = toClones ? hasOriginalAppletId(targetId) : m_currentAppletIds.contains(targetId);

        if (!mappedTarget) {
            continue;
        }

        result[i] = translated[translatedIndex];
        ++translatedIndex;
    }

    while (translatedIndex < translated.count()) {
        result << translated[translatedIndex];
        ++translatedIndex;
    }

    return result;
}

bool ClonedView::structuralSyncReady() const
{
    return m_originalInitialized && m_cloneInitialized && !m_currentAppletIds.isEmpty();
}

void ClonedView::debugSyncState(const QString &where) const
{
    qDebug() << "org.kde.sync"
             << where
             << "originalContainment" << (m_originalView && m_originalView->containment() ? m_originalView->containment()->id() : 0)
             << "cloneContainment" << (containment() ? containment()->id() : 0)
             << "screen" << (screen() ? screen()->name() : QStringLiteral("<none>"))
             << "originalInitialized" << m_originalInitialized
             << "cloneInitialized" << m_cloneInitialized
             << "mapping" << m_currentAppletIds
             << "originalOrder" << (m_originalView && m_originalView->extendedInterface() ? m_originalView->extendedInterface()->appletsOrder() : QList<int>())
             << "cloneOrder" << (extendedInterface() ? extendedInterface()->appletsOrder() : QList<int>());
}

void ClonedView::showConfigurationInterface(Plasma::Applet *applet)
{
    Plasma::Containment *c = qobject_cast<Plasma::Containment *>(applet);

    if (Layouts::Storage::self()->isLatteContainment(c)) {
        m_originalView->showSettingsWindow();
    } else {
        View::showConfigurationInterface(applet);
    }
}

void ClonedView::onCloneAppletRemoved(const int &id)
{
    debugSyncState(QStringLiteral("onCloneAppletRemoved %1").arg(id));
    if (m_cloneRemovalsFromOriginal.remove(id) || m_syncingFromOriginal || !structuralSyncReady()) {
        debugSyncState(QStringLiteral("onCloneAppletRemoved blocked %1").arg(id));
        return;
    }

    int originalid = originalAppletId(id);

    if (originalid < 0) {
        return;
    }

    m_currentAppletIds.remove(originalid);
    m_originalView->extendedInterface()->removeApplet(originalid);
}

void ClonedView::onCloneAppletInScheduledDestructionChanged(const int &id, const bool &enabled)
{
    if (m_syncingFromOriginal || !structuralSyncReady()) {
        return;
    }

    int originalid = originalAppletId(id);

    if (originalid < 0) {
        return;
    }

    m_originalView->extendedInterface()->setAppletInScheduledDestruction(originalid, enabled);
}

void ClonedView::onOriginalAppletRemoved(const int &id)
{
    debugSyncState(QStringLiteral("onOriginalAppletRemoved %1").arg(id));
    if (!structuralSyncReady()) {
        debugSyncState(QStringLiteral("onOriginalAppletRemoved blocked not ready %1").arg(id));
        return;
    }

    if (!m_currentAppletIds.contains(id)) {
        debugSyncState(QStringLiteral("onOriginalAppletRemoved blocked unmapped %1").arg(id));
        return;
    }

    const int clonedid = m_currentAppletIds[id];
    m_cloneRemovalsFromOriginal.insert(clonedid);
    extendedInterface()->removeApplet(clonedid);
    m_currentAppletIds.remove(id);
}

void ClonedView::onOriginalAppletConfigPropertyChanged(const int &id, const QString &key, const QVariant &value)
{
    if (!m_currentAppletIds.contains(id)) {
        return;
    }

    extendedInterface()->updateAppletConfigProperty(m_currentAppletIds[id], key, value);
}

void ClonedView::onOriginalAppletInScheduledDestructionChanged(const int &id, const bool &enabled)
{
    if (!structuralSyncReady()) {
        return;
    }

    if (!m_currentAppletIds.contains(id)) {
        return;
    }

    m_syncingFromOriginal = true;
    extendedInterface()->setAppletInScheduledDestruction(m_currentAppletIds[id], enabled);
    m_syncingFromOriginal = false;
}

void ClonedView::updateContainmentConfigProperty(const QString &key, const QVariant &value)
{
    if (!CONTAINMENTMANUALSYNCEDPROPERTIES.contains(key)) {
        extendedInterface()->updateContainmentConfigProperty(key, value);
    } else {
        //qDebug() << "org.kde.sync :: containment config value syncing blocked :: " << key;
    }
}

void ClonedView::updateOriginalAppletConfigProperty(const int &clonedid, const QString &key, const QVariant &value)
{
    if (!hasOriginalAppletId(clonedid)) {
        return;
    }

    m_originalView->extendedInterface()->updateAppletConfigProperty(originalAppletId(clonedid), key, value);
}

void ClonedView::onCloneAppletsOrderChanged()
{
    debugSyncState(QStringLiteral("onCloneAppletsOrderChanged"));
    if (m_syncingFromOriginal || !structuralSyncReady()) {
        debugSyncState(QStringLiteral("onCloneAppletsOrderChanged blocked"));
        return;
    }

    updateAppletIdsHash();
    QList<int> clonedorder = extendedInterface()->appletsOrder();
    QList<int> neworiginalorder = orderWithUnmappedAppletsPreserved(clonedorder, m_originalView->extendedInterface()->appletsOrder(), false);

    if (neworiginalorder.isEmpty()) {
        return;
    }

    m_originalView->extendedInterface()->setAppletsOrder(neworiginalorder);
}

void ClonedView::onCloneAppletsInLockedZoomChanged(const QList<int> &clonedapplets)
{
    if (m_syncingFromOriginal || !structuralSyncReady()) {
        return;
    }

    QList<int> neworiginalorder = translateToOriginalsOrder(clonedapplets);

    m_originalView->extendedInterface()->setAppletsInLockedZoom(neworiginalorder);
}

void ClonedView::onCloneAppletsDisabledColoringChanged(const QList<int> &clonedapplets)
{
    if (m_syncingFromOriginal || !structuralSyncReady()) {
        return;
    }

    QList<int> neworiginalorder = translateToOriginalsOrder(clonedapplets);

    m_originalView->extendedInterface()->setAppletsDisabledColoring(neworiginalorder);
}

void ClonedView::onOriginalAppletsOrderChanged()
{
    debugSyncState(QStringLiteral("onOriginalAppletsOrderChanged"));
    if (!structuralSyncReady()) {
        updateAppletIdsHash();
        debugSyncState(QStringLiteral("onOriginalAppletsOrderChanged blocked"));
        return;
    }

    updateAppletIdsHash();
    QList<int> originalorder = m_originalView->extendedInterface()->appletsOrder();
    QList<int> newclonesorder = orderWithUnmappedAppletsPreserved(originalorder, extendedInterface()->appletsOrder(), true);

    if (newclonesorder.isEmpty()) {
        return;
    }

    m_syncingFromOriginal = true;
    extendedInterface()->setAppletsOrder(newclonesorder);
    m_syncingFromOriginal = false;
}

void ClonedView::onOriginalAppletsInLockedZoomChanged(const QList<int> &originalapplets)
{
    if (!structuralSyncReady()) {
        return;
    }

    QList<int> newclonesorder = translateToClonesOrder(originalapplets);

    m_syncingFromOriginal = true;
    extendedInterface()->setAppletsInLockedZoom(newclonesorder);
    m_syncingFromOriginal = false;
}

void ClonedView::onOriginalAppletsDisabledColoringChanged(const QList<int> &originalapplets)
{
    if (!structuralSyncReady()) {
        return;
    }

    QList<int> newclonesorder = translateToClonesOrder(originalapplets);

    m_syncingFromOriginal = true;
    extendedInterface()->setAppletsDisabledColoring(newclonesorder);
    m_syncingFromOriginal = false;
}


}
