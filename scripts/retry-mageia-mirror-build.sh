#!/usr/bin/env bash
set -euo pipefail

if (($# == 0)); then
    echo "Usage: retry-mageia-mirror-build.sh <build-command> [args...]" >&2
    exit 2
fi

# Cauldron metadata can briefly advertise packages before all upstream mirrors have synced them.
# Retry only package mirror 404s; unrelated build failures must remain immediately visible.
max_attempts=3
log_file="$(mktemp)"
trap 'rm -f "$log_file"' EXIT

for ((attempt = 1; attempt <= max_attempts; attempt++)); do
    if "$@" 2>&1 | tee "$log_file"; then
        exit 0
    fi

    if ! grep -Eiq 'Status code: 404 for .*mageia/|Cannot download .*All mirrors were tried' "$log_file"; then
        exit 1
    fi

    if ((attempt < max_attempts)); then
        echo "::warning::Mageia Cauldron package mirrors are temporarily out of sync; retrying build ($attempt/$max_attempts)."
        sleep "$((attempt * 20))"
    fi
done

exit 1
