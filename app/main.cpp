/*
    SPDX-FileCopyrightText: 2016 Smith AR <audoban@openmailbox.org>
    SPDX-FileCopyrightText: 2016 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-FileCopyrightText: 2024-2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

// local
#include <latte_debug.h>
#include "config-latte.h"
#include "apptypes.h"
#include "knscompat.h"
#include "lattecorona.h"
#include "layouts/importer.h"
#include "layouts/manager.h"
#include "session/shutdownmonitor.h"
#include "templates/templatesmanager.h"
#include "view/view.h"
#include "wm/abstractwindowinterface.h"

// C
#include <csignal>
#include <cstdio>

// POSIX
#include <fcntl.h>
#include <unistd.h>

// C++
#include <functional>
#include <memory>

// Qt
#include <QApplication>
#include <QDebug>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QLockFile>
#include <QSessionManager>
#include <QSocketNotifier>
#include <QStandardPaths>
#include <QTextStream>
#include <QQmlComponent>
#include <QUrl>

// KDE
#include <KLocalizedString>
#include <KAboutData>
#include <KConfigGroup>
#include <KCoreAddons/KSignalHandler>
#include <KDBusService>
#include <KSharedConfig>
#include <KWindowSystem>
#include <QQmlEngine>
#if __has_include(<plasmaquick/sharedqmlengine.h>)
#include <plasmaquick/sharedqmlengine.h>
#else
#include <PlasmaQuick/PlasmaQuick>
#endif

//! COLORS
#define CNORMAL  "\x1b[0m"
#define CIGREEN  "\x1b[1;32m"
#define CGREEN   "\x1b[0;32m"
#define CICYAN   "\x1b[1;36m"
#define CCYAN    "\x1b[0;36m"
#define CIRED    "\x1b[1;31m"
#define CRED     "\x1b[0;31m"

//! Layout choices from the command line, read once when the Corona is constructed.
struct StartupLayoutOptions {
    bool defaultLayout{false};
    int memoryUsage{-1};
    QString layoutName;
    QString addViewTemplateName;
};

inline void configureAboutData();
inline void detectPlatform(int argc, char **argv);
inline void filterDebugMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);
inline bool shouldUseUserLocalQmlImports(int argc, char **argv);
inline void collectUserLocalPaths(int argc, char **argv);
inline void applyUserLocalPluginPaths();
inline void ensureKdeSessionEnvironment();
inline void autoClearQmlCacheOnVersionChange();
inline void configureQtQuickGraphicsPreference();
inline bool configureProcessEnvironment(int argc, char **argv);
inline void addCommandLineOptions(QCommandLineParser &parser);
inline void addHiddenDeveloperOptions(QCommandLineParser &parser);
inline bool runOneShotCommand(const QCommandLineParser &parser);
inline void watchShutdownSignals();
inline void connectSignalShutdown(QApplication &app, const std::function<void()> &markSessionEnding);
inline void connectSessionManagement(QApplication &app);
inline bool chooseStartupLayout(const QCommandLineParser &parser, StartupLayoutOptions &startup);
inline QString singleInstanceLockPath();
inline int stopReplacedInstance(const QCommandLineParser &parser, const QLockFile &lockFile);
inline void forwardToRunningInstance(const QCommandLineParser &parser, const QLockFile &lockFile);
inline bool applyPostLockOptions(const QCommandLineParser &parser, StartupLayoutOptions &startup);
inline void installDebugMessageHandler(const QCommandLineParser &parser);
inline int printVersion();
inline int runPrivateAppHostSmoke();
inline void prepareApplication(QApplication &app);

QString filterDebugMessageText;
QString filterDebugLogFile;

int main(int argc, char **argv)
{
    // Qt reads these settings when QApplication is constructed, so they come first.
    const bool qpaVariable = configureProcessEnvironment(argc, argv);
    QCoreApplication::setAttribute(Qt::AA_DisableSessionManager);
    QApplication app(argc, argv);
    applyUserLocalPluginPaths();
    qunsetenv("QT_WAYLAND_DISABLE_FIXED_POSITIONS");

    // Packaging probes and headless CI need --version before a compositor
    // exists. Keep this before the Wayland check; otherwise --version
    // exits with the "requires a Wayland Plasma session" error.
    const QStringList earlyArguments = QCoreApplication::arguments();
    if (earlyArguments.contains(QStringLiteral("--version")) || earlyArguments.contains(QStringLiteral("-v"))) {
        return printVersion();
    }

    if (qEnvironmentVariableIntValue("LATTE_PRIVATE_APP_HOST_SMOKE") == 1) {
        return runPrivateAppHostSmoke();
    }

    // The QPA platform is fixed when QApplication is constructed and
    // KWindowSystem caches it (isPlatformWayland is a CONSTANT property), so
    // a later retry can never observe Wayland and only delayed the failure.
    // detectPlatform() already requests Wayland before construction; ordering
    // the dock after the compositor is the session's job (XDG autostart and
    // systemd start it after KWin), not a sleep loop here.
    if (!KWindowSystem::isPlatformWayland()) {
        qCritical() << "Latte-Dock Wayland-only build requires a Wayland Plasma session.";
        return 1;
    }

    if (!qpaVariable) {
        // don't leak the env variable to processes we start
        qunsetenv("QT_QPA_PLATFORM");
    }

    prepareApplication(app);

    configureAboutData();

    QCommandLineParser parser;
    addCommandLineOptions(parser);
    addHiddenDeveloperOptions(parser);

    parser.process(app);

    if (runOneShotCommand(parser)) {
        return 0;
    }

    auto markSessionEnding = [&app]() {
        if (!qEnvironmentVariableIsSet("LATTE_SESSION_ENDING")) {
            qputenv("LATTE_SESSION_ENDING", "1");
        }

        if (!app.property("latte_session_ending").toBool()) {
            app.setProperty("latte_session_ending", true);
            qInfo() << "[shutdown] session-ending flag set; fast teardown enabled.";
        }
    };

    watchShutdownSignals();

    //! Self-pipe signal handler for SIGINT (Ctrl-C) as a reliable fallback
    //! independent of KSignalHandler's signalfd mechanism.  On some platforms
    //! signalfd-based delivery can silently miss signals, leaving the process
    //! unkillable with Ctrl-C and resistant to SIGTERM from --replace.
    //!
    //! The self-pipe trick (pipe + QSocketNotifier) is the canonical pattern
    //! for signal handling in event-loop applications and does not depend on
    //! signalfd or sigprocmask behaviour.
    static int sigPipe[2] = {-1, -1};

    //! O_NONBLOCK keeps the signal handler from blocking when the pipe buffer
    //! is full. O_CLOEXEC keeps both ends out of helper processes such as
    //! latte-dock-ng-preview; an inherited write end would let a helper wake
    //! the dock's SIGINT path and keeps the descriptors open in every child.
    if (pipe2(sigPipe, O_CLOEXEC | O_NONBLOCK) == 0) {
        // Use sigaction (async-signal-safe) to install a handler that writes
        // a byte to the pipe.  This runs in signal context so only
        // async-signal-safe functions are used.
        struct sigaction sa {};

        sa.sa_handler = [](int) {
            const char c = 1;
            // The pipe is non-blocking; when it is full a wake-up is already
            // pending, so a failed write can be safely ignored.
            const ssize_t written = write(sigPipe[1], &c, 1);
            (void)written;
        };

        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        sigaction(SIGINT, &sa, nullptr);

        // QSocketNotifier translates the pipe byte into a Qt event that
        // safely calls quit() from the main event loop.
        auto *sigNotifier = new QSocketNotifier(sigPipe[0], QSocketNotifier::Read, &app);
        QObject::connect(sigNotifier, &QSocketNotifier::activated, &app, [&app, &markSessionEnding](int) {
            char c;

            if (read(sigPipe[0], &c, 1) == 1) {
                qInfo() << "[shutdown] SIGINT received via self-pipe → calling quit()";
                markSessionEnding();
                app.quit();
            }
        });
    }

    connectSignalShutdown(app, markSessionEnding);
    connectSessionManagement(app);

    //! choose layout for startup
    StartupLayoutOptions startupLayout;
    if (!chooseStartupLayout(parser, startupLayout)) {
        return 0;
    }

    //! --replace option
    QLockFile lockFile {singleInstanceLockPath()};
    const int timeout = stopReplacedInstance(parser, lockFile);

    if (!lockFile.tryLock(timeout)) {
        forwardToRunningInstance(parser, lockFile);
        return 0;
    }

    if (!applyPostLockOptions(parser, startupLayout)) {
        return 0;
    }

    installDebugMessageHandler(parser);

    //! Prime the Plasma global shared QML engine singleton after one-instance
    //! checks. Duplicate invocations only forward DBus actions and exit; they
    //! must not create a shared engine that would need full app teardown.
#if __has_include(<plasmaquick/sharedqmlengine.h>)
    std::shared_ptr<PlasmaQuick::SharedQmlEngine> sharedEngine =
        std::make_shared<PlasmaQuick::SharedQmlEngine>(&app);
    QQmlEngine *const sharedQmlEngine = sharedEngine->engine().get();
#else
    //! libplasma 6.8 replaced SharedQmlEngine with globalEngine(), which only
    //! keeps a weak reference: holding this shared_ptr for the whole run keeps
    //! one engine (and the import paths added below) for all views and applets.
    std::shared_ptr<QQmlEngine> sharedEngine = PlasmaQuick::globalEngine();
    QQmlEngine *const sharedQmlEngine = sharedEngine.get();
#endif

    // Add latte-dock QML import paths to the shared engine (engine-scoped, no env var leakage)
    addLatteQmlImportPaths(sharedQmlEngine);

    int result;
    {
        Latte::Corona corona(startupLayout.defaultLayout,
                             startupLayout.layoutName,
                             startupLayout.addViewTemplateName,
                             startupLayout.memoryUsage);

        //! Session shutdown observer.  On Wayland, ksmserver may expose the
        //! logout confirmation phase only through isShuttingDown().  Use that
        //! state to prepare the fast teardown path, but never quit while the
        //! user can still cancel the logout.  The monitor samples ksmserver
        //! only while plasma-shutdown runs; it is declared after the Corona so
        //! it is destroyed first and its hooks never outlive the views.
        Latte::Session::ShutdownMonitor sessionShutdownMonitor(QDBusConnection::sessionBus(), {
            .isSessionShuttingDown = []() {
                return Latte::Session::ksmServerShutdownState(QDBusConnection::sessionBus());
            },
            .isSessionEnding = [&app]() {
                return app.property("latte_session_ending").toBool();
            },
            .markSessionEnding = markSessionEnding,
            .clearSessionEnding = [&app]() {
                app.setProperty("latte_session_ending", false);
                qunsetenv("LATTE_SESSION_ENDING");
            },
            .hideViews = [&corona]() {
                if (auto *manager = corona.layoutsManager(); manager && manager->synchronizer()) {
                    manager->synchronizer()->hideAllViews();
                    manager->synchronizer()->setHelperWindowsSuspended(true);
                }
            },
            .restoreViews = [&corona]() {
                if (auto *manager = corona.layoutsManager(); manager && manager->synchronizer()) {
                    const auto views = manager->synchronizer()->currentViews();

                    for (Latte::View *view : views) {
                        if (!view->isVisible()) {
                            view->setVisible(true);
                        }
                    }

                    manager->synchronizer()->setHelperWindowsSuspended(false);
                }
            },
            .quit = [&app]() {
                app.quit();
            },
        });

        KDBusService service(KDBusService::Unique);
        result = app.exec();
    }

#if __has_include(<plasmaquick/sharedqmlengine.h>)
    // Detach the SharedQmlEngine from the QApplication before app is
    // destroyed.  Without this, ~QApplication() → deleteChildren() deletes
    // the SharedQmlEngine that is managed by a static shared_ptr, and the
    // subsequent shared_ptr destructor double-frees it.
    if (sharedEngine) {
        sharedEngine->setParent(nullptr);
    }
#endif

    // Drain any remaining DeferredDelete events.  The Corona destructor
    // already processes its own via deleteLater() + immediate flush, so
    // this loop mainly handles events from KDBusService or other globals.
    // Limit to a few passes to avoid spinning on cascading events.
    for (int pass = 0; pass < 5; ++pass) {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    // Destroy the shared engine while QApplication and QtGui globals are
    // still alive.  Leaving it to static destruction can crash when QQC2
    // popups owned by the engine tear down after QApplication has gone away.
    sharedEngine.reset();

    return result;
}

//! Process-wide Qt settings and environment fix-ups that must exist before QApplication is constructed.
inline bool configureProcessEnvironment(int argc, char **argv)
{
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    // Keep fractional scale behavior deterministic under Wayland.
    // Qt6 enables High DPI by default, so we only sanitize legacy env overrides.
    if (!qEnvironmentVariableIsSet("PLASMA_USE_QT_SCALING")) {
        qunsetenv("QT_DEVICE_PIXEL_RATIO");
    }

    QQuickWindow::setDefaultAlphaBuffer(true);
    configureQtQuickGraphicsPreference();
    collectUserLocalPaths(argc, argv);

    // Qt's Wayland client library reads this variable, so it must be set before
    // QApplication is constructed. main() unsets it right after QApplication.
    qputenv("QT_WAYLAND_DISABLE_FIXED_POSITIONS", {});
    const bool qpaVariable = qEnvironmentVariableIsSet("QT_QPA_PLATFORM");
    detectPlatform(argc, argv);
    // Set desktop file id before app initialization so Wayland app_id is correct
    // from the earliest possible point for privileged interface checks.
    QGuiApplication::setDesktopFileName(QString::fromLatin1(Latte::App::DESKTOPFILENAME));
    ensureKdeSessionEnvironment();
    // main() removes the QT_QPA_PLATFORM that detectPlatform() forced, unless the user set it.
    return qpaVariable;
}

//! Prints "<component> <version>" for --version and -v; needs no Wayland session.
inline int printVersion()
{
    KLocalizedString::setApplicationDomain(Latte::App::TRANSLATIONDOMAIN);
    configureAboutData();
    std::printf("%s %s\n",
                qPrintable(KAboutData::applicationData().componentName()),
                qPrintable(KAboutData::applicationData().version()));
    return 0;
}

//! Loads private.app in this executable; packaging CI runs it with LATTE_PRIVATE_APP_HOST_SMOKE=1.
inline int runPrivateAppHostSmoke()
{
    KLocalizedString::setApplicationDomain(Latte::App::TRANSLATIONDOMAIN);
    QQmlEngine engine;
    engine.addImportPath(QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("../qml")));

    QQmlComponent component(&engine);
    component.setData(QByteArrayLiteral("import QtQml\n"
                                        "import org.kde.latte.private.app 0.1 as LatteApp\n"
                                        "LatteApp.ContextMenuLayer {}"),
                      QUrl(QStringLiteral("latte-private-app-host-smoke.qml")));
    QObject *root = component.create();

    if (!root) {
        qCritical() << "private.app host smoke failed:" << component.errors();
        return 1;
    }

    // This process is the real exporting application executable. Loading
    // the module here verifies private.app's host symbol contract, which a
    // standalone QQmlEngine test cannot establish.
    delete root;
    qInfo() << "private.app loaded in latte-dock-ng host process";
    return 0;
}

//! Translation domain, QML disk cache policy, KNS compat, window icon and quit policy for the QApplication.
inline void prepareApplication(QApplication &app)
{
    KLocalizedString::setApplicationDomain(Latte::App::TRANSLATIONDOMAIN);

    // Automatically clear stale QML disk cache when the installed version changes.
    // This prevents "works in user-mode, broken after ebuild install" regressions
    // where the QML engine loads old compiled files from ~/.cache/lattedock/qmlcache.
    autoClearQmlCacheOnVersionChange();

    //! Set up user-local QML module overrides so the KNS download dialog
    //! (opened by "Download New Plasma Widgets") renders correctly.  This
    //! works around the Qt 6.10.3 incompatibility in Kirigami's
    //! DrawerHandle.qml without modifying system files.
    ensureKnsCompat();

    app.setWindowIcon(QIcon::fromTheme(Latte::App::ICONNAME));
    //protect from closing app when changing to "alternative session" and back
    app.setQuitOnLastWindowClosed(false);
}

//! Help, version and the user-facing command-line options.
inline void addCommandLineOptions(QCommandLineParser &parser)
{
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOptions({
        {{QStringLiteral("r"), QStringLiteral("replace")}, i18nc("command line", "Replace the current Latte instance.")}
        , {{QStringLiteral("d"), QStringLiteral("debug")}, i18nc("command line", "Show the debugging messages on stdout.")}
        , {{QStringLiteral("cc"), QStringLiteral("clear-cache")}, i18nc("command line", "Clear qml cache. It can be useful after system upgrades.")}
        , {QStringLiteral("enable-autostart"), i18nc("command line", "Enable autostart for this application")}
        , {QStringLiteral("disable-autostart"), i18nc("command line", "Disable autostart for this application")}
        , {QStringLiteral("default-layout"), i18nc("command line", "Import and load default layout on startup.")}
        , {QStringLiteral("available-layouts"), i18nc("command line", "Print available layouts")}
        , {QStringLiteral("available-dock-templates"), i18nc("command line", "Print available dock templates")}
        , {QStringLiteral("available-layout-templates"), i18nc("command line", "Print available layout templates")}
        , {QStringLiteral("layout"), i18nc("command line", "Load specific layout on startup."), i18nc("command line: load", "layout_name")}
        , {QStringLiteral("import-layout"), i18nc("command line", "Import and load a layout."), i18nc("command line: import", "absolute_filepath")}
        , {QStringLiteral("suggested-layout-name"), i18nc("command line", "Suggested layout name when importing a layout file"), i18nc("command line: import", "suggested_name")}
        , {QStringLiteral("import-full"), i18nc("command line", "Import full configuration."), i18nc("command line: import", "file_name")}
        , {QStringLiteral("add-dock"), i18nc("command line", "Add Dock"), i18nc("command line: add", "template_name")}
        , {QStringLiteral("single"), i18nc("command line", "Single layout memory mode. Only one layout is active at any case.")}
        , {QStringLiteral("multiple"), i18nc("command line", "Multiple layouts memory mode. Multiple layouts can be active at any time based on Activities running.")}
    });
}

//! Hidden options for developer and debugging usage; parsed, but not listed in --help.
inline void addHiddenDeveloperOptions(QCommandLineParser &parser)
{
    //! START: Hidden options for Developer and Debugging usage
    QCommandLineOption graphicsOption(QStringList() << QStringLiteral("graphics"));
    graphicsOption.setDescription(QStringLiteral("Draw boxes around of the applets."));
    graphicsOption.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(graphicsOption);

    QCommandLineOption withWindowOption(QStringList() << QStringLiteral("with-window"));
    withWindowOption.setDescription(QStringLiteral("Open a window with much debug information"));
    withWindowOption.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(withWindowOption);

    QCommandLineOption maskOption(QStringList() << QStringLiteral("mask"));
    maskOption.setDescription(QStringLiteral("Show messages of debugging for the mask (Only useful to devs)."));
    maskOption.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(maskOption);

    QCommandLineOption timersOption(QStringList() << QStringLiteral("timers"));
    timersOption.setDescription(QStringLiteral("Show messages for debugging the timers (Only useful to devs)."));
    timersOption.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(timersOption);

    QCommandLineOption spacersOption(QStringList() << QStringLiteral("spacers"));
    spacersOption.setDescription(QStringLiteral("Show visual indicators for debugging spacers (Only useful to devs)."));
    spacersOption.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(spacersOption);

    QCommandLineOption overloadedIconsOption(QStringList() << QStringLiteral("overloaded-icons"));
    overloadedIconsOption.setDescription(QStringLiteral("Show visual indicators for debugging overloaded applets icons (Only useful to devs)."));
    overloadedIconsOption.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(overloadedIconsOption);

    QCommandLineOption edgesOption(QStringList() << QStringLiteral("kwinedges"));
    edgesOption.setDescription(QStringLiteral("Show visual window indicators for hidden screen edge windows."));
    edgesOption.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(edgesOption);

    QCommandLineOption localGeometryOption(QStringList() << QStringLiteral("localgeometry"));
    localGeometryOption.setDescription(QStringLiteral("Show visual window indicators for calculated local geometry."));
    localGeometryOption.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(localGeometryOption);

    QCommandLineOption layouterOption(QStringList() << QStringLiteral("layouter"));
    layouterOption.setDescription(QStringLiteral("Show visual debug tags for items sizes."));
    layouterOption.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(layouterOption);

    QCommandLineOption filterDebugTextOption(QStringList() << QStringLiteral("debug-text"));
    filterDebugTextOption.setDescription(QStringLiteral("Show only debug messages that contain specific text."));
    filterDebugTextOption.setFlags(QCommandLineOption::HiddenFromHelp);
    filterDebugTextOption.setValueName(i18nc("command line: debug-text", "filter_debug_text"));
    parser.addOption(filterDebugTextOption);

    QCommandLineOption filterDebugInputMask(QStringList() << QStringLiteral("input"));
    filterDebugInputMask.setDescription(QStringLiteral("Show visual window indicators for calculated input mask."));
    filterDebugInputMask.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(filterDebugInputMask);

    QCommandLineOption filterDebugEventSinkMask(QStringList() << QStringLiteral("events-sink"));
    filterDebugEventSinkMask.setDescription(QStringLiteral("Show visual indicators for areas of EventsSink."));
    filterDebugEventSinkMask.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(filterDebugEventSinkMask);

    QCommandLineOption filterDebugLogCmd(QStringList() << QStringLiteral("log-file"));
    filterDebugLogCmd.setDescription(QStringLiteral("Forward debug output in a log text file."));
    filterDebugLogCmd.setFlags(QCommandLineOption::HiddenFromHelp);
    filterDebugLogCmd.setValueName(i18nc("command line: log-filepath", "filter_log_filepath"));
    parser.addOption(filterDebugLogCmd);
    //! END: Hidden options
}

//! Autostart toggles and --available-* listings; returns true when one ran, before any dock or lock exists.
inline bool runOneShotCommand(const QCommandLineParser &parser)
{
    if (parser.isSet(QStringLiteral("enable-autostart"))) {
        Latte::Layouts::Importer::enableAutostart();

        qGuiApp->exit();
        return true;
    }

    if (parser.isSet(QStringLiteral("disable-autostart"))) {
        Latte::Layouts::Importer::disableAutostart();

        qGuiApp->exit();
        return true;
    }

    //! print available-layouts
    if (parser.isSet(QStringLiteral("available-layouts"))) {
        QStringList layouts = Latte::Layouts::Importer::availableLayouts();

        if (layouts.count() > 0) {
            qInfo() << i18n("Available layouts that can be used to start Latte:");

            for (const auto &layout : std::as_const(layouts)) {
                qInfo() << "     " << layout;
            }
        } else {
            qInfo() << i18n("There are no available layouts, during startup Default will be used.");
        }

        qGuiApp->exit();
        return true;
    }

    //! print available-layout-templates
    if (parser.isSet(QStringLiteral("available-layout-templates"))) {
        QStringList templates = Latte::Layouts::Importer::availableLayoutTemplates();

        if (templates.count() > 0) {
            qInfo() << i18n("Available layout templates found in your system:");

            for (const auto &templatename : std::as_const(templates)) {
                qInfo() << "     " << templatename;
            }
        } else {
            qInfo() << i18n("There are no available layout templates in your system.");
        }

        qGuiApp->exit();
        return true;
    }

    //! print available-dock-templates
    if (parser.isSet(QStringLiteral("available-dock-templates"))) {
        QStringList templates = Latte::Layouts::Importer::availableViewTemplates();

        if (templates.count() > 0) {
            qInfo() << i18n("Available dock templates found in your system:");

            for (const auto &templatename : std::as_const(templates)) {
                qInfo() << "     " << templatename;
            }
        } else {
            qInfo() << i18n("There are no available dock templates in your system.");
        }

        qGuiApp->exit();
        return true;
    }

    return false;
}

//! Quit-on-signal policy: KSignalHandler watches SIGTERM and SIGHUP, and a KJob cannot lock quit().
inline void watchShutdownSignals()
{
    //! Follow Plasma 6 plasmashell's shutdown pattern exactly:
    //!   1. KSignalHandler (signalfd) watches SIGTERM → calls app.quit()
    //!   2. setQuitLockEnabled(false) — prevents KJob from blocking quit
    //!   3. setQuitOnLastWindowClosed(false) — already set above
    //!
    //! Reference: plasma-workspace/shell/main.cpp (bug 470604)
    //! plasmashell also sets AA_DisableSessionManager to opt out of the
    //! legacy XSMP-based Qt session protocol on Wayland.
    //!
    //! SIGINT is excluded from KSignalHandler because it is handled by the
    //! self-pipe fallback below, which is more reliable across platforms.
    KSignalHandler::self()->watchSignal(SIGTERM);
    KSignalHandler::self()->watchSignal(SIGHUP);

    //! Prevent KJob and friends from locking quit() — same as plasmashell.
    //! Without this, an in-flight KJob can silently suppress QCoreApplication::quit(),
    //! causing latte-dock to stay alive and trigger the KSMServer "wait or cancel" dialog.
    QCoreApplication::setQuitLockEnabled(false);
}

//! Routes KSignalHandler::signalReceived to the session-ending marker and to quit().
inline void connectSignalShutdown(QApplication &app, const std::function<void()> &markSessionEnding)
{
    //! Signal handler: the primary shutdown trigger (matches plasmashell).
    QObject::connect(KSignalHandler::self(), &KSignalHandler::signalReceived,
    &app, [&app, markSessionEnding](int signal) {
        qInfo() << "[shutdown] KSignalHandler received signal" << signal << "→ calling quit()";
        markSessionEnding();
        app.quit();
    });
}

//! Disables Qt session restoration; commitData and saveState requests never quit the dock.
inline void connectSessionManagement(QApplication &app)
{
    //! Match plasmashell: disable Qt session restoration, but do not quit from
    //! commitDataRequest/saveStateRequest.  Those requests can happen while
    //! the logout confirmation is still cancellable.
    auto disableSessionManagement = [](QSessionManager & sm) {
        qInfo() << "[shutdown] session management disabled → RestartNever.";
        sm.setRestartHint(QSessionManager::RestartNever);
    };

    QObject::connect(&app, &QGuiApplication::commitDataRequest, &app, disableSessionManagement, Qt::DirectConnection);
    QObject::connect(&app, &QGuiApplication::saveStateRequest, &app, disableSessionManagement, Qt::DirectConnection);
}

//! Reads --default-layout and --layout before the instance lock; returns false when the named layout is missing.
inline bool chooseStartupLayout(const QCommandLineParser &parser, StartupLayoutOptions &startup)
{
    //! --default-layout option
    if (parser.isSet(QStringLiteral("default-layout"))) {
        startup.defaultLayout = true;
    } else if (parser.isSet(QStringLiteral("layout"))) {
        startup.layoutName = parser.value(QStringLiteral("layout"));

        if (!Latte::Layouts::Importer::layoutExists(startup.layoutName)) {
            qInfo() << i18nc("layout missing", "This layout doesn't exist in the system.");
            qGuiApp->exit();
            return false;
        }
    }

    return true;
}

//! Per-user lock file that makes latte-dock-ng a single instance.
inline QString singleInstanceLockPath()
{
    QString username = QString::fromLocal8Bit(qgetenv("USER"));

    if (username.isEmpty())
        username = QString::fromLocal8Bit(qgetenv("USERNAME"));

    return QDir::tempPath() + QLatin1Char('/') + QString::fromLatin1(Latte::App::BINARYNAME) + QLatin1Char('.') + username + QLatin1String(".lock");
}

//! Asks a running instance to quit for --replace and --import-full; returns the lock timeout in milliseconds.
inline int stopReplacedInstance(const QCommandLineParser &parser, const QLockFile &lockFile)
{
    int timeout {100};

    if (parser.isSet(QStringLiteral("replace")) || parser.isSet(QStringLiteral("import-full"))) {
        qint64 pid{ -1};

        if (lockFile.getLockInfo(&pid, nullptr, nullptr)) {
            kill(static_cast<pid_t>(pid), SIGTERM);
            // Wait up to 5 seconds for the old instance to release the lock.
            // Avoids blocking forever if the old process cannot process SIGTERM
            // (e.g. signalfd delivery failure).
            timeout = 5000;
        }
    }

    return timeout;
}

//! Forwards this start to the running instance over D-Bus, or reports why it cannot take over.
inline void forwardToRunningInstance(const QCommandLineParser &parser, const QLockFile &lockFile)
{
    bool addview{parser.isSet(QStringLiteral("add-dock"))};
    bool importlayout{parser.isSet(QStringLiteral("import-layout"))};
    bool enableautostart{parser.isSet(QStringLiteral("enable-autostart"))};
    bool disableautostart{parser.isSet(QStringLiteral("disable-autostart"))};

    bool validaction{false};

    if (addview) {
        validaction = true;
        QDBusMessage msg = QDBusMessage::createMethodCall(
                               QStringLiteral("org.kde.lattedock"),
                               QStringLiteral("/Latte"), QString(),
                               QStringLiteral("addView"));
        msg.setArguments({static_cast<uint>(0), parser.value(QStringLiteral("add-dock"))});
        QDBusConnection::sessionBus().call(msg);
        return;
    } else if (importlayout) {
        validaction = true;
        QString suggestedname = parser.isSet(QStringLiteral("suggested-layout-name")) ? parser.value(QStringLiteral("suggested-layout-name")) : QString();
        QDBusMessage msg = QDBusMessage::createMethodCall(
                               QStringLiteral("org.kde.lattedock"),
                               QStringLiteral("/Latte"), QString(),
                               QStringLiteral("importLayoutFile"));
        msg.setArguments({parser.value(QStringLiteral("import-layout")), suggestedname});
        QDBusConnection::sessionBus().call(msg);
        return;
    } else if (enableautostart || disableautostart) {
        validaction = true;
    } else {
        // LayoutPage = 0
        QDBusMessage msg = QDBusMessage::createMethodCall(
                               QStringLiteral("org.kde.lattedock"),
                               QStringLiteral("/Latte"), QString(),
                               QStringLiteral("showSettingsWindow"));
        msg.setArguments({0});
        QDBusConnection::sessionBus().call(msg);
    }

    if (!validaction) {
        if (parser.isSet(QStringLiteral("replace")) || parser.isSet(QStringLiteral("import-full"))) {
            qint64 pid{ -1};

            if (lockFile.getLockInfo(&pid, nullptr, nullptr)) {
                qInfo() << i18n("Old instance (PID %1) did not exit in time. Try killing it manually.", pid);
            } else {
                qInfo() << i18n("Failed to acquire lock. Another instance may be starting.");
            }
        } else {
            qInfo() << i18n("An instance is already running!, use --replace to restart Latte");
        }
    }
}

//! Options that change the configuration or the QML cache; they run only while this instance holds the lock. Returns false when an import failed.
inline bool applyPostLockOptions(const QCommandLineParser &parser, StartupLayoutOptions &startup)
{
    //! clear-cache option
    if (parser.isSet(QStringLiteral("clear-cache"))) {
        QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QStringLiteral("/lattedock/qmlcache"));

        if (cacheDir.exists()) {
            cacheDir.removeRecursively();
            qCDebug(latteApp) << "Cache directory found and cleared...";
        }
    }

    //! import-full option
    if (parser.isSet(QStringLiteral("import-full"))) {
        bool imported = Latte::Layouts::Importer::importHelper(parser.value(QStringLiteral("import-full")));

        if (!imported) {
            qInfo() << i18n("The configuration cannot be imported");
            qGuiApp->exit();
            return false;
        }
    }

    //! import-layout option
    if (parser.isSet(QStringLiteral("import-layout"))) {
        QString suggestedname = parser.isSet(QStringLiteral("suggested-layout-name")) ? parser.value(QStringLiteral("suggested-layout-name")) : QString();
        QString importedLayout = Latte::Layouts::Importer::importLayoutHelper(parser.value(QStringLiteral("import-layout")), suggestedname);

        if (importedLayout.isEmpty()) {
            qInfo() << i18n("The layout cannot be imported");
            qGuiApp->exit();
            return false;
        } else {
            startup.layoutName = importedLayout;
        }
    }

    //! memory usage option
    if (parser.isSet(QStringLiteral("multiple"))) {
        startup.memoryUsage = static_cast<int>(Latte::MemoryUsage::MultipleLayouts);
    } else if (parser.isSet(QStringLiteral("single"))) {
        startup.memoryUsage = static_cast<int>(Latte::MemoryUsage::SingleLayout);
    }

    //! add-dock usage option
    if (parser.isSet(QStringLiteral("add-dock"))) {
        QString viewTemplateName = parser.value(QStringLiteral("add-dock"));
        QStringList viewTemplates = Latte::Layouts::Importer::availableViewTemplates();

        if (viewTemplates.contains(viewTemplateName)) {
            if (startup.layoutName.isEmpty()) {
                //! Clean layout template is applied and proper name is used
                QString emptytemplatepath = Latte::Layouts::Importer::layoutTemplateSystemFilePath(QLatin1String(Latte::Templates::EMPTYLAYOUTTEMPLATENAME));
                QString suggestedname = parser.isSet(QStringLiteral("suggested-layout-name")) ? parser.value(QStringLiteral("suggested-layout-name")) : viewTemplateName;
                QString importedLayout = Latte::Layouts::Importer::importLayoutHelper(emptytemplatepath, suggestedname);

                if (importedLayout.isEmpty()) {
                    qInfo() << i18n("The layout cannot be imported");
                    qGuiApp->exit();
                    return false;
                } else {
                    startup.layoutName = importedLayout;
                }
            }

            startup.addViewTemplateName = viewTemplateName;
        }
    }

    return true;
}

//! Routes Qt messages through the --debug, --mask, --debug-text and --log-file filters.
inline void installDebugMessageHandler(const QCommandLineParser &parser)
{
    //! text filter for debug messages
    if (parser.isSet(QStringLiteral("debug-text"))) {
        filterDebugMessageText = parser.value(QStringLiteral("debug-text"));
    }

    //! log file for debug output
    if (parser.isSet(QStringLiteral("log-file")) && !parser.value(QStringLiteral("log-file")).isEmpty()) {
        filterDebugLogFile = parser.value(QStringLiteral("log-file"));
    }

    //! debug/mask options
    if (parser.isSet(QStringLiteral("debug")) || parser.isSet(QStringLiteral("mask")) || parser.isSet(QStringLiteral("debug-text"))) {
        qInstallMessageHandler(filterDebugMessageOutput);
    } else {
        const auto noMessageOutput = [](QtMsgType, const QMessageLogContext &, const QString &) {};

        qInstallMessageHandler(noMessageOutput);
    }
}

inline bool shouldUseUserLocalQmlImports(int argc, char **argv)
{
    if (qEnvironmentVariableIntValue("LATTE_FORCE_USER_LOCAL_QML_IMPORTS") == 1) {
        return true;
    }

    if (qEnvironmentVariableIntValue("LATTE_DISABLE_USER_LOCAL_QML_IMPORTS") == 1) {
        return false;
    }

    QString executable;

    if (argc > 0 && argv && argv[0]) {
        executable = QStandardPaths::findExecutable(QString::fromLocal8Bit(argv[0]));

        if (executable.isEmpty()) {
            executable = QFileInfo(QString::fromLocal8Bit(argv[0])).canonicalFilePath();
        }
    }

    if (executable.isEmpty()) {
        return false;
    }

    const QString userLocalBinPrefix = QDir::homePath() + QStringLiteral("/.local/bin/");
    return executable.startsWith(userLocalBinPrefix);
}

// Collected at startup, applied to each engine via addLatteQmlImportPaths().
// The lists are function-local statics, not namespace-scope objects. A
// namespace-scope QStringList is constructed during static initialization, whose
// order across translation units is unspecified, so a static initializer elsewhere
// could use it before it exists. clazy's non-pod-global-static also does not
// report a static without an initializer in a file named main.cpp. The accessor
// constructs each list on its first call, so a reader always gets a constructed list.
static QStringList &userLocalQmlPathList()
{
    static QStringList paths;
    return paths;
}

static QStringList &userLocalPluginPathList()
{
    static QStringList paths;
    return paths;
}

const QStringList &userLocalQmlImportPaths()
{
    return userLocalQmlPathList();
}

void addLatteQmlImportPaths(QQmlEngine *engine)
{
    if (!engine) {
        return;
    }

    const QStringList currentPaths = engine->importPathList();

    for (const QString &path : std::as_const(userLocalQmlPathList())) {
        if (!currentPaths.contains(path)) {
            engine->addImportPath(path);
        }
    }

    const QString knsRoot = knsCompatUserQmlRoot();

    if (!knsRoot.isEmpty() && !currentPaths.contains(knsRoot)) {
        engine->addImportPath(knsRoot);
    }
}

// Collect user-local QML and plugin paths at startup without modifying
// environment variables.  The paths are later applied per-engine via
// addLatteQmlImportPaths() and per-process via QCoreApplication::addLibraryPath().
inline void collectUserLocalPaths(int argc, char **argv)
{
    if (!shouldUseUserLocalQmlImports(argc, argv)) {
        return;
    }

    const QString userLocalPath = QDir::homePath() + QStringLiteral("/.local");
    QStringList qmlCandidates;
    const auto addQmlCandidate = [&qmlCandidates](const QString & candidate) {
        const QString cleaned = QDir::cleanPath(candidate);

        if (!cleaned.isEmpty() && !qmlCandidates.contains(cleaned)) {
            qmlCandidates << cleaned;
        }
    };

    const auto addUserLocalForSystemQml = [&addQmlCandidate, &userLocalPath](const QString & systemQmlPath) {
        if (systemQmlPath.startsWith(QStringLiteral("/usr/local/"))) {
            addQmlCandidate(userLocalPath + QLatin1Char('/') + systemQmlPath.mid(11));
        } else if (systemQmlPath.startsWith(QStringLiteral("/usr/"))) {
            addQmlCandidate(userLocalPath + QLatin1Char('/') + systemQmlPath.mid(5));
        }
    };

    addUserLocalForSystemQml(QLibraryInfo::path(QLibraryInfo::QmlImportsPath));
    addQmlCandidate(userLocalPath + QStringLiteral("/lib/qt6/qml"));
    addQmlCandidate(userLocalPath + QStringLiteral("/lib64/qt6/qml"));
    addQmlCandidate(userLocalPath + QStringLiteral("/lib/x86_64-linux-gnu/qt6/qml"));

    for (const QString &candidate : std::as_const(qmlCandidates)) {
        if (QFileInfo::exists(candidate)) {
            userLocalQmlPathList() << candidate;
        }
    }

    // Containmentactions plugins (e.g. org.kde.latte.contextmenu) are .so
    // files under <prefix>/lib*/plugins/plasma/containmentactions/. The
    // user-local install puts them in ~/.local/lib*/plugins, which Qt
    // doesn't search by default — so Plasma's PluginLoader silently falls
    // back to the system .so (or fails to find anything) and right-click
    // on the dock loses its menu. Add the user-local plugin dirs via
    // QCoreApplication::addLibraryPath() instead of env vars.
    const QStringList pluginCandidates{
        userLocalPath + QStringLiteral("/lib/plugins"),
        userLocalPath + QStringLiteral("/lib64/plugins"),
        userLocalPath + QStringLiteral("/lib/x86_64-linux-gnu/plugins")
    };

    for (const QString &candidate : pluginCandidates) {
        if (QFileInfo::exists(candidate)) {
            userLocalPluginPathList() << candidate;
        }
    }
}

