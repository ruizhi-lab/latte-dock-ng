/*
    SPDX-FileCopyrightText: 2016 Smith AR <audoban@openmailbox.org>
    SPDX-FileCopyrightText: 2016 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0

import org.kde.plasma.plasmoid 2.0

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.private.mpris as Mpris
import org.kde.activities 0.1 as Activities
import org.kde.taskmanager 0.1 as TaskManager

import org.kde.latte.core 0.2 as LatteCore
import org.kde.latte.private.tasks 0.1 as LatteTasks

import "../code/activitiesTools.js" as ActivitiesTools

PlasmaExtras.Menu {
    id: menu

    property bool changingLayout: false

    property QtObject mpris2Source
    property QtObject backend

    property var modelIndex
    readonly property var atm: TaskManager.AbstractTasksModel

    placement: {
        if (root.location === PlasmaCore.Types.LeftEdge) {
            return PlasmaExtras.Menu.RightPosedTopAlignedPopup;
        } else if (root.location === PlasmaCore.Types.TopEdge) {
            return PlasmaExtras.Menu.BottomPosedLeftAlignedPopup;
        } else if (root.location === PlasmaCore.Types.RightEdge) {
            return PlasmaExtras.Menu.LeftPosedTopAlignedPopup;
        } else {
            return PlasmaExtras.Menu.TopPosedLeftAlignedPopup;
        }
    }

    minimumWidth: visualParent ? visualParent.width : 1

    property bool showAllPlaces: false

    readonly property string tailSeparatorText: plasmoid.formFactor === PlasmaCore.Types.Vertical ? i18n("Top Separator") :
                                                                                                    (!root.LayoutMirroring.enabled ? i18n("Left Separator") : i18n("Right Separator"))
    readonly property string headSeparatorText: plasmoid.formFactor === PlasmaCore.Types.Vertical ? i18n("Bottom Separator") :
                                                                                                    (!root.LayoutMirroring.enabled ? i18n("Right Separator") : i18n("Left Separator"))

    onStatusChanged: {
        if (visualParent && status == PlasmaExtras.Menu.Open) {
            launcherToggleAction.checked = launcherIsPinned();
            activitiesDesktopsMenu.refresh();
        } else if (status == PlasmaExtras.Menu.Closed) {
            root.contextMenu = null;
            menu.destroy();
            backend.ungrabMouse(visualParent);
        }
    }

    function get(modelProp) {
        return tasksModel.data(modelIndex, modelProp)
    }

    function normalizedLauncherUrl(url) {
        var launcherUrl = String(url || "");
        var iconDataPos = launcherUrl.indexOf("?iconData=");

        if (iconDataPos > 0) {
            launcherUrl = launcherUrl.substring(0, iconDataPos);
        }

        return launcherUrl;
    }

    function launcherUrlForPin() {
        var withoutIcon = normalizedLauncherUrl(get(atm.LauncherUrlWithoutIcon));
        if (withoutIcon !== "") {
            return withoutIcon;
        }

        return normalizedLauncherUrl(get(atm.LauncherUrl));
    }

    function launcherUrlWithIcon() {
        return normalizedLauncherUrl(get(atm.LauncherUrl));
    }

    function launcherIsPinned() {
        var withoutIcon = launcherUrlForPin();
        var withIcon = launcherUrlWithIcon();

        if (withoutIcon !== "" && tasksModel.launcherPosition(withoutIcon) !== -1) {
            return true;
        }

        if (withIcon !== "" && tasksModel.launcherPosition(withIcon) !== -1) {
            return true;
        }

        return false;
    }

    function isStartupItem() {
        return get(atm.IsStartup) === true;
    }

    function isWindowItem() {
        return get(atm.IsWindow) === true;
    }

    function canLaunchNewWindow() {
        if (typeof atm.CanLaunchNewInstance !== "undefined") {
            var canLaunch = get(atm.CanLaunchNewInstance);
            if (canLaunch !== undefined && canLaunch !== null) {
                return canLaunch === true;
            }
        }

        return !isStartupItem() && launcherUrlForPin() !== "";
    }

    function appletAction(name) {
        if (plasmoid && typeof plasmoid.action === "function") {
            return plasmoid.action(name);
        }

        if (typeof Plasmoid !== "undefined" && Plasmoid && typeof Plasmoid.action === "function") {
            return Plasmoid.action(name);
        }

        return null;
    }

    function show() {
        if (typeof Plasmoid !== "undefined"
                && Plasmoid
                && typeof Plasmoid.contextualActionsAboutToShow === "function") {
            Plasmoid.contextualActionsAboutToShow();
        }

        loadDynamicLaunchActions(launcherUrlForPin());
        loadMyViewActions();
        // backend.ungrabMouse(visualParent);
        openRelative();
        //! Hiding previews needs a delay to avoid race conditions while opening context menus.
        windowsPreviewCheckerToNotShowTimer.start();
    }

    function newMenuItem(parent) {
        return Qt.createQmlObject(
                    "import org.kde.plasma.extras 2.0 as PlasmaExtras;" +
                    "PlasmaExtras.MenuItem {}",
                    parent);
    }

    function newSeparator(parent) {
        return Qt.createQmlObject(
                    "import org.kde.plasma.extras 2.0 as PlasmaExtras;" +
                    "PlasmaExtras.MenuItem { separator: true }",
                    parent);
    }

    function dynamicSectionsForLauncherUrl(launcherUrl) {
        var placesActions = backend.placesActions(launcherUrl, showAllPlaces, menu);
        if (!placesActions) {
            placesActions = [];
        }

        var recentActions = backend.recentDocumentActions(launcherUrl, menu);
        if (!recentActions) {
            recentActions = [];
        }

        var jumpListActions = backend.jumpListActions(launcherUrl, menu);
        if (!jumpListActions) {
            jumpListActions = [];
        }

        var sections = [];

        if (placesActions.length > 0) {
            sections.push({
                              title: i18n("Places"),
                              group: "places",
                              actions: placesActions
                          });
        } else {
            sections.push({
                              title: i18n("Recent Files"),
                              group: "recents",
                              actions: recentActions
                          });
        }

        sections = sections.filter(function(section) {
                                       return section.actions.length > 0;
                                   });

        sections.push({
                          title: i18n("Actions"),
                          group: "actions",
                          actions: jumpListActions
                      });

        return sections;
    }

    function selectDynamicSections(launcherUrl) {
        var rawCandidates = [];
        var candidates = [];

        function pushRaw(value) {
            if (value === undefined || value === null) {
                return;
            }
            rawCandidates.push(String(value));
        }

        function pushCandidate(value) {
            var candidate = String(value || "");
            if (candidate === "" || candidates.indexOf(candidate) !== -1) {
                return;
            }
            candidates.push(candidate);
        }

        pushRaw(launcherUrl);
        pushRaw(get(atm.LauncherUrlWithoutIcon));
        pushRaw(get(atm.LauncherUrl));

        if (visualParent) {
            pushRaw(visualParent.launcherUrl);
            pushRaw(visualParent.launcherUrlWithIcon);

            var launcherName = String(visualParent.launcherName || "");
            if (launcherName !== "") {
                pushRaw(launcherName);
                pushRaw(launcherName + ".desktop");
                pushRaw("applications:" + launcherName);
                pushRaw("applications:" + launcherName + ".desktop");
            }
        }

        var appId = String(get(atm.AppId) || "");
        if (appId !== "") {
            pushRaw(appId);
            if (appId.indexOf("applications:") !== 0) {
                pushRaw("applications:" + appId);
                if (!appId.endsWith(".desktop")) {
                    pushRaw("applications:" + appId + ".desktop");
                }
            } else if (!appId.endsWith(".desktop")) {
                pushRaw(appId + ".desktop");
            }
        }

        for (var i = 0; i < rawCandidates.length; ++i) {
            pushCandidate(rawCandidates[i]);
            pushCandidate(normalizedLauncherUrl(rawCandidates[i]));
        }

        if (candidates.length === 0) {
            return dynamicSectionsForLauncherUrl("");
        }

        var bestSections = dynamicSectionsForLauncherUrl(candidates[0]);
        var bestScore = -1;

        for (var j = 0; j < candidates.length; ++j) {
            var sections = dynamicSectionsForLauncherUrl(candidates[j]);
            var score = 0;

            for (var k = 0; k < sections.length; ++k) {
                score += sections[k].actions.length;
            }

            if (score > bestScore) {
                bestScore = score;
                bestSections = sections;
            }
        }

        return bestSections;
    }

    function loadDynamicLaunchActions(launcherUrl) {
        var sections = selectDynamicSections(launcherUrl);

        // C++ can override section heading by returning a QString as first action
        sections.forEach((section) => {
                             if (typeof section.actions[0] === "string") {
                                 section.title = section.actions.shift(); // take first
                             }
                         });

        // QMenu does not limit its width automatically. Even if we set a maximumWidth
        // it would just cut off text rather than eliding. So we do this manually.
        var textMetrics = Qt.createQmlObject("import QtQuick 2.4; TextMetrics {}", menu);
        textMetrics.elide = Qt.ElideRight;
        var gridUnit = 16;
        if (typeof PlasmaCore !== "undefined" && PlasmaCore && PlasmaCore.Units && PlasmaCore.Units.gridUnit) {
            gridUnit = PlasmaCore.Units.gridUnit;
        } else if (typeof Kirigami !== "undefined" && Kirigami && Kirigami.Units && Kirigami.Units.gridUnit) {
            gridUnit = Kirigami.Units.gridUnit;
        }
        textMetrics.elideWidth = gridUnit * 22;

        sections.forEach(function (section) {
            if (section["actions"].length > 0 || section["group"] == "actions") {
                // Don't add the "Actions" header if the menu has nothing but actions
                // in it, because then it's redundant (all menus have actions)
                if (section["group"] != "actions" || sections.length > 1) {
                    var sectionHeader = newMenuItem(menu);
                    sectionHeader.text = section["title"];
                    sectionHeader.section = true;
                    menu.addMenuItem(sectionHeader, startNewInstanceItem);
                }
            }

            for (var i = 0; i < section["actions"].length; ++i) {
                var item = newMenuItem(menu);
                item.action = section["actions"][i];

                textMetrics.text = String(item.action.text || "").replace("&", "&&");
                item.action.text = textMetrics.elidedText;

                menu.addMenuItem(item, startNewInstanceItem);
            }
        });

        // Add Media Player control actions
        var winIdList = atm.WinIdList;

        var playerData = mpris2Source ? mpris2Source.playerForLauncherUrl(launcherUrl, get(atm.AppPid)) : null;

        if (playerData && playerData.canControl && !(get(winIdList) !== undefined && get(winIdList).length > 1)) {
            var playing = playerData.playbackStatus === Mpris.PlaybackStatus.Playing;

                var menuItem = menu.newMenuItem(menu);
                menuItem.text = i18nc("Play previous track", "Previous Track");
                menuItem.icon = "media-skip-backward";
                menuItem.enabled = Qt.binding(function() {
                    return playerData.canGoPrevious;
                });
                menuItem.clicked.connect(function() {
                    playerData.Previous();
                });
                menu.addMenuItem(menuItem, virtualDesktopsMenuItem);

                menuItem = menu.newMenuItem(menu);
                // PlasmaCore Menu doesn't actually handle icons or labels changing at runtime...
                menuItem.text = Qt.binding(function() {
                    return playing && playerData.canPause ? i18nc("Pause playback", "Pause") : i18nc("Start playback", "Play");
                });
                menuItem.icon = Qt.binding(function() {
                    return playing && playerData.canPause ? "media-playback-pause" : "media-playback-start";
                });
                menuItem.enabled = Qt.binding(function() {
                    return playing ? playerData.canPause : playerData.canPlay;
                });
                menuItem.clicked.connect(function() {
                    if (playing) {
                        playerData.Pause();
                    } else {
                        playerData.Play();
                    }
                });
                menu.addMenuItem(menuItem, virtualDesktopsMenuItem);

                menuItem = menu.newMenuItem(menu);
                menuItem.text = i18nc("Play next track", "Next Track");
                menuItem.icon = "media-skip-forward";
                menuItem.enabled = Qt.binding(function() {
                    return playerData.canGoNext;
                });
                menuItem.clicked.connect(function() {
                    playerData.Next();
                });
                menu.addMenuItem(menuItem, virtualDesktopsMenuItem);

                menuItem = menu.newMenuItem(menu);
                menuItem.text = i18nc("Stop playback", "Stop");
                menuItem.icon = "media-playback-stop";
                menuItem.enabled = Qt.binding(function() {
                    return playerData.canStop;
                });
                menuItem.clicked.connect(function() {
                    playerData.Stop();
                });
                menu.addMenuItem(menuItem, virtualDesktopsMenuItem);

                // Technically media controls and audio streams are separate but for the user they're
                // semantically related, don't add a separator inbetween.
                if (!menu.visualParent.hasAudioStream) {
                    menu.addMenuItem(newSeparator(menu), virtualDesktopsMenuItem);
                }

                // If we don't have a window associated with the player but we can quit
                // it through MPRIS we'll offer a "Quit" option instead of "Close"
                if (!closeWindowItem.visible && playerData.canQuit) {
                    menuItem = menu.newMenuItem(menu);
                    menuItem.text = i18nc("Quit media player app", "Quit");
                    menuItem.icon = "application-exit";
                    menuItem.visible = Qt.binding(function() {
                        return !closeWindowItem.visible;
                    });
                    menuItem.clicked.connect(function() {
                        playerData.Quit();
                    });
                    menu.addMenuItem(menuItem);
                }

                // If we don't have a window associated with the player but we can raise
                // it through MPRIS we'll offer a "Restore" option
                if (get(atm.IsLauncher) === true && !startNewInstanceItem.visible && playerData.canRaise) {
                    menuItem = menu.newMenuItem(menu);
                    menuItem.text = i18nc("Open or bring to the front window of media player app", "Restore");
                    menuItem.icon = playerData.iconName;
                    menuItem.visible = Qt.binding(function() {
                        return !startNewInstanceItem.visible;
                    });
                    menuItem.clicked.connect(function() {
                        playerData.Raise();
                    });
                    menu.addMenuItem(menuItem, startNewInstanceItem);
                }
        }

        // We allow mute/unmute whenever an application has a stream, regardless of whether it
        // is actually playing sound.
        // This way you can unmute, e.g. a telephony app, even after the conversation has ended,
        // so you still have it ringing later on.
        if (menu.visualParent.hasAudioStream) {
            var muteItem = menu.newMenuItem(menu);
            muteItem.checkable = true;
            muteItem.checked = Qt.binding(function() {
                return menu.visualParent && menu.visualParent.muted;
            });
            muteItem.clicked.connect(function() {
                menu.visualParent.toggleMuted();
            });
            muteItem.text = i18n("Mute");
            muteItem.icon = "audio-volume-muted";
            menu.addMenuItem(muteItem, virtualDesktopsMenuItem);

            menu.addMenuItem(newSeparator(menu), virtualDesktopsMenuItem);
        }
    }

    function loadMyViewActions() {
        if (!appletAbilities.myView.isReady) {
            return;
        }

        var actionsCount = appletAbilities.myView.containmentActions.length;

        for (var i=0; i<actionsCount; ++i) {
            var item = newMenuItem(menu);
            item.action = appletAbilities.myView.containmentActions[i];
            item.visible = Qt.binding(function() {
                return this.action.visible;
            });
            menu.addMenuItem(item, myViewActions);
        }
    }

    Component.onCompleted: {
        ActivitiesTools.launchersOnActivities = root.launchersOnActivities
        ActivitiesTools.currentActivity = activityInfo.currentActivity;
        ActivitiesTools.plasmoid = plasmoid;

        // Cannot have "Connections" as child of PlasmaCoponents.ContextMenu.
        backend.showAllPlaces.connect(function() {
            visualParent.showContextMenu({showAllPlaces: true});
        });
    }


    Component.onDestruction: {
        if (!changingLayout) {
            root.contextMenu = null;
            backend.ungrabMouse(visualParent);
        }
    }

    /// Sub Items

    PlasmaExtras.MenuItem {
        id: startNewInstanceItem
        visible: visualParent && canLaunchNewWindow()

        enabled: visible

        text: i18n("Open New Window")
        icon: "window-new"

        onClicked: tasksModel.requestNewInstance(menu.modelIndex)
    }

    PlasmaExtras.MenuItem {
        id: virtualDesktopsMenuItem

        visible: virtualDesktopInfo.numberOfDesktops > 1
                 && (visualParent && isWindowItem()
                     && !isStartupItem()
                     && get(atm.IsVirtualDesktopsChangeable) === true)

        enabled: visible

        text: i18n("Move To &Desktop")

        readonly property var virtualDesktopsMenuConnections: Connections {
            target: virtualDesktopInfo
            function onDesktopIdsChanged() { Qt.callLater(virtualDesktopsMenu.refresh) }
            function onDesktopNamesChanged() { Qt.callLater(virtualDesktopsMenu.refresh) }
            function onNumberOfDesktopsChanged() { Qt.callLater(virtualDesktopsMenu.refresh) }
        }

        readonly property var _virtualDesktopsMenu: PlasmaExtras.Menu {
            id: virtualDesktopsMenu

            visualParent: virtualDesktopsMenuItem.action

            function refresh() {
                clearMenuItems();

                if (virtualDesktopInfo.numberOfDesktops <= 1) {
                    return;
                }

                var menuItem = menu.newMenuItem(virtualDesktopsMenu);
                menuItem.text = i18n("Move &To Current Desktop");
                menuItem.enabled = Qt.binding(function() {
                    var vds = menu.get(atm.VirtualDesktops);
                    return menu.visualParent && (vds !== undefined) && vds.indexOf(virtualDesktopInfo.currentDesktop) == -1;
                });
                menuItem.clicked.connect(function() {
                    tasksModel.requestVirtualDesktops(menu.modelIndex, [virtualDesktopInfo.currentDesktop]);
                });

                menuItem = menu.newMenuItem(virtualDesktopsMenu);
                menuItem.text = i18n("&All Desktops");
                menuItem.checkable = true;
                menuItem.checked = Qt.binding(function() {
                    return menu.visualParent && menu.visualParent.m.IsOnAllVirtualDesktops === true;
                });
                menuItem.clicked.connect(function() {
                    tasksModel.requestVirtualDesktops(menu.modelIndex, []);
                });
                backend.setActionGroup(menuItem.action);

                menu.newSeparator(virtualDesktopsMenu);

                for (var i = 0; i < virtualDesktopInfo.desktopNames.length; ++i) {
                    menuItem = menu.newMenuItem(virtualDesktopsMenu);
                    //menuItem.text = i18nc("1 = number of desktop, 2 = desktop name", "%1 Desktop %2", i + 1, virtualDesktopInfo.desktopNames[i]);
                    menuItem.text = (i + 1) + ". " + virtualDesktopInfo.desktopNames[i];
                    menuItem.checkable = true;
                    menuItem.checked = Qt.binding((function(i) {
                        return function() {
                            return (menu.visualParent && menu.get(atm.VirtualDesktops).indexOf(virtualDesktopInfo.desktopIds[i]) > -1);
                        };
                    })(i));
                    menuItem.clicked.connect((function(i) {
                        return function() {
                            return tasksModel.requestVirtualDesktops(menu.modelIndex, [virtualDesktopInfo.desktopIds[i]]);
                        };
                    })(i));
                    backend.setActionGroup(menuItem.action);
                }

                menu.newSeparator(virtualDesktopsMenu);

                menuItem = menu.newMenuItem(virtualDesktopsMenu);
                menuItem.text = i18n("&New Desktop");
                menuItem.clicked.connect(function() {
                    tasksModel.requestNewVirtualDesktop(menu.modelIndex);
                });
            }

            Component.onCompleted: refresh()
        }
    }


    PlasmaExtras.MenuItem {
        id: activitiesDesktopsMenuItem

        visible: activityInfo.numberOfRunningActivities > 1
                 && (visualParent && isWindowItem()
                     && !isStartupItem())
                 && !root.disableAllWindowsFunctionality

        enabled: visible

        text: i18n("Move To &Activity")

        readonly property var activityInfoConnections: Connections {
            target: activityInfo

            function onNumberOfRunningActivitiesChanged() { activitiesDesktopsMenu.refresh() }
        }

        readonly property var _activitiesDesktopsMenu: PlasmaExtras.Menu {
            id: activitiesDesktopsMenu

            visualParent: activitiesDesktopsMenuItem.action

            function refresh() {
                clearMenuItems();

                if (activityInfo.numberOfRunningActivities <= 1) {
                    return;
                }

                var menuItem = menu.newMenuItem(activitiesDesktopsMenu);
                menuItem.text = i18n("Add To Current Activity");
                menuItem.enabled = Qt.binding(function() {
                    return menu.visualParent && menu.visualParent.m.Activities.length > 0 &&
                            menu.visualParent.m.Activities.indexOf(activityInfo.currentActivity) < 0;
                });
                menuItem.clicked.connect(function() {
                    tasksModel.requestActivities(menu.modelIndex, menu.visualParent.m.Activities.concat(activityInfo.currentActivity));
                });

                menuItem = menu.newMenuItem(activitiesDesktopsMenu);
                menuItem.text = i18n("All Activities");
                menuItem.checkable = true;
                menuItem.checked = Qt.binding(function() {
                    return menu.visualParent && menu.visualParent.m.Activities.length === 0;
                });
                menuItem.clicked.connect(function() {
                    var checked = menuItem.checked;
                    var newActivities = menu.visualParent.m.Activities;
                    var size = newActivities.length;

                    newActivities = undefined; // will cast to an empty QStringList i.e all activities
                    if (size === 0) {
                        newActivities = new Array(activityInfo.currentActivity);
                    }

                    tasksModel.requestActivities(menu.modelIndex, newActivities);
                });

                menu.newSeparator(activitiesDesktopsMenu);

                var runningActivities = activityInfo.runningActivities();
                for (var i = 0; i < runningActivities.length; ++i) {
                    var activityId = runningActivities[i];

                    menuItem = menu.newMenuItem(activitiesDesktopsMenu);
                    menuItem.text = activityInfo.activityName(runningActivities[i]);
                    menuItem.checkable = true;
                    menuItem.checked = Qt.binding( (function(activityId) {
                        return function() {
                            return menu.visualParent && menu.visualParent.m.Activities.indexOf(activityId) >= 0;
                        };
                    })(activityId));
                    menuItem.clicked.connect((function(activityId) {
                        return function () {
                            var checked = menuItem.checked;
                            var newActivities = menu.visualParent.m.Activities;
                            var index = newActivities.indexOf(activityId)

                            if (index < 0) {
                                newActivities = newActivities.concat(activityId);
                            } else {
                                //newActivities = newActivities.splice(index, 1);  //this does not work!!!
                                newActivities.splice(index, 1);
                            }
                            return tasksModel.requestActivities(menu.modelIndex, newActivities);
                        };
                    })(activityId));
                }

                menu.newSeparator(activitiesDesktopsMenu);
            }

            Component.onCompleted: refresh()
        }
    }

    PlasmaExtras.MenuItem {
        id: moreActionsMenuItem

        visible: (visualParent
                  && isWindowItem()
                  && !isStartupItem()
                  && root.showWindowActions
                  && !root.disableAllWindowsFunctionality)

        enabled: visible

        text: i18n("More Actions")
        icon: "view-more-symbolic"

        readonly property var moreMenu: PlasmaExtras.Menu {
            visualParent: moreActionsMenuItem.action

            PlasmaExtras.MenuItem {
                enabled: menu.visualParent && menu.visualParent.m.IsMovable === true

                text: i18n("&Move")
                icon: "transform-move"

                onClicked: tasksModel.requestMove(menu.modelIndex)
            }

            PlasmaExtras.MenuItem {
                enabled: menu.visualParent && menu.visualParent.m.IsResizable === true

                text: i18n("Re&size")
                icon: "transform-scale"

                onClicked: tasksModel.requestResize(menu.modelIndex)
            }

            PlasmaExtras.MenuItem {
                visible: (visualParent
                          && isWindowItem()
                          && !isStartupItem()
                          && root.showWindowActions
                          && !root.disableAllWindowsFunctionality)

                enabled: visualParent && visualParent.m.IsMaximizable === true

                checkable: true
                checked: visualParent && visualParent.m.IsMaximized === true

                text: i18n("Ma&ximize")
                icon: "window-maximize"

                onClicked: tasksModel.requestToggleMaximized(menu.modelIndex)
            }

            PlasmaExtras.MenuItem {
                visible: (visualParent
                          && isWindowItem()
                          && !isStartupItem()
                          && root.showWindowActions
                          && !root.disableAllWindowsFunctionality)

                enabled: visualParent && visualParent.m.IsMinimizable === true

                checkable: true
                checked: visualParent && visualParent.m.IsMinimized === true

                text: i18n("Mi&nimize")
                icon: "window-minimize"

                onClicked: tasksModel.requestToggleMinimized(menu.modelIndex)
            }

            PlasmaExtras.MenuItem {
                checkable: true
                checked: menu.visualParent && menu.visualParent.m.IsKeepAbove === true

                text: i18n("Keep &Above Others")
                icon: "window-keep-above"

                onClicked: tasksModel.requestToggleKeepAbove(menu.modelIndex)
            }

            PlasmaExtras.MenuItem {
                checkable: true
                checked: menu.visualParent && menu.visualParent.m.IsKeepBelow === true

                text: i18n("Keep &Below Others")
                icon: "window-keep-below"

                onClicked: tasksModel.requestToggleKeepBelow(menu.modelIndex)
            }

            PlasmaExtras.MenuItem {
                enabled: menu.visualParent && menu.visualParent.m.IsFullScreenable === true

                checkable: true
                checked: menu.visualParent && menu.visualParent.m.IsFullScreen === true

                text: i18n("&Fullscreen")
                icon: "view-fullscreen"

                onClicked: tasksModel.requestToggleFullScreen(menu.modelIndex)
            }

            PlasmaExtras.MenuItem {
                enabled: menu.visualParent && menu.visualParent.m.IsShadeable === true

                checkable: true
                checked: menu.visualParent && menu.visualParent.m.IsShaded === true

                text: i18n("&Shade")
                icon: "window-shade"

                onClicked: tasksModel.requestToggleShaded(menu.modelIndex)
            }

            PlasmaExtras.MenuItem {
                enabled: menu.visualParent
                         && typeof atm.CanSetNoBoder !== "undefined"
                         && get(atm.CanSetNoBoder) === true

                checkable: true
                checked: menu.visualParent
                         && typeof atm.HasNoBorder !== "undefined"
                         && get(atm.HasNoBorder) === true

                text: i18n("&No Titlebar and Frame")
                icon: "edit-none-border"

                onClicked: {
                    if (typeof tasksModel.requestToggleNoBorder === "function") {
                        tasksModel.requestToggleNoBorder(menu.modelIndex);
                    }
                }
            }

            PlasmaExtras.MenuItem {
                enabled: menu.visualParent

                checkable: true
                checked: menu.visualParent
                         && typeof atm.IsExcludedFromCapture !== "undefined"
                         && get(atm.IsExcludedFromCapture) === true
                visible: Qt.platform.pluginName === "wayland"

                text: i18n("&Hide from Screencast")
                icon: "view-private"

                onClicked: {
                    if (typeof tasksModel.requestToggleExcludeFromCapture === "function") {
                        tasksModel.requestToggleExcludeFromCapture(menu.modelIndex);
                    }
                }
            }

            PlasmaExtras.MenuItem {
                separator: true
            }

            PlasmaExtras.MenuItem {
                visible: (plasmoid.configuration.groupingStrategy !== 0) && menu.visualParent.m.IsWindow === true

                checkable: true
                checked: menu.visualParent && menu.visualParent.m.IsGroupable === true

                text: i18n("Allow this program to be grouped")

                onClicked: tasksModel.requestToggleGrouping(menu.modelIndex)
            }
        }
    }

    /*    PlasmaExtras.MenuItem {
        separator: true

        visible: (visualParent
                  && visualParent.m.IsLauncher !== true
                  && visualParent.m.IsStartup !== true
                  && root.showWindowActions)
    }*/

    //// NEW Launchers Mechanism
    PlasmaExtras.MenuItem {
        id: launcherToggleAction

        visible: visualParent
                 && get(atm.IsStartup) !== true
                 && (activityInfo.numberOfRunningActivities < 2)
        //&& plasmoid.immutability !== PlasmaCore.Types.SystemImmutable

        enabled: visualParent && launcherUrlForPin() !== ""

        checkable: true

        text: i18n("&Pin Launcher")
        icon: "window-pin"

        onClicked: {
            var pinUrl = launcherUrlForPin();
            var iconUrl = launcherUrlWithIcon();

            if (launcherIsPinned()) {
                if (pinUrl !== "" && tasksModel.launcherPosition(pinUrl) !== -1) {
                    appletAbilities.launchers.removeLauncher(pinUrl);
                } else if (iconUrl !== "" && tasksModel.launcherPosition(iconUrl) !== -1) {
                    appletAbilities.launchers.removeLauncher(iconUrl);
                }
            } else {
                appletAbilities.launchers.addLauncher(pinUrl);
            }
        }
    }

    PlasmaExtras.MenuItem {
        id: showLauncherInActivitiesItem

        text: i18n("&Pin Launcher")
        icon: "window-pin"

        visible: visualParent && (!visualParent.isSeparator || (visualParent.isSeparator && root.inEditMode))
        // && get(atm.IsLauncher) !== true
                 && get(atm.IsStartup) !== true
                 && plasmoid.immutability !== PlasmaCore.Types.SystemImmutable
                 && (activityInfo.numberOfRunningActivities >= 2)

        readonly property var activitiesLaunchersMenuConnections: Connections {
            target: activityInfo
            function onNumberOfRunningActivitiesChanged() { activitiesDesktopsMenu.refresh() }
        }

        readonly property var _activitiesLaunchersMenu: PlasmaExtras.Menu {
            id: activitiesLaunchersMenu
            visualParent: showLauncherInActivitiesItem.action

            function refresh() {
                clearMenuItems();

                if (menu.visualParent === null) return;

                var createNewItem = function(id, title, url, activities) {
                    var result = menu.newMenuItem(activitiesLaunchersMenu);
                    result.text = title;

                    result.visible = true;
                    result.checkable = true;

                    result.checked = activities.some(function(activity) { return activity === id });

                    result.clicked.connect(
                                function() {
                                    if (result.checked) {
                                        appletAbilities.launchers.addLauncherToActivity(url,id);
                                    } else {
                                        appletAbilities.launchers.removeLauncherFromActivity(url, id);
                                    }
                                }
                                );

                    return result;
                }

                if (menu.visualParent === null) return;

                var url = menu.launcherUrlForPin();

                var activities = tasksModel.launcherActivities(url);

                var NULL_UUID = "00000000-0000-0000-0000-000000000000";

                createNewItem(NULL_UUID, i18n("On All Activities"), url, activities);

                if (activityInfo.numberOfRunningActivities <= 1) {
                    return;
                }

                createNewItem(activityInfo.currentActivity, i18n("On The Current Activity"), url, activities);

                menu.newSeparator(activitiesLaunchersMenu);

                var runningActivities = activityInfo.runningActivities();

                runningActivities.forEach(function(id) {
                    createNewItem(id, activityInfo.activityName(id), url, activities);
                });
            }

            Component.onCompleted: {
                menu.onVisualParentChanged.connect(refresh);
                refresh();
            }
        }
    }

    PlasmaExtras.MenuItem {
        visible: (visualParent && !visualParent.isSeparator && get(atm.IsLauncher) === true)
                 && (activityInfo.numberOfRunningActivities >= 2)
                 && plasmoid.immutability !== PlasmaCore.Types.SystemImmutable

        text: i18n("Unpin Launcher")
        icon: "window-unpin"

        onClicked: {
            appletAbilities.launchers.removeLauncher(get(atm.LauncherUrlWithoutIcon));
        }
    }

    //////END OF NEW ARCHITECTURE

    PlasmaExtras.MenuItem {
        id: addInternalSeparatorItem
        enabled: !visualParent.tailItemIsSeparator || !visualParent.headItemIsSeparator
        visible: visualParent.hasShownLauncher
        icon: "add"
        text: !visualParent.tailItemIsSeparator ? i18nc("add separator","Add %1", tailSeparatorText) : i18nc("add separator","Add %1", headSeparatorText)

        onClicked: {
            var pos=visualParent.itemIndex;

            if (!visualParent.tailItemIsSeparator) {
                appletAbilities.launchers.addInternalSeparatorAtPos(pos);
            } else {
                appletAbilities.launchers.addInternalSeparatorAtPos(pos+1);
            }
        }
    }

    PlasmaExtras.MenuItem {
        id: removeFollowingInternalSeparatorItem
        visible: visualParent && visualParent.headItemIsSeparator

        icon: "remove"
        text: i18nc("remove separator", "Remove %1", headSeparatorText)

        onClicked: {
            if (visualParent.headItemIsSeparator) {
                appletAbilities.launchers.removeInternalSeparatorAtPos(visualParent.itemIndex + 1);
            }
        }
    }

    PlasmaExtras.MenuItem {
        id: removeTailInternalSeparatorItem
        visible: visualParent && visualParent.tailItemIsSeparator

        icon: "remove"
        text: i18nc("remove separator", "Remove %1", tailSeparatorText)

        onClicked: {
            if (visualParent.tailItemIsSeparator) {
                appletAbilities.launchers.removeInternalSeparatorAtPos(visualParent.itemIndex - 1);
            }
        }
    }

    PlasmaExtras.MenuItem {
        id: alternativesMenuItem
        readonly property var alternativesAction: menu.appletAction("alternatives")
        visible: (appletAbilities.myView.isReady && appletAbilities.myView.inEditMode)
                 || (!appletAbilities.myView.isReady && plasmoid.userConfiguring /*normal plasmoid in the desktop*/)
        text: alternativesAction ? alternativesAction.text : i18n("Alternatives")
        icon: alternativesAction ? alternativesAction.icon : ""

        onClicked: {
            if (alternativesAction) {
                alternativesAction.trigger();
            }
        }
    }

    PlasmaExtras.MenuItem {
        id: myViewActions
        separator: true
        visible: false
    }

    PlasmaExtras.MenuItem {
        separator: true
        visible: removePlasmoidInMyViewEditMode.visible
    }

    PlasmaExtras.MenuItem {
        id: removePlasmoidInMyViewEditMode
        readonly property var removeAction: menu.appletAction("remove")
        //! Workaround: this is preferred compared to:
        //!   action:plasmoid.action("remove")
        //! which shows the action always and not dependent of myView.inEditMode flag
        text: removeAction ? removeAction.text : i18n("Remove")
        icon: removeAction ? removeAction.icon : ""
        visible: appletAbilities.myView.isReady && appletAbilities.myView.inEditMode && !!removeAction

        onClicked: {
            if (removeAction) {
                removeAction.trigger();
            }
        }
    }

    PlasmaExtras.MenuItem {
        section: true
        text: i18n("Window")
        visible: closeWindowItem.visible
    }

    //!move window Close button at the very bottom in order to not alter users workflow
    //!comparing with the design decisions of other taskmanagers
    PlasmaExtras.MenuItem {
        id: closeWindowItem
        visible: (visualParent && isWindowItem() && !isStartupItem()) && !root.disableAllWindowsFunctionality

        enabled: visualParent && visualParent.m.IsClosable === true

        text: get(atm.IsGroupParent) === true ? i18n("&Close All") : i18n("&Close")
        icon: "window-close"

        onClicked: {
            if (appletAbilities.parabolic.factor.zoom>1) {
                delayWindowRemovalTimer.modelIndex = menu.modelIndex;
                delayWindowRemovalTimer.start();
            } else {
                tasksModel.requestClose(menu.modelIndex);
            }
        }
    }
}
