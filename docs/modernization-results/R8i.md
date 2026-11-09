# R8i: clang-tidy gate over the clazy gate's production sources

Status: Complete for the gate change, verified below. The Build run on the merge commit
is the remaining check (its Clang Debug job runs this gate); the ledger row says so.

Batch / slice and selected scope: R8i, step 1 of the gate order in [R8c](R8c.md)
("Proposed gate order"): the gate's two checks over all production sources instead of
`app/data/errordata.cpp`, each proven by its own negative fixture under the production
flags. Acceptance: both fixtures rejected; no finding in the 155 production sources with
clang-tidy 18.1.3 (CI's version) and 22.1.8; GCC and Clang Debug with 0 compiler warnings
and CTest; CI's Clang Debug clang-tidy step green.

Source baseline and start HEAD: `86de58ed6` (local and remote `main`). Branch
`claude/distracted-chatterjee-722eed`, worktree `.claude/worktrees/distracted-chatterjee-722eed`.
Commits: `3baa2252c` (runner, configuration, fixtures and testing guide), `85f34625b`
(a diagnostic the pattern cannot read fails the run), and the record commit. The series
changes no C++, CMake or QML input: the only `.cpp` files it touches are the fixtures under
`autotests/clang-tidy/`, which no target compiles. `85f34625b` changes only the runner
after `3baa2252c`, the commit that was built and tested.

## What changed

- `scripts/test-clang-tidy.py` reuses the clazy gate's source selection. `scripts/test-clazy.py`
  is imported with importlib and registered in `sys.modules` first (its dataclasses need that),
  and `select_translation_units`, `unit_path`, `first_party`, `unique`, `retarget` and `relative`
  come from it. One compile command per production source; `autotests/` and generated sources
  are skipped. One clang-tidy process per source, from a temporary compile database that holds
  only the selected commands.
- `GATED_CHECKS` names `bugprone-unused-return-value` and `bugprone-use-after-move`.
  `clang-tidy --config-file=.clang-tidy --list-checks` must list exactly these two.
- `autotests/clang-tidy/<check>.cpp`, one per gated check. `bugprone-unused-return-value.cpp`
  is `unused-return-value.cpp` renamed to its full check name (same code, one comment line
  added); `bugprone-use-after-move.cpp` is new. Each fixture is copied and compiled with the
  flags of `app/lattecorona.cpp`, as the clazy gate does, and must produce an `error:` for its
  own check.
- `.clang-tidy`: `WarningsAsErrors: '*'` is kept. `HeaderFilterRegex` changes from
  `^.*/app/data/.*$` to `'\.h$'`, which admits every header that a translation unit reaches.
  clang-tidy's regular expressions have no lookahead and the file does not know the checkout
  path, so the file cannot exclude generated build files or `autotests/` itself. The runner's
  `first_party` filter does that: clang-tidy may report such findings, but they never count.
- `docs/development-testing-guide.md`: the clang-tidy paragraph describes the scope and the
  proof. The CI command line is unchanged.

## Decisions

- **Errors, not warnings.** With `WarningsAsErrors`, clang-tidy prints `error:` and appends
  `-warnings-as-errors` to the bracket list. The runner strips that marker, which is not a
  check name. A fixture counts as rejected only with an `error:` for its own check, so a
  configuration that loses `WarningsAsErrors` fails the proof.
- **Compiler diagnostics.** The production commands carry the strict-warning flags
  (`-Wall -Wextra` and more) and `-Werror`, so the analysis adds `--extra-arg=-Wno-error`.
  Compiler warnings belong to the strict-warning gate and do not stop this gate; a compile
  error (`clang-diagnostic-error`) does.
- **Fail closed.** A gated finding in a first-party source fails the run. So do a compile
  error, a timeout, a failed exit with no finding, a failure line, and a diagnostic with a
  check bracket that the finding pattern cannot read. The last rule was added in `85f34625b`:
  such a line may hide a gated finding, for example one with a relative path.
- **Counting.** A finding in a header that several translation units reach is counted once
  (`unique`), as in the clazy gate.

## Verification

The gate prints the same lines on the container and on the host:

```
selected 155 production sources; skipped 55 autotest source, 137 duplicate compile command of a selected source, 86 generated source in the build directory
2 negative fixtures: each is rejected as an error by its own gated check
155 production sources: no findings from 2 gated clang-tidy checks
```

The skipped counts equal the R8c inventory's (55, 137 and 86). The last line has no
"not counted" suffix, so clang-tidy reported no finding of either check outside the
first-party sources either.

### Container (CI image)

`latte-r2c-clazy:noble` (Ubuntu 24.04, `clang` and `clang-tidy` 18.1.3, the versions the CI
Clang Debug job installs), run with rootless Docker and without `--user`. The source was a
`git archive` export mounted read-only at `/src` with an empty `build/` as the mount point of
a writable build tree under `/data/tmp/r8i/container-build`.

| Step | Commit | Result |
|---|---|---|
| Configure and build, `clang-debug` preset | `3baa2252c` | exit 0; 0 `warning:` lines; 429 units compiled |
| Gate, `--jobs 4` | `3baa2252c` | exit 0 in 233 s |
| CTest, `ctest --preset clang-debug` | `3baa2252c` | 60/60 passed in 36.9 s |
| Gate, `--jobs 4`, final runner | `85f34625b` | exit 0 in 200 s |

The container's CTest has 60 tests where the host has 61: `appstreamtest` is missing because
the container has no `appstreamcli` (its CMake cache has `APPSTREAMCLI-NOTFOUND`), so the test
is not registered there.

The 233 s run began right after the build ended, with a one-minute load of about 25. The
200 s run is the reported figure: the one-minute load was 0.42 when it began and 5.68 when it
ended, a rise consistent with the gate's four processes.

### Host (Gentoo; clang 22.1.8 and g++ 16.2.1)

| Step | Preset | Result |
|---|---|---|
| Configure and build | `clang-debug` | 0 `warning:` lines (the build log has 5356 `Warning:` lines from qmllint, see below) |
| Gate with clang-tidy 22.1.8, `--jobs 16` | `clang-debug`, `85f34625b` | exit 0 in 156 s; load not recorded |
| CTest | `clang-debug` | 61/61 passed in 37.1 s |
| Configure and build | `gcc-debug` | exit 0; 0 `warning:` lines; 429 units compiled |
| CTest | `gcc-debug` | 61/61 passed in 34.5 s |

The `Warning:` lines are qmllint messages that the build prints for QML files, for example
`Warning: ../plasmoid/package/contents/ui/PulseAudio.qml:396:13: Unqualified access [unqualified]`.
They are not compiler warnings. The counts differ between the Ubuntu image (6814) and the
host (5356) because the qmllint versions differ. This series changes no QML or build input;
the lines were not compared with a build of `86de58ed6`.

### Fail-closed checks (host, clang-tidy 22.1.8)

Three scratch edits to the worktree, each run through the gate and then restored with
`git checkout --`; the tree was clean afterwards.

| Edit | Expected | Observed |
|---|---|---|
| Remove `WarningsAsErrors: '*'` from `.clang-tidy` | fixtures not rejected | exit 1: `the gate did not reject its negative fixture(s): bugprone-unused-return-value.cpp, bugprone-use-after-move.cpp` |
| Add `misc-no-recursion` to `Checks` | list mismatch | exit 1: `clang-tidy enabled [...three names...], but GATED_CHECKS names [...two names...]` |
| Insert a function that uses `std::move` and then `moved.size()` into `app/data/errordata.h`, before its final `#endif` | the finding counted once | exit 1: `1 clang-tidy finding(s) in gated checks: app/data/errordata.h:60:29: [bugprone-use-after-move] 'moved' used after it was moved` |

That header is reached through `app/layouts/storage.h`, `app/layout/genericlayout.h` and
`app/data/errordata.cpp`, so several translation units report it and the runner counts it once.

Not exercised: the unreadable-diagnostic rule of `85f34625b` (no edit produced a line the
pattern cannot read), and clang-tidy 18.1.3 (the edits ran with 22.1.8 only).

## Cost

| Run | Tree | clang-tidy | Jobs | Wall (s) | One-minute load at start | Note |
|---|---|---|---:|---:|---:|---|
| R8c inventory, the same two checks | container | 18.1.3 | 4 | 213 | 0.22 | R8c record; no fixture proof |
| Gate, `3baa2252c` | container | 18.1.3 | 4 | 233 | about 25 | started right after the build |
| Gate, `85f34625b` | container | 18.1.3 | 4 | 227 | 8.8 | the chain's wait for a quiet host expired |
| Gate, `85f34625b` | container | 18.1.3 | 4 | **200** | 0.42 | reported figure |
| Gate, `85f34625b` | host | 22.1.8 | 16 | 156 | not recorded | run after the host Clang build |

On the same image and jobs the gate costs about what R8c's two-check run cost (200 s against
213 s); the fixture proof adds two clang-tidy processes.

## Reproduce

```bash
python3 scripts/test-clang-tidy.py --build-dir build/modernization/clang-debug --jobs 4
```

For the container run, export the commit with `git archive` into a directory with an empty
`build/`, mount the export read-only at `/src`, mount a writable build tree at `/src/build`, and
run the CI steps inside `latte-r2c-clazy:noble` (the recipe is in [R2c](R2c.md)).

## Not verified

- CI: the Build run on the merge commit has not been observed when this record is written.
  Its Clang Debug job's clang-tidy step is the check the ledger row names.
- The timings come from this host's 16 cores with `--jobs 4`, not from CI's runners.
- The qmllint warnings were not compared with a build of `86de58ed6`.
- The runner has no unit test of its own. Its fail-closed paths were checked by the three
  edits above, not by a fake-based test like `autotests/test_clazy_runner.py`.
- `scripts/test-clazy.py` is unchanged and its gate was not re-run here; its CI step runs
  after this one.
- The gate runs on Clang only, as before; the GCC build and CTest are verified above but are
  not part of the clang-tidy gate.
