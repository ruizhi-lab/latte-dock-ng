// SPDX-License-Identifier: GPL-2.0-or-later
// External, single-window TaskManager observer for the isolated P4 fixture.
import QtQuick
import QtQuick.Window
import org.kde.taskmanager as TaskManager

Window {
    id: root

    visible: false
    width: 1
    height: 1
    title: "P4 TaskManager observer"

    property string targetAppId: ""
    property string targetTitle: ""
    property int timeoutMs: 20000
    property int stageTimeoutMs: 5000
    property int timeoutTicks: 200
    property int stageTimeoutTicks: 50
    property int elapsedTicks: 0
    property int phaseTicks: 0
    property int phase: 0
    property bool completed: false
    property bool reactivateRequested: false
    property bool baselineUnminimizeRequested: false
    property bool activateRequested: false
    property string targetWindowId: ""

    readonly property rect observedScreenGeometry: Qt.rect(Screen.virtualX,
                                                           Screen.virtualY,
                                                           Screen.width,
                                                           Screen.height)

    TaskManager.ActivityInfo {
        id: activityInfo
    }

    TaskManager.VirtualDesktopInfo {
        id: desktopInfo
    }

    TaskManager.TasksModel {
        id: tasksModel

        groupMode: TaskManager.TasksModel.GroupDisabled
        sortMode: TaskManager.TasksModel.SortManual
        filterByVirtualDesktop: true
        virtualDesktop: desktopInfo.currentDesktop
        filterByScreen: true
        screenGeometry: root.observedScreenGeometry
        filterByActivity: true
        activity: activityInfo.currentActivity
    }

    function event(name, values) {
        var record = { event: name, phase: phaseName(phase) };
        if (values) {
            for (var key in values) {
                record[key] = values[key];
            }
        }
        // The controller parses this stream; keep verdict events visible when Qt debug categories are filtered.
        console.warn("P4_EVENT " + JSON.stringify(record));
    }

    function phaseName(value) {
        switch (value) {
        case 0: return "waiting-readiness";
        case 1: return "waiting-window";
        case 2: return "waiting-active";
        case 3: return "waiting-minimized";
        case 4: return "waiting-restored";
        case 5: return "complete";
        default: return "unknown";
        }
    }

    function zeroUuid(value) {
        return String(value).toLowerCase() === "00000000-0000-0000-0000-000000000000";
    }

    function desktopIdentityReady(value) {
        if (value === undefined || value === null) {
            return false;
        }
        if (typeof value === "string") {
            var text = value.trim();
            if (text.length === 0 || zeroUuid(text)) {
                return false;
            }
        }
        var ids = desktopInfo.desktopIds || [];
        for (var index = 0; index < ids.length; ++index) {
            if (ids[index] === value || String(ids[index]) === String(value)) {
                return true;
            }
        }
        return false;
    }

    function contextStatus() {
        var activityId = String(activityInfo.currentActivity || "");
        var activityReady = activityInfo.numberOfRunningActivities > 0
            && activityId.length > 0 && !zeroUuid(activityId);
        var desktopId = desktopInfo.currentDesktop;
        var desktopReady = desktopInfo.numberOfDesktops > 0 && desktopIdentityReady(desktopId);
        var screen = observedScreenGeometry;
        var screenReady = screen.width > 0 && screen.height > 0;
        return {
            ready: activityReady && desktopReady && screenReady,
            activityReady: activityReady,
            activityId: activityId,
            runningActivities: activityInfo.numberOfRunningActivities,
            desktopReady: desktopReady,
            desktopId: desktopId === undefined ? "undefined" : desktopId,
            desktopCount: desktopInfo.numberOfDesktops,
            desktopIds: desktopInfo.desktopIds,
            screenReady: screenReady,
            screen: [screen.x, screen.y, screen.width, screen.height]
        };
    }

    function matchingRows() {
        var matches = [];
        var roles = TaskManager.AbstractTasksModel;
        for (var row = 0; row < tasksModel.count; ++row) {
            var modelIndex = tasksModel.makeModelIndex(row);
            if (!modelIndex) {
                continue;
            }
            var appId = String(tasksModel.data(modelIndex, roles.AppId) || "");
            var title = String(tasksModel.data(modelIndex, Qt.DisplayRole) || "");
            if (appId !== targetAppId || title !== targetTitle) {
                continue;
            }
            var ids = tasksModel.data(modelIndex, roles.WinIdList);
            matches.push({
                index: modelIndex,
                appId: appId,
                title: title,
                windowId: ids && ids.length > 0 ? String(ids[0]) : "",
                isWindow: tasksModel.data(modelIndex, roles.IsWindow) === true,
                isStartup: tasksModel.data(modelIndex, roles.IsStartup) === true,
                isActive: tasksModel.data(modelIndex, roles.IsActive) === true,
                isMinimized: tasksModel.data(modelIndex, roles.IsMinimized) === true
            });
        }
        return matches;
    }

    function finish(verdict, reason) {
        if (completed) {
            return;
        }
        completed = true;
        event("result", { verdict: verdict, reason: reason, windowId: targetWindowId });
        Qt.quit();
    }

    function rowFailure(rows, reason) {
        finish("model-or-driver-failure", reason + "; matchingRows=" + rows.length);
    }

    function check() {
        if (completed) {
            return;
        }

        var context = contextStatus();
        if (!context.ready) {
            if (phase !== 0) {
                finish("invalid-readiness", "context-became-invalid");
            }
            return;
        }

        if (phase === 0) {
            phase = 1;
            phaseTicks = 0;
            event("readiness", context);
        }

        var rows = matchingRows();
        if (rows.length > 1) {
            rowFailure(rows, "target-title-not-unique");
            return;
        }

        if (phase === 1) {
            if (rows.length === 0) {
                return;
            }
            var baseline = rows[0];
            if (!baseline.isWindow || baseline.isStartup || baseline.windowId.length === 0) {
                rowFailure(rows, "target-row-not-a-stable-window");
                return;
            }
            targetWindowId = baseline.windowId;
            event("baseline-row", baselineSummary(baseline));
            phase = 2;
            phaseTicks = 0;
            if (baseline.isMinimized) {
                baselineUnminimizeRequested = true;
                event("controller-unminimize-baseline", { windowId: targetWindowId });
                tasksModel.requestToggleMinimized(baseline.index);
            } else {
                activateRequested = true;
                event("controller-activate", { windowId: targetWindowId });
                tasksModel.requestActivate(baseline.index);
            }
            return;
        }

        if (rows.length !== 1 || rows[0].windowId !== targetWindowId
                || !rows[0].isWindow || rows[0].isStartup) {
            rowFailure(rows, "target-window-identity-changed");
            return;
        }

        var current = rows[0];
        if (phase === 2) {
            if (current.isMinimized && baselineUnminimizeRequested) {
                return;
            }
            if (!current.isMinimized && !activateRequested) {
                activateRequested = true;
                event("controller-activate", { windowId: targetWindowId });
                tasksModel.requestActivate(current.index);
                return;
            }
            if (current.isActive && !current.isMinimized) {
                phase = 3;
                phaseTicks = 0;
                event("observed-active", baselineSummary(current));
                event("controller-minimize", { windowId: targetWindowId });
                tasksModel.requestToggleMinimized(current.index);
                return;
            }
        } else if (phase === 3) {
            if (current.isMinimized) {
                phase = 4;
                phaseTicks = 0;
                event("observed-minimized", baselineSummary(current));
                // Keep restoration separate from activation and verify both reported roles.
                event("controller-restore", { windowId: targetWindowId });
                tasksModel.requestToggleMinimized(current.index);
                return;
            }
        } else if (phase === 4) {
            if (!current.isMinimized && !reactivateRequested) {
                reactivateRequested = true;
                event("controller-reactivate", { windowId: targetWindowId });
                tasksModel.requestActivate(current.index);
                return;
            }
            if (!current.isMinimized && current.isActive) {
                phase = 5;
                event("observed-restored-active", baselineSummary(current));
                finish("valid-sequence", "activation-minimize-restore-observed");
                return;
            }
        }

    }

    function onTimerTick() {
        // Local watchdog only; the Python runner's monotonic deadline is authoritative.
        if (completed) {
            return;
        }
        elapsedTicks++;
        if (phase !== 0) {
            phaseTicks++;
        }
        if (elapsedTicks >= timeoutTicks) {
            if (phase === 0) {
                finish("invalid-readiness", "context-timeout");
            } else {
                rowFailure([], "overall-timeout-" + phaseName(phase));
            }
            return;
        }
        if (phase !== 0 && phaseTicks >= stageTimeoutTicks) {
            rowFailure([], "stage-timeout-" + phaseName(phase));
            return;
        }
        check();
    }

    function baselineSummary(row) {
        return {
            appId: row.appId,
            title: row.title,
            windowId: row.windowId,
            isWindow: row.isWindow,
            isStartup: row.isStartup,
            isActive: row.isActive,
            isMinimized: row.isMinimized
        };
    }

    Component.onCompleted: {
        var args = Qt.application.arguments;
        for (var i = 0; i < args.length; ++i) {
            if (args[i] === "--app-id" && i + 1 < args.length) {
                targetAppId = args[++i];
            } else if (args[i] === "--target-title" && i + 1 < args.length) {
                targetTitle = args[++i];
            } else if (args[i] === "--timeout-ms" && i + 1 < args.length) {
                timeoutMs = Number(args[++i]);
            } else if (args[i] === "--stage-timeout-ms" && i + 1 < args.length) {
                stageTimeoutMs = Number(args[++i]);
            }
        }

        if (targetAppId.length === 0 || targetTitle.length === 0
                || !Number.isFinite(timeoutMs) || timeoutMs < 1000
                || !Number.isFinite(stageTimeoutMs) || stageTimeoutMs < 500) {
            finish("invalid-readiness", "invalid-or-missing-arguments");
            return;
        }
        timeoutTicks = Math.ceil(timeoutMs / poll.interval);
        stageTimeoutTicks = Math.ceil(stageTimeoutMs / poll.interval);
        event("started", { appId: targetAppId, targetTitle: targetTitle,
                            timeoutMs: timeoutMs, stageTimeoutMs: stageTimeoutMs });
        poll.start();
        check();
    }

    Timer {
        id: poll
        interval: 100
        repeat: true
        onTriggered: root.onTimerTick()
    }
}
