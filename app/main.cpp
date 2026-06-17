/*
    SPDX-FileCopyrightText: 2016 Smith AR <audoban@openmailbox.org>
    SPDX-FileCopyrightText: 2016 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-FileCopyrightText: 2024-2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

// local
#include "config-latte.h"
#include "apptypes.h"
#include "knscompat.h"
#include "lattecorona.h"
#include "layouts/importer.h"
#include "templates/templatesmanager.h"

// C
#include <csignal>

// C++
#include <memory>

// Qt
#include <QApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QQuickWindow>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QSessionManager>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>

// KDE
#include <KLocalizedString>
#include <KAboutData>
#include <KCoreAddons/KSignalHandler>
#include <KDBusService>
#include <KQuickAddons/QtQuickSettings>
#include <KWindowSystem>
#include <plasmaquick/sharedqmlengine.h>

//! COLORS
#define CNORMAL  "\x1b[0m"
#define CIGREEN  "\x1b[1;32m"
#define CGREEN   "\x1b[0;32m"
#define CICYAN   "\x1b[1;36m"
#define CCYAN    "\x1b[0;36m"
#define CIRED    "\x1b[1;31m"
#define CRED     "\x1b[0;31m"

inline void configureAboutData();
inline void detectPlatform(int argc, char **argv);
inline void filterDebugMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);
inline bool shouldUseUserLocalQmlImports(int argc, char **argv);
inline void ensureUserLocalQmlImportPaths(int argc, char **argv);
inline void ensureKdeSessionEnvironment();
inline bool isKdeSessionShuttingDown();
inline void autoClearQmlCacheOnVersionChange();

QString filterDebugMessageText;
QString filterDebugLogFile;

int main(int argc, char **argv)
{
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    // Keep fractional scale behavior deterministic under Wayland.
    // Qt6 enables High DPI by default, so we only sanitize legacy env overrides.
    if (!qEnvironmentVariableIsSet("PLASMA_USE_QT_SCALING")) {
        qunsetenv("QT_DEVICE_PIXEL_RATIO");
    }

    QQuickWindow::setDefaultAlphaBuffer(true);
    ensureUserLocalQmlImportPaths(argc, argv);

    qputenv("QT_WAYLAND_DISABLE_FIXED_POSITIONS", {});
    const bool qpaVariable = qEnvironmentVariableIsSet("QT_QPA_PLATFORM");
    detectPlatform(argc, argv);
    // Set desktop file id before app initialization so Wayland app_id is correct
    // from the earliest possible point for privileged interface checks.
    QGuiApplication::setDesktopFileName(QString::fromLatin1(Latte::App::DESKTOPFILENAME));
    ensureKdeSessionEnvironment();
    QApplication app(argc, argv);
    qunsetenv("QT_WAYLAND_DISABLE_FIXED_POSITIONS");

    if (!KWindowSystem::isPlatformWayland()) {
        qCritical() << "Latte-Dock Wayland-only build requires a Wayland Plasma session.";
        return 1;
    }

    if (!qpaVariable) {
        // don't leak the env variable to processes we start
        qunsetenv("QT_QPA_PLATFORM");
    }

    KQuickAddons::QtQuickSettings::init();

    KLocalizedString::setApplicationDomain(Latte::App::TRANSLATIONDOMAIN);

    // Automatically clear stale QML disk cache when the installed version changes.
    // This prevents "works in user-mode, broken after ebuild install" regressions
    // where the QML engine loads old compiled files from ~/.cache/lattedock/qmlcache.
    autoClearQmlCacheOnVersionChange();

    //! Prime the Plasma global shared QML engine singleton early.
    //! plasmashell creates a process-wide shared QQmlEngine via
    //! SharedQmlEnginePrivate::engine() (a static weak_ptr).  When
    //! KNSWidgets::Dialog later opens its internal QQuickWidget, the
    //! engine singleton is already initialised and types resolved through
    //! it take precedence, avoiding the incompatible AOT-compiled
    //! DrawerHandle.qml path from libKirigamiTemplates.so.
    static std::shared_ptr<PlasmaQuick::SharedQmlEngine> s_sharedEngine =
        std::make_shared<PlasmaQuick::SharedQmlEngine>(&app);

    //! Set up user-local QML module overrides so the KNS download dialog
    //! (opened by "Download New Plasma Widgets") renders correctly.  This
    //! works around the Qt 6.10.3 incompatibility in Kirigami's
    //! DrawerHandle.qml without modifying system files.
    ensureKnsCompat();

    app.setWindowIcon(QIcon::fromTheme(QString::fromLatin1(Latte::App::ICONNAME)));
    //protect from closing app when changing to "alternative session" and back
    app.setQuitOnLastWindowClosed(false);

    configureAboutData();

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOptions({
                          {{"r", "replace"}, i18nc("command line", "Replace the current Latte instance.")}
                          , {{"d", "debug"}, i18nc("command line", "Show the debugging messages on stdout.")}
                          , {{"cc", "clear-cache"}, i18nc("command line", "Clear qml cache. It can be useful after system upgrades.")}
                          , {"enable-autostart", i18nc("command line", "Enable autostart for this application")}
                          , {"disable-autostart", i18nc("command line", "Disable autostart for this application")}
                          , {"default-layout", i18nc("command line", "Import and load default layout on startup.")}
                          , {"available-layouts", i18nc("command line", "Print available layouts")}
                          , {"available-dock-templates", i18nc("command line", "Print available dock templates")}
                          , {"available-layout-templates", i18nc("command line", "Print available layout templates")}
                          , {"layout", i18nc("command line", "Load specific layout on startup."), i18nc("command line: load", "layout_name")}
                          , {"import-layout", i18nc("command line", "Import and load a layout."), i18nc("command line: import", "absolute_filepath")}
                          , {"suggested-layout-name", i18nc("command line", "Suggested layout name when importing a layout file"), i18nc("command line: import", "suggested_name")}
                          , {"import-full", i18nc("command line", "Import full configuration."), i18nc("command line: import", "file_name")}
                          , {"add-dock", i18nc("command line", "Add Dock"), i18nc("command line: add", "template_name")}
                          , {"single", i18nc("command line", "Single layout memory mode. Only one layout is active at any case.")}
                          , {"multiple", i18nc("command line", "Multiple layouts memory mode. Multiple layouts can be active at any time based on Activities running.")}
                      });

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

    parser.process(app);

    if (parser.isSet(QStringLiteral("enable-autostart"))) {
        Latte::Layouts::Importer::enableAutostart();
    }

    if (parser.isSet(QStringLiteral("disable-autostart"))) {
        Latte::Layouts::Importer::disableAutostart();
        qGuiApp->exit();
        return 0;
    }

    //! print available-layouts
    if (parser.isSet(QStringLiteral("available-layouts"))) {
        QStringList layouts = Latte::Layouts::Importer::availableLayouts();

        if (layouts.count() > 0) {
            qInfo() << i18n("Available layouts that can be used to start Latte:");

            for (const auto &layout : layouts) {
                qInfo() << "     " << layout;
            }
        } else {
            qInfo() << i18n("There are no available layouts, during startup Default will be used.");
        }

        qGuiApp->exit();
        return 0;
    }

    //! print available-layout-templates
    if (parser.isSet(QStringLiteral("available-layout-templates"))) {
        QStringList templates = Latte::Layouts::Importer::availableLayoutTemplates();

        if (templates.count() > 0) {
            qInfo() << i18n("Available layout templates found in your system:");

            for (const auto &templatename : templates) {
                qInfo() << "     " << templatename;
            }
        } else {
            qInfo() << i18n("There are no available layout templates in your system.");
        }

        qGuiApp->exit();
        return 0;
    }

    //! print available-dock-templates
    if (parser.isSet(QStringLiteral("available-dock-templates"))) {
        QStringList templates = Latte::Layouts::Importer::availableViewTemplates();

        if (templates.count() > 0) {
            qInfo() << i18n("Available dock templates found in your system:");

            for (const auto &templatename : templates) {
                qInfo() << "     " << templatename;
            }
        } else {
            qInfo() << i18n("There are no available dock templates in your system.");
        }

        qGuiApp->exit();
        return 0;
    }

    //! Follow Plasma 6 plasmashell's shutdown pattern exactly:
    //!   1. KSignalHandler (signalfd) watches SIGTERM → calls app.quit()
    //!   2. setQuitLockEnabled(false) — prevents KJob from blocking quit
    //!   3. setQuitOnLastWindowClosed(false) — already set above
    //!
    //! Reference: plasma-workspace/shell/main.cpp (bug 470604)
    //! plasmashell also sets AA_DisableSessionManager to opt out of the
    //! legacy XSMP-based Qt session protocol on Wayland.
    KSignalHandler::self()->watchSignal(SIGTERM);
    KSignalHandler::self()->watchSignal(SIGINT);
    KSignalHandler::self()->watchSignal(SIGHUP);

    //! Prevent KJob and friends from locking quit() — same as plasmashell.
    //! Without this, an in-flight KJob can silently suppress QCoreApplication::quit(),
    //! causing latte-dock to stay alive and trigger the KSMServer "wait or cancel" dialog.
    QCoreApplication::setQuitLockEnabled(false);

    auto markSessionEnding = [&app]() {
        if (!qEnvironmentVariableIsSet("LATTE_SESSION_ENDING")) {
            qputenv("LATTE_SESSION_ENDING", "1");
        }

        if (!app.property("latte_session_ending").toBool()) {
            app.setProperty("latte_session_ending", true);
            qInfo() << "[shutdown] session-ending flag set; fast teardown enabled.";
        }
    };

    //! Signal handler: the primary shutdown trigger (matches plasmashell).
    QObject::connect(KSignalHandler::self(), &KSignalHandler::signalReceived,
                     &app, [&app, &markSessionEnding](int signal) {
        qInfo() << "[shutdown] KSignalHandler received signal" << signal << "→ calling quit()";
        markSessionEnding();
        app.quit();
    });

    //! commitDataRequest: legacy XSMP path (may not fire on pure Wayland).
    //! When it does fire, treat it the same as SIGTERM — the session IS ending.
    QObject::connect(&app, &QGuiApplication::commitDataRequest, &app, [&app, &markSessionEnding](QSessionManager &sm) {
        qInfo() << "[shutdown] commitDataRequest received → RestartNever + quit()";
        sm.setRestartHint(QSessionManager::RestartNever);
        QMetaObject::invokeMethod(&app, [&app, &markSessionEnding]() {
            markSessionEnding();
            app.quit();
        }, Qt::QueuedConnection);
    }, Qt::DirectConnection);

    //! saveStateRequest: may fire during session checkpointing (not just
    //! logout), so only set the flag – do NOT call quit().
    QObject::connect(&app, &QGuiApplication::saveStateRequest, &app, [&app, &markSessionEnding](QSessionManager &sm) {
        qInfo() << "[shutdown] saveStateRequest received → RestartNever (no quit)";
        sm.setRestartHint(QSessionManager::RestartNever);
        QMetaObject::invokeMethod(&app, markSessionEnding, Qt::QueuedConnection);
    }, Qt::DirectConnection);

    //! Polling fallback: on Wayland, ksmserver may not send commitDataRequest.
    //! Poll isShuttingDown() via DBus with a SHORT timeout (1 s) to avoid
    //! blocking the main thread during compositor teardown.
    //!
    //! Two-phase behaviour:
    //!   Phase 1 (confirmation dialog): isShuttingDown() = true → set flag, do NOT quit.
    //!     The user may still cancel the logout at this point.
    //!   Phase 2 (committed): flag was set ≥ 5 s ago AND isShuttingDown() still true.
    //!     The confirmation phase has passed; treat this as a committed logout and quit.
    //!   Cancel: isShuttingDown() = false after flag was set → clear flag.
    QElapsedTimer flagSetTimer;
    flagSetTimer.invalidate();

    QTimer sessionShutdownPoll;
    sessionShutdownPoll.setInterval(500);
    QObject::connect(&sessionShutdownPoll, &QTimer::timeout, [&app, &markSessionEnding, &flagSetTimer]() {
        const bool shuttingDown = isKdeSessionShuttingDown();

        if (app.property("latte_session_ending").toBool()) {
            if (!shuttingDown) {
                // User cancelled the logout confirmation.
                qInfo() << "[shutdown] logout cancelled; clearing session-ending flag.";
                app.setProperty("latte_session_ending", false);
                qunsetenv("LATTE_SESSION_ENDING");
                flagSetTimer.invalidate();
                return;
            }

            // Flag is set and ksmserver still reports shutting down.
            // If the flag was set ≥ 5 seconds ago, the confirmation phase
            // has almost certainly passed — treat this as a committed logout
            // and call quit().  This is the fallback for Wayland sessions
            // where commitDataRequest never fires.
            if (flagSetTimer.isValid() && flagSetTimer.hasExpired(5000)) {
                qInfo() << "[shutdown] session-ending flag active for 5+ s;"
                        << "commitDataRequest did not fire, triggering quit() from poller.";
                app.quit();
            }
            return;
        }

        if (shuttingDown) {
            // Phase 1: logout confirmation dialog is showing.
            // Set the flag but do NOT quit — user may still cancel.
            qInfo() << "[shutdown] isShuttingDown() = true; setting flag (not quitting yet).";
            markSessionEnding();
            flagSetTimer.start();
        }
    });
    sessionShutdownPoll.start();

    //! choose layout for startup
    bool defaultLayoutOnStartup = false;
    int memoryUsage = -1;
    QString layoutNameOnStartup = "";
    QString addViewTemplateNameOnStartup = "";

    //! --default-layout option
    if (parser.isSet(QStringLiteral("default-layout"))) {
        defaultLayoutOnStartup = true;
    } else if (parser.isSet(QStringLiteral("layout"))) {
        layoutNameOnStartup = parser.value(QStringLiteral("layout"));

        if (!Latte::Layouts::Importer::layoutExists(layoutNameOnStartup)) {
            qInfo() << i18nc("layout missing", "This layout doesn't exist in the system.");
            qGuiApp->exit();
            return 0;
        }
    }

    //! --replace option
    QString username = qgetenv("USER");

    if (username.isEmpty())
        username = qgetenv("USERNAME");

    QLockFile lockFile {QDir::tempPath() + "/" + QString::fromLatin1(Latte::App::BINARYNAME) + "." + username + ".lock"};

    int timeout {100};

    if (parser.isSet(QStringLiteral("replace")) || parser.isSet(QStringLiteral("import-full"))) {
        qint64 pid{ -1};

        if (lockFile.getLockInfo(&pid, nullptr, nullptr)) {
            kill(static_cast<pid_t>(pid), SIGTERM);
            timeout = -1;
        }
    }

    if (!lockFile.tryLock(timeout)) {
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
            msg.setArguments({(uint)0, parser.value(QStringLiteral("add-dock"))});
            QDBusConnection::sessionBus().call(msg);
            qGuiApp->exit();
            return 0;
        } else if (importlayout) {
            validaction = true;
            QString suggestedname = parser.isSet(QStringLiteral("suggested-layout-name")) ? parser.value(QStringLiteral("suggested-layout-name")) : QString();
            QDBusMessage msg = QDBusMessage::createMethodCall(
                QStringLiteral("org.kde.lattedock"),
                QStringLiteral("/Latte"), QString(),
                QStringLiteral("importLayoutFile"));
            msg.setArguments({parser.value(QStringLiteral("import-layout")), suggestedname});
            QDBusConnection::sessionBus().call(msg);
            qGuiApp->exit();
            return 0;
        } else if (enableautostart || disableautostart){
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
            qInfo() << i18n("An instance is already running!, use --replace to restart Latte");
        }

        qGuiApp->exit();
        return 0;
    }

    //! clear-cache option
    if (parser.isSet(QStringLiteral("clear-cache"))) {
        QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QStringLiteral("/lattedock/qmlcache"));

        if (cacheDir.exists()) {
            cacheDir.removeRecursively();
            qDebug() << "Cache directory found and cleared...";
        }
    }

    //! import-full option
    if (parser.isSet(QStringLiteral("import-full"))) {
        bool imported = Latte::Layouts::Importer::importHelper(parser.value(QStringLiteral("import-full")));

        if (!imported) {
            qInfo() << i18n("The configuration cannot be imported");
            qGuiApp->exit();
            return 0;
        }
    }

    //! import-layout option
    if (parser.isSet(QStringLiteral("import-layout"))) {
        QString suggestedname = parser.isSet(QStringLiteral("suggested-layout-name")) ? parser.value(QStringLiteral("suggested-layout-name")) : QString();
        QString importedLayout = Latte::Layouts::Importer::importLayoutHelper(parser.value(QStringLiteral("import-layout")), suggestedname);

        if (importedLayout.isEmpty()) {
            qInfo() << i18n("The layout cannot be imported");
            qGuiApp->exit();
            return 0;
        } else {
            layoutNameOnStartup = importedLayout;
        }
    }

    //! memory usage option
    if (parser.isSet(QStringLiteral("multiple"))) {
        memoryUsage = static_cast<int>(Latte::MemoryUsage::MultipleLayouts);
    } else if (parser.isSet(QStringLiteral("single"))) {
        memoryUsage = static_cast<int>(Latte::MemoryUsage::SingleLayout);
    }

    //! add-dock usage option
    if (parser.isSet(QStringLiteral("add-dock"))) {
        QString viewTemplateName = parser.value(QStringLiteral("add-dock"));
        QStringList viewTemplates = Latte::Layouts::Importer::availableViewTemplates();

        if (viewTemplates.contains(viewTemplateName)) {
            if (layoutNameOnStartup.isEmpty()) {
                //! Clean layout template is applied and proper name is used
                QString emptytemplatepath = Latte::Layouts::Importer::layoutTemplateSystemFilePath(Latte::Templates::EMPTYLAYOUTTEMPLATENAME);
                QString suggestedname = parser.isSet(QStringLiteral("suggested-layout-name")) ? parser.value(QStringLiteral("suggested-layout-name")) : viewTemplateName;
                QString importedLayout = Latte::Layouts::Importer::importLayoutHelper(emptytemplatepath, suggestedname);

                if (importedLayout.isEmpty()) {
                    qInfo() << i18n("The layout cannot be imported");
                    qGuiApp->exit();
                    return 0;
                } else {
                    layoutNameOnStartup = importedLayout;
                }
            }

            addViewTemplateNameOnStartup = viewTemplateName;
        }
    }

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

    int result;
    {
        Latte::Corona corona(defaultLayoutOnStartup, layoutNameOnStartup, addViewTemplateNameOnStartup, memoryUsage);
        KDBusService service(KDBusService::Unique);
        result = app.exec();
    }
    // Detach the SharedQmlEngine from the QApplication before app is
    // destroyed.  Without this, ~QApplication() → deleteChildren() deletes
    // the SharedQmlEngine that is managed by a static shared_ptr, and the
    // subsequent shared_ptr destructor double-frees it.
    if (s_sharedEngine) {
        s_sharedEngine->setParent(nullptr);
    }

    // Drain any remaining DeferredDelete events.  The Corona destructor
    // already processes its own via deleteLater() + immediate flush, so
    // this loop mainly handles events from KDBusService or other globals.
    // Limit to a few passes to avoid spinning on cascading events.
    for (int pass = 0; pass < 5; ++pass) {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
    return result;
}

inline void prependEnvironmentPath(const char *envName, const QString &path)
{
    if (path.isEmpty()) {
        return;
    }

    const QString normalizedPath = QFileInfo(path).canonicalFilePath();

    if (normalizedPath.isEmpty()) {
        return;
    }

    const QString existing = qEnvironmentVariable(envName);
    const QChar separator = QDir::listSeparator();
    const QStringList existingPaths = existing.split(separator, Qt::SkipEmptyParts);

    if (existingPaths.contains(normalizedPath)) {
        return;
    }

    if (existing.isEmpty()) {
        qputenv(envName, normalizedPath.toUtf8());
        return;
    }

    const QString updated = normalizedPath + separator + existing;
    qputenv(envName, updated.toUtf8());
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

inline void ensureUserLocalQmlImportPaths(int argc, char **argv)
{
    if (!shouldUseUserLocalQmlImports(argc, argv)) {
        return;
    }

    const QString userLocalPath = QDir::homePath() + QStringLiteral("/.local");
    const QStringList qmlCandidates{
        userLocalPath + QStringLiteral("/lib/qt6/qml"),
        userLocalPath + QStringLiteral("/lib64/qt6/qml")
    };

    for (const QString &candidate : qmlCandidates) {
        const QFileInfo info(candidate);

        if (!info.exists() || !info.isDir()) {
            continue;
        }

        prependEnvironmentPath("QML2_IMPORT_PATH", candidate);
        prependEnvironmentPath("QML_IMPORT_PATH", candidate);
        prependEnvironmentPath("QT_QML_IMPORT_PATH", candidate);
    }

    // Containmentactions plugins (e.g. org.kde.latte.contextmenu) are .so
    // files under <prefix>/lib*/plugins/plasma/containmentactions/. The
    // user-local install puts them in ~/.local/lib*/plugins, which Qt
    // doesn't search by default — so Plasma's PluginLoader silently falls
    // back to the system .so (or fails to find anything) and right-click
    // on the dock loses its menu. Prepend the user-local plugin dirs.
    const QStringList pluginCandidates{
        userLocalPath + QStringLiteral("/lib/plugins"),
        userLocalPath + QStringLiteral("/lib64/plugins"),
        userLocalPath + QStringLiteral("/lib/x86_64-linux-gnu/plugins")
    };

    for (const QString &candidate : pluginCandidates) {
        const QFileInfo info(candidate);

        if (!info.exists() || !info.isDir()) {
            continue;
        }

        prependEnvironmentPath("QT_PLUGIN_PATH", candidate);
    }
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
    const QString currentVersion = QStringLiteral(VERSION);

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
            qDebug() << "QML cache cleared — version changed from"
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

