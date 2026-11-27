/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein.org>
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef TASKTOOLS_H
#define TASKTOOLS_H

#include <QIcon>
#include <QUrl>

#include <KService>
#include <KSharedConfig>

namespace Latte {
namespace WindowSystem {

struct AppData
{
    QString id; // Application id (*.desktop sans extension).
    QString name; // Application name.
    QString genericName; // Generic application name.
    QIcon icon;
    QUrl url;
    bool skipTaskbar = false;
};

/**
 * Fills in and returns an AppData struct based on the given URL.
 *
 * If the URL contains iconData in its query string, it is decoded and
 * set as AppData.icon, taking precedence over normal icon discovery.
 *
 * If the URL is using the preferred:// scheme, the URL it resolves to
 * is set as AppData.url.
 *
 * The supplied fallback icon is set as AppData.icon if no other icon
 * could be found.
 *
 * @see defaultApplication
 * @param url A URL to a .desktop file or executable, or a preferred:// URL.
 * @param fallbackIcon An icon to use when none could be read from the URL or
 * otherwise found.
 * @returns @c AppData filled in based on the given URL.
 */
AppData appDataFromUrl(const QUrl &url, const QIcon &fallbackIcon = QIcon());

/**
 * Takes several bits of window metadata as input and tries to find
 * the .desktop file for the application owning this window, or,
 * failing that, the path to its executable.
 *
 * The source for the metadata is generally the window's appId on
 * Wayland, or the window class part of the WM_CLASS window property
 * on X Windows.
 *
 * TODO: The supplied config object can contain various mapping and
 * mangling rules that affect the behavior of this function, allowing
 * to map bits of metadata to different values and other things. This
 * config file format still needs to be documented fully; in the
 * meantime the bundled default rules in taskmanagerrulesrc (the
 * config file opened by various models in this library) can be used
 * for reference.
 *
 * @param appId A string uniquely identifying the application owning
 * the window, ideally matching a .desktop file name.
 * @param pid The process id for the process owning the window.
 * @param config A KConfig object parameterizing the matching
 * behavior.
 * @param xWindowsWMClassName The instance name part of X Windows'
 * WM_CLASS window property.
 * @returns A .desktop file or executable path for the application
 * owning the window.
 */
QUrl windowUrlFromMetadata(const QString &appId, quint32 pid = 0,
                           KSharedConfig::Ptr config = KSharedConfig::Ptr(), const QString &xWindowsWMClassName = QString());

/**
 * Returns a list of (usually application) KService instances for the
 * given process id, by examining the process and querying the service
 * database for process metadata.
 *
 * @param pid A process id.
 * @param rulesConfig A KConfig object parameterizing the matching
 * behavior.
 * @returns A list of KService instances.
 */
KService::List servicesFromPid(quint32 pid,
                               KSharedConfig::Ptr rulesConfig = KSharedConfig::Ptr());

/**
 * Returns a list of (usually application) KService instances for the
 * given process command line and process name, by mangling the command
 * line in various ways and checking the data against the Exec keys in
 * the service database. Mangling is done e.g. to check for executable
 * names with and without paths leading to them and to ignore arguments.
 * if needed.
 *
 * The [Settings]TryIgnoreRuntimes key in the supplied config object can
 * hold a comma-separated list of runtime executables that this code will
 * try to ignore in the process command line. This is useful in cases where
 * the command line has the contents of a .desktop Exec key prefixed with
 * a runtime executable. The code tries to strip the path to the runtime
 * executable if needed.
 *
 * @param cmdLine A process command line.
 * @param processName The process name.
 * @param rulesConfig A KConfig object parameterizing the matching
 * behavior.
 * @returns A list of KService instances.
 */
KService::List servicesFromCmdLine(const QString &cmdLine, const QString &processName,
                                   KSharedConfig::Ptr rulesConfig = KSharedConfig::Ptr());

/**
 * Returns an application id for an URL using the preferred:// scheme.
 *
 * Recognized values for the host component of the URL are:
 * - "browser"
 * - "mailer"
 * - "terminal"
 * - "filemanager"
 * - "systemsettings"
 *
 * If the host component matches none of the above, an attempt is made
 * to match to application links stored in kcm_componentchooser/.
 *
 * @param url A URL using the preferred:// scheme.
 * @returns an application id for the given URL.
 **/
QString defaultApplication(const QUrl &url);

}
}

#endif
