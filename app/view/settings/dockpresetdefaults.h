/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LATTE_DOCKPRESETDEFAULTS_H
#define LATTE_DOCKPRESETDEFAULTS_H

#include <KConfigGroup>
#include <KConfigLoader>
#include <KConfigPropertyMap>
#include <QStringList>

namespace Latte::ViewPart::Config::DockPreset {

// Only editor options belong to a preset. Layout identity, applet order,
// launchers, migration flags and per-widget contents must never be reset.
// The shipped Dock template overrides schema defaults; schemas remain the
// authority for options not explicitly set in that template.
inline QStringList
dockKeys()
{
    return {
        QStringLiteral("activeWindowFilter"),
        QStringLiteral("alignment"),
        QStringLiteral("animationsEnabled"),
        QStringLiteral("appletShadowsEnabled"),
        QStringLiteral("appletsZoomEnabled"),
        QStringLiteral("autoSizeEnabled"),
        QStringLiteral("backgroundAllCorners"),
        QStringLiteral("backgroundColorScheduleEnabled"),
        QStringLiteral("backgroundDayStart"),
        QStringLiteral("backgroundNightStart"),
        QStringLiteral("backgroundOnlyOnMaximized"),
        QStringLiteral("backgroundRadius"),
        QStringLiteral("backgroundShadowSize"),
        QStringLiteral("blurEnabled"),
        QStringLiteral("closeActiveWindowEnabled"),
        QStringLiteral("customBackgroundColor"),
        QStringLiteral("customBackgroundColorEnabled"),
        QStringLiteral("customNightBackgroundColor"),
        QStringLiteral("disablePanelShadowForMaximized"),
        QStringLiteral("dragActiveWindowEnabled"),
        QStringLiteral("durationTime"),
        QStringLiteral("floatingGapHidingWaitsMouse"),
        QStringLiteral("floatingGapIsMirrored"),
        QStringLiteral("floatingInternalGapIsForced"),
        QStringLiteral("hideFloatingGapForMaximized"),
        QStringLiteral("iconSize"),
        QStringLiteral("itemsAlignment"),
        QStringLiteral("lastDodgeVisibilityMode"),
        QStringLiteral("lastWindowsVisibilityMode"),
        QStringLiteral("lengthExtMargin"),
        QStringLiteral("maxLength"),
        QStringLiteral("maximizeWhenMaximized"),
        QStringLiteral("minLength"),
        QStringLiteral("mouseWheelActions"),
        QStringLiteral("offset"),
        QStringLiteral("panelOutline"),
        QStringLiteral("panelShadows"),
        QStringLiteral("panelSize"),
        QStringLiteral("panelTransparency"),
        QStringLiteral("plasmaBackgroundForPopups"),
        QStringLiteral("proportionIconSize"),
        QStringLiteral("screenEdgeMargin"),
        QStringLiteral("scrollAction"),
        QStringLiteral("shadowColor"),
        QStringLiteral("shadowColorType"),
        QStringLiteral("shadowOpacity"),
        QStringLiteral("shadowSize"),
        QStringLiteral("solidBackgroundForMaximized"),
        QStringLiteral("themeColors"),
        QStringLiteral("thickMargin"),
        QStringLiteral("titleTooltips"),
        QStringLiteral("useThemePanel"),
        QStringLiteral("windowColors"),
        QStringLiteral("windowGap"),
        QStringLiteral("zoomLevel"),
    };
}

inline QStringList
taskKeys()
{
    return {
        QStringLiteral("animationLauncherBouncing"),
        QStringLiteral("animationNewWindowSliding"),
        QStringLiteral("animationWindowAddedInGroup"),
        QStringLiteral("animationWindowInAttention"),
        QStringLiteral("animationWindowRemovedFromGroup"),
        QStringLiteral("audioBadgeActionsEnabled"),
        QStringLiteral("autoScrollTasksEnabled"),
        QStringLiteral("groupTasksByDefault"),
        QStringLiteral("hideAllTasks"),
        QStringLiteral("hoverAction"),
        QStringLiteral("infoBadgeProminentColorEnabled"),
        QStringLiteral("isPreferredForDroppedLaunchers"),
        QStringLiteral("isPreferredForPositionShortcuts"),
        QStringLiteral("launchersGroup"),
        QStringLiteral("leftClickAction"),
        QStringLiteral("manualScrollTasksType"),
        QStringLiteral("middleClickAction"),
        QStringLiteral("modifier"),
        QStringLiteral("modifierClick"),
        QStringLiteral("modifierClickAction"),
        QStringLiteral("previewWindowAsPopup"),
        QStringLiteral("scrollTasksEnabled"),
        QStringLiteral("showAudioBadge"),
        QStringLiteral("showInfoBadge"),
        QStringLiteral("showOnlyCurrentActivity"),
        QStringLiteral("showOnlyCurrentDesktop"),
        QStringLiteral("showOnlyCurrentScreen"),
        QStringLiteral("showProgressBadge"),
        QStringLiteral("showWindowActions"),
        QStringLiteral("showWindowsOnlyFromLaunchers"),
        QStringLiteral("taskScrollAction"),
    };
}

inline bool
restore(KConfigLoader *schema, KConfigPropertyMap *map, const QStringList &keys, const KConfigGroup &preset)
{
    if (!schema || !map) {
        return false;
    }

    for (const QString &key : keys) {
        auto *item = schema->findItemByName(key);
        if (!item || item->isImmutable() || map->isImmutable(key)) {
            continue;
        }
        const QVariant value = preset.isValid() ? preset.readEntry(key, item->getDefault()) : item->getDefault();
        if (map->value(key) != value) {
            map->insert(key, value);
            // Programmatic insert updates bindings but not valueChanged. Emit
            // the same notification as an editor write so existing persistence
            // and applet observers see the reset instead of stale cached values.
            Q_EMIT map->valueChanged(key, value);
        }
    }
    map->writeConfig();
    return true;
}
}

#endif
