/*
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include <QPair>
#include <QQuickItem>
#include <QQuickView>
#include <QSignalSpy>
#include <QTest>

class HelpMenuQmlTest : public QObject
{
    Q_OBJECT
  private Q_SLOTS:
    void opensOutsideSettingsTabs();
};

void
HelpMenuQmlTest::opensOutsideSettingsTabs()
{
    const QList<QPair<QString, QString>> translations = {
        { QStringLiteral("Help"), QStringLiteral("User Guide") },
        { QString::fromUtf8("帮助"), QString::fromUtf8("使用手册") },
        { QString::fromUtf8("說明"), QString::fromUtf8("使用手冊") },
    };

    QQuickView view;
    view.setInitialProperties({ { QStringLiteral("text"), translations.constFirst().first },
                                { QStringLiteral("guideText"), translations.constFirst().second } });
    view.setSource(QUrl::fromLocalFile(QStringLiteral(LATTE_SOURCE_DIR "/shell/package/contents/controls/HelpMenuButton.qml")));
    QVERIFY2(view.status() == QQuickView::Ready, qPrintable(view.errors().isEmpty() ? QString() : view.errors().constFirst().toString()));
    auto *button = view.rootObject();
    QVERIFY(button);
    QVERIFY(button->implicitWidth() > 0);
    QVERIFY(button->implicitHeight() > 0);
    view.resize(450, 240);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QSignalSpy requested(button, SIGNAL(guideRequested(QString)));
    QVERIFY(requested.isValid());
    QTest::mouseClick(&view, Qt::LeftButton, Qt::NoModifier, QPoint(20, 15));

    auto *menu = button->findChild<QObject *>(QStringLiteral("helpMenu"));
    QVERIFY(menu);
    QTRY_VERIFY_WITH_TIMEOUT(menu->property("visible").toBool(), 15000);
    QVERIFY(!button->findChild<QObject *>(QStringLiteral("copiesAction")));
    QCOMPARE(menu->property("count").toInt(), 1);

    auto *guideAction = button->findChild<QObject *>(QStringLiteral("guideAction"));
    QVERIFY(guideAction);
    auto *guideItem = qobject_cast<QQuickItem *>(guideAction);
    QVERIFY(guideItem);

    // Keep one popup alive while swapping translations. Recreating popup
    // windows for every data row is unreliable on minimal/offscreen hosts.
    for (const auto &translation : translations) {
        button->setProperty("text", translation.first);
        button->setProperty("guideText", translation.second);
        QCoreApplication::processEvents();
        QCOMPARE(guideAction->property("text").toString(), translation.second);
        QTRY_VERIFY_WITH_TIMEOUT(guideItem->width() >= guideItem->implicitWidth(), 5000);
    }

    QVERIFY(QMetaObject::invokeMethod(guideAction, "triggered"));
    QCOMPARE(requested.count(), 1);
    QCOMPARE(requested.takeFirst().at(0).toString(), QStringLiteral("edit-dock"));
    QMetaObject::invokeMethod(menu, "close");
}

QTEST_MAIN(HelpMenuQmlTest)
#include "helpmenuqmltest.moc"
