/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

//! Drives the production IndicatorConfigUiManager.qml, which places the
//! selected indicator's options into the Effects page StackView, through
//! sequences of indicator switches. Indicators without X-Latte-ConfigUi used
//! to keep the previous indicator's options on screen and, after switching
//! back, left the options area blank for good, or showed the options again
//! fully transparent once the area had collapsed.

#include <QHash>
#include <QPointer>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QTest>

#include <memory>

namespace {

const QString kLatte = QStringLiteral("org.kde.latte.default");
const QString kWithOptions = QStringLiteral("org.example.indicator.withoptions");
const QString kWithoutOptions = QStringLiteral("org.example.indicator.withoutoptions");

//! Distinct option page heights prove that the options area follows the
//! selected indicator instead of keeping a previous indicator's size.
const QHash<QString, qreal> kOptionHeights = {
    {kLatte, 120},
    {kWithOptions, 64},
};

}

//! Latte::ViewPart::Indicator, reduced to what the options manager reads.
class IndicatorStub : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString type READ type NOTIFY pluginChanged)

public:
    QString type() const
    {
        return m_type;
    }

    void setType(const QString &type)
    {
        if (m_type == type) {
            return;
        }

        m_type = type;
        Q_EMIT pluginChanged();
    }

Q_SIGNALS:
    void pluginChanged();

private:
    QString m_type{kLatte};
};

class LatteViewStub : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *indicator READ indicator CONSTANT)

public:
    QObject *indicator()
    {
        return &m_indicator;
    }

    IndicatorStub m_indicator;
};

//! Follows the contract of Config::IndicatorUiManager (indicatoruimanager.cpp)
//! that the QML relies on: every ui() request re-parents the requested type's
//! cached options item into the item given to setParentItem(), even when that
//! item is already shown, provides nothing for types without a config UI and
//! leaves showing the page to the QML. index() is the position inside the
//! cache, ordered like the indicators.
class IndicatorUiManagerStub : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE void setParentItem(QQuickItem *parentItem)
    {
        m_parentItem = parentItem;
    }

    Q_INVOKABLE int index(const QString &type) const
    {
        QStringList cached;

        for (const QString &candidate : {kLatte, kWithOptions}) {
            if (m_items.contains(candidate)) {
                cached << candidate;
            }
        }

        return cached.indexOf(type);
    }

    Q_INVOKABLE void ui(const QString &type, QObject *view)
    {
        Q_UNUSED(view)

        if (!m_parentItem || !kOptionHeights.contains(type)) {
            return;
        }

        if (!m_items.contains(type)) {
            auto *item = new QQuickItem;
            item->setParent(this);
            item->setObjectName(type);
            item->setImplicitWidth(200);
            item->setImplicitHeight(kOptionHeights.value(type));
            m_items.insert(type, item);
        }

        m_items.value(type)->setParentItem(m_parentItem);
    }

    QQuickItem *optionsItem(const QString &type) const
    {
        return m_items.value(type);
    }

    QList<QQuickItem *> optionsItems() const
    {
        return m_items.values();
    }

private:
    QPointer<QQuickItem> m_parentItem;
    QHash<QString, QQuickItem *> m_items;
};

class ViewConfigStub : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isReady READ isReady NOTIFY isReadyChanged)
    Q_PROPERTY(QObject *indicatorUiManager READ indicatorUiManager CONSTANT)

public:
    bool isReady() const
    {
        return m_isReady;
    }

    void setIsReady(bool ready)
    {
        if (m_isReady == ready) {
            return;
        }

        m_isReady = ready;
        Q_EMIT isReadyChanged();
    }

    QObject *indicatorUiManager()
    {
        return &m_manager;
    }

    IndicatorUiManagerStub m_manager;

Q_SIGNALS:
    void isReadyChanged();

private:
    bool m_isReady{true};
};

