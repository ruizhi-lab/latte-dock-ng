# M5b: Replace copied blocker tests and verify wiring

Status: Pending validation. The user reports all functional interaction checks
passed; the live Dock exit with its preview helper still active was not
reproduced.

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
prove emitted signal counts or live hide/show behavior. The user reports the
live blocker interactions passed; retain this fixture limitation for future
source-level coverage work.

## Validation

- `cmake --build build/modernization/gcc-debug --target visibilitylogictest sourcecontracttest --parallel 8` — passed with no C++ compiler warnings.
- `ctest --test-dir build/modernization/gcc-debug --output-on-failure -R '^(visibilitylogictest|sourcecontracttest)$'` — passed (2/2).
- `cmake --build build/modernization/clang-debug --target visibilitylogictest sourcecontracttest --parallel 8` — passed with no C++ compiler warnings.
- `ctest --test-dir build/modernization/clang-debug --output-on-failure -R '^(visibilitylogictest|sourcecontracttest)$'` — passed (2/2).
- `ctest --preset gcc-debug` — 45/46 passed. Existing `windowviewbackendtest` could not bind a D-Bus Unix socket in this sandbox (`Operation not permitted`).
- `ctest --preset clang-debug` — 45/46 passed, with the same sandbox-only D-Bus bind failure; the other 45 tests passed in both builds.
- `python3 autotests/coverageestimate.py` — 79/152 = 52.0%. This is the project's coarse file-level reference estimate, not line or branch coverage; the source contract does not count as production execution.
- The production helper's controlled mutation rejection and GCC ASan/UBSan focused test are recorded in [M5a](M5a.md). LeakSanitizer was unavailable locally due to the host ptrace restriction. Full M5a CI run 36642361102 passed the GCC/Clang matrix and sanitizer job.
- For each of `gcc-debug`, `gcc-release`, `clang-debug` and `clang-release`, `cmake --build build/modernization/<config> --target lattetasksplugin previewprocessunittest -j8` passed with no compiler warnings/errors, followed by `ctest --test-dir build/modernization/<config> --output-on-failure -R '^previewprocessunittest$'` passing (1/1 target). Direct `build/modernization/gcc-debug/bin/previewprocessunittest -v1` reported 9 passed, 0 failed, 0 skipped, including the no-QProcess-destruction-warning case.
- Fedora GUI retest confirms task status indicators and hover window previews work with the canonical user-mode install and Plasma session environment; Debian task indicators and the configured `PreviewWindows` mode are also confirmed. See [P0](P0.md) for the SSH session-environment diagnosis.
- Restarting Fedora while a preview helper was active exposed `QProcess: Destroyed while process ... is still running` during `PreviewProcess` destruction. The destructor now closes the helper input and reaps it synchronously with a bounded graceful/forced shutdown; ordinary failure recovery remains asynchronous.
- On 2026-09-30, the user reported that all remaining functional checks in the final matrix passed. Per-OS and per-hover-mode details were not supplied.
- The GCC Debug `previewprocessunittest` was rebuilt and run again: CTest passed (1/1), and direct QtTest reported 9 passed, 0 failed, 0 skipped, including `destructionReapsRunningHelperWithoutWarning` with a live fake helper.
- Fedora 44 user-mode Debug Dock: SIGTERM produced the expected shutdown path and `Latte Corona - deleted...` / `QuickWindowSystem destructed` markers, no new coredump, no fatal/error/warning markers, and no helper survivor. No helper was active at the signal.
- Debian 13.7 user-mode Debug Dock: the same clean result. A `latte-dock-ng-preview` PID was observed shortly before the test but exited before the baseline/signal; it was not active during the Dock teardown.
- Both VMs were restarted from their canonical user-mode Debug binaries; fresh idle startup logs had no Warning, Error, Fatal or ASSERT entries.

## Handoff

The user reports the functional blocker interactions and hover choices passed.
Keep this slice Pending validation only for a live Dock exit while its preview
helper is still running; the standalone production-object lifecycle test passes.
