# M4a: Targeted clang-tidy checks

Date: 2026-09-30

Branch: `codex/modernization-m0-baseline`

Status: Complete.

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
- Feature-branch CI run `36633737377` at `46a4cd39b` — the Clang Debug
  `Run targeted clang-tidy checks` step passed.

Feature-branch run `36633737377` at `46a4cd39b` completed the Clang Debug
`Run targeted clang-tidy checks` step successfully. It used the matrix
compilation database and passed both production and negative-fixture checks.

No production code changed. `app/data/errordata.cpp` and its headers retain
their existing behavior and have no diagnostic requiring cleanup.

## Handoff

The targeted static-analysis gate is validated. Continue with M4b's separate
sanitizer configuration; if the selected checks report new production findings
in a future run, keep them as a separate scoped warning-family slice.
