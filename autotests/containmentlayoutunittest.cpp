/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "layoutmanager.h"
#include "../app/view/layoutmanagerapidispatcher.h"

#include <QQuickItem>
#include <QQmlPropertyMap>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTest>

class TestConfig : public QQmlPropertyMap
{
  public:
    TestConfig()
      : QQmlPropertyMap(this, nullptr)
    {
    }
};

// Exercise the real restore/save path without a Plasma shell or user config.
class RestoreRoot : public QQuickItem
{
    Q_OBJECT
  public:
    Q_INVOKABLE QVariant createAppletItem(QVariant applet)
    {
        ++m_appletItemCreationCount;
        auto *item = new QQuickItem(this);
        item->setProperty("backendAppletRef", applet);
        return QVariant::fromValue(item);
    }

    int appletItemCreationCount() const
    {
        return m_appletItemCreationCount;
    }

    Q_INVOKABLE void initAppletContainer(QVariant container, QVariant applet)
    {
        if (auto *item = container.value<QQuickItem *>()) {
            item->setProperty("applet", applet);
        }
    }

private:
    int m_appletItemCreationCount{0};
};

class MissingInitRoot : public QQuickItem
{
    Q_OBJECT
public:
    Q_INVOKABLE QVariant createAppletItem(QVariant applet)
    {
        auto *item = new QQuickItem(this);
        item->setProperty("backendAppletRef", applet);
        return QVariant::fromValue(item);
    }
};

class MissingCreateRoot : public QQuickItem
{
    Q_OBJECT
public:
    Q_INVOKABLE void initAppletContainer(QVariant, QVariant)
    {
    }
};

// Simulates a containment plugin loaded from an installation root that predates
// LayoutManagerApi but still implements the established QObject contract.
class LegacyLayoutManagerObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickItem *rootItem READ rootItem WRITE setRootItem)
    Q_PROPERTY(QQuickItem *dndSpacerItem READ dndSpacerItem WRITE setDndSpacerItem)
    Q_PROPERTY(QList<int> order READ order WRITE setOrder)
    Q_PROPERTY(QList<int> lockedZoomApplets READ lockedZoomApplets WRITE setLockedZoomApplets)
    Q_PROPERTY(QList<int> userBlocksColorizingApplets READ userBlocksColorizingApplets WRITE setUserBlocksColorizingApplets)

public:
    QQuickItem *rootItem() const { return m_rootItem; }
    void setRootItem(QQuickItem *item) { m_rootItem = item; }
    QQuickItem *dndSpacerItem() const { return m_dndSpacerItem; }
    void setDndSpacerItem(QQuickItem *item) { m_dndSpacerItem = item; }
    QList<int> order() const { return m_order; }
    void setOrder(const QList<int> &value) { m_order = value; }
    QList<int> lockedZoomApplets() const { return m_lockedZoomApplets; }
    void setLockedZoomApplets(const QList<int> &value) { m_lockedZoomApplets = value; }
    QList<int> userBlocksColorizingApplets() const { return m_userBlocksColorizingApplets; }
    void setUserBlocksColorizingApplets(const QList<int> &value) { m_userBlocksColorizingApplets = value; }

    Q_INVOKABLE void setOption(int appletId, const QString &property, const QVariant &value)
    {
        lastAppletId = appletId;
        lastOption = property;
        lastOptionValue = value;
        if (property == QStringLiteral("userBlocksColorizing")) {
            m_userBlocksColorizingApplets = value.toBool() ? QList<int>{appletId} : QList<int>{};
        }
    }
    Q_INVOKABLE void addAppletItem(QObject *applet, int index) { lastApplet = applet; lastIndex = index; ++addCount; }
    Q_INVOKABLE void removeAppletItem(QObject *) { ++removeCount; }
    Q_INVOKABLE void hideAppletItem(QObject *) { ++hideCount; }
    Q_INVOKABLE void showAppletItem(QObject *) { ++showCount; }
    Q_INVOKABLE void insertAtCoordinates(QQuickItem *item, int x, int y) { lastItem = item; lastX = x; lastY = y; }
    Q_INVOKABLE int dndSpacerIndex() { return 4; }
    Q_INVOKABLE void requestAppletsOrder(const QList<int> &value) { m_order = value; }
    Q_INVOKABLE void requestAppletsInLockedZoom(const QList<int> &value) { m_lockedZoomApplets = value; }
    Q_INVOKABLE void requestAppletsDisabledColoring(const QList<int> &value) { m_userBlocksColorizingApplets = value; }
    Q_INVOKABLE void setAppletInScheduledDestruction(int id, bool enabled) { lastScheduledId = id; lastScheduled = enabled; }

    int addCount{0};
    int removeCount{0};
    int hideCount{0};
    int showCount{0};
    int lastIndex{-1};
    int lastX{-1};
    int lastY{-1};
    int lastAppletId{-1};
    int lastScheduledId{-1};
    bool lastScheduled{false};
    QString lastOption;
    QVariant lastOptionValue;
    QObject *lastApplet{nullptr};
    QQuickItem *lastItem{nullptr};

