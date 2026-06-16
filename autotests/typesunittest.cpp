/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "containment/plugin/types.h"
#include "plasmoid/plugin/types.h"

#include <QMetaEnum>
#include <QTest>

class TypesUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void containmentEnumsExposeExpectedKeys();
    void taskEnumsExposeExpectedKeys();
};

void TypesUnitTest::containmentEnumsExposeExpectedKeys()
{
    const QMetaEnum scrollAction = QMetaEnum::fromType<Latte::Containment::Types::ScrollAction>();
    QCOMPARE(QString::fromLatin1(scrollAction.valueToKey(Latte::Containment::Types::ScrollNone)), QStringLiteral("ScrollNone"));
    QCOMPARE(QString::fromLatin1(scrollAction.valueToKey(Latte::Containment::Types::ScrollToggleMinimized)),
             QStringLiteral("ScrollToggleMinimized"));

    const QMetaEnum themeColors = QMetaEnum::fromType<Latte::Containment::Types::ThemeColorsGroup>();
    QCOMPARE(QString::fromLatin1(themeColors.valueToKey(Latte::Containment::Types::LayoutThemeColors)), QStringLiteral("LayoutThemeColors"));

    const QMetaEnum windowColors = QMetaEnum::fromType<Latte::Containment::Types::WindowColorsGroup>();
    QCOMPARE(QString::fromLatin1(windowColors.valueToKey(Latte::Containment::Types::TouchingWindowColors)),
             QStringLiteral("TouchingWindowColors"));
}

void TypesUnitTest::taskEnumsExposeExpectedKeys()
{
    const QMetaEnum modifier = QMetaEnum::fromType<Latte::Tasks::Types::Modifier>();
    QCOMPARE(QString::fromLatin1(modifier.valueToKey(Latte::Tasks::Types::Meta)), QStringLiteral("Meta"));

    const QMetaEnum taskAction = QMetaEnum::fromType<Latte::Tasks::Types::TaskAction>();
    QCOMPARE(QString::fromLatin1(taskAction.valueToKey(Latte::Tasks::Types::PreviewAndHighlightWindows)),
             QStringLiteral("PreviewAndHighlightWindows"));

    const QMetaEnum manualScroll = QMetaEnum::fromType<Latte::Tasks::Types::ManualScrollType>();
    QCOMPARE(QString::fromLatin1(manualScroll.valueToKey(Latte::Tasks::Types::ManualScrollVerticalHorizontal)),
             QStringLiteral("ManualScrollVerticalHorizontal"));
}

QTEST_GUILESS_MAIN(TypesUnitTest)

#include "typesunittest.moc"
