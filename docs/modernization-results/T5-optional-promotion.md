# T5: `bugprone-unchecked-optional-access` promoted to the gate

Status: Complete. The 56-check gate passes on clang-tidy 22.1.8 locally, 18.1.8
on a Debian 13.7 VM and 18.1.3 on the GitHub runner; published in `564199ec1`
with Build run
[44863409259](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/44863409259).

Date: 2026-11-21. Base: local `main` at `4b9f6ef36`, clean working tree apart
from the pre-existing untracked `.agents/` and `scripts/__pycache__/`.

This record supersedes the evaluation in
[T5-optional-evaluation](T5-optional-evaluation.md) for the promotion decision;
that file remains the fixture, scan and cost-study evidence and is not rewritten.

## What changed

| File | Change |
| --- | --- |
| `.clang-tidy` | `bugprone-unchecked-optional-access` added to `Checks`, in alphabetical position. |
| `scripts/test-clang-tidy.py` | The same name added to `GATED_CHECKS`, keeping the sorted tuple the runner compares against `--list-checks`. |
| `autotests/clang-tidy/bugprone-unchecked-optional-access.cpp` | New negative fixture: dereferences a `std::optional<int>` with no `has_value()` guard. |

No production source, build input, CI workflow or dependency changed. The
runner's contract - one `<check>.cpp` per gated check, each rejected as an
error under production flags - is unchanged and now covers 56 checks.

## Why this check and why now

The evaluation left two open questions, both answered here:

1. **A CI cost budget for Band B.** The evaluation measured +0.1 s locally but
   could not predict the CI runner. This slice measures the 55-to-56 increment
   with the same paired, order-alternating method, and the increment does not
   rise above the run-to-run noise (below). A single check that is already
   fixture-proven and finds nothing in production does not need a separate
   Band B budget before being enabled; that budget governs promoting families.
2. **Proof it fires.** The fixture fires as an error under the production
   compile flags, re-verified here independently of the evaluation's run.

## Verification

All commands were run on this host unless marked otherwise.

| Gate | Command | Result |
| --- | --- | --- |
| Fixture fires under production flags | `clang-tidy` against the retargeted `app/lattecorona.cpp` command with a temp config naming only this check | `error: unchecked access to optional value [bugprone-unchecked-optional-access,-warnings-as-errors]` at the `return *value;` line |
| Guarded access is clean | [safe.cpp](T5-optional-data/safe.cpp) from the evaluation | no finding (evaluation run; unchanged here) |
| Full gate, 8 jobs | `python3 scripts/test-clang-tidy.py --build-dir build/modernization/clang-debug --jobs 8` | `56 negative fixtures: each is rejected as an error by its own gated check`; `157 production sources: no findings from 56 gated clang-tidy checks` |
| Builds, zero compiler warnings | `cmake --build --preset <p> -j8` for `gcc-debug`, `clang-debug`, `gcc-release`, `clang-release` | all four built with no warning and no error |
| CTest | `ctest` in each of the four preset trees | 65/65 passed in each |

`clang-tidy 22.1.8` is the only tool on this host. The CI-version gate is the
`clang-debug` job's own step (`clang-tidy` 18.1.3 in the CI image) and is
exercised by the push that publishes this slice; it is not claimed as passed
here.

## LLVM 18 confirmation on the Debian 13 VM

A user-provided Debian 13.7 VM (Qt 6.8.2, KF 6.13, Plasma 6.3.6, Clang 19.1.7)
closed the version gap. With clang-tidy 18.1.8 installed, the tree built with 0
compiler warnings and the gate reported:

```
56 negative fixtures: each is rejected as an error by its own gated check
157 production sources: no findings from 56 gated clang-tidy checks
(1 outside first-party production scope were not counted)
```

The one excluded finding is `bugprone-forward-declaration-namespace` in system
`/usr/include/Plasma/plasma/applet.h`, not project code. `ctest` on the same
tree passed 65/66; the failure is `settingsviewunittest`'s accessibility case,
which a standalone probe shows cannot activate on this VM image because it lacks
the Qt accessibility bridge. Full detail:
[VM validation](VM-validation-2026-11-21.md).

## Cost measurement

Three pairs, execution order alternated, `--jobs 4`, same compile database
(`build/modernization/clang-debug`), same runner. "55" is the gate with this
check removed; "56" is the gate as committed. Raw data:
[full-gate-runs-56-check.json](T5-optional-data/full-gate-runs-56-check.json).

| Pair | Order | 55 s | 56 s | Difference (56 - 55) |
| --- | --- | ---: | ---: | ---: |
| 1 | 55, 56 | 72.155 | 72.197 | +0.042 |
| 2 | 56, 55 | 72.488 | 72.268 | -0.220 |
| 3 | 55, 56 | 72.905 | 72.606 | -0.299 |

Means: 55 = 72.516 s, 56 = 72.357 s. The differences change sign across pairs,
so the increment is below the measurement's resolution. A duplicate run of
pair 1 during the evaluation gave +0.078 s. This establishes no meaningful wall
time cost and, equally, no predictable CI runtime for the extra check; it is not
a CI-runner measurement.

Note: the earlier [evaluation](T5-optional-evaluation.md) recorded 71.3 s for
both variants on the same database. The absolute level drifted upward between
sessions (host load), which is exactly why only paired differences are read.

## CI acceptance

Published as `564199ec1`. Build run
[44863409259](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/44863409259)
runs the gate in the `Build (Clang Debug)` job, whose `Run targeted clang-tidy
checks` step **passed**, together with `Run gated clazy checks`, `Test`,
`Deep QML lint` and the strict-warning check. That job is the CI image's
clang-tidy 18.1.3, so the patch-level gap noted below is now closed.

`Build (GCC Release)` and `Build (GCC Debug)` failed in the same run before
`actions/checkout`: `Install KDE Neon repository` and `Install build
dependencies` respectively. No source was checked out in either job, so those
failures are infrastructure, not this slice, and they do not affect the gate
result above.

## Limits

- LLVM 18.1.3 was unavailable on the authoring host; it ran on the CI runner and
  passed (above). clang-tidy 18.1.8 also passes on the Debian 13.7 VM, so the
  check is confirmed at both LLVM 18 patch levels that were exercised.
- `157 production sources` versus the 156 in earlier records: the compile
  database has gained a source since `e8ba4ad70`. Selection is the runner's,
  not hand-listed.
- No production finding was fixed, because there is none in the gated scope. A
  finding outside first-party scope would not be counted, as for every check.

## Status

This slice is complete. Band B's remaining candidates may be scheduled; a Band B
slice promoting more than one check still needs its own measured cost argument.