private:
    QQuickItem *m_rootItem{nullptr};
    QQuickItem *m_dndSpacerItem{nullptr};
    QList<int> m_order;
    QList<int> m_lockedZoomApplets;
    QList<int> m_userBlocksColorizingApplets;
};

class ContainmentLayoutUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void tracksRequestedAppletOptionLists();
    void independentCopyWaitsForLateApplet_data();
    void independentCopyWaitsForLateApplet();
    void tracksScheduledDestructionIds();
    void restoresScheduledDestructionAppletAtOriginalIndex();
    void roundTripsMasqueradedIndexes();
    void scheduledDestructionHidesAndShowsAppletItem();
    void scheduledDestructionToleratesMissingLayouts();
    void repairSkipsAppletScheduledForDestruction();
    void resolveAppletQuickItemStopsAtCyclicPropertyGraph();
    void resolveAppletQuickItemFindsQuickItemThroughPropertyChain();
    void restoresAndSavesLegacyJustifyFromPersistedMap();
    void joinsLegacyJustifyLayoutsThroughProductionEntryPoint();
    void dndSpacerIndexUsesMainLayoutAndSkipsParabolicSpacers();
    void insertAtCoordinatesUsesMainLayoutForJustify();
    void requestAppletsOrderMaintainsSideAppletOrderInJustify();
    void addAppletItemByIndexUsesMainLayoutInJustify();
    void warnsWhenContainmentRootMethodsAreMissing();
    void restoreWarnsWhenCreateAppletItemIsMissing();
    void warnsOnceForMissingInitMethodDuringRepair();
    void rootDestructionClearsMethodsAndReplacementRebinds();
    void managerDestructionDisconnectsChildRootCallback();
    void dispatcherUsesLegacyQObjectContractAndTracksLifetime();
    void dispatcherPrefersTypedInterface();
};

void
ContainmentLayoutUnitTest::dispatcherUsesLegacyQObjectContractAndTracksLifetime()
{
    Latte::ViewPart::LayoutManagerApiDispatcher dispatcher;
    auto *legacy = new LegacyLayoutManagerObject;
    QQuickItem root;
    QQuickItem spacer;
    legacy->setRootItem(&root);
    legacy->setDndSpacerItem(&spacer);
    legacy->setOrder({31, 37});
    legacy->setLockedZoomApplets({31});
    legacy->setUserBlocksColorizingApplets({37});

    QVERIFY(!qobject_cast<Latte::Containment::LayoutManagerApi *>(legacy));
    dispatcher.setObject(legacy);
    QCOMPARE(dispatcher.rootItem(), &root);
    QCOMPARE(dispatcher.dndSpacer(), &spacer);
    QCOMPARE(dispatcher.order(), QList<int>({31, 37}));
    QCOMPARE(dispatcher.lockedZoomApplets(), QList<int>({31}));
    QCOMPARE(dispatcher.userBlocksColorizingApplets(), QList<int>({37}));

    QObject applet;
    dispatcher.addAppletItem(&applet, 2);
    QCOMPARE(legacy->addCount, 1);
    QCOMPARE(legacy->lastApplet, &applet);
    QCOMPARE(legacy->lastIndex, 2);
    dispatcher.setOption(37, QStringLiteral("userBlocksColorizing"), false);
    QCOMPARE(legacy->lastAppletId, 37);
    QCOMPARE(legacy->userBlocksColorizingApplets(), QList<int>{});
    dispatcher.requestAppletsOrder({41, 43});
    QCOMPARE(dispatcher.order(), QList<int>({41, 43}));
    dispatcher.requestAppletsInLockedZoom({41});
    QCOMPARE(dispatcher.lockedZoomApplets(), QList<int>({41}));
    dispatcher.requestAppletsDisabledColoring({43});
    QCOMPARE(dispatcher.userBlocksColorizingApplets(), QList<int>({43}));
    dispatcher.insertAtCoordinates(&spacer, 8, 13);
    QCOMPARE(legacy->lastItem, &spacer);
    QCOMPARE(legacy->lastX, 8);
    QCOMPARE(legacy->lastY, 13);
    QCOMPARE(dispatcher.dndSpacerIndex(), 4);
    dispatcher.hideAppletItem(&applet);
    dispatcher.showAppletItem(&applet);
    dispatcher.removeAppletItem(&applet);
    dispatcher.setAppletInScheduledDestruction(43, true);
    QCOMPARE(legacy->hideCount, 1);
    QCOMPARE(legacy->showCount, 1);
    QCOMPARE(legacy->removeCount, 1);
    QCOMPARE(legacy->lastScheduledId, 43);
    QVERIFY(legacy->lastScheduled);

    delete legacy;
    QVERIFY(!dispatcher.object());
    QVERIFY(!dispatcher.rootItem());
    QVERIFY(dispatcher.order().isEmpty());
    LegacyLayoutManagerObject replacementObject;
    dispatcher.setObject(&replacementObject);
    QCOMPARE(dispatcher.order(), QList<int>{});
    replacementObject.setOrder({53});
    QCOMPARE(dispatcher.order(), QList<int>{53});
}

