#!/usr/bin/env bash
# Author: Michail Vourlakos
# Summary: Uninstallation script for Latte Dock Panel
#
# Run as root / sudo  → removes system install from /usr  AND all user data
# Run as normal user  → removes user install from ~/.local only
# Override with --system / --user and --no-purge-user-data flags.

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
dry_run="false"
manifest_provided="false"
purge_user_data="false"
install_mode="auto"   # auto | user | system

declare -a manifest_paths=()
declare -a user_homes=()

add_user_home() {
    local candidate="${1:-}"
    [[ -n "$candidate" ]] || return 0
    [[ -d "$candidate" ]] || return 0

    local existing
    for existing in "${user_homes[@]:-}"; do
        [[ "$existing" == "$candidate" ]] && return
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
        [[ -z "$sudo_home" ]] && sudo_home="/home/${SUDO_USER}"
        add_user_home "$sudo_home"
    fi
}

usage() {
    cat <<'EOF'
Usage:
  bash uninstall.sh [OPTIONS]

Install mode (auto-detected from EUID / saved metadata when not specified):
  --user      Remove user-local install from ~/.local  (no sudo needed)
  --system    Remove system install from /usr AND all user data (requires root/sudo)

Options:
  --manifest <path>     Use a specific install manifest
  --dry-run             Print what would be removed without deleting
  --purge-user-data     Also remove user config/cache (default ON for --system)
  --no-purge-user-data  Skip user data removal even in --system mode
EOF
}

while (($# > 0)); do
    case "$1" in
        --user)          install_mode="user" ;;
        --system)        install_mode="system" ;;
        --manifest)
            shift
            (($# > 0)) || { echo "Error: --manifest requires a path." >&2; usage; exit 2; }
            manifest_paths=("$1")
            manifest_provided="true"
            ;;
        --dry-run)            dry_run="true" ;;
        --purge-user-data)    purge_user_data="true" ;;
        --no-purge-user-data) purge_user_data="no" ;;
        --help|-h)            usage; exit 0 ;;
        *)
            echo "Error: unknown option '$1'." >&2; usage; exit 2 ;;
    esac
    shift
done

# ── Resolve install mode ──────────────────────────────────────────────────────
if [[ "$install_mode" == "auto" ]]; then
    # 1. Try reading saved metadata from build directory
    saved_mode=""
    for meta_candidate in \
            "${script_dir}/build/.install-mode" \
            "${script_dir}/build-user-${USER:-user}/.install-mode" \
            "${script_dir}"/build-*/.install-mode; do
        if [[ -f "$meta_candidate" ]]; then
            saved_mode="$(cat "$meta_candidate" 2>/dev/null || true)"
            break
        fi
    done

    if [[ "$saved_mode" == "user" || "$saved_mode" == "system" ]]; then
        install_mode="$saved_mode"
        echo "Info: detected previous install mode from metadata: ${install_mode}"
    elif [[ "${EUID}" -eq 0 ]]; then
        install_mode="system"
    else
        install_mode="user"
    fi
fi

# System mode defaults to full purge; user can override with --no-purge-user-data
if [[ "$install_mode" == "system" && "$purge_user_data" != "no" ]]; then
    purge_user_data="true"
fi

echo "Info: uninstall mode = ${install_mode}, purge user data = ${purge_user_data}"

# ── Resolve install prefix ────────────────────────────────────────────────────
if [[ "$install_mode" == "user" ]]; then
    install_prefix="${HOME}/.local"
    build_dir_pattern="${script_dir}/build-user-${USER:-user}"
    sudo_cmd=()

    # Load saved QML dir if available
    kde_install_qmldir=""
    for qml_meta in \
            "${script_dir}/build-user-${USER:-user}/.install-qmldir" \
            "${script_dir}/build/.install-qmldir"; do
        if [[ -f "$qml_meta" ]]; then
            kde_install_qmldir="$(cat "$qml_meta" 2>/dev/null || true)"
            break
        fi
    done
    # Fallback: derive from lib name
    if [[ -z "$kde_install_qmldir" ]]; then
        qml_lib_name="lib"
        [[ -d "${install_prefix}/lib64/qt6" ]] && qml_lib_name="lib64"
        kde_install_qmldir="${install_prefix}/${qml_lib_name}/qt6/qml"
    fi
