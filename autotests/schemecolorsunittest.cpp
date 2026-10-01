/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "schemecolors.h"
#include "schemesmodel.h"
#include "importer.h"

#include <KConfigGroup>
#include <KConfigGui>
#include <KSharedConfig>

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

namespace {
QString s_configPath;
QStringList s_dataRoots;

//! Force linkage of KConfigGui so its QColor/QFont conversion registration
//! runs at library load. The test never calls a KConfigGui symbol directly,
//! so linkers with --as-needed would otherwise drop the dependency and the
//! KConfigGroup QColor reads would fail with "KConfigGui isn't linked".
[[maybe_unused]] static const bool s_kconfigGuiLinked = KConfigGui::hasSessionConfig();

QString writeScheme(const QString &path, const QString &name)
{
    QDir().mkpath(QFileInfo(path).absolutePath());

    KSharedConfigPtr config = KSharedConfig::openConfig(path);
    KConfigGroup general(config, QStringLiteral("General"));
    general.writeEntry(QStringLiteral("Name"), name);

    KConfigGroup wm(config, QStringLiteral("WM"));
    wm.writeEntry(QStringLiteral("activeBackground"), QColor(1, 2, 3));
    wm.writeEntry(QStringLiteral("activeForeground"), QColor(4, 5, 6));
    wm.writeEntry(QStringLiteral("inactiveBackground"), QColor(7, 8, 9));
    wm.writeEntry(QStringLiteral("inactiveForeground"), QColor(10, 11, 12));

    KConfigGroup selection(config, QStringLiteral("Colors:Selection"));
    selection.writeEntry(QStringLiteral("BackgroundNormal"), QColor(13, 14, 15));
    selection.writeEntry(QStringLiteral("ForegroundNormal"), QColor(16, 17, 18));

    KConfigGroup window(config, QStringLiteral("Colors:Window"));
    window.writeEntry(QStringLiteral("BackgroundNormal"), QColor(19, 20, 21));
    window.writeEntry(QStringLiteral("ForegroundNormal"), QColor(22, 23, 24));
    window.writeEntry(QStringLiteral("BackgroundAlternate"), QColor(25, 26, 27));
    window.writeEntry(QStringLiteral("ForegroundInactive"), QColor(28, 29, 30));
    window.writeEntry(QStringLiteral("ForegroundPositive"), QColor(31, 32, 33));
    window.writeEntry(QStringLiteral("ForegroundNeutral"), QColor(34, 35, 36));
    window.writeEntry(QStringLiteral("ForegroundNegative"), QColor(37, 38, 39));

    KConfigGroup button(config, QStringLiteral("Colors:Button"));
    button.writeEntry(QStringLiteral("ForegroundNormal"), QColor(40, 41, 42));
    button.writeEntry(QStringLiteral("BackgroundNormal"), QColor(43, 44, 45));
    button.writeEntry(QStringLiteral("DecorationHover"), QColor(46, 47, 48));
    button.writeEntry(QStringLiteral("DecorationFocus"), QColor(49, 50, 51));

    config->sync();
    return path;
}
}

namespace Latte {
QString configPath()
{
    return s_configPath;
}

namespace Layouts {
QString Importer::standardPath(QString subPath, bool localfirst)
{
    Q_UNUSED(localfirst)

    for (const QString &root : std::as_const(s_dataRoots)) {
        const QString path = root + QStringLiteral("/") + subPath;

        if (QFileInfo::exists(path)) {
            return path;
        }
    }

    return QString();
}

QStringList Importer::standardPathsFor(QString subPath, bool localfirst)
{
    Q_UNUSED(localfirst)

    QStringList paths;

    for (const QString &root : std::as_const(s_dataRoots)) {
        paths << root + QStringLiteral("/") + subPath;
    }

    return paths;
}
}
}

class SchemeColorsUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void schemeNameReadsConfigNameOrFallsBackToFileName();
    void possibleSchemeFileFindsExactAndSimplifiedNames();
    void possibleSchemeFileResolvesAutoAccentKdeglobals();
    void schemeColorsLoadsWindowManagerColors();
    void schemeColorsCanLoadPlasmaThemeColors();
    void schemesModelListsSystemColorsFirstAndUniqueSchemeFiles();
};

void SchemeColorsUnitTest::schemeNameReadsConfigNameOrFallsBackToFileName()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString named = writeScheme(dir.path() + QStringLiteral("/Named.colors"), QStringLiteral("Readable Name"));
    QCOMPARE(Latte::WindowSystem::SchemeColors::schemeName(named), QStringLiteral("Readable Name"));

    const QString fallback = dir.path() + QStringLiteral("/Fallback.colors");
    QDir().mkpath(QFileInfo(fallback).absolutePath());
    QFile fallbackFile(fallback);
    QVERIFY(fallbackFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    fallbackFile.close();
    QCOMPARE(Latte::WindowSystem::SchemeColors::schemeName(fallback), QStringLiteral("Fallback"));

    QCOMPARE(Latte::WindowSystem::SchemeColors::schemeName(dir.path() + QStringLiteral("/NotColors.txt")), QString());
}

void SchemeColorsUnitTest::possibleSchemeFileFindsExactAndSimplifiedNames()
{
    QTemporaryDir dataRoot;
    QVERIFY(dataRoot.isValid());
    s_dataRoots = {dataRoot.path()};

    const QString exact = writeScheme(dataRoot.path() + QStringLiteral("/color-schemes/Exact.colors"), QStringLiteral("Exact"));
    QCOMPARE(Latte::WindowSystem::SchemeColors::possibleSchemeFile(exact), exact);
    QCOMPARE(Latte::WindowSystem::SchemeColors::possibleSchemeFile(QStringLiteral("Exact")), exact);

    const QString simplified = writeScheme(dataRoot.path() + QStringLiteral("/color-schemes/MyScheme.colors"), QStringLiteral("My Scheme"));
    QCOMPARE(Latte::WindowSystem::SchemeColors::possibleSchemeFile(QStringLiteral("My Scheme")), simplified);
    QCOMPARE(Latte::WindowSystem::SchemeColors::possibleSchemeFile(QStringLiteral("My-Scheme")), simplified);
}