void
ContainmentLayoutUnitTest::dispatcherPrefersTypedInterface()
{
    Latte::Containment::LayoutManager manager;
    Latte::ViewPart::LayoutManagerApiDispatcher dispatcher;
    dispatcher.setObject(&manager);

    QVERIFY(qobject_cast<Latte::Containment::LayoutManagerApi *>(&manager));
    dispatcher.requestAppletsOrder({59, 61});
    QCOMPARE(dispatcher.order(), QList<int>({59, 61}));
}

void
ContainmentLayoutUnitTest::warnsWhenContainmentRootMethodsAreMissing()
{
    QQuickItem root;
    Latte::Containment::LayoutManager manager;

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral(".*missing required function createAppletItem\\(QVariant\\).*")));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral(".*missing required function initAppletContainer\\(QVariant,QVariant\\).*")));
    manager.setRootItem(&root);
}

void
ContainmentLayoutUnitTest::warnsOnceForMissingInitMethodDuringRepair()
{
    TestConfig config;
    QObject plasmoid;
    plasmoid.setProperty("configuration", QVariant::fromValue(static_cast<QObject *>(&config)));

    QQuickItem applet;
    applet.setProperty("id", 47);
    applet.setProperty("_plasma_graphicObject", QVariant::fromValue(static_cast<QObject *>(&applet)));
    plasmoid.setProperty("applets", QVariant::fromValue(QList<QObject *>{ &applet }));

    MissingInitRoot root;
    QQuickItem start;
    QQuickItem main;
    QQuickItem end;
    Latte::Containment::LayoutManager manager;
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral(".*missing required function initAppletContainer\\(QVariant,QVariant\\).*")));
    manager.setRootItem(&root);
    manager.setStartLayout(&start);
    manager.setMainLayout(&main);
    manager.setEndLayout(&end);
    manager.setPlasmoid(&plasmoid);
    QVERIFY(manager.repairAppletContainers());
    QVERIFY(manager.repairAppletContainers());
}

void
ContainmentLayoutUnitTest::restoreWarnsWhenCreateAppletItemIsMissing()
{
    TestConfig config;
    config.insert(QStringLiteral("appletOrder"), QStringLiteral("44"));
    QObject plasmoid;
    plasmoid.setProperty("configuration", QVariant::fromValue(static_cast<QObject *>(&config)));

    QQuickItem applet;
    applet.setProperty("id", 44);
    applet.setProperty("_plasma_graphicObject", QVariant::fromValue(static_cast<QObject *>(&applet)));
    plasmoid.setProperty("applets", QVariant::fromValue(QList<QObject *>{ &applet }));

    MissingCreateRoot root;
    QQuickItem start;
    QQuickItem main;
    QQuickItem end;
    Latte::Containment::LayoutManager manager;
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral(".*missing required function createAppletItem\\(QVariant\\).*")));
    manager.setRootItem(&root);
    manager.setStartLayout(&start);
    manager.setMainLayout(&main);
    manager.setEndLayout(&end);
    manager.setPlasmoid(&plasmoid);

    manager.restore();

    QVERIFY(!manager.hasRestoredApplets());
    QCOMPARE(config.value(QStringLiteral("appletOrder")).toString(), QStringLiteral("44"));
    QVERIFY(main.childItems().isEmpty());
}

void
ContainmentLayoutUnitTest::rootDestructionClearsMethodsAndReplacementRebinds()
{
    TestConfig config;
    config.insert(QStringLiteral("appletOrder"), QStringLiteral("44"));
    QObject plasmoid;
    plasmoid.setProperty("configuration", QVariant::fromValue(static_cast<QObject *>(&config)));

    QQuickItem applet;
    applet.setProperty("id", 44);
    applet.setProperty("_plasma_graphicObject", QVariant::fromValue(static_cast<QObject *>(&applet)));
    plasmoid.setProperty("applets", QVariant::fromValue(QList<QObject *>{ &applet }));

    auto *root = new RestoreRoot;
    QQuickItem start;
    QQuickItem main;
    QQuickItem end;
    Latte::Containment::LayoutManager manager;
    manager.setRootItem(root);
    manager.setStartLayout(&start);
    manager.setMainLayout(&main);
    manager.setEndLayout(&end);
    manager.setPlasmoid(&plasmoid);
    QSignalSpy rootChangedSpy(&manager, &Latte::Containment::LayoutManager::rootItemChanged);
    delete root;
    QVERIFY(manager.rootItem() == nullptr);
    QCOMPARE(rootChangedSpy.count(), 1);

    RestoreRoot replacement;
    manager.setRootItem(&replacement);
    manager.restore();
    QCOMPARE(replacement.appletItemCreationCount(), 1);
}

