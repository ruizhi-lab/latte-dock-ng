/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "tasktools.h"
#include "windowinfowrap.h"

#include <KConfigGroup>

#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QPixmap>
#include <QTemporaryDir>
#include <QTest>
#include <QTextStream>
#include <QUrlQuery>

using Latte::WindowSystem::AppData;

class WindowSystemUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultApplicationRejectsNonPreferredUrls();
    void defaultApplicationRejectsUnknownPreferredUrl();
    void metadataLookupRequiresRulesConfig();
    void metadataLookupUsesDirectDesktopMapping();
    void processLookupsRejectEmptyInputs();
    void processLookupFallsBackToExecutableWhenConfigured();
    void processLookupCanIgnoreConfiguredRuntimeWrapper();
    void windowInfoWrapCopiesAndAssignsWindowState();
    void windowInfoWrapTracksParentsAndMembership();
    void appDataFromUrlReadsLocalDesktopFile();
    void appDataFromUrlPreservesFallbackForUnknownUrls();
    void appDataFromUrlReadsQueryFlags();
    void appDataFromUrlReadsIconDataQuery();
};

void WindowSystemUnitTest::defaultApplicationRejectsNonPreferredUrls()
{
    QVERIFY(Latte::WindowSystem::defaultApplication(QUrl(QStringLiteral("https://example.test"))).isEmpty());
    QVERIFY(Latte::WindowSystem::defaultApplication(QUrl(QStringLiteral("preferred:"))).isEmpty());
    QVERIFY(Latte::WindowSystem::defaultApplication(QUrl(QStringLiteral("preferred://"))).isEmpty());
}

void WindowSystemUnitTest::defaultApplicationRejectsUnknownPreferredUrl()
{
    QVERIFY(Latte::WindowSystem::defaultApplication(QUrl(QStringLiteral("preferred://latte-does-not-exist"))).isEmpty());
}

void WindowSystemUnitTest::metadataLookupRequiresRulesConfig()
{
    QVERIFY(!Latte::WindowSystem::windowUrlFromMetadata(QStringLiteral("org.kde.foo")).isValid());
    QVERIFY(!Latte::WindowSystem::windowUrlFromMetadata(QString(), 0, KSharedConfig::Ptr(), QStringLiteral("foo")).isValid());
}

void WindowSystemUnitTest::metadataLookupUsesDirectDesktopMapping()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    KSharedConfig::Ptr config = KSharedConfig::openConfig(dir.filePath(QStringLiteral("rulesrc")), KConfig::SimpleConfig);
    KConfigGroup mapping(config, QStringLiteral("Mapping"));
    mapping.writeEntry(QStringLiteral("org.example.App"), QStringLiteral("/tmp/org.example.App.desktop"));
    mapping.writeEntry(QStringLiteral("org.example.Other::other-class"), QStringLiteral("/tmp/org.example.Other.desktop"));
    config->sync();

    QCOMPARE(Latte::WindowSystem::windowUrlFromMetadata(QStringLiteral("org.example.App"), 0, config),
             QUrl(QStringLiteral("/tmp/org.example.App.desktop")));
    QCOMPARE(Latte::WindowSystem::windowUrlFromMetadata(QStringLiteral("org.example.Other"), 0, config, QStringLiteral("other-class")),
             QUrl(QStringLiteral("/tmp/org.example.Other.desktop")));
}

void WindowSystemUnitTest::processLookupsRejectEmptyInputs()
{
    QVERIFY(Latte::WindowSystem::servicesFromPid(0).isEmpty());
    QVERIFY(Latte::WindowSystem::servicesFromCmdLine(QString(), QString()).isEmpty());
    QVERIFY(Latte::WindowSystem::servicesFromCmdLine(QStringLiteral("/usr/bin/foo"), QStringLiteral("foo")).isEmpty());
}

void WindowSystemUnitTest::processLookupFallsBackToExecutableWhenConfigured()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    KSharedConfig::Ptr config = KSharedConfig::openConfig(dir.filePath(QStringLiteral("rulesrc")), KConfig::SimpleConfig);
    const KService::List services = Latte::WindowSystem::servicesFromCmdLine(QStringLiteral("/bin/true"), QStringLiteral("True"), config);

    QCOMPARE(services.count(), 1);
    QCOMPARE(services.constFirst()->name(), QStringLiteral("True"));
    QCOMPARE(services.constFirst()->exec(), QStringLiteral("/bin/true"));
}