//! Mirrors the parts of EffectsConfig.qml that host the manager: the tab bar
//! it selects by id and the StackView with its replace transitions.
static const QByteArray kHarness = QByteArrayLiteral(R"(
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import "../shell/package/contents/controls" as LatteExtraControls

Item {
    id: root
    width: 420
    height: 480

    property int transitionDuration: 0
    property string selectedTab: ""
    readonly property Item stackView: indicatorsStackView

    QtObject {
        id: tabBar
        function selectTab(type) {
            root.selectedTab = type;
        }
    }

    QtObject {
        id: dialogStub
        property int optionsWidth: 360
    }

    ColumnLayout {
        width: parent.width

        QQC2.StackView {
            id: indicatorsStackView
            Layout.fillWidth: true
            Layout.maximumHeight: Layout.minimumHeight

            property bool forwardSliding: true

            replaceEnter: Transition {
                ParallelAnimation {
                    PropertyAnimation {
                        property: "x"
                        from: indicatorsStackView.forwardSliding ? -indicatorsStackView.width : indicatorsStackView.width
                        to: 0
                        duration: root.transitionDuration
                    }
                    PropertyAnimation {
                        property: "opacity"
                        from: 0
                        to: 1
                        duration: root.transitionDuration
                    }
                }
            }

            replaceExit: Transition {
                ParallelAnimation {
                    PropertyAnimation {
                        property: "x"
                        from: 0
                        to: indicatorsStackView.forwardSliding ? indicatorsStackView.width : -indicatorsStackView.width
                        duration: root.transitionDuration
                    }
                    PropertyAnimation {
                        property: "opacity"
                        from: 1
                        to: 0
                        duration: root.transitionDuration
                    }
                }
            }
        }
    }

    LatteExtraControls.IndicatorConfigUiManager {
        visible: false
        dialog: dialogStub
        latteView: testLatteView
        viewConfig: testViewConfig
        stackView: indicatorsStackView
    }
}
)");

class IndicatorConfigUiManagerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void showsTheSelectedIndicatorOptions_data();
    void showsTheSelectedIndicatorOptions();
    void rerequestKeepsTheShownOptions_data();
    void rerequestKeepsTheShownOptions();
    void switchesDuringTransitionsSettle_data();
    void switchesDuringTransitionsSettle();
    void switchingBackWhileTheAreaCollapses();

private:
    struct Harness {
        LatteViewStub latteView;
        ViewConfigStub viewConfig;
        //! Declared after the stubs so the QML tree goes away first.
        std::unique_ptr<QQuickView> view;
        std::unique_ptr<QQuickItem> root;
        QQuickItem *stackView{nullptr};
    };

    static QString load(Harness &harness, const QString &initialType, int transitionDuration);
    static QString shownOptionsError(Harness &harness, const QString &type);
};

QString IndicatorConfigUiManagerTest::load(Harness &harness, const QString &initialType, int transitionDuration)
{
    harness.latteView.m_indicator.setType(initialType);

    harness.view = std::make_unique<QQuickView>();
    harness.view->rootContext()->setContextProperty(QStringLiteral("testLatteView"), &harness.latteView);
    harness.view->rootContext()->setContextProperty(QStringLiteral("testViewConfig"), &harness.viewConfig);

    //! The harness URL sits in autotests/ so its relative import resolves the
    //! production controls directory exactly like EffectsConfig.qml does.
    QQmlComponent component(harness.view->engine());
    component.setData(kHarness, QUrl::fromLocalFile(QStringLiteral(LATTE_SOURCE_DIR "/autotests/indicatorconfiguimanagertest.qml")));

    if (!component.isReady()) {
        return component.errorString();
    }

    //! Initial properties apply before the manager completes and shows the
    //! first page, so that page already uses the requested transition.
    harness.root.reset(qobject_cast<QQuickItem *>(
        component.createWithInitialProperties({{QStringLiteral("transitionDuration"), transitionDuration}}, harness.view->rootContext())));

    if (!harness.root) {
        return component.errorString();
    }

    harness.root->setParentItem(harness.view->contentItem());
    harness.stackView = qvariant_cast<QQuickItem *>(harness.root->property("stackView"));

    if (!harness.stackView) {
        return QStringLiteral("harness does not expose its StackView");
    }

    harness.view->resize(420, 480);
    harness.view->show();

    if (!QTest::qWaitForWindowExposed(harness.view.get())) {
        return QStringLiteral("harness window was not exposed");
    }

    return QString();
}

