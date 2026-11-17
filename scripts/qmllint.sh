#!/usr/bin/env bash
# Lint every tracked source and test QML file, excluding archived result copies.
#
# Gate policy: fail on QML *errors* only (syntax errors and anything qmllint
# promotes to error level). Every analyzer warning category is disabled so the
# large legacy QML inherited from upstream latte-dock does not drown the log,
# and unresolved org.kde.* imports (Plasma modules are not installed in the
# CI container) cannot affect the result. Exit code 255 on a syntax error is
# exactly the regression class this gate exists to catch.
#
# Usage: scripts/qmllint.sh [file.qml ...]
#        With no arguments, the shared tracked-QML inventory is linted.

set -uo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

# Locate qmllint across distros (Fedora: /usr/lib64/qt6/bin,
# Debian/Ubuntu: /usr/lib/qt6/bin, or on PATH as qmllint-qt6/qmllint).
qmllint=""
for candidate in qmllint-qt6 qmllint /usr/lib/qt6/bin/qmllint /usr/lib64/qt6/bin/qmllint; do
    if command -v "$candidate" >/dev/null 2>&1; then
        qmllint="$candidate"
        break
    fi
done
if [[ -z "$qmllint" ]]; then
    echo "error: qmllint not found (install qt6-declarative-dev / qt6-declarative)" >&2
    exit 1
fi

# Deterministic run: ignore any .qmllint.ini in the tree and let warnings
# never influence the exit code (-1 = unlimited, non-failing).
args=(--ignore-settings --max-warnings -1)

# Disable every analyzer warning category, including built-in plugin
# categories such as Quick.anchor-combinations (hence the case-insensitive
# pattern). Parse the list from --help-all so the script keeps working
# across Qt minor versions. If the parse ever yields nothing, the gate still
# works (just noisier). NOTE: do NOT use "-D all" — disabling all plugins
# also disables the core syntax verifier, silently neutering the gate.
while read -r category; do
    args+=("$category" disable)
done < <("$qmllint" --help-all 2>/dev/null | awk '/^[[:space:]]+--[a-zA-Z.-]+ <level>/ {print $1}')

# Collect tracked QML sources only (build trees and untracked files excluded).
if [[ $# -gt 0 ]]; then
    files=("$@")
else
    if ! file_list="$("$root/scripts/qmllint-files.sh" "$root")"; then
        echo "error: cannot discover tracked QML files" >&2
        exit 1
    fi
    if [[ -n "$file_list" ]]; then
        mapfile -t files <<< "$file_list"
    else
        files=()
    fi
fi
if [[ ${#files[@]} -eq 0 ]]; then
    echo "qmllint: no QML files found"
    exit 0
fi

echo "qmllint: linting ${#files[@]} QML files with $qmllint (gate: errors only)"

failed=0
chunk=200
total=${#files[@]}
for ((start = 0; start < total; start += chunk)); do
    batch=("${files[@]:start:chunk}")
    if ! "$qmllint" "${args[@]}" "${batch[@]}"; then
        failed=1
    fi
done

if [[ $failed -ne 0 ]]; then
    echo "qmllint: FAILED (syntax/type errors above)" >&2
    exit 1
fi
echo "qmllint: OK"
