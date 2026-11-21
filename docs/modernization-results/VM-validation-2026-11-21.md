# VM validation, 2026-11-21: Debian 13 and Arch Plasma 6.8

Status: executed on both user-provided VMs. T5's LLVM 18 gate is confirmed on the
Debian distribution floor; the KSycoca validity rule is confirmed on a live
Plasma session. The Arch run also reproduces the known LLVM 23 tool transition.

The user supplied both VMs and authorized full access with passwordless sudo.

## Environments

| | Debian 13.7 (`192.168.122.177`) | Arch (`192.168.122.194`) |
| --- | --- | --- |
| OS | Debian GNU/Linux 13 (trixie), kernel 6.12.116 | Arch, kernel 7.3.4 |
| Plasma / KF | `plasma-workspace 4:6.3.6-2`, KF 6.13.0, libplasma 6.3.5 | `plasma-workspace 6.8.3-1` |
| Qt | 6.8.2 | 6.8.2 |
| Compiler | GCC 14.2.0, Clang 19.1.7 | GCC 16.2.1, Clang 23.1.5 |
| CMake | 3.31.6 | 4.5.0 |
| clang-tidy | 18.1.8 (installed for this run) | 23.1.5 default; 18.1.8 installed for the comparison |
| Session | Plasma Wayland, seat0, active | Plasma Wayland, seat0, dock running as PID 714 |
| Disk free | 6.8 G | 1.1 G, 4.5 G after `pacman -Sc` |

Debian is the oldest supported desktop stack (the floor the plan records as
Debian 13.7). Arch is the forward-compatibility stack. Neither VM had the
repository; a source archive excluding build trees and `docs/*-data` was copied
in, including `.github/` after the first run showed that directory missing.

## T5 on the Debian distribution floor - LLVM 18 gate passes

| Step | Result |
| --- | --- |
| `cmake --preset clang-debug` | configured; `clang++` 19.1.7 |
| `cmake --build --preset clang-debug -j8` | built, **0 compiler warnings, 0 errors** over the whole tree; the 3268 `Warning:` lines in the log are qmllint QML diagnostics, not compiler output |
| `scripts/test-clang-tidy.py --build-dir build/modernization/clang-debug --jobs 8` (clang-tidy 18.1.8) | `56 negative fixtures: each is rejected as an error by its own gated check`; `157 production sources: no findings from 56 gated clang-tidy checks (1 outside first-party production scope were not counted)`; 55.3 s wall |
| The one excluded finding | `bugprone-forward-declaration-namespace` at `/usr/include/Plasma/plasma/applet.h:45` - a system libplasma header, not project code, the same class of out-of-scope SDK finding as T4i recorded |

This is the acceptance T5 was waiting for: the fifty-sixth check, including its new
negative fixture, holds under clang-tidy 18.1.8 on the oldest supported stack. The
earlier slice record could only measure clang-tidy 22.1.8 locally.

## Debian CTest: 65/66, one accessibility-environment failure

`ctest` on the Debian Clang Debug tree: 65 passed, 1 failed. The failure is
`settingsviewunittest::viewsTableEmitsSelectionAccessibilityEvents`, which asserts
`QAccessible::isActive()`.

The failure is the environment, not the code:

