#!/usr/bin/env bash
# NixOS/Nix build verification used by docker-compose and release CI.
set -euo pipefail

result_link="/build/latte-nix-result"
nix_source="/tmp/latte-nix-source"
nixpkgs_args=()
if [[ -n "${NIXPKGS_TARBALL_URL:-}" ]]; then
    # Match legacy nix-build/nix-env checks to flake.lock so both paths resolve
    # the same dependencies and can reuse the same Nix binary cache entries.
    nixpkgs_args=(-I "nixpkgs=${NIXPKGS_TARBALL_URL}")
fi

# Do not copy ignored in-tree build caches into the Nix source: Nix relocates
# the checkout into the store, where a copied CMakeCache.txt points at /src.
rm -rf "${nix_source}"
mkdir -p "${nix_source}"
tar --exclude-vcs --exclude='./build*' --exclude='./.cache' \
    -cf - -C /src . | tar -xf - -C "${nix_source}"

echo "=== NixOS: nix-build ==="
nix-build "${nixpkgs_args[@]}" "${nix_source}" -o "${result_link}"

binary="${result_link}/bin/latte-dock-ng"
if [[ ! -x "${binary}" ]]; then
    echo "Missing expected binary at ${binary}" >&2
    exit 1
fi

echo "--- NixOS: nix-env install ---"
nix-env "${nixpkgs_args[@]}" -if "${nix_source}"
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

# The host NixOS release-check job already runs flake check and builds .#default.
# Keep the container-specific legacy install path and the dev-shell compiler build.
echo "=== NixOS: development shell preset build ==="
nix --extra-experimental-features "nix-command flakes" \
    develop --impure --command bash -c \
    'args=(); if [[ -n "${LATTE_RELEASE_VERSION:-}" ]]; then args+=(-DVERSION="$LATTE_RELEASE_VERSION"); fi; cmake --preset gcc-debug "${args[@]}" && cmake --build --preset gcc-debug'

echo "=== NixOS: LEGACY INSTALL + DEV SHELL BUILD SUCCESS ==="
