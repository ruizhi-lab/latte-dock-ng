#!/usr/bin/env bash
# Cross-distro build/install/uninstall verification used by docker-compose.
set -euo pipefail

distro="${1:-unknown}"
jobs="${JOBS:-}"
default_locale="C"
if command -v locale >/dev/null 2>&1 && locale -a | grep -Eiq '^C[.]utf-?8$'; then
    default_locale="C.UTF-8"
fi
export LANG="${LANG:-${default_locale}}"
export LC_ALL="${LC_ALL:-${default_locale}}"

record_build_stack() {
    echo "=== ${distro}: build stack versions ==="
    cmake --version | head -n 1
    if command -v pkg-config >/dev/null 2>&1; then
        for module in Qt6Core KF6CoreAddons KF6Plasma Plasma; do
            if pkg-config --exists "$module"; then
                printf '%s %s\n' "$module" "$(pkg-config --modversion "$module")"
            fi
        done
    fi
    if command -v plasmashell >/dev/null 2>&1; then
        local plasma_version
        plasma_version="$( (ulimit -c 0; plasmashell --version) 2>/dev/null )" || plasma_version=""
        if [[ -n "$plasma_version" ]]; then
            printf '%s\n' "$plasma_version"
        else
            echo "Plasma runtime version unavailable in headless verification image"
        fi
    fi
}

verify_debian13_build_stack() {
    local package minimum version
    local requirements=(
        "cmake:3.31.6"
        "qt6-base-dev:6.8.2"
        "libkf6coreaddons-dev:6.13.0"
        "libplasma-dev:6.3.5"
        "plasma-workspace-dev:6.3.6"
    )

    echo "=== Debian 13 minimum build stack ==="
    for requirement in "${requirements[@]}"; do
        package="${requirement%%:*}"
        minimum="${requirement#*:}"
        version="$(dpkg-query --show --showformat='${Version}' "$package")"
        printf '  %s %s (minimum %s)\n' "$package" "$version" "$minimum"
        if ! dpkg --compare-versions "$version" ge "$minimum"; then
            echo "${package} ${version} is below the Debian 13 supported minimum ${minimum}" >&2
            return 1
        fi
    done
}

record_build_stack
if [[ "$distro" == "debian13" ]]; then
    verify_debian13_build_stack
fi

install_with_optional_jobs() {
    local mode="$1"
    local build_dir="$2"
    shift 2

    local args=("--${mode}" "--build-dir" "${build_dir}" "$@")
    if [[ -n "$jobs" ]]; then
        args+=("--jobs" "$jobs")
    fi

    bash /src/install.sh "${args[@]}"
}

path_exists() {
    local candidate
    for candidate in "$@"; do
        if [[ "$candidate" == *"*"* ]]; then
            compgen -G "$candidate" >/dev/null && return 0
        elif [[ -e "$candidate" ]]; then
            return 0
        fi
    done

    return 1
}

require_path() {
    local description="$1"
    shift

    if ! path_exists "$@"; then
        echo "Missing ${description}; checked:" >&2
        printf '  - %s\n' "$@" >&2
        exit 1
    fi
}

forbid_path() {
    local description="$1"
    shift

    if path_exists "$@"; then
        echo "Unexpected ${description}; checked:" >&2
        printf '  - %s\n' "$@" >&2
        exit 1
    fi
}

require_executable_file() {
    local description="$1"
    local path="$2"

    require_path "$description" "$path"

    if ! find "$path" -maxdepth 0 -perm -111 | grep -q .; then
        echo "${description} is not executable: ${path}" >&2
        exit 1
    fi
}

require_non_executable_file() {
    local description="$1"
    local path="$2"

    require_path "$description" "$path"

    if find "$path" -maxdepth 0 -perm -111 | grep -q .; then
        echo "${description} should not be executable: ${path}" >&2
        exit 1
    fi
}

system_qml_module_patterns() {
    printf '%s\n' \
        "/usr/lib/qt6/qml/org/kde/latte/core/qmldir" \
        "/usr/lib64/qt6/qml/org/kde/latte/core/qmldir" \
        "/usr/lib/x86_64-linux-gnu/qt6/qml/org/kde/latte/core/qmldir"
}

