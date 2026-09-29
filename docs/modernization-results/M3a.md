# M3a: Stable QML diagnostic identities

Date: 2026-09-30

Branch: `codex/modernization-m0-baseline`

Starting revision: `2e6c823436c43b4b5a906a5299dffbc7f2a93f40`

Scope: the standalone comparison tool, parser fixtures, optional CTest
registration and plan ledger. No QML source, warning policy, production
behavior, CI workflow or baseline data changed.

## Implementation

Added `scripts/qmllint-baseline.py` with JSON capture and human-output fallback
parsers, report validation and a comparison command. Diagnostic identities are
source-relative QML file, category, severity, whitespace-normalized message
and multiplicity; source line/column numbers and absolute paths are omitted.
Tool version, qmllint JSON revision/format, OS/architecture, supplied toolchain
attributes and import-root labels form the comparison fingerprint.

Capture requires an explicit complete manifest, non-empty expected QML file
inventory, one successful exit code per expected file, successful process exit
codes and exact coverage in both JSON and manifest. Unknown JSON revisions,
malformed/truncated records, tool/environment changes and partial coverage fail
closed. Comparison reports additions and removals separately and exits 1 when
new identities appear even if old identities disappear. It never updates a
reviewed baseline. The Python regression test is registered when Python 3.7+
is available; this keeps Python out of production and packaging dependencies.

## Validation

The fixture suite contains 15 cases covering duplicate multiplicity, changed
and removed identities, line/path normalization, meaningful message changes,
tool and environment fingerprint changes, execution/coverage errors,
truncated and malformed input, and valid zero-diagnostic runs. The CLI test
also verifies that a new diagnostic returns failure.

- `python3 -m py_compile scripts/qmllint-baseline.py autotests/test_qmllint_baseline.py`: passed.
- `python3 autotests/test_qmllint_baseline.py`: 15/15 passed.
- A live probe against `qmllint 6.11.2` confirmed `--json -` emits JSON
  revision 4. The human-format fallback is covered by the checked-in fixture.
- All four presets reconfigured and rebuilt. Full CTest passed **44/44** in
  GCC Debug, GCC Release, Clang Debug and Clang Release. Each configuration
  registered and executed `qmllintbaselinetest`.
- `git diff --check`: passed.

Full CTest logs are in the ignored local directory
`build-modernization-baseline/logs/{gcc-debug,gcc-release,clang-debug,clang-release}-m3a-final-ctest.log`.
The test suite uses the isolated D-Bus setup described in M0; these runs were
executed with host permission because the sandbox denies creation of its
private socket.

## Exact checks

```bash
python3 -m py_compile scripts/qmllint-baseline.py autotests/test_qmllint_baseline.py
python3 autotests/test_qmllint_baseline.py
for preset in gcc-debug gcc-release clang-debug clang-release; do
    cmake --preset "$preset"
    cmake --build --preset "$preset"
    ctest --preset "$preset"
done
```

The diagnostic format probe used:

```bash
qmllint --ignore-settings --max-warnings -1 --json - <probe.qml>
```

## Handoff

Next action: implement **M3b only**. Add a complete-file collector/manifest to
the deep-lint path, generate a reviewed baseline and additions/removals report
in one declared reproducible CI environment, create
`docs/qmllint-backlog-plan.md`, and gate the CI matrix against the reviewed
baseline. Keep syntax errors, failed generated-module imports and incomplete
coverage fatal; do not auto-accept a baseline change or promote a non-empty
category. The prior runs show equal counts across local presets but are not a
controlled CI baseline. M1a's CMake 3.20 execution and M1b's remote workflow
run remain Pending validation while M3b proceeds.
