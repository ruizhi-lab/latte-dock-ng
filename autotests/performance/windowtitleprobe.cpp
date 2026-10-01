/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QGuiApplication>
#include <QTimer>
#include <QWindow>

int
main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QWindow window;
    window.resize(420, 240);
    window.setTitle(QStringLiteral("Latte synthetic title probe"));
    window.show();

    QTimer::singleShot(6000, &app, [&window, &app]() {
        for (int index = 0; index < 10; ++index) {
            QTimer::singleShot(index * 500, &app, [&window, index]() { window.setTitle(QStringLiteral("Latte synthetic title probe %1").arg(index)); });
        }
    });
    QTimer::singleShot(12500, &app, &QCoreApplication::quit);
    return app.exec();
}