system_contextmenu_plugin_patterns() {
    printf '%s\n' \
        "/usr/lib/qt6/plugins/plasma/containmentactions/org.kde.latte.contextmenu.so" \
        "/usr/lib64/qt6/plugins/plasma/containmentactions/org.kde.latte.contextmenu.so" \
        "/usr/lib/x86_64-linux-gnu/qt6/plugins/plasma/containmentactions/org.kde.latte.contextmenu.so" \
        "/usr/lib/plugins/plasma/containmentactions/org.kde.latte.contextmenu.so" \
        "/usr/lib64/plugins/plasma/containmentactions/org.kde.latte.contextmenu.so" \
        "/usr/lib/x86_64-linux-gnu/plugins/plasma/containmentactions/org.kde.latte.contextmenu.so"
}

verify_system_installed() {
    local qml_patterns=()
    local plugin_patterns=()
    mapfile -t qml_patterns < <(system_qml_module_patterns)
    mapfile -t plugin_patterns < <(system_contextmenu_plugin_patterns)

    require_path "system binary" /usr/bin/latte-dock-ng
    require_path "system helper binary" /usr/bin/latte-dock-ng-add-launcher
    require_path "desktop file" /usr/share/applications/org.kde.latte-dock.desktop
    require_non_executable_file "system kicker action" /usr/share/plasma/kickeractions/org.kde.latte-dock.kickeractions.desktop
    require_path "containment package" /usr/share/plasma/plasmoids/org.kde.latte.containment
    require_path "shell package" /usr/share/plasma/shells/org.kde.latte.shell
    require_path "Latte core QML module" "${qml_patterns[@]}"
    require_path "Latte context menu plugin" "${plugin_patterns[@]}"
}

verify_extra_assets_removed() {
    local prefix="$1" path
    shopt -s nullglob
    for path in \
        "${prefix}"/lib*/plugins/kf6/packagestructure/latte_indicator.so \
        "${prefix}"/lib*/qt6/plugins/kf6/packagestructure/latte_indicator.so \
        "${prefix}"/lib/*/qt6/plugins/kf6/packagestructure/latte_indicator.so \
        "${prefix}"/lib*/plugins/plasma/containmentactions/org.kde.latte.contextmenu.so \
        "${prefix}"/lib*/qt6/plugins/plasma/containmentactions/org.kde.latte.contextmenu.so \
        "${prefix}"/lib/*/qt6/plugins/plasma/containmentactions/org.kde.latte.contextmenu.so \
        "${prefix}"/share/icons/hicolor/*/apps/latte-dock*.svg \
        "${prefix}"/share/icons/hicolor/*/apps/latte-dock*.png \
        "${prefix}"/share/icons/hicolor/scalable/apps/org.kde.latte.plasmoid.svg \
        "${prefix}"/share/locale/*/LC_MESSAGES/latte-dock.mo \
        "${prefix}"/share/locale/*/LC_MESSAGES/plasma*latte*.mo \
        "${prefix}"/share/locale/*/LC_MESSAGES/latte_indicator_*.mo; do
        forbid_path "remaining Latte install artifact" "$path"
    done
    shopt -u nullglob
}

verify_system_removed() {
    verify_extra_assets_removed /usr
    local qml_patterns=()
    local plugin_patterns=()
    mapfile -t qml_patterns < <(system_qml_module_patterns)
    mapfile -t plugin_patterns < <(system_contextmenu_plugin_patterns)

    forbid_path "system binary" /usr/bin/latte-dock-ng
    forbid_path "system helper binary" /usr/bin/latte-dock-ng-add-launcher
    forbid_path "desktop file" /usr/share/applications/org.kde.latte-dock.desktop
    forbid_path "system kicker action" /usr/share/plasma/kickeractions/org.kde.latte-dock.kickeractions.desktop
    forbid_path "containment package" /usr/share/plasma/plasmoids/org.kde.latte.containment
    forbid_path "shell package" /usr/share/plasma/shells/org.kde.latte.shell
    forbid_path "Latte core QML module" "${qml_patterns[@]}"
    forbid_path "Latte context menu plugin" "${plugin_patterns[@]}"
}