- A standalone probe (`QGuiApplication` + `QAccessible::isActive()`, linked against
  this VM's Qt 6.8.2) also returns false under the session's `QT_ACCESSIBILITY=1`.
- `org.a11y.Status IsEnabled` was `false`; setting it true through D-Bus changed the
  property but the probe still reported false.
- This VM image has no Qt accessibility bridge plugin directory
  (`qt6/plugins/accessible/` is absent) and no `libqspiaccessibility.so`, so a Qt 6.8.2
  process on it cannot activate accessibility at all.

[Qt-test-compatibility-2026-11-13](Qt-test-compatibility-2026-11-13.md) records that
the case passes on Debian in its actual Wayland session. That statement holds for an
image with the accessibility bridge; this VM image does not carry it. The case was not
skipped or relaxed, and no code change is proposed for it.

Three other tests failed on the first run because the archive lacked `.github/`:
`packagingcontracttest`, `releaseversiontest` and `neonaptretrytest` read
`.github/workflows/`. After that directory was added all three passed. That was an
artifact of how the sources were copied into the VM, not a source defect.

## KSycoca validity rule: live positive and negative cases (Arch)

`scripts/measure-process-family.py` was copied to the Arch VM and run with
`systemd-run --user` against the running session, so the dock had the session
environment that R5 requires.

**Negative case (real session environment):**

```
{"cpu_percent_one_core": 0.5996, ..., "invalid_reasons": [], "sycoca_cache_versions_seen": 3, "valid": true}
```

**Positive case (a rebuild forced inside the window with `kbuildsycoca6 --noincremental`):**

```
{"cpu_percent_one_core": 0.9996, ...,
 "invalid_reasons": ["KSycoca rewritten at second 4: ksycoca6_en_V_JKMtoq1LX1Crmf31fIi+UXyz4="],
 "sycoca_cache_versions_seen": 3, "valid": false}
```

The collector reported the exact cache file and the second it changed, and marked the
window invalid. This is the first live proof of the rule; until now it had only unit
tests and mutations. It also shows the CPU reading more than doubling in the
contaminated window, which is the error the rule exists to keep out of a comparison.

Two attempts did **not** trigger a rewrite and are recorded so they are not repeated:
changing only the collector's own `XDG_CONFIG_DIRS` does not affect an already running
dock (R5's trigger is the dock's own launch environment), and launching a fresh
`konsole` with a short `XDG_CONFIG_DIRS` did not rebuild the cache within its window.

## Arch: the LLVM 23 deprecated-check transition reproduces

The user's Arch `clang-tidy` is 23.1.5. The gate stops before analysis:

```
error: 'performance-faster-string-find' check is deprecated and will be removed in a
future release; consider using 'performance-prefer-single-char-overloads' instead
[clang-tidy-config,-warnings-as-errors]
```

`WarningsAsErrors: '*'` turns the deprecation notice into a failure for every fixture
(0.76 s, exit non-zero). The Arch Clang Debug build itself is clean: **0 compiler
diagnostics**, `EXIT=0`, on Clang 23.1.5 with GCC 16.2.1.

This is a tool-version transition, not a regression from T5. The plan already lists
"the newer Arch LLVM 23 tool deprecates an older check name; that migration remains
separate from this gate promotion" (T4c). It now has a reproducible environment and an
exact message.

Running the 18.1.8 gate against the Clang 23 compile database is not meaningful: LLVM
18 cannot parse GCC 16's libstdc++ headers, and it fails on
`stl_iterator.h` with `clang-diagnostic-error` before reaching any project source. The
gate is a Clang-build gate, and the Debian run above is the correct 18.1.8 pairing.

## What this changes in the ledgers

- **T5**: the CI-version question is answered on Debian 13.7 with clang-tidy 18.1.8.
  The remaining claim in [T5-optional-promotion](T5-optional-promotion.md) is that the
  GitHub runner's own clang-tidy 18.1.3 job has not been observed; this VM is 18.1.8,
  one patch release later.
- **P1b measurement foundation**: the rule now has a live positive case.
- **LLVM 23 migration**: unchanged as a candidate, but now has a recorded environment,
  command and message instead of a note.

## Limits

- The Debian VM's batch-mode SSH on first contact reset the Arch SSH daemon once; retries
  succeeded. Not a project condition.
- Neither VM ran the Release presets. Both runs are Debug, which is what the clang-tidy
  gate requires.
- No desktop interaction beyond the running session was performed; the dock was not
  restarted or reconfigured on either VM.
- The Debian accessibility limitation is recorded as an image property. Proving the case
  passes needs an image with the Qt accessibility bridge, or a different desktop stack.