void WindowSystemUnitTest::processLookupCanIgnoreConfiguredRuntimeWrapper()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    KSharedConfig::Ptr config = KSharedConfig::openConfig(dir.filePath(QStringLiteral("rulesrc")), KConfig::SimpleConfig);
    KConfigGroup settings(config, QStringLiteral("Settings"));
    settings.writeEntry(QStringLiteral("TryIgnoreRuntimes"), QStringList{QStringLiteral("latte-runtime-wrapper")});
    config->sync();

    const KService::List services = Latte::WindowSystem::servicesFromCmdLine(QStringLiteral("latte-runtime-wrapper /bin/true"),
                                                                             QStringLiteral("True"),
                                                                             config);

    QCOMPARE(services.count(), 1);
    QCOMPARE(services.constFirst()->name(), QStringLiteral("True"));
    QCOMPARE(services.constFirst()->exec(), QStringLiteral("/bin/true"));
}

void WindowSystemUnitTest::windowInfoWrapCopiesAndAssignsWindowState()
{
    Latte::WindowSystem::WindowInfoWrap info;
    info.setWid("window-1");
    info.setParentId("parent-1");
    info.setGeometry(QRect(1, 2, 300, 400));
    info.setIsValid(true);
    info.setIsActive(true);
    info.setIsMinimized(true);
    info.setIsMaxVert(true);
    info.setIsMaxHoriz(true);
    info.setIsFullscreen(true);
    info.setIsShaded(true);
    info.setIsKeepAbove(true);
    info.setIsKeepBelow(true);
    info.setHasSkipPager(true);
    info.setHasSkipSwitcher(true);
    info.setHasSkipTaskbar(true);
    info.setIsClosable(true);
    info.setIsFullScreenable(true);
    info.setIsGroupable(true);
    info.setIsMaximizable(true);
    info.setIsMinimizable(true);
    info.setIsMovable(true);
    info.setIsResizable(true);
    info.setIsShadeable(true);
    info.setIsVirtualDesktopsChangeable(true);
    info.setAppName(QStringLiteral("App"));
    info.setDisplay(QStringLiteral("Window Title"));
    QPixmap pixmap(1, 1);
    pixmap.fill(Qt::blue);
    info.setIcon(QIcon(pixmap));
    info.setDesktops({QStringLiteral("desktop-1")});
    info.setActivities({QStringLiteral("activity-1")});

    const Latte::WindowSystem::WindowInfoWrap copied(info);
    QCOMPARE(copied.wid(), QByteArray("window-1"));
    QCOMPARE(copied.parentId(), QByteArray("parent-1"));
    QCOMPARE(copied.geometry(), QRect(1, 2, 300, 400));
    QVERIFY(copied.isValid());
    QVERIFY(copied.isActive());
    QVERIFY(copied.isMinimized());
    QVERIFY(copied.isMaximized());
    QVERIFY(copied.isFullscreen());
    QVERIFY(copied.isShaded());
    QVERIFY(copied.isKeepAbove());
    QVERIFY(copied.isKeepBelow());
    QVERIFY(copied.hasSkipPager());
    QVERIFY(copied.hasSkipSwitcher());
    QVERIFY(copied.hasSkipTaskbar());
    QVERIFY(copied.isCloseable());
    QVERIFY(copied.isFullScreenable());
    QVERIFY(copied.isGroupable());
    QVERIFY(copied.isMaximizable());
    QVERIFY(copied.isMinimizable());
    QVERIFY(copied.isMovable());
    QVERIFY(copied.isResizable());
    QVERIFY(copied.isShadeable());
    QVERIFY(copied.isVirtualDesktopsChangeable());
    QCOMPARE(copied.appName(), QStringLiteral("App"));
    QCOMPARE(copied.display(), QStringLiteral("Window Title"));
    QVERIFY(!copied.icon().isNull());
    QCOMPARE(copied.desktops(), QStringList({QStringLiteral("desktop-1")}));
    QCOMPARE(copied.activities(), QStringList({QStringLiteral("activity-1")}));

    Latte::WindowSystem::WindowInfoWrap assigned;
    assigned = copied;
    QCOMPARE(assigned.display(), copied.display());
    QCOMPARE(assigned.geometry(), copied.geometry());
}

