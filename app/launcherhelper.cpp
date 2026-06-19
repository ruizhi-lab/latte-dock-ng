/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QTextStream>

namespace {

int fail(const QString &message)
{
    QTextStream(stderr) << message << Qt::endl;
    return 1;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("latte-dock-ng-add-launcher"));

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addPositionalArgument(QStringLiteral("launcher_url"),
                                 QStringLiteral("Application launcher URL or desktop file path."));
    parser.process(app);

    const QStringList arguments = parser.positionalArguments();

    if (arguments.isEmpty()) {
        parser.showHelp(2);
    }

    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.kde.lattedock"),
        QStringLiteral("/Latte"),
        QStringLiteral("org.kde.LatteDock"),
        QStringLiteral("addLauncher"));
    msg.setArguments({arguments.constFirst(), QString()});

    QDBusMessage reply = QDBusConnection::sessionBus().call(msg, QDBus::Block, 1000);

    if (reply.type() == QDBusMessage::ErrorMessage) {
        return fail(reply.errorMessage());
    }

    if (reply.arguments().isEmpty() || !reply.arguments().constFirst().toBool()) {
        return fail(QStringLiteral("Latte Dock did not accept the launcher."));
    }

    return 0;
}