void
ContainmentLayoutUnitTest::managerDestructionDisconnectsChildRootCallback()
{
    auto *manager = new Latte::Containment::LayoutManager;
    auto *root = new RestoreRoot;
    root->setParent(manager);
    manager->setRootItem(root);
    QSignalSpy destroyedSpy(root, &QObject::destroyed);

    delete manager;

    QCOMPARE(destroyedSpy.count(), 1);
}

void
ContainmentLayoutUnitTest::independentCopyWaitsForLateApplet_data()
{
    QTest::addColumn<bool>("originalColors");
    QTest::newRow("copied-trash-original-colors") << true;
    QTest::newRow("copied-trash-explicit-colorization") << false;
}

void
ContainmentLayoutUnitTest::independentCopyWaitsForLateApplet()
{
    QFETCH(bool, originalColors);
    const QString savedColors = originalColors ? QStringLiteral("45") : QString();
    TestConfig config;
    config.insert(QStringLiteral("appletOrder"), QStringLiteral("44;46;45"));
    config.insert(QStringLiteral("lockedZoomApplets"), QStringLiteral("45"));
    config.insert(QStringLiteral("userBlocksColorizingApplets"), savedColors);
    config.insert(QStringLiteral("trashKeepOriginalColorsDefaulted"), true);
    QObject plasmoid;
    plasmoid.setProperty("configuration", QVariant::fromValue(static_cast<QObject *>(&config)));
    QQuickItem launcher;
    launcher.setProperty("id", 44);
    launcher.setProperty("_plasma_graphicObject", QVariant::fromValue(static_cast<QObject *>(&launcher)));
    QQuickItem tasks;
    tasks.setProperty("id", 46);
    tasks.setProperty("_plasma_graphicObject", QVariant::fromValue(static_cast<QObject *>(&tasks)));
    QQuickItem trash;
    trash.setProperty("id", 45);
    trash.setProperty("_plasma_graphicObject", QVariant::fromValue(static_cast<QObject *>(&trash)));
    plasmoid.setProperty("applets", QVariant::fromValue(QList<QObject *>{ &launcher, &tasks }));
    RestoreRoot root;
    QQuickItem start;
    QQuickItem main;
    QQuickItem end;
    Latte::Containment::LayoutManager manager;
    manager.setRootItem(&root);
    manager.setStartLayout(&start);
    manager.setMainLayout(&main);
    manager.setEndLayout(&end);
    manager.setPlasmoid(&plasmoid);
    manager.restore();
    QCOMPARE(config.value(QStringLiteral("appletOrder")).toString(), QStringLiteral("44;46;45"));
    QCOMPARE(config.value(QStringLiteral("userBlocksColorizingApplets")).toString(), savedColors);
    QVERIFY(!manager.hasRestoredApplets());

    plasmoid.setProperty("applets", QVariant::fromValue(QList<QObject *>{ &launcher, &tasks, &trash }));
    manager.restore();
    QCOMPARE(manager.appletOrder(), (QList<int>{ 44, 46, 45 }));
    QCOMPARE(manager.lockedZoomApplets(), (QList<int>{ 45 }));
    QCOMPARE(manager.userBlocksColorizingApplets(), originalColors ? QList<int>{ 45 } : QList<int>());
    manager.saveOptions();
    QCOMPARE(config.value(QStringLiteral("lockedZoomApplets")).toString(), QStringLiteral("45"));
    QCOMPARE(config.value(QStringLiteral("userBlocksColorizingApplets")).toString(), savedColors);
}

void ContainmentLayoutUnitTest::tracksRequestedAppletOptionLists()
{
    Latte::Containment::LayoutManager manager;
    QSignalSpy lockedSpy(&manager, &Latte::Containment::LayoutManager::lockedZoomAppletsChanged);
    QSignalSpy coloringSpy(&manager, &Latte::Containment::LayoutManager::userBlocksColorizingAppletsChanged);

    manager.requestAppletsInLockedZoom(QList<int> {3, 5});
    manager.requestAppletsInLockedZoom(QList<int> {3, 5});
    QCOMPARE(manager.lockedZoomApplets(), (QList<int> {3, 5}));
    QCOMPARE(lockedSpy.count(), 1);

    manager.requestAppletsDisabledColoring(QList<int> {7});
    manager.requestAppletsDisabledColoring(QList<int> {7});
    QCOMPARE(manager.userBlocksColorizingApplets(), (QList<int> {7}));
    QCOMPARE(coloringSpy.count(), 1);
}