// Apply user-local plugin paths via QCoreApplication::addLibraryPath().
// Must be called after QApplication is created.  Unlike the old env var
// approach (QT_PLUGIN_PATH via qputenv), this does not leak to child
// processes.
inline void applyUserLocalPluginPaths()
{
    for (const QString &path : std::as_const(userLocalPluginPathList())) {
        QCoreApplication::addLibraryPath(path);
    }
}

inline void configureQtQuickGraphicsPreference()
{
    const bool hasExplicitGraphicsOverride = qEnvironmentVariableIsSet("QT_QUICK_BACKEND")
        || qEnvironmentVariableIsSet("QSG_RHI_BACKEND")
        || qEnvironmentVariableIsSet("QT_OPENGL");

    if (hasExplicitGraphicsOverride) {
        qInfo() << "Latte Dock respecting explicit Qt Quick graphics override";
        return;
    }

    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    qInfo() << "Latte Dock requested Qt Quick OpenGL rendering";
}

inline void ensureKdeSessionEnvironment()
{
    // Latte relies on KDE session metadata for launcher -> desktop file
    // resolution (app names/icons in task model). When started from stripped
    // environments (e.g. remote shell), these variables may be missing.
    if (qEnvironmentVariableIsEmpty("DESKTOP_SESSION")) {
        qputenv("DESKTOP_SESSION", "plasma");
    }

    if (qEnvironmentVariableIsEmpty("XDG_CURRENT_DESKTOP")) {
        qputenv("XDG_CURRENT_DESKTOP", "KDE");
    }

    if (qEnvironmentVariableIsEmpty("XDG_MENU_PREFIX")) {
        qputenv("XDG_MENU_PREFIX", "plasma-");
    }

    if (qEnvironmentVariableIsEmpty("KDE_FULL_SESSION")) {
        qputenv("KDE_FULL_SESSION", "true");
    }

    if (qEnvironmentVariableIsEmpty("KDE_SESSION_VERSION")) {
        qputenv("KDE_SESSION_VERSION", "6");
    }
}

