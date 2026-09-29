# M5a: Extract only hiding-blocker policy

Status: Pending remote CI validation.

Recorded main baseline: `e3ef1ddf001aa032cffa62db21cfd97196174746`.
Actual start HEAD: `35a164d99` on `codex/modernization-m0-baseline`.
Implementation commit: `15b64b60a`.

## Scope

Extract the unique, nonempty event-name set from `VisibilityManager` into a
small production value helper and test that helper directly. `VisibilityManager`
remains the sole owner and authority for blocker state; it continues to emit
`hidingIsBlockedChanged` and handle hide timers, view visibility and QML calls.
Leave the copied logic in `visibilitylogictest` for its separate M5b removal.

## Invariants

- Duplicate and empty names do not change state.
- Removing an unknown name does not change state.
- Adding/removing an event reports true only when overall blocked state flips.
- Overlapping menu/drag/edit owners keep the view blocked until the last name
  is removed.

## Validation

- `cmake --build build/modernization/gcc-debug --target latte-dock-ng blockhidingeventstest --parallel 8` — passed.
- `ctest --test-dir build/modernization/gcc-debug --output-on-failure -R '^blockhidingeventstest$'` — passed (1/1).
- `cmake --build build/modernization/clang-debug --target latte-dock-ng blockhidingeventstest --parallel 8` — passed with no C++ compiler warnings. The build's existing `qmllint` phase reports unrelated baseline diagnostics.
- `ctest --test-dir build/modernization/clang-debug --output-on-failure -R '^blockhidingeventstest$'` — passed (1/1).
- `ctest --preset gcc-debug` — 45/46 passed, including the new helper test. Existing `windowviewbackendtest` could not bind a D-Bus Unix socket in the local sandbox (`Operation not permitted`); retry in CI.
- Controlled mutation changed `addEvent` to report a transition while the set remained blocked. The helper test failed four relevant assertions; after restoring production logic, the focused GCC test passed.
- `cmake --build build/modernization/gcc-asan-ubsan --target blockhidingeventstest --parallel 8` — passed. Normal CTest with LeakSanitizer failed after all eight QtTest cases passed because this host runs under ptrace. `ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/modernization/gcc-asan-ubsan --output-on-failure -R '^blockhidingeventstest$'` — passed (1/1), exercising ASan/UBSan while LeakSanitizer remains unverified locally.
- Docker sanitizer rerun is unavailable because access to `/run/user/1000/docker.sock` is denied. Remote CI run 36637219288 is still in progress; M5a sanitizer, full CTest and clean GCC/Clang workflow validation remain pending there.
- No desktop restart was performed. User retest is reserved until all planned implementation batches are complete.

## Handoff

After run 36637219288 completes, commit and push this slice so CI validates the
new helper under the full matrix and sanitizer preset. Then continue to M5b;
keep local D-Bus and LeakSanitizer limitations recorded if CI does not cover
them.
