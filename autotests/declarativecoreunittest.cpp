/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "dialog.h"
#include "iconitem.h"

#include <QSignalSpy>
#include <QGuiApplication>
#include <QDir>
#include <QFile>
#include <QIconEngine>
#include <KIconLoader>
#include <KIconTheme>
#include <QIcon>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTest>
#include <QtMath>

#include <memory>
#include <utility>

class DeclarativeCoreUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void iconItemEmitsOnlyForChangedState();
    void iconItemRepaintsChangedRasterSource();
    void iconItemRepaintsChangedIconSource();
    void iconItemReloadsForEnabledAndActiveState();
    void iconItemReloadsThemedSourcesOnIconThemeChange();
    void iconItemReloadsSvgForColorGroupChange();
    void iconItemClearsAndRestoresRasterAtZeroSize();
    void iconItemRendersAndResizesInSceneGraph();
    void dialogTracksMouseAndEdgeChanges();
};

class TrackingIconItem final : public Latte::IconItem
{
public:
    using Latte::IconItem::IconItem;

    void updatePolish() override
    {
        ++polishCount;
        Latte::IconItem::updatePolish();
    }

    int polishCount{0};
};

class CountingIconEngine final : public QIconEngine
{
public:
    explicit CountingIconEngine(std::shared_ptr<int> rasterRequests)
        : m_rasterRequests(std::move(rasterRequests))
    {
    }

    void paint(QPainter *painter, const QRect &rect, QIcon::Mode, QIcon::State) override
    {
        painter->fillRect(rect, Qt::yellow);
    }

    QIconEngine *clone() const override
    {
        return new CountingIconEngine(m_rasterRequests);
    }

    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override
    {
        ++*m_rasterRequests;
        return QIconEngine::pixmap(size, mode, state);
    }

private:
    std::shared_ptr<int> m_rasterRequests;
};

void DeclarativeCoreUnitTest::iconItemEmitsOnlyForChangedState()
{
    Latte::IconItem item;

    QVERIFY(item.smooth());
    QVERIFY(!item.isActive());
    QVERIFY(!item.providesColors());
    QVERIFY(!item.usesPlasmaTheme());
    QVERIFY(item.overlays().isEmpty());

    QSignalSpy activeSpy(&item, &Latte::IconItem::activeChanged);
    QSignalSpy overlaysSpy(&item, &Latte::IconItem::overlaysChanged);
    QSignalSpy providesSpy(&item, &Latte::IconItem::providesColorsChanged);
    QSignalSpy plasmaThemeSpy(&item, &Latte::IconItem::usesPlasmaThemeChanged);

    item.setActive(true);
    item.setActive(true);
    QCOMPARE(activeSpy.count(), 1);
    QVERIFY(item.isActive());

    item.setOverlays(QStringList{QStringLiteral("emblem-favorite")});
    item.setOverlays(QStringList{QStringLiteral("emblem-favorite")});
    QCOMPARE(overlaysSpy.count(), 1);

    item.setProvidesColors(true);
    item.setProvidesColors(true);
    QCOMPARE(providesSpy.count(), 1);

    item.setUsesPlasmaTheme(true);
    item.setUsesPlasmaTheme(true);
    QCOMPARE(plasmaThemeSpy.count(), 1);
}

void DeclarativeCoreUnitTest::iconItemRepaintsChangedRasterSource()
{
    QImage first(QSize(8, 8), QImage::Format_ARGB32_Premultiplied);
    first.fill(Qt::red);
    QImage replacement(QSize(8, 8), QImage::Format_ARGB32_Premultiplied);
    replacement.fill(Qt::blue);

    Latte::IconItem item;
    item.setWidth(8);
    item.setHeight(8);
    item.setSource(QVariant::fromValue(first));
    item.componentComplete();
    item.updatePolish();

    QVERIFY(!item.m_iconPixmap.isNull());
    QCOMPARE(item.m_iconPixmap.toImage().pixelColor(0, 0), QColor(Qt::red));

    item.setSource(QVariant::fromValue(replacement));
    item.updatePolish();

    QVERIFY(!item.m_iconPixmap.isNull());
    QCOMPARE(item.m_iconPixmap.toImage().pixelColor(0, 0), QColor(Qt::blue));
}