else
    install_prefix="/usr"
    build_dir_pattern="${script_dir}/build"
    kde_install_qmldir=""

    if [[ "${EUID}" -ne 0 ]]; then
        sudo_cmd=(sudo)
    else
        sudo_cmd=()
    fi
fi

# ── Collect manifests ─────────────────────────────────────────────────────────
if [[ "$manifest_provided" == "false" ]]; then
    shopt -s nullglob
    for candidate in \
            "${build_dir_pattern}/install_manifest.txt" \
            "${script_dir}/build/install_manifest.txt" \
            "${script_dir}"/build-*/install_manifest.txt; do
        [[ -f "$candidate" ]] && manifest_paths+=("$candidate")
    done
    shopt -u nullglob

    # Keep only manifests matching the install mode (by prefix in paths)
    if [[ ${#manifest_paths[@]} -gt 0 && "$install_mode" == "user" ]]; then
        filtered=()
        for mp in "${manifest_paths[@]}"; do
            if grep -q "^${HOME}/" "$mp" 2>/dev/null || \
               grep -q "^\\.local/" "$mp" 2>/dev/null; then
                filtered+=("$mp")
            elif ! grep -q "^/usr" "$mp" 2>/dev/null; then
                filtered+=("$mp")
            fi
        done
        [[ ${#filtered[@]} -gt 0 ]] && manifest_paths=("${filtered[@]}")
    fi

    [[ ${#manifest_paths[@]} -eq 0 ]] && \
        manifest_paths=("${build_dir_pattern}/install_manifest.txt")
fi

# ── Check manifests ───────────────────────────────────────────────────────────
detect_user_homes

existing_manifest_count=0
for mp in "${manifest_paths[@]}"; do
    [[ -f "$mp" ]] && existing_manifest_count=$((existing_manifest_count + 1))
done

if [[ "$existing_manifest_count" -eq 0 ]]; then
    echo "Warning: uninstall manifests not found — doing package directory cleanup only."
    printf '  - %s\n' "${manifest_paths[@]}"
elif [[ ${#manifest_paths[@]} -eq 1 ]]; then
    echo "Using uninstall manifest: ${manifest_paths[0]}"
else
    echo "Using uninstall manifests:"
    printf '  - %s\n' "${manifest_paths[@]}"
fi

[[ "$dry_run" == "true" ]] && echo "Dry-run mode — no files will be deleted."

# ── Helper functions ──────────────────────────────────────────────────────────
run_as_root() {
    "${sudo_cmd[@]}" "$@"
}

remove_file() {
    local path="$1"
    [[ "$dry_run" == "true" ]] && { echo "rm -f -- $path"; return; }
    run_as_root rm -f -- "$path"
}

remove_tree() {
    local path="$1"
    [[ "$dry_run" == "true" ]] && { echo "rm -rf -- $path"; return; }
    run_as_root rm -rf -- "$path"
}

remove_dir_if_empty() {
    local path="$1"
    [[ "$dry_run" == "true" ]] && { echo "rmdir --ignore-fail-on-non-empty -- $path"; return; }
    run_as_root rmdir --ignore-fail-on-non-empty -- "$path" 2>/dev/null || true
}

remove_tree_if_marked() {
    local path="$1"; local marker="$2"
    [[ -f "$marker" ]] || return 0
    [[ "$dry_run" == "true" ]] && { echo "rm -rf -- $path"; return; }
    run_as_root rm -rf -- "$path"
}

# ── Remove manifest-listed files ─────────────────────────────────────────────
for manifest_file in "${manifest_paths[@]}"; do
    [[ -f "$manifest_file" ]] || continue
    while IFS= read -r file; do
        [[ -z "$file" ]] && continue
        remove_file "$file"
    done < "$manifest_file"
done

# ── Remove managed install-prefix directories and files ───────────────────────
managed_dirs=(
    "${install_prefix}/share/plasma/plasmoids/org.kde.latte.containment"
    "${install_prefix}/share/plasma/plasmoids/org.kde.latte.plasmoid"
    "${install_prefix}/share/plasma/shells/org.kde.latte.shell"
    "${install_prefix}/share/latte/indicators"
)

managed_files=(
    "${install_prefix}/bin/latte-dock-ng"
    "${install_prefix}/share/applications/org.kde.latte-dock.desktop"
    "${install_prefix}/share/metainfo/org.kde.latte-dock.appdata.xml"
    "${install_prefix}/share/dbus-1/interfaces/org.kde.LatteDock.xml"
    "${install_prefix}/share/knotifications6/lattedock.notifyrc"
    "${install_prefix}/share/knsrcfiles/latte-layouts.knsrc"
    "${install_prefix}/share/knsrcfiles/latte-indicators.knsrc"
    "${install_prefix}/share/kservicetypes6/latte-indicator.desktop"
    "${install_prefix}/share/kservices6/latte-indicator.desktop"
    "${install_prefix}/share/icons/breeze/applets/256/org.kde.latte.plasmoid.svg"
)

for dir_path in "${managed_dirs[@]}"; do
    remove_tree "$dir_path"
done

for file_path in "${managed_files[@]}"; do
    remove_file "$file_path"
done

# For system installs also clean /usr/local paths (legacy)
if [[ "$install_mode" == "system" ]]; then
    for prefix in "/usr/local"; do
        for dir_path in \
                "${prefix}/share/plasma/plasmoids/org.kde.latte.containment" \
                "${prefix}/share/plasma/plasmoids/org.kde.latte.plasmoid" \
                "${prefix}/share/plasma/shells/org.kde.latte.shell" \
                "${prefix}/share/latte/indicators"; do
            remove_tree "$dir_path"
        done
        for file_path in \
                "${prefix}/bin/latte-dock-ng" \
                "${prefix}/share/applications/org.kde.latte-dock.desktop" \
                "${prefix}/share/metainfo/org.kde.latte-dock.appdata.xml"; do
            remove_file "$file_path"
        done
    done
fi

# ── Remove user-local overrides (both modes clean user dirs) ──────────────────
remove_user_local_launcher_link() {
    local user_home="$1"
    local local_bin="${user_home}/.local/bin/latte-dock-ng"

    [[ -L "$local_bin" ]] || return 0
    local resolved="$(readlink -f "$local_bin" 2>/dev/null || true)"
    # Remove if pointing to any latte-dock-ng binary
    [[ "$resolved" == */latte-dock-ng ]] || return 0

    [[ "$dry_run" == "true" ]] && { echo "rm -f -- $local_bin"; return; }
    rm -f -- "$local_bin"
}

remove_user_stale_launchers() {
    local user_home="$1"
    shopt -s nullglob
    for stale_entry in "${user_home}"/.local/bin/latte-dock-ng.stale.*; do
        [[ -e "$stale_entry" ]] || continue
        [[ "$dry_run" == "true" ]] && { echo "rm -f -- $stale_entry"; continue; }
        rm -f -- "$stale_entry"
    done
    shopt -u nullglob
}

for user_home in "${user_homes[@]:-}"; do
    if [[ "$install_mode" == "user" ]]; then
        # Full removal of user-local managed dirs
        for dir_path in \
                "${user_home}/.local/share/plasma/plasmoids/org.kde.latte.containment" \
                "${user_home}/.local/share/plasma/plasmoids/org.kde.latte.plasmoid" \
                "${user_home}/.local/share/plasma/shells/org.kde.latte.shell" \
                "${user_home}/.local/share/latte/indicators"; do
            [[ "$dry_run" == "true" ]] && { echo "rm -rf -- $dir_path"; continue; }
            rm -rf -- "$dir_path"
        done

        for file_path in \
                "${user_home}/.local/share/applications/org.kde.latte-dock.desktop" \
                "${user_home}/.local/share/applications/latte-dock.desktop" \
                "${user_home}/.config/autostart/org.kde.latte-dock.desktop" \
                "${user_home}/.config/autostart/latte-dock.desktop"; do
            [[ "$dry_run" == "true" ]] && { echo "rm -f -- $file_path"; continue; }
            rm -f -- "$file_path"
        done
    else
        # System uninstall: clean shadowing user-local overrides too
        for dir_path in \
                "${user_home}/.local/share/plasma/plasmoids/org.kde.latte.containment" \
                "${user_home}/.local/share/plasma/plasmoids/org.kde.latte.plasmoid" \
                "${user_home}/.local/share/plasma/shells/org.kde.latte.shell" \
                "${user_home}/.local/share/latte/indicators"; do
            [[ "$dry_run" == "true" ]] && { echo "rm -rf -- $dir_path"; continue; }
            rm -rf -- "$dir_path"
        done

        for file_path in \
                "${user_home}/.local/share/applications/org.kde.latte-dock.desktop" \
                "${user_home}/.local/share/applications/latte-dock.desktop" \
                "${user_home}/.config/autostart/org.kde.latte-dock.desktop" \
                "${user_home}/.config/autostart/latte-dock.desktop"; do
            [[ "$dry_run" == "true" ]] && { echo "rm -f -- $file_path"; continue; }
            rm -f -- "$file_path"
        done
    fi

    remove_user_local_launcher_link "$user_home"
    remove_user_stale_launchers "$user_home"

    if [[ "$purge_user_data" == "true" ]]; then
        for purge_path in \
                "${user_home}/.config/latte" \
                "${user_home}/.config/lattedockrc" \
                "${user_home}/.config/autostart/org.kde.latte-dock.desktop" \
                "${user_home}/.config/latte-dock-ng" \
                "${user_home}/.local/share/latte-layouts" \
                "${user_home}/.local/share/latte" \
                "${user_home}/.cache/latte-dock" \
                "${user_home}/.cache/lattedock" \
                "${user_home}/.local/state/latte"; do
            if [[ -d "$purge_path" ]]; then
                [[ "$dry_run" == "true" ]] && { echo "rm -rf -- $purge_path"; continue; }
                rm -rf -- "$purge_path"
            elif [[ -e "$purge_path" ]]; then
                [[ "$dry_run" == "true" ]] && { echo "rm -f -- $purge_path"; continue; }
                rm -f -- "$purge_path"
            fi
        done
    fi
done

# ── Remove QML plugins ────────────────────────────────────────────────────────
if [[ "$install_mode" == "user" ]]; then
    qml_dirs=("$kde_install_qmldir")
else
    qml_dirs=(
        "/usr/lib64/qt6/qml"
        "/usr/lib/qt6/qml"
        "/usr/lib/x86_64-linux-gnu/qt6/qml"
        "/usr/local/lib64/qt6/qml"
        "/usr/local/lib/qt6/qml"
    )
    if command -v qtpaths6 >/dev/null 2>&1; then
        qml_dirs+=("$(qtpaths6 --query QT_INSTALL_QML 2>/dev/null || true)")
    elif command -v qtpaths >/dev/null 2>&1; then
        qml_dirs+=("$(qtpaths --query QT_INSTALL_QML 2>/dev/null || true)")
    fi
fi

for qml_dir in "${qml_dirs[@]}"; do
    [[ -z "$qml_dir" ]] && continue

    for qml_module_dir in \
            "${qml_dir}/org/kde/latte/core" \
            "${qml_dir}/org/kde/latte/components" \
            "${qml_dir}/org/kde/latte/abilities" \
            "${qml_dir}/org/kde/latte/private/tasks" \
            "${qml_dir}/org/kde/latte/private/containment"; do
        remove_tree "$qml_module_dir"
    done

    fallback_dir="${qml_dir}/org/kde/plasma/private/taskmanager"
    marker_file="${fallback_dir}/.latte-fallback-module"
    remove_tree_if_marked "$fallback_dir" "$marker_file"
done

# ── Remove containment actions plugin ─────────────────────────────────────────
if [[ "$install_mode" == "system" ]]; then
    plugin_dirs=(
        "/usr/lib/plugins/plasma/containmentactions"
        "/usr/lib64/plugins/plasma/containmentactions"
        "/usr/lib/x86_64-linux-gnu/plugins/plasma/containmentactions"
        "/usr/local/lib/plugins/plasma/containmentactions"
        "/usr/local/lib64/plugins/plasma/containmentactions"
    )
    for plugin_dir in "${plugin_dirs[@]}"; do
        remove_file "${plugin_dir}/plasma_containmentactions_lattecontextmenu.so"
    done
fi

# ── Clean empty parent dirs ───────────────────────────────────────────────────
remove_dir_if_empty "${install_prefix}/share/latte/indicators"
remove_dir_if_empty "${install_prefix}/share/latte"

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

# ── Refresh application menu caches ──────────────────────────────────────────
# Run kbuildsycoca6 (KDE menu cache) and update-desktop-database (XDG) so
# latte-dock entries disappear from launchers immediately after uninstall.

run_as_user() {
    # run_as_user <username> <cmd...>
    local target_user="$1"; shift

    if [[ "${EUID}" -eq 0 && "$target_user" != "root" ]]; then
        if command -v runuser >/dev/null 2>&1; then
            runuser -l "$target_user" -c "$*" 2>/dev/null || true
        else
            su -l "$target_user" -c "$*" 2>/dev/null || true
        fi
    else
        "$@" 2>/dev/null || true
    fi
}

resolve_username() {
    local user_home="$1"
    # Prefer SUDO_USER when the home matches, otherwise stat the directory
    if [[ -n "${SUDO_USER:-}" && "${SUDO_USER}" != "root" ]]; then
        local sudo_home=""
        if command -v getent >/dev/null 2>&1; then
            sudo_home="$(getent passwd "${SUDO_USER}" | cut -d: -f6 2>/dev/null || true)"
        fi
        [[ "$sudo_home" == "$user_home" ]] && echo "$SUDO_USER" && return
    fi
    stat -c '%U' "$user_home" 2>/dev/null || basename "$user_home"
}

for user_home in "${user_homes[@]:-}"; do
    [[ -d "$user_home" ]] || continue
    target_user="$(resolve_username "$user_home")"

    # Refresh user-level KDE service cache
    if command -v kbuildsycoca6 >/dev/null 2>&1; then
        if [[ "$dry_run" == "true" ]]; then
            echo "su $target_user -c 'kbuildsycoca6 --noincremental'"
        else
            echo "Info: refreshing KDE menu cache for user '${target_user}'..."
            run_as_user "$target_user" kbuildsycoca6 --noincremental
        fi
    fi

    # Refresh user-level XDG desktop database
    if command -v update-desktop-database >/dev/null 2>&1; then
        user_apps_dir="${user_home}/.local/share/applications"
        if [[ -d "$user_apps_dir" ]]; then
            if [[ "$dry_run" == "true" ]]; then
                echo "update-desktop-database $user_apps_dir"
            else
                run_as_user "$target_user" update-desktop-database "$user_apps_dir"
            fi
        fi
    fi
done

# Refresh system-level XDG desktop database (system install only)
if [[ "$install_mode" == "system" ]]; then
    if command -v update-desktop-database >/dev/null 2>&1; then
        if [[ "$dry_run" == "true" ]]; then
            echo "update-desktop-database /usr/share/applications"
        else
            run_as_root update-desktop-database /usr/share/applications 2>/dev/null || true
        fi
    fi
fi

# ── Remove build metadata ─────────────────────────────────────────────────────
for meta_file in \
        "${script_dir}/build/.install-mode" \
        "${script_dir}/build/.install-prefix" \
        "${script_dir}/build/.install-qmldir" \
        "${script_dir}/build-user-${USER:-user}/.install-mode" \
        "${script_dir}/build-user-${USER:-user}/.install-prefix" \
        "${script_dir}/build-user-${USER:-user}/.install-qmldir"; do
    [[ -f "$meta_file" ]] || continue
    [[ "$dry_run" == "true" ]] && { echo "rm -f -- $meta_file"; continue; }
    rm -f -- "$meta_file"
done

[[ "$dry_run" == "true" ]] && echo "Dry run complete." || echo "Uninstall complete."
