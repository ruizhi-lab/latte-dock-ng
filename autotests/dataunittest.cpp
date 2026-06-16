/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "activitydata.h"
#include "apptypes.h"
#include "appletdata.h"
#include "errordata.h"
#include "errorinformationdata.h"
#include "genericbasictable.h"
#include "genericdata.h"
#include "layoutdata.h"
#include "layouticondata.h"
#include "layoutcolordata.h"
#include "layoutstable.h"
#include "preferencesdata.h"
#include "pluginids.h"
#include "screendata.h"
#include "uniqueidinfo.h"
#include "viewdata.h"
#include "viewstable.h"
#include "commontools.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTest>

using Latte::Data::Activity;
using Latte::Data::Applet;
using Latte::Data::Error;
using Latte::Data::ErrorInformation;
using Latte::Data::Generic;
using Latte::Data::GenericBasicTable;
using Latte::Data::Layout;
using Latte::Data::LayoutColor;
using Latte::Data::LayoutIcon;
using Latte::Data::LayoutsTable;
using Latte::Data::Preferences;
using Latte::Data::Screen;
using Latte::Data::UniqueIdInfo;
using Latte::Data::View;
using Latte::Data::ViewsTable;

class DataUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void activityCopiesAndAssignsAllFields();
    void activityStatePredicatesReflectState();
    void appTypesMatchKnownWaylandApplicationIds();
    void appletDefaultConstructsUnselected();
    void appletValidityAndVisibleNameFollowIdAndName();
    void appletEqualityComparesAllFields();
    void commonToolsSerializeRectAndStripSuffix();
    void commonToolsCalculateColorBrightnessAndLumina();
    void commonToolsFindStandardAndConfigPaths();
    void errorInformationValidityFollowsNestedApplets();
    void errorValidityAndEqualityIncludeInformation();
    void genericBasicTableOrdersAndFindsRecords();
    void genericBasicTableRemovesAndClearsRecords();
    void genericDefaultConstructsEmptyValues();
    void genericCopiesAndAssignsAllFields();
    void genericEqualityComparesIdAndName();
    void layoutActivityPredicatesReflectActivitySet();
    void layoutEqualityIgnoresRuntimeState();
    void layoutTemplatePredicatesRespectUserAndTempPaths();
    void layoutColorSetDataPopulatesAllFields();
    void layoutIconDetectsEmptyAndComparesFields();
    void layoutsTableSubtractsAndMarksFreeActivities();
    void preferencesDefaultsRoundTrip();
    void preferencesEqualityComparesAllFields();
    void pluginIdClassifiesLatteBuiltInsAndSeparators();
    void screenInitializesFromSerializedGeometry();
    void screenDetectsScreenGroups();
    void screenEqualityIgnoresRuntimeState();
    void uniqueIdInfoDefaultConstructsEmptyValues();
    void uniqueIdInfoCopiesAndAssignsAllFields();
    void uniqueIdInfoEqualityComparesAllFields();
    void viewStatePredicatesReflectOriginAndCloneState();
    void viewEqualityIgnoresRuntimeState();
    void viewStringShowsCombinedMoveOriginAndDestination();
    void viewsTableFindsSubcontainmentsAndTemporaryViews();
    void viewsTableSubtractsAndFiltersOriginalViews();
};

void DataUnitTest::activityCopiesAndAssignsAllFields()
{
    Activity source;
    source.id = QStringLiteral("activity-id");
    source.name = QStringLiteral("Activity Name");
    source.icon = QStringLiteral("activity-icon");
    source.isCurrent = true;
    source.state = Activity::Running;

    const Activity copied(source);
    QCOMPARE(copied.id, source.id);
    QCOMPARE(copied.name, source.name);
    QCOMPARE(copied.icon, source.icon);
    QCOMPARE(copied.isCurrent, source.isCurrent);
    QCOMPARE(copied.state, source.state);

    Activity assigned;
    assigned = source;
    QCOMPARE(assigned.id, source.id);
    QCOMPARE(assigned.name, source.name);
    QCOMPARE(assigned.icon, source.icon);
    QCOMPARE(assigned.isCurrent, source.isCurrent);
    QCOMPARE(assigned.state, source.state);
}