void ContainmentLayoutUnitTest::tracksScheduledDestructionIds()
{
    Latte::Containment::LayoutManager manager;
    QSignalSpy scheduledSpy(&manager, &Latte::Containment::LayoutManager::appletsInScheduledDestructionChanged);

    manager.setAppletInScheduledDestruction(9, true);
    manager.setAppletInScheduledDestruction(9, true);
    QVERIFY(manager.appletsInScheduledDestruction().contains(9));
    QCOMPARE(scheduledSpy.count(), 1);

    manager.setAppletInScheduledDestruction(9, false);
    manager.setAppletInScheduledDestruction(9, false);
    QVERIFY(!manager.appletsInScheduledDestruction().contains(9));
    QCOMPARE(scheduledSpy.count(), 2);
}

void ContainmentLayoutUnitTest::restoresScheduledDestructionAppletAtOriginalIndex()
{
    Latte::Containment::LayoutManager manager;

    manager.requestAppletsOrder(QList<int> {1, 2, 3, 4});

    QVERIFY(QMetaObject::invokeMethod(&manager,
                                      "rememberAppletRemovalIndex",
                                      Qt::DirectConnection,
                                      Q_ARG(int, 2)));

    manager.requestAppletsOrder(QList<int> {1, 3, 4});

    QVERIFY(QMetaObject::invokeMethod(&manager,
                                      "restoreAppletOrderForApplet",
                                      Qt::DirectConnection,
                                      Q_ARG(int, 2)));

    QCOMPARE(manager.appletOrder(), (QList<int> {1, 2, 3, 4}));
}

void ContainmentLayoutUnitTest::roundTripsMasqueradedIndexes()
{
    Latte::Containment::LayoutManager manager;

    const QPoint encoded = manager.indexToMasquearadedPoint(12);
    QVERIFY(manager.isMasqueradedIndex(encoded.x(), encoded.y()));
    QCOMPARE(manager.masquearadedIndex(encoded.x(), encoded.y()), 12);
    QVERIFY(!manager.isMasqueradedIndex(encoded.x(), encoded.y() + 1));
}

void ContainmentLayoutUnitTest::scheduledDestructionHidesAndShowsAppletItem()
{
    Latte::Containment::LayoutManager manager;

    // Stack-allocated layout and applet item — QQuickItem can be
    // stack-allocated when no QObject parent is needed.
    QQuickItem mainLayout;
    manager.setMainLayout(&mainLayout);

    // Mock backend object: a plain QObject whose "id" property is the
    // target applet id (appletId() reads candidate->property("id")).
    constexpr int testId{42};
    QObject backend;
    backend.setProperty("id", testId);

    // Applet item — the item whose visibility we want to verify.
    // The visual parent (mainLayout) is also the QObject parent, so
    // appletItem appears in mainLayout.childItems().
    QQuickItem appletItem(&mainLayout);
    appletItem.setProperty("applet", QVariant::fromValue(static_cast<QObject *>(&backend)));
    appletItem.setVisible(true);
    QVERIFY(appletItem.isVisible());

    QSignalSpy scheduledSpy(&manager, &Latte::Containment::LayoutManager::appletsInScheduledDestructionChanged);

    // Schedule destruction — the item should be hidden immediately.
    manager.setAppletInScheduledDestruction(testId, true);
    QVERIFY(manager.appletsInScheduledDestruction().contains(testId));
    QVERIFY(!appletItem.isVisible());
    QCOMPARE(scheduledSpy.count(), 1);

    // Redundant enable — no change, no extra signal.
    manager.setAppletInScheduledDestruction(testId, true);
    QVERIFY(manager.appletsInScheduledDestruction().contains(testId));
    QCOMPARE(scheduledSpy.count(), 1);

    // Undo — the item should become visible again.
    manager.setAppletInScheduledDestruction(testId, false);
    QVERIFY(!manager.appletsInScheduledDestruction().contains(testId));
    QVERIFY(appletItem.isVisible());
    QCOMPARE(scheduledSpy.count(), 2);

    // Redundant disable — no change, no extra signal.
    manager.setAppletInScheduledDestruction(testId, false);
    QVERIFY(!manager.appletsInScheduledDestruction().contains(testId));
    QCOMPARE(scheduledSpy.count(), 2);
}

