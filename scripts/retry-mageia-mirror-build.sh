#!/usr/bin/env bash
set -euo pipefail

if (($# == 0)); then
    echo "Usage: retry-mageia-mirror-build.sh <build-command> [args...]" >&2
    exit 2
fi

# Cauldron is a rolling repository whose mirrors sync independently. Two transient failures are
# retried: metadata that advertises a package some mirrors do not have yet (a 404 or "All mirrors
# were tried"), and a mirror whose metadata is unusable while it updates ("Usable URL not found").
# Build run 40703989933 failed on the first and, after the retry, on the second, which this script
# then reported as an unrelated failure. Retry only these mirror failures; unrelated build failures
# must remain immediately visible.
max_attempts=3
log_file="$(mktemp)"
trap 'rm -f "$log_file"' EXIT

for ((attempt = 1; attempt <= max_attempts; attempt++)); do
    if "$@" 2>&1 | tee "$log_file"; then
        exit 0
    fi

    if ! grep -Eiq 'Status code: 404 for .*mageia/|Cannot download .*All mirrors were tried|Failed to download metadata .*mageia/.*Usable URL not found' "$log_file"; then
        exit 1
    fi

    if ((attempt < max_attempts)); then
        echo "::warning::Mageia Cauldron package mirrors are temporarily out of sync; retrying build ($attempt/$max_attempts)."
        sleep "$((attempt * 20))"
    fi
done

exit 1
