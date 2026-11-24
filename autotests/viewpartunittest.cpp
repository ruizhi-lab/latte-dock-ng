/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "appletinsertion.h"
#include "indicatorinfo.h"
#include "indicatorresources.h"
#include "tasksmodel.h"

#include <QSignalSpy>
#include <QTest>

class ViewPartUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void widgetInsertionKeepsBoundarySeparatorsAdjacent();
    void tasksModelIgnoresNullTasks();
    void tasksModelExposesTaskRoleOnlyForValidRows();
    void indicatorInfoEmitsOnlyWhenValuesChange();
    void indicatorResourcesIgnoreRepeatedSvgPaths();
};

void ViewPartUnitTest::widgetInsertionKeepsBoundarySeparatorsAdjacent()
{
    const QHash<int, QString> plugins{
        {2, QString::fromLatin1(Latte::PluginId::kSeparator)},
        {3, QString::fromLatin1(Latte::PluginId::kPlasmoid)},
        {4, QString::fromLatin1(Latte::PluginId::kLegacySeparator)},
        {6, QString::fromLatin1(Latte::PluginId::kSystemTray)},
        {7, QString::fromLatin1(Latte::PluginId::kNomadSystemTray)},
    };
    const QList<int> order{1, 2, 3, 4, 5};

    QCOMPARE(Latte::ViewPart::widgetInsertionIndex(order, plugins, -1), 1);
    QCOMPARE(Latte::ViewPart::widgetInsertionIndex(order, plugins, 2), 1);
    QCOMPARE(Latte::ViewPart::widgetInsertionIndex(order, plugins, 3), 4);
    QCOMPARE(Latte::ViewPart::widgetInsertionIndex(order, plugins, 0), 0);
    QCOMPARE(Latte::ViewPart::widgetInsertionIndex({1, 3, 5}, plugins, -1), 1);
    QCOMPARE(Latte::ViewPart::widgetInsertionIndex({1, 5}, plugins, -1), 2);
    QCOMPARE(Latte::ViewPart::widgetInsertionIndex({1, 2, 6}, plugins, -1), 1);
    QCOMPARE(Latte::ViewPart::widgetInsertionIndex({1, 2, 7}, plugins, -1), 1);
}

void ViewPartUnitTest::tasksModelIgnoresNullTasks()
{
    Latte::ViewPart::TasksModel model;
    QSignalSpy countSpy(&model, &Latte::ViewPart::TasksModel::countChanged);

    model.addTask(nullptr);
    model.removeTask(nullptr);

    QCOMPARE(model.count(), 0);
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(countSpy.count(), 0);
}

void ViewPartUnitTest::tasksModelExposesTaskRoleOnlyForValidRows()
{
    Latte::ViewPart::TasksModel model;

    QCOMPARE(model.roleNames().value(Qt::UserRole), QByteArray("tasks"));
    QVERIFY(!model.data(model.index(-1, 0), Qt::UserRole).isValid());
    QVERIFY(!model.data(model.index(0, 0), Qt::UserRole).isValid());
    QVERIFY(!model.data(model.index(0, 0), Qt::DisplayRole).isValid());
}

