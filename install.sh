#!/usr/bin/env bash
# Author: Michail Vourlakos, Smith Ar
# Summary: Installation script for Latte Dock Panel
#
# Run as root / sudo  → system install to /usr        (needs write access)
# Run as normal user  → user install to ~/.local      (no sudo needed)
# Override with --system or --user flags.

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

build_type="Release"
enable_make_unique="OFF"
l10n_auto_translations="OFF"
l10n_branch=""
preclean_install="true"
purge_user_data="false"
install_mode="auto"   # auto | user | system

declare -a user_homes=()

add_user_home() {
    local candidate="${1:-}"
    [[ -n "$candidate" ]] || return
    [[ -d "$candidate" ]] || return

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
  bash install.sh [OPTIONS] [BuildType]

Install mode (auto-detected from EUID when not specified):
  --user      Install to ~/.local  (no sudo; for debugging)
  --system    Install to /usr      (requires root/sudo)

Build options:
  Debug | Release | RelWithDebInfo | MinSizeRel
  --enable-make-unique
  --translations | --translations-stable
  --clean | --no-clean      (default: --clean)
  --purge-user-data         Wipe user config/cache on clean

Examples:
  bash install.sh                 # auto: user if no root, system if root
  bash install.sh Debug           # same, Debug build
  bash install.sh --user Debug    # always user-local
  sudo bash install.sh            # system install
  sudo bash install.sh --system   # explicit system install
EOF
}

build_type_is_set="false"

for arg in "$@"; do
    case "$arg" in
        --help|-h)            usage; exit 0 ;;
        --user)               install_mode="user" ;;
        --system)             install_mode="system" ;;
        --enable-make-unique) enable_make_unique="ON" ;;
        --clean)              preclean_install="true" ;;
        --no-clean)           preclean_install="false" ;;
        --purge-user-data)    purge_user_data="true" ;;
        --translations)       l10n_auto_translations="ON"; l10n_branch="trunk" ;;
        --translations-stable) l10n_auto_translations="ON"; l10n_branch="stable" ;;
        Release|Debug|RelWithDebInfo|MinSizeRel)
            if [[ "$build_type_is_set" == "true" ]]; then
                echo "Error: multiple build types provided." >&2; usage; exit 2
            fi
            build_type="$arg"; build_type_is_set="true" ;;
        -*)
            echo "Error: unknown option '$arg'." >&2; usage; exit 2 ;;
        *)
            if [[ "$build_type_is_set" == "true" ]]; then
                echo "Error: multiple build types provided." >&2; usage; exit 2
            fi
            build_type="$arg"; build_type_is_set="true" ;;
    esac
done

# ── Resolve install mode ─────────────────────────────────────────────────────
if [[ "$install_mode" == "auto" ]]; then
    if [[ "${EUID}" -eq 0 ]]; then
        install_mode="system"
    else
        install_mode="user"
    fi
fi

echo "Info: install mode = ${install_mode}"

if [[ "$install_mode" == "user" ]]; then
    install_prefix="${HOME}/.local"
    build_dir="${script_dir}/build-user-${USER:-user}"

    # Detect Qt QML lib subdir (lib vs lib64) by querying the system path
    sys_qml_dir=""
    if command -v qtpaths6 >/dev/null 2>&1; then
        sys_qml_dir="$(qtpaths6 --query QT_INSTALL_QML 2>/dev/null || true)"
    fi
    if [[ -z "$sys_qml_dir" ]] && command -v qtpaths >/dev/null 2>&1; then
        sys_qml_dir="$(qtpaths --query QT_INSTALL_QML 2>/dev/null || true)"
    fi
    # Extract the lib* component (e.g. "lib64" from "/usr/lib64/qt6/qml")
    if [[ -n "$sys_qml_dir" ]]; then
        qml_lib_name="$(echo "$sys_qml_dir" | grep -oP 'lib\d*(?=/qt6)' || echo "lib")"
    else
        qml_lib_name="lib"
        # Fall back: check which lib64 or lib exists on the system
        [[ -d "/usr/lib64/qt6" ]] && qml_lib_name="lib64"
    fi
    kde_install_qmldir="${install_prefix}/${qml_lib_name}/qt6/qml"

    sudo_cmd=()   # Never need sudo for user-local install
