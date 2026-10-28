// R5 read-only probe: pointer position and window geometry by class, no window titles.
// KWin's print() is not routed to the journal, so the report is sent as a D-Bus call that
// kwin-probe.sh captures with busctl monitor.
var rows = [];
var wins = workspace.windowList();
for (var i = 0; i < wins.length; i++) {
    var w = wins[i];
    var g = w.frameGeometry;
    rows.push([w.resourceClass, w.dock ? "dock" : "-", w.normalWindow ? "normal" : "-", w.fullScreen ? "fs" : "-",
               w.minimized ? "min" : "-", w.active ? "active" : "-", Math.round(g.x), Math.round(g.y),
               Math.round(g.width), Math.round(g.height), w.layer, w.keepAbove ? "above" : "-"].join("|"));
}
var report = "R5PROBE cursor=" + Math.round(workspace.cursorPos.x) + "," + Math.round(workspace.cursorPos.y) +
             " screen=" + workspace.virtualScreenSize.width + "x" + workspace.virtualScreenSize.height +
             " windows=" + wins.length + " :: " + rows.join(" ;; ");
callDBus("org.r5.probe", "/r5", "org.r5.probe", "report", report);
