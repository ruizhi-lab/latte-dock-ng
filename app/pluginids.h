/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong@outlook.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LATTE_PLUGINIDS_H
#define LATTE_PLUGINIDS_H

#include <QLatin1String>
#include <QString>

namespace Latte {
namespace PluginId {

// Latte built-in applets
inline constexpr const char kSeparator[] = "org.kde.latte.separator";
inline constexpr const char kSpacer[] = "org.kde.latte.spacer";
inline constexpr const char kPlasmoid[] = "org.kde.latte.plasmoid";
inline constexpr const char kSplitter[] = "org.kde.latte.splitter";
inline constexpr const char kContainment[] = "org.kde.latte.containment";
inline constexpr const char kDefaultIndicator[] = "org.kde.latte.default";
inline constexpr const char kPlasmaIndicator[] = "org.kde.latte.plasma";
inline constexpr const char kPlasmaTabStyleIndicator[] = "org.kde.latte.plasmatabstyle";

// Legacy / third-party
inline constexpr const char kLegacySeparator[] = "audoban.applet.separator";

// Plasma system applets
inline constexpr const char kSystemTray[] = "org.kde.plasma.systemtray";
inline constexpr const char kAnalogClock[] = "org.kde.plasma.analogclock";
inline constexpr const char kDigitalClock[] = "org.kde.plasma.digitalclock";
inline constexpr const char kDesktopContainment[] = "org.kde.desktopcontainment";
inline constexpr const char kMultiTasking[] = "org.kde.plasma.multitasking";
inline constexpr const char kLauncherMenu[] = "org.kde.plasma.launchermenu";
inline constexpr const char kMarginsSeparator[] = "org.kde.plasma.marginsseparator";
inline constexpr const char kFolder[] = "org.kde.plasma.folder";

// Third-party
inline constexpr const char kNomadSystemTray[] = "org.nomad.systemtray";

// KDE session
inline constexpr const char kKsmServer[] = "org.kde.ksmserver";
inline constexpr const char kKsmServerInterface[] = "org.kde.KSMServerInterface";

// KWin effect / KDE globals
inline constexpr const char kKWinLatteWindowColors[] = "kwin/scripts/lattewindowcolors";
inline constexpr const char kKdeGlobals[] = "kdeglobals";
inline constexpr const char kPlasmaShellRc[] = "plasma-org.kde.plasma.desktop-appletsrc";

inline bool isSeparatorPluginId(const QString &pluginId)
{
    return pluginId == QLatin1String(kSeparator)
            || pluginId == QLatin1String(kLegacySeparator);
}

inline bool isLatteBuiltIn(const QString &pluginId)
{
    return pluginId == QLatin1String(kSeparator)
            || pluginId == QLatin1String(kSpacer)
            || pluginId == QLatin1String(kPlasmoid)
            || pluginId == QLatin1String(kSplitter);
}

} // namespace PluginId
} // namespace Latte

#endif // LATTE_PLUGINIDS_H