void DataUnitTest::activityStatePredicatesReflectState()
{
    Activity activity;

    activity.state = Activity::Invalid;
    QVERIFY(!activity.isValid());
    QVERIFY(!activity.isRunning());

    activity.state = Activity::Stopped;
    QVERIFY(activity.isValid());
    QVERIFY(!activity.isRunning());

    activity.state = Activity::Starting;
    QVERIFY(activity.isRunning());

    activity.state = Activity::Running;
    QVERIFY(activity.isRunning());
}

void DataUnitTest::appTypesMatchKnownWaylandApplicationIds()
{
    QCOMPARE(Latte::App::preferredWaylandAppId(), QStringLiteral("latte-dock-ng"));
    QVERIFY(Latte::App::matchesSelfAppId(QStringLiteral("latte-dock-ng")));
    QVERIFY(Latte::App::matchesSelfAppId(QStringLiteral("latte-dock")));
    QVERIFY(Latte::App::matchesSelfAppId(QStringLiteral("org.kde.latte-dock")));
    QVERIFY(!Latte::App::matchesSelfAppId(QStringLiteral("org.kde.plasmashell")));
}

void DataUnitTest::appletDefaultConstructsUnselected()
{
    const Applet applet;

    QVERIFY(!applet.isSelected);
}

void DataUnitTest::appletValidityAndVisibleNameFollowIdAndName()
{
    Applet applet;
    QVERIFY(!applet.isValid());
    QVERIFY(applet.visibleName().isEmpty());

    applet.id = QStringLiteral("org.kde.latte.tasks");
    QVERIFY(applet.isValid());
    QCOMPARE(applet.visibleName(), applet.id);
    QVERIFY(applet.isInstalled());

    applet.name = applet.id;
    QCOMPARE(applet.visibleName(), applet.name);
    QVERIFY(!applet.isInstalled());

    applet.name = QStringLiteral("Latte Tasks");
    QCOMPARE(applet.visibleName(), applet.name);
    QVERIFY(applet.isInstalled());
}

void DataUnitTest::appletEqualityComparesAllFields()
{
    Applet original;
    original.id = QStringLiteral("org.kde.latte.tasks");
    original.name = QStringLiteral("Latte Tasks");
    original.description = QStringLiteral("Task manager");
    original.icon = QStringLiteral("latte");
    original.isSelected = true;
    original.storageId = QStringLiteral("storage-id");
    original.subcontainmentId = QStringLiteral("subcontainment-id");

    Applet same(original);
    Applet changedStorageId(original);
    changedStorageId.storageId = QStringLiteral("other-storage-id");

    QVERIFY(original == same);
    QVERIFY(!(original != same));
    QVERIFY(original != changedStorageId);
}

void DataUnitTest::commonToolsSerializeRectAndStripSuffix()
{
    const QRect rect(10, 20, 300, 400);

    QCOMPARE(Latte::rectToString(rect), QStringLiteral("10,20 300x400"));
    QCOMPARE(Latte::stringToRect(QStringLiteral("10,20 300x400")), rect);
    QCOMPARE(Latte::stringToRect(Latte::rectToString(QRect(-5, -6, 7, 8))), QRect(-5, -6, 7, 8));
    QCOMPARE(Latte::stripUniqueNameSuffix(QStringLiteral("Layout - 2")), QStringLiteral("Layout"));
    QCOMPARE(Latte::stripUniqueNameSuffix(QStringLiteral("Layout - 002")), QStringLiteral("Layout"));
    QCOMPARE(Latte::stripUniqueNameSuffix(QStringLiteral("Layout - 2 - 3")), QStringLiteral("Layout - 2"));
    QCOMPARE(Latte::stripUniqueNameSuffix(QStringLiteral(" - 2")), QStringLiteral(" - 2"));
    QCOMPARE(Latte::stripUniqueNameSuffix(QStringLiteral("Layout - copy")), QStringLiteral("Layout - copy"));
}

