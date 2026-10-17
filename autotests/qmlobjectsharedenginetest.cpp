/*
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <KDeclarative/QmlObjectSharedEngine>

#include <Plasma/version.h>

#include <QFile>
#include <QPointer>
#include <QQmlContext>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

//! Exercises the subset of PlasmaQuick::SharedQmlEngine that Latte relies on:
//! the indicator configuration UIs and the applet alternatives dialog
//! (Corona::showAlternativesForApplet()). On libplasma 6.8 the type is Latte's
//! compat implementation on top of PlasmaQuick::globalEngine(); before 6.8 it
//! is the libplasma class itself, so both must honour the same contract.
class QmlObjectSharedEngineTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void delayedInitializationSeesRootContextProperties();
    void instancesShareOneEngine();
    void translationsAreAvailable();
    void rootObjectIsDeletedWithTheWrapper();
    void brokenSourceCreatesNoRootObject();

private:
    QUrl writeQml(const QString &name, const QByteArray &source);

    QTemporaryDir m_dir;
};

void QmlObjectSharedEngineTest::initTestCase()
{
    QVERIFY(m_dir.isValid());
}

QUrl QmlObjectSharedEngineTest::writeQml(const QString &name, const QByteArray &source)
{
    QFile file(m_dir.filePath(name));

    if (!file.open(QIODevice::WriteOnly) || file.write(source) != source.size()) {
        return {};
    }

    return QUrl::fromLocalFile(file.fileName());
}

void QmlObjectSharedEngineTest::delayedInitializationSeesRootContextProperties()
{
    // The alternatives dialog publishes alternativesHelper after setSource()
    // and before completeInitialization(); it must be visible both to bindings
    // and to Component.onCompleted.
    const QUrl source = writeQml(QStringLiteral("delayed.qml"), R"(
import QtQml

QtObject {
    property string helperName: typeof alternativesHelper !== "undefined" ? alternativesHelper.objectName : ""
    property string seenOnCompleted: ""
    Component.onCompleted: seenOnCompleted = alternativesHelper.objectName
}
)");
    QVERIFY(source.isValid());

    QObject helper;
    helper.setObjectName(QStringLiteral("helper"));

    KDeclarative::QmlObjectSharedEngine qmlObj;
    qmlObj.setInitializationDelayed(true);
    qmlObj.setSource(source);
    QVERIFY(qmlObj.rootObject());
    QCOMPARE(qmlObj.rootObject()->property("seenOnCompleted").toString(), QString());

    qmlObj.rootContext()->setContextProperty(QStringLiteral("alternativesHelper"), &helper);
    qmlObj.completeInitialization();

    QCOMPARE(qmlObj.rootObject()->property("seenOnCompleted").toString(), QStringLiteral("helper"));
    QCOMPARE(qmlObj.rootObject()->property("helperName").toString(), QStringLiteral("helper"));
}

void QmlObjectSharedEngineTest::instancesShareOneEngine()
{
    KDeclarative::QmlObjectSharedEngine first;
    KDeclarative::QmlObjectSharedEngine second;

    QVERIFY(first.engine());
    QCOMPARE(first.engine().get(), second.engine().get());
    QVERIFY(first.rootContext() != second.rootContext());

#if PLASMA_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    // Indicator configuration UIs instantiate applet types, which are only
    // registered in the engine that owns the applets.
    QCOMPARE(first.engine().get(), PlasmaQuick::globalEngine().get());
#endif
}

void QmlObjectSharedEngineTest::translationsAreAvailable()
{
    const QUrl source = writeQml(QStringLiteral("i18n.qml"), R"(
import QtQml

QtObject {
    property string text: i18n("Show Alternatives")
    property string plural: i18np("%1 widget", "%1 widgets", 2)
}
)");
    QVERIFY(source.isValid());

    KDeclarative::QmlObjectSharedEngine qmlObj;
    qmlObj.setTranslationDomain(QStringLiteral("latte-dock"));
    qmlObj.setSource(source);
    QVERIFY(qmlObj.rootObject());
    QCOMPARE(qmlObj.rootObject()->property("text").toString(), QStringLiteral("Show Alternatives"));
    QCOMPARE(qmlObj.rootObject()->property("plural").toString(), QStringLiteral("2 widgets"));
}

void QmlObjectSharedEngineTest::rootObjectIsDeletedWithTheWrapper()
{
    const QUrl source = writeQml(QStringLiteral("owned.qml"), "import QtQml\nQtObject {}\n");
    QVERIFY(source.isValid());

    auto qmlObj = std::make_unique<KDeclarative::QmlObjectSharedEngine>();
    qmlObj->setSource(source);
    QPointer<QObject> root = qmlObj->rootObject();
    QVERIFY(root);

    qmlObj.reset();
    QVERIFY(root.isNull());
}

void QmlObjectSharedEngineTest::brokenSourceCreatesNoRootObject()
{
    const QUrl source = writeQml(QStringLiteral("broken.qml"), "import QtQml\nQtObject { property int value: }\n");
    QVERIFY(source.isValid());

    KDeclarative::QmlObjectSharedEngine qmlObj;
    qmlObj.setInitializationDelayed(true);
    qmlObj.setSource(source);
    qmlObj.completeInitialization();

    QVERIFY(!qmlObj.rootObject());
}

QTEST_MAIN(QmlObjectSharedEngineTest)

#include "qmlobjectsharedenginetest.moc"
