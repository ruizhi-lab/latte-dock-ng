/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "appletsmodel.h"
#include "screensmodel.h"

#include <QSignalSpy>
#include <QTest>
#include <QFont>

#include <KLocalizedString>

using Latte::Data::Applet;
using Latte::Data::AppletsTable;
using Latte::Data::Screen;
using Latte::Data::ScreensTable;
using Latte::Settings::Model::Applets;
using Latte::Settings::Model::Screens;

class ModelUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void appletsModelInitializesDefaultsAndRoles();
    void appletsModelHandlesInvalidIndexesAndEmptyData();
    void appletsModelSelectionOperationsUpdateSelectedRows();
    void appletsModelDoesNotEmitWhenSelectionIsUnchanged();
    void appletsModelSetSelectedOnlyChangesMatchingRows();
    void appletsModelRejectsUnsupportedRolesAndColumns();
    void screensModelInitializesDefaultsAndRoles();
    void screensModelHandlesInvalidIndexesAndEmptyData();
    void screensModelSelectionOperationsUpdateCheckedRows();
    void screensModelDoesNotEmitForUnchangedSetSelected();
    void screensModelFlagsDependOnRemovableState();
    void screensModelRejectsUnsupportedRolesAndColumns();
};

namespace {

Applet makeApplet(const QString &id, const QString &name, const QString &icon, const QString &description)
{
    Applet applet;
    applet.id = id;
    applet.name = name;
    applet.icon = icon;
    applet.description = description;
    return applet;
}

AppletsTable makeAppletsTable()
{
    AppletsTable applets;
    applets << makeApplet(QStringLiteral("org.kde.latte.separator"),
                          QStringLiteral("Separator"),
                          QStringLiteral("separator-icon"),
                          QStringLiteral("No personal data"));
    applets << makeApplet(QStringLiteral("org.kde.latte.tasks"),
                          QStringLiteral("Latte Tasks"),
                          QStringLiteral("tasks-icon"),
                          QStringLiteral("Task manager"));
    return applets;
}

Screen makeScreen(const QString &id, const QString &name, bool active, bool removable)
{
    Screen screen;
    screen.id = id;
    screen.name = name;
    screen.isActive = active;
    screen.isRemovable = removable;
    return screen;
}

ScreensTable makeScreensTable()
{
    ScreensTable screens;
    screens << makeScreen(QStringLiteral("10"), QStringLiteral("DP-1"), true, false);
    screens << makeScreen(QStringLiteral("11"), QStringLiteral("HDMI-A-1"), false, true);
    return screens;
}

}

void ModelUnitTest::initTestCase()
{
    KLocalizedString::setApplicationDomain("latte-dock");
}

void ModelUnitTest::appletsModelInitializesDefaultsAndRoles()
{
    Applets model(nullptr);
    const AppletsTable applets = makeAppletsTable();

    QSignalSpy changedSpy(&model, &Applets::appletsDataChanged);
    model.setData(applets);

    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.rowCount(QModelIndex()), 2);
    QCOMPARE(model.columnCount(QModelIndex()), 1);
    QCOMPARE(model.row(QStringLiteral("org.kde.latte.tasks")), 1);
    QCOMPARE(changedSpy.count(), 1);
    QVERIFY(model.inDefaultValues());
    QVERIFY(!model.hasChangedData());

    const QModelIndex separatorIndex = model.index(0, Applets::NAMECOLUMN);
    QCOMPARE(model.data(separatorIndex, Applets::IDROLE).toString(), QStringLiteral("org.kde.latte.separator"));
    QCOMPARE(model.data(separatorIndex, Applets::NAMEROLE).toString(), QStringLiteral("Separator"));
    QCOMPARE(model.data(separatorIndex, Applets::ICONROLE).toString(), QStringLiteral("separator-icon"));
    QCOMPARE(model.data(separatorIndex, Applets::DESCRIPTIONROLE).toString(), QStringLiteral("No personal data"));
    QCOMPARE(model.data(separatorIndex, Applets::SELECTEDROLE).toBool(), true);
    QCOMPARE(model.data(separatorIndex, Qt::CheckStateRole).value<Qt::CheckState>(), Qt::Checked);

    const QModelIndex tasksIndex = model.index(1, Applets::NAMECOLUMN);
    QCOMPARE(model.data(tasksIndex, Applets::SELECTEDROLE).toBool(), false);
    QCOMPARE(model.data(tasksIndex, Qt::CheckStateRole).value<Qt::CheckState>(), Qt::Unchecked);
    QVERIFY(model.data(tasksIndex, Applets::SORTINGROLE).toString().startsWith(QStringLiteral("1000")));
    QCOMPARE(model.headerData(Applets::NAMECOLUMN, Qt::Horizontal, Qt::DisplayRole).toString(), QStringLiteral("Current Applets"));
    QVERIFY(model.headerData(Applets::NAMECOLUMN, Qt::Horizontal, Qt::FontRole).value<QFont>().bold());
}