inline void autoClearQmlCacheOnVersionChange()
{
    const QString cachePath = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation)
                              + QStringLiteral("/lattedock/qmlcache");
    const QString versionFilePath = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation)
                                    + QStringLiteral("/lattedock/qmlcache_version");
    const QString currentVersion = QStringLiteral(VERSION) + QLatin1Char('-')
                                   + QString::fromLatin1(Latte::App::QMLCACHEREVISION);

    QFile versionFile(versionFilePath);
    QString cachedVersion;

    if (versionFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        cachedVersion = QString::fromUtf8(versionFile.readAll()).trimmed();
        versionFile.close();
    }

    if (cachedVersion != currentVersion) {
        QDir cacheDir(cachePath);

        if (cacheDir.exists()) {
            cacheDir.removeRecursively();
            qCDebug(latteApp) << "QML cache cleared — version changed from"
                              << (cachedVersion.isEmpty() ? QStringLiteral("(none)") : cachedVersion)
                              << "to" << currentVersion;
        }
    }

    // Ensure parent directory exists before writing the version marker.
    QFileInfo versionFileInfo(versionFilePath);
    QDir().mkpath(versionFileInfo.absolutePath());

    if (versionFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        versionFile.write(currentVersion.toUtf8());
        versionFile.close();
    }
}

