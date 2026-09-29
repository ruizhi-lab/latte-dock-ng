#!/usr/bin/env bash
# Type-resolution QML lint against a configured and built build tree.
# Raw output, execution manifests and normalized diagnostics stay under the
# build directory so a failed CI job can be compared without rerunning it.

set -uo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${1:-$root/build}"

qmllint=""
for candidate in qmllint-qt6 qmllint /usr/lib/qt6/bin/qmllint /usr/lib64/qt6/bin/qmllint; do
    if command -v "$candidate" >/dev/null 2>&1; then
        qmllint="$candidate"
        break
    fi
done
if [[ -z "$qmllint" ]]; then
    echo "error: qmllint not found (install qt6-declarative-dev / qt6-declarative)" >&2
    exit 1
fi

module_root="$build_dir/qml/org/kde/latte"
if [[ ! -d "$module_root/core" ]]; then
    echo "error: $module_root/core not found — configure and build QML modules first" >&2
    exit 1
fi

stage="$(mktemp -d)"
trap 'rm -rf "$stage"' EXIT
mkdir -p "$stage/org/kde/latte/compat"
ln -sfn "$root/declarativeimports/abilities" "$stage/org/kde/latte/abilities"
ln -sfn "$root/declarativeimports/components" "$stage/org/kde/latte/components"
ln -sfn "$root/compat/qml/org/kde/latte/compat/taskmanager" "$stage/org/kde/latte/compat/taskmanager"

mapfile -t files < <(git -C "$root" ls-files '*.qml')
if [[ ${#files[@]} -eq 0 ]]; then
    echo "qmllint-deep: no QML files found" >&2
    exit 1
fi

evidence="$build_dir/qmllint-baseline"
mkdir -p "$evidence/raw"
log="$build_dir/qmllint-deep.log"
: > "$log"

# Promote only categories with two comparable zero-warning runs and green CI.
PROMOTED_ERROR_CATEGORIES=(
    # The backlog has no category with two consecutive zero-warning runs yet.
)

echo "qmllint-deep: linting ${#files[@]} QML files with import resolution ($qmllint)"
echo "qmllint-deep: import roots: build QML modules, staged Latte modules, system QML"
version=$("$qmllint" --version 2>&1)
printf '%s\n' "$version" > "$evidence/qmllint-version.txt"
python3 - "$root" "$evidence/expected-files.json" "${files[@]}" <<'PY'
import json
import sys
from pathlib import Path

root = Path(sys.argv[1]).resolve()
files = sorted({Path(path).resolve().relative_to(root).as_posix() for path in sys.argv[3:]})
Path(sys.argv[2]).write_text(json.dumps(files, indent=2) + "\n", encoding="utf-8")
PY

run_args=(run --qmllint "$qmllint" --source-root "$root"
    --expected-files "$evidence/expected-files.json"
    --build-qml "$build_dir/qml" --staged-qml "$stage"
    --manifest "$evidence/manifest.json" --raw-dir "$evidence/raw"
    --tool-version "$version" --import-root "<build-qml>" --import-root "<staged-latte-qml>"
    --import-root "<system-qml>" --environment "host=$(. /etc/os-release && printf '%s-%s' "$ID" "$VERSION_ID")")
for category in "${PROMOTED_ERROR_CATEGORIES[@]}"; do
    run_args+=(--error-category "$category")
done
failed=0
python3 "$root/scripts/qmllint-baseline.py" "${run_args[@]}" --output "$evidence/current.json" >"$log" 2>&1 || failed=1

python3 "$root/scripts/qmllint-baseline.py" check-modules --build-qml "$build_dir/qml" || failed=1
if [[ -f "$evidence/current.json" ]]; then
    python3 "$root/scripts/qmllint-baseline.py" check-imports --report "$evidence/current.json" || failed=1
fi

if [[ -f "$evidence/current.json" ]]; then
    python3 - "$evidence/current.json" <<'PY'
import collections
import json
import sys
from pathlib import Path

report = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
counts = collections.Counter()
for diagnostic in report["diagnostics"]:
    counts[diagnostic["category"]] += diagnostic["count"]
print("qmllint-deep: warning counts (raw logs and report: " + str(Path(sys.argv[1]).parent) + "):")
for category, count in counts.most_common():
    print(f"  {count:5} {category}")
PY
fi

if [[ -f "$root/docs/qmllint-baseline.json" && -f "$evidence/current.json" ]]; then
    python3 "$root/scripts/qmllint-baseline.py" compare \
        --baseline "$root/docs/qmllint-baseline.json" --current "$evidence/current.json" \
        --output "$evidence/comparison.json" >"$evidence/comparison.log" 2>&1 || failed=1
fi

if [[ $failed -ne 0 ]]; then
    echo "qmllint-deep: FAILED (see $log and $evidence)" >&2
    exit 1
fi
echo "qmllint-deep: OK (evidence: $evidence)"
