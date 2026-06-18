/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "abstractlayout.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

class LayoutUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void layoutNameStripsPathAndLatteSuffix();
    void combinedFreeEdgesPreservesFirstListOrder();
    void abstractLayoutUsesAssignedNameWhenProvided();
    void abstractLayoutLoadsSettingsFromFile();
    void abstractLayoutSettersPersistSettings();
    void abstractLayoutPersistsCustomSchemeFile();
};

void LayoutUnitTest::layoutNameStripsPathAndLatteSuffix()
{
    QCOMPARE(Latte::Layout::AbstractLayout::layoutName(QStringLiteral("/tmp/My Layout.layout.latte")),
             QStringLiteral("My Layout"));
    QCOMPARE(Latte::Layout::AbstractLayout::layoutName(QStringLiteral("Relative.layout.latte")),
             QStringLiteral("Relative"));
}

void LayoutUnitTest::combinedFreeEdgesPreservesFirstListOrder()
{
    const QList<Plasma::Types::Location> primary{
        Plasma::Types::BottomEdge,
        Plasma::Types::LeftEdge,
        Plasma::Types::TopEdge
    };

    const QList<Plasma::Types::Location> secondary{
        Plasma::Types::TopEdge,
        Plasma::Types::BottomEdge
    };

    const QList<Plasma::Types::Location> combined = Latte::Layout::AbstractLayout::combinedFreeEdges(primary, secondary);
    QCOMPARE(combined.count(), 2);
    QCOMPARE(combined[0], Plasma::Types::BottomEdge);
    QCOMPARE(combined[1], Plasma::Types::TopEdge);
}

void LayoutUnitTest::abstractLayoutUsesAssignedNameWhenProvided()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString layoutFile = dir.path() + QStringLiteral("/Imported File Name.layout.latte");
    QVERIFY(QFile(layoutFile).open(QIODevice::WriteOnly));

    Latte::Layout::AbstractLayout layout(nullptr, layoutFile, QStringLiteral("Stable Imported Layout"));
    QCOMPARE(layout.name(), QStringLiteral("Stable Imported Layout"));
    QCOMPARE(layout.file(), layoutFile);
}

void LayoutUnitTest::abstractLayoutLoadsSettingsFromFile()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString layoutFile = dir.path() + QStringLiteral("/Loaded.layout.latte");
    KSharedConfigPtr config = KSharedConfig::openConfig(layoutFile);
    KConfigGroup group(config, "LayoutSettings");
    group.writeEntry("version", 7);
    group.writeEntry("launchers", QStringList{QStringLiteral("applications:org.kde.konsole.desktop")});
    group.writeEntry("lastUsedActivity", QStringLiteral("activity-1"));
    group.writeEntry("preferredForShortcutsTouched", true);
    group.writeEntry("popUpMargin", 12);
    group.writeEntry("schemeFile", QStringLiteral("/missing/color-scheme.colors"));
    group.writeEntry("icon", QStringLiteral("latte-dock"));
    group.sync();

    Latte::Layout::AbstractLayout layout(nullptr, layoutFile);
    QCOMPARE(layout.name(), QStringLiteral("Loaded"));
    QCOMPARE(layout.file(), layoutFile);
    QCOMPARE(layout.version(), 7);
    QCOMPARE(layout.launchers(), QStringList{QStringLiteral("applications:org.kde.konsole.desktop")});
    QCOMPARE(layout.lastUsedActivity(), QStringLiteral("activity-1"));
    QCOMPARE(layout.preferredForShortcutsTouched(), true);
    QCOMPARE(layout.popUpMargin(), 12);
    QCOMPARE(layout.icon(), QStringLiteral("latte-dock"));
    QCOMPARE(layout.schemeFile(), QStringLiteral("kdeglobals"));
}

void LayoutUnitTest::abstractLayoutSettersPersistSettings()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString layoutFile = dir.path() + QStringLiteral("/Persisted.layout.latte");
    QVERIFY(QFile(layoutFile).open(QIODevice::WriteOnly));

    Latte::Layout::AbstractLayout layout(nullptr, layoutFile);
    QSignalSpy iconSpy(&layout, &Latte::Layout::AbstractLayout::iconChanged);
    QSignalSpy launchersSpy(&layout, &Latte::Layout::AbstractLayout::launchersChanged);

    layout.setIcon(QStringLiteral("new-icon"));
    layout.setLaunchers(QStringList{QStringLiteral("applications:org.kde.dolphin.desktop")});
    layout.setVersion(9);
    layout.setPopUpMargin(22);
    layout.setPreferredForShortcutsTouched(true);
    layout.clearLastUsedActivity();

    QCOMPARE(iconSpy.count(), 1);
    QCOMPARE(launchersSpy.count(), 1);

    KSharedConfigPtr config = KSharedConfig::openConfig(layoutFile);
    KConfigGroup group(config, "LayoutSettings");
    QCOMPARE(group.readEntry("icon", QString()), QStringLiteral("new-icon"));
    QCOMPARE(group.readEntry("launchers", QStringList()), QStringList{QStringLiteral("applications:org.kde.dolphin.desktop")});
    QCOMPARE(group.readEntry("version", 0), 9);
    QCOMPARE(group.readEntry("popUpMargin", -1), 22);
    QCOMPARE(group.readEntry("preferredForShortcutsTouched", false), true);
    QCOMPARE(group.readEntry("lastUsedActivity", QStringLiteral("not-cleared")), QString());
}

void LayoutUnitTest::abstractLayoutPersistsCustomSchemeFile()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString layoutFile = dir.path() + QStringLiteral("/Scheme.layout.latte");
    const QString schemeFile = dir.path() + QStringLiteral("/Custom.colors");
    QVERIFY(QFile(layoutFile).open(QIODevice::WriteOnly));
    QVERIFY(QFile(schemeFile).open(QIODevice::WriteOnly));

    Latte::Layout::AbstractLayout layout(nullptr, layoutFile);
    QSignalSpy schemeSpy(&layout, &Latte::Layout::AbstractLayout::schemeFileChanged);

    layout.setSchemeFile(schemeFile);

    QCOMPARE(schemeSpy.count(), 1);
    QCOMPARE(layout.schemeFile(), schemeFile);

    KSharedConfigPtr config = KSharedConfig::openConfig(layoutFile);
    KConfigGroup group(config, "LayoutSettings");
    QCOMPARE(group.readEntry("schemeFile", QString()), schemeFile);

    Latte::Layout::AbstractLayout reloaded(nullptr, layoutFile);
    QCOMPARE(reloaded.schemeFile(), schemeFile);
}

QTEST_GUILESS_MAIN(LayoutUnitTest)

#include "layoutunittest.moc"