void ModelUnitTest::appletsModelHandlesInvalidIndexesAndEmptyData()
{
    Applets model(nullptr);
    model.setData(makeAppletsTable());

    QVERIFY(!model.data(QModelIndex(), Qt::DisplayRole).isValid());
    QVERIFY(!model.data(model.index(9, Applets::NAMECOLUMN), Qt::DisplayRole).isValid());
    QVERIFY(!model.setData(model.index(9, Applets::NAMECOLUMN), Qt::Checked, Qt::CheckStateRole));

    AppletsTable empty;
    QSignalSpy changedSpy(&model, &Applets::appletsDataChanged);
    model.setData(empty);

    QCOMPARE(model.rowCount(), 0);
    QVERIFY(changedSpy.count() >= 1);
}

void ModelUnitTest::appletsModelSelectionOperationsUpdateSelectedRows()
{
    Applets model(nullptr);
    model.setData(makeAppletsTable());

    model.selectAll();
    QCOMPARE(model.selectedApplets().rowCount(), 2);
    QVERIFY(model.hasChangedData());

    model.deselectAll();
    QCOMPARE(model.selectedApplets().rowCount(), 0);

    model.reset();
    QCOMPARE(model.selectedApplets().rowCount(), 1);
    QCOMPARE(model.selectedApplets()[0u].id, QStringLiteral("org.kde.latte.separator"));
    QVERIFY(model.inDefaultValues());
}

void ModelUnitTest::appletsModelDoesNotEmitWhenSelectionIsUnchanged()
{
    Applets model(nullptr);
    model.setData(makeAppletsTable());

    model.selectAll();
    QSignalSpy changedSpy(&model, &Applets::appletsDataChanged);
    model.selectAll();
    QCOMPARE(changedSpy.count(), 0);

    model.deselectAll();
    changedSpy.clear();
    model.deselectAll();
    QCOMPARE(changedSpy.count(), 0);
}

void ModelUnitTest::appletsModelSetSelectedOnlyChangesMatchingRows()
{
    Applets model(nullptr);
    model.setData(makeAppletsTable());

    AppletsTable selected;
    Applet tasks;
    tasks.id = QStringLiteral("org.kde.latte.tasks");
    tasks.isSelected = true;
    selected << tasks;

    model.setSelected(selected);
    QCOMPARE(model.selectedApplets().rowCount(), 2);
    QCOMPARE(model.data(model.index(1, Applets::NAMECOLUMN), Qt::CheckStateRole).value<Qt::CheckState>(), Qt::Checked);

    QVERIFY(model.setData(model.index(1, Applets::NAMECOLUMN), Qt::Unchecked, Qt::CheckStateRole));
    QCOMPARE(model.data(model.index(1, Applets::NAMECOLUMN), Qt::CheckStateRole).value<Qt::CheckState>(), Qt::Unchecked);
    QVERIFY(!model.setData(QModelIndex(), Qt::Checked, Qt::CheckStateRole));
    QVERIFY(model.flags(QModelIndex()) & Qt::ItemIsUserCheckable);
}

void ModelUnitTest::appletsModelRejectsUnsupportedRolesAndColumns()
{
    Applets model(nullptr);
    model.setData(makeAppletsTable());

    const QModelIndex firstIndex = model.index(0, Applets::NAMECOLUMN);
    QVERIFY(!model.setData(firstIndex, Qt::Unchecked, Qt::EditRole));
    QCOMPARE(model.data(firstIndex, Qt::CheckStateRole).value<Qt::CheckState>(), Qt::Checked);

    QVERIFY(!model.setData(model.index(0, Applets::NAMECOLUMN + 1), Qt::Unchecked, Qt::CheckStateRole));
    QVERIFY(!model.data(model.index(0, Applets::NAMECOLUMN + 1), Qt::DisplayRole).isValid());
}

void ModelUnitTest::screensModelInitializesDefaultsAndRoles()
{
    Screens model(nullptr);
    const ScreensTable screens = makeScreensTable();

    QSignalSpy changedSpy(&model, &Screens::screenDataChanged);
    model.setData(screens);

    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.rowCount(QModelIndex()), 2);
    QCOMPARE(model.columnCount(QModelIndex()), 1);
    QCOMPARE(model.row(QStringLiteral("11")), 1);
    QCOMPARE(changedSpy.count(), 1);
    QVERIFY(model.inDefaultValues());
    QVERIFY(!model.hasChangedData());
    QVERIFY(!model.hasChecked());

    const QModelIndex primaryIndex = model.index(0, Screens::SCREENCOLUMN);
    QCOMPARE(model.data(primaryIndex, Screens::IDROLE).toString(), QStringLiteral("10"));
    QCOMPARE(model.data(primaryIndex, Qt::DisplayRole).toString(), QStringLiteral("{10} DP-1"));
    QCOMPARE(model.data(primaryIndex, Screens::ISSCREENACTIVEROLE).toBool(), true);
    QCOMPARE(model.data(primaryIndex, Screens::ISSELECTEDROLE).toBool(), false);
    QVERIFY(model.data(primaryIndex, Screens::SCREENDATAROLE).value<Screen>() == screens[0u]);
    QVERIFY(model.data(primaryIndex, Screens::SORTINGROLE).toString().startsWith(QStringLiteral("010000")));
    QCOMPARE(model.headerData(Screens::SCREENCOLUMN, Qt::Horizontal, Qt::DisplayRole).toString(), QStringLiteral("Screens"));
    QVERIFY(model.headerData(Screens::SCREENCOLUMN, Qt::Horizontal, Qt::FontRole).value<QFont>().bold());
}

