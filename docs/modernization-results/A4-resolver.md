# A4 Mapping Resolver Slice

Status: bounded production resolver implemented and locally accepted, including
the user's desktop smoke. A4 remains Pending validation for full-chain cases.
Date: 2026-11-13.

`GenericLayout::screenForContainment()` now adapts its authoritative pending
table and live/waiting view maps to `ContainmentScreenResolver`. The helper
owns only precedence: a pending screen wins, `onPrimary` resolves through the
actual primary ID only when Corona exists, a present view mapping is returned
even when its ID is zero or the negative unknown/disconnected sentinel, and
persisted `lastScreen()` is consulted only when no pending or live mapping
answers. A higher caller may apply a fallback to the negative sentinel. Lazy
callbacks keep reads of primary state, view maps, connector identity and
fallback behind the matching decision. The existing live-map-before-waiting-
map rule, including a null live entry, remains in the GenericLayout adapter.

`containmentscreenresolvertest` calls the production helper and checks pending
precedence, nonzero primary IDs, missing-Corona behavior, negative sentinel,
zero and positive IDs, missing mappings, and callback laziness. A source-
contract check guards the GenericLayout adapter's null guard, live/waiting
order, connector lookup, pending fields and primary callback, and fallback
wiring. These checks do not execute GenericLayout, the Synchronizer, Corona's
nested-containment traversal, or Plasma runtime state; they are adapter and
policy guards, not full-chain integration evidence.

Focused verification passed on GCC Debug and Clang Debug: both targets built
and both CTest cases passed. Mutation checks also made the tests fail as
expected: bypassing pending precedence returned the live value instead of the
pending screen; replacing the primary callback with a zero-producing expression
failed the nonzero-primary assertion; treating absent/zero-or-negative view
IDs as unavailable made the negative and zero rows fall through to `lastScreen`.
Two adapter mutations were independently caught by the source-contract test:
dropping `update.onPrimary` and replacing the primary callback with a constant.
The header and adapter were restored before the final passing GCC test run.
Pending validation still includes primary-not-zero and nested containment
observations through the full production chain. The coordinator's desktop
acceptance is recorded below; existing R7 hotplug observations remain separate evidence. No
app-sized duplicate target or runtime ownership/test hook was introduced.

## Focused validation record

Baseline repository `HEAD`: `7c6c88845e6db3431152b2cf5019a23e3c39398b`.
The implementation was uncommitted. Focused build output was emitted directly
to the terminal and was not captured in log files; therefore there are no
persistent focused-build log paths. Exit statuses below are from the completed
commands. The coordinator's subsequent four-preset validation is recorded below.

GCC Debug commands, in order:

```sh
cmake --preset gcc-debug                                      # exit 0
cmake --build build/modernization/gcc-debug --target containmentscreenresolvertest sourcecontracttest -j8 # exit 0
ctest --test-dir build/modernization/gcc-debug --output-on-failure -R '^(containmentscreenresolvertest|sourcecontracttest)$' # exit 0, 2/2 passed
```

Clang Debug commands, in order:

```sh
cmake --preset clang-debug                                      # exit 0
cmake --build build/modernization/clang-debug --target containmentscreenresolvertest sourcecontracttest -j8 # exit 0
ctest --test-dir build/modernization/clang-debug --output-on-failure -R '^(containmentscreenresolvertest|sourcecontracttest)$' # exit 0, 2/2 passed
```

Mutation checks used the GCC Debug `containmentscreenresolvertest` binary and
ran one mutation at a time, rebuilding the target before each test:

| Temporary mutation | Focused command/result |
| --- | --- |
| Change `if (pending)` to `if (pending && pending->onPrimary && hasCorona)` | `cmake --build build/modernization/gcc-debug --target containmentscreenresolvertest -j8` exited 0; `ctest --test-dir build/modernization/gcc-debug --output-on-failure -R '^containmentscreenresolvertest$'` exited 8. Tests failed with actual live result `8` vs pending `-7`, and exposed the no-Corona pending case. |
| Change the primary result expression to `primaryScreenId() - primaryScreenId()` | Build exited 0; focused CTest exited 8. `onPrimaryUsesActualPrimaryIdOnlyWhenCoronaExists` failed with actual `0` vs expected primary ID `6`. |
| Change the live result guard to `if (const auto screenId = viewScreenId(); screenId && *screenId > 0)` | Build exited 0; focused CTest exited 8. The negative sentinel and zero ID rows both fell through to `lastScreen` result `10`. |
| Change `PendingContainmentScreen{ update.onPrimary, update.screen }` to `PendingContainmentScreen{ false, update.screen }` | `build/modernization/gcc-debug/bin/sourcecontracttest genericLayoutScreenMappingUsesLazyAuthoritativeAdapters` exited 1 on the missing pending-field contract. |
| Change the primary callback body from `primaryScreenId()` to `0` | The same source-contract command exited 1 on the missing `primaryScreenId()` adapter contract. |

