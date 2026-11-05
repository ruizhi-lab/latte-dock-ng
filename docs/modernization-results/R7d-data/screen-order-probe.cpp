// R7d probe, not part of the Latte build. It answers one question for the
// UniversalSettings::screens NOTIFY: when screenAdded / screenRemoved are
// emitted, is QGuiApplication::screens() already updated?
//
// Build: g++ -std=c++20 -fPIC $(pkg-config --cflags Qt6Gui) screen-order-probe.cpp $(pkg-config --libs Qt6Gui) -o screen-order-probe
// Run:   QT_QPA_PLATFORM=offscreen ./screen-order-probe

#include <QDebug>
#include <QGuiApplication>
#include <QScreen>
#include <QStringList>

#include <qpa/qplatformscreen.h>
#include <qpa/qwindowsysteminterface.h>

namespace {

class ProbePlatformScreen : public QPlatformScreen
{
public:
    explicit ProbePlatformScreen(int x)
        : m_x(x)
    {
    }

    QRect geometry() const override { return QRect(m_x, 0, 1000, 800); }
    int depth() const override { return 32; }
    QImage::Format format() const override { return QImage::Format_ARGB32_Premultiplied; }

private:
    int m_x;
};

// Lists the x of every screen in the list. The screen that a removal signal
// names is compared by pointer only: it may already be deleted.
QString listing(const QScreen *removed = nullptr)
{
    QStringList parts;
    for (const QScreen *screen : QGuiApplication::screens()) {
        parts << (screen == removed ? QStringLiteral("removed-but-listed") : QString::number(screen->geometry().x()));
    }
    return QStringLiteral("list=[%1] count=%2 primary.x=%3")
        .arg(parts.join(QLatin1Char(',')))
        .arg(QGuiApplication::screens().count())
        .arg(QGuiApplication::primaryScreen()->geometry().x());
}

} // namespace

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QWindowSystemInterface::setSynchronousWindowSystemEvents(true);

    QObject::connect(qGuiApp, &QGuiApplication::screenAdded, qGuiApp, [](QScreen *screen) {
        qInfo().noquote() << "  screenAdded: new screen is listed:" << QGuiApplication::screens().contains(screen) << "|" << listing();
    });
    QObject::connect(qGuiApp, &QGuiApplication::screenRemoved, qGuiApp, [](QScreen *screen) {
        qInfo().noquote() << "  screenRemoved: removed screen is still listed:" << QGuiApplication::screens().contains(screen) << "|" << listing(screen);
        // Lifetime: when is the removed QScreen destroyed relative to this handler?
        QObject::connect(screen, &QObject::destroyed, qGuiApp, [] {
            qInfo() << "  removed QScreen destroyed";
        });
    });
    QObject::connect(qGuiApp, &QGuiApplication::primaryScreenChanged, qGuiApp, [](QScreen *) {
        qInfo().noquote() << "  primaryScreenChanged |" << listing();
    });

    qInfo().noquote() << "initial               " << listing();

    auto *second = new ProbePlatformScreen(1000);
    QWindowSystemInterface::handleScreenAdded(second);
    qInfo().noquote() << "after add(second)     " << listing();

    auto *third = new ProbePlatformScreen(2000);
    QWindowSystemInterface::handleScreenAdded(third, true);
    qInfo().noquote() << "after add(third, primary)" << listing();

    QWindowSystemInterface::handleScreenRemoved(second);
    qInfo().noquote() << "after remove(second)  " << listing();


    QWindowSystemInterface::handleScreenRemoved(third);
    qInfo().noquote() << "after remove(third)   " << listing();

    // A primary change without any add or remove: does the order of the list change?
    auto *fourth = new ProbePlatformScreen(3000);
    QWindowSystemInterface::handleScreenAdded(fourth);
    qInfo().noquote() << "after add(fourth)     " << listing();

    QWindowSystemInterface::handlePrimaryScreenChanged(fourth);
    qInfo().noquote() << "after primary(fourth) " << listing();

    return 0;
}