void DeclarativeCoreUnitTest::iconItemRepaintsChangedIconSource()
{
    QImage first(QSize(8, 8), QImage::Format_ARGB32_Premultiplied);
    first.fill(Qt::red);
    QImage replacement(QSize(8, 8), QImage::Format_ARGB32_Premultiplied);
    replacement.fill(Qt::blue);

    Latte::IconItem item;
    item.setWidth(8);
    item.setHeight(8);
    item.setSource(QVariant::fromValue(QIcon(QPixmap::fromImage(first))));
    item.componentComplete();
    item.updatePolish();

    QVERIFY(!item.m_iconPixmap.isNull());
    QCOMPARE(item.m_iconPixmap.toImage().pixelColor(0, 0), QColor(Qt::red));

    item.setSource(QVariant::fromValue(QIcon(QPixmap::fromImage(replacement))));
    item.updatePolish();

    QVERIFY(!item.m_iconPixmap.isNull());
    QCOMPARE(item.m_iconPixmap.toImage().pixelColor(0, 0), QColor(Qt::blue));
}

void DeclarativeCoreUnitTest::iconItemReloadsForEnabledAndActiveState()
{
    QImage image(QSize(32, 32), QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(40, 100, 220));

    Latte::IconItem item;
    item.setWidth(32);
    item.setHeight(32);
    item.setSource(QVariant::fromValue(image));
    item.componentComplete();
    item.updatePolish();
    const QImage normal = item.m_iconPixmap.toImage();

    item.setActive(true);
    item.updatePolish();
    const QImage active = item.m_iconPixmap.toImage();
    QVERIFY(active != normal);

    item.setEnabled(false);
    item.updatePolish();
    QVERIFY(item.m_iconPixmap.toImage() != active);
}

void DeclarativeCoreUnitTest::iconItemReloadsThemedSourcesOnIconThemeChange()
{
    if (QGuiApplication::platformName() == QLatin1String("offscreen")
        || QGuiApplication::platformName() == QLatin1String("minimal")) {
        QSKIP("Icon theme change requires a desktop scene-graph platform");
    }

    QTemporaryDir themeRoot;
    QVERIFY(themeRoot.isValid());
    auto *iconLoader = KIconLoader::global();
    if (!iconLoader->theme() || iconLoader->theme()->name() == QLatin1String("hicolor")) {
        QSKIP("A non-hicolor desktop icon theme is required for theme switching");
    }
    const QString redTheme = iconLoader->theme()->name();
    const QString blueTheme = QStringLiteral("hicolor");

    const auto writeTheme = [&themeRoot](const QString &themeName, const QColor &color) {
        const QString themePath = themeRoot.filePath(themeName);
        QDir root(themePath);
        if (!root.mkpath(QStringLiteral("32x32/apps"))) {
            return false;
        }

        QFile indexFile(root.filePath(QStringLiteral("index.theme")));
        if (!indexFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }
        indexFile.write(QStringLiteral("[Icon Theme]\nName=%1\nComment=IconItem test\nDirectories=32x32/apps\n\n"
                                       "[32x32/apps]\nSize=32\nType=Fixed\nContext=Applications\n")
                            .arg(themeName)
                            .toUtf8());
        indexFile.close();

        QImage iconImage(QSize(32, 32), QImage::Format_ARGB32_Premultiplied);
        iconImage.fill(color);
        QImage overlayImage(QSize(32, 32), QImage::Format_ARGB32_Premultiplied);
        overlayImage.fill(Qt::white);
        return iconImage.save(root.filePath(QStringLiteral("32x32/apps/p2a-theme-probe.png")))
            && overlayImage.save(root.filePath(QStringLiteral("32x32/apps/p2a-overlay-probe.png")));
    };

    QVERIFY(writeTheme(redTheme, Qt::red));
    QVERIFY(writeTheme(blueTheme, Qt::blue));

    const QString originalTheme = QIcon::themeName();
    const QStringList originalSearchPaths = QIcon::themeSearchPaths();
    QIcon::setThemeSearchPaths({themeRoot.path()});
    QIcon::setThemeName(redTheme);

    {
        const auto icon = QIcon::fromTheme(QStringLiteral("p2a-theme-probe"));
        QVERIFY(!icon.isNull());

        QQuickWindow window;
        window.resize(120, 120);

        auto *item = new TrackingIconItem(window.contentItem());
        item->setWidth(48);
        item->setHeight(48);
        item->setProvidesColors(true);
        item->componentComplete();
        item->setSource(QVariant::fromValue(icon));

        QImage plainImage(QSize(16, 16), QImage::Format_ARGB32_Premultiplied);
        plainImage.fill(Qt::green);
        auto *plainItem = new TrackingIconItem(window.contentItem());
        plainItem->setWidth(48);
        plainItem->setHeight(48);
        plainItem->componentComplete();
        plainItem->setSource(QVariant::fromValue(plainImage));

        window.show();
        QTRY_VERIFY_WITH_TIMEOUT(window.isExposed(), 5000);
        QVERIFY(!window.grabWindow().isNull());
        QVERIFY(!item->m_iconPixmap.isNull());
        QVERIFY(!plainItem->m_iconPixmap.isNull());
        QCOMPARE(item->m_iconPixmap.toImage().pixelColor(16, 16), QColor(Qt::red));
        const QColor initialBackgroundColor = item->backgroundColor();
        QVERIFY(initialBackgroundColor.isValid());

        const auto rasterWithoutOverlay = item->m_iconPixmap.toImage();
        const auto previousOverlayPolishCount = item->polishCount;
        item->setOverlays({QStringLiteral("p2a-overlay-probe")});
        QTRY_VERIFY_WITH_TIMEOUT(item->polishCount > previousOverlayPolishCount, 5000);
        QVERIFY(!window.grabWindow().isNull());
        QVERIFY(item->m_iconPixmap.toImage() != rasterWithoutOverlay);

        QSignalSpy iconChangedSpy(iconLoader, &KIconLoader::iconChanged);
        const auto previousPolishCount = item->polishCount;
        const auto previousPlainPolishCount = plainItem->polishCount;
        QIcon::setThemeName(blueTheme);
        Q_EMIT iconLoader->iconChanged(KIconLoader::Desktop);
        QCOMPARE(iconChangedSpy.count(), 1);
        QTRY_VERIFY_WITH_TIMEOUT(item->polishCount > previousPolishCount, 5000);
        QCOMPARE(plainItem->polishCount, previousPlainPolishCount);

        QVERIFY(!window.grabWindow().isNull());
        QCOMPARE(item->m_iconPixmap.toImage().pixelColor(2, 2), QColor(Qt::blue));
        QVERIFY(item->backgroundColor() != initialBackgroundColor);
    }

    QIcon::setThemeName(originalTheme);
    QIcon::setThemeSearchPaths(originalSearchPaths);
}

