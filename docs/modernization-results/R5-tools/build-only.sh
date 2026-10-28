#!/bin/bash
# Configure and build one archived revision exactly as `install.sh --user ... Release` does, but
# stop before the install step, so several revisions can be built back to back and installed
# later with swap-install.sh. The configure arguments come from the revision's own install.sh.
# usage: build-only.sh <label>   (source in ~/latte-r5/src-<label>, build in ~/latte-r5/build-<label>)
set -u
label="$1"
src="$HOME/latte-r5/src-$label"
build="$HOME/latte-r5/build-$label"
log="$HOME/latte-r5/logs/build-$label.log"
{
  echo "start $(date -Is) revision $(cat "$src/SOURCE_REVISION")"
  cd "$src" || exit 2
  # Insert `exit 0` in front of the install section (the "Helper functions" banner).
  sed '/^# .* Helper functions/i exit 0' install.sh > install-buildonly.sh
  grep -q '^exit 0$' install-buildonly.sh || { echo "install.sh marker missing"; exit 3; }
  # R5_CXXFLAGS (optional) seeds CMAKE_CXX_FLAGS in a fresh build tree, for example -DQT_QML_DEBUG
  # to let qmlprofiler attach to a Release build.
  CXXFLAGS="${R5_CXXFLAGS:-}" bash install-buildonly.sh --user --jobs 4 --build-dir "$build" Release
  rc=$?
  echo "build-only exit=$rc at $(date -Is)"
} > "$log" 2>&1
