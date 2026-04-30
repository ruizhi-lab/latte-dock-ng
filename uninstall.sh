#!/usr/bin/env bash
# Author: Michail Vourlakos
# Summary: Uninstallation script for Latte Dock Panel

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
manifest_path="${script_dir}/build/install_manifest.txt"
dry_run="false"
manifest_provided="false"

declare -a user_homes=()

add_user_home() {
    local candidate="${1:-}"
    [[ -n "$candidate" ]] || return
    [[ -d "$candidate" ]] || return

    local existing
    for existing in "${user_homes[@]:-}"; do
        if [[ "$existing" == "$candidate" ]]; then
            return
        fi
    done

    user_homes+=("$candidate")
}

detect_user_homes() {
    add_user_home "${HOME:-}"

    if [[ -n "${SUDO_USER:-}" && "${SUDO_USER}" != "root" ]]; then
        local sudo_home=""
        if command -v getent >/dev/null 2>&1; then
            sudo_home="$(getent passwd "${SUDO_USER}" | cut -d: -f6 2>/dev/null || true)"
        fi
        if [[ -z "$sudo_home" ]]; then
            sudo_home="/home/${SUDO_USER}"
        fi
        add_user_home "$sudo_home"
    fi
}

usage() {
    cat <<'EOF'
Usage:
  bash uninstall.sh [--manifest <path>] [--dry-run]

Defaults:
  --manifest build/install_manifest.txt
EOF
}

