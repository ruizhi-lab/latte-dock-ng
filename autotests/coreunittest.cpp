/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "environment.h"
#include "extras.h"
#include "quickwindowsystem.h"
#include "tools.h"

#include <QIcon>
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
    void quickWindowSystemReportsWaylandCompositing();
    void singletonsCreateExpectedObjects();
};

void CoreUnitTest::initTestCase()
{
    // Offscreen Qt starts with only its resource icon path. Add the standard
    // system theme directory and select Breeze so theme-name assertions are
    // independent of the developer's active KDE session.
    QIcon::setThemeSearchPaths({QStringLiteral("/usr/share/icons")});
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