//! POSIX signal handler for SIGTERM/SIGINT/SIGHUP.
//! Qt6 catches these signals and translates them to application events,
//! but the default translation may not set session-ending flags early
//! enough. This handler ensures the session-ending property is set
//! immediately so the Corona fast-shutdown path activates.
//!
//! During system shutdown the Wayland compositor may disconnect before
//! the Qt event loop processes the signal → quit translation.  Setting
//! the session-ending marker on the signal handler side guarantees that
inline void filterDebugMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // `context` (function/line) is only printed in debug builds.
    Q_UNUSED(context)

    if (msg.endsWith(
            QStringLiteral("QML Binding: Not restoring previous value because restoreMode has not been set.This behavior is deprecated.In Qt < 6.0 the default is Binding.RestoreBinding.In Qt >= 6.0 the default is Binding.RestoreBindingOrValue."))
        || msg.endsWith(
            QStringLiteral("QML Binding: Not restoring previous value because restoreMode has not been set.\nThis behavior is deprecated.\nYou have to import QtQml 2.15 after any QtQuick imports and set\nthe restoreMode of the binding to fix this warning.\nIn Qt < 6.0 the default is Binding.RestoreBinding.\nIn Qt >= 6.0 the default is Binding.RestoreBindingOrValue.\n"))
        || msg.endsWith(
            QStringLiteral("QML Binding: Not restoring previous value because restoreMode has not been set.\nThis behavior is deprecated.\nYou have to import QtQml 2.15 after any QtQuick imports and set\nthe restoreMode of the binding to fix this warning.\nIn Qt < 6.0 the default is Binding.RestoreBinding.\nIn Qt >= 6.0 the default is Binding.RestoreBindingOrValue."))
        || msg.endsWith(QStringLiteral("QML Connections: Implicitly defined onFoo properties in Connections are deprecated. Use this syntax instead: function onFoo(<arguments>) { ... }"))
        || msg.contains(QStringLiteral("Toolbox not loading, toolbox package is either invalid or disabled."))
        || (msg.contains(QStringLiteral("Could not find required file \"mainscript\" for package \"/usr/share/plasma/plasmoids/org.kde.plasma."))
            && msg.contains(QStringLiteral("should be QList(\"ui/main.qml\")")))
        || msg.contains(QStringLiteral("qrc:/qt/qml/org/kde/plasma/components/ScrollView.qml"))
        || msg.contains(QStringLiteral("qrc:/qt/qml/org/kde/plasma/components/ScrollBar.qml"))
        || msg.startsWith(QStringLiteral("QFont::setPointSizeF: Point size <= 0 (0.000000), must be greater than 0"))
        // Plasma theme SVG rendering — cosmetic only: an SVG element with
        // invalid path data is truncated, and an oversized render request
        // (transient dock geometry during startup) is refused.
        || msg.startsWith(QStringLiteral("Invalid path data; path truncated."))
        || msg == QStringLiteral("The requested buffer size is too big, ignoring")
        || (msg.contains(QStringLiteral("QModelIndex(")) && msg.contains(QStringLiteral("is not valid (expected valid)")))
        || (msg.contains(QStringLiteral("Member palette of the object")) && msg.contains(QStringLiteral("overrides a member of the base object")))
        // Property shadowing in KDE frameworks — not actionable in this project.
        || (msg.contains(QStringLiteral("Member visible of the object PlasmaQuick::Dialog")) && msg.contains(QStringLiteral("overrides")))
        || (msg.contains(QStringLiteral("Member enabled of the object DeclarativeDropArea")) && msg.contains(QStringLiteral("overrides")))
        || (msg.contains(QStringLiteral("Member enabled of the object DeclarativeDragArea")) && msg.contains(QStringLiteral("overrides")))
        || (msg.contains(QStringLiteral("Member implicitHeight of the object Button_QMLTYPE")) && msg.contains(QStringLiteral("overrides")))
        || (msg.contains(QStringLiteral("Member implicitWidth of the object HeaderSwitch_QMLTYPE")) && msg.contains(QStringLiteral("overrides")))
        || (msg.contains(QStringLiteral("Member implicitHeight of the object HeaderSwitch_QMLTYPE")) && msg.contains(QStringLiteral("overrides")))
        // Qt 6 internal: Drag.imageSource from grabToImage triggers this
        // even when targetSize is passed — harmless.
        || msg.contains(QLatin1String("sourceSize request for image url that came from grabToImage"))
        // Plasma notifications applet in Latte's system tray may race plasmashell
        // for system-level DBus names. plasmashell owning them is expected.
        || msg == QLatin1String("Failed to register Notification service on DBus")
        || msg == QLatin1String("Failed to register JobViewServer service on DBus, is kuiserver running?")
        || msg == QLatin1String("Failed to register JobViewServer DBus object")
        || msg == QLatin1String("<Unknown File>: QML ToolTipDialog: location should be set before showing popup window")
        // Plasma digital clock Tooltip — internal TypeError, harmless.
        || msg.contains(QStringLiteral("digitalclock/Tooltip.qml:40: TypeError"))
        // Plasma clipboard applet — QML type mismatch with Plasma 6 framework.
        || (msg.contains(QStringLiteral("org.kde.plasma.clipboard")) && msg.contains(QStringLiteral("error when loading")))) {
        //! block warnings from dependencies that still ship legacy QML snippets.
        //! this project requires Qt 6.6+, so warnings related to Qt < 6 fallback code are irrelevant here.
        //! this also filters a known Qt/Plasma startup warning from workspace calendar internals.
        return;
    }

    if (!filterDebugMessageText.isEmpty() && !msg.contains(filterDebugMessageText)) {
        return;
    }

