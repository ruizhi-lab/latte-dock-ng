#!/usr/bin/env bash
# NixOS/Nix build verification used by docker-compose and release CI.
set -euo pipefail

result_link="/build/latte-nix-result"

echo "=== NixOS: nix-build ==="
nix-build /src -o "${result_link}"

binary="${result_link}/bin/latte-dock-ng"
if [[ ! -x "${binary}" ]]; then
    echo "Missing expected binary at ${binary}" >&2
    exit 1
fi

echo "--- NixOS: nix-env install ---"
nix-env -if /src
if ! command -v latte-dock-ng >/dev/null; then
    echo "latte-dock-ng not on PATH after nix-env -i" >&2
    exit 1
fi

echo "--- NixOS: nix-env uninstall ---"
nix-env -e latte-dock-ng
if command -v latte-dock-ng >/dev/null; then
    echo "latte-dock-ng still on PATH after nix-env -e" >&2
    exit 1
fi

echo "=== NixOS: BUILD + INSTALL + UNINSTALL SUCCESS ==="
