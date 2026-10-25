/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "session/teardown.h"

#include <QCoreApplication>
#include <QPointer>
#include <QTest>
#include <QTimer>

class TeardownTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void deleteNowDeletesServiceImmediately();
    void deleteNowSkipsServiceFreedByQuitCleanup();
};

void TeardownTest::deleteNowDeletesServiceImmediately()
{
    // ~Corona() relies on synchronous deletion to impose its dependency order.
    // Test functions run outside an event loop, like main() after exec().
    auto *service = new QObject;
    QPointer<QObject> guard(service);

    Latte::Session::deleteNow(service);
    QVERIFY(guard.isNull());
}

void TeardownTest::deleteNowSkipsServiceFreedByQuitCleanup()
{
    // Mirrors a normal quit: Corona::onAboutToQuit() deleteLater()s the view
    // settings factory from inside QCoreApplication::exit().
    auto *service = new QObject;
    QPointer<QObject> guard(service);
    connect(qApp, &QCoreApplication::aboutToQuit, service, &QObject::deleteLater);

    QTimer::singleShot(0, qApp, &QCoreApplication::quit);
    QCOMPARE(QCoreApplication::exec(), 0);

    // exec() flushed the deferred delete before returning, so the service is
    // already gone when main() destroys the Corona. A raw member would now
    // dangle; the guarded one is null and the teardown must skip it.
    QVERIFY(guard.isNull());
    Latte::Session::deleteNow(guard);
}

QTEST_GUILESS_MAIN(TeardownTest)

#include "teardowntest.moc"
