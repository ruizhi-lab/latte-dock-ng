# M5b: Replace copied blocker tests and verify wiring

Status: Pending validation.

Recorded main baseline: `e3ef1ddf001aa032cffa62db21cfd97196174746`.
Actual start HEAD: `c6ea29dd9` on `codex/modernization-m0-baseline`.
Implementation commit: `e4502e67b` (included in the pushed modernization branch).

## Scope

Remove only the duplicate `BlockHidingEvents` value type and its blocker cases
from `visibilitylogictest`. Keep all unrelated visibility geometry and mode
tests. Retain direct production-helper tests from M5a and add a narrow source
contract for the public wrapper and its state-change handler.

`VisibilityManager` requires a real `PlasmaQuick::ContainmentView` and the
application's window-system/compositor services. No viable lightweight
production host fixture was found, so the source contract does not claim to
prove emitted signal counts or live hide/show behavior. Those cases remain in
the final desktop retest.

## Validation

- `cmake --build build/modernization/gcc-debug --target visibilitylogictest sourcecontracttest --parallel 8` — passed with no C++ compiler warnings.
- `ctest --test-dir build/modernization/gcc-debug --output-on-failure -R '^(visibilitylogictest|sourcecontracttest)$'` — passed (2/2).
- `cmake --build build/modernization/clang-debug --target visibilitylogictest sourcecontracttest --parallel 8` — passed with no C++ compiler warnings.
- `ctest --test-dir build/modernization/clang-debug --output-on-failure -R '^(visibilitylogictest|sourcecontracttest)$'` — passed (2/2).
- `ctest --preset gcc-debug` — 45/46 passed. Existing `windowviewbackendtest` could not bind a D-Bus Unix socket in this sandbox (`Operation not permitted`).
- `ctest --preset clang-debug` — 45/46 passed, with the same sandbox-only D-Bus bind failure; the other 45 tests passed in both builds.
- `python3 autotests/coverageestimate.py` — 79/152 = 52.0%. This is the project's coarse file-level reference estimate, not line or branch coverage; the source contract does not count as production execution.
- The production helper's controlled mutation rejection and GCC ASan/UBSan focused test are recorded in [M5a](M5a.md). LeakSanitizer was unavailable locally due to the host ptrace restriction. Full M5a CI run 36642361102 passed the GCC/Clang matrix and sanitizer job.
- Fedora GUI retest now confirms task status indicators and hover window previews work with the canonical user-mode install and Plasma session environment; Debian task indicators are also restored. See [P0](P0.md) for the SSH session-environment diagnosis.
- The full blocker interaction matrix has not yet been recorded. Menu popup, drag hover, shortcut popup, edit/configuration mode, overlapping blockers, and clean exit/log review remain for the Fedora GUI VM and the final user retest after all batches finish.

## Handoff

The implementation and local checks are recorded above. Record the remaining
blocker interactions in the Fedora GUI VM as runtime evidence becomes available.
The final user retest must verify the runtime scenarios above; keep this slice
Pending validation until that feedback is received.