verify_user_installed() {
    local home_dir="$1"

    require_path "user binary" "${home_dir}/.local/bin/latte-dock-ng"
    require_path "user helper binary" "${home_dir}/.local/bin/latte-dock-ng-add-launcher"
    require_path "user desktop file" "${home_dir}/.local/share/applications/org.kde.latte-dock.desktop"
    require_executable_file "user kicker action" "${home_dir}/.local/share/plasma/kickeractions/org.kde.latte-dock.kickeractions.desktop"
    require_path "user containment package" "${home_dir}/.local/share/plasma/plasmoids/org.kde.latte.containment"
    require_path "user shell package" "${home_dir}/.local/share/plasma/shells/org.kde.latte.shell"
    require_path "user Latte core QML module" \
        "${home_dir}/.local/lib/qt6/qml/org/kde/latte/core/qmldir" \
        "${home_dir}/.local/lib64/qt6/qml/org/kde/latte/core/qmldir" \
        "${home_dir}/.local/lib/x86_64-linux-gnu/qt6/qml/org/kde/latte/core/qmldir"
}

verify_user_removed() {
    local home_dir="$1"
    verify_extra_assets_removed "${home_dir}/.local"
    forbid_path "user environment script" "${home_dir}/.config/latte-dock-ng/dev-env.sh"

    forbid_path "user binary" "${home_dir}/.local/bin/latte-dock-ng"
    forbid_path "user helper binary" "${home_dir}/.local/bin/latte-dock-ng-add-launcher"
    forbid_path "user desktop file" "${home_dir}/.local/share/applications/org.kde.latte-dock.desktop"
    forbid_path "user kicker action" "${home_dir}/.local/share/plasma/kickeractions/org.kde.latte-dock.kickeractions.desktop"
    forbid_path "user containment package" "${home_dir}/.local/share/plasma/plasmoids/org.kde.latte.containment"
    forbid_path "user shell package" "${home_dir}/.local/share/plasma/shells/org.kde.latte.shell"
    forbid_path "user Latte core QML module" \
        "${home_dir}/.local/lib/qt6/qml/org/kde/latte/core/qmldir" \
        "${home_dir}/.local/lib64/qt6/qml/org/kde/latte/core/qmldir" \
        "${home_dir}/.local/lib/x86_64-linux-gnu/qt6/qml/org/kde/latte/core/qmldir"
}

# Contract: install.sh --system --build-dir <path>
echo "=== ${distro}: system install via install.sh ==="
install_with_optional_jobs system /build/system
verify_system_installed

echo "--- ${distro}: manifest uninstall dry-run ---"
bash /src/uninstall.sh --system --no-purge-user-data --dry-run --manifest /build/system/install_manifest.txt
bash /src/uninstall.sh --system --no-purge-user-data --manifest /build/system/install_manifest.txt
verify_system_removed

echo "--- ${distro}: manifestless uninstall fallback ---"
cmake --install /build/system
verify_system_installed
rm -f /build/system/install_manifest.txt /usr/share/latte-dock-ng/install-manifest.txt
bash /src/uninstall.sh --system --no-purge-user-data --dry-run
bash /src/uninstall.sh --system --no-purge-user-data
verify_system_removed

# Contract: install.sh --user --build-dir <path>
echo "=== ${distro}: user install via install.sh ==="
export HOME=/tmp/latte-user
mkdir -p "$HOME"
install_with_optional_jobs user /build/user
verify_user_installed "$HOME"

echo "--- ${distro}: user uninstall ---"
bash /src/uninstall.sh --user --no-purge-user-data --dry-run --manifest /build/user/install_manifest.txt
bash /src/uninstall.sh --user --no-purge-user-data --manifest /build/user/install_manifest.txt
verify_user_removed "$HOME"

echo "--- ${distro}: user manifestless uninstall fallback ---"
cmake --install /build/user
verify_user_installed "$HOME"
rm -f /build/user/install_manifest.txt "$HOME/.local/share/latte-dock-ng/install-manifest.txt"
bash /src/uninstall.sh --user --no-purge-user-data
verify_user_removed "$HOME"

echo "=== ${distro}: BUILD + INSTALL + UNINSTALL SUCCESS ==="