void WindowSystemUnitTest::windowInfoWrapTracksParentsAndMembership()
{
    Latte::WindowSystem::WindowInfoWrap info;
    info.setWid("window-1");
    QVERIFY(info.isMainWindow());
    QVERIFY(!info.isChildWindow());

    info.setParentId("window-1");
    QVERIFY(info.isMainWindow());
    info.setParentId("parent-1");
    QVERIFY(info.isChildWindow());

    info.setDesktops({QStringLiteral("desktop-1")});
    QVERIFY(info.isOnDesktop(QStringLiteral("desktop-1")));
    QVERIFY(!info.isOnDesktop(QStringLiteral("desktop-2")));
    info.setIsOnAllDesktops(true);
    QVERIFY(info.isOnDesktop(QStringLiteral("desktop-2")));

    info.setActivities({QStringLiteral("activity-1")});
    QVERIFY(info.isOnActivity(QStringLiteral("activity-1")));
    QVERIFY(!info.isOnActivity(QStringLiteral("activity-2")));
    info.setIsOnAllActivities(true);
    QVERIFY(info.isOnActivity(QStringLiteral("activity-2")));
}

void WindowSystemUnitTest::appDataFromUrlReadsLocalDesktopFile()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString desktopPath = dir.filePath(QStringLiteral("sample.desktop"));
    QFile desktopFile(desktopPath);
    QVERIFY(desktopFile.open(QFile::WriteOnly | QFile::Text));
    QTextStream stream(&desktopFile);
    stream << "[Desktop Entry]\n"
           << "Type=Application\n"
           << "Name=Sample App\n"
           << "GenericName=Sample Generic\n"
           << "Exec=/bin/true\n"
           << "Icon=applications-system\n";
    desktopFile.close();

    const AppData data = Latte::WindowSystem::appDataFromUrl(QUrl::fromLocalFile(desktopPath));

    QCOMPARE(data.name, QStringLiteral("Sample App"));
    QCOMPARE(data.genericName, QStringLiteral("Sample Generic"));
    QCOMPARE(data.id, QStringLiteral("sample"));
    QCOMPARE(data.skipTaskbar, false);
}

void WindowSystemUnitTest::appDataFromUrlPreservesFallbackForUnknownUrls()
{
    const QIcon fallback = QIcon::fromTheme(QStringLiteral("application-x-executable"));
    const QUrl url(QStringLiteral("file:///tmp/not-a-desktop-file"));

    const AppData data = Latte::WindowSystem::appDataFromUrl(url, fallback);

    QCOMPARE(data.url, url);
    QCOMPARE(data.name, QStringLiteral("not-a-desktop-file"));
    QVERIFY(data.id.isEmpty());
    QCOMPARE(data.skipTaskbar, false);
}

void WindowSystemUnitTest::appDataFromUrlReadsQueryFlags()
{
    const QUrl url(QStringLiteral("file:///tmp/launcher?skipTaskbar=true"));

    const AppData data = Latte::WindowSystem::appDataFromUrl(url);

    QCOMPARE(data.url, url);
    QCOMPARE(data.skipTaskbar, true);
    QCOMPARE(data.name, QStringLiteral("launcher"));

    const AppData falseData = Latte::WindowSystem::appDataFromUrl(QUrl(QStringLiteral("file:///tmp/launcher?skipTaskbar=false")));
    QCOMPARE(falseData.skipTaskbar, false);
}

void WindowSystemUnitTest::appDataFromUrlReadsIconDataQuery()
{
    QImage image(QSize(1, 1), QImage::Format_ARGB32);
    image.fill(Qt::red);

    QByteArray pngData;
    QBuffer buffer(&pngData);
    QVERIFY(buffer.open(QIODevice::WriteOnly));
    QVERIFY(image.save(&buffer, "PNG"));

    QUrl url(QStringLiteral("file:///tmp/launcher"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("iconData"), QString::fromLatin1(pngData.toBase64(QByteArray::Base64UrlEncoding)));
    url.setQuery(query);

    const AppData data = Latte::WindowSystem::appDataFromUrl(url);

    QVERIFY(!data.icon.isNull());
    QCOMPARE(data.name, QStringLiteral("launcher"));
}

QTEST_MAIN(WindowSystemUnitTest)

#include "wmunittest.moc"
