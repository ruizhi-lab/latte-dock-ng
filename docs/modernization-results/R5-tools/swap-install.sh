#!/bin/bash
# Replace the user-mode install with an already built revision (see build-only.sh).
# 1. uninstall the currently installed revision with its own uninstall.sh and the manifest under
#    ~/.local/share/latte-dock-ng, so no file of another revision stays behind;
# 2. run the new revision's own install.sh with the compile step removed and without its pre-clean.
# The dock must be stopped first. User configuration is kept (--no-purge-user-data).
# usage: swap-install.sh <label>
set -u
label="$1"
src="$HOME/latte-r5/src-$label"
build="$HOME/latte-r5/build-$label"
log="$HOME/latte-r5/logs/swap-$label.log"
current="$(cat "$HOME/latte-r5/INSTALLED_LABEL" 2>/dev/null || true)"
{
  echo "start $(date -Is) swap ${current:-<unknown>} -> $label"
  if pgrep -x latte-dock-ng >/dev/null; then echo "dock still running"; exit 4; fi
  if [[ -n "$current" && -f "$HOME/latte-r5/src-$current/uninstall.sh" ]]; then
    bash "$HOME/latte-r5/src-$current/uninstall.sh" --user --manifest "$HOME/.local/share/latte-dock-ng/install-manifest.txt" \
      --no-purge-user-data --preserve-autostart || echo "uninstall returned $?"
  fi
  cd "$src" || exit 2
  sed -e 's|cmake --build \. --parallel "${build_jobs}" --clean-first|true|' \
      -e 's|cmake --build \. --parallel --clean-first|true|' install.sh > install-nobuild.sh
  bash install-nobuild.sh --user --no-clean --jobs 4 --build-dir "$build" Release
  rc=$?
  [[ $rc -eq 0 ]] && echo "$label" > "$HOME/latte-r5/INSTALLED_LABEL"
  echo "swap exit=$rc at $(date -Is)"
  exit $rc
} > "$log" 2>&1
