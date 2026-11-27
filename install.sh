#!/usr/bin/env bash
# Author: Ruizhi Zhong <ruizhi.zhong88@gmail.com>
# Summary: Installation script for Latte Dock NG
#
# Run as root / sudo  → system install to /usr        (needs write access)
# Run as normal user  → user install under ~/.local (no sudo needed)
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
build_jobs=""         # empty = auto-detect from available memory
build_dir_override="${LATTE_BUILD_DIR:-}"

# Auto-detect parallel jobs based on available memory (each job needs ~2GB)
detect_build_jobs() {
    local mem_kb=0
    if [[ -r /proc/meminfo ]]; then
        mem_kb=$(awk '/MemAvailable/ {print $2}' /proc/meminfo 2>/dev/null)
        [[ -z "$mem_kb" ]] && mem_kb=$(awk '/MemTotal/ {print $2}' /proc/meminfo 2>/dev/null)
    fi
    local mem_gb=$((mem_kb / 1048576))
    ((mem_gb < 1)) && mem_gb=1
    local cpus=$(nproc 2>/dev/null || echo 1)
    local max_jobs=$((mem_gb / 2))
    ((max_jobs < 1)) && max_jobs=1
    ((max_jobs > cpus)) && max_jobs=$cpus
    build_jobs="$max_jobs"
    echo "Info: auto-detected ${build_jobs} parallel job(s) (${mem_gb}GB mem, ${cpus} CPUs)"
}

# Validate a release override before pre-clean can remove an existing install.
if [[ -n "${LATTE_RELEASE_VERSION:-}" ]] &&
   [[ ! "$LATTE_RELEASE_VERSION" =~ ^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$ ]]; then
    echo "Error: LATTE_RELEASE_VERSION must be X.Y.Z." >&2
    exit 2
fi

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
  bash install.sh [OPTIONS] [BuildType]

Install mode (auto-detected from EUID when not specified):
  --user      Install to ~/.local (Debug uses ~/.local/opt/latte-dock-ng-dev)
  --system    Install to /usr      (requires root/sudo)

Build options:
  Debug | Release | RelWithDebInfo | MinSizeRel
  --enable-make-unique
  --translations | --translations-stable
  --clean | --no-clean      (default: --clean)
  --purge-user-data         Wipe user config/cache on clean
  --build-dir <path>        Build outside the source tree (or set LATTE_BUILD_DIR)
  --jobs N | -jN | --jobs=N Cap parallel compile jobs (default: auto-detect from memory).
                            Use a small value on memory-constrained hosts to
                            avoid OOM (each clang/g++ peaks at 1-2 GiB).

Examples:
  bash install.sh                 # auto: user if no root, system if root
  bash install.sh Debug           # same, Debug build
  bash install.sh --user Debug    # always user-local
  bash install.sh --build-dir /tmp/latte-build
  bash install.sh --user --jobs 2 # cap at 2 parallel compile jobs
  sudo bash install.sh            # system install
  sudo bash install.sh --system   # explicit system install
EOF
}

build_type_is_set="false"

validate_jobs() {
    [[ "$1" =~ ^[1-9][0-9]*$ ]] || { echo "Error: --jobs requires a positive integer, got '$1'." >&2; exit 2; }
}