void DataUnitTest::commonToolsCalculateColorBrightnessAndLumina()
{
    QCOMPARE(Latte::colorBrightness(255.0f, 255.0f, 255.0f), 255.0f);
    QCOMPARE(Latte::colorBrightness(0.0f, 0.0f, 0.0f), 0.0f);
    QCOMPARE(Latte::colorLumina(QColor(Qt::black)), 0.0f);
    QCOMPARE(Latte::colorLumina(QColor(Qt::white)), 1.0f);
}

void DataUnitTest::commonToolsFindStandardAndConfigPaths()
{
    QStandardPaths::setTestModeEnabled(true);

    const QString marker = QStringLiteral("latte-dock-ng/autotests/standard-path-marker");
    const QString basePath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QVERIFY(!basePath.isEmpty());
    QVERIFY(QDir().mkpath(QFileInfo(basePath + QLatin1Char('/') + marker).absolutePath()));

    QFile file(basePath + QLatin1Char('/') + marker);
    QVERIFY(file.open(QFile::WriteOnly));
    file.close();

    QCOMPARE(Latte::standardPath(marker), file.fileName());
    QCOMPARE(Latte::standardPath(QStringLiteral("latte-dock-ng/autotests/missing-marker")), QString());

    const QString configPath = Latte::configPath();
    QVERIFY(!configPath.isEmpty());
    QVERIFY(QDir::isAbsolutePath(configPath));
}

void DataUnitTest::errorInformationValidityFollowsNestedApplets()
{
    ErrorInformation info;
    QVERIFY(!info.isValid());

    info.containment.id = QStringLiteral("containment-id");
    QVERIFY(info.isValid());

    ErrorInformation copied(info);
    QVERIFY(info == copied);

    copied.applet.id = QStringLiteral("applet-id");
    QVERIFY(info != copied);
}

void DataUnitTest::errorValidityAndEqualityIncludeInformation()
{
    Error error;
    QVERIFY(!error.isValid());

    error.id = Error::APPLETSWITHSAMEID;
    error.name = QStringLiteral("Duplicate applets");
    QVERIFY(error.isValid());

    Error same(error);
    QVERIFY(error == same);

    ErrorInformation info;
    info.id = QStringLiteral("info-id");
    info.applet.id = QStringLiteral("applet-id");
    error.information << info;
    QVERIFY(error != same);
}

