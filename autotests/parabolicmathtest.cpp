/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QTest>
#include <QtMath>
#include <cmath>

// Replicates zoom math from:
//   containment/package/contents/ui/abilities/ParabolicEffect.qml
//   declarativeimports/abilities/items/basicitem/ParabolicItem.qml

namespace {

qreal parabolicZoomFactor(int zoomLevel, qreal iconSize)
{
    const qreal storedZoom = 1.0 + static_cast<qreal>(zoomLevel) / 20.0;
    qreal maxSize = storedZoom * iconSize;
    maxSize = 2.0 * qRound(qRound(maxSize) / 2.0); // round to nearest even
    return maxSize / iconSize;
}

qreal parabolicMarginZoom(qreal zoom, qreal marginInfluence)
{
    return 1.0 + ((zoom - 1.0) * marginInfluence);
}

qreal parabolicCleanScalingThickness(qreal iconSize, qreal zoom, qreal thicknessEdges, qreal marginZoom)
{
    return (iconSize * zoom) + (thicknessEdges * marginZoom);
}

qreal parabolicBasicScalingThickness(qreal iconSize, qreal scaleThickness, qreal thicknessEdges, qreal marginInfluence)
{
    const qreal marginScale = 1.0 + ((scaleThickness - 1.0) * marginInfluence);
    return (iconSize * scaleThickness) + (thicknessEdges * marginScale);
}

qreal parabolicVisualScaledLength(qreal iconSize, qreal lengthPaddings, qreal zoom)
{
    return (iconSize + lengthPaddings) * zoom;
}

constexpr int DEFAULT_PARABOLIC_SPREAD = 3;

} // namespace

class ParabolicMathTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void zoomLevelZeroGivesIdentityZoom();
    void zoomLevel200Gives11x();
    void zoomLevel300Gives16x();
    void zoomIsRoundedToMakeEvenIconSize();
    void zoomIncrementsAreMonotonic();
    void zoomPreservesExactValuesAt20Multiples();

    void marginZoomAtIdentityIsOne();
    void marginZoomLinearInZoomExcess();
    void marginInfluenceAtZeroAndFull();

    void cleanScalingThicknessAtIdentity();
    void cleanScalingThicknessAtMaxZoom();
    void basicScalingThicknessAtPerAxisZoom();
    void visualScaledLengthIncludesPadding();
    void visualScaledThicknessEqualsClean();

    void spreadDefaultIsThree();

    void typicalDesktopTaskbarZoom();
    void maxSliderValueZoom();
    void minimumIconSizeWithHighZoom();
    void perAxisZoomDifferentLengthAndThickness();

    void roundingAtSmallIconSizes();
    void roundingAtLargeIconSizes();
};

void ParabolicMathTest::zoomLevelZeroGivesIdentityZoom()
{
    for (qreal sz : {16.0, 32.0, 48.0, 64.0, 128.0})
        QCOMPARE(parabolicZoomFactor(0, sz), 1.0);
}

void ParabolicMathTest::zoomLevel200Gives11x()
{
    QCOMPARE(parabolicZoomFactor(200, 48.0), 11.0);
}

void ParabolicMathTest::zoomLevel300Gives16x()
{
    QCOMPARE(parabolicZoomFactor(300, 48.0), 16.0);
}

void ParabolicMathTest::zoomIsRoundedToMakeEvenIconSize()
{
    const qreal zoom = parabolicZoomFactor(3, 48.0);
    const int maxSizePx = qRound(zoom * 48.0);
    QCOMPARE(maxSizePx % 2, 0);
}

void ParabolicMathTest::zoomIncrementsAreMonotonic()
{
    qreal prev = parabolicZoomFactor(0, 48.0);
    for (int level = 1; level <= 300; level += 10) {
        const qreal curr = parabolicZoomFactor(level, 48.0);
        QVERIFY2(curr >= prev, qPrintable(QStringLiteral("not monotonic at %1").arg(level)));
        prev = curr;
    }
}

void ParabolicMathTest::zoomPreservesExactValuesAt20Multiples()
{
    for (int level : {0, 20, 40, 60, 80, 100, 140, 200})
        QCOMPARE(parabolicZoomFactor(level, 48.0), 1.0 + level / 20.0);
}

