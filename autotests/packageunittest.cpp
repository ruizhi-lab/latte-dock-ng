/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "lattepackage.h"

#include <KPackage/Package>
#include <KPackage/PackageLoader>

#include <Plasma/version.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

class PackageUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void lattePackageResolvesShellDefinitions();
    void appletAlternativesDialogMatchesPlasmaRelease();
    void indicatorPackageLoadsFromPluginAndResolvesDirectories();
    void indicatorPackageResolvesBundledDefaultIndicator();
    void packagePathChangesHandleForeignMetadata();
};

void PackageUnitTest::lattePackageResolvesShellDefinitions()
{
    KPackage::Package package(new Latte::Package);
    package.setPath(QStringLiteral(LATTE_SOURCE_DIR "/shell/package"));

    QVERIFY(package.filePath("lattedockui").endsWith(QStringLiteral("views/Panel.qml")));
    QVERIFY(package.filePath("widgetexplorerui").endsWith(QStringLiteral("views/WidgetExplorer.qml")));
    QVERIFY(package.filePath("configmodel").endsWith(QStringLiteral("configuration/config.qml")));
    QVERIFY(package.filePath("templates").endsWith(QStringLiteral("templates")));
    QVERIFY(package.filePath("compactapplet").endsWith(QStringLiteral("applet/CompactApplet.qml")));
}

void PackageUnitTest::appletAlternativesDialogMatchesPlasmaRelease()
{
    // From plasma-desktop 6.5.0 the shared explorer/AppletAlternatives.qml
    // requires the plasmashell-only org.kde.plasma.shell module and cannot
    // load in Latte, so the shell package must resolve the dialog to Latte's
    // own copy. Older releases keep the shared file, which works there.
    KPackage::Package package(new Latte::Package);
    package.setPath(QStringLiteral(LATTE_SOURCE_DIR "/shell/package"));
    const QString dialog = QFileInfo(package.filePath("appletalternativesui")).canonicalFilePath();
    const QString lattesDialog = QFileInfo(QStringLiteral(LATTE_SOURCE_DIR "/shell/package/contents/explorer/AppletAlternatives.qml")).canonicalFilePath();
    QVERIFY(!lattesDialog.isEmpty());

#if PLASMA_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    QCOMPARE(dialog, lattesDialog);
#else
    QVERIFY(dialog != lattesDialog);
#endif
}

void PackageUnitTest::indicatorPackageLoadsFromPluginAndResolvesDirectories()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QStringList directories{
        QStringLiteral("config"),
        QStringLiteral("ui"),
        QStringLiteral("data"),
        QStringLiteral("code"),
        QStringLiteral("locale")
    };

    for (const QString &directory : directories) {
        QVERIFY(QDir(dir.path()).mkpath(QStringLiteral("contents/") + directory));
    }

    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Latte/Indicator"));
    package.setPath(dir.path());

    QVERIFY(package.filePath("config").endsWith(QStringLiteral("config")));
    QVERIFY(package.filePath("ui").endsWith(QStringLiteral("ui")));
    QVERIFY(package.filePath("data").endsWith(QStringLiteral("data")));
    QVERIFY(package.filePath("scripts").endsWith(QStringLiteral("code")));
    QVERIFY(package.filePath("translations").endsWith(QStringLiteral("locale")));
}

void PackageUnitTest::indicatorPackageResolvesBundledDefaultIndicator()
{
    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(
                                    QStringLiteral("Latte/Indicator"),
                                    QStringLiteral(LATTE_SOURCE_DIR "/indicators/default"));

    QVERIFY(package.isValid());
    QCOMPARE(package.metadata().pluginId(), QStringLiteral("org.kde.latte.default"));
    QCOMPARE(package.metadata().value(QStringLiteral("X-Latte-MainScript")), QStringLiteral("ui/main.qml"));
    QVERIFY(package.filePath("ui", QStringLiteral("main.qml")).endsWith(QStringLiteral("package/ui/main.qml")));
    QVERIFY(package.filePath("config", QStringLiteral("main.xml")).endsWith(QStringLiteral("package/config/main.xml")));
}

void PackageUnitTest::packagePathChangesHandleForeignMetadata()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QFile metadata(dir.filePath(QStringLiteral("metadata.json")));
    QVERIFY(metadata.open(QIODevice::WriteOnly | QIODevice::Text));
    QVERIFY(metadata.write(R"({"KPlugin":{"Id":"org.example.foreign","Name":"Foreign shell"}})") > 0);
    metadata.close();

    KPackage::Package package(new Latte::Package);
    package.setPath(dir.path());
    QCOMPARE(package.metadata().pluginId(), QStringLiteral("org.example.foreign"));

    // Switching back to Latte's own package exercises the branch that clears
    // the fallback package after a foreign package was previously selected.
    package.setPath(QStringLiteral(LATTE_SOURCE_DIR "/shell/package"));
    QCOMPARE(package.metadata().pluginId(), QStringLiteral("org.kde.latte.shell"));
}

QTEST_GUILESS_MAIN(PackageUnitTest)

#include "packageunittest.moc"
