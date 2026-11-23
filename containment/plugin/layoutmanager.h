/*
    SPDX-FileCopyrightText: 2021 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef CONTAINMENTLAYOUTMANAGER_H
#define CONTAINMENTLAYOUTMANAGER_H

//Qt
#include <QHash>
#include <QMetaMethod>
#include <QObject>
#include <QPointer>
#include <QQmlPropertyMap>
#include <QQuickItem>
#include <QSet>
#include <QTimer>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

// local
#include "layoutmanagerapi.h"

// Plasma
// Full definition needed: moc's metatype helpers require the return type of
// containmentObject() to be complete.
#include <Plasma/Containment>

namespace Latte {
namespace Containment {

class LayoutManager : public QObject, public LayoutManagerApi
{
    Q_OBJECT
    QML_NAMED_ELEMENT(LayoutManager)
    Q_INTERFACES(Latte::Containment::LayoutManagerApi)
    Q_PROPERTY(QObject *plasmoidObj READ plasmoid() WRITE setPlasmoid NOTIFY plasmoidChanged)

    Q_PROPERTY(QQuickItem *rootItem READ rootItem WRITE setRootItem NOTIFY rootItemChanged)
    Q_PROPERTY(QQuickItem *mainLayout READ mainLayout WRITE setMainLayout NOTIFY mainLayoutChanged)
    Q_PROPERTY(QQuickItem *startLayout READ startLayout WRITE setStartLayout NOTIFY startLayoutChanged)
    Q_PROPERTY(QQuickItem *endLayout READ endLayout WRITE setEndLayout NOTIFY endLayoutChanged)

    Q_PROPERTY(QQuickItem *dndSpacerItem READ dndSpacer WRITE setDndSpacer NOTIFY dndSpacerChanged)
    Q_PROPERTY(QQuickItem *metrics READ metrics WRITE setMetrics NOTIFY metricsChanged)

    Q_PROPERTY(bool hasRestoredApplets READ hasRestoredApplets NOTIFY hasRestoredAppletsChanged)

    //! this is the only way I have found to write their values properly in the configuration file in Multiple mode
    //! if they are not used from qml side in the form of plasmoid.configuration..... then
    //! appletsOrder is not stored when needed and applets additions/removals are not valid on next startup
    Q_PROPERTY(int splitterPosition READ splitterPosition NOTIFY splitterPositionChanged)
    Q_PROPERTY(int splitterPosition2 READ splitterPosition2 NOTIFY splitterPosition2Changed)
    Q_PROPERTY(QList<int> appletOrder READ appletOrder NOTIFY appletOrderChanged)
    Q_PROPERTY(QList<int> order READ order NOTIFY orderChanged)
    Q_PROPERTY(QList<int> lockedZoomApplets READ lockedZoomApplets NOTIFY lockedZoomAppletsChanged)
    Q_PROPERTY(QList<int> userBlocksColorizingApplets READ userBlocksColorizingApplets NOTIFY userBlocksColorizingAppletsChanged)
    Q_PROPERTY(QList<int> appletsInScheduledDestruction READ appletsInScheduledDestruction NOTIFY appletsInScheduledDestructionChanged)

public:
    static const int JUSTIFYSPLITTERID = -10;

    LayoutManager(QObject *parent = nullptr);
    ~LayoutManager() override;

    bool hasRestoredApplets() const;

    int splitterPosition() const;
    int splitterPosition2() const;
    QList<int> appletOrder() const;
    QList<int> lockedZoomApplets() const override;
    QList<int> order() const override;
    QList<int> userBlocksColorizingApplets() const override;
    QList<int> appletsInScheduledDestruction() const;

    QObject *plasmoid() const;
    void setPlasmoid(QObject *plasmoid);

    QQuickItem *rootItem() const override;
    void setRootItem(QQuickItem *root);

    QQuickItem *mainLayout() const;
    void setMainLayout(QQuickItem *main);

    QQuickItem *startLayout() const;
    void setStartLayout(QQuickItem *start);

    QQuickItem *endLayout() const;
    void setEndLayout(QQuickItem *end);

    QQuickItem *dndSpacer() const override;
    void setDndSpacer(QQuickItem *dnd);

    QQuickItem *metrics() const;
    void setMetrics(QQuickItem *metrics);

public Q_SLOTS:
    Q_INVOKABLE void restore();
    Q_INVOKABLE void save();
    Q_INVOKABLE void saveOptions();
    Q_INVOKABLE void setOption(const int &appletId, const QString &property, const QVariant &value) override;

    Q_INVOKABLE void addAppletItem(QObject *applet, int x, int y);
    Q_INVOKABLE void addAppletItem(QObject *applet, int index) override;
    Q_INVOKABLE void removeAppletItem(QObject *applet) override;
    Q_INVOKABLE void hideAppletItem(QObject *applet) override;
    Q_INVOKABLE void showAppletItem(QObject *applet) override;

    Q_INVOKABLE void addJustifySplittersInMainLayout();
    Q_INVOKABLE void moveAppletsBasedOnJustifyAlignment();
    Q_INVOKABLE void joinLayoutsToMainLayout();
    Q_INVOKABLE void insertBefore(QQuickItem *hoveredItem, QQuickItem *item);
    Q_INVOKABLE void insertAfter(QQuickItem *hoveredItem, QQuickItem *item);
    Q_INVOKABLE void insertAtCoordinates(QQuickItem *item, const int &x, const int &y) override;
    Q_INVOKABLE bool repairAppletContainers();
    Q_INVOKABLE QObject *resolveAppletQuickItem(QObject *applet) const;
    Q_INVOKABLE int appletId(QObject *applet) const;
    Q_INVOKABLE QString appletIcon(QObject *applet) const;
    Q_INVOKABLE QString appletIconPath(QObject *applet) const;

    Q_INVOKABLE int dndSpacerIndex() override;
    Q_INVOKABLE int defaultInsertionIndex() const;

    Q_INVOKABLE bool isMasqueradedIndex(const int &x, const int &y);
    Q_INVOKABLE int masquearadedIndex(const int &x, const int &y);
    Q_INVOKABLE QPoint indexToMasquearadedPoint(const int &index);

    Q_INVOKABLE void requestAppletsOrder(const QList<int> &order) override;
    Q_INVOKABLE void requestAppletsInLockedZoom(const QList<int> &applets) override;
    Q_INVOKABLE void requestAppletsDisabledColoring(const QList<int> &applets) override;
    Q_INVOKABLE void setAppletInScheduledDestruction(const int &id, const bool &enabled) override;

    //! One-shot insertion protocol; see LayoutManagerApi for the lifetime rule.
    //! The negative expect overload clears an unconsumed request.
    void expectAppletInsertionAt(const int index) override;
    int takeAppletInsertionIndex() override;
    void expectSkipSpacerReorderOnce() override;


Q_SIGNALS:
    void appletOrderChanged();
    void appletsInScheduledDestructionChanged();
    void hasRestoredAppletsChanged();
    void plasmoidChanged();
    void rootItemChanged();
    void dndSpacerChanged();
    void lockedZoomAppletsChanged();
    void userBlocksColorizingAppletsChanged();

    void mainLayoutChanged();
    void metricsChanged();
    void orderChanged();
    void splitterPositionChanged();
    void splitterPosition2Changed();
    void startLayoutChanged();
    void endLayoutChanged();

private Q_SLOTS:
    void onRootItemChanged();
    void destroyJustifySplitters();

    void updateOrder();
    void cleanupOptions();
    void reorderParabolicSpacers();
    void rememberAppletRemovalIndex(const int &id);
    void restoreAppletOrderForApplet(const int &id);
    void forgetAppletRemovalIndex(const int &id);
    void applyTrashKeepOriginalColorsDefault(QObject *applet, const int &id);
    void applyTrashKeepOriginalColorsMigration();
    void writeConfigValue(const QString &key, const QVariant &value);

private:
    //! Const getters are not slots (clazy const-signal-or-slot), so they are Q_INVOKABLE:
    //! the meta-object still lists them, as it listed them when they were private slots,
    //! and QMetaObject::invokeMethod() still finds them by name.
    Q_INVOKABLE bool isTrashWidget(QObject *applet) const;
    Q_INVOKABLE bool usesLegacyJustifySplitters() const;
    Q_INVOKABLE QVariant readConfigValue(const QString &key, const QVariant &defaultValue = QVariant()) const;
    Q_INVOKABLE Plasma::Containment *containmentObject() const;
    Q_INVOKABLE QObject *resolveAppletQuickItemObject(QObject *applet) const;
    Q_INVOKABLE QList<QObject *> plasmoidApplets() const;

    void restoreOptions();
    void restoreOption(const QString &option);
    void saveOption(const char *option);

    void destroyAppletContainer(QObject *applet);

    //! Shared visited-set across recursion frames so cyclic property
    //! graphs (e.g. two QML objects exposing each other as "applet")
    //! cannot recurse forever.
    QObject *resolveAppletQuickItemObjectInternal(QObject *applet, QSet<QObject *> &visited) const;

    void initSaveConnections();

    void insertAtLayoutTail(QQuickItem *layout, QQuickItem *item);
    void insertAtLayoutHead(QQuickItem *layout, QQuickItem *item);
    void insertAtLayoutIndex(QQuickItem *layout, QQuickItem *item, const int &index);

    void setSplitterPosition(const int &position);
    void setSplitterPosition2(const int &position);

    void setAppletOrder(const QList<int> &order);
    void setOrder(const QList<int> &order);
    void setLockedZoomApplets(const QList<int> &applets);
    void setUserBlocksColorizingApplets(const QList<int> &applets);

    bool isJustifySplitter(const QQuickItem *item) const;
    bool insertAtLayoutCoordinates(QQuickItem *layout, QQuickItem *item, int x, int y);
    int configuredAppletCount() const;

    int distanceFromTail(QQuickItem *layout, QPointF pos) const;
    int distanceFromHead(QQuickItem *layout, QPointF pos) const;

    QQuickItem *appletItem(const int &id);
    QQuickItem *appletItemInLayout(QQuickItem *layout, const int &id);

    QList<int> toIntList(const QString &serialized);
    QString toStr(const QList<int> &list);

private:
    void warnRootInvocationFailureOnce(const QMetaMethod &method, const char *functionName);

    //! A layout-child role the manager reads by name (isInternalViewSplitter,
    //! isParabolicEdgeSpacer, backendAppletRef) is declared in the containment QML.
    //! A rename turns every read into QVariant() -> false without a single line of
    //! output, so a broken layout silently keeps working with wrong answers.
    //! Report the absence once per name instead: it costs nothing at run time and
    //! needs no QML root-type change. `applet` is deliberately excluded because
    //! Plasma supplies it on backend applet objects rather than in this package.
    void warnLayoutChildRoleMissingOnce(const QMetaObject *metaObject, const char *propertyName);
    void noteLayoutChildRoles(const QQuickItem *item);

    QSet<QByteArray> m_warnedLayoutChildRoles;

    //! This is needed in order to overcome plasma frameworks limitations and instead of adding dropped widgets
    //! based on coordinates, to be able to add them directly at the correct index
    static const int MASQUERADEDINDEXTOPOINTBASE = -23456;

    int m_splitterPosition{-1};
    int m_splitterPosition2{-1};
    QList<int> m_appletOrder;
    QList<int> m_lockedZoomApplets;
    QList<int> m_order;
    QList<int> m_userBlocksColorizingApplets;
    QHash<int, int> m_appletRemovalIndexes;

    //! The containment QML assigns these items and the plasmoid; Plasma and
    //! the QML engine own them and can destroy them before this manager
    //! during a containment reload or teardown. QPointer makes late timers
    //! and signals see null instead of a dangling object.
    QPointer<QQuickItem> m_rootItem;
    QMetaObject::Connection m_rootDestroyedConnection;
    QPointer<QQuickItem> m_dndSpacer;

    QPointer<QQuickItem> m_mainLayout;
    QPointer<QQuickItem> m_startLayout;
    QPointer<QQuickItem> m_endLayout;
    QPointer<QQuickItem> m_metrics;

    QPointer<QObject> m_plasmoid;
    //! Owned by the plasmoid above; guarded for the same reason.
    QPointer<QQmlPropertyMap> m_configuration;

    QHash<int, QQuickItem *> m_appletsInScheduledDestruction;

    QMetaMethod m_createAppletItemMethod;
    QMetaMethod m_initAppletContainerMethod;
    QSet<QByteArray> m_warnedRootInvocationFailures;
    quint64 m_rootGeneration{0};

    bool m_hasRestoredApplets{false};
    int m_restoreRetryCount{0};
    QTimer m_hasRestoredAppletsTimer;

    //! first QString is the option in AppletItem
    //! second QString is how the option is stored in
    QHash<QString, QString> m_option;

    //! Pending one-shot insertion request. A non-negative value is the index an
    //! applet created by the next addAppletItem() call must use instead of the
    //! index or (x,y) the caller passes; -1 means no request. It is consumed by
    //! the first addAppletItem() dispatch and cleared when the requester drops
    //! it, so a creation that never reports an applet cannot displace a later
    //! applet to a stale position.
    int m_pendingInsertionIndex{-1};

    //! Set while a programmatic separator creation runs. save() must skip
    //! reorderParabolicSpacers() once while it is set and then clear it.
    bool m_skipSpacerReorderOnce{false};
};
}
}

#endif // CONTAINMENTLAYOUTMANAGER_H
