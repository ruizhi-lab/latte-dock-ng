# R8d: Idle memory growth of the dock (150-minute nested session)

Status: In progress. The 150-minute run started at 2026-11-06 10:06 (+08:00). This
draft is committed so that the work survives an interruption; the results and the
verdict are filled in when the run ends.

Batch / slice and selected scope: R8d, measurement only. No production source, CMake,
ledger or qmllint baseline change. Deliverables: the tools in `R8d-tools/`, the data in
`R8d-data/` and this record.

Source baseline and start HEAD: `6bdd43086` (local `main`, documentation on top of the
Qt 6.7.0 / KF6 6.10.0 floor raise). Branch: `claude/r8d-idle-memory-ee7a3e`, worktree
`/data/projects/latte-dock-ng/.claude/worktrees/r8d-idle-memory-ee7a3e`. Commits:
`cc87be538` (tools), `7f14dbcb1` (analysis fixes); the data and record commit follows.
Nothing is pushed.

Dependencies and evidence: `docs/architecture-assessment-2026-10.md`, Performance
findings, "Memory" row (567 MiB RSS and 265 MiB anonymous after 58 minutes on hardware);
`docs/modernization-results/P0.md` (PSS rising during its samples). Neither measured
growth over a long idle period. This run does, for one scenario.

## Question and scenario

Does the dock's memory keep growing while it idles? One run: a Release dock idles for
150 minutes in a nested, headless KWin session with three idle qml6 windows in the task
list. There is no user input and no click. The desktop's own dock (`/usr/bin/latte-dock-ng`,
pid 4760) is sampled read-only in parallel as the desktop series. The first 15 minutes
are warm-up and are dropped from the verdict.

## Method

- `R8d-tools/launch.py` builds the nested session: a private session bus from
  `dbus-run-session` with no service directories, a scratch HOME under the run root,
  a private `TMPDIR` and a short private `XDG_RUNTIME_DIR`, and
  `kwin_wayland --virtual --width 1920 --height 1080 --socket wayland-r8d --no-lockscreen --no-global-shortcuts`.
  The dock is the scratch HOME's user-mode install of this checkout
  (`home/.local/bin/latte-dock-ng`), started without `--debug` and without `--replace`.
  `dev-env.sh` is not sourced: install.sh calls it development-only and says the binary
  finds its own QML path. The desktop's `XDG_CONFIG_DIRS` and `XDG_DATA_DIRS` are kept for
  every nested process, as the logout simulation keeps them, so all of them read one
  KSycoca database. No desktop display, bus or session manager reaches the session.
- `R8d-tools/memsample.py` (standard library only, read-only, `/proc` plus two read-only
  queries): every 30 s one CSV row per watched process with the smaps_rollup sizes, the
  status sizes, the fd count and the CPU ticks; at 0, 15 and 75 minutes and at the end a
  snapshot with the 15 largest mappings by Pss, the totals per object and the GPU memory
  from `nvidia-smi`. It stops on the duration, on the stop file, or when a watched process
  exits, and then counts the new latte-dock-ng coredumps.
- `R8d-tools/analyze.py`: the warm-up is dropped; samples are reduced to 5-minute medians
  (a bucket needs at least 5 of its 10 samples); each metric's least-squares slope against
  hours has a 95 % confidence interval (Student t, n-2 degrees of freedom), for the whole
  post-warm-up window and for its first and second halves. The verdict rule for the dock's
  Pss_Anon was fixed before the run:
  - "no growth" when the whole-run slope interval includes 0 or its upper bound is below
    1 MiB/h;
  - "plateau" when the first-half slope exceeds 1 MiB/h and the second-half slope does not;
  - otherwise "growth", with the hours the slope needs to add 100 MiB.
- `R8d-tools/test_analyze.py`: 14 unit tests (synthetic flat, linear and plateau series,
  the t quantiles, the mapping comparison, and the parsers of /proc and nvidia-smi text).
  It is run by hand and is not registered in CTest.

## Environment (checked on the host)

| item | value | source |
|---|---|---|
| Host | Gentoo, kernel 6.18.57-gentoo-dist, 32 CPUs, 93 GiB RAM, `/tmp` on tmpfs (16 GiB) | `uname`, `nproc`, `free`, `df` |
| Dock under test | `lattedock 1.2.59` from this checkout (`CMakeLists.txt` `VERSION 1.2.59`), Release | `--version` in the scratch HOME; `CMakeCache.txt` |
| Compiler | GNU 16.2.1 (`/usr/bin/c++`), CMake 4.3.4, no warning in the Release build log | `CMakeFiles/*/CMakeCXXCompiler.cmake`; `install.log` |
| Qt | 6.11.2 (`libQt6Core.so.6.11.2`) | `ldd` of the dock, resolved (`session/versions.txt`) |
| KF6 | 6.29.0 (e.g. `libKF6CoreAddons.so.6.29.0`) | same |
| Plasma | `libPlasma.so.7` → 6.7.5 | same |
| KWin | 6.7.5 (`kwin_wayland --version`) | `session/versions.txt` |
| GPU | NVIDIA GeForce RTX 5080, driver 595.104.02 | `nvidia-smi` |
| Python | 3.14.10 | `python3 --version` |
| Desktop series | `/usr/bin/latte-dock-ng` pid 4760; its executable is a deleted inode (`/proc/4760/exe` shows "(deleted)"); package `latte-dock-ng-1.3.0` from `ruizhi-overlay` with `CMAKE_BUILD_TYPE="RelWithDebInfo"` in the package environment | `/proc`, `/var/db/pkg` (read only) |

