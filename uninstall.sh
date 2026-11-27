#!/usr/bin/env bash
# Author: Ruizhi Zhong <ruizhi.zhong88@gmail.com>
# Summary: Uninstallation script for Latte Dock NG
#
# Run as root / sudo  → remove the source installation from /usr
# Run as normal user  → remove the user installation under ~/.local
# User data and autostart entries are preserved by default.

set -euo pipefail

dry_run="false"
manifest_file=""
purge_user_data="false"
legacy_user="false"
keep_install="false"
install_mode="auto"   # auto | user | system

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

Install mode (auto-detected from EUID when not specified):
  --user      Remove the isolated Debug install, or the legacy user install
  --legacy-user  Target the legacy ~/.local install when both are present
  --system    Remove the source installation from /usr (requires root/sudo)

Options:
  --manifest <path>     Use a manifest for a shared-prefix installation
  --keep-install        Clear caches without removing the installation
  --dry-run             Print what would be removed without deleting
  --purge-user-data     Also remove user config/layout data
  --no-purge-user-data  Keep user config/layout data; app caches are always removed
  --preserve-autostart  Keep user XDG autostart entries (default)
EOF
}

while (($# > 0)); do
    case "$1" in
        --user)          install_mode="user" ;;
        --system)        install_mode="system" ;;
        --legacy-user)   install_mode="user"; legacy_user="true" ;;
        --manifest)
            shift
            (($# > 0)) || { echo "Error: --manifest requires a path." >&2; usage; exit 2; }
            manifest_file="$1"
            ;;
        --keep-install)       keep_install="true" ;;
        --dry-run)            dry_run="true" ;;
        --purge-user-data)    purge_user_data="true" ;;
        --no-purge-user-data) purge_user_data="false" ;;
        --preserve-autostart) ;;
        --help|-h)            usage; exit 0 ;;
        *)
            echo "Error: unknown option '$1'." >&2; usage; exit 2 ;;
    esac
    shift
done

# ── Resolve install mode ──────────────────────────────────────────────────────
if [[ "$install_mode" == "auto" ]]; then
    if [[ "${EUID}" -eq 0 ]]; then
        install_mode="system"
    else
        install_mode="user"
    fi
fi

echo "Info: uninstall mode = ${install_mode}, purge user data = ${purge_user_data}"

# ── Resolve install prefix ────────────────────────────────────────────────────
if [[ "$install_mode" == "user" ]]; then
    install_prefix="${HOME}/.local"
    sudo_cmd=()
else
    install_prefix="/usr"
    if [[ "$keep_install" != "true" ]]; then
        for package_db in /var/db/pkg/kde-misc/latte-dock-ng-*/CONTENTS; do
            if [[ -f "$package_db" ]]; then
                echo "Error: Portage manages the system Latte Dock NG installation." >&2
                exit 1
            fi
        done
    fi

    if [[ "${EUID}" -ne 0 ]]; then
        sudo_cmd=(sudo)
    else
        sudo_cmd=()
    fi
fi

detect_user_homes

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

# User-home files are removed without sudo, as the invoking user.
remove_user_tree() {
    local path="$1"
    [[ "$dry_run" == "true" ]] && { echo "rm -rf -- $path"; return; }
    rm -rf -- "$path"
}