void ModelUnitTest::screensModelHandlesInvalidIndexesAndEmptyData()
{
    Screens model(nullptr);
    model.setData(makeScreensTable());

    QVERIFY(!model.data(QModelIndex(), Qt::DisplayRole).isValid());
    QVERIFY(!model.data(model.index(9, Screens::SCREENCOLUMN), Qt::DisplayRole).isValid());
    QVERIFY(!model.setData(model.index(9, Screens::SCREENCOLUMN), Qt::Checked, Qt::CheckStateRole));

    ScreensTable empty;
    QSignalSpy changedSpy(&model, &Screens::screenDataChanged);
    model.setData(empty);

    QCOMPARE(model.rowCount(), 0);
    QVERIFY(changedSpy.count() >= 1);
}

void ModelUnitTest::screensModelSelectionOperationsUpdateCheckedRows()
{
    Screens model(nullptr);
    model.setData(makeScreensTable());

    QVERIFY(model.setData(model.index(1, Screens::SCREENCOLUMN), Qt::Checked, Qt::CheckStateRole));
    QVERIFY(model.hasChecked());
    QCOMPARE(model.checkedScreens().rowCount(), 1);
    QCOMPARE(model.checkedScreens()[0u].id, QStringLiteral("11"));

    ScreensTable selected;
    Screen primary;
    primary.id = QStringLiteral("10");
    primary.isSelected = true;
    selected << primary;
    model.setSelected(selected);

    QCOMPARE(model.data(model.index(0, Screens::SCREENCOLUMN), Screens::ISSELECTEDROLE).toBool(), true);
    QCOMPARE(model.checkedScreens().rowCount(), 1);

    model.deselectAll();
    QVERIFY(!model.hasChecked());

    model.reset();
    QVERIFY(model.inDefaultValues());
}

void ModelUnitTest::screensModelDoesNotEmitForUnchangedSetSelected()
{
    Screens model(nullptr);
    model.setData(makeScreensTable());

    ScreensTable selected;
    Screen secondary;
    secondary.id = QStringLiteral("11");
    secondary.isSelected = false;
    selected << secondary;

    QSignalSpy changedSpy(&model, &Screens::screenDataChanged);
    model.setSelected(selected);
    QCOMPARE(changedSpy.count(), 0);
}

void ModelUnitTest::screensModelFlagsDependOnRemovableState()
{
    Screens model(nullptr);
    model.setData(makeScreensTable());

    const Qt::ItemFlags primaryFlags = model.flags(model.index(0, Screens::SCREENCOLUMN));
    QVERIFY(!(primaryFlags & Qt::ItemIsUserCheckable));
    QVERIFY(!(primaryFlags & Qt::ItemIsSelectable));

    const Qt::ItemFlags removableFlags = model.flags(model.index(1, Screens::SCREENCOLUMN));
    QVERIFY(removableFlags & Qt::ItemIsUserCheckable);

    const Qt::ItemFlags invalidFlags = model.flags(QModelIndex());
    QVERIFY(!(invalidFlags & Qt::ItemIsUserCheckable));
}

void ModelUnitTest::screensModelRejectsUnsupportedRolesAndColumns()
{
    Screens model(nullptr);
    model.setData(makeScreensTable());

    const QModelIndex secondaryIndex = model.index(1, Screens::SCREENCOLUMN);
    QVERIFY(!model.setData(secondaryIndex, Qt::Checked, Qt::EditRole));
    QCOMPARE(model.data(secondaryIndex, Qt::CheckStateRole).value<Qt::CheckState>(), Qt::Unchecked);

    QVERIFY(!model.setData(model.index(1, Screens::SCREENCOLUMN + 1), Qt::Checked, Qt::CheckStateRole));
    QVERIFY(!model.data(model.index(1, Screens::SCREENCOLUMN + 1), Qt::DisplayRole).isValid());

    QVERIFY(model.setData(secondaryIndex, Qt::Checked, Qt::CheckStateRole));
    QVERIFY(!model.setData(model.index(0, Screens::SCREENCOLUMN), Qt::Checked, Qt::EditRole));
    QCOMPARE(model.checkedScreens().rowCount(), 1);
}

QTEST_GUILESS_MAIN(ModelUnitTest)

#include "modelunittest.moc"
