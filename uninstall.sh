#!/usr/bin/env bash
# Author: Michail Vourlakos
# Summary: Uninstallation script for Latte Dock Panel

set -euo pipefail

manifest_path="build/install_manifest.txt"
dry_run="false"

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

if [[ ! -f "$manifest_path" ]]; then
    echo "Uninstallation manifest does not exist: $manifest_path"
    exit 1
fi

echo "Using uninstall manifest: $manifest_path"

if [[ "$dry_run" == "true" ]]; then
    echo "Dry run mode. Files that would be removed:"
    cat "$manifest_path"
    exit 0
fi

if [[ "${EUID}" -eq 0 ]]; then
    sudo_cmd=()
else
    sudo_cmd=(sudo)
fi

while IFS= read -r file; do
    [[ -z "$file" ]] && continue
    "${sudo_cmd[@]}" rm -f -- "$file"
done < "$manifest_path"
