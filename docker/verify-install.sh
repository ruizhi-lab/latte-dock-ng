#!/usr/bin/env bash
# Cross-distro build/install/uninstall verification used by docker-compose.
set -euo pipefail

distro="${1:-unknown}"
jobs="${JOBS:-}"
export LANG="${LANG:-C.UTF-8}"
export LC_ALL="${LC_ALL:-C.UTF-8}"

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

verify_system_removed() {
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
rm -f /build/system/install_manifest.txt
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

echo "=== ${distro}: BUILD + INSTALL + UNINSTALL SUCCESS ==="
