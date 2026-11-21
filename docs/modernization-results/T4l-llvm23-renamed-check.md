# LLVM 23 deprecated-check migration: `performance-faster-string-find`

Status: Complete. The gate runs green on clang-tidy 18.1.8, 22.1.8 and 23.1.5
without lowering any supported tool version and without disabling the check.

Date: 2026-11-21. Base: `main` at `399bdff19`.

## Problem

LLVM 23 deprecates `performance-faster-string-find` in favour of
`performance-prefer-single-char-overloads`. Because the gate sets
`WarningsAsErrors: '*'`, the deprecation notice is itself an error, emitted per
fixture before any production source is analysed. On the user's Arch VM the gate
therefore stopped in 0.76 s with

```
error: 'performance-faster-string-find' check is deprecated and will be removed in a
future release; consider using 'performance-prefer-single-char-overloads' instead
[clang-tidy-config,-warnings-as-errors]
```

The obvious fix - rename the check - would break the CI image and the Debian
floor, because the successor does not exist before LLVM 23:

| Check | LLVM 18.1.8 | LLVM 22.1.8 | LLVM 23.1.5 |
| --- | --- | --- | --- |
| `performance-faster-string-find` | present | present | present, **deprecated** |
| `performance-prefer-single-char-overloads` | absent | absent | present |

Measured with `clang-tidy --list-checks -checks='-*,<name>'` on the Debian 13.7
VM, this host and the Arch VM. A single fixed spelling cannot work on all three.

## Decision

Keep the **successor** in `.clang-tidy` and in `GATED_CHECKS`, and substitute the
working predecessor at run time when the installed tool does not know the
successor. The configuration states where the tooling is going; the gate adapts to
the tool that is actually present.

The successor is preferred in the checked-in files because it is the name every
future clang-tidy has, so the configuration only gets older tools translated
instead of needing periodic edits.

`RENAMED_CHECKS` in `scripts/test-clang-tidy.py` records the pair. The runner
probes `--list-checks` over all checks (a capability probe, not a version
comparison, per the project's compatibility rules), writes an effective
configuration into a temporary directory, and reports every substitution:

```
performance-prefer-single-char-overloads is unknown to this clang-tidy; gated performance-faster-string-find instead
```

Fail-closed behaviour: if the tool knows neither spelling, or if `.clang-tidy`
stops naming the successor while `RENAMED_CHECKS` still tracks it, the gate
aborts. A silently dropped check cannot pass.

## Why the two checks are interchangeable

The successor's documented trigger and options match the old check: a
single-character string literal passed to `find`, `rfind`, `find_first_of`,
`find_first_not_of`, `find_last_of`, `find_last_not_of`, `starts_with`,
`ends_with`, `contains` or `operator+=` of a `basic_string`/`basic_string_view`.

Confirmed on the same fixture rather than from the documentation alone: on LLVM
23 the unchanged fixture produces the same diagnostic at the same line and column,
differing only in one word of the message.

| Tool | Check that fired | Message |
| --- | --- | --- |
| LLVM 22.1.8 | `performance-faster-string-find` | `'find' called with a string literal consisting of a single character; consider using the more effective overload accepting a character` |
| LLVM 23.1.5 | `performance-prefer-single-char-overloads` | `... consider using the more efficient overload accepting a character` |

The fixture file is renamed to `autotests/clang-tidy/performance-prefer-single-char-overloads.cpp`
so it keeps one file per gated check. `prove_fixtures` compares a fixture's
diagnostic against the spelling in force for the running tool, so the renamed
fixture is proven on all three releases.

## Files

| File | Change |
| --- | --- |
| `.clang-tidy` | Names `performance-prefer-single-char-overloads` instead of the deprecated check. |
| `scripts/test-clang-tidy.py` | `RENAMED_CHECKS`, `known_checks()`, `write_check_config()`; the effective configuration threads through `enabled_checks`, `analyze`, `prove_fixtures` and `run_gate`; the enabled-check comparison and the fixture match resolve through the substitution. |
| `autotests/clang-tidy/performance-prefer-single-char-overloads.cpp` | Renamed from the predecessor's file name; content unchanged. |
| `autotests/test_clang_tidy_renames.py` | New: six cases over the selection, the substitution report, config preservation and both fail-closed paths. |
| `autotests/CMakeLists.txt` | Registers `clangtidyrenamestest`. |

No production source changed. The number of gated checks is still 56.

## Verification

| Gate | Result |
| --- | --- |
| LLVM 22.1.8, this host, `--jobs 8` | substitution reported; `56 negative fixtures: each is rejected...`; `157 production sources: no findings from 56 gated clang-tidy checks` |
| LLVM 18.1.8, Debian 13.7 VM | substitution reported; 56/56 fixtures; no production findings (1 out-of-scope SDK finding, as before) |
| LLVM 23.1.5, Arch VM, Clang 23 build | **no substitution reported** (successor used directly); 56/56 fixtures; no production findings. This is the failure this slice fixes, now passing |
| `autotests/test_clang_tidy_renames.py` | 6/6 pass; registered as `clangtidyrenamestest`, passes on Arch and on this host |
| Mutations | never substitute: 3 failures; always substitute: 1 failure; drop the not-tracked guard: 1 failure |
| Four presets, `cmake --preset` + `--build --preset -j8` | 0 compiler warnings each |
| Four presets, `ctest` | 67/67 in each (was 66; adds `clangtidyrenamestest`) |

The Arch and Debian builds behind these runs had 0 compiler diagnostics
themselves.

## CI acceptance

Published as `cad5b3146`. Build run
[44874290411](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/44874290411)
passed all 23 jobs. The `Build (Clang Debug)` job, which owns the gate, passed its
`Run targeted clang-tidy checks` step on the CI image's clang-tidy 18.1.3, so the
substitution path is confirmed on the CI tool as well as on the Debian 13.7 VM's
18.1.8.

Two jobs in the first attempt failed at `Install KDE Neon repository` and
`Install Git and KDE Neon repository`, before `actions/checkout`; the mirror was
degraded at the time (an earlier check had returned HTTP 502). After it answered
HTTP 200 again, `gh run rerun --failed` completed them and the run turned green.
No source change was involved.

## Limits

- LLVM 19, 20 and 21 were not available. The rule they follow is read from the
  capability probe at run time, so no assumption about a specific version is
  baked in; only 18, 22 and 23 were executed.
- The Arch and Debian CTest suites were re-run only for the two Python tests this
  slice touches (`clangtidyrenamestest`, `measurefamilyrulestest`). Their full
  suites were last recorded in [VM validation](VM-validation-2026-11-21.md);
  nothing in this slice changes C++ or QML.
- If a future clang-tidy removes the predecessor entirely, nothing needs to change:
  the probe finds the successor and no substitution is made (verified here by
  passing only the successor to `write_check_config`). A tool that knows
  **neither** spelling still aborts, which is the intended fail-closed behaviour.

## Next action

None for this slice. When a later clang-tidy renames another gated check, add the
pair to `RENAMED_CHECKS` with a fixture-level equivalence check like the one above.
