/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
.pragma library

function canonicalId(value) {
    var id = String(value || "").toLowerCase().split("?")[0].split("#")[0];
    try { id = decodeURIComponent(id); } catch (error) { /* Keep malformed external IDs literal. */ }
    id = id.toLowerCase();
    id = id.substring(Math.max(id.lastIndexOf("/"), id.lastIndexOf(":")) + 1);
    return id.endsWith(".desktop") ? id.substring(0, id.length - 8) : id;
}

function shortId(value) {
    var id = canonicalId(value);
    return id.substring(id.lastIndexOf(".") + 1);
}

function variantId(value) {
    // Only known channel suffixes are aliases. Treating every foo-* as foo
    // merges Chromium's generated PWA desktop IDs with the browser itself.
    var id = value.replace(/-(esr|stable|beta|dev|nightly)$/, "");
    // Exact browser aliases do not include generated chrome-<app>-<profile> IDs.
    if (id === "google-chrome" || id === "google chrome") { return "chrome"; }
    if (id === "chromium-browser") { return "chromium"; }
    if (id === "microsoft-edge" || id === "microsoft edge" || id === "edge") { return "msedge"; }
    if (id === "brave-browser" || id === "brave browser") { return "brave"; }
    return id;
}

function sameId(left, right) {
    var a = canonicalId(left);
    var b = canonicalId(right);
    if (!a || !b) { return false; }
    if (a === b || variantId(a) === variantId(b)) { return true; }
    // Keep distinct reverse-DNS identities distinct (org.a.Player != org.b.Player).
    if (a.indexOf(".") !== -1 && b.indexOf(".") !== -1) { return false; }
    return variantId(shortId(a)) === variantId(shortId(b));
}

function identity(task) {
    return canonicalId(task.launcherName) || canonicalId(task.appId)
            || ("pid:" + Number(task.pid || 0));
}

function token(task) {
    return JSON.stringify([identity(task), Math.max(0, Number(task.pid || 0))]);
}

function matchesIdentity(task, value) {
    if (sameId(identity(task), value)) { return true; }
    // A generated PWA launcher is authoritative when its window reports the
    // generic browser appId. That shared appId must not override the launcher.
    return sameId(identity(task), task.appId) && sameId(task.appId, value);
}

function isBrowserId(value) {
    return ["chrome", "chromium", "firefox", "msedge", "brave", "vivaldi", "opera", "epiphany", "falkon"].indexOf(variantId(shortId(value))) !== -1;
}

function isHostedApp(task) {
    var id = identity(task);
    return /^(chrome|chromium|msedge|microsoft-edge|brave)-.+-(default|profile[_ -]?[0-9]+)$/.test(id);
}

function ancestry(pid, parentPid, cache) {
    var key = String(pid);
    if (cache[key]) { return cache[key]; }
    var result = [];
    var current = Number(pid || 0);
    for (var depth = 0; current > 1 && depth < 16; ++depth) {
        if (result.indexOf(current) !== -1) { break; }
        result.push(current);
        current = Number(parentPid(current) || 0);
    }
    cache[key] = result;
    return result;
}

function score(task, stream, parents) {
    // Per-stream desktop identity beats client-wide metadata. Portal IDs are
    // authoritative too; a conflicting explicit ID must never fall back to PID.
    var explicitId = stream.desktopId || stream.portalAppId;
    if (explicitId) { return matchesIdentity(task, explicitId) ? 40 : 0; }
    var processMatch = Number(task.pid) > 0 && parents.indexOf(Number(task.pid)) !== -1;
    if (matchesIdentity(task, stream.appId)) {
        // Electron can inherit a generic Chromium application.id. A matching
        // independent process outranks that browser-wide label; explicit portal
        // or desktop IDs above remain authoritative for sandboxed applications.
        return isBrowserId(stream.appId) && !processMatch ? 15 : 30;
    }
    var candidates = [stream.iconName, stream.binaryName, stream.appName];
    for (var i = 0; i < candidates.length; ++i) {
        if (sameId(identity(task), candidates[i])) { return processMatch ? 22 : 12; }
    }
    // Screen/activity filters can hide the browser task from this Dock. Even a
    // lone PWA must not inherit the browser's generic PID/name in that case.
    if (isHostedApp(task)) { return 0; }
    if (processMatch) { return 20; }
    // Electron clients may expose a Chromium window appId but have their own
    // launcher and process. Keep their independent PID match above, while never
    // assigning generic browser audio through that misleading shared window name.
    if (isBrowserId(task.appId) && !sameId(identity(task), task.appId)) { return 0; }
    if (task.appName && String(task.appName).toLowerCase() === String(stream.appName || "").toLowerCase()) {
        return 10;
    }
    return 0;
}

function assign(tasks, streams, parentPid) {
    var result = Object.create(null);
    var parentCache = Object.create(null);
    for (var s = 0; s < streams.length; ++s) {
        var stream = streams[s];
        var parents = ancestry(stream.pid, parentPid, parentCache);
        var best = 0;
        var owners = [];
        for (var t = 0; t < tasks.length; ++t) {
            var strength = score(tasks[t], stream, parents);
            if (strength > best) { best = strength; owners = []; }
            if (strength > 0 && strength === best) { owners.push(tasks[t]); }
        }
        var ids = [];
        for (var o = 0; o < owners.length; ++o) {
            var id = identity(owners[o]);
            if (ids.indexOf(id) === -1) { ids.push(id); }
        }
        // Shared browser PIDs and names cannot identify a particular PWA. Do
        // not expose a badge or mute/volume controls on every possible owner.
        if (ids.length !== 1) { continue; }
        var processOwners = owners.filter(function(task) { return Number(task.pid) > 0 && parents.indexOf(Number(task.pid)) !== -1; });
        if (processOwners.length) { owners = processOwners; }
        for (var i = 0; i < owners.length; ++i) {
            var key = token(owners[i]);
            if (!result[key]) { result[key] = []; }
            if (result[key].indexOf(stream) === -1) { result[key].push(stream); }
        }
    }
    return result;
}