inline bool isKdeSessionShuttingDown()
{
    // Use a short timeout (1 s) to avoid blocking the main thread when
    // ksmserver is unresponsive during compositor teardown.  The default
    // DBus timeout is 25 s, which would stall the event loop and prevent
    // quit() from being processed in time.
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.kde.ksmserver"),
        QStringLiteral("/KSMServer"),
        QStringLiteral("org.kde.KSMServerInterface"),
        QStringLiteral("isShuttingDown"));
    QDBusMessage reply = QDBusConnection::sessionBus().call(msg, QDBus::Block, 1000);
    return (reply.type() == QDBusMessage::ReplyMessage
            && !reply.arguments().isEmpty()
            && reply.arguments().first().toBool());
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
    if (msg.endsWith("QML Binding: Not restoring previous value because restoreMode has not been set.This behavior is deprecated.In Qt < 6.0 the default is Binding.RestoreBinding.In Qt >= 6.0 the default is Binding.RestoreBindingOrValue.")
        || msg.endsWith("QML Binding: Not restoring previous value because restoreMode has not been set.\nThis behavior is deprecated.\nYou have to import QtQml 2.15 after any QtQuick imports and set\nthe restoreMode of the binding to fix this warning.\nIn Qt < 6.0 the default is Binding.RestoreBinding.\nIn Qt >= 6.0 the default is Binding.RestoreBindingOrValue.\n")
        || msg.endsWith("QML Binding: Not restoring previous value because restoreMode has not been set.\nThis behavior is deprecated.\nYou have to import QtQml 2.15 after any QtQuick imports and set\nthe restoreMode of the binding to fix this warning.\nIn Qt < 6.0 the default is Binding.RestoreBinding.\nIn Qt >= 6.0 the default is Binding.RestoreBindingOrValue.")
        || msg.endsWith("QML Connections: Implicitly defined onFoo properties in Connections are deprecated. Use this syntax instead: function onFoo(<arguments>) { ... }")
        || msg.contains("Toolbox not loading, toolbox package is either invalid or disabled.")
        || (msg.contains("Could not find required file \"mainscript\" for package \"/usr/share/plasma/plasmoids/org.kde.plasma.")
            && msg.contains("should be QList(\"ui/main.qml\")"))
        || msg.contains("qrc:/qt/qml/org/kde/plasma/components/ScrollView.qml")
        || msg.contains("qrc:/qt/qml/org/kde/plasma/components/ScrollBar.qml")
        || msg.startsWith("QFont::setPointSizeF: Point size <= 0 (0.000000), must be greater than 0")
        || (msg.contains("QModelIndex(") && msg.contains("is not valid (expected valid)"))
        || (msg.contains("Member palette of the object") && msg.contains("overrides a member of the base object"))
        // Property shadowing in KDE frameworks — not actionable in this project.
        || (msg.contains("Member visible of the object PlasmaQuick::Dialog") && msg.contains("overrides"))
        || (msg.contains("Member enabled of the object DeclarativeDropArea") && msg.contains("overrides"))
        || (msg.contains("Member enabled of the object DeclarativeDragArea") && msg.contains("overrides"))
        || (msg.contains("Member implicitHeight of the object Button_QMLTYPE") && msg.contains("overrides"))
        || (msg.contains("Member implicitWidth of the object HeaderSwitch_QMLTYPE") && msg.contains("overrides"))
        || (msg.contains("Member implicitHeight of the object HeaderSwitch_QMLTYPE") && msg.contains("overrides"))
        // Plasma notifications applet in Latte's system tray may race plasmashell
        // for system-level DBus names. plasmashell owning them is expected.
        || msg == QLatin1String("Failed to register Notification service on DBus")
        || msg == QLatin1String("Failed to register JobViewServer service on DBus, is kuiserver running?")
        || msg == QLatin1String("Failed to register JobViewServer DBus object")
        || msg == QLatin1String("<Unknown File>: QML ToolTipDialog: location should be set before showing popup window")
        // Plasma digital clock Tooltip — internal TypeError, harmless.
        || msg.contains("digitalclock/Tooltip.qml:40: TypeError")
        // Plasma clipboard applet — QML type mismatch with Plasma 6 framework.
        || (msg.contains("org.kde.plasma.clipboard") && msg.contains("error when loading"))) {
        //! block warnings from dependencies that still ship legacy QML snippets.
        //! this project requires Qt 6.6+, so warnings related to Qt < 6 fallback code are irrelevant here.
        //! this also filters a known Qt/Plasma startup warning from workspace calendar internals.
        return;
    }

    if (!filterDebugMessageText.isEmpty() && !msg.contains(filterDebugMessageText)) {
        return;
    }

    const char *function = context.function ? context.function : "";

    QString typeStr;
    switch (type) {
    case QtDebugMsg:
        typeStr = "Debug";
        break;
    case QtInfoMsg:
        typeStr = "Info";
        break;
    case QtWarningMsg:
        typeStr = "Warning" ;
        break;
    case QtCriticalMsg:
        typeStr = "Critical";
        break;
    case QtFatalMsg:
        typeStr = "Fatal";
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
        qDebug().nospace() << TypeColor << "[" << typeStr.toStdString().c_str() << " : " << CGREEN << QTime::currentTime().toString("h:mm:ss.zz").toStdString().c_str() << TypeColor << "]" << CNORMAL
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
        logts << "[" << typeStr.toStdString().c_str() << " : " << QTime::currentTime().toString("h:mm:ss.zz").toStdString().c_str() << "]"
              <<  " - " << msg << Qt::endl;
    }
}

inline void configureAboutData()
{
    // Keep the historical internal component id to preserve D-Bus service compatibility
    // with existing callers that address org.kde.lattedock.
    KAboutData about(QStringLiteral("lattedock")
                     , QStringLiteral("Latte Dock NG")
                     , QStringLiteral(VERSION)
                     , i18n("Latte Dock NG is a Wayland-first dock for KDE Plasma 6.5+ that provides an elegant and "
                            "intuitive experience for your tasks and widgets. It animates its contents "
                            "using a parabolic zoom effect and stays out of the way when not needed."
                            "\n\nSpecial thanks to the original Latte Dock authors and contributors."
                            "\n\n\"Art in Coffee\"")
                     , KAboutLicense::GPL_V3
                     , QStringLiteral("(C) 2024-2026 Ruizhi Zhong"));

    about.setHomepage(WEBSITE);
    about.setBugAddress(BUG_ADDRESS);
    about.setProgramLogo(QIcon::fromTheme(QString::fromLatin1(Latte::App::ICONNAME)));
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