#ifndef QT_NO_DEBUG
    const char *function = context.function ? context.function : "";
#endif

    QString typeStr;

    switch (type) {
        case QtDebugMsg:
            typeStr = QStringLiteral("Debug");
            break;

        case QtInfoMsg:
            typeStr = QStringLiteral("Info");
            break;

        case QtWarningMsg:
            typeStr = QStringLiteral("Warning");
            break;

        case QtCriticalMsg:
            typeStr = QStringLiteral("Critical");
            break;

        case QtFatalMsg:
            typeStr = QStringLiteral("Fatal");
            break;
    };

    const char *TypeColor;

    if (type == QtInfoMsg || type == QtWarningMsg) {
        TypeColor = CGREEN;
    } else if (type == QtCriticalMsg || type == QtFatalMsg) {
        TypeColor = CRED;
    } else {
        TypeColor = CIGREEN;
    }

    if (filterDebugLogFile.isEmpty()) {
        qCDebug(latteApp).nospace() << TypeColor << "[" << qPrintable(typeStr) << " : " << CGREEN << QTime::currentTime().toString(QStringLiteral("h:mm:ss.zz")).toStdString().c_str() << TypeColor << "]" <<
                                    CNORMAL
#ifndef QT_NO_DEBUG
                                    << CIRED << " [" << CCYAN << function << CIRED << ":" << CCYAN << context.line << CIRED << "]"
#endif
                                    << CICYAN << " - " << CNORMAL << msg;
    } else {
        QFile logfile(filterDebugLogFile);

        if (!logfile.open(QIODevice::WriteOnly | QIODevice::Append)) {
            return;
        }

        QTextStream logts(&logfile);
        logts << "[" << qPrintable(typeStr) << " : " << QTime::currentTime().toString(QStringLiteral("h:mm:ss.zz")).toStdString().c_str() << "]"
              <<  " - " << msg << Qt::endl;
    }
}

