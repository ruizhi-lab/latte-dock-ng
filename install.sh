#!/usr/bin/env bash
# Author: Michail Vourlakos, Smith Ar
# Summary: Installation script for Latte Dock Panel

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

build_type="Release"
enable_make_unique="OFF"
l10n_auto_translations="OFF"
l10n_branch=""

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
  bash install.sh [BuildType] [--enable-make-unique] [--translations|--translations-stable]

Examples:
  bash install.sh
  bash install.sh Debug
  bash install.sh RelWithDebInfo --enable-make-unique
  bash install.sh --translations
  bash install.sh --translations-stable
EOF
}

build_type_is_set="false"

for arg in "$@"; do
    case "$arg" in
        --help|-h)
            usage
            exit 0
            ;;
        --enable-make-unique)
            enable_make_unique="ON"
            ;;
        --translations)
            l10n_auto_translations="ON"
            l10n_branch="trunk"
            ;;
        --translations-stable)
            l10n_auto_translations="ON"
            l10n_branch="stable"
            ;;
        Release|Debug|RelWithDebInfo|MinSizeRel)
            if [[ "$build_type_is_set" == "true" ]]; then
                echo "Error: multiple build types provided." >&2
                usage
                exit 2
            fi
            build_type="$arg"
            build_type_is_set="true"
            ;;
        *)
            if [[ "$arg" == -* ]]; then
                echo "Error: unknown option '$arg'." >&2
                usage
                exit 2
            fi

            if [[ "$build_type_is_set" == "true" ]]; then
                echo "Error: multiple build types provided." >&2
                usage
                exit 2
            fi

            build_type="$arg"
            build_type_is_set="true"
            ;;
    esac
done

build_dir="${script_dir}/build"
if [[ -d "$build_dir" && ! -w "$build_dir" ]]; then
    user_tag="${USER:-user}"
    build_dir="${script_dir}/build-${user_tag}"
    echo "Info: '${script_dir}/build' is not writable, using '${build_dir}' instead."
fi

mkdir -p "$build_dir"
cd "$build_dir"

cmake_args=(
    -DCMAKE_INSTALL_PREFIX=/usr
    -DENABLE_MAKE_UNIQUE="${enable_make_unique}"
    -DCMAKE_BUILD_TYPE="${build_type}"
)

if [[ "$l10n_auto_translations" == "ON" ]]; then
    cmake_args+=(
        -DKDE_L10N_AUTO_TRANSLATIONS=ON
        -DKDE_L10N_BRANCH="${l10n_branch}"
    )
else
    cmake_args+=(-DKDE_L10N_AUTO_TRANSLATIONS=OFF)
fi

cmake "${cmake_args[@]}" ..

if [[ "$l10n_auto_translations" == "ON" ]]; then
    cmake --build . --target fetch-translations
fi

cmake --build . --parallel --clean-first

if [[ "${EUID}" -eq 0 ]]; then
    sudo_cmd=()
else
    sudo_cmd=(sudo)
fi

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

    if [[ ! -d "$src" ]]; then
        return
    fi

    if [[ ! -d "$dst" ]]; then
        return
    fi

    echo "Info: syncing existing user override: $dst"
    if command -v rsync >/dev/null 2>&1; then
        rsync -a --delete "$src"/ "$dst"/
    else
        rm -rf "$dst"
        mkdir -p "$dst"
        cp -a "$src"/. "$dst"/
    fi
}

ensure_user_local_launcher() {
    local user_home="$1"
    local local_bin_dir="${user_home}/.local/bin"
    local local_bin="${local_bin_dir}/latte-dock-ng"
    local system_bin="/usr/bin/latte-dock-ng"

    if [[ ! -x "$system_bin" ]]; then
        return
    fi

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

detect_user_homes

run_as_root cmake --install .

# Gentoo/CMake setups may leave Plasma package directories without files.
# Sync full package trees explicitly so metadata.json and QML are always present.
sync_tree "${script_dir}/containment/package" "/usr/share/plasma/plasmoids/org.kde.latte.containment"
sync_tree "${script_dir}/plasmoid/package" "/usr/share/plasma/plasmoids/org.kde.latte.plasmoid"
sync_tree "${script_dir}/shell/package" "/usr/share/plasma/shells/org.kde.latte.shell"
sync_tree "${script_dir}/indicators" "/usr/share/latte/indicators"

# If user-level overrides already exist, keep them in sync with the installed
# system package trees. This avoids loading stale QML from ~/.local/share.
for user_home in "${user_homes[@]:-}"; do
    sync_tree_if_exists "${script_dir}/containment/package" "${user_home}/.local/share/plasma/plasmoids/org.kde.latte.containment"
    sync_tree_if_exists "${script_dir}/plasmoid/package" "${user_home}/.local/share/plasma/plasmoids/org.kde.latte.plasmoid"
    sync_tree_if_exists "${script_dir}/shell/package" "${user_home}/.local/share/plasma/shells/org.kde.latte.shell"
    sync_tree_if_exists "${script_dir}/indicators" "${user_home}/.local/share/latte/indicators"
    ensure_user_local_launcher "$user_home"
done

# Some Plasma 6 distros do not ship org.kde.plasma.private.taskmanager.
# Install a minimal compatibility module only when it is missing.
qt_qml_dir="/usr/lib64/qt6/qml"
if command -v qtpaths6 >/dev/null 2>&1; then
    qt_qml_dir="$(qtpaths6 --query QT_INSTALL_QML 2>/dev/null || true)"
elif command -v qtpaths >/dev/null 2>&1; then
    qt_qml_dir="$(qtpaths --query QT_INSTALL_QML 2>/dev/null || true)"
fi

if [[ -z "$qt_qml_dir" ]]; then
    qt_qml_dir="/usr/lib64/qt6/qml"
fi

fallback_taskmanager_dir="${qt_qml_dir}/org/kde/plasma/private/taskmanager"
if [[ ! -d "$fallback_taskmanager_dir" ]]; then
    sync_tree "${script_dir}/compat/qml/org/kde/plasma/private/taskmanager" "$fallback_taskmanager_dir"
    run_as_root touch "${fallback_taskmanager_dir}/.latte-fallback-module"
fi

cat <<'EOF'
Info: Installation finished.
Info: If your terminal session has a cached command path, refresh it before launching:
  - zsh:  rehash
  - bash: hash -r
EOF
