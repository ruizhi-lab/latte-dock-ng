/*
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include <QQuickItem>
#include <QQuickView>
#include <QSignalSpy>
#include <QTest>

class HelpMenuQmlTest : public QObject
{
    Q_OBJECT
  private Q_SLOTS:
    void opensOutsideSettingsTabs_data();
    void opensOutsideSettingsTabs();
};

void
HelpMenuQmlTest::opensOutsideSettingsTabs_data()
{
    QTest::addColumn<QString>("help");
    QTest::addColumn<QString>("guide");
    QTest::newRow("english") << QStringLiteral("Help") << QStringLiteral("User Guide");
    QTest::newRow("simplified-chinese") << QString::fromUtf8("帮助") << QString::fromUtf8("使用手册");
    QTest::newRow("traditional-chinese") << QString::fromUtf8("說明") << QString::fromUtf8("使用手冊");
}

void
HelpMenuQmlTest::opensOutsideSettingsTabs()
{
    QFETCH(QString, help);
    QFETCH(QString, guide);
    QQuickView view;
    view.setInitialProperties({ { QStringLiteral("text"), help }, { QStringLiteral("guideText"), guide } });
    view.setSource(QUrl::fromLocalFile(QStringLiteral(LATTE_SOURCE_DIR "/shell/package/contents/controls/HelpMenuButton.qml")));
    QVERIFY2(view.status() == QQuickView::Ready, qPrintable(view.errors().isEmpty() ? QString() : view.errors().constFirst().toString()));
    auto *button = view.rootObject();
    QVERIFY(button);
    QVERIFY(button->implicitWidth() > 0);
    QVERIFY(button->implicitHeight() > 0);
    view.resize(450, 240);
    view.show();
    QSignalSpy requested(button, SIGNAL(guideRequested(QString)));
    QVERIFY(requested.isValid());
    QTest::mouseClick(&view, Qt::LeftButton, Qt::NoModifier, QPoint(20, 15));
    auto *menu = button->findChild<QObject *>(QStringLiteral("helpMenu"));
    QVERIFY(menu);
    QTRY_VERIFY(menu->property("visible").toBool());
    auto *guideAction = button->findChild<QObject *>(QStringLiteral("guideAction"));
    QVERIFY(!button->findChild<QObject *>(QStringLiteral("copiesAction")));
    QCOMPARE(menu->property("count").toInt(), 1);
    QVERIFY(guideAction);
    QCOMPARE(guideAction->property("text").toString(), guide);
    auto *guideItem = qobject_cast<QQuickItem *>(guideAction);
    QVERIFY(guideItem);
    QTRY_VERIFY(guideItem->width() >= guideItem->implicitWidth());
    QVERIFY(QMetaObject::invokeMethod(guideAction, "triggered"));
    QCOMPARE(requested.count(), 1);
    QCOMPARE(requested.takeFirst().at(0).toString(), QStringLiteral("edit-dock"));
}

QTEST_MAIN(HelpMenuQmlTest)
#include "helpmenuqmltest.moc"
