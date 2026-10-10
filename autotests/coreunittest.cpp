/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "environment.h"
#include "extras.h"
#include "quickwindowsystem.h"
#include "tools.h"

#include <KConfigGroup>
#include <KIconLoader>
#include <KIconTheme>
#include <KSharedConfig>

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QImage>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTest>

#include <memory>

class CoreUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void extrasFormatRectsAndEnums();
    void extrasCompareFloatingPointValues();
    void toolsCalculateColorBrightnessAndLumina();
    void toolsCalculatePrimaryColorWeights();
    void environmentExposesConstantsAndVersionEncoding();
    void environmentEncodesVersionBytes();
    void environmentReturnsThemeIconNamesAsSources();
    void environmentDescribesIconAndStringSources();
    void environmentRefreshesWidgetIconPathsAfterThemeChange();
    void quickWindowSystemReportsWaylandCompositing();
    void singletonsCreateExpectedObjects();
};

void CoreUnitTest::initTestCase()
{
    // Offscreen Qt may not inherit the active desktop's icon paths. Allow
    // isolated package builds to provide their Breeze icon directory while
    // retaining the conventional system path for distro builds.
    const QString iconThemePath = qEnvironmentVariable(
        "LATTE_TEST_ICON_THEME_PATH", QStringLiteral("/usr/share/icons"));
    QIcon::setThemeSearchPaths({iconThemePath});
    QIcon::setThemeName(QStringLiteral("breeze"));
}

void CoreUnitTest::extrasFormatRectsAndEnums()
{
    QCOMPARE(qRectToStr(QRect(-1, 2, 30, 40)), QStringLiteral("(-1, 2) 30x40"));
    QCOMPARE(QString::fromLatin1(qEnumToStr(Plasma::Types::LeftEdge)), QStringLiteral("LeftEdge"));
    QCOMPARE(QString::fromLatin1(qEnumToStr(Plasma::Types::Horizontal)), QStringLiteral("Horizontal"));
}

void CoreUnitTest::extrasCompareFloatingPointValues()
{
    QVERIFY(almost_equal(0.1 + 0.2, 0.3, 2));
    QVERIFY(!almost_equal(0.1, 0.2, 2));
}

void CoreUnitTest::toolsCalculateColorBrightnessAndLumina()
{
    Latte::Tools tools;

    QCOMPARE(tools.colorBrightness(QColor(Qt::white)), 255.0f);
    QCOMPARE(tools.colorBrightness(QColor(Qt::black)), 0.0f);
    QCOMPARE(tools.colorLumina(QColor(Qt::white)), 1.0f);
    QCOMPARE(tools.colorLumina(QColor(Qt::black)), 0.0f);
}

void CoreUnitTest::toolsCalculatePrimaryColorWeights()
{
    Latte::Tools tools;

    QVERIFY(qAbs(tools.colorBrightness(QColor(Qt::red)) - 76.245f) < 0.001f);
    QVERIFY(qAbs(tools.colorBrightness(QColor(Qt::green)) - 149.685f) < 0.001f);
    QVERIFY(qAbs(tools.colorBrightness(QColor(Qt::blue)) - 29.07f) < 0.001f);

    QVERIFY(qAbs(tools.colorLumina(QColor(Qt::red)) - 0.2126f) < 0.0001f);
    QVERIFY(qAbs(tools.colorLumina(QColor(Qt::green)) - 0.7152f) < 0.0001f);
    QVERIFY(qAbs(tools.colorLumina(QColor(Qt::blue)) - 0.0722f) < 0.0001f);
}

void CoreUnitTest::environmentExposesConstantsAndVersionEncoding()
{
    Latte::Environment environment;

    QCOMPARE(environment.separatorLength(), 5);
    QCOMPARE(environment.shortDuration(), 40u);
    QCOMPARE(environment.longDuration(), 240u);
    QCOMPARE(environment.iconThemeVersion(), 0u);
    QCOMPARE(environment.makeVersion(6, 10, 3), 0x060a03u);
}

void CoreUnitTest::environmentEncodesVersionBytes()
{
    Latte::Environment environment;

    QCOMPARE(environment.makeVersion(0, 0, 0), 0x000000u);
    QCOMPARE(environment.makeVersion(1, 0, 0), 0x010000u);
    QCOMPARE(environment.makeVersion(1, 2, 0), 0x010200u);
    QCOMPARE(environment.makeVersion(255, 255, 255), 0xffffffu);
}