while (($# > 0)); do
    arg="$1"
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
        --build-dir)
            shift
            (($# > 0)) || { echo "Error: --build-dir requires a path." >&2; exit 2; }
            build_dir_override="$1" ;;
        --build-dir=*)
            build_dir_override="${arg#--build-dir=}" ;;
        --jobs)
            shift
            (($# > 0)) || { echo "Error: --jobs requires a value." >&2; exit 2; }
            validate_jobs "$1"; build_jobs="$1" ;;
        --jobs=*)
            validate_jobs "${arg#--jobs=}"; build_jobs="${arg#--jobs=}" ;;
        -j*)
            validate_jobs "${arg#-j}"; build_jobs="${arg#-j}" ;;
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
    shift
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
    if [[ "$build_type" == "Debug" ]]; then
        install_prefix="${HOME}/.local/opt/latte-dock-ng-dev"
    fi
    build_dir="${script_dir}/build-user-${USER:-user}"
    if [[ "$build_type" == "Debug" ]]; then
        build_dir+="-debug"
    fi

    # Detect the Qt QML relative path (lib, lib64, or Debian multiarch) by
    # querying the system path and mirroring it under ~/.local.
    sys_qml_dir=""
    if command -v qtpaths6 >/dev/null 2>&1; then
        sys_qml_dir="$(qtpaths6 --query QT_INSTALL_QML 2>/dev/null || true)"
    fi
    if [[ -z "$sys_qml_dir" ]] && command -v qtpaths >/dev/null 2>&1; then
        sys_qml_dir="$(qtpaths --query QT_INSTALL_QML 2>/dev/null || true)"
    fi
    qml_relative_dir=""
    if [[ -n "$sys_qml_dir" ]]; then
        case "$sys_qml_dir" in
            /usr/local/*) qml_relative_dir="${sys_qml_dir#/usr/local/}" ;;
            /usr/*)       qml_relative_dir="${sys_qml_dir#/usr/}" ;;
        esac
    fi
    if [[ -z "$qml_relative_dir" ]]; then
        qml_relative_dir="lib/qt6/qml"
        [[ -d "/usr/lib64/qt6" ]] && qml_relative_dir="lib64/qt6/qml"
        [[ -d "/usr/lib/x86_64-linux-gnu/qt6" ]] && qml_relative_dir="lib/x86_64-linux-gnu/qt6/qml"
    fi
    kde_install_qmldir="${install_prefix}/${qml_relative_dir}"

    sudo_cmd=()   # Never need sudo for user-local install
else
    install_prefix="/usr"
    for package_db in /var/db/pkg/kde-misc/latte-dock-ng-*/CONTENTS; do
        if [[ -f "$package_db" ]]; then
            echo "Error: Portage manages the system Latte Dock NG installation." >&2
            exit 1
        fi
    done
    build_dir="${script_dir}/build"
    kde_install_qmldir=""   # Use cmake/KDE defaults for system install

    if [[ "${EUID}" -ne 0 ]]; then
        sudo_cmd=(sudo)
    else
        sudo_cmd=()
    fi

fi

if [[ -n "$build_dir_override" ]]; then
    build_dir="$build_dir_override"
    echo "Info: build directory = ${build_dir}"
elif [[ "$install_mode" == "system" && -d "$build_dir" && ! -w "$build_dir" ]]; then
    # If the default build dir isn't writable (built by another user), use a personal one
    build_dir="${script_dir}/build-${USER:-user}"
    echo "Info: '${script_dir}/build' is not writable, using '${build_dir}'."
fi

# ── Build ─────────────────────────────────────────────────────────────────────
mkdir -p "$build_dir"
build_dir="$(cd "$build_dir" && pwd)"
cd "$build_dir"
exec {build_lock}< "$build_dir"
flock --exclusive "$build_lock"

cmake_args=(
    -DCMAKE_INSTALL_PREFIX="${install_prefix}"
    # Installation builds must not require the optional autotest runtime
    # (dbus-run-session, a graphical session, etc.); CI runs autotests
    # separately in the dedicated GCC/Clang jobs.
    -DBUILD_TESTING=OFF
    -DENABLE_MAKE_UNIQUE="${enable_make_unique}"
    -DCMAKE_BUILD_TYPE="${build_type}"
)

# CI supplies the candidate version independently of the checkout's default.
# Forward it for native install verification as well as package builds.
if [[ -n "${LATTE_RELEASE_VERSION:-}" ]]; then
    cmake_args+=(-DVERSION="$LATTE_RELEASE_VERSION")
else
    # A previous candidate may have cached VERSION in a reused build tree.
    # Clear that override so ordinary installs return to the source default.
    cmake_args+=(-UVERSION)
fi

if [[ "$install_mode" == "user" ]]; then
    cmake_args+=(-DLATTE_INSTALL_USER_KICKERACTION_EXECUTABLE=ON)
else
    cmake_args+=(-DLATTE_INSTALL_USER_KICKERACTION_EXECUTABLE=OFF)
fi

if [[ -n "$kde_install_qmldir" ]]; then
    cmake_args+=(-DKDE_INSTALL_QMLDIR="${kde_install_qmldir}")
    if [[ "$build_type" == "Debug" ]]; then
        cmake_args+=(-DKDE_INSTALL_QTPLUGINDIR="${kde_install_qmldir%/qml}/plugins")
    fi
fi

if [[ "$l10n_auto_translations" == "ON" ]]; then
    cmake_args+=(-DKDE_L10N_AUTO_TRANSLATIONS=ON -DKDE_L10N_BRANCH="${l10n_branch}")
else
    cmake_args+=(-DKDE_L10N_AUTO_TRANSLATIONS=OFF)
fi

cmake "${cmake_args[@]}" "${script_dir}"

[[ "$l10n_auto_translations" == "ON" ]] && cmake --build . --target fetch-translations

if [[ -z "$build_jobs" ]]; then
    detect_build_jobs
fi

if [[ -n "$build_jobs" ]]; then
    echo "Info: capping parallel compile jobs at ${build_jobs}"
    cmake --build . --parallel "${build_jobs}" --clean-first
else
    cmake --build . --parallel --clean-first
fi

# ── Helper functions ──────────────────────────────────────────────────────────
run_as_root() {
    "${sudo_cmd[@]}" "$@"
}

run_as_user() {
    local target_user="$1"
    shift

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

    if [[ -n "${SUDO_USER:-}" && "${SUDO_USER}" != "root" ]]; then
        local sudo_home=""

        if command -v getent >/dev/null 2>&1; then
            sudo_home="$(getent passwd "${SUDO_USER}" | cut -d: -f6 2>/dev/null || true)"
        fi

        [[ "$sudo_home" == "$user_home" ]] && echo "$SUDO_USER" && return
    fi

    stat -c '%U' "$user_home" 2>/dev/null || basename "$user_home"
}

refresh_service_caches() {
    if command -v update-desktop-database >/dev/null 2>&1; then
        run_as_root update-desktop-database "${install_prefix}/share/applications" >/dev/null 2>&1 || true
    fi

    local sycoca_tool=""

    if command -v kbuildsycoca6 >/dev/null 2>&1; then
        sycoca_tool="kbuildsycoca6"
    elif command -v kbuildsycoca5 >/dev/null 2>&1; then
        sycoca_tool="kbuildsycoca5"
    elif command -v kbuildsycoca >/dev/null 2>&1; then
        sycoca_tool="kbuildsycoca"
    fi

    [[ -n "$sycoca_tool" ]] || return 0

    if [[ "$install_mode" == "system" ]]; then
        local user_home

        for user_home in "${user_homes[@]:-}"; do
            [[ -d "$user_home" ]] || continue
            run_as_user "$(resolve_username "$user_home")" "$sycoca_tool" --noincremental
        done
    else
        "$sycoca_tool" --noincremental >/dev/null 2>&1 || true
    fi
}

# ── Install ───────────────────────────────────────────────────────────────────
detect_user_homes

publish_shared_install() {
    run_as_root bash -s -- "$staged_prefix" "$install_prefix" "$stage_dir" "$preclean_install" <<'PUBLISH'
set -euo pipefail
staged_prefix="$1"
prefix="$2"
work_dir="$3"
clean="$4"
manifest="share/latte-dock-ng/install-manifest.txt"
mkdir -p "$prefix" "$work_dir/previous-files"

list_files() {
    local list="$1"
    local path parent
    [[ -f "$list" ]] || return 0
    while IFS= read -r path || [[ -n "$path" ]]; do
        [[ "$path" == "$prefix"/* && "${path##*/}" != "." && "${path##*/}" != ".." ]] || continue
        parent="$(realpath -m -- "${path%/*}")"
        case "$parent" in
            "$prefix"|"$prefix"/*) printf '%s\n' "${path#"$prefix"/}" ;;
            *) echo "Error: install path escapes the prefix: $path" >&2; return 1 ;;
        esac
    done < "$list" | LC_ALL=C sort -u
}

list_files "$staged_prefix/$manifest" > "$work_dir/new-list"
list_files "$prefix/$manifest" > "$work_dir/old-list"
if [[ "$clean" == "true" ]]; then
    LC_ALL=C comm -23 "$work_dir/old-list" "$work_dir/new-list" > "$work_dir/obsolete-list"
else
    : > "$work_dir/obsolete-list"
fi

# Snapshot every affected file before publishing any part of the new build.
LC_ALL=C sort -u "$work_dir/new-list" "$work_dir/obsolete-list" > "$work_dir/affected-list"
cd "$prefix"
while IFS= read -r path; do
    if [[ -e "$path" || -L "$path" ]]; then
        if [[ ! -f "$path" && ! -L "$path" ]]; then
            echo "Error: install target is not a file: $prefix/$path" >&2
            exit 1
        fi
        cp -a --parents -- "$path" "$work_dir/previous-files"
    fi
done < "$work_dir/affected-list"

rollback() {
    local status=$?
    local restore_failed=false
    trap - EXIT
    if ((status != 0)); then
        echo "Info: restoring the previous installation." >&2
        while IFS= read -r path; do
            if [[ -e "$work_dir/previous-files/$path" || -L "$work_dir/previous-files/$path" ]]; then
                cp -a --remove-destination -- "$work_dir/previous-files/$path" "$path" || restore_failed=true
            else
                rm -f -- "$path" || restore_failed=true
            fi
        done < "$work_dir/affected-list"
        if [[ "$restore_failed" == true ]]; then
            touch "$work_dir/retain-backup"
            echo "Error: restore failed. Backup files remain in $work_dir." >&2
        fi
    fi
    exit "$status"
}

trap rollback EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
while IFS= read -r path; do
    if [[ ! -e "$staged_prefix/$path" && ! -L "$staged_prefix/$path" ]]; then
        continue
    fi
    if [[ "$path" == */* && ! -d "${path%/*}" ]]; then
        mkdir -p -- "${path%/*}"
    fi
    cp -a --remove-destination --no-preserve=ownership -- "$staged_prefix/$path" "$path"
done < "$work_dir/new-list"
while IFS= read -r path; do
    rm -f -- "$path"
done < "$work_dir/obsolete-list"
PUBLISH
}

# Complete the install into a staging tree before touching the active prefix.
# Keep it on the same filesystem for the isolated Debug directory swap.
stage_parent="${TMPDIR:-/tmp}"
if [[ "$install_mode" == "user" && "$build_type" == "Debug" ]]; then
    stage_parent="${install_prefix%/*}"
    mkdir -p "$stage_parent"
fi
stage_dir="$(mktemp -d "${stage_parent}/latte-install.XXXXXXXX")"
trap '[[ -f "$stage_dir/retain-backup" ]] || rm -rf -- "$stage_dir" 2>/dev/null || run_as_root rm -rf -- "$stage_dir"' EXIT
DESTDIR="$stage_dir" cmake --install .
staged_prefix="${stage_dir}${install_prefix}"
for artifact in \
        bin/latte-dock-ng \
        share/applications/org.kde.latte-dock.desktop \
        share/plasma/plasmoids/org.kde.latte.plasmoid/metadata.json \
        share/plasma/plasmoids/org.kde.latte.containment/metadata.json \
        share/plasma/shells/org.kde.latte.shell/metadata.json \
        share/latte/indicators/default/metadata.json; do
    if [[ ! -s "${staged_prefix}/${artifact}" ]]; then
        echo "Error: staged install is missing ${artifact}." >&2
        exit 1
    fi
done
if [[ -n "$kde_install_qmldir" && ! -s "${stage_dir}${kde_install_qmldir}/org/kde/latte/core/qmldir" ]]; then
    echo "Error: staged install is missing the Latte core QML module." >&2
    exit 1
fi
mkdir -p "${staged_prefix}/share/latte-dock-ng"

# The installed manifest includes only this staged payload. Use CMake's
# installation rules for package resources so ebuild and script installs agree.
staged_manifest="${staged_prefix}/share/latte-dock-ng/install-manifest.txt"
(
    cd "$staged_prefix"
    find . \( -type f -o -type l \) -print | while IFS= read -r entry; do
        printf '%s/%s\n' "$install_prefix" "${entry#./}"
    done
) | LC_ALL=C sort -u > "$staged_manifest"

# Serialize publication with other installs and uninstalls of this prefix.
lock_dir="$install_prefix"
if [[ "$install_mode" == "user" ]]; then
    lock_dir="${HOME}/.local"
    mkdir -p "$lock_dir"
fi
exec {install_lock}< "$lock_dir"
flock --exclusive "$install_lock"

if [[ "$preclean_install" == "true" ]]; then
    cleanup_cmd=(bash "${script_dir}/uninstall.sh" "--${install_mode}" --keep-install)
    if [[ "$purge_user_data" == "true" ]]; then
        cleanup_cmd+=(--purge-user-data)
    fi
    "${cleanup_cmd[@]}"
elif [[ "$purge_user_data" == "true" ]]; then
    echo "Warning: --purge-user-data is ignored when --no-clean is set."
fi

if [[ "$install_mode" == "user" && "$build_type" == "Debug" ]]; then
    # Replacing a dedicated prefix keeps every binary and package on one build.
    # Keep the previous tree outside the staging trap until promotion succeeds.
    backup="$(mktemp -d "${stage_parent}/latte-previous.XXXXXXXX")"
    if [[ -e "$install_prefix" || -L "$install_prefix" ]]; then
        mv -T -- "$install_prefix" "$backup/install"
    fi
    if ! mv -T -- "$staged_prefix" "$install_prefix"; then
        if [[ -e "$backup/install" || -L "$backup/install" ]]; then
            mv -T -- "$backup/install" "$install_prefix"
        fi
        rmdir -- "$backup"
        exit 1
    fi
    rm -rf -- "$backup"
else
    previous_manifest="${install_prefix}/share/latte-dock-ng/install-manifest.txt"
    if [[ "$preclean_install" != "true" && -f "$previous_manifest" ]]; then
        LC_ALL=C sort -u "$staged_manifest" "$previous_manifest" > "$stage_dir/overlay-manifest"
        mv -- "$stage_dir/overlay-manifest" "$staged_manifest"
    fi
    publish_shared_install
    refresh_service_caches
fi

# ── Save install metadata ─────────────────────────────────────────────────────
printf '%s\n' "$install_mode"   > "${build_dir}/.install-mode"
printf '%s\n' "$install_prefix" > "${build_dir}/.install-prefix"
[[ -n "$kde_install_qmldir" ]] && printf '%s\n' "$kde_install_qmldir" > "${build_dir}/.install-qmldir"

# ── Post-install instructions ─────────────────────────────────────────────────
if [[ "$install_mode" == "user" && "$build_type" == "Debug" ]]; then
    env_file="${HOME}/.config/latte-dock-ng/dev-env.sh"
    mkdir -p "${HOME}/.config/latte-dock-ng"
    cat > "$env_file" <<ENVEOF
# latte-dock-ng user-local install environment
# Generated by install.sh — source this before running latte-dock-ng
export XDG_DATA_DIRS="${install_prefix}/share:\${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"
export QT_PLUGIN_PATH="${kde_install_qmldir%/qml}/plugins\${QT_PLUGIN_PATH:+:\${QT_PLUGIN_PATH}}"
export LATTE_DISABLE_USER_LOCAL_QML_IMPORTS=1
# KSycoca hashes the data search path. Debug's extra directory would otherwise
# invalidate the cache used by the desktop session on each alternating launch.
latte_debug_cache="\${XDG_CACHE_HOME:-\${HOME}/.cache}/latte-dock-ng-dev"
mkdir -p -- "\$latte_debug_cache" || return
export KDESYCOCA="\$latte_debug_cache/ksycoca"
unset latte_debug_cache
export QML2_IMPORT_PATH="${kde_install_qmldir}\${QML2_IMPORT_PATH:+:\${QML2_IMPORT_PATH}}"
export QML_IMPORT_PATH="${kde_install_qmldir}\${QML_IMPORT_PATH:+:\${QML_IMPORT_PATH}}"
export QT_QML_IMPORT_PATH="${kde_install_qmldir}\${QT_QML_IMPORT_PATH:+:\${QT_QML_IMPORT_PATH}}"
# Plasma stores the session's selected look-and-feel package in the KDE
# defaults overlay. A set XDG_CONFIG_DIRS keeps its search path behind the
# overlay, as the XDG desktop-session launch does. An unset or empty value (the
# XDG Base Directory specification treats both alike) must not become
# "defaults:/etc/xdg" alone: a dock started from a bare SSH shell with that value
# made the Dock, plasmashell and KWin rewrite the shared KSycoca database against
# each other. The session's value is read from the user manager instead, used
# exactly as reported, and the variable stays unset when the manager reports
# none. autotests/installscriptbehavior.py renders this block and checks the cases.
if [[ -n "\${XDG_CONFIG_DIRS:-}" ]]; then
    export XDG_CONFIG_DIRS="\${HOME}/.config/kdedefaults:/etc/xdg:\${XDG_CONFIG_DIRS}"
else
    latte_session_config_dirs="\$(systemctl --user show-environment 2>/dev/null | sed -n 's/^XDG_CONFIG_DIRS=//p' | head -n 1)"
    if [[ -n "\${latte_session_config_dirs}" ]]; then
        export XDG_CONFIG_DIRS="\${latte_session_config_dirs}"
    else
        unset XDG_CONFIG_DIRS
    fi
    unset latte_session_config_dirs
fi
# Match the KDE session language for terminal/nohup debug launches.  A stale
# LC_ALL from the command runner otherwise overrides plasma-localerc.
if [[ -f "\${HOME}/.config/plasma-localerc" ]]; then
    latte_debug_lang="\$(sed -n 's/^LANG=//p' "\${HOME}/.config/plasma-localerc" | head -n 1)"
    latte_debug_language="\$(sed -n 's/^LANGUAGE=//p' "\${HOME}/.config/plasma-localerc" | head -n 1)"
    [[ -n "\${latte_debug_lang}" ]] && export LANG="\${latte_debug_lang}"
    [[ -n "\${latte_debug_language}" ]] && export LANGUAGE="\${latte_debug_language}"
    [[ -n "\${latte_debug_lang}" ]] && export LC_MESSAGES="\${latte_debug_lang}"
    unset LC_ALL
fi
ENVEOF

    # KWin uses the real executable in Exec to authorize Wayland protocols.
    # Register only desktop metadata, without exporting the Debug data tree.
    applications_dir="${HOME}/.local/share/applications"
    mkdir -p "$applications_dir" "${HOME}/.local/bin"
    awk '
        /^\[Desktop Entry\]$/ { print; print "NoDisplay=true"; next }
        !/^NoDisplay=/
    ' "${install_prefix}/share/applications/org.kde.latte-dock.desktop" \
        > "${applications_dir}/org.kde.latte-dock.dev.desktop"
    if [[ -f "${install_prefix}/share/applications/org.kde.latte-dock.preview.desktop" ]]; then
        cp -- "${install_prefix}/share/applications/org.kde.latte-dock.preview.desktop" \
            "${applications_dir}/org.kde.latte-dock.preview.dev.desktop"
    fi
    cat > "${HOME}/.local/bin/latte-dock-ng-dev" <<'LAUNCHER'
#!/usr/bin/env bash
set -e
source "${HOME}/.config/latte-dock-ng/dev-env.sh"
exec "${HOME}/.local/opt/latte-dock-ng-dev/bin/latte-dock-ng" "$@"
LAUNCHER
    chmod +x "${HOME}/.local/bin/latte-dock-ng-dev"
    refresh_service_caches
    printf 'Info: Run the isolated build with %s/.local/bin/latte-dock-ng-dev --replace\n' "$HOME"
elif [[ "$install_mode" == "user" ]]; then
    printf 'Info: Run the user build with %s/bin/latte-dock-ng --replace\n' "$install_prefix"
else
    echo "Info: System installation complete."
fi
