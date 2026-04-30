#!/usr/bin/env bash
# Author: Michail Vourlakos
# Summary: Uninstallation script for Latte Dock Panel

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
manifest_path="${script_dir}/build/install_manifest.txt"
dry_run="false"
manifest_provided="false"
purge_user_data="false"

declare -a manifest_paths=()

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
  bash uninstall.sh [--manifest <path>] [--dry-run] [--purge-user-data]

Defaults:
  --manifest build/install_manifest.txt (or auto-detected from build*/install_manifest.txt)

Options:
  --purge-user-data   Also remove user config/cache/state files under ~/.config and ~/.local.
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
        --purge-user-data)
            purge_user_data="true"
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

if [[ "$manifest_provided" == "true" ]]; then
    manifest_paths=("$manifest_path")
else
    # Include all manifests from build/ and build-* so stale files from old build dirs
    # are also removed during cleanup.
    shopt -s nullglob

    for candidate in "$script_dir"/build/install_manifest.txt "$script_dir"/build-*/install_manifest.txt; do
        if [[ -f "$candidate" ]]; then
            manifest_paths+=("$candidate")
        fi
    done

    shopt -u nullglob

    if [[ ${#manifest_paths[@]} -eq 0 ]]; then
        manifest_paths=("$manifest_path")
    fi
fi

detect_user_homes

existing_manifest_count=0
for manifest_candidate in "${manifest_paths[@]}"; do
    if [[ -f "$manifest_candidate" ]]; then
        existing_manifest_count=$((existing_manifest_count + 1))
    fi
done

if [[ "$existing_manifest_count" -eq 0 ]]; then
    echo "Warning: uninstall manifests do not exist."
    printf '  - %s\n' "${manifest_paths[@]}"
    echo "Proceeding with package directory cleanup only."
else
    if [[ ${#manifest_paths[@]} -eq 1 ]]; then
        echo "Using uninstall manifest: ${manifest_paths[0]}"
    else
        echo "Using uninstall manifests:"
        printf '  - %s\n' "${manifest_paths[@]}"
    fi
fi

if [[ "$dry_run" == "true" ]]; then
    echo "Dry run mode. Files that would be removed:"
    for manifest_candidate in "${manifest_paths[@]}"; do
        if [[ -f "$manifest_candidate" ]]; then
            echo "--- ${manifest_candidate} ---"
            cat "$manifest_candidate"
        fi
    done
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

remove_user_local_launcher_link() {
    local user_home="$1"
    local local_bin="${user_home}/.local/bin/latte-dock-ng"
    local resolved=""

    if [[ ! -L "$local_bin" ]]; then
        return
    fi

    resolved="$(readlink -f "$local_bin" 2>/dev/null || true)"
    if [[ "$resolved" != "/usr/bin/latte-dock-ng" ]]; then
        return
    fi

    if [[ "$dry_run" == "true" ]]; then
        echo "rm -f -- $local_bin"
        return
    fi

    rm -f -- "$local_bin"
}

remove_user_stale_launchers() {
    local user_home="$1"
    local stale_entry=""

    shopt -s nullglob
    for stale_entry in "${user_home}"/.local/bin/latte-dock-ng.stale.*; do
        [[ -e "$stale_entry" ]] || continue
        if [[ "$dry_run" == "true" ]]; then
            echo "rm -f -- $stale_entry"
        else
            rm -f -- "$stale_entry"
        fi
    done
    shopt -u nullglob
}

for manifest_file in "${manifest_paths[@]}"; do
    if [[ -f "$manifest_file" ]]; then
        while IFS= read -r file; do
            [[ -z "$file" ]] && continue
            remove_file "$file"
        done < "$manifest_file"
    fi
done

# install.sh explicitly syncs these directories with rsync --delete.
# Remove them during uninstall to guarantee no stale files remain.
managed_prefixes=("/usr" "/usr/local")
for prefix in "${managed_prefixes[@]}"; do
    managed_dirs=(
        "${prefix}/share/plasma/plasmoids/org.kde.latte.containment"
        "${prefix}/share/plasma/plasmoids/org.kde.latte.plasmoid"
        "${prefix}/share/plasma/shells/org.kde.latte.shell"
        "${prefix}/share/latte/indicators"
    )

    managed_files=(
        "${prefix}/bin/latte-dock-ng"
        "${prefix}/share/applications/org.kde.latte-dock.desktop"
        "${prefix}/share/metainfo/org.kde.latte-dock.appdata.xml"
        "${prefix}/share/dbus-1/interfaces/org.kde.LatteDock.xml"
        "${prefix}/share/knotifications6/lattedock.notifyrc"
        "${prefix}/share/knsrcfiles/latte-layouts.knsrc"
        "${prefix}/share/knsrcfiles/latte-indicators.knsrc"
        "${prefix}/share/kservicetypes6/latte-indicator.desktop"
        "${prefix}/share/kservices6/latte-indicator.desktop"
        "${prefix}/share/icons/breeze/applets/256/org.kde.latte.plasmoid.svg"
    )

    for dir_path in "${managed_dirs[@]}"; do
        remove_tree "$dir_path"
    done

    for file_path in "${managed_files[@]}"; do
        remove_file "$file_path"
    done
done

# Also remove user-level overrides that can shadow /usr/share content.
for user_home in "${user_homes[@]:-}"; do
    user_managed_dirs=(
        "${user_home}/.local/share/plasma/plasmoids/org.kde.latte.containment"
        "${user_home}/.local/share/plasma/plasmoids/org.kde.latte.plasmoid"
        "${user_home}/.local/share/plasma/shells/org.kde.latte.shell"
        "${user_home}/.local/share/latte/indicators"
    )
    user_managed_files=(
        "${user_home}/.local/share/applications/org.kde.latte-dock.desktop"
        "${user_home}/.local/share/applications/latte-dock.desktop"
        "${user_home}/.config/autostart/org.kde.latte-dock.desktop"
        "${user_home}/.config/autostart/latte-dock.desktop"
    )

    for dir_path in "${user_managed_dirs[@]}"; do
        if [[ "$dry_run" == "true" ]]; then
            echo "rm -rf -- $dir_path"
        else
            rm -rf -- "$dir_path"
        fi
    done

    for file_path in "${user_managed_files[@]}"; do
        if [[ "$dry_run" == "true" ]]; then
            echo "rm -f -- $file_path"
        else
            rm -f -- "$file_path"
        fi
    done

    remove_user_local_launcher_link "$user_home"
    remove_user_stale_launchers "$user_home"

    if [[ "$purge_user_data" == "true" ]]; then
        user_purge_paths=(
            "${user_home}/.config/latte"
            "${user_home}/.config/lattedockrc"
            "${user_home}/.config/autostart/org.kde.latte-dock.desktop"
            "${user_home}/.local/share/latte-layouts"
            "${user_home}/.local/share/latte"
            "${user_home}/.cache/latte-dock"
            "${user_home}/.cache/lattedock"
            "${user_home}/.local/state/latte"
        )

        for purge_path in "${user_purge_paths[@]}"; do
            if [[ -d "$purge_path" ]]; then
                if [[ "$dry_run" == "true" ]]; then
                    echo "rm -rf -- $purge_path"
                else
                    rm -rf -- "$purge_path"
                fi
            elif [[ -e "$purge_path" ]]; then
                if [[ "$dry_run" == "true" ]]; then
                    echo "rm -f -- $purge_path"
                else
                    rm -f -- "$purge_path"
                fi
            fi
        done
    fi
done

qt_qml_dirs=(
    "/usr/lib64/qt6/qml"
    "/usr/lib/qt6/qml"
    "/usr/lib/x86_64-linux-gnu/qt6/qml"
    "/usr/local/lib64/qt6/qml"
    "/usr/local/lib/qt6/qml"
    "/usr/local/lib/x86_64-linux-gnu/qt6/qml"
)
if command -v qtpaths6 >/dev/null 2>&1; then
    qt_qml_dirs+=("$(qtpaths6 --query QT_INSTALL_QML 2>/dev/null || true)")
elif command -v qtpaths >/dev/null 2>&1; then
    qt_qml_dirs+=("$(qtpaths --query QT_INSTALL_QML 2>/dev/null || true)")
fi

for qml_dir in "${qt_qml_dirs[@]}"; do
    [[ -z "$qml_dir" ]] && continue

    latte_qml_module_dirs=(
        "${qml_dir}/org/kde/latte/core"
        "${qml_dir}/org/kde/latte/components"
        "${qml_dir}/org/kde/latte/abilities"
        "${qml_dir}/org/kde/latte/private/tasks"
        "${qml_dir}/org/kde/latte/private/containment"
    )

    for qml_module_dir in "${latte_qml_module_dirs[@]}"; do
        remove_tree "$qml_module_dir"
    done

    fallback_dir="${qml_dir}/org/kde/plasma/private/taskmanager"
    marker_file="${fallback_dir}/.latte-fallback-module"
    remove_tree_if_marked "$fallback_dir" "$marker_file"
done

plugin_dirs=(
    "/usr/lib/plugins/plasma/containmentactions"
    "/usr/lib64/plugins/plasma/containmentactions"
    "/usr/lib/x86_64-linux-gnu/plugins/plasma/containmentactions"
    "/usr/local/lib/plugins/plasma/containmentactions"
    "/usr/local/lib64/plugins/plasma/containmentactions"
    "/usr/local/lib/x86_64-linux-gnu/plugins/plasma/containmentactions"
)

for plugin_dir in "${plugin_dirs[@]}"; do
    remove_file "${plugin_dir}/plasma_containmentactions_lattecontextmenu.so"
done

# Best-effort cleanup for now-empty parent dirs created by install.sh sync.
remove_dir_if_empty "/usr/share/latte/indicators"
remove_dir_if_empty "/usr/share/latte"
remove_dir_if_empty "/usr/local/share/latte/indicators"
remove_dir_if_empty "/usr/local/share/latte"
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
