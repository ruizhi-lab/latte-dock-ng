# M4a: Targeted clang-tidy checks

Date: 2026-09-30

Branch: `codex/modernization-m0-baseline`

Status: Pending validation until the next feature-branch workflow runs the
targeted clang-tidy step.

Starting revision: `d8c669e91`.

## Scope and implementation

Added a small `.clang-tidy` check set: `bugprone-use-after-move` and
`bugprone-unused-return-value`. The first production target is only
`app/data/errordata.cpp`, a small data-value implementation with a Clang Debug
compile command. The runner verifies that the selected compilation database
contains this file and comes from Clang; it does not analyze generated sources
or sweep the repository.

The runner also copies a disposable fixture that ignores the result of
`std::string::empty()`. `WarningsAsErrors: '*'` requires clang-tidy to reject
that issue. CI installs clang-tidy and runs the runner in the Clang Debug job;
GCC jobs keep their existing build-only behavior for this lint category.

## Validation

- Toolchain: clang-tidy / LLVM 22.1.8; Clang 22.1 Debug compilation database
  at `/tmp/latte-m2a-final/clang-debug/compile_commands.json`.
- `python3 scripts/test-clang-tidy.py --build-dir /tmp/latte-m2a-final/clang-debug`
  — passed. `errordata.cpp` had no selected diagnostics; the temporary fixture
  failed with `bugprone-unused-return-value` as required.
- Direct check selection for both configured checks on the production source —
  passed with no diagnostics.
- CI YAML parsed; the check runs only in the Clang Debug matrix job.
- `python3 -m py_compile scripts/test-clang-tidy.py` — passed.
- `git diff --check` — passed.
- Feature-branch CI for this new step — Pending validation.

No production code changed. `app/data/errordata.cpp` and its headers retain
their existing behavior and have no diagnostic requiring cleanup.

## Handoff

The current workflow run validates M2a and M3b. After it passes, push this
M4a slice and confirm the Clang Debug job installs clang-tidy, reads the
matching compilation database and rejects the fixture. If the selected checks
report new production findings, keep them as a separate scoped warning-family
slice rather than rewriting unrelated files.