while (($# > 0)); do
    case "$1" in
        --manifest)
            shift
            if (($# == 0)); then
                echo "Error: --manifest requires a path." >&2
                usage
                exit 2
            fi
            manifest_path="$1"
            manifest_provided="true"
            ;;
        --dry-run)
            dry_run="true"
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            echo "Error: unknown option '$1'." >&2
            usage
            exit 2
            ;;
    esac
    shift
done

if [[ "$manifest_provided" != "true" ]]; then
    # Pick the most recently generated manifest across build/ and build-* dirs.
    # Use portable stat probes because GNU find -printf is unavailable on BSD/macOS.
    detected_manifest=""
    latest_mtime=0
    shopt -s nullglob

    for candidate in "$script_dir"/build/install_manifest.txt "$script_dir"/build-*/install_manifest.txt; do
        [[ -f "$candidate" ]] || continue

        mtime=0
        if mtime="$(stat -c %Y "$candidate" 2>/dev/null)"; then
            :
        elif mtime="$(stat -f %m "$candidate" 2>/dev/null)"; then
            :
        fi

        if (( mtime >= latest_mtime )); then
            latest_mtime="$mtime"
            detected_manifest="$candidate"
        fi
    done

    shopt -u nullglob

    if [[ -n "$detected_manifest" ]]; then
        manifest_path="$detected_manifest"
    fi
fi

detect_user_homes

if [[ ! -f "$manifest_path" ]]; then
    echo "Warning: uninstall manifest does not exist: $manifest_path"
    echo "Proceeding with package directory cleanup only."
fi

echo "Using uninstall manifest: $manifest_path"

if [[ "$dry_run" == "true" ]]; then
    echo "Dry run mode. Files that would be removed:"
    if [[ -f "$manifest_path" ]]; then
        cat "$manifest_path"
    fi
fi

if [[ "${EUID}" -eq 0 ]]; then
    sudo_cmd=()
else
    sudo_cmd=(sudo)
fi

run_as_root() {
    "${sudo_cmd[@]}" "$@"
}

remove_file() {
    local path="$1"

    if [[ "$dry_run" == "true" ]]; then
        echo "rm -f -- $path"
        return
    fi

    run_as_root rm -f -- "$path"
}

remove_tree() {
    local path="$1"

    if [[ "$dry_run" == "true" ]]; then
        echo "rm -rf -- $path"
        return
    fi

    run_as_root rm -rf -- "$path"
}

remove_dir_if_empty() {
    local path="$1"

    if [[ "$dry_run" == "true" ]]; then
        echo "rmdir --ignore-fail-on-non-empty -- $path"
        return
    fi

    run_as_root rmdir --ignore-fail-on-non-empty -- "$path" 2>/dev/null || true
}

remove_tree_if_marked() {
    local path="$1"
    local marker="$2"

    if [[ ! -f "$marker" ]]; then
        return
    fi

    if [[ "$dry_run" == "true" ]]; then
        echo "rm -rf -- $path"
        return
    fi

    run_as_root rm -rf -- "$path"
}

if [[ -f "$manifest_path" ]]; then
    while IFS= read -r file; do
        [[ -z "$file" ]] && continue
        remove_file "$file"
    done < "$manifest_path"
fi

# install.sh explicitly syncs these directories with rsync --delete.
# Remove them during uninstall to guarantee no stale files remain.
managed_dirs=(
    "/usr/share/plasma/plasmoids/org.kde.latte.containment"
    "/usr/share/plasma/plasmoids/org.kde.latte.plasmoid"
    "/usr/share/plasma/shells/org.kde.latte.shell"
    "/usr/share/latte/indicators"
)

for dir_path in "${managed_dirs[@]}"; do
    remove_tree "$dir_path"
done

# Also remove user-level overrides that can shadow /usr/share content.
for user_home in "${user_homes[@]:-}"; do
    user_managed_dirs=(
        "${user_home}/.local/share/plasma/plasmoids/org.kde.latte.containment"
        "${user_home}/.local/share/plasma/plasmoids/org.kde.latte.plasmoid"
        "${user_home}/.local/share/plasma/shells/org.kde.latte.shell"
        "${user_home}/.local/share/latte/indicators"
    )

    for dir_path in "${user_managed_dirs[@]}"; do
        if [[ "$dry_run" == "true" ]]; then
            echo "rm -rf -- $dir_path"
        else
            rm -rf -- "$dir_path"
        fi
    done
done

qt_qml_dirs=("/usr/lib64/qt6/qml")
if command -v qtpaths6 >/dev/null 2>&1; then
    qt_qml_dirs+=("$(qtpaths6 --query QT_INSTALL_QML 2>/dev/null || true)")
elif command -v qtpaths >/dev/null 2>&1; then
    qt_qml_dirs+=("$(qtpaths --query QT_INSTALL_QML 2>/dev/null || true)")
fi

for qml_dir in "${qt_qml_dirs[@]}"; do
    [[ -z "$qml_dir" ]] && continue
    fallback_dir="${qml_dir}/org/kde/plasma/private/taskmanager"
    marker_file="${fallback_dir}/.latte-fallback-module"
    remove_tree_if_marked "$fallback_dir" "$marker_file"
done

# Best-effort cleanup for now-empty parent dirs created by install.sh sync.
remove_dir_if_empty "/usr/share/latte/indicators"
remove_dir_if_empty "/usr/share/latte"
for user_home in "${user_homes[@]:-}"; do
    if [[ "$dry_run" == "true" ]]; then
        echo "rmdir --ignore-fail-on-non-empty -- ${user_home}/.local/share/latte/indicators"
        echo "rmdir --ignore-fail-on-non-empty -- ${user_home}/.local/share/latte"
        echo "rmdir --ignore-fail-on-non-empty -- ${user_home}/.local/share/plasma/plasmoids"
        echo "rmdir --ignore-fail-on-non-empty -- ${user_home}/.local/share/plasma/shells"
    else
        rmdir --ignore-fail-on-non-empty -- "${user_home}/.local/share/latte/indicators" 2>/dev/null || true
        rmdir --ignore-fail-on-non-empty -- "${user_home}/.local/share/latte" 2>/dev/null || true
        rmdir --ignore-fail-on-non-empty -- "${user_home}/.local/share/plasma/plasmoids" 2>/dev/null || true
        rmdir --ignore-fail-on-non-empty -- "${user_home}/.local/share/plasma/shells" 2>/dev/null || true
    fi
done

if [[ "$dry_run" == "true" ]]; then
    echo "Dry run complete."
else
    echo "Uninstall complete."
fi