void SchemeColorsUnitTest::possibleSchemeFileResolvesAutoAccentKdeglobals()
{
    QTemporaryDir configRoot;
    QVERIFY(configRoot.isValid());
    s_configPath = configRoot.path();
    s_dataRoots.clear();

    QFile kdeglobals(configRoot.path() + QStringLiteral("/kdeglobals"));
    QVERIFY(kdeglobals.open(QIODevice::WriteOnly | QIODevice::Truncate));
    kdeglobals.write("[WM]\nactiveBackground=1,2,3\n");
    kdeglobals.close();

    QCOMPARE(Latte::WindowSystem::SchemeColors::possibleSchemeFile(QStringLiteral("kdeglobals")),
             configRoot.path() + QStringLiteral("/kdeglobals"));
}

void SchemeColorsUnitTest::schemeColorsLoadsWindowManagerColors()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString scheme = writeScheme(dir.path() + QStringLiteral("/Colors.colors"), QStringLiteral("Colors"));
    Latte::WindowSystem::SchemeColors colors(nullptr, scheme);
    QSignalSpy schemeFileSpy(&colors, &Latte::WindowSystem::SchemeColors::schemeFileChanged);
    colors.setSchemeFile(scheme);

    QCOMPARE(colors.schemeFile(), scheme);
    QCOMPARE(colors.schemeName(), QStringLiteral("Colors"));
    QCOMPARE(colors.backgroundColor(), QColor(1, 2, 3));
    QCOMPARE(colors.textColor(), QColor(4, 5, 6));
    QCOMPARE(colors.inactiveBackgroundColor(), QColor(7, 8, 9));
    QCOMPARE(colors.inactiveTextColor(), QColor(10, 11, 12));
    QCOMPARE(colors.highlightColor(), QColor(13, 14, 15));
    QCOMPARE(colors.highlightedTextColor(), QColor(16, 17, 18));
    QCOMPARE(colors.positiveTextColor(), QColor(31, 32, 33));
    QCOMPARE(colors.buttonFocusColor(), QColor(49, 50, 51));
    QCOMPARE(schemeFileSpy.count(), 0);
}

void SchemeColorsUnitTest::schemeColorsCanLoadPlasmaThemeColors()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString scheme = writeScheme(dir.path() + QStringLiteral("/Plasma.colors"), QStringLiteral("Plasma"));
    Latte::WindowSystem::SchemeColors colors(nullptr, scheme, true);

    QCOMPARE(colors.backgroundColor(), QColor(19, 20, 21));
    QCOMPARE(colors.textColor(), QColor(22, 23, 24));
    QCOMPARE(colors.inactiveBackgroundColor(), QColor(25, 26, 27));
    QCOMPARE(colors.inactiveTextColor(), QColor(28, 29, 30));
}

void SchemeColorsUnitTest::schemesModelListsSystemColorsFirstAndUniqueSchemeFiles()
{
    QTemporaryDir configRoot;
    QTemporaryDir firstDataRoot;
    QTemporaryDir secondDataRoot;
    QVERIFY(configRoot.isValid());
    QVERIFY(firstDataRoot.isValid());
    QVERIFY(secondDataRoot.isValid());

    s_configPath = configRoot.path();
    s_dataRoots = {firstDataRoot.path(), secondDataRoot.path()};

    const QString beta = writeScheme(firstDataRoot.path() + QStringLiteral("/color-schemes/Beta.colors"), QStringLiteral("Beta"));
    const QString alpha = writeScheme(firstDataRoot.path() + QStringLiteral("/color-schemes/Alpha.colors"), QStringLiteral("Alpha"));
    writeScheme(secondDataRoot.path() + QStringLiteral("/color-schemes/Alpha.colors"), QStringLiteral("Alpha Duplicate"));

    Latte::Settings::Model::Schemes model;

    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.data(model.index(0, 0), Latte::Settings::Model::Schemes::IDROLE).toString(), QStringLiteral("kdeglobals"));
    QCOMPARE(model.data(model.index(0, 0), Qt::DisplayRole).toString(), QStringLiteral("System Colors"));

    QCOMPARE(model.data(model.index(1, 0), Latte::Settings::Model::Schemes::NAMEROLE).toString(), QStringLiteral("Alpha"));
    QCOMPARE(model.data(model.index(1, 0), Latte::Settings::Model::Schemes::IDROLE).toString(), alpha);
    QCOMPARE(model.data(model.index(2, 0), Latte::Settings::Model::Schemes::NAMEROLE).toString(), QStringLiteral("Beta"));
    QCOMPARE(model.data(model.index(2, 0), Latte::Settings::Model::Schemes::IDROLE).toString(), beta);

    QCOMPARE(model.row(QString()), 0);
    QCOMPARE(model.row(QStringLiteral("kdeglobals")), 0);
    QCOMPARE(model.row(alpha), 1);
    QCOMPARE(model.row(QStringLiteral("/missing/colors")), -1);
    QCOMPARE(model.data(model.index(1, 0), Latte::Settings::Model::Schemes::TEXTCOLORROLE).value<QColor>(), QColor(4, 5, 6));
    QVERIFY(!model.data(model.index(10, 0), Qt::DisplayRole).isValid());
}

QTEST_GUILESS_MAIN(SchemeColorsUnitTest)

#include "schemecolorsunittest.moc"