//! Returns an empty string when the options area shows exactly the options of
//! \a type, or nothing for an indicator without options.
QString IndicatorConfigUiManagerTest::shownOptionsError(Harness &harness, const QString &type)
{
    QQuickItem *stackView = harness.stackView;

    if (!QTest::qWaitFor([stackView]() { return !stackView->property("busy").toBool(); }, 5000)) {
        return QStringLiteral("StackView transition did not finish");
    }

    auto *page = qvariant_cast<QQuickItem *>(stackView->property("currentItem"));

    if (!page) {
        return QStringLiteral("StackView has no current page");
    }

    if (page->property("type").toString() != type) {
        return QStringLiteral("current page belongs to %1").arg(page->property("type").toString());
    }

    if (!page->isVisible()) {
        return QStringLiteral("current page is hidden");
    }

    //! Visible alone is not on screen: a page that already slid out once keeps
    //! the replaceExit end state (opacity 0, x off-view), and StackView skips
    //! replaceEnter while the view has no height.
    if (!qFuzzyCompare(page->opacity(), 1.0) || !qFuzzyIsNull(page->x())) {
        return QStringLiteral("current page is not in place (opacity %1, x %2)").arg(page->opacity()).arg(page->x());
    }

    const QString selectedTab = harness.root->property("selectedTab").toString();

    if (selectedTab != type) {
        return QStringLiteral("tab bar selected %1").arg(selectedTab);
    }

    const IndicatorUiManagerStub &manager = harness.viewConfig.m_manager;
    QQuickItem *expected = manager.optionsItem(type);
    const QList<QQuickItem *> pageChildren = page->childItems();

    if (expected && (pageChildren.size() != 1 || pageChildren.constFirst() != expected)) {
        return QStringLiteral("current page does not hold the %1 options (%2 children)").arg(type).arg(pageChildren.size());
    }

    if (!expected && !pageChildren.isEmpty()) {
        return QStringLiteral("current page of an indicator without options holds %1").arg(pageChildren.constFirst()->objectName());
    }

    const auto items = manager.optionsItems();

    for (QQuickItem *item : items) {
        if (item->isVisible() != (item == expected)) {
            return QStringLiteral("options of %1 are %2").arg(item->objectName(), item->isVisible() ? QStringLiteral("visible") : QStringLiteral("hidden"));
        }
    }

    const qreal expectedHeight = expected ? expected->implicitHeight() : 0;

    if (!QTest::qWaitFor([stackView, expectedHeight]() { return qFuzzyCompare(stackView->height() + 1, expectedHeight + 1); }, 2000)) {
        return QStringLiteral("options area is %1px high instead of %2px").arg(stackView->height()).arg(expectedHeight);
    }

    return QString();
}

void IndicatorConfigUiManagerTest::showsTheSelectedIndicatorOptions_data()
{
    QTest::addColumn<QStringList>("types");
    QTest::addColumn<int>("transitionDuration");

    //! The first type is selected when the settings window opens.
    const QList<std::pair<const char *, QStringList>> sequences = {
        {"latte to no options and back", {kLatte, kWithoutOptions, kLatte, kWithoutOptions, kLatte}},
        {"custom options and no options", {kLatte, kWithOptions, kWithoutOptions, kWithOptions, kWithoutOptions, kLatte, kWithOptions}},
        {"opened on no options", {kWithoutOptions, kLatte, kWithoutOptions, kWithOptions, kWithoutOptions}},
    };

    for (const auto &[name, types] : sequences) {
        QTest::addRow("%s, immediate", name) << types << 0;
        QTest::addRow("%s, animated", name) << types << 60;
    }
}

void IndicatorConfigUiManagerTest::showsTheSelectedIndicatorOptions()
{
    QFETCH(QStringList, types);
    QFETCH(int, transitionDuration);

    Harness harness;
    const QString loadError = load(harness, types.constFirst(), transitionDuration);
    QVERIFY2(loadError.isEmpty(), qPrintable(loadError));

    QPointer<QQuickItem> shownPage;

    for (int step = 0; step < types.size(); ++step) {
        harness.latteView.m_indicator.setType(types.at(step));
        const QString error = shownOptionsError(harness, types.at(step));
        QVERIFY2(error.isEmpty(), qPrintable(QStringLiteral("step %1, %2: %3").arg(step).arg(types.at(step), error)));

        //! A page that slid out is deleted, and the cached options it held
        //! survive it with a parent for the next request.
        auto *page = qvariant_cast<QQuickItem *>(harness.stackView->property("currentItem"));

        if (shownPage && shownPage != page) {
            QTRY_VERIFY2(shownPage.isNull(), qPrintable(QStringLiteral("step %1: a removed page is kept alive").arg(step)));
        }

        const auto items = harness.viewConfig.m_manager.optionsItems();

        for (QQuickItem *item : items) {
            QVERIFY2(item->parentItem(), qPrintable(QStringLiteral("step %1: options of %2 have no parent").arg(step).arg(item->objectName())));
        }

        shownPage = page;
    }
}