else
    install_prefix="/usr"
    build_dir="${script_dir}/build"
    kde_install_qmldir=""   # Use cmake/KDE defaults for system install

    if [[ "${EUID}" -ne 0 ]]; then
        sudo_cmd=(sudo)
    else
        sudo_cmd=()
    fi

    # If the default build dir isn't writable (built by another user), use a personal one
    if [[ -d "$build_dir" && ! -w "$build_dir" ]]; then
        build_dir="${script_dir}/build-${USER:-user}"
        echo "Info: '${script_dir}/build' is not writable, using '${build_dir}'."
    fi
fi

# ── Pre-install cleanup ───────────────────────────────────────────────────────
if [[ "$preclean_install" == "true" ]]; then
    uninstall_cmd=(bash "${script_dir}/uninstall.sh" "--${install_mode}")
    [[ "$purge_user_data" == "true" ]] && uninstall_cmd+=(--purge-user-data)
    echo "Info: running pre-install cleanup: ${uninstall_cmd[*]}"
    "${uninstall_cmd[@]}"
elif [[ "$purge_user_data" == "true" ]]; then
    echo "Warning: --purge-user-data is ignored when --no-clean is set."
fi

# ── Build ─────────────────────────────────────────────────────────────────────
mkdir -p "$build_dir"
cd "$build_dir"

cmake_args=(
    -DCMAKE_INSTALL_PREFIX="${install_prefix}"
    -DENABLE_MAKE_UNIQUE="${enable_make_unique}"
    -DCMAKE_BUILD_TYPE="${build_type}"
)

if [[ -n "$kde_install_qmldir" ]]; then
    cmake_args+=(-DKDE_INSTALL_QMLDIR="${kde_install_qmldir}")
fi

if [[ "$l10n_auto_translations" == "ON" ]]; then
    cmake_args+=(-DKDE_L10N_AUTO_TRANSLATIONS=ON -DKDE_L10N_BRANCH="${l10n_branch}")
else
    cmake_args+=(-DKDE_L10N_AUTO_TRANSLATIONS=OFF)
fi

cmake "${cmake_args[@]}" ..

[[ "$l10n_auto_translations" == "ON" ]] && cmake --build . --target fetch-translations

cmake --build . --parallel --clean-first

# ── Helper functions ──────────────────────────────────────────────────────────
run_as_root() {
    "${sudo_cmd[@]}" "$@"
}

sync_tree() {
    local src="$1"
    local dst="$2"

    if [[ ! -d "$src" ]]; then
        echo "Warning: source directory not found, skipping sync: $src" >&2
        return
    fi

    run_as_root mkdir -p "$dst"

    if command -v rsync >/dev/null 2>&1; then
        run_as_root rsync -a --delete "$src"/ "$dst"/
    else
        run_as_root rm -rf "$dst"
        run_as_root mkdir -p "$dst"
        run_as_root cp -a "$src"/. "$dst"/
    fi
}

sync_tree_if_exists() {
    local src="$1"
    local dst="$2"

    [[ -d "$src" ]] || return
    [[ -d "$dst" ]] || return

    echo "Info: syncing existing user override: $dst"
    if command -v rsync >/dev/null 2>&1; then
        rsync -a --delete "$src"/ "$dst"/
    else
        rm -rf "$dst"; mkdir -p "$dst"; cp -a "$src"/. "$dst"/
    fi
}

# ── Install ───────────────────────────────────────────────────────────────────
detect_user_homes

run_as_root cmake --install .

# Sync full package trees (CMake may leave directories incomplete)
share_dir="${install_prefix}/share"
sync_tree "${script_dir}/containment/package" "${share_dir}/plasma/plasmoids/org.kde.latte.containment"
sync_tree "${script_dir}/plasmoid/package"    "${share_dir}/plasma/plasmoids/org.kde.latte.plasmoid"
sync_tree "${script_dir}/shell/package"       "${share_dir}/plasma/shells/org.kde.latte.shell"
sync_tree "${script_dir}/indicators"          "${share_dir}/latte/indicators"

