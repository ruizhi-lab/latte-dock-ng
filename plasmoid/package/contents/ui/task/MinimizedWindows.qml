/*
    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQml.Models

Item {
    id: section

    required property var sourceModel
    required property var launcherAbility
    required property bool onlyFromLaunchers
    required property string activity
    readonly property alias orderedWindows: orderedWindows
    property var windowsById: new Map()

    function hasLauncher(url, urlWithIcon) {
        return url !== "" && (sourceModel.launcherPosition(url) >= 0
                             || sourceModel.launcherPosition(urlWithIcon) >= 0)
                && launcherAbility.inCurrentActivity(url);
    }

    function includesWindow(ids) {
        return ids && ids.length > 0 && windowsById.has(String(ids[0]));
    }

    function ordinaryActiveWindowIndex(activeIndex: int, minimized: var): int {
        if (activeIndex < 0 || activeIndex >= minimized.length || minimized[activeIndex])
            return -1;

        // The ordinary indicator drops minimized slots in source-window order.
        return activeIndex - minimized.slice(0, activeIndex).filter(hidden => hidden).length;
    }

    function windowFor(id) {
        const window = windowsById.get(id);
        return window && window.windowId === id && window.eligible ? window : null;
    }

    function activate(id) {
        // Recheck the delegate's current identity and state before using its
        // source index. A group can change before the pending refresh runs.
        const window = windowFor(id);
        if (!window)
            return;

        const index = window.sourceIndex();
        if (index.valid) {
            sourceModel.requestToggleMinimized(index);
            sourceModel.requestActivate(index);
        }
    }

    function refresh() {
        const current = new Map();
        for (let i = 0; i < rows.count; ++i) {
            const row = rows.itemAt(i) as SourceRow;
            if (!row)
                continue;

            if (row.grouped) {
                for (let j = 0; j < row.childrenModel.count; ++j) {
                    const child = row.childrenModel.itemAt(j) as WindowRow;
                    if (child && child.eligible)
                        current.set(child.windowId, child);
                }
            } else if (row.eligible) {
                current.set(row.windowId, row);
            }
        }

        windowsById = current;
        // Keep existing delegates and keyboard focus when unrelated windows
        // enter or leave the section.
        for (let i = orderedWindows.count - 1; i >= 0; --i) {
            if (!current.has(orderedWindows.get(i).windowId))
                orderedWindows.remove(i);
        }
        const known = new Set();
        for (let i = 0; i < orderedWindows.count; ++i)
            known.add(orderedWindows.get(i).windowId);

        for (const id of current.keys()) {
            if (!known.has(id)) {
                const window = current.get(id);
                orderedWindows.append({windowId: id, applicationKey: window.applicationKey,
                    applicationOrdinal: 1, applicationCount: 1, applicationBoundary: true});
            }
        }

        const applicationCounts = new Map();
        for (let i = 0; i < orderedWindows.count; ++i) {
            const key = current.get(orderedWindows.get(i).windowId).applicationKey;
            applicationCounts.set(key, (applicationCounts.get(key) || 0) + 1);
        }

        const applicationOrdinals = new Map();
        let previousApplicationKey = "";
        for (let i = 0; i < orderedWindows.count; ++i) {
            const key = current.get(orderedWindows.get(i).windowId).applicationKey;
            const ordinal = (applicationOrdinals.get(key) || 0) + 1;
            applicationOrdinals.set(key, ordinal);
            orderedWindows.set(i, {applicationKey: key, applicationOrdinal: ordinal,
                applicationCount: applicationCounts.get(key),
                applicationBoundary: i === 0 || key !== previousApplicationKey});
            previousApplicationKey = key;
        }
    }

    onOnlyFromLaunchersChanged: refreshTimer.restart()
    onActivityChanged: refreshTimer.restart()

    Component.onCompleted: refreshTimer.restart()

    Timer {
        id: refreshTimer
        interval: 0
        onTriggered: section.refresh()
    }

    ListModel { id: orderedWindows }

    component WindowRow: Item {
        required property int index
        required property var model

        property bool pinned: false
        readonly property string windowId: model.WinIdList && model.WinIdList.length > 0
                                           ? String(model.WinIdList[0]) : ""
        readonly property string title: String(model.display || model.AppName || "")
        readonly property string applicationName: String(model.AppName || "")
        readonly property string applicationKey: {
            const identity = String(model.AppId || model.LauncherUrlWithoutIcon
                                    || model.LauncherUrl || applicationName || "");
            return identity !== "" ? identity : "window:" + windowId;
        }
        readonly property var icon: model.decoration || "application-x-executable"
        readonly property bool eligible: model.IsWindow === true && model.IsMinimized === true
                                         && windowId !== "" && (!section.onlyFromLaunchers || pinned)

        onWindowIdChanged: refreshTimer.restart()
        onApplicationKeyChanged: refreshTimer.restart()
        onEligibleChanged: refreshTimer.restart()
        onIndexChanged: refreshTimer.restart()
    }

    component SourceRow: WindowRow {
        id: row
        readonly property bool grouped: model.IsGroupParent === true
        readonly property alias childrenModel: childrenRepeater

        pinned: {
            // Launcher positions are a query. Reevaluate when the list or
            // activity changes even if the source window row stays put.
            const launchers = section.sourceModel.launcherList;
            const activity = section.activity;
            return launchers.length > 0 && activity !== undefined
                    && section.hasLauncher(String(model.LauncherUrlWithoutIcon || ""),
                                           String(model.LauncherUrl || ""));
        }

        function sourceIndex() {
            return section.sourceModel.makeModelIndex(index);
        }

        onGroupedChanged: refreshTimer.restart()

        Repeater {
            id: childrenRepeater
            model: row.grouped ? groupWindows : null
            onItemAdded: refreshTimer.restart()
            onItemRemoved: refreshTimer.restart()
        }

        DelegateModel {
            id: groupWindows
            model: section.sourceModel
            rootIndex: row.sourceIndex()

            delegate: WindowRow {
                pinned: row.pinned
                function sourceIndex() {
                    return section.sourceModel.makeModelIndex(row.index, index);
                }
            }
        }
    }

    Repeater {
        id: rows
        model: section.sourceModel
        onItemAdded: refreshTimer.restart()
        onItemRemoved: refreshTimer.restart()
        delegate: SourceRow {}
    }
}
