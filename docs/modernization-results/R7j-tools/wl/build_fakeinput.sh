#!/bin/sh
# Builds the pointer client of R7j (wl/fakeinput.c): generates the org_kde_kwin_fake_input
# bindings from the plasma-wayland-protocols XML and compiles the client against wayland-client.
# Usage: build_fakeinput.sh OUTPUT_DIR [PATH_TO_fake-input.xml]
set -eu
here=$(cd "$(dirname "$0")" && pwd)
out=${1:?usage: build_fakeinput.sh OUTPUT_DIR [fake-input.xml]}
xml=${2:-/usr/share/plasma-wayland-protocols/fake-input.xml}
mkdir -p "$out"
wayland-scanner client-header "$xml" "$out/fake-input-client-protocol.h"
wayland-scanner private-code "$xml" "$out/fake-input-protocol.c"
cc -std=gnu11 -O1 -Wall -Wextra -I"$out" -o "$out/fakeinput" "$here/fakeinput.c" "$out/fake-input-protocol.c" $(pkg-config --cflags --libs wayland-client)
echo "built $out/fakeinput"