if [[ "$install_mode" == "system" ]]; then
    # For system installs, keep any user-local overrides in sync to avoid stale QML
    for user_home in "${user_homes[@]:-}"; do
        sync_tree_if_exists "${script_dir}/containment/package" "${user_home}/.local/share/plasma/plasmoids/org.kde.latte.containment"
        sync_tree_if_exists "${script_dir}/plasmoid/package"    "${user_home}/.local/share/plasma/plasmoids/org.kde.latte.plasmoid"
        sync_tree_if_exists "${script_dir}/shell/package"       "${user_home}/.local/share/plasma/shells/org.kde.latte.shell"
        sync_tree_if_exists "${script_dir}/indicators"          "${user_home}/.local/share/latte/indicators"
    done

    # Create a ~/.local/bin symlink to the system binary for convenience
    ensure_user_local_launcher() {
        local user_home="$1"
        local local_bin_dir="${user_home}/.local/bin"
        local local_bin="${local_bin_dir}/latte-dock-ng"
        local system_bin="${install_prefix}/bin/latte-dock-ng"

        [[ -x "$system_bin" ]] || return

        if [[ -e "$local_bin" ]]; then
            local resolved_local
            resolved_local="$(readlink -f "$local_bin" 2>/dev/null || true)"
            if [[ "$resolved_local" != "$system_bin" ]]; then
                local backup_path="${local_bin}.stale.$(date +%Y%m%d-%H%M%S)"
                echo "Info: found conflicting user-local launcher '${local_bin}', moving to '${backup_path}'."
                mv -f "$local_bin" "$backup_path"
            fi
        fi

        mkdir -p "$local_bin_dir"
        ln -sfn "$system_bin" "$local_bin"
        echo "Info: linked '${local_bin}' -> '${system_bin}'."
    }

    for user_home in "${user_homes[@]:-}"; do
        ensure_user_local_launcher "$user_home"
    done
fi

# ── Compat QML module (fallback for missing org.kde.plasma.private.taskmanager) ──
if [[ "$install_mode" == "user" ]]; then
    qt_qml_dir="$kde_install_qmldir"
else
    qt_qml_dir="/usr/lib64/qt6/qml"
    if command -v qtpaths6 >/dev/null 2>&1; then
        qt_qml_dir="$(qtpaths6 --query QT_INSTALL_QML 2>/dev/null || true)"
    elif command -v qtpaths >/dev/null 2>&1; then
        qt_qml_dir="$(qtpaths --query QT_INSTALL_QML 2>/dev/null || true)"
    fi
    [[ -z "$qt_qml_dir" ]] && qt_qml_dir="/usr/lib64/qt6/qml"
fi

fallback_taskmanager_dir="${qt_qml_dir}/org/kde/plasma/private/taskmanager"
if [[ ! -d "$fallback_taskmanager_dir" ]]; then
    sync_tree "${script_dir}/compat/qml/org/kde/plasma/private/taskmanager" "$fallback_taskmanager_dir"
    run_as_root touch "${fallback_taskmanager_dir}/.latte-fallback-module"
fi

# ── Save install metadata ─────────────────────────────────────────────────────
printf '%s\n' "$install_mode"   > "${build_dir}/.install-mode"
printf '%s\n' "$install_prefix" > "${build_dir}/.install-prefix"
[[ -n "$kde_install_qmldir" ]] && printf '%s\n' "$kde_install_qmldir" > "${build_dir}/.install-qmldir"

# ── Post-install instructions ─────────────────────────────────────────────────
if [[ "$install_mode" == "user" ]]; then
    env_file="${HOME}/.config/latte-dock-ng/dev-env.sh"
    mkdir -p "${HOME}/.config/latte-dock-ng"
    cat > "$env_file" <<ENVEOF
# latte-dock-ng user-local install environment
# Generated by install.sh — source this before running latte-dock-ng
export QT_QML_IMPORT_PATH="${kde_install_qmldir}\${QT_QML_IMPORT_PATH:+:\${QT_QML_IMPORT_PATH}}"
ENVEOF

    cat <<EOF
Info: User-local installation complete.
Info: Binary:   ${install_prefix}/bin/latte-dock-ng
Info: QML path: ${kde_install_qmldir}

To run latte-dock-ng from this install, set the QML import path first:

  source ${env_file}
  latte-dock-ng

Or add it permanently to your shell config:

  echo 'source ${env_file}' >> ~/.bashrc    # bash
  echo 'source ${env_file}' >> ~/.zshrc     # zsh

EOF
else
    cat <<'EOF'
Info: System installation complete.
Info: If your shell has a cached binary path, refresh it:
  zsh:  rehash
  bash: hash -r
EOF
fi