void ContainmentLayoutUnitTest::scheduledDestructionToleratesMissingLayouts()
{
    Latte::Containment::LayoutManager manager;
    QSignalSpy scheduledSpy(&manager, &Latte::Containment::LayoutManager::appletsInScheduledDestructionChanged);

    // No layouts set — appletItem() returns nullptr.
    // The function must not crash and must still track IDs correctly.
    manager.setAppletInScheduledDestruction(7, true);
    QVERIFY(manager.appletsInScheduledDestruction().contains(7));
    QCOMPARE(scheduledSpy.count(), 1);

    manager.setAppletInScheduledDestruction(7, false);
    QVERIFY(!manager.appletsInScheduledDestruction().contains(7));
    QCOMPARE(scheduledSpy.count(), 2);
}

void ContainmentLayoutUnitTest::repairSkipsAppletScheduledForDestruction()
{
    TestConfig config;
    QObject plasmoid;
    plasmoid.setProperty("configuration", QVariant::fromValue(static_cast<QObject *>(&config)));

    QQuickItem applet;
    applet.setProperty("id", 47);
    applet.setProperty("_plasma_graphicObject", QVariant::fromValue(static_cast<QObject *>(&applet)));
    plasmoid.setProperty("applets", QVariant::fromValue(QList<QObject *>{ &applet }));

    RestoreRoot root;
    QQuickItem start;
    QQuickItem main;
    QQuickItem end;
    Latte::Containment::LayoutManager manager;
    manager.setRootItem(&root);
    manager.setStartLayout(&start);
    manager.setMainLayout(&main);
    manager.setEndLayout(&end);
    manager.setPlasmoid(&plasmoid);

    manager.setAppletInScheduledDestruction(47, true);
    QCOMPARE(root.appletItemCreationCount(), 0);

    const bool didRepair = manager.repairAppletContainers();
    QCOMPARE(root.appletItemCreationCount(), 0);
    QVERIFY(!didRepair);
    QVERIFY(main.childItems().isEmpty());
}

void ContainmentLayoutUnitTest::resolveAppletQuickItemStopsAtCyclicPropertyGraph()
{
    QObject first;
    QObject second;
    first.setProperty("applet", QVariant::fromValue(static_cast<QObject *>(&second)));
    second.setProperty("applet", QVariant::fromValue(static_cast<QObject *>(&first)));

    Latte::Containment::LayoutManager manager;
    QVERIFY(!manager.resolveAppletQuickItem(&first));
}

void ContainmentLayoutUnitTest::resolveAppletQuickItemFindsQuickItemThroughPropertyChain()
{
    QObject appletWrapper;
    QObject backendApplet;
    QQuickItem quickItem;
    appletWrapper.setProperty("applet", QVariant::fromValue(static_cast<QObject *>(&backendApplet)));
    backendApplet.setProperty("item", QVariant::fromValue(static_cast<QObject *>(&quickItem)));

    Latte::Containment::LayoutManager manager;
    QCOMPARE(manager.resolveAppletQuickItem(&appletWrapper), static_cast<QObject *>(&quickItem));
}

void ContainmentLayoutUnitTest::restoresAndSavesLegacyJustifyFromPersistedMap()
{
    // This persisted-map fixture exercises production restore/save logic, but
    // deliberately does not claim to cover the Plasma-to-KConfig disk reader.
    TestConfig configuration;
    configuration.insert(QStringLiteral("alignment"), QStringLiteral("Justify"));
    configuration.insert(QStringLiteral("appletOrder"), QStringLiteral("44;-1;46;-1;45"));
    configuration.insert(QStringLiteral("splitterPosition"), 2);
    configuration.insert(QStringLiteral("splitterPosition2"), 4);

    QObject plasmoid;
    plasmoid.setProperty("configuration", QVariant::fromValue(static_cast<QObject *>(&configuration)));

    QQuickItem launcher;
    launcher.setProperty("id", 44);
    launcher.setProperty("_plasma_graphicObject", QVariant::fromValue(static_cast<QObject *>(&launcher)));
    QQuickItem tasks;
    tasks.setProperty("id", 46);
    tasks.setProperty("_plasma_graphicObject", QVariant::fromValue(static_cast<QObject *>(&tasks)));
    QQuickItem trash;
    trash.setProperty("id", 45);
    trash.setProperty("_plasma_graphicObject", QVariant::fromValue(static_cast<QObject *>(&trash)));
    plasmoid.setProperty("applets", QVariant::fromValue(QList<QObject *>{ &launcher, &tasks, &trash }));

    RestoreRoot root;
    QQuickItem start;
    QQuickItem main;
    QQuickItem end;
    Latte::Containment::LayoutManager manager;
    manager.setRootItem(&root);
    manager.setStartLayout(&start);
    manager.setMainLayout(&main);
    manager.setEndLayout(&end);
    manager.setPlasmoid(&plasmoid);

    manager.restore();

    QVERIFY(start.childItems().isEmpty());
    QCOMPARE(main.childItems().count(), 3);
    QVERIFY(end.childItems().isEmpty());
    QCOMPARE(manager.appletOrder(), (QList<int>{ 44, 46, 45 }));
    QCOMPARE(manager.order(), (QList<int>{ 44, 46, 45 }));
    QCOMPARE(manager.splitterPosition(), 2);
    QCOMPARE(manager.splitterPosition2(), 4);
    QCOMPARE(configuration.value(QStringLiteral("appletOrder")).toString(), QStringLiteral("44;46;45"));

    QList<int> restoredAppletIds;
    for (QQuickItem *item : main.childItems()) {
        restoredAppletIds << item->property("backendAppletRef").value<QObject *>()->property("id").toInt();
    }
    QCOMPARE(restoredAppletIds, (QList<int>{ 44, 46, 45 }));

    manager.requestAppletsOrder(QList<int>{ 45, 44, 46 });
    manager.save();

    QCOMPARE(configuration.value(QStringLiteral("appletOrder")).toString(), QStringLiteral("45;44;46"));
    QCOMPARE(configuration.value(QStringLiteral("splitterPosition")).toInt(), 2);
    QCOMPARE(configuration.value(QStringLiteral("splitterPosition2")).toInt(), 4);
}

