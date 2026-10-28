#!/bin/bash
# Stop any dock the R5 tools or the session's own dev unit started: by unit first, then by exact process name.
systemctl --user stop latte-r5-dock.service 2>/dev/null
for unit in $(systemctl --user list-units --no-legend --plain 'latte-dock-ng-dev-*.service' | awk '{print $1}'); do systemctl --user stop "$unit"; done
for _ in $(seq 1 25); do pgrep -x latte-dock-ng >/dev/null || exit 0; sleep 1; done
pkill -x latte-dock-ng
sleep 2
