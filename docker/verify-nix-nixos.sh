#!/usr/bin/env bash
# NixOS/Nix build verification used by docker-compose and release CI.
set -euo pipefail

result_link="/build/latte-nix-result"
nix_source="/tmp/latte-nix-source"

# Do not copy ignored in-tree build caches into the Nix source: Nix relocates
# the checkout into the store, where a copied CMakeCache.txt points at /src.
rm -rf "${nix_source}"
mkdir -p "${nix_source}"
tar --exclude-vcs --exclude='./build*' --exclude='./.cache' \
    -cf - -C /src . | tar -xf - -C "${nix_source}"

echo "=== NixOS: nix-build ==="
nix-build "${nix_source}" -o "${result_link}"

binary="${result_link}/bin/latte-dock-ng"
if [[ ! -x "${binary}" ]]; then
    echo "Missing expected binary at ${binary}" >&2
    exit 1
fi

echo "--- NixOS: nix-env install ---"
nix-env -if "${nix_source}"
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

cd "${nix_source}"

# Keep flake tests in the same updated NixOS image that validates the package,
# so the remote gate also covers the documented developer preset.
echo "=== NixOS: explicit flake check ==="
nix --extra-experimental-features "nix-command flakes" \
    flake check --print-build-logs

echo "=== NixOS: release flake package ==="
nix --extra-experimental-features "nix-command flakes" \
    build .#default --no-link --print-build-logs

echo "=== NixOS: development shell preset build ==="
nix --extra-experimental-features "nix-command flakes" \
    develop --command bash -c \
    'cmake --preset gcc-debug && cmake --build --preset gcc-debug'

echo "=== NixOS: FLAKE CHECK + PACKAGE + DEV SHELL SUCCESS ==="