void ContainmentLayoutUnitTest::joinsLegacyJustifyLayoutsThroughProductionEntryPoint()
{
    TestConfig config;
    config.insert(QStringLiteral("alignment"), QStringLiteral("Justify"));
    QObject plasmoid;
    plasmoid.setProperty("configuration", QVariant::fromValue(static_cast<QObject *>(&config)));

    RestoreRoot root;
    QQuickItem start;
    QQuickItem main;
    QQuickItem end;
    QQuickItem startApplet;
    QQuickItem mainApplet;
    QQuickItem endApplet;
    startApplet.setParentItem(&start);
    mainApplet.setParentItem(&main);
    endApplet.setParentItem(&end);

    Latte::Containment::LayoutManager manager;
    manager.setRootItem(&root);
    manager.setStartLayout(&start);
    manager.setMainLayout(&main);
    manager.setEndLayout(&end);
    manager.setPlasmoid(&plasmoid);
    manager.addJustifySplittersInMainLayout();

    QVERIFY(start.childItems().isEmpty());
    QVERIFY(end.childItems().isEmpty());
    QCOMPARE(main.childItems(), (QList<QQuickItem *>{ &startApplet, &mainApplet, &endApplet }));
}

void ContainmentLayoutUnitTest::dndSpacerIndexUsesMainLayoutAndSkipsParabolicSpacers()
{
    QQuickItem start;
    QQuickItem main;
    QQuickItem end;
    QQuickItem startApplet;
    QQuickItem firstApplet;
    QQuickItem parabolicSpacer;
    QQuickItem spacer;
    QQuickItem secondApplet;
    startApplet.setParentItem(&start);
    firstApplet.setParentItem(&main);
    parabolicSpacer.setParentItem(&main);
    spacer.setParentItem(&main);
    secondApplet.setParentItem(&main);
    parabolicSpacer.setProperty("isParabolicEdgeSpacer", true);

    Latte::Containment::LayoutManager manager;
    manager.setStartLayout(&start);
    manager.setMainLayout(&main);
    manager.setEndLayout(&end);
    manager.setDndSpacer(&spacer);

    QCOMPARE(manager.dndSpacerIndex(), 1);

    spacer.setParentItem(&start);
    QCOMPARE(manager.dndSpacerIndex(), -1);
}

void ContainmentLayoutUnitTest::insertAtCoordinatesUsesMainLayoutForJustify()
{
    TestConfig configuration;
    configuration.insert(QStringLiteral("alignment"), QStringLiteral("Justify"));
    QObject plasmoid;
    plasmoid.setProperty("formFactor", 0);
    plasmoid.setProperty("configuration", QVariant::fromValue(static_cast<QObject *>(&configuration)));

    QQuickItem root;
    QQuickItem start;
    QQuickItem main;
    QQuickItem end;
    root.setWidth(300);
    root.setHeight(50);
    start.setParentItem(&root);
    start.setWidth(100);
    start.setHeight(50);
    main.setParentItem(&root);
    main.setX(100);
    main.setWidth(100);
    main.setHeight(50);
    end.setParentItem(&root);
    end.setX(200);
    end.setWidth(100);
    end.setHeight(50);

    QQuickItem existingApplet;
    existingApplet.setWidth(30);
    existingApplet.setHeight(50);
    existingApplet.setParentItem(&main);
    QQuickItem draggedItem;

    Latte::Containment::LayoutManager manager;
    manager.setRootItem(&root);
    manager.setStartLayout(&start);
    manager.setMainLayout(&main);
    manager.setEndLayout(&end);
    manager.setPlasmoid(&plasmoid);

    manager.insertAtCoordinates(&draggedItem, 105, 25);

    QCOMPARE(main.childItems(), (QList<QQuickItem *>{ &draggedItem, &existingApplet }));
    QVERIFY(start.childItems().isEmpty());
    QVERIFY(end.childItems().isEmpty());

    QQuickItem fallbackHeadItem;
    manager.insertAtCoordinates(&fallbackHeadItem, 205, 25);

    QCOMPARE(main.childItems().last(), &fallbackHeadItem);
    QVERIFY(end.childItems().isEmpty());
}

