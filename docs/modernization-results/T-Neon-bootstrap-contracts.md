# CI Neon bootstrap: retry-policy ordering and copy drift

Status: Complete. Two defects in the KDE Neon bootstrap steps are fixed and
pinned by `neonrepositorytest`; both are CI-configuration defects, no product
source changed.

Date: 2026-11-21. Base: `main` at `0afefd927`.

## How this slice started

Three consecutive pushes on 2026-11-21 lost CI jobs at the Neon steps:

| Run | Jobs lost | Step |
| --- | --- | --- |
| 44863409259 | Build (GCC Release), Build (GCC Debug) | `Install KDE Neon repository`, `Install build dependencies` |
| 44866313690 | Build (Clang Debug), Build (GCC Release) | `Install KDE Neon repository` |
| 44874290411 | QML lint, Build (GCC Release) | `Install Git and KDE Neon repository`, `Install KDE Neon repository` |

Direct mirror checks during the same period returned
`archive.neon.kde.org` -> **HTTP 502** while `origin.archive.neon.kde.org`
returned HTTP 200, so the mirror was genuinely degraded. A rerun after it
recovered turned every job green, and run 44874290411 finished 23/23.

## Defect 1: the first index update ran outside the retry policy

Both steps started with a bare `apt-get update` and only afterwards wrote
`Acquire::Retries "5"`:

```sh
apt-get update                                     # no retry policy yet
apt-get install -y ... wget gnupg git
echo 'Acquire::Retries "5";' > /etc/apt/apt.conf.d/80-retries
```

The base Ubuntu indexes were therefore fetched without retries. That is what
failed the QML lint job, which does not use the Neon repository for its own
packages until later. The policy is now written **before** the update, so that
same command retries itself.

## Defect 2: the Neon bootstrap was duplicated with no guard

Each step carried its own ~30-line copy of `neon_try()` and the mirror loop.
Both steps run before `actions/checkout`, so they cannot share a repository file,
which makes the duplication unavoidable - but nothing detected drift, so an edit
applied to one copy would have silently weakened the other.

The copies are still inline, now with a comment naming the reason and the test
that pins them. `autotests/test_neon_repository_contract.py` requires them to be
byte-identical.

## What the retry loop was already doing right

The loop was verified by simulation before changing anything, so the slice would
not add retries that already existed:

| Scenario | Result |
| --- | --- |
| `archive` answers 502 twice, then 200 | recovers on attempt 3 |
| `archive` answers 502 all three times, `origin` works | falls back to `origin.archive.neon.kde.org` |

Three attempts per mirror over both mirrors is therefore sufficient for the
outage that was observed; the ordering defect above was the real gap.

## Files

| File | Change |
| --- | --- |
| `.github/workflows/build.yml` | In both Neon steps, the `Acquire::Retries` policy now precedes `apt-get update`; the duplicated bootstrap carries a comment naming the duplication reason and `neonrepositorytest`. |
| `autotests/test_neon_repository_contract.py` | New: seven contracts - two copies exist, are identical, try both mirrors three times, fail loudly, keep `--error-on=any`, write the policy before updating, and do not start with a bare update. |
| `autotests/CMakeLists.txt` | Registers `neonrepositorytest`. |

## Verification

| Check | Result |
| --- | --- |
| `python3 -I autotests/test_neon_repository_contract.py` | 7/7 pass |
| Mutation: restore the old order (update, then policy) | 1 failure |
| Mutation: drop `--error-on=any` from one copy | 2 failures |
| `yaml.safe_load` of `build.yml` | parses; both Neon steps present |
| `bash -n` over every `run:` script in the workflow | no syntax errors |
| Four presets, `ctest` | 68/68 in each (was 67; adds `neonrepositorytest`) |

The test reads the workflow as text and needs no network, Docker or GitHub
access, so it runs in every local CTest session.

## Limits

- The fix removes the ordering defect that failed the QML lint job, but it does
  not make CI independent of the Neon mirror: an outage longer than three
  attempts on both mirrors still fails the job, by design. Adding more attempts
  would only lengthen a real outage.
- The failing job logs had already been replaced by the successful rerun, so the
  exact per-attempt output of the failed steps is not retained. The defects were
  established from the workflow text and reproduced with the contracts and the
  simulations above, not from those logs.
- No CI run has yet exercised the reordered steps; the push that publishes this
  slice is the first.

## Next action

Confirm on the publishing run that both Neon steps log
`KDE Neon repository: <host>` and that no job fails at them. If a future outage
outlives three attempts on both mirrors, treat it as an upstream incident rather
than raising the attempt count.
