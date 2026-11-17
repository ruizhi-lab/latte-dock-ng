#!/usr/bin/env bash
# Shared tracked-QML inventory for syntax and deep lint.
#
# Archived diagnostics and run evidence are not production QML. The canonical
# P4 observer remains covered by its independent offscreen readiness check;
# preserve all other tracked files, including docs/help and autotest QML.
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "usage: qmllint-files.sh GIT_ROOT" >&2
    exit 2
fi

git -C "$1" ls-files -- '*.qml' ':(exclude)docs/modernization-results/**'
