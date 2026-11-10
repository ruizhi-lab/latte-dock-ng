/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QTest>

// The production resolver is compiled directly (see autotests/CMakeLists.txt).
#include "templates/viewtemplatenames.h"

using Latte::Templates::ViewTemplateEntry;
using Latte::Templates::selectViewTemplateId;
using Latte::Templates::viewTemplateBaseName;

namespace {

//! Shown name of the shell's "Default Dock" under zh_CN (po/zh_CN/latte-dock.po).
QString shownDefaultDock()
{
    return QString::fromUtf8("默认停靠栏");
}

QString shellDefaultDockPath()
{
    return QStringLiteral("/usr/share/plasma/shells/org.kde.latte.shell/contents/templates/Default Dock.view.latte");
}

QString userDockPath()
{
    return QStringLiteral("/home/user/.config/latte/templates/My Dock.view.latte");
}

QString userDottedDockPath()
{
    return QStringLiteral("/home/user/.config/latte/templates/My.Dock.view.latte");
}

//! The table the template manager builds under zh_CN: the shell template is shown translated, the user templates are not.
//! A user template's shown name is its base name, so the dotted one takes the name from the derivation under test.
QList<ViewTemplateEntry> translatedTable()
{
    return {
        {shellDefaultDockPath(), shownDefaultDock()},
        {userDockPath(), QStringLiteral("My Dock")},
        {userDottedDockPath(), viewTemplateBaseName(userDottedDockPath())},
    };
}

} // namespace

class ViewTemplateNamesTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void baseNameIsFileNameWithoutSuffix();
    void baseNameKeepsDots();

    void translatedNameSelectsSystemTemplate();
    void untranslatedBaseNameSelectsSystemTemplate();
    void userTemplateSelectsByItsName();

    void unknownNameSelectsNothing();
    void shownNameWinsOverAnotherTemplatesBaseName();
};

void ViewTemplateNamesTest::baseNameIsFileNameWithoutSuffix()
{
    QCOMPARE(viewTemplateBaseName(shellDefaultDockPath()), QStringLiteral("Default Dock"));
    QCOMPARE(viewTemplateBaseName(QStringLiteral("Plain.view.latte")), QStringLiteral("Plain"));
}

void ViewTemplateNamesTest::baseNameKeepsDots()
{
    QCOMPARE(viewTemplateBaseName(userDottedDockPath()), QStringLiteral("My.Dock"));
}

void ViewTemplateNamesTest::translatedNameSelectsSystemTemplate()
{
    QCOMPARE(selectViewTemplateId(translatedTable(), shownDefaultDock()), shellDefaultDockPath());
}

void ViewTemplateNamesTest::untranslatedBaseNameSelectsSystemTemplate()
{
    // --available-dock-templates prints this name; it must add the same dock as the shown name.
    QCOMPARE(selectViewTemplateId(translatedTable(), QStringLiteral("Default Dock")), shellDefaultDockPath());
}

void ViewTemplateNamesTest::userTemplateSelectsByItsName()
{
    QCOMPARE(selectViewTemplateId(translatedTable(), QStringLiteral("My Dock")), userDockPath());
    QCOMPARE(selectViewTemplateId(translatedTable(), QStringLiteral("My.Dock")), userDottedDockPath());
}

void ViewTemplateNamesTest::unknownNameSelectsNothing()
{
    QVERIFY(selectViewTemplateId(translatedTable(), QString()).isEmpty());
    QVERIFY(selectViewTemplateId(translatedTable(), QStringLiteral("Missing Dock")).isEmpty());
    // Matching is exact, as it was for the shown names.
    QVERIFY(selectViewTemplateId(translatedTable(), QStringLiteral("default dock")).isEmpty());
    QVERIFY(selectViewTemplateId(QList<ViewTemplateEntry>(), QStringLiteral("Default Dock")).isEmpty());
}

void ViewTemplateNamesTest::shownNameWinsOverAnotherTemplatesBaseName()
{
    // A user template whose file is named like the shown name of the system template must not take that name:
    // the shown name is matched first, so the name keeps selecting the template the UI shows under it.
    const QList<ViewTemplateEntry> table = {
        {shellDefaultDockPath(), shownDefaultDock()},
        {QStringLiteral("/home/user/.config/latte/templates/") + shownDefaultDock() + QStringLiteral(".view.latte"), QStringLiteral("Other Dock")},
    };

    QCOMPARE(selectViewTemplateId(table, shownDefaultDock()), shellDefaultDockPath());
    QCOMPARE(selectViewTemplateId(table, QStringLiteral("Other Dock")),
             QStringLiteral("/home/user/.config/latte/templates/") + shownDefaultDock() + QStringLiteral(".view.latte"));
}

QTEST_MAIN(ViewTemplateNamesTest)

#include "viewtemplatenamestest.moc"
