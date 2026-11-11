# R7a: Closing items for R1a, R4, dev-env.sh and the Mageia build

Status: Complete for the four items. The `dev-env.sh` rule passed on the real desktop on 2026-11-10
(see [Desktop validation](#desktop-validation)). A VM session and the live Cauldron mirrors
were not checked; see [Not verified](#not-verified).
Date: 2026-11-06. Branch: `claude/r7a-closing-items-417aa9`, cut from `main` at
`e18a0f9a7` (release v1.3.0).

| Item | Commit | Subject |
| --- | --- | --- |
| 1. R1a superseded statements | `637e1b318` | docs: mark the superseded logout statements in the R1a record |
| 2. R4 pending checks | `0453ba35e` | docs: record the R4 CI results and its release status |
| 3. Generated `dev-env.sh` | `d2d6cfe78` | fix: take XDG_CONFIG_DIRS from the user manager in dev-env.sh |
| 4. Mageia dnf retry | `82a8bf58d` | ci: retry the Mageia dnf install inside the image build |
| Record | the commit that adds this file | docs: add the R7a closing record |

Nothing is pushed or merged. Build and test logs are in
`build/modernization/r7a-logs/` in the worktree; git ignores that directory, so
the commands below are the durable part of the evidence.

Debian 13 verification: deferred by the user until the refactor is complete.

## Scope

Each item touched only the files it named: [R1a.md](R1a.md), [R4.md](R4.md),
[install.sh](../../install.sh),
[installscriptbehavior.py](../../autotests/installscriptbehavior.py),
[development-testing-guide.md](../development-testing-guide.md),
[Dockerfile.mageia](../../docker/Dockerfile.mageia),
[packagingcontracttest.cpp](../../autotests/packagingcontracttest.cpp) and this
file. The shared ledger `docs/architecture-modernization-plan.md` and
`docs/qmllint-baseline.json` were not edited. No QML changed, so the qmllint
baseline procedure does not apply.

## 1. R1a: superseded statements (`637e1b318`)

R1a said the logout monitor samples ksmserver and KWin, that the hide, restore
and quit decisions and `shouldQuitForCommittedShutdown()` are unchanged, and that
a committed logout quits when ordinary windows close. [PR #66](https://github.com/ruizhi-lab/latte-dock-ng/pull/66)
took the window list out of the decision. R1a now opens with a "Superseded in
part" paragraph that links [PR66-logout.md](PR66-logout.md). Two one-line
pointers mark the monitor bullet and the "Committed logout (user)" row. The
original text and evidence stay as they were.

Each claim in the paragraph, checked against this checkout:

| Claim | Checked in |
| --- | --- |
| The monitor samples ksmserver only | [shutdownmonitor.cpp](../../app/session/shutdownmonitor.cpp) line 105 is the only probe (`m_hooks.isSessionShuttingDown()`, which [main.cpp](../../app/main.cpp) binds to `ksmServerShutdownState()`); the file makes no KWin call |
| `app/session/shutdownstate.h` was removed | Deleted by `0dc44bdf5`; not in `git ls-files app/session/` |
| `AbstractWindowInterface::hasSessionBlockingWindows()` was removed | Removed by `6d42a9d71`; no hit under `app/` in HEAD |
| `shouldQuitForCommittedShutdown()` no longer exists | Removed by `0dc44bdf5`; no hit under `app/` in HEAD |
| Completion quits: plasma-shutdown leaves `org.kde.Shutdown` while ksmserver still reports the logout | shutdownmonitor.cpp lines 133-136 (`m_hooks.quit()`) |
| Cancel restores: ksmserver reports no logout | shutdownmonitor.cpp lines 118-127 (`restoreViews()`) |
| The confirmed-logout hook also suspends the helper windows | [main.cpp](../../app/main.cpp) lines 669-674 (`hideViews`, `setHelperWindowsSuspended(true)` at line 672); the restore at line 685 passes `false`; the method is at [synchronizer.cpp](../../app/layouts/synchronizer.cpp) line 495 |

## 2. R4: pending checks and next action (`0453ba35e`)

R4 said the Debian 13 job and the Release presets had not run, that nothing was
pushed, and that the review host's dock still ran the build. CI run
[41901895873](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/41901895873)
(`Build`, workflow_dispatch, "Release candidate v1.3.0", head `e18a0f9a7`)
concluded `success` with 23 of 23 jobs successful. Queried with
`gh run view 41901895873 --json conclusion,headSha,jobs` through the local proxy;
the summary query printed `conclusion=success jobs=23 success=23`.

| Job | Result | Steps |
| --- | --- | --- |
| Install verify (debian13) | success | Build debian13 verification image; Verify build and installation |
| Package install verify (debian13) | success | Build debian13 packaging image; Build and install native package; Retain validated release package |
| Build (GCC Release) | success | Build application and autotests; Test; Deep QML lint |
| Build (Clang Release) | success | Build application and autotests; Test; Deep QML lint |
| NixOS release check | success | Check flake; Build NixOS package; Install and uninstall validated Nix package |
| QML lint (qmllint) | success | Lint QML sources |

`5bac5f723` ("Merge the settings window root inputs") is an ancestor of
`e18a0f9a7`, and `git tag --contains 5bac5f723` lists `v1.3.0`.

Changes in [R4.md](R4.md):
- The Evidence sentence about the Release presets now says "on the review host"
  and points to the CI result.
- "Pending and unavailable checks": the CI bullet records the run and the jobs
  above; a separate Nix bullet says the author ran no Nix check and that CI's
  `NixOS release check` passed; the Qt 6.6 floor note is kept.
- "Next action": merged as `5bac5f723`. The review host's dock state is recorded
  as it was at the retest; it was not checked again.
- Kept as written: the clang-format note and the performance note.

## 3. Generated `dev-env.sh`: `XDG_CONFIG_DIRS` (`d2d6cfe78`)

Before this change, an unset `XDG_CONFIG_DIRS` became
`~/.config/kdedefaults:/etc/xdg`, which lacks the session's
`/usr/share/kde-settings/kde-profile/default/xdg`.
[R5 section 3](R5.md#3-the-launch-environment-reproduces-it) measured the Dock,
plasmashell and KWin rewriting the shared KSycoca database against each other in
that state (about 19 %, 22 % and 26 % of a core). The generated file now follows
the table below. The rule is also commented in [install.sh](../../install.sh)
beside the `export`.

| Environment when the file is sourced | Result for `XDG_CONFIG_DIRS` |
| --- | --- |
| Set and non-empty | `${HOME}/.config/kdedefaults:/etc/xdg:` followed by the value, as before. The user manager is not read. |
| Unset, or empty, and `systemctl --user show-environment` prints `XDG_CONFIG_DIRS=<value>` | `<value>`, exactly as printed |
| Unset, or empty, and the query prints no such line or fails | Unset (never invented) |

Decision for review: an empty value counts as unset. The XDG Base Directory
specification treats both alike, and before this change an empty value also
produced the overlay alone. This reads "if it is unset" more widely than the
literal wording. Reverting to "unset only" is a one-line change in the `-n` test
in `install.sh`.

State authority and lifetime:
- A set, non-empty value in the calling shell is authoritative. Otherwise the
  user manager's environment supplies the session value. This host reports
  `XDG_CONFIG_DIRS=/home/ruizhi/.config/kdedefaults:/etc/xdg` (read-only query;
  nothing changed).
- The lookup runs once, when the file is sourced. Nothing is cached.
- The generated file sets no timeout on the `systemctl` call, so a hung user bus
  would delay `source`.

Consumer to review: [test-logout-simulation.py](../../scripts/test-logout-simulation.py)
sources `dev-env.sh` into the simulated dock (`start_latte`) with a scratch
`HOME`, and its `simulation_env()` never sets `XDG_CONFIG_DIRS`. When the caller's
environment lacks that variable, the simulated dock now receives the user
manager's value, which names the real home's `kdedefaults`. Before, it received
the scratch home's path. That script is not part of this slice; see
[Follow-ups](#follow-ups). The simulation was not run: it needs a nested KWin
session, so this finding comes from reading the code.

Test design, in [installscriptbehavior.py](../../autotests/installscriptbehavior.py):
the heredoc between `cat > "$env_file" <<ENVEOF` and `ENVEOF` is extracted from
`install.sh` and rendered by bash with `kde_install_qmldir=/x`. The rendered file
is syntax-checked with `bash -n` and sourced in `env -i` shells whose `PATH`
starts with a fake `systemctl`, which answers only `--user show-environment`. The
test asserts each outcome in the table above.

Checks:

| Command or mutant | Exit | Result |
| --- | --- | --- |
| `bash -n install.sh` | 0 | No output |
| `python3 -I autotests/installscriptbehavior.py` | 0 | `PASS: ... dev-env XDG_CONFIG_DIRS handling` |
| Mutant A: old line from `git show e18a0f9a7:install.sh`, new test, scratch copy | 1 | `AssertionError` at the unset-with-manager-value assertion; the value was `.../kdedefaults:/etc/xdg` |
| Mutant B: working `install.sh` with the `unset` branch changed to `:`, new test, scratch copy | 1 | `AssertionError` at the empty-without-manager-value assertion |

The mutants ran in scratch copies, so the worktree was not edited for them.

## 4. Mageia: retry the dnf install inside the RUN (`82a8bf58d`)

Evidence of the failure, from `gh run list --workflow build.yml --limit 25` and
the job logs:
- Run [41651332853](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/41651332853)
  (2026-11-03): `Package install verify (mageia)` failed.
- Run [41688050686](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/41688050686)
  (2026-11-03): `Install verify (mageia)` and `Package install verify (mageia)` failed.
- Run [41867804572](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/41867804572)
  (2026-11-04): `Package install verify (mageia)` failed.

In each log the dnf step ends with `Librepo error: Cannot download <package>: All
mirrors were tried`. The package differs by run (`lib64kf6syndication6`,
`lib64opencv_reg410`, `sysprof`). The failing step had run for 132 s, 162 s and
524 s before the error. The wrapper then printed `retrying build (1/3)` and started
the whole docker build again, including this RUN. In that listing,
`Package install verify (mageia)` failed in three runs and `Install verify
(mageia)` in one (41688050686); the brief's count of three failures for
`Install verify (mageia)` is not what this listing shows. Run 41901895873 passed
`Install verify (mageia)`.

The RUN now reads:

```dockerfile
RUN dnf clean all && \
    for attempt in 1 2 3; do \
    dnf install -y --refresh --setopt=install_weak_deps=False \
      ... package list unchanged ... \
      gcc-c++ make gettext git pkgconf-pkg-config rpm-build && break; \
    if [ "$attempt" -eq 3 ]; then exit 1; fi; \
    echo "dnf install attempt $attempt failed; retrying in $((attempt * 30)) s" >&2; \
    sleep "$((attempt * 30))"; \
    done && \
    dnf clean all
```

Behaviour:
- A first attempt that succeeds runs the same commands as before (`dnf clean
  all`, the same install, `dnf clean all`) and sleeps nothing.
- `--refresh` is on every attempt, so each one re-reads the metadata. No `dnf
  clean` runs between attempts.
- The RUN fails when the third attempt fails. The pauses are 30 s and 60 s.
- The retry also repeats failures that are not mirror problems, such as an
  unknown package name. Those now cost three attempts and 90 s of pauses. The
  wrapper's pattern check is unchanged.
- Cost on the slowest failure in these logs (524 s per failing attempt): about
  3 × 524 s + 90 s ≈ 28 min per outer attempt, so up to about 83 min for the
  wrapper's three attempts.

Testing without Docker. The loop under test is the RUN text itself.
`extract-run.py` takes the instruction from the Dockerfile, from `RUN dnf clean
all` to its last continuation line. `run-scenario.sh` runs that text with `sh`, a
fake `dnf` and a fake `sleep`. Both fakes log their arguments, and `dnf` fails
`install` as many times as `DNF_FAILS` says. The scripts are archived in
`build/modernization/r7a-logs/mageia-retry/`. Those archived copies write under
`/tmp/r7a-mageia` and expect `bin/dnf` and `bin/sleep` there. The commands:

```bash
git show e18a0f9a7:docker/Dockerfile.mageia > Dockerfile.mageia.base
python3 -I extract-run.py Dockerfile.mageia.base run-base.sh
python3 -I extract-run.py docker/Dockerfile.mageia run-head.sh
sh run-scenario.sh run-head.sh 2 new-flaky-2
sh run-scenario.sh run-head.sh 99 new-always-fails
sh run-scenario.sh run-head.sh 0 new-first-ok
sh run-scenario.sh run-base.sh 0 old-first-ok
sh run-scenario.sh run-base.sh 2 old-flaky-2
```

Results, from the final run (`build/modernization/r7a-logs/final-checks.log`):

| Scenario | RUN | `DNF_FAILS` | Exit | `dnf install` calls | `dnf clean` calls | `sleep` calls |
| --- | --- | --- | --- | --- | --- | --- |
| Fails twice, then succeeds | new | 2 | 0 | 3 | 2 | 30, 60 |
| Always fails | new | 99 | 1 | 3 | 1 | 30, 60 |
| Succeeds at once | new | 0 | 0 | 1 | 2 | none |
| Succeeds at once | old (`e18a0f9a7`) | 0 | 0 | 1 | 2 | none; the logged dnf calls are identical to the new run |
| Fails twice | old | 2 | 1 | 1 | 1 | none |

The contract, `packagingcontracttest`, is built for GCC and Clang Debug, and
ctest reports `8/61 Test #8: packagingcontracttest ... Passed` in both logs. It
asserts the loop text in order: the loop, the `--refresh` install inside it,
`rpm-build && break;`, the last-attempt `exit 1`, the growing `sleep`,
`done && \`, and the final `dnf clean all`. Mutation runs were made on the GCC
build. The file was replaced in the worktree and then restored from a copy, which
was checked with `cmp`.

| Dockerfile under test | ctest exit | Failing check |
| --- | --- | --- |
| The `e18a0f9a7` version, with no loop | 8 | `'mageiaRetryLoop >= 0' returned FALSE` |
| The new file without the last-attempt `exit 1` | 8 | `'mageiaLastAttemptFails > mageiaAttemptSucceeded' returned FALSE` |
| The new file, restored | 0 | 100 % passed |

Limits:
- Whether dnf reuses packages it downloaded in an earlier attempt of the same RUN
  is not verified. The Dockerfile only guarantees that no `dnf clean` runs
  between attempts.
- The 30 s and 60 s pauses and the three attempts were chosen, not measured
  against Cauldron mirror sync times.
- The live Cauldron mirrors were not checked, and no image was built, because
  Docker is not run here. The loop ran under bash as `sh`: `dash` is not installed
  on this host, so the POSIX compatibility of the constructs used is checked by
  reading only.
- The next Mageia job in CI shows whether the pauses are long enough.

## Compiler and test results

| Preset | Command sequence | Warnings | Failed build steps | ctest |
| --- | --- | --- | --- | --- |
| gcc-debug | `cmake --preset gcc-debug`; `cmake --build --preset gcc-debug --parallel 6`; `ctest --preset gcc-debug` | 0 | 0 | 61 of 61 passed |
| clang-debug (`clang++` from `/usr/lib/llvm/22`) | the same with `clang-debug` | 0 | 0 | 61 of 61 passed |

The last run's exit codes are in `build/modernization/r7a-logs/rerun-exit-codes.log`:
configure 0, build 0 and ctest 0 for each preset. Both caches have
`LATTE_STRICT_WARNINGS=ON`. The build logs contain four lines that match
`error:`. They are QML source text printed by the build
(`console.warn("ContextMenu component load error:", ...)`), not compiler
diagnostics. No test was added to or removed from CTest; only the body of
`packagingcontracttest` changed.

clazy is not installed on this host, and CI runs it. The change adds no
`connect()`, lambda or container iteration. clang-format was not run; the edits
are kept to the lines they need.

## Commands and exit status

| Command | Exit |
| --- | --- |
| `git diff --check e18a0f9a7..HEAD` | 0 |
| `bash -n install.sh` | 0 |
| `python3 -I autotests/installscriptbehavior.py` | 0 |
| `gh run view 41901895873 --json conclusion,headSha,jobs`, through `HTTPS_PROXY=http://127.0.0.1:10808` | 0 |
| `git merge-base --is-ancestor 5bac5f723 e18a0f9a7` | 0 |
| Configure, build and ctest for `gcc-debug`, then `clang-debug` | 0 each |
| Mutants and Mageia scenarios | as in the tables above |

Logs in `build/modernization/r7a-logs/`: `final-checks.log`, `rerun-exit-codes.log`,
the configure, build and ctest logs for both presets, `gcc-contract-*.log` for the
contract mutations, and `mageia-retry/`.

## Not verified

- Desktop and VM: the dev-env change was not launched in a real or VM Plasma
  session. R5's launch was not repeated, and this host's user manager value was
  only read.
- `scripts/test-logout-simulation.py` was not run; see section 3.
- The live Cauldron mirrors, and any Docker build (Docker is not run here, per the
  instructions).
- dnf cache reuse between attempts, and whether the pauses are long enough for
  mirror sync.
- The Release presets and Debian 13 were run by CI only (section 2). Debian 13
  verification: deferred by the user until the refactor is complete.

## Follow-ups

Outside this slice, not changed here:
1. `scripts/test-logout-simulation.py`: set `XDG_CONFIG_DIRS` explicitly in
   `simulation_env()`, so that a simulated dock never takes the user manager's
   value.
2. `AGENTS.md`, "Remote VM launches" (line 166): it describes the old KSycoca
   loop. Update it after the new `dev-env.sh` has been checked on the VM.
3. `docs/architecture-modernization-plan.md`, R5 row: its "Candidates" text names
   this change. The ledger is the main session's to update.
4. Optional: a timeout around `systemctl --user show-environment` in the generated
   file, if a hung user bus is ever seen.

## Next action

Review the four commits on `claude/r7a-closing-items-417aa9` (`637e1b318`,
`0453ba35e`, `d2d6cfe78`, `82a8bf58d`), then merge the branch if the user approves.
After that, make the follow-up 1 change in `scripts/test-logout-simulation.py`.

Commit approval status: committed on the branch; not pushed; not merged. A push or
a merge needs the user's approval.

## Desktop validation

Checked on 2026-11-10 and 2026-11-11 on the user's desktop. Both the subshell check and the real-session check of the unset-`XDG_CONFIG_DIRS` branch passed. The dock got the user manager's value once, used 0 CPU ticks in 10 s and caused no KSycoca activity. A VM session, the Mageia retry and dnf's reuse of downloaded packages were not checked. Full record: [R7-desktop-check](R7-desktop-check.md).
