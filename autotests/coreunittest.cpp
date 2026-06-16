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
    void extrasFormatRectsAndEnums();
    void extrasCompareFloatingPointValues();
    void toolsCalculateColorBrightnessAndLumina();
    void environmentExposesConstantsAndVersionEncoding();
    void environmentReturnsThemeIconNamesAsSources();
    void environmentDescribesIconAndStringSources();
    void quickWindowSystemReportsWaylandCompositing();
    void singletonProvidersCreateExpectedObjects();
};

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

void CoreUnitTest::environmentExposesConstantsAndVersionEncoding()
{
    Latte::Environment environment;

    QCOMPARE(environment.separatorLength(), 5);
    QCOMPARE(environment.shortDuration(), 40u);
    QCOMPARE(environment.longDuration(), 240u);
    QCOMPARE(environment.iconThemeVersion(), 0u);
    QCOMPARE(environment.makeVersion(6, 10, 3), 0x060a03u);
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

void CoreUnitTest::singletonProvidersCreateExpectedObjects()
{
    std::unique_ptr<QObject> tools(Latte::tools_qobject_singletontype_provider(nullptr, nullptr));
    QVERIFY(qobject_cast<Latte::Tools *>(tools.get()));

    std::unique_ptr<QObject> environment(Latte::environment_qobject_singletontype_provider(nullptr, nullptr));
    QVERIFY(qobject_cast<Latte::Environment *>(environment.get()));

    std::unique_ptr<QObject> windowSystem(Latte::windowsystem_qobject_singletontype_provider(nullptr, nullptr));
    QVERIFY(qobject_cast<Latte::QuickWindowSystem *>(windowSystem.get()));
}

QTEST_MAIN(CoreUnitTest)

#include "coreunittest.moc"