void ContainmentLayoutUnitTest::requestAppletsOrderMaintainsSideAppletOrderInJustify()
{
    TestConfig configuration;
    configuration.insert(QStringLiteral("alignment"), QStringLiteral("Justify"));
    QObject plasmoid;
    plasmoid.setProperty("configuration", QVariant::fromValue(static_cast<QObject *>(&configuration)));

    QObject firstBackend;
    firstBackend.setProperty("id", 1);
    QObject mainBackend;
    mainBackend.setProperty("id", 2);
    QObject lastBackend;
    lastBackend.setProperty("id", 3);

    QQuickItem root;
    QQuickItem start;
    QQuickItem main;
    QQuickItem end;
    QQuickItem first;
    first.setProperty("backendAppletRef", QVariant::fromValue(static_cast<QObject *>(&firstBackend)));
    first.setParentItem(&start);
    QQuickItem middle;
    middle.setProperty("backendAppletRef", QVariant::fromValue(static_cast<QObject *>(&mainBackend)));
    middle.setParentItem(&main);
    QQuickItem last;
    last.setProperty("backendAppletRef", QVariant::fromValue(static_cast<QObject *>(&lastBackend)));
    last.setParentItem(&end);

    Latte::Containment::LayoutManager manager;
    manager.setRootItem(&root);
    manager.setStartLayout(&start);
    manager.setMainLayout(&main);
    manager.setEndLayout(&end);
    manager.setPlasmoid(&plasmoid);

    manager.requestAppletsOrder(QList<int>{ 3, 1, 2 });

    QVERIFY(start.childItems().isEmpty());
    QVERIFY(end.childItems().isEmpty());
    QCOMPARE(main.childItems(), (QList<QQuickItem *>{ &last, &first, &middle }));
    QCOMPARE(configuration.value(QStringLiteral("appletOrder")).toString(), QStringLiteral("3;1;2"));
}

void ContainmentLayoutUnitTest::addAppletItemByIndexUsesMainLayoutInJustify()
{
    TestConfig configuration;
    configuration.insert(QStringLiteral("alignment"), QStringLiteral("Justify"));
    QObject plasmoid;
    plasmoid.setProperty("configuration", QVariant::fromValue(static_cast<QObject *>(&configuration)));

    QObject firstBackend;
    firstBackend.setProperty("id", 10);
    QObject lastBackend;
    lastBackend.setProperty("id", 20);
    QObject newBackend;
    newBackend.setProperty("id", 15);

    RestoreRoot root;
    QQuickItem start;
    QQuickItem main;
    QQuickItem end;
    QQuickItem first;
    first.setProperty("backendAppletRef", QVariant::fromValue(static_cast<QObject *>(&firstBackend)));
    first.setParentItem(&main);
    QQuickItem last;
    last.setProperty("backendAppletRef", QVariant::fromValue(static_cast<QObject *>(&lastBackend)));
    last.setParentItem(&main);

    Latte::Containment::LayoutManager manager;
    manager.setRootItem(&root);
    manager.setStartLayout(&start);
    manager.setMainLayout(&main);
    manager.setEndLayout(&end);
    manager.setPlasmoid(&plasmoid);
    manager.requestAppletsOrder(QList<int>{ 10, 20 });

    manager.addAppletItem(&newBackend, 1);

    QCOMPARE(main.childItems().count(), 3);
    QCOMPARE(main.childItems().at(0), &first);
    QCOMPARE(main.childItems().at(2), &last);
    QCOMPARE(main.childItems().at(1)->property("backendAppletRef").value<QObject *>(), static_cast<QObject *>(&newBackend));
    QCOMPARE(configuration.value(QStringLiteral("appletOrder")).toString(), QStringLiteral("10;15;20"));
}



QTEST_MAIN(ContainmentLayoutUnitTest)

#include "containmentlayoutunittest.moc"
