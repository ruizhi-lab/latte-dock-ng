/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

// Qt
#include <QDir>
#include <QFileInfo>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QTemporaryDir>
#include <QtQml/qqml.h>
#include <QTest>

#include <memory>

// local
#include "../app/view/layoutmanagerapidispatcher.h"

class PluginRegistrationUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void containmentPluginRegistersTypes();
};

//! The plugin .so and its generated qmldir are staged into a temporary
//! import path and loaded by a real QML engine, asserting the same
//! contract as the former hand-written registerTypes(): the LayoutManager
//! type and the flat "types.<Value>" enum access used across the QML
//! sources keep working.
void PluginRegistrationUnitTest::containmentPluginRegistersTypes()
{
    QTemporaryDir importRoot;
    QVERIFY(importRoot.isValid());

    const QString modulePath = importRoot.path() + QStringLiteral("/org/kde/latte/private/containment");
    QVERIFY(QDir().mkpath(modulePath));

    const QString pluginFile = QStringLiteral(LATTE_CONTAINMENT_PLUGIN);
    QVERIFY(QFile::copy(QStringLiteral(LATTE_CONTAINMENT_QMLDIR), modulePath + QStringLiteral("/qmldir")));
    // $<TARGET_FILE> already includes the platform's shared-library prefix.
    QVERIFY(QFile::copy(pluginFile, modulePath + QLatin1Char('/')
                                  + QFileInfo(pluginFile).fileName()));

    QQmlEngine engine;
    engine.addImportPath(importRoot.path());

    QQmlComponent component(&engine);
    component.setData(QByteArrayLiteral(
                          "import QtQml\n"
                          "import org.kde.latte.private.containment\n"
                          "QtObject {\n"
                          "    readonly property int scroll: types.ScrollNone\n"
                          "    property LayoutManager manager: LayoutManager {}\n"
                          "}\n"),
                      QUrl(QStringLiteral("qrc:/containmentpluginregistrationtest.qml")));
    if (component.isError()) {
        qWarning() << component.errors();
        qWarning() << "staged module files:" << QDir(modulePath).entryList(QDir::Files);
    }
    QCOMPARE(component.status(), QQmlComponent::Ready);

    QVERIFY(qmlTypeId("org.kde.latte.private.containment", 0, 1, "LayoutManager") >= 0);
    // The probe above is the authoritative check for the Types gadget:
    // declaratively registered gadgets may not be visible through
    // qmlTypeId's revision lookup, but must resolve in real QML.

    std::unique_ptr<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QPointer<QObject> manager = root->property("manager").value<QObject *>();
    QVERIFY(manager);
    auto *api = qobject_cast<Latte::Containment::LayoutManagerApi *>(manager.data());
    QVERIFY(api);

    Latte::ViewPart::LayoutManagerApiDispatcher dispatcher;
    dispatcher.setObject(manager.data());
    dispatcher.requestAppletsOrder({71, 73});
    QCOMPARE(dispatcher.order(), QList<int>({71, 73}));

    root.reset();
    QVERIFY(manager.isNull());
    QVERIFY(dispatcher.order().isEmpty());
}

QTEST_GUILESS_MAIN(PluginRegistrationUnitTest)

#include "pluginregistrationunittest.moc"