void DataUnitTest::genericBasicTableOrdersAndFindsRecords()
{
    GenericBasicTable table;
    table.insertBasedOnName(Generic(QStringLiteral("2"), QStringLiteral("Beta")));
    table.insertBasedOnName(Generic(QStringLiteral("1"), QStringLiteral("alpha")));
    table.insertBasedOnName(Generic(QStringLiteral("3"), QStringLiteral("gamma")));

    QCOMPARE(table.rowCount(), 3);
    QCOMPARE(table.names(), QStringList({QStringLiteral("alpha"), QStringLiteral("Beta"), QStringLiteral("gamma")}));
    QCOMPARE(table.ids(), QStringList({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3")}));
    QVERIFY(table.containsId(QStringLiteral("2")));
    QVERIFY(table.containsName(QStringLiteral("Beta")));
    QCOMPARE(table.indexOf(QStringLiteral("3")), 2);
    QCOMPARE(table.idForName(QStringLiteral("alpha")), QStringLiteral("1"));
    QCOMPARE(QString(table), QStringLiteral("1, 2, 3"));
}

void DataUnitTest::genericBasicTableRemovesAndClearsRecords()
{
    GenericBasicTable table;
    table << Generic(QStringLiteral("1"), QStringLiteral("One"));
    table << Generic(QString(), QStringLiteral("Ignored"));
    table << Generic(QStringLiteral("2"), QStringLiteral("Two"));

    QCOMPARE(table.rowCount(), 2);
    QVERIFY(table.rowExists(1));
    QVERIFY(!table.rowExists(2));

    table.remove(QStringLiteral("1"));
    QCOMPARE(table.rowCount(), 1);
    QCOMPARE(table[0u].id, QStringLiteral("2"));

    table.remove(0);
    QVERIFY(table.isEmpty());

    table << Generic(QStringLiteral("3"), QStringLiteral("Three"));
    table.clear();
    QVERIFY(table.isEmpty());
}

void DataUnitTest::genericDefaultConstructsEmptyValues()
{
    const Generic data;

    QVERIFY(data.id.isEmpty());
    QVERIFY(data.name.isEmpty());
}

void DataUnitTest::genericCopiesAndAssignsAllFields()
{
    const Generic source(QStringLiteral("layout-id"), QStringLiteral("Layout Name"));

    const Generic copied(source);
    QCOMPARE(copied.id, source.id);
    QCOMPARE(copied.name, source.name);

    Generic assigned;
    assigned = source;
    QCOMPARE(assigned.id, source.id);
    QCOMPARE(assigned.name, source.name);

    Generic moved{Generic(source)};
    QCOMPARE(moved.id, source.id);
    QCOMPARE(moved.name, source.name);
}

void DataUnitTest::genericEqualityComparesIdAndName()
{
    const Generic original(QStringLiteral("layout-id"), QStringLiteral("Layout Name"));
    const Generic same(QStringLiteral("layout-id"), QStringLiteral("Layout Name"));
    const Generic differentId(QStringLiteral("other-id"), QStringLiteral("Layout Name"));
    const Generic differentName(QStringLiteral("layout-id"), QStringLiteral("Other Name"));

    QVERIFY(original == same);
    QVERIFY(!(original != same));
    QVERIFY(original != differentId);
    QVERIFY(original != differentName);
}

void DataUnitTest::layoutActivityPredicatesReflectActivitySet()
{
    Layout layout;
    QVERIFY(layout.isNull());
    QVERIFY(layout.isEmpty());

    layout.id = QStringLiteral("layout-id");
    layout.name = QStringLiteral("Layout");
    layout.activities = QStringList(Layout::ALLACTIVITIESID);
    QVERIFY(layout.isOnAllActivities());
    QVERIFY(!layout.isForFreeActivities());

    layout.activities = QStringList(Layout::FREEACTIVITIESID);
    QVERIFY(layout.isForFreeActivities());
    QVERIFY(!layout.isOnAllActivities());

    layout.id = QStringLiteral("/tmp/latte-layout");
    QVERIFY(layout.isTemporary());
}

void DataUnitTest::layoutEqualityIgnoresRuntimeState()
{
    Layout first;
    first.id = QStringLiteral("layout-id");
    first.name = QStringLiteral("Layout");
    first.icon = QStringLiteral("latte");
    first.isShownInMenu = true;
    first.activities = QStringList(Layout::ALLACTIVITIESID);

    Layout second(first);
    second.isActive = !first.isActive;
    second.isConsideredActive = !first.isConsideredActive;
    second.errors = first.errors + 1;
    second.warnings = first.warnings + 1;

    QVERIFY(first == second);

    second.isLocked = !first.isLocked;
    QVERIFY(first != second);
}

void DataUnitTest::layoutTemplatePredicatesRespectUserAndTempPaths()
{
    Layout systemTemplate;
    systemTemplate.id = QStringLiteral("/usr/share/latte/templates/default.layout.latte");
    systemTemplate.name = QStringLiteral("System Template");
    systemTemplate.isTemplate = true;
    QVERIFY(systemTemplate.isSystemTemplate());

    Layout tempTemplate(systemTemplate);
    tempTemplate.id = QDir::tempPath() + QStringLiteral("/default.layout.latte");
    QVERIFY(!tempTemplate.isSystemTemplate());

    Layout userTemplate(systemTemplate);
    userTemplate.id = QDir::homePath() + QStringLiteral("/.local/share/latte/default.layout.latte");
    QVERIFY(!userTemplate.isSystemTemplate());

    Layout regularLayout(systemTemplate);
    regularLayout.isTemplate = false;
    QVERIFY(!regularLayout.isSystemTemplate());
}

void DataUnitTest::layoutColorSetDataPopulatesAllFields()
{
    LayoutColor color;
    color.setData(QStringLiteral("layout-id"),
                  QStringLiteral("Layout Name"),
                  QStringLiteral("/tmp/layout.colors"),
                  QStringLiteral("#fefefe"));

    QCOMPARE(color.id, QStringLiteral("layout-id"));
    QCOMPARE(color.name, QStringLiteral("Layout Name"));
    QCOMPARE(color.path, QStringLiteral("/tmp/layout.colors"));
    QCOMPARE(color.textColor, QStringLiteral("#fefefe"));

    LayoutColor same(color);
    QVERIFY(color == same);

    same.textColor = QStringLiteral("#101010");
    QVERIFY(color != same);
}

void DataUnitTest::layoutIconDetectsEmptyAndComparesFields()
{
    LayoutIcon icon;
    QVERIFY(icon.isEmpty());

    icon.id = QStringLiteral("layout-id");
    icon.name = QStringLiteral("Layout Name");
    icon.isBackgroundFile = false;
    QVERIFY(!icon.isEmpty());

    LayoutIcon same(icon);
    QVERIFY(icon == same);

    same.isBackgroundFile = true;
    QVERIFY(icon != same);
}

void DataUnitTest::layoutsTableSubtractsAndMarksFreeActivities()
{
    Layout first;
    first.id = QStringLiteral("first-layout");
    first.name = QStringLiteral("First");

    Layout second;
    second.id = QStringLiteral("second-layout");
    second.name = QStringLiteral("Second");

    LayoutsTable all;
    all << first;
    all << second;

    LayoutsTable subset;
    subset << first;

    const LayoutsTable subtracted = all.subtracted(subset);
    QCOMPARE(subtracted.rowCount(), 1);
    QCOMPARE(subtracted[0u].id, second.id);

    all.setLayoutForFreeActivities(second.id);
    QVERIFY(all[second.id].isForFreeActivities());
}

void DataUnitTest::preferencesDefaultsRoundTrip()
{
    Preferences preferences;
    QVERIFY(preferences.inDefaultValues());

    preferences.autostart = !Preferences::AUTOSTART;
    preferences.contextMenuAlwaysActions.clear();
    QVERIFY(!preferences.inDefaultValues());

    preferences.setToDefaults();
    QVERIFY(preferences.inDefaultValues());
    QCOMPARE(preferences.contextMenuAlwaysActions, Latte::Data::ContextMenu::ACTIONSALWAYSVISIBLE);
}

void DataUnitTest::preferencesEqualityComparesAllFields()
{
    Preferences original;
    Preferences same(original);
    Preferences changed(original);
    changed.parabolicSpread = Preferences::PARABOLICSPREAD + 1;

    QVERIFY(original == same);
    QVERIFY(!(original != same));
    QVERIFY(original != changed);
}

void DataUnitTest::pluginIdClassifiesLatteBuiltInsAndSeparators()
{
    QVERIFY(Latte::PluginId::isSeparatorPluginId(QStringLiteral("org.kde.latte.separator")));
    QVERIFY(Latte::PluginId::isSeparatorPluginId(QStringLiteral("audoban.applet.separator")));
    QVERIFY(!Latte::PluginId::isSeparatorPluginId(QStringLiteral("org.kde.latte.spacer")));

    QVERIFY(Latte::PluginId::isLatteBuiltIn(QStringLiteral("org.kde.latte.separator")));
    QVERIFY(Latte::PluginId::isLatteBuiltIn(QStringLiteral("org.kde.latte.spacer")));
    QVERIFY(Latte::PluginId::isLatteBuiltIn(QStringLiteral("org.kde.latte.plasmoid")));
    QVERIFY(Latte::PluginId::isLatteBuiltIn(QStringLiteral("org.kde.latte.splitter")));
    QVERIFY(!Latte::PluginId::isLatteBuiltIn(QStringLiteral("audoban.applet.separator")));
    QVERIFY(!Latte::PluginId::isLatteBuiltIn(QStringLiteral("org.kde.plasma.systemtray")));
}

void DataUnitTest::screenInitializesFromSerializedGeometry()
{
    const Screen screen(QStringLiteral("7"), QStringLiteral("DP-1:::10,20 300x400"));

    QCOMPARE(screen.id, QStringLiteral("7"));
    QCOMPARE(screen.name, QStringLiteral("DP-1"));
    QCOMPARE(screen.geometry, QRect(10, 20, 300, 400));
    QCOMPARE(screen.serialize(), QStringLiteral("DP-1:::10,20 300x400"));
    QVERIFY(!screen.isActive);
    QVERIFY(!screen.isRemovable);
    QVERIFY(!screen.isSelected);
}

void DataUnitTest::screenDetectsScreenGroups()
{
    Screen primary;
    primary.id = QString::number(Screen::ONPRIMARYID);
    QVERIFY(!primary.isScreensGroup());

    Screen allScreens;
    allScreens.id = QString::number(Screen::ONALLSCREENSID);
    QVERIFY(allScreens.isScreensGroup());

    Screen allSecondaryScreens;
    allSecondaryScreens.id = QString::number(Screen::ONALLSECONDARYSCREENSID);
    QVERIFY(allSecondaryScreens.isScreensGroup());
}

void DataUnitTest::screenEqualityIgnoresRuntimeState()
{
    Screen first(QStringLiteral("7"), QStringLiteral("DP-1:::10,20 300x400"));
    Screen second(first);

    second.isActive = !first.isActive;
    second.isRemovable = !first.isRemovable;

    QVERIFY(first == second);

    second.isSelected = !first.isSelected;
    QVERIFY(first != second);
}

void DataUnitTest::uniqueIdInfoDefaultConstructsEmptyValues()
{
    const UniqueIdInfo info;

    QVERIFY(info.newId.isEmpty());
    QVERIFY(info.newName.isEmpty());
    QVERIFY(info.oldId.isEmpty());
    QVERIFY(info.oldName.isEmpty());
}

void DataUnitTest::uniqueIdInfoCopiesAndAssignsAllFields()
{
    UniqueIdInfo source;
    source.newId = QStringLiteral("new-id");
    source.newName = QStringLiteral("New Name");
    source.oldId = QStringLiteral("old-id");
    source.oldName = QStringLiteral("Old Name");

    const UniqueIdInfo copied(source);
    QCOMPARE(copied.newId, source.newId);
    QCOMPARE(copied.newName, source.newName);
    QCOMPARE(copied.oldId, source.oldId);
    QCOMPARE(copied.oldName, source.oldName);

    UniqueIdInfo assigned;
    assigned = source;
    QCOMPARE(assigned.newId, source.newId);
    QCOMPARE(assigned.newName, source.newName);
    QCOMPARE(assigned.oldId, source.oldId);
    QCOMPARE(assigned.oldName, source.oldName);

    UniqueIdInfo moved{UniqueIdInfo(source)};
    QCOMPARE(moved.newId, source.newId);
    QCOMPARE(moved.newName, source.newName);
    QCOMPARE(moved.oldId, source.oldId);
    QCOMPARE(moved.oldName, source.oldName);
}

void DataUnitTest::uniqueIdInfoEqualityComparesAllFields()
{
    UniqueIdInfo original;
    original.newId = QStringLiteral("new-id");
    original.newName = QStringLiteral("New Name");
    original.oldId = QStringLiteral("old-id");
    original.oldName = QStringLiteral("Old Name");

    UniqueIdInfo same(original);
    UniqueIdInfo changedNewId(original);
    changedNewId.newId = QStringLiteral("different-new-id");
    UniqueIdInfo changedNewName(original);
    changedNewName.newName = QStringLiteral("Different New Name");
    UniqueIdInfo changedOldId(original);
    changedOldId.oldId = QStringLiteral("different-old-id");
    UniqueIdInfo changedOldName(original);
    changedOldName.oldName = QStringLiteral("Different Old Name");

    QVERIFY(original == same);
    QVERIFY(!(original != same));
    QVERIFY(original != changedNewId);
    QVERIFY(original != changedNewName);
    QVERIFY(original != changedOldId);
    QVERIFY(original != changedOldName);
}

void DataUnitTest::viewStatePredicatesReflectOriginAndCloneState()
{
    View view(QStringLiteral("view-id"), QStringLiteral("View"));
    QVERIFY(!view.isValid());
    QVERIFY(view.isOriginal());
    QVERIFY(!view.isCloned());
    QVERIFY(view.isHorizontal());

    view.setState(View::IsCreated);
    QVERIFY(view.isValid());
    QVERIFY(view.isCreated());

    view.isClonedFrom = 12;
    QVERIFY(view.isCloned());
    QVERIFY(!view.isOriginal());

    view.edge = Plasma::Types::LeftEdge;
    QVERIFY(view.isVertical());
    QVERIFY(!view.isHorizontal());

    view.setState(View::OriginFromLayout,
                  QStringLiteral("/tmp/view.layout.latte"),
                  QStringLiteral("layout-id"),
                  QStringLiteral("source-view-id"));
    QVERIFY(view.hasLayoutOrigin());
    QCOMPARE(view.originFile(), QStringLiteral("/tmp/view.layout.latte"));
    QCOMPARE(view.originLayout(), QStringLiteral("layout-id"));
    QCOMPARE(view.originView(), QStringLiteral("source-view-id"));
}

void DataUnitTest::viewEqualityIgnoresRuntimeState()
{
    View first(QStringLiteral("view-id"), QStringLiteral("View"));
    first.setState(View::IsCreated);
    first.subcontainments << Generic(QStringLiteral("subcontainment-id"), QStringLiteral("Subcontainment"));

    View second(first);
    second.isActive = !first.isActive;
    second.isMoveOrigin = !first.isMoveOrigin;
    second.isMoveDestination = !first.isMoveDestination;
    second.errors = first.errors + 1;
    second.warnings = first.warnings + 1;

    QVERIFY(first == second);

    second.onPrimary = !first.onPrimary;
    QVERIFY(first != second);
}

void DataUnitTest::viewStringShowsCombinedMoveOriginAndDestination()
{
    View view(QStringLiteral("view-id"), QStringLiteral("View"));
    view.setState(View::OriginFromLayout);
    view.isMoveOrigin = true;
    view.isMoveDestination = true;

    QVERIFY(QString(view).contains(QStringLiteral("↑↓")));
}

void DataUnitTest::viewsTableFindsSubcontainmentsAndTemporaryViews()
{
    View view(QStringLiteral("view-id"), QStringLiteral("View"));
    view.subcontainments << Generic(QStringLiteral("subcontainment-id"), QStringLiteral("Subcontainment"));

    ViewsTable table;
    table << view;

    QVERIFY(table.hasContainmentId(QStringLiteral("view-id")));
    QVERIFY(table.hasContainmentId(QStringLiteral("subcontainment-id")));
    QVERIFY(!table.hasContainmentId(QStringLiteral("missing-id")));

    table.appendTemporaryView(view);
    QCOMPARE(table.rowCount(), 2);
    QVERIFY(table[1u].id.startsWith(QStringLiteral("temp:")));

    const ViewsTable originals = table.onlyOriginals();
    QCOMPARE(originals.rowCount(), 2);
}

void DataUnitTest::viewsTableSubtractsAndFiltersOriginalViews()
{
    View original(QStringLiteral("original-view"), QStringLiteral("Original"));
    View cloned(QStringLiteral("cloned-view"), QStringLiteral("Cloned"));
    cloned.isClonedFrom = 7;

    ViewsTable all;
    all << original;
    all << cloned;

    ViewsTable subset;
    subset << original;

    const ViewsTable subtracted = all.subtracted(subset);
    QCOMPARE(subtracted.rowCount(), 1);
    QCOMPARE(subtracted[0u].id, cloned.id);

    const ViewsTable originals = all.onlyOriginals();
    QCOMPARE(originals.rowCount(), 1);
    QCOMPARE(originals[0u].id, original.id);
}

QTEST_GUILESS_MAIN(DataUnitTest)

#include "dataunittest.moc"
