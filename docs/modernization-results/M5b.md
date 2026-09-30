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
- For each of `gcc-debug`, `gcc-release`, `clang-debug` and `clang-release`, `cmake --build build/modernization/<config> --target lattetasksplugin previewprocessunittest -j8` passed with no compiler warnings/errors, followed by `ctest --test-dir build/modernization/<config> --output-on-failure -R '^previewprocessunittest$'` passing (1/1 target). Direct `build/modernization/gcc-debug/bin/previewprocessunittest -v1` reported 9 passed, 0 failed, 0 skipped, including the no-QProcess-destruction-warning case.
- Fedora GUI retest confirms task status indicators and hover window previews work with the canonical user-mode install and Plasma session environment; Debian task indicators and the configured `PreviewWindows` mode are also confirmed. See [P0](P0.md) for the SSH session-environment diagnosis.
- Restarting Fedora while a preview helper was active exposed `QProcess: Destroyed while process ... is still running` during `PreviewProcess` destruction. The destructor now closes the helper input and reaps it synchronously with a bounded graceful/forced shutdown; ordinary failure recovery remains asynchronous. `destructionReapsRunningHelperWithoutWarning` passes in GCC/Clang Debug/Release, and both VMs still need a shutdown-log retest with the helper active.
- After that fix, `bash install.sh --user Debug` completed on Fedora 44 and Debian 13.7 / Plasma 6.3.6 from the branch source. Both user-mode Debug processes restarted with their actual Plasma session environments; fresh idle startup logs contain no Warning, Error, Fatal, ASSERT or missing-icon entries. The old process emitted the captured teardown warning before its log was cleared; the fixed-build active-helper exit remains a manual check.
- The full blocker interaction matrix has not yet been recorded. Menu popup, drag hover, shortcut popup, edit/configuration mode, overlapping blockers, and clean exit/log review remain for the Fedora GUI VM and the final user retest after all batches finish.
- The task-icon hover setting also needs three separate checks: window preview only, highlight only, and preview plus highlight. The user's confirmation that previews have returned does not prove the other choices or their combined behavior.

## Handoff

The implementation and local checks are recorded above. Record the remaining
blocker interactions and all three task-icon hover choices in the Fedora GUI VM
as runtime evidence becomes available. The final user retest must verify the
runtime scenarios above; keep this slice Pending validation until that feedback
is received.
