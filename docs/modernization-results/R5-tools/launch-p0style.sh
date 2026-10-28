#!/bin/bash
# The launcher pattern of docs/development-testing-guide.md ("Remote VM GUI Testing") that P0 used from SSH:
# import only six Plasma session variables, then source the installer's dev-env.sh, then exec the dock.
# r5-scenario.py --launcher p0-ssh starts it from an SSH-like bare environment (env -i plus the variables an
# SSH command receives on the Fedora VM) so that the dock sees what a P0 launch gave it.
# R5_STDERR_FILE redirects stderr, as in launch-dock.sh.
imported='DISPLAY|WAYLAND_DISPLAY|XDG_SESSION_TYPE|XDG_RUNTIME_DIR|DBUS_SESSION_BUS_ADDRESS|XAUTHORITY'
# R5_IMPORT_XDG=1 additionally imports the variables that select the KSycoca database (the R5 fix check).
if [[ "${R5_IMPORT_XDG:-0}" == 1 ]]; then
    imported+='|XDG_CONFIG_DIRS|XDG_DATA_DIRS|XDG_CURRENT_DESKTOP|XDG_MENU_PREFIX|KDE_FULL_SESSION|KDE_SESSION_VERSION|DESKTOP_SESSION|LANG|LANGUAGE'
fi
while IFS= read -r line; do export "$line"; done \
  < <(systemctl --user show-environment | grep -E "^(${imported})=")
# R5_SKIP_DEV_ENV=1 launches with the bare SSH values instead (LANG as forwarded by the SSH client, no XDG_CONFIG_DIRS).
if [[ "${R5_SKIP_DEV_ENV:-0}" != 1 ]]; then
    source "$HOME/.config/latte-dock-ng/dev-env.sh"
fi
# R5_EXEC (qml-profile.sh) replaces the dock by another command line, so qmlprofiler and the dock it starts get the
# P0 launch environment.
if [[ -n "${R5_EXEC:-}" ]]; then
    exec $R5_EXEC
fi
if [[ -n "${R5_STDERR_FILE:-}" ]]; then
    exec "$HOME/.local/bin/latte-dock-ng" --replace "$@" 2>"$R5_STDERR_FILE"
fi
exec "$HOME/.local/bin/latte-dock-ng" --replace "$@"