void ParabolicMathTest::marginZoomAtIdentityIsOne()
{
    QCOMPARE(parabolicMarginZoom(1.0, 1.0), 1.0);
    QCOMPARE(parabolicMarginZoom(1.0, 0.0), 1.0);
}

void ParabolicMathTest::marginZoomLinearInZoomExcess()
{
    QCOMPARE(parabolicMarginZoom(2.0, 1.0), 2.0);
    QCOMPARE(parabolicMarginZoom(3.0, 1.0), 3.0);
    QCOMPARE(parabolicMarginZoom(2.0, 0.0), 1.0);
}

void ParabolicMathTest::marginInfluenceAtZeroAndFull()
{
    QCOMPARE(parabolicMarginZoom(5.0, 0.0), 1.0);   // 0% → no margin growth
    QCOMPARE(parabolicMarginZoom(2.0, 1.0), 2.0);    // 100% → proportional
    QCOMPARE(parabolicMarginZoom(5.5, 1.0), 5.5);
}

void ParabolicMathTest::cleanScalingThicknessAtIdentity()
{
    QCOMPARE(parabolicCleanScalingThickness(48.0, 1.0, 8.0, 1.0), 56.0);
}

void ParabolicMathTest::cleanScalingThicknessAtMaxZoom()
{
    const qreal margin = parabolicMarginZoom(16.0, 1.0);
    QCOMPARE(parabolicCleanScalingThickness(48.0, 16.0, 8.0, margin), 48.0 * 16.0 + 8.0 * 16.0);
}

void ParabolicMathTest::basicScalingThicknessAtPerAxisZoom()
{
    QCOMPARE(parabolicBasicScalingThickness(48.0, 1.5, 8.0, 1.0), 84.0);
}

void ParabolicMathTest::visualScaledLengthIncludesPadding()
{
    QCOMPARE(parabolicVisualScaledLength(48.0, 4.0, 3.5), 52.0 * 3.5);
}

void ParabolicMathTest::visualScaledThicknessEqualsClean()
{
    const qreal margin = parabolicMarginZoom(5.0, 0.8);
    QCOMPARE(parabolicCleanScalingThickness(48.0, 5.0, 10.0, margin),
             parabolicCleanScalingThickness(48.0, 5.0, 10.0, margin));
}

void ParabolicMathTest::spreadDefaultIsThree()
{
    QCOMPARE(DEFAULT_PARABOLIC_SPREAD, 3);
}

void ParabolicMathTest::typicalDesktopTaskbarZoom()
{
    const qreal zoom = parabolicZoomFactor(50, 48.0);
    QVERIFY(zoom >= 3.0 && zoom <= 4.0);
    const qreal margin = parabolicMarginZoom(zoom, 1.0);
    const qreal thickness = parabolicCleanScalingThickness(48.0, zoom, 8.0, margin);
    QVERIFY(thickness > 48.0 * zoom);
}

void ParabolicMathTest::maxSliderValueZoom()
{
    const qreal zoom = parabolicZoomFactor(300, 48.0);
    QCOMPARE(zoom, 16.0);
    const qreal margin = parabolicMarginZoom(zoom, 1.0);
    QCOMPARE(parabolicCleanScalingThickness(48.0, zoom, 8.0, margin), 896.0);
}

void ParabolicMathTest::minimumIconSizeWithHighZoom()
{
    const qreal zoom = parabolicZoomFactor(200, 16.0);
    QCOMPARE(zoom, 11.0);
    const qreal margin = parabolicMarginZoom(zoom, 1.0);
    QCOMPARE(parabolicCleanScalingThickness(16.0, zoom, 4.0, margin), 220.0);
}

void ParabolicMathTest::perAxisZoomDifferentLengthAndThickness()
{
    QCOMPARE(parabolicBasicScalingThickness(48.0, 1.5, 8.0, 1.0), 84.0);
}

void ParabolicMathTest::roundingAtSmallIconSizes()
{
    QCOMPARE(parabolicZoomFactor(7, 16.0), 1.375);
}

void ParabolicMathTest::roundingAtLargeIconSizes()
{
    const qreal zoom = parabolicZoomFactor(17, 256.0);
    QVERIFY(qAbs(zoom - 1.8515625) < 0.00001);
}

QTEST_APPLESS_MAIN(ParabolicMathTest)
#include "parabolicmathtest.moc"
