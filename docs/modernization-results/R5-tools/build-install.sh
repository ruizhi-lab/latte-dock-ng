#!/bin/bash
# Build and user-install one archived revision as Release, logging everything.
# usage: build-install.sh <label>   (source in ~/latte-r5/src-<label>)
set -u
label="$1"
src="$HOME/latte-r5/src-$label"
build="$HOME/latte-r5/build-$label"
log="$HOME/latte-r5/logs/build-$label.log"
{
  echo "start $(date -Is) revision $(cat "$src/SOURCE_REVISION")"
  cd "$src" || exit 2
  bash install.sh --user --jobs 4 --build-dir "$build" Release
  rc=$?
  echo "install.sh exit=$rc at $(date -Is)"
} > "$log" 2>&1
