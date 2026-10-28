#!/bin/bash
# Start the user-mode dock. Run it through `systemd-run --user` so the process gets the Plasma
# session environment; a bare SSH shell lacks it and the dock then misses the compositor
# integration (see docs/development-testing-guide.md, Remote VM GUI Testing).
#
# The Debug workflow sources ~/.config/latte-dock-ng/dev-env.sh. Release measurements do not:
# the installed Release binary finds its own QML prefix (P0.md), so the environment stays the
# production one. Set R5_SOURCE_DEV_ENV=1 for a Debug build that needs the overrides.
# R5_STDERR_FILE redirects stderr to a file; WAYLAND_DEBUG output is far too large for the journal.
if [[ "${R5_SOURCE_DEV_ENV:-0}" == 1 ]]; then
    source "$HOME/.config/latte-dock-ng/dev-env.sh"
fi
if [[ -n "${R5_STDERR_FILE:-}" ]]; then
    exec "$HOME/.local/bin/latte-dock-ng" --replace "$@" 2>"$R5_STDERR_FILE"
fi
exec "$HOME/.local/bin/latte-dock-ng" --replace "$@"
