#!/usr/bin/env bash
# Author: Michail Vourlakos, Smith Ar
# Summary: Installation script for Latte Dock Panel

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

build_type="Release"
enable_make_unique="OFF"
l10n_auto_translations="OFF"
l10n_branch=""

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

mkdir -p "${script_dir}/build"
cd "${script_dir}/build"

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

cmake --build . --parallel

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

run_as_root cmake --install .

# Gentoo/CMake setups may leave Plasma package directories without files.
# Sync full package trees explicitly so metadata.json and QML are always present.
sync_tree "${script_dir}/containment/package" "/usr/share/plasma/plasmoids/org.kde.latte.containment"
sync_tree "${script_dir}/plasmoid/package" "/usr/share/plasma/plasmoids/org.kde.latte.plasmoid"
sync_tree "${script_dir}/shell/package" "/usr/share/plasma/shells/org.kde.latte.shell"
sync_tree "${script_dir}/indicators" "/usr/share/latte/indicators"