remove_latte_caches() {
    local user_home="$1"
    local cache_root runtime_dir account_home
    local -a cache_roots=("${user_home}/.cache")

    if [[ "$user_home" == "${HOME:-}" && -O "$user_home" ]]; then
        if [[ "${XDG_CACHE_HOME:-}" == /* && -O "$XDG_CACHE_HOME" ]]; then
            cache_roots+=("$XDG_CACHE_HOME")
        fi

        # Plasma can put its cache under the per-user runtime directory even
        # when this script runs from a shell without the session environment.
        runtime_dir="${XDG_RUNTIME_DIR:-}"
        if [[ -z "$runtime_dir" ]]; then
            # An alternate HOME, such as a test profile, does not own the
            # account's graphical session cache.
            account_home="$(getent passwd "$EUID" | cut -d: -f6 || true)"
            if [[ "$user_home" == "$account_home" ]]; then
                runtime_dir="/run/user/${EUID}"
            fi
        fi
        if [[ "$runtime_dir" == /* && -d "$runtime_dir/cache" && -O "$runtime_dir/cache" ]]; then
            cache_roots+=("$runtime_dir/cache")
        fi
    fi

    for cache_root in "${cache_roots[@]}"; do
        remove_user_tree "${cache_root}/latte-dock"
        remove_user_tree "${cache_root}/lattedock"
        remove_user_tree "${cache_root}/latte-dock-ng-dev"
    done
}

# latte/indicators also holds indicators installed through "Get New Indicators"
# (KNewStuff) in the user's data directory, and distribution packages may add
# their own under /usr. Only Latte's entries are removed here: the shipped
# packages, org.kde.latte.plasma from earlier releases, and the build files
# that older install.sh versions copied there. Removing the whole directory
# deleted downloaded indicators on every upgrade; --purge-user-data still
# deletes the whole user data directory, downloads included.
latte_indicator_entries=(
    default
    org.kde.latte.plasmatabstyle
    org.kde.latte.plasma
    CMakeLists.txt
    Messages.sh
)

# Usage: remove_latte_indicator_packages <indicators dir> <remove function>
remove_latte_indicator_packages() {
    local indicators_dir="$1"
    local remove_function="$2"
    local entry
    for entry in "${latte_indicator_entries[@]}"; do
        "$remove_function" "${indicators_dir}/${entry}"
    done
}

purge_latte_user_data() {
    local user_home="$1"
    local purge_path
    if [[ "$purge_user_data" == "true" ]]; then
        for purge_path in \
                "${user_home}/.config/latte" \
                "${user_home}/.config/lattedockrc" \
                "${user_home}/.local/share/latte-layouts" \
                "${user_home}/.local/share/latte" \
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
}

if [[ "$keep_install" == "true" ]]; then
    for user_home in "${user_homes[@]}"; do
        remove_latte_caches "$user_home"
        purge_latte_user_data "$user_home"
    done
    exit 0
fi

if [[ "$dry_run" != "true" && -d "$install_prefix" ]]; then
    exec {install_lock}< "$install_prefix"
    flock --exclusive "$install_lock"
fi

debug_prefix="${HOME}/.local/opt/latte-dock-ng-dev"
if [[ "$install_mode" == "user" && "$legacy_user" != "true" && -z "$manifest_file" ]] \
        && [[ -d "$debug_prefix" || -f "${HOME}/.local/bin/latte-dock-ng-dev" ]]; then
    remove_user_tree "$debug_prefix"
    remove_file "${HOME}/.local/bin/latte-dock-ng-dev"
    remove_file "${HOME}/.config/latte-dock-ng/dev-env.sh"
    remove_file "${HOME}/.local/share/applications/org.kde.latte-dock.dev.desktop"
    remove_file "${HOME}/.local/share/applications/org.kde.latte-dock.preview.dev.desktop"
    remove_latte_caches "$HOME"
    purge_latte_user_data "$HOME"
    if [[ "$dry_run" != "true" ]] && command -v kbuildsycoca6 >/dev/null 2>&1; then
        kbuildsycoca6 --noincremental >/dev/null 2>&1 || true
    fi
    exit 0
fi

# ── Remove manifest-listed files ─────────────────────────────────────────────
installed_manifest="${install_prefix}/share/latte-dock-ng/install-manifest.txt"
manifest_file="${manifest_file:-$installed_manifest}"
complete_manifest="false"
if [[ "$manifest_file" == "$installed_manifest" && -f "$installed_manifest" ]] &&
        grep -Fxq -- "$installed_manifest" "$installed_manifest"; then
    complete_manifest="true"
fi
if [[ -f "$manifest_file" ]]; then
    echo "Using uninstall manifest: $manifest_file"
    while IFS= read -r file || [[ -n "$file" ]]; do
        [[ -z "$file" ]] && continue
        # Skip old fallback taskmanager files that may still appear in
        # manifests from previous builds. These are handled safely by
        # the migration cleanup below (checks .latte-fallback-module).
        if [[ "$file" == */org/kde/plasma/private/taskmanager/* ]]; then
            continue
        fi
        # A stale or supplied manifest must never remove files outside this
        # install prefix. Canonicalization also rejects escaping parent links.
        if [[ "$file" != /* || "${file##*/}" == "." || "${file##*/}" == ".." ]]; then
            continue
        fi
        parent_dir="$(realpath -m -- "${file%/*}")"
        case "$parent_dir" in
            "${install_prefix}"|"${install_prefix}"/*)
                remove_file "$parent_dir/${file##*/}"
                if [[ "$complete_manifest" == "true" ]]; then
                    # Stop at the top-level prefix directories, which are shared.
                    while [[ "${parent_dir#"$install_prefix"/}" == */* && "$parent_dir" != "$install_prefix" ]]; do
                        remove_dir_if_empty "$parent_dir"
                        parent_dir="${parent_dir%/*}"
                    done
                fi
                ;;
            *) echo "Warning: skipping manifest path outside install prefix: $file" >&2 ;;
        esac
    done < "$manifest_file"
else
    echo "Warning: uninstall manifest not found. Using legacy package cleanup."
fi

if [[ "$complete_manifest" != "true" ]]; then
# ── Remove managed install-prefix directories and files ───────────────────────
managed_dirs=(
    "${install_prefix}/share/plasma/plasmoids/org.kde.latte.containment"
    "${install_prefix}/share/plasma/plasmoids/org.kde.latte.plasmoid"
    "${install_prefix}/share/plasma/plasmoids/org.kde.latte.separator"
    "${install_prefix}/share/plasma/shells/org.kde.latte.shell"
    "${install_prefix}/share/latte-dock-ng/help"
)

# The preview desktop entry is an install artifact, not purgeable user data.
# KWin resolves screencast authorization through its Exec path; a stale user
# entry shadows the system entry and denies capture after switching installs.
managed_files=(
    "${install_prefix}/share/latte-dock-ng/install-manifest.txt"
    "${install_prefix}/bin/latte-dock-ng"
    "${install_prefix}/bin/latte-dock-ng-add-launcher"
    "${install_prefix}/bin/latte-dock-ng-preview"
    "${install_prefix}/share/applications/org.kde.latte-dock.desktop"
    "${install_prefix}/share/applications/org.kde.latte-dock.preview.desktop"
    "${install_prefix}/share/plasma/kickeractions/org.kde.latte-dock.kickeractions.desktop"
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
remove_latte_indicator_packages "${install_prefix}/share/latte/indicators" remove_tree

for file_path in "${managed_files[@]}"; do
    remove_file "$file_path"
done

# Remove only named Latte assets, never shared icon or locale directories.
cleanup_prefix_assets() {
    local prefix="$1" size name domain path lib
    for size in 16x16 22x22 24x24 32x32 48x48 scalable; do
        for name in latte-dock.svg latte-dock.png latte-dock-ng.svg latte-dock-ng.png; do
            remove_file "${prefix}/share/icons/hicolor/${size}/apps/${name}"
        done
    done
    remove_file "${prefix}/share/icons/hicolor/scalable/apps/org.kde.latte.plasmoid.svg"
    for domain in latte-dock plasma_containmentactions_lattecontextmenu plasma_applet_org.kde.latte.plasmoid plasma_applet_org.kde.latte.containment latte_indicator_org.kde.latte.default latte_indicator_org.kde.latte.plasma; do
        for path in "${prefix}"/share/locale/*/LC_MESSAGES/"${domain}.mo"; do
            [[ -f "$path" ]] && remove_file "$path"
        done
    done
    for lib in lib lib64 lib/x86_64-linux-gnu; do
        for path in "${prefix}/${lib}/plugins" "${prefix}/${lib}/qt6/plugins"; do
            remove_file "${path}/kf6/packagestructure/latte_indicator.so"
            remove_file "${path}/plasma/containmentactions/org.kde.latte.contextmenu.so"
            remove_file "${path}/plasma/containmentactions/plasma_containmentactions_lattecontextmenu.so"
        done
    done
    return 0
}
cleanup_prefix_assets "$install_prefix"
[[ "$install_mode" != "system" ]] || cleanup_prefix_assets /usr/local

# For system installs also clean /usr/local paths (legacy)
if [[ "$install_mode" == "system" ]]; then
    for prefix in "/usr/local"; do
        for dir_path in \
                "${prefix}/share/plasma/plasmoids/org.kde.latte.containment" \
                "${prefix}/share/plasma/plasmoids/org.kde.latte.plasmoid" \
                "${prefix}/share/plasma/plasmoids/org.kde.latte.separator" \
                "${prefix}/share/plasma/shells/org.kde.latte.shell"; do
            remove_tree "$dir_path"
        done
        remove_latte_indicator_packages "${prefix}/share/latte/indicators" remove_tree
        remove_dir_if_empty "${prefix}/share/latte/indicators"
        for file_path in \
                "${prefix}/bin/latte-dock-ng" \
                "${prefix}/bin/latte-dock-ng-add-launcher" \
                "${prefix}/bin/latte-dock-ng-preview" \
                "${prefix}/share/applications/org.kde.latte-dock.desktop" \
                "${prefix}/share/applications/org.kde.latte-dock.preview.desktop" \
                "${prefix}/share/plasma/kickeractions/org.kde.latte-dock.kickeractions.desktop" \
                "${prefix}/share/metainfo/org.kde.latte-dock.appdata.xml"; do
            remove_file "$file_path"
        done
    done
fi

fi

# ── Clean the user's cache and optional data ──────────────────────────────────
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
    if [[ "$install_mode" == "user" && ! -d "${user_home}/.local/opt/latte-dock-ng-dev" ]]; then
        remove_file "${user_home}/.config/latte-dock-ng/dev-env.sh"
    fi

    # Compiled QML and graphics caches can survive an upgrade with stale
    # runtime artifacts, so clear application-owned caches on every uninstall
    # and install pre-clean. These caches are reproducible, unlike user data.
    remove_latte_caches "$user_home"

    if [[ "$install_mode" == "system" && "$purge_user_data" != "true" ]]; then
        continue
    fi

    remove_user_local_launcher_link "$user_home"
    remove_user_stale_launchers "$user_home"

    # This private compatibility tree is generated by Latte. Shared Qt
    # module directories can belong to other installations.
    remove_user_tree "${user_home}/.local/share/latte-dock-ng/qml-kns-compat"
    remove_file "${user_home}/.local/share/latte-dock-ng/kns-compat.stamp"
    remove_dir_if_empty "${user_home}/.local/share/latte-dock-ng"

    purge_latte_user_data "$user_home"
done

if [[ "$complete_manifest" != "true" ]]; then
# ── Remove QML plugins ────────────────────────────────────────────────────────
if [[ "$install_mode" == "user" ]]; then
    qml_dirs=(
        "${install_prefix}/lib64/qt6/qml"
        "${install_prefix}/lib/qt6/qml"
        "${install_prefix}/lib/x86_64-linux-gnu/qt6/qml"
        "${install_prefix}/lib64/qml"
        "${install_prefix}/lib/qml"
    )
else
    qml_dirs=(
        "/usr/lib64/qt6/qml"
        "/usr/lib/qt6/qml"
        "/usr/lib/x86_64-linux-gnu/qt6/qml"
        "/usr/lib64/qml"
        "/usr/lib/qml"
    )
    if command -v qtpaths6 >/dev/null 2>&1; then
        qml_dirs+=("$(qtpaths6 --query QT_INSTALL_QML 2>/dev/null || true)")
    elif command -v qtpaths >/dev/null 2>&1; then
        qml_dirs+=("$(qtpaths --query QT_INSTALL_QML 2>/dev/null || true)")
    fi
fi

for qml_dir in "${qml_dirs[@]}"; do
    [[ -n "$qml_dir" ]] || continue
    resolved_qml_dir="$(realpath -m -- "$qml_dir")"
    case "$resolved_qml_dir" in
        "${install_prefix}"/lib/*|"${install_prefix}"/lib64/*) ;;
        *) continue ;;
    esac

    for qml_module_dir in \
            "${qml_dir}/org/kde/latte/core" \
            "${qml_dir}/org/kde/latte/components" \
            "${qml_dir}/org/kde/latte/abilities" \
            "${qml_dir}/org/kde/latte/private/tasks" \
            "${qml_dir}/org/kde/latte/private/containment" \
            "${qml_dir}/org/kde/latte/private/app" \
            "${qml_dir}/org/kde/latte/compat/taskmanager"; do
        remove_tree "$qml_module_dir"
    done

    # Only Latte-marked legacy fallbacks belong to us; Plasma owns unmarked
    # modules at this shared URI and they must survive uninstall.
    legacy_module="${qml_dir}/org/kde/plasma/private/taskmanager"
    if [[ -f "${legacy_module}/.latte-fallback-module" ]]; then
        remove_tree "$legacy_module"
    fi

    # Drop empty parent dirs left after module removal
    for parent_dir in \
            "${qml_dir}/org/kde/latte/private" \
            "${qml_dir}/org/kde/latte/compat" \
            "${qml_dir}/org/kde/latte"; do
        remove_dir_if_empty "$parent_dir"
    done
done

# ── Remove containment actions plugin ─────────────────────────────────────────
if [[ "$install_mode" == "system" ]]; then
    plugin_dirs=(
        "/usr/lib/qt6/plugins/plasma/containmentactions"
        "/usr/lib64/qt6/plugins/plasma/containmentactions"
        "/usr/lib/x86_64-linux-gnu/qt6/plugins/plasma/containmentactions"
        "/usr/local/lib/qt6/plugins/plasma/containmentactions"
        "/usr/local/lib64/qt6/plugins/plasma/containmentactions"
        "/usr/lib/plugins/plasma/containmentactions"
        "/usr/lib64/plugins/plasma/containmentactions"
        "/usr/lib/x86_64-linux-gnu/plugins/plasma/containmentactions"
        "/usr/local/lib/plugins/plasma/containmentactions"
        "/usr/local/lib64/plugins/plasma/containmentactions"
    )
    for plugin_dir in "${plugin_dirs[@]}"; do
        remove_file "${plugin_dir}/org.kde.latte.contextmenu.so"
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

fi

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

[[ "$dry_run" == "true" ]] && echo "Dry run complete." || echo "Uninstall complete."