inline void configureAboutData()
{
    // Keep the historical internal component id to preserve D-Bus service compatibility
    // with existing callers that address org.kde.lattedock.
    // The configured VERSION is shared by --version and the Help/About dialog.
    // Release builds override it; local Debug installs can use an older source
    // default, so neither UI translations nor runtime tags may supply a version.
    KAboutData about(QStringLiteral("lattedock")
                     , QStringLiteral("Latte Dock NG")
                     , QStringLiteral(VERSION)
                     , i18n("Latte Dock NG is a Wayland-first dock for KDE Plasma 6.3+ that provides an elegant and"
           "intuitive experience for your tasks and widgets. It animates its contents "
           "using a parabolic zoom effect and stays out of the way when not needed."
           "\n\nSpecial thanks to the original Latte Dock authors and contributors."
           "\n\n\"Art in Coffee\"")
                     , KAboutLicense::GPL_V3
                     , QStringLiteral("(C) 2024-2026 Ruizhi Zhong"));

    about.setHomepage(QStringLiteral(WEBSITE));
    about.setBugAddress(BUG_ADDRESS);
    about.setProgramLogo(QIcon::fromTheme(Latte::App::ICONNAME));
    about.setDesktopFileName(QString::fromLatin1(Latte::App::DESKTOPFILENAME));
    about.setProductName(QByteArray("lattedock"));

    // Author
    about.addAuthor(QStringLiteral("Ruizhi Zhong"), QString(), QStringLiteral("ruizhi.zhong88@gmail.com"));

    // Acknowledgement
    about.addCredit(QStringLiteral("Latte Dock Authors and Contributors"),
                    i18n("This project is based on Latte Dock. Thanks to all original authors and contributors."));

    KAboutData::setApplicationData(about);
}

//! used the version provided by PW:KWorkspace
inline void detectPlatform(int argc, char **argv)
{
    if (qEnvironmentVariableIsSet("QT_QPA_PLATFORM")) {
        return;
    }

    for (int i = 0; i < argc; i++) {
        if (qstrcmp(argv[i], "-platform") == 0 ||
            qstrcmp(argv[i], "--platform") == 0 ||
            QByteArray(argv[i]).startsWith("-platform=") ||
            QByteArray(argv[i]).startsWith("--platform=")) {
            return;
        }
    }

    qputenv("QT_QPA_PLATFORM", "wayland");
}
