/*
    SPDX-FileCopyrightText: 2016 Smith AR <audoban@openmailbox.org>
    SPDX-FileCopyrightText: 2016 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0

import org.kde.plasma.core 2.0 as PlasmaCore

import org.kde.latte.components 1.0 as LatteComponents

Item {
    id: background

    readonly property int contentWidth: progressCircle.width + 0.1*height
    readonly property bool hasTheme: (typeof theme !== "undefined") && theme
    readonly property color fallbackBackgroundColor: "#2d2d2d"
    readonly property color fallbackTextColor: "#f3f3f3"
    readonly property color fallbackHighlightedTextColor: "#f3f3f3"
    readonly property color fallbackHighlightColor: "#3daee9"

    Item {
        id: subRectangle
        width: contentWidth
        height: parent.height / 2

        states: [
            State {
                name: "default"
                when: (root.location !== PlasmaCore.Types.RightEdge)

                AnchorChanges {
                    target: subRectangle
                    anchors{ top:parent.top; bottom:undefined; left:undefined; right:parent.right;}
                }
            },
            State {
                name: "right"
                when: (root.location === PlasmaCore.Types.RightEdge)

                AnchorChanges {
                    target: subRectangle
                    anchors{ top:parent.top; bottom:undefined; left:parent.left; right:undefined;}
                }
            }
        ]

        LatteComponents.BadgeText {
            id: progressCircle
            anchors.centerIn: parent
            minimumWidth: Math.min(0.8 * parent.height, maximumWidth)
            maximumWidth: {
                if (showsAudioBadge) {
                    return height;
                } else if ( index === taskItem.abilities.indexer.firstVisibleItemIndex && !taskItem.containsMouse) {
                    var shadowSize = (taskItem.abilities.myView.itemShadow.isEnabled ? taskItem.abilities.myView.itemShadow.size/2 : 0);
                    return (taskItem.parabolicItem.zoom * (taskItem.abilities.metrics.iconSize - shadowSize));
                } else {
                    return 999999;
                }
            }
            height: 0.8 * parent.height
            numberValue: {
                if (taskIcon.smartLauncherItem && (taskIcon.smartLauncherItem.countVisible || taskIcon.smartLauncherItem.progressVisible)) {
                    return taskIcon.smartLauncherItem.count;
                } else if (taskItem.badgeIndicator > 0) {
                    return taskItem.badgeIndicator;
                }

                return 0;
            }
            fullCircle: true
            showNumber: true

            color: hasTheme && theme.backgroundColor !== undefined ? theme.backgroundColor : fallbackBackgroundColor
            textColor: {
                if (showsInfoBadge) {
                    return root.infoBadgeProminentColorEnabled
                            ? prominentTextColor
                            : ((hasTheme && theme.highlightedTextColor !== undefined) ? theme.highlightedTextColor : fallbackHighlightedTextColor)
                }

                return (hasTheme && theme.textColor !== undefined) ? theme.textColor : fallbackTextColor;
            }
            borderColor: root.lightTextColor

            highlightedColor: {
                if (showsInfoBadge) {
                    return root.infoBadgeProminentColorEnabled
                            ? prominentBackColor
                            : ((hasTheme && theme.highlightColor !== undefined) ? theme.highlightColor : fallbackHighlightColor)
                }

                return (hasTheme && theme.buttonFocusColor !== undefined) ? theme.buttonFocusColor : fallbackHighlightColor;
            }

            style3d: taskItem.abilities.myView.badgesIn3DStyle
            textWithBackgroundColor: false

            proportion: {
                if (taskIcon.smartLauncherItem && taskIcon.smartLauncherItem.progressVisible) {
                    return taskIcon.smartLauncherItem.progress / 100;
                }

                if (taskItem.badgeIndicator > 0 || (taskIcon.smartLauncherItem && taskIcon.smartLauncherItem.countVisible)) {
                    return 1;
                }

                return 0;
            }

            readonly property color prominentBackColor: "#cc0000" //redish  (deprecated: theme.negativeTextColor)
            readonly property color prominentTextColor: "#f3f3f3" //whitish (deprecated: root.lightTextColor)

            readonly property bool showsInfoBadge: ((taskItem.badgeIndicator > 0)
                                                    || (taskIcon.smartLauncherItem && taskIcon.smartLauncherItem.countVisible && !taskIcon.smartLauncherItem.progressVisible))

            readonly property bool showsAudioBadge: root.showAudioBadge && taskItem.hasAudioStream && taskItem.playingAudio && !taskItem.isSeparator
        }
    }
}