The nested KWin does not render without a driver on this host. `nvidia-smi` lists the
nested `kwin_wayland` (29 MiB) and the nested dock (28 MiB) as GPU clients, so the
session renders on the NVIDIA GPU. The premise that the nested KWin "renders without a
real GPU driver" therefore does not hold here. The consequences are recorded in the
Limitations: the dock's RSS includes the driver's user-mode mappings (the mapping table
shows `/dev/nvidiactl`), video memory is not in RSS, and it is sampled only at the four
snapshot times.

## Commands and exit status

Commands are run from the worktree unless noted. `$R8D` is the run root
`/tmp/r8d-20261106-095302`.

| step | command | exit |
|---|---|---|
| build | `env -u DBUS_SESSION_BUS_ADDRESS -u WAYLAND_DISPLAY -u DISPLAY HOME=$R8D/home TMPDIR=$R8D/tmp XDG_CACHE_HOME=… XDG_CONFIG_HOME=… XDG_DATA_HOME=… XDG_STATE_HOME=… bash install.sh --user --jobs 8 --build-dir $R8D/build-user Release` (log `$R8D/install.log`, last line `exit=0`) | 0 |
| syntax | `python3 -m py_compile` on the three tools | 0 |
| sampler check | `memsample.py` on a 15 s `sleep` process | 0 |
| smoke 1 | `launch.py` plus `memsample.py`, 2 min | sampler 0, but it stopped at once (see Smoke tests) |
| smoke 2 | the same, 2 min, after the fix | launcher 0, sampler 0 |
| smoke 3 | the same, 30 s, with GPU memory per snapshot | launcher 0, sampler 0 |
| unit tests | `python3 docs/modernization-results/R8d-tools/test_analyze.py` | 0 (14 tests, OK) |
| run | `launch.py --root $R8D --max-min 170` (detached) | RUN_STATUS_PENDING |
| run | `memsample.py --out $R8D/series --watch-file nested-dock=… --watch-file nested-kwin=… --interval-s 30 --duration-min 150 --snapshot-min 0,15,75 --stop-file $R8D/stop --finish-touch $R8D/stop` (detached) | SERIES_STATUS_PENDING |
| run | `memsample.py --out $R8D/desktop-series --watch desktop-dock=4760 --interval-s 30 --duration-min 150 --snapshot-min 0,15,75` (detached, read-only) | DESKTOP_STATUS_PENDING |
| analysis | `python3 -I docs/modernization-results/R8d-tools/analyze.py --nested R8d-data/nested --desktop R8d-data/desktop --out R8d-data --warmup-min 15` | ANALYSIS_STATUS_PENDING |

## Results

RESULTS_PENDING

## Smoke tests

1. **Smoke 1 (2 min) found a sampler bug.** The nested KWin carries `cap_sys_nice`
   (`getcap /usr/bin/kwin_wayland`), so it is non-dumpable: `/proc/<pid>/smaps_rollup`,
   `smaps`, `fd` and `exe` are hidden from this user, while `stat` and `status` are
   readable. The sampler treated the permission error as an exit, so it stopped before
   writing the dock's row. Fixed in `cc87be538`: a permission error leaves the cells
   empty and the sampler reports it once; the analysis falls back to the status fields for
   the KWin control and labels them as RSS.
2. **Smoke 2 (2 min) passed:** 13 ticks, the KWin hidden fields reported as unreadable, no
   new coredumps, and the stop file led to a clean teardown (dock exit status 0, the
   qml6 windows and KWin terminated, no process left).
3. **Smoke 3 (30 s) checked GPU memory per snapshot:** KWin 29 MiB, the dock 0 MiB at the
   0-minute snapshot (no GPU context yet) and 28 MiB at the end, matching `nvidia-smi`.

## Limitations

- One scenario, one run, 150 minutes, no user input and no clicks, three idle qml6
  windows. Only the dock's idle behaviour is measured.
- The control's smaps is hidden (non-dumpable KWin), so the KWin control has RSS fields
  from the status file (RssAnon, RssFile, VmRSS), not PSS, and no mapping table.
- Video memory is read with `nvidia-smi` at the snapshot times only (0, 15, 75 and end).
- The desktop series comes from a different build (package `RelWithDebInfo`; the running
  executable has been replaced on disk). Compare its slope, not its absolute level.
- The desktop dock was read only; nothing was signalled, attached to, traced or restarted.
- The nested session runs on the host's GPU and the desktop's theme settings
  (`XDG_CONFIG_DIRS` is inherited and only read), so it is not a clean-room measurement.
- Debian 13 verification: deferred by the user until the refactor is complete.

## Next exact action

NEXT_ACTION_PENDING