void DeclarativeCoreUnitTest::iconItemReloadsSvgForColorGroupChange()
{
    if (QGuiApplication::platformName() == QLatin1String("offscreen")
        || QGuiApplication::platformName() == QLatin1String("minimal")) {
        QSKIP("SVG color-group change requires a desktop scene-graph platform");
    }

    const auto icon = QIcon::fromTheme(QStringLiteral("folder"));
    if (icon.isNull()) {
        QSKIP("The current icon theme does not provide the folder icon");
    }

    QQuickWindow window;
    window.resize(120, 120);

    auto *item = new TrackingIconItem(window.contentItem());
    item->setWidth(48);
    item->setHeight(48);
    item->componentComplete();
    item->setSource(QVariant::fromValue(icon));

    if (!item->m_svgIcon) {
        QSKIP("The current folder icon is not backed by an SVG source");
    }

    window.show();
    QTRY_VERIFY_WITH_TIMEOUT(window.isExposed(), 5000);
    QVERIFY(!window.grabWindow().isNull());

    QSignalSpy repaintSpy(item->m_svgIcon.get(), &KSvg::Svg::repaintNeeded);
    const auto previousPolishCount = item->polishCount;
    item->setColorGroup(1);
    QCOMPARE(item->m_svgIcon->colorSet(), KSvg::Svg::Button);
    QTRY_VERIFY_WITH_TIMEOUT(repaintSpy.count() > 0, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(item->polishCount > previousPolishCount, 5000);
    QVERIFY(!window.grabWindow().isNull());
}

void DeclarativeCoreUnitTest::iconItemClearsAndRestoresRasterAtZeroSize()
{
    QImage image(QSize(8, 8), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::green);

    Latte::IconItem item;
    item.setWidth(8);
    item.setHeight(8);
    item.setSource(QVariant::fromValue(image));
    item.componentComplete();
    item.updatePolish();
    QVERIFY(!item.m_iconPixmap.isNull());

    item.setWidth(0);
    item.setHeight(0);
    item.updatePolish();
    QVERIFY(item.m_iconPixmap.isNull());

    item.setWidth(8);
    item.setHeight(8);
    item.updatePolish();
    QVERIFY(!item.m_iconPixmap.isNull());
    QCOMPARE(item.m_iconPixmap.toImage().pixelColor(0, 0), QColor(Qt::green));
}

void DeclarativeCoreUnitTest::iconItemRendersAndResizesInSceneGraph()
{
    if (QGuiApplication::platformName() == QLatin1String("offscreen")
        || QGuiApplication::platformName() == QLatin1String("minimal")) {
        QSKIP("IconItem texture creation requires a scene-graph platform");
    }

    auto rasterRequests = std::make_shared<int>(0);
    const QIcon sourceIcon(new CountingIconEngine(rasterRequests));

    {
        QQuickWindow window;
        window.setTitle(QStringLiteral("Latte IconItem Scene Graph Test"));
        window.resize(160, 120);

        auto *item = new Latte::IconItem(window.contentItem());
        item->setWidth(48);
        item->setHeight(48);
        item->componentComplete();
        item->setSource(QVariant::fromValue(sourceIcon));

        window.show();
        QTRY_VERIFY_WITH_TIMEOUT(window.isExposed(), 5000);

        QVERIFY(!window.grabWindow().isNull());
        const auto minimumRasterSize = qCeil(48 * window.effectiveDevicePixelRatio());
        QVERIFY(item->m_iconPixmap.width() >= minimumRasterSize);
        QVERIFY(item->m_iconPixmap.height() >= minimumRasterSize);
        const auto initialRasterSize = item->m_iconPixmap.size();
        const auto initialRasterDpr = item->m_iconPixmap.devicePixelRatio();
        const auto initialRasterRequests = *rasterRequests;

        item->setWidth(80);
        QVERIFY(!window.grabWindow().isNull());
        QCOMPARE(item->m_iconPixmap.size(), initialRasterSize);
        QCOMPARE(item->m_iconPixmap.devicePixelRatio(), initialRasterDpr);
        QCOMPARE(*rasterRequests, initialRasterRequests);

        item->setHeight(64);
        QVERIFY(!window.grabWindow().isNull());
        QVERIFY(*rasterRequests > initialRasterRequests);
        QVERIFY(item->m_iconPixmap.width() > initialRasterSize.width());
    }

    {
        QQuickWindow replacementWindow;
        replacementWindow.setTitle(QStringLiteral("Latte IconItem Recreated Window Test"));
        replacementWindow.resize(160, 120);

        auto *item = new Latte::IconItem(replacementWindow.contentItem());
        item->setWidth(48);
        item->setHeight(48);
        item->componentComplete();
        item->setSource(QVariant::fromValue(sourceIcon));

        replacementWindow.show();
        QTRY_VERIFY_WITH_TIMEOUT(replacementWindow.isExposed(), 5000);

        QVERIFY(!replacementWindow.grabWindow().isNull());
        const auto minimumRasterSize = qCeil(48 * replacementWindow.effectiveDevicePixelRatio());
        QVERIFY(item->m_iconPixmap.width() >= minimumRasterSize);
        QVERIFY(item->m_iconPixmap.height() >= minimumRasterSize);
    }
}

void DeclarativeCoreUnitTest::dialogTracksMouseAndEdgeChanges()
{
    if (QGuiApplication::platformName() == QLatin1String("offscreen")
        || QGuiApplication::platformName() == QLatin1String("minimal")) {
        QSKIP("PlasmaQuick dialogs require a real desktop platform");
    }

    Latte::Quick::Dialog dialog;
    QCOMPARE(dialog.edge(), Plasma::Types::BottomEdge);
    QVERIFY(!dialog.containsMouse());

    QSignalSpy edgeSpy(&dialog, &Latte::Quick::Dialog::edgeChanged);
    QSignalSpy containsSpy(&dialog, &Latte::Quick::Dialog::containsMouseChanged);

    dialog.setEdge(Plasma::Types::TopEdge);
    dialog.setEdge(Plasma::Types::TopEdge);
    QCOMPARE(dialog.edge(), Plasma::Types::TopEdge);
    QCOMPARE(edgeSpy.count(), 1);

    QEnterEvent enterEvent{QPointF(), QPointF(), QPointF()};
    QCoreApplication::sendEvent(&dialog, &enterEvent);
    QVERIFY(dialog.containsMouse());
    QCOMPARE(containsSpy.count(), 1);

    QEvent leaveEvent(QEvent::Leave);
    QCoreApplication::sendEvent(&dialog, &leaveEvent);
    QVERIFY(!dialog.containsMouse());
    QCOMPARE(containsSpy.count(), 2);

    QEvent hideEvent(QEvent::Hide);
    QCoreApplication::sendEvent(&dialog, &hideEvent);
    QCOMPARE(containsSpy.count(), 2);
}

QTEST_MAIN(DeclarativeCoreUnitTest)

#include "declarativecoreunittest.moc"