void CoreUnitTest::environmentReturnsThemeIconNamesAsSources()
{
    Latte::Environment environment;

    const QVariant themedIcon = QVariant::fromValue(QIcon::fromTheme(QStringLiteral("application-x-executable")));
    QCOMPARE(environment.iconSourceForTheme(themedIcon).toString(), QStringLiteral("application-x-executable"));

    const QVariant plainSource(QStringLiteral("file:///tmp/icon.png"));
    QCOMPARE(environment.iconSourceForTheme(plainSource), plainSource);
}

void CoreUnitTest::environmentDescribesIconAndStringSources()
{
    Latte::Environment environment;

    const QString iconDescriptor = environment.iconDescriptor(QVariant::fromValue(QIcon::fromTheme(QStringLiteral("folder"))));
    QVERIFY(iconDescriptor.contains(QStringLiteral("QIcon")));
    QVERIFY(iconDescriptor.contains(QStringLiteral("iconName=\"folder\"")));

    const QString textDescriptor = environment.iconDescriptor(QVariant(QStringLiteral("plain")));
    QVERIFY(textDescriptor.contains(QStringLiteral("QString")));
    QVERIFY(textDescriptor.contains(QStringLiteral("string=\"plain\"")));
}

void
CoreUnitTest::environmentRefreshesWidgetIconPathsAfterThemeChange()
{
    if (QGuiApplication::platformName() == QLatin1String("offscreen") || QGuiApplication::platformName() == QLatin1String("minimal")) {
        QSKIP("The kdeglobals watcher requires a desktop platform; run this case on an isolated desktop session bus");
    }

    QTemporaryDir fixture;
    QVERIFY(fixture.isValid());
    const QByteArray originalConfigHome = qgetenv("XDG_CONFIG_HOME");
    const QByteArray originalDataHome = qgetenv("XDG_DATA_HOME");
    const QString originalTheme = QIcon::themeName();
    const QStringList originalSearchPaths = QIcon::themeSearchPaths();
    const auto restore = qScopeGuard([&]() {
        qputenv("XDG_CONFIG_HOME", originalConfigHome);
        qputenv("XDG_DATA_HOME", originalDataHome);
        QIcon::setThemeName(originalTheme);
        QIcon::setThemeSearchPaths(originalSearchPaths);
        KIconTheme::reconfigure();
        KIconLoader::global()->reconfigure(QString());
    });
    qputenv("XDG_CONFIG_HOME", fixture.filePath(QStringLiteral("config")).toUtf8());
    qputenv("XDG_DATA_HOME", fixture.filePath(QStringLiteral("data")).toUtf8());

    // Both themes expose the same widget icon name. Only its resolved file
    // changes, matching the original-color fallback's production input.
    for (const QString &name : { QStringLiteral("latte-red"), QStringLiteral("latte-blue") }) {
        const QString themePath = fixture.filePath(QStringLiteral("data/icons/") + name);
        QVERIFY(QDir().mkpath(themePath + QStringLiteral("/64x64/apps")));
        QFile index(themePath + QStringLiteral("/index.theme"));
        QVERIFY(index.open(QIODevice::WriteOnly));
        index.write(QStringLiteral("[Icon Theme]\nName=%1\nDirectories=64x64/apps\n"
                                   "[64x64/apps]\nSize=64\nType=Fixed\nContext=Applications\n")
                      .arg(name)
                      .toUtf8());
        index.close();
        QImage icon(QSize(64, 64), QImage::Format_ARGB32_Premultiplied);
        icon.fill(name.endsWith(QLatin1String("red")) ? Qt::red : Qt::blue);
        QVERIFY(icon.save(themePath + QStringLiteral("/64x64/apps/latte-widget-probe.png")));
        icon.fill(Qt::green);
        QVERIFY(icon.save(themePath + QStringLiteral("/64x64/apps/latte-widget-next.png")));
    }
    QIcon::setThemeSearchPaths({ fixture.filePath(QStringLiteral("data/icons")) });
    auto config = KSharedConfig::openConfig(QStringLiteral("kdeglobals"));
    KConfigGroup icons(config, QStringLiteral("Icons"));
    icons.writeEntry("Theme", QStringLiteral("latte-red"));
    QVERIFY(config->sync());
    QIcon::setThemeName(QStringLiteral("latte-red"));
    KIconTheme::reconfigure();
    auto *loader = KIconLoader::global();
    loader->reconfigure(QString());
    QCOMPARE(loader->iconPath(QStringLiteral("latte-widget-probe"), -64, true), fixture.filePath(QStringLiteral("data/icons/latte-red/64x64/apps/latte-widget-probe.png")));

    Latte::Environment environment;
    QQmlEngine engine;
    QQuickWindow window;
    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQuick
        import org.kde.kirigami as Kirigami
        Item {
            width: 64; height: 64
            property string iconName: "latte-widget-probe"
            Item {
                anchors.fill: parent
                Kirigami.Icon {
                    objectName: "nativeWidgetIcon"
                    anchors.fill: parent
                    animated: false
                    source: parent.parent.iconName
                }
            }
        }
    )",
                      QUrl());
    QTRY_VERIFY_WITH_TIMEOUT(component.isReady(), 5000);
    std::unique_ptr<QObject> object(component.create());
    QVERIFY(object);
    auto *root = qobject_cast<QQuickItem *>(object.get());
    QVERIFY(root);
    auto *nativeIcon = root->findChild<QQuickItem *>(QStringLiteral("nativeWidgetIcon"));
    QVERIFY(nativeIcon);
    QSignalSpy sourceChanged(nativeIcon, SIGNAL(sourceChanged()));
    window.resize(64, 64);
    root->setParentItem(window.contentItem());
    window.show();
    QTRY_VERIFY_WITH_TIMEOUT(window.isExposed(), 5000);
    const auto renderedColor = [&window]() {
        const QImage pixels = window.grabWindow();
        return pixels.isNull() ? QColor() : pixels.pixelColor(pixels.width() / 2, pixels.height() / 2);
    };
    QTRY_COMPARE_WITH_TIMEOUT(renderedColor(), QColor(Qt::red), 5000);
    QSignalSpy changed(&environment, &Latte::Environment::iconThemeVersionChanged);
    icons.writeEntry("Theme", QStringLiteral("latte-blue"));
    QVERIFY(config->sync());
    QTRY_VERIFY_WITH_TIMEOUT(!changed.isEmpty(), 5000);
    QCOMPARE(QIcon::themeName(), QStringLiteral("latte-blue"));
    QCOMPARE(loader->iconPath(QStringLiteral("latte-widget-probe"), -64, true), fixture.filePath(QStringLiteral("data/icons/latte-blue/64x64/apps/latte-widget-probe.png")));
    // Reproduce the missing native repaint: changing the theme and resolving
    // a new file is insufficient while the widget's source binding is stable.
    QCOMPARE(renderedColor(), QColor(Qt::red));
    environment.refreshAppletIcons(root);
    QTRY_COMPARE_WITH_TIMEOUT(renderedColor(), QColor(Qt::blue), 5000);
    QCOMPARE(sourceChanged.count(), 0);
    QVERIFY(root->setProperty("iconName", QStringLiteral("latte-widget-next")));
    QTRY_COMPARE_WITH_TIMEOUT(renderedColor(), QColor(Qt::green), 5000);
    QCOMPARE(nativeIcon->property("source").toString(), QStringLiteral("latte-widget-next"));
    root->setParentItem(nullptr);
}

void CoreUnitTest::quickWindowSystemReportsWaylandCompositing()
{
    Latte::QuickWindowSystem windowSystem;

    QVERIFY(windowSystem.compositingActive());
    QVERIFY(windowSystem.isPlatformWayland());
}

void CoreUnitTest::singletonsCreateExpectedObjects()
{
    //! The singletons are declared with QML_SINGLETON; the QML engine simply
    //! default-constructs them, so constructing them directly asserts the
    //! same contract the old singleton provider functions did.
    Latte::Tools tools;
    QVERIFY(qobject_cast<Latte::Tools *>(&tools));

    Latte::Environment environment;
    QVERIFY(qobject_cast<Latte::Environment *>(&environment));

    Latte::QuickWindowSystem windowSystem;
    QVERIFY(qobject_cast<Latte::QuickWindowSystem *>(&windowSystem));
}

QTEST_MAIN(CoreUnitTest)

#include "coreunittest.moc"