void IndicatorConfigUiManagerTest::rerequestKeepsTheShownOptions_data()
{
    QTest::addColumn<QString>("type");

    QTest::newRow("latte") << kLatte;
    QTest::newRow("custom with options") << kWithOptions;
    QTest::newRow("custom without options") << kWithoutOptions;
}

//! PrimaryConfigView::initParentView() toggles isReady whenever the shared
//! settings window moves to another dock, and the manager asks for the same
//! indicator type again when both docks use it.
void IndicatorConfigUiManagerTest::rerequestKeepsTheShownOptions()
{
    QFETCH(QString, type);

    Harness harness;
    const QString loadError = load(harness, kLatte, 0);
    QVERIFY2(loadError.isEmpty(), qPrintable(loadError));

    harness.latteView.m_indicator.setType(type);
    QString error = shownOptionsError(harness, type);
    QVERIFY2(error.isEmpty(), qPrintable(error));

    for (int round = 0; round < 2; ++round) {
        harness.viewConfig.setIsReady(false);
        harness.viewConfig.setIsReady(true);
        error = shownOptionsError(harness, type);
        QVERIFY2(error.isEmpty(), qPrintable(QStringLiteral("round %1: %2").arg(round).arg(error)));
    }
}

void IndicatorConfigUiManagerTest::switchesDuringTransitionsSettle_data()
{
    QTest::addColumn<QStringList>("types");

    QTest::newRow("back to latte") << QStringList{kLatte, kWithoutOptions, kLatte};
    QTest::newRow("back to custom options") << QStringList{kWithOptions, kLatte, kWithOptions};
    QTest::newRow("through all types") << QStringList{kLatte, kWithOptions, kWithoutOptions, kLatte, kWithoutOptions};
}

//! A new selection may arrive while the previous page is still sliding out.
void IndicatorConfigUiManagerTest::switchesDuringTransitionsSettle()
{
    QFETCH(QStringList, types);

    Harness harness;
    const QString loadError = load(harness, types.constFirst(), 250);
    QVERIFY2(loadError.isEmpty(), qPrintable(loadError));
    QString error = shownOptionsError(harness, types.constFirst());
    QVERIFY2(error.isEmpty(), qPrintable(error));

    //! No events run between the switches, so each one replaces the pages
    //! while the transition started by the previous switch is still running.
    for (int step = 1; step < types.size(); ++step) {
        harness.latteView.m_indicator.setType(types.at(step));
        QVERIFY2(harness.stackView->property("busy").toBool(), qPrintable(QStringLiteral("step %1 did not start a transition").arg(step)));
    }

    error = shownOptionsError(harness, types.constLast());
    QVERIFY2(error.isEmpty(), qPrintable(error));
}

//! Selecting an indicator without options collapses the options area at the
//! next layout pass while the previous page is still sliding out; switching
//! back then shows the options while the view has no height yet, a case in
//! which StackView skips the replaceEnter transition.
void IndicatorConfigUiManagerTest::switchingBackWhileTheAreaCollapses()
{
    Harness harness;
    const QString loadError = load(harness, kLatte, 1000);
    QVERIFY2(loadError.isEmpty(), qPrintable(loadError));
    QString error = shownOptionsError(harness, kLatte);
    QVERIFY2(error.isEmpty(), qPrintable(error));

    harness.latteView.m_indicator.setType(kWithoutOptions);
    QTRY_COMPARE(harness.stackView->height(), 0.0);
    QVERIFY2(harness.stackView->property("busy").toBool(), "the Latte options finished sliding out before the area collapsed");

    harness.latteView.m_indicator.setType(kLatte);
    error = shownOptionsError(harness, kLatte);
    QVERIFY2(error.isEmpty(), qPrintable(error));
}

QTEST_MAIN(IndicatorConfigUiManagerTest)
#include "indicatorconfiguimanagertest.moc"