An initial primary mutation written as `? 0 : ...` was rejected at compile time
by `-Werror=unused-parameter`; it is excluded as mutation evidence. The
compiling zero-producing expression above supplied the test-failure evidence.
After all mutations were restored, GCC focused CTest was rerun:
`ctest --test-dir build/modernization/gcc-debug --output-on-failure -R
'^(containmentscreenresolvertest|sourcecontracttest)$'` exited 0 (2/2 passed).

Worker-restored source SHA-256 values, before the coordinator's capture fix:

| File | SHA-256 |
| --- | --- |
| `app/layout/containmentscreenresolver.h` | `d62685e56fc02a17ed5bde53ef6b0c1ffa8ecfd0afcee0e8c5cc0171d35fdbb4` |
| `app/layout/genericlayout.cpp` | `7b273e0800096b2eb35755cfa95f6d72d1b66ce8612f8d13ca21372f527efaef` |

## Coordinator verification and desktop acceptance

The first sandbox full-suite attempt could not bind private D-Bus sockets;
`qmlsmoketest` and `importedcontainmentsunittest` timed out and two bus tests
failed. Those are not accepted passes. The coordinator reran the suite outside
that IPC restriction in a private `dbus-run-session`, with temporary XDG
configuration/data/cache/runtime directories and `QT_QPA_PLATFORM=offscreen`.
The offscreen setting also prevents the private ActivityManager service from
attempting an unavailable display. No test result was waived or target removed.

The first full Clang application build caught an unused `this` capture on the
fallback callback. The coordinator removed that capture, without a warning
suppression, before the final matrix:

| Preset | Configure/build/CTest exit | Compiler warnings | CTest |
| --- | --- | ---: | --- |
| gcc-debug | 0 / 0 / 0 | 0 | 64/64 |
| clang-debug | 0 / 0 / 0 | 0 | 64/64 |
| gcc-release | 0 / 0 / 0 | 0 | 64/64 |
| clang-release | 0 / 0 / 0 | 0 | 64/64 |

Commands per preset: `cmake --preset <preset>`,
`cmake --build --preset <preset> --parallel 8`, `ctest --preset <preset>`.
No targets were skipped. Both Debug registrations grew from 63 to 64; the
stale Release registrations grew from 61 to 64, adding the existing
`persistentmenutest` and `viewtemplatenamestest` as well as the new resolver
test. No previously registered name disappeared.

The coordinator independently repeated all five mutations, requiring the
three policy mutants to compile successfully before their tests failed.
All five were rejected; restoring the exact source bytes made both focused
tests pass again. Final accepted adapter SHA-256:
`de4257f155a3018525e28c3ac37196300b34812a80a69dbd6fe798193d9893c8`.
The resolver header hash remains the worker-restored value above.

Evidence: [matrix summary](A4-resolver-data/validation-summary.json),
[registration comparison](A4-resolver-data/test-registration-summary.json),
[mutation commands and statuses](A4-resolver-data/mutation-summary.json),
[failing assertions](A4-resolver-data/mutation-failures.txt), and
[full validation logs and harnesses](A4-resolver-data/validation-logs.tar.gz).
The archive retains the failed initial attempts as well as final passes.
It excludes user configuration backups and live desktop logs.

QML syntax checking passed for all 241 tracked files; module metadata checks
passed in all four trees. The real private.app host smoke is part of each
passing suite. Deep comparison against the reviewed qmllint 6.11.1 baseline
was not run with the incomparable host 6.11.3 tool; no QML or type registration
was changed. clazy is unavailable locally; remote CI, Debian and Nix checks
were not run. No dependency or minimum-version change is needed.

User-mode installation used `bash install.sh --user Debug --jobs 8`, exit 0,
with zero compiler warnings. The installed executable and user build have
Build ID `6bb4d66801266717542d88ed9abc6616b5361f56`. CMake changes RUNPATH
on installation, so their whole-file hashes differ as expected. The initial
checksum guard stopped before touching the running dock; matching build IDs
and the observed RUNPATH update established the installed identity instead.

The coordinator backed up the relevant configuration under
`/tmp/latte-modernization-wave2/config-before`, saved a coredump baseline,
stopped the packaged dock and launched the user-mode Debug binary through
`systemd-run --user --collect`, with `PartOf=graphical-session.target`,
`After=graphical-session.target` and `TimeoutStopSec=5s`. The current unit is
`latte-dock-ng-debug-a4-20261113-100255.service`, PID 61657, executing
`~/.local/bin/latte-dock-ng`. It remains active for the user.

The user reported the requested screen/position, task/hover and popup smoke
passed. Layout switching was optional and not separately confirmed. Afterwards,
the complete 471-line `/tmp/latte-ng.log` had no emitted warning/error, fatal or
assertion diagnostic, and no new dock coredump appeared. A second pass for
unlabelled Qt/QML diagnostics found only ordinary debug messages, the expected
QML debugging notice and version-cache invalidation. The local Debug version
label is 1.2.59 (the checkout default); source/build identity is recorded above,
not inferred from that label. See [runtime summary](A4-resolver-data/runtime-summary.json).

This acceptance does not prove primary-not-zero or nested-containment behavior
through the entire Corona/Synchronizer chain. Keep those original A4 cases
pending; the isolated production policy tests and adapter contracts are narrower
evidence. No commit or push was performed on `main`.
