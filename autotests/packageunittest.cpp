/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "lattepackage.h"

#include <KPackage/Package>
#include <KPackage/PackageLoader>

#include <Plasma/version.h>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

namespace {

// Sets an environment variable while the object lives and then restores the
// previous value or its absence, so one case's XDG redirection cannot leak into
// the cases that run after it.
class EnvironmentOverride
{
  public:
    EnvironmentOverride(const char *name, const QString &value)
      : m_name(name)
      , m_wasSet(qEnvironmentVariableIsSet(name))
      , m_previous(qgetenv(name))
    { qputenv(m_name, value.toLocal8Bit()); }

    ~EnvironmentOverride()
    {
        if (m_wasSet) {
            qputenv(m_name, m_previous);
        } else {
            qunsetenv(m_name);
        }
    }

    EnvironmentOverride(const EnvironmentOverride &) = delete;
    EnvironmentOverride &operator=(const EnvironmentOverride &) = delete;

  private:
    const char *const m_name;
    const bool m_wasSet;
    const QByteArray m_previous;
};

}

class PackageUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void lattePackageResolvesShellDefinitions();
    void appletAlternativesDialogMatchesPlasmaRelease();
    void indicatorPackageLoadsFromPluginAndResolvesDirectories();
    void indicatorPackageResolvesBundledDefaultIndicator();
    void pathChangeGivesForeignShellsTheLatteFallback();
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

void
PackageUnitTest::pathChangeGivesForeignShellsTheLatteFallback()
{
    // Package::pathChanged() gives every shell package other than Latte's own
    // the org.kde.latte.shell package as fallback, so a third-party shell needs
    // only the files it changes, and clears the fallback when the package points
    // at Latte's shell again. Production builds the package once (Corona), and
    // initPackage() sets the org.kde.plasma.desktop fallback after its own
    // setPath(), so the clearing only shows when a package is re-pointed, as here.
    //
    // The fallback comes from PackageLoader::loadPackage("Plasma/Shell", ...),
    // whose structure is a libplasma plugin. CTest sets QT_PLUGIN_PATH to the
    // build's bin directory only, so the structure is found only where
    // libplasma's plugins are in Qt's own plugin directory. The Nix check phase
    // keeps them elsewhere, and the case skips there instead of failing.
    if (!KPackage::PackageLoader::self()->loadPackageStructure(QStringLiteral("Plasma/Shell"))) {
        QSKIP("libplasma's Plasma/Shell package structure is not on the plugin path");
    }

    // The fallback package is looked up in the XDG data directories. Point them
    // at this source tree alone, so that an installed Latte (a distribution
    // package or a user-mode install) cannot answer for org.kde.latte.shell.
    QTemporaryDir xdg;
    QVERIFY(xdg.isValid());
    QVERIFY(QDir(xdg.path()).mkpath(QStringLiteral("data/plasma/shells")));
    QVERIFY(QDir(xdg.path()).mkpath(QStringLiteral("home")));
    QVERIFY(QFile::link(QStringLiteral(LATTE_SOURCE_DIR "/shell/package"), xdg.filePath(QStringLiteral("data/plasma/shells/org.kde.latte.shell"))));
    const EnvironmentOverride dataDirs("XDG_DATA_DIRS", xdg.filePath(QStringLiteral("data")));
    const EnvironmentOverride dataHome("XDG_DATA_HOME", xdg.filePath(QStringLiteral("home")));

    const QString latteShell = QFileInfo(QStringLiteral(LATTE_SOURCE_DIR "/shell/package")).canonicalFilePath();
    QVERIFY(!latteShell.isEmpty());

    QTemporaryDir foreign;
    QVERIFY(foreign.isValid());
    QFile metadata(foreign.filePath(QStringLiteral("metadata.json")));
    QVERIFY(metadata.open(QIODevice::WriteOnly | QIODevice::Text));
    QVERIFY(metadata.write(R"({"KPlugin":{"Id":"org.example.foreign","Name":"Foreign shell"}})") > 0);
    metadata.close();

    KPackage::Package package(new Latte::Package);
    package.setPath(foreign.path());
    QCOMPARE(package.metadata().pluginId(), QStringLiteral("org.example.foreign"));

    const KPackage::Package fallback = package.fallbackPackage();
    QVERIFY(fallback.isValid());
    QCOMPARE(fallback.metadata().pluginId(), QStringLiteral("org.kde.latte.shell"));
    QCOMPARE(QFileInfo(fallback.path()).canonicalFilePath(), latteShell);

    package.setPath(QStringLiteral(LATTE_SOURCE_DIR "/shell/package"));
    QCOMPARE(package.metadata().pluginId(), QStringLiteral("org.kde.latte.shell"));
    QVERIFY(!package.fallbackPackage().isValid());
}

QTEST_GUILESS_MAIN(PackageUnitTest)

#include "packageunittest.moc"