void ViewPartUnitTest::indicatorInfoEmitsOnlyWhenValuesChange()
{
    Latte::ViewPart::IndicatorPart::Info info(nullptr);

    QSignalSpy iconColorsSpy(&info, &Latte::ViewPart::IndicatorPart::Info::needsIconColorsChanged);
    QSignalSpy mouseCoordinatesSpy(&info, &Latte::ViewPart::IndicatorPart::Info::needsMouseEventCoordinatesChanged);
    QSignalSpy clickedAnimationSpy(&info, &Latte::ViewPart::IndicatorPart::Info::providesClickedAnimationChanged);
    QSignalSpy hoveredAnimationSpy(&info, &Latte::ViewPart::IndicatorPart::Info::providesHoveredAnimationChanged);
    QSignalSpy attentionAnimationSpy(&info, &Latte::ViewPart::IndicatorPart::Info::providesInAttentionAnimationChanged);
    QSignalSpy launcherAnimationSpy(&info, &Latte::ViewPart::IndicatorPart::Info::providesTaskLauncherAnimationChanged);
    QSignalSpy groupedAddedSpy(&info, &Latte::ViewPart::IndicatorPart::Info::providesGroupedWindowAddedAnimationChanged);
    QSignalSpy groupedRemovedSpy(&info, &Latte::ViewPart::IndicatorPart::Info::providesGroupedWindowRemovedAnimationChanged);
    QSignalSpy frontLayerSpy(&info, &Latte::ViewPart::IndicatorPart::Info::providesFrontLayerChanged);
    QSignalSpy maskThicknessSpy(&info, &Latte::ViewPart::IndicatorPart::Info::extraMaskThicknessChanged);
    QSignalSpy lengthPaddingSpy(&info, &Latte::ViewPart::IndicatorPart::Info::minLengthPaddingChanged);
    QSignalSpy thicknessPaddingSpy(&info, &Latte::ViewPart::IndicatorPart::Info::minThicknessPaddingChanged);

    info.setNeedsIconColors(true);
    info.setNeedsIconColors(true);
    info.setNeedsMouseEventCoordinates(true);
    info.setNeedsMouseEventCoordinates(true);
    info.setProvidesClickedAnimation(true);
    info.setProvidesClickedAnimation(true);
    info.setProvidesHoveredAnimation(true);
    info.setProvidesHoveredAnimation(true);
    info.setProvidesInAttentionAnimation(true);
    info.setProvidesInAttentionAnimation(true);
    info.setProvidesTaskLauncherAnimation(true);
    info.setProvidesTaskLauncherAnimation(true);
    info.setProvidesGroupedWindowAddedAnimation(true);
    info.setProvidesGroupedWindowAddedAnimation(true);
    info.setProvidesGroupedWindowRemovedAnimation(true);
    info.setProvidesGroupedWindowRemovedAnimation(true);
    info.setProvidesFrontLayer(true);
    info.setProvidesFrontLayer(true);
    info.setExtraMaskThickness(3);
    info.setExtraMaskThickness(3);
    info.setMinLengthPadding(1.25f);
    info.setMinLengthPadding(1.25f);
    info.setMinThicknessPadding(2.5f);
    info.setMinThicknessPadding(2.5f);

    QVERIFY(info.needsIconColors());
    QVERIFY(info.needsMouseEventCoordinates());
    QVERIFY(info.providesClickedAnimation());
    QVERIFY(info.providesHoveredAnimation());
    QVERIFY(info.providesInAttentionAnimation());
    QVERIFY(info.providesTaskLauncherAnimation());
    QVERIFY(info.providesGroupedWindowAddedAnimation());
    QVERIFY(info.providesGroupedWindowRemovedAnimation());
    QVERIFY(info.providesFrontLayer());
    QCOMPARE(info.extraMaskThickness(), 3);
    QCOMPARE(info.minLengthPadding(), 1.25f);
    QCOMPARE(info.minThicknessPadding(), 2.5f);

    QCOMPARE(iconColorsSpy.count(), 1);
    QCOMPARE(mouseCoordinatesSpy.count(), 1);
    QCOMPARE(clickedAnimationSpy.count(), 1);
    QCOMPARE(hoveredAnimationSpy.count(), 1);
    QCOMPARE(attentionAnimationSpy.count(), 1);
    QCOMPARE(launcherAnimationSpy.count(), 1);
    QCOMPARE(groupedAddedSpy.count(), 1);
    QCOMPARE(groupedRemovedSpy.count(), 1);
    QCOMPARE(frontLayerSpy.count(), 1);
    QCOMPARE(maskThicknessSpy.count(), 1);
    QCOMPARE(lengthPaddingSpy.count(), 1);
    QCOMPARE(thicknessPaddingSpy.count(), 1);
}

void ViewPartUnitTest::indicatorResourcesIgnoreRepeatedSvgPaths()
{
    Latte::ViewPart::IndicatorPart::Resources resources(nullptr);
    QSignalSpy svgsSpy(&resources, &Latte::ViewPart::IndicatorPart::Resources::svgsChanged);

    const QStringList paths{QStringLiteral("widgets/panel-background")};
    resources.setSvgImagePaths(paths);

    QCOMPARE(resources.svgs().count(), 1);
    QCOMPARE(svgsSpy.count(), 1);
    QObject *firstSvg = resources.svgs().constFirst();

    resources.setSvgImagePaths(paths);

    QCOMPARE(resources.svgs().count(), 1);
    QCOMPARE(resources.svgs().constFirst(), firstSvg);
    QCOMPARE(svgsSpy.count(), 1);
}

QTEST_GUILESS_MAIN(ViewPartUnitTest)

#include "viewpartunittest.moc"
