#!/bin/bash
# Sample the dock's main-thread stack while a KSycoca storm runs (VM side, main build installed).
# usage: stack-storm.sh OUTFILE [SAMPLES=90] [applet plugin to remove from the layout, optional]
set -u
out="$1"; samples="${2:-90}"; remove="${3:-}"
layout="$HOME/.config/latte/我的布局.layout.latte"
pristine="$HOME/latte-r5/layout-pristine/我的布局.layout.latte"
cleanup() {
    "$HOME/latte-r5/tools/r5-stim.sh" stop sycoca
    "$HOME/latte-r5/tools/stop-dock.sh"
    cp -p "$pristine" "$layout"
    sha256sum "$layout" >> "${out%.txt}.layout-restore.txt"
}
trap cleanup EXIT
"$HOME/latte-r5/tools/stop-dock.sh"
python3 -I - "$pristine" "$layout" "$remove" <<'PY'
import importlib.util, sys
spec = importlib.util.spec_from_file_location("r5", f"{__import__('os').path.expanduser('~')}/latte-r5/tools/r5-scenario.py")
m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
text = open(sys.argv[1]).read()
text = "\n".join("visibility=0" if l.startswith("visibility=") else l for l in text.splitlines()) + "\n"
if sys.argv[3]:
    text = m.remove_applets(text, [sys.argv[3]])
open(sys.argv[2], "w").write(text)
PY
systemd-run --user --unit=latte-r5-dock --collect --quiet "$HOME/latte-r5/tools/launch-dock.sh"
sleep 35
"$HOME/latte-r5/tools/r5-stim.sh" start sycoca 0.5
sleep 12
pid="$(pgrep -x latte-dock-ng)"
"$HOME/latte-r5/tools/stack-sample.sh" "$pid" "$samples" 0.3 "$out"
