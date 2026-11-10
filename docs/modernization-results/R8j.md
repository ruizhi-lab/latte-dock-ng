# R8j: allocator retention or leak in the dock after desktop activity

Status: Complete (measurement). The design in this record was committed before the first
run of any cycle (`2e7325481`). Under the rule fixed in advance, the idle control is "not
reproduced", and the window and application cycles are "inconclusive": the trim returned
34 % and 43 % of the growth, and the rule needs 50 % for "allocator retention". The leak
branch is not met either. See [Results](#results).

Batch / slice and selected scope: R8j, measurement only (plan row R8j, after R8d). No
production source, CMake, qmllint or clang-tidy baseline changes. Deliverables: the tools
in `R8j-tools/`, the data in `R8j-data/` and this record.

## Question

R8d's 12-hour desktop series ([R8d](R8d.md#12-hour-desktop-series-2026-11-09)) held
+122.88 MiB of Pss_Anon after 4.33 h of use; 116.45 MiB came in ten steps, the three largest
within 30 s after an application start. The dock's memory did not change during 7.31 h
without use. Two explanations fit that record:

- **Leak:** the dock keeps live objects it no longer needs, or allocates new ones on each
  use, so its live memory grows with the number of uses.
- **Allocator retention:** glibc keeps freed memory after a burst of allocations (the
  thread spikes in R8d suggest per-thread malloc arenas), so the process holds memory it
  could give back. `malloc_trim(0)` returns that memory to the kernel.

This study tells them apart with a cycle that is repeated in a nested session, an
allocator probe, and a verdict rule fixed before the first run.

## Design (fixed before the runs)

### Build under test

- Current `main` at `04589b1e9` (the R8h merge is included), built as a Release user-mode
  install into a scratch HOME under `/data/tmp/r8j-20261110`:
  `install.sh --user --jobs 8 --build-dir <root>/build-user Release`, with HOME, TMPDIR,
  XDG_RUNTIME_DIR and the XDG cache, config, data and state directories set to the scratch
  tree. The desktop's bus, display and Wayland variables are unset for the build.
- Not the desktop dock: `/usr/bin/latte-dock-ng` (package 1.3.0, RelWithDebInfo) is never
  signalled, attached to, traced or restarted. R8d measured that package and it is not the
  build under test here.

### Nested session

The harness is `R8j-tools/session.py`. It follows `R8d-tools/launch.py`: a private session
bus from `dbus-run-session` with no service directories, a scratch HOME (a copy of the
installed template for each run), a private `TMPDIR`, a short private `XDG_RUNTIME_DIR`
under `/data/tmp`, and `kwin_wayland --virtual` 1920x1080 on its own socket. `XDG_CONFIG_DIRS`
and `XDG_DATA_DIRS` are kept from the desktop, as in R8d. The dock starts without `--debug`
and without `--replace`, with `QT_FORCE_STDERR_LOGGING=1` (its log is kept).

The dock also gets `LD_PRELOAD` of the allocator probe (`R8j-tools/r8j_trim.c`, a scratch
tool, not production code). Its constructor does nothing unless the program is named
`latte-dock-ng` and `R8J_LOG` is set, so the preview helper (`latte-dock-ng-preview`) that
the dock may start is not touched. `kernel.yama.ptrace_scope` is 1 on this host, so the
probe is preloaded rather than attached with gdb: gdb can only start a child, and the
dock must keep running across the cycles. Signals reach the nested dock by the pid that
the harness started (its `Popen` pid, checked against its start time), never by name.

- `SIGUSR2`: the probe writes glibc `mallinfo2()` counters (in-use bytes `uordblks`, free
  bytes `fordblks`, mmapped bytes `hblkhd`, arena size and `keepcost`) to `R8J_LOG`.
- `SIGUSR1`: the probe writes the counters, calls `malloc_trim(0)`, and writes the counters
  and the return value again. A helper thread waits in `sigwait()`, so no allocation runs
  in a signal handler. The thread adds one to the dock's thread count, as in every run.

The sampler is `R8d-tools/memsample.py`'s `Watch` class (read-only `/proc`), run every 5 s
on the nested dock: Rss, Pss, Pss_Anon, Pss_File, Pss_Shmem, Private_Dirty, Swap, the
status sizes, Threads, descriptors and CPU ticks. Every row goes to `samples.csv` with the
R8d column set.

### Cycles

Each scenario is one nested session with one dock process and runs the same sequence. The
cycles are fixed here; no cycle was tried before this design.

| scenario | cycle (about 40 to 45 s) | what it reproduces |
|---|---|---|
| I, idle control | 20 s with nothing started, then 20 s rest | nothing; shows the baseline and the probe do not add drift |
| W, window | start a `qml6` window with a new title, hold 20 s, SIGTERM the window's process, rest 20 s | a window open and close: a new task appears and goes |
| A, application | start `kcalc` on the nested Wayland socket, hold 20 s, SIGTERM, rest 20 s | a real application's start, window and icon, closest to R8d's application starts |

Sequence per scenario:

1. Warm-up of 12 min from the dock's start, no action (R8d's warm-up was 15 min; the
   idle series had settled by then).
2. Pre-trim baseline `A_pre`: the mean of the samples in the last 15 s of the warm-up.
3. `SIGUSR1` (trim), then `A_0`: the mean of the samples 15 to 25 s after the trim.
4. Cycles 1 to 10 as above. After each cycle's rest: `SIGUSR2`, then `A_k`, the mean of the
   samples in the last 15 s of the rest.
5. `SIGUSR1` (trim), then `T`: the mean of the samples 15 to 25 s after the trim.
6. Post-trim cycles 11 and 12, the same as cycles 1 to 10 (their `A_11` and `A_12`
   test whether the trimmed memory is reused).
7. Teardown: SIGTERM to the dock (its normal quit path), then the windows and KWin.

Each scenario runs in a fresh session, so the scenarios do not share arenas or caches.

### Measured quantities

Per cycle k (MiB, Pss_Anon from `smaps_rollup`): `A_k`; `P_k`, the peak of Pss_Anon from
the start of the cycle to the end of its rest; the thread count peak; `U_k` and `F_k`
(glibc `uordblks` and `fordblks` from the dump after the rest, in MiB).

For the scenario: `G = A_10 - A_0` (growth over the ten cycles from the trimmed baseline);
`TR = A_10 - T` (what `malloc_trim(0)` returned); `dU = U_10 - U_0` and `dF = F_10 - F_0`
(glibc in-use and free bytes); `n_up` is the number of cycles with `A_k - A_(k-1) >= 0.5`
MiB; `S_late = A_10 - A_3`; a cycle reproduces a step when `P_k - A_(k-1) >= 1` MiB.

### Verdict rule (fixed)

Apply the first row that matches:

1. `G < 5 MiB`: **not reproduced**. No sustained growth in this scenario.
2. `TR >= 0.5 G`: **allocator retention**. The trim gives back at least half of the growth,
   so it was memory the allocator held. Pattern, reported with it: growth only in the first
   cycles when `S_late < 1 MiB`; growth on most cycles when `n_up >= 8`.
3. `TR < 0.25 G` and `dU >= 0.5 G`: **live growth (leak candidate)** when `n_up >= 8`;
   otherwise **live growth that stopped** (a one-time fill that is still in use).
4. `TR < 0.25 G` and `dU < 0.5 G`: **outside malloc**. The growth is in anonymous memory
   that glibc does not account for (for example the QML JS heap in memfd mappings or thread
   stacks); this rule does not decide it.
5. Anything else: **inconclusive**.

Why the trim decides before the pattern: growth on every cycle looks like a leak, but if
the trim returns it, the memory is free and reusable, so it is retention even when it
accumulates. The pattern is only a description. The post-trim cycles are a supporting
check, not part of the rule: retention predicts `A_11 - T >= 0.5 TR` (the trimmed memory
is reused), and a leak predicts little reuse of the released part.

Thresholds: 5 MiB for `G`, 1 MiB for a step, 0.5 MiB for a growing cycle, 8 of 10 cycles
for "most", 3 cycles for "first", 1 MiB for `S_late`, and 0.25 and 0.5 for the trim
fractions. A rule that cannot be evaluated (a missing dump or sample) is reported as such.

### Limits fixed in advance

- Synthetic cycles: the harness starts the windows and the application; no pointer or
  keyboard input reaches the dock. Hovering a task (the user's `PreviewWindows` setting)
  is not in this design. It is added only as a recorded amendment, and only if the pilot
  shows that the pointer can reach a task without input devices. The R8d desktop steps cannot be matched to a particular
  action, so "reproduced" means reproduced by this cycle only.
- Fresh HOME with the default layout, not the user's configuration. The user's dock sets
  `hoverAction=PreviewWindows` for its task applets (`~/.config/latte/我的布局.layout.latte`),
  which the cycles do not exercise.
- One build, one host (Gentoo, NVIDIA GPU, nested KWin on the GPU, as in R8d).
- Ten cycles per scenario; the 12-hour series had many more uses over hours. A rule that
  says "not reproduced" means this cycle did not reproduce the steps, not that they never
  happen.
- `mallinfo2` covers glibc's malloc arenas only. Thread stacks, QML memfd heaps and file
  mappings are outside it.
- heaptrack and valgrind are not installed; installing them needs root, which is the
  user's step. They are needed only if the verdict is "live growth" or "inconclusive".

## Pilot runs (not counted)

Six short nested sessions ran before the formal runs, to check the harness and the
configuration. No pilot is in a verdict. Their screenshots and summaries are in
`R8j-data/pilot/`.

| run | scenario | settings | result |
|---|---|---|---|
| W1 | W | warm-up 60 s, 1 cycle, 1 post-trim cycle | the qml6 window is on the nested output, but the dock shows only its launchers: no task for it (`R8j-data/pilot/W1-window-no-task.png`) |
| W2 | W | the same | still no task; the copied desktop file still named the template's binary (see amendment 1) |
| W3 | W | no `--debug`; Qt logging rules set | three lines in the dock log: Latte drops its debug messages without `--debug` (`app/main.cpp`) |
| W4 | W | `--dock-arg=--debug`, warm-up 15 s, screenshot in cycle 1 | the window's task appears in the dock; KWin logs `authorized … org_kde_plasma_window_management` for the run's binary |
| W5 | W | the formal configuration (no debug, no screenshot), warm-up 15 s, screenshot in cycle 1 | the window's task appears (`R8j-data/pilot/W5-window-task.png`); exit 0, no coredump |
| A1 | A (kcalc) | the formal configuration, warm-up 15 s, screenshot in cycle 1 | KCalc's window and its task with the KCalc icon appear (`R8j-data/pilot/A1-kcalc-task.png`); exit 0, no coredump |

W4 also started without a session once: argparse read `--dock-arg --debug` as two
options. The value is passed as `--dock-arg=--debug` from then on. The probe worked in
W1: `SIGUSR2` wrote the counters, and `SIGUSR1` returned 1 from `malloc_trim(0)`, with
`fordblks` 6.24 MB to 4.01 MB and the arena 94.0 MB to 91.8 MB (a pilot, not a result).

### Amendments before the formal runs

The design above is unchanged except where this list says so. The verdict rule and its
thresholds are unchanged.

1. **Task list.** KWin gives the dock its window-management interface only to the
   executable that the desktop file names in its `Exec` line
   (`X-KDE-Wayland-Interfaces`; the comment in `install.sh` says the same). The copied
   desktop files named the template's binary, and the copied KSycoca cache recorded the
   template's paths, so the copy's dock got no task list (W1, W2). `session.py` now
   rewrites the copy's `org.kde.latte-dock.desktop` and its preview helper's desktop file to
   the run's binaries (`localize_desktop_files`) and rebuilds KSycoca once
   (`rebuild_ksycoca`: `kbuildsycoca6 --noincremental`, its status is in
   `session/ksycoca.txt`). The first version of the rewrite kept a newline in the `Exec`
   value and changed nothing; W4, W5 and A1 use the fixed version.
2. **Application.** `kcalc` (scenario A) opens its window and its task. Nothing changed.
3. **Pilot-only options.** `--dock-arg` and `--shot-cycle` / `--shot-warm` are for the
   pilots. The formal runs pass none of them, so they take no screenshot and no debug
   messages.
4. **Trim after the cycles.** The harness always sends it after cycle 10, also when a
   run has no post-trim cycles (the pilots with `--post-cycles 0` did not). The formal
   sequence is the one in the design: the trim, then cycles 11 and 12.

### Limits found in the pilots

- No PipeWire daemon runs in the nested session: `kwin_screencast: Failed to connect
  PipeWire context` is in each KWin log. So the preview helper (`latte-dock-ng-preview`)
  cannot capture window thumbnails here. The helper is a separate process, which this
  study does not measure. Hover is not in the design, so no preview is exercised.
- With `--debug` (W4) the dock logs one startup warning, "Failed to register with host
  portal …", because no portal service is on the private bus. Without `--debug` the dock
  log has no warning line (W1, W5, A1 each have three lines). No fatal message, assertion
  or coredump appeared in any pilot.

## Environment (checked on the host, 2026-11-10)

| item | value | source |
|---|---|---|
| Host | Gentoo, kernel 6.18.62-gentoo-dist, 16 CPUs, 31 GiB RAM | `uname -r`, `nproc`, `free -g` |
| GPU | NVIDIA GeForce GTX 1080 Ti, driver 580.178.04 | `nvidia-smi` |
| Same host as the 12-hour series | yes: R8d's 12-hour record lists these CPUs, RAM, GPU, driver, Qt 6.11.3, KF6 6.31.0 and Plasma 6.7.6 | `R8d.md`, "Run and environment" |
| Toolchain | GCC 16.2.1, CMake 4.3.4, glibc 2.43 (`mallinfo2` available), Python 3.14.10 | `gcc --version`, `cmake --version`, `ldd --version`, `python3 --version` |
| Qt, KF6, Plasma, KWin | Qt 6.11.3, KF6 6.31.0, Plasma 6.7.6, KWin 6.7.6 | `libQt6Core.so.6.11.3`, `libKF6CoreAddons.so.6.31.0`, `libPlasma.so.6.7.6`, `kwin_wayland --version` |
| Yama | `kernel.yama.ptrace_scope` = 1 | `/proc/sys/kernel/yama/ptrace_scope` |
| Dock under test | `lattedock 1.2.59`, Release user-mode install of `main` at `04589b1e9`, in the scratch HOME `/data/tmp/r8j-20261110/home`; the binary's SHA-256 is `0bae01a0942656526ac3a4b9fdde0e0efd408af13478497fbf7583b9fb8e2fed` | `--version`, `CMakeCache.txt`, `sha256sum` |
| Probe | `r8j_trim.so`, SHA-256 `d9c3b60c550bce59e4dc3b419b3bda7c4dd64eb33a76a3323ca5f4f25a745f58` | `sha256sum` |
| Desktop dock | `/usr/bin/latte-dock-ng` (pid 310593 at the start of the runs) was never signalled, traced or restarted; the runs address only the nested dock's `Popen` pid | `ps`, `session.py` |
| Missing tools | heaptrack and valgrind are not installed (not needed unless the verdict asks for them) | `command -v` |

## Commands and exit status

Commands are run from the worktree at `b9ef20acf` (the tools) unless noted. `$R8J` is
`/data/tmp/r8j-20261110`.

| step | command | exit |
|---|---|---|
| build and install | `env -u DBUS_SESSION_BUS_ADDRESS -u WAYLAND_DISPLAY -u DISPLAY HOME=$R8J/home TMPDIR=$R8J/tmp XDG_RUNTIME_DIR=$R8J/runtime XDG_CACHE_HOME=$R8J/home/.cache XDG_CONFIG_HOME=$R8J/home/.config XDG_DATA_HOME=$R8J/home/.local/share XDG_STATE_HOME=$R8J/home/.local/state bash install.sh --user --jobs 8 --build-dir $R8J/build-user Release` (log `$R8J/install.log`, last line `exit=0`) | 0 |
| build warnings | the install log has 183 compile lines and no compiler warning. Its three lines with "warning" are "Strict compiler warnings enabled" (information), and two install messages that R8d's log has too: `dialog.h:25: PlasmaQuick::Dialog is used as base type but cannot be found`, printed during the automatic QML type registration of `lattecoreplugin` (a registration-tool message, not a compiler diagnostic; not investigated here), and `uninstall manifests not found` | (informational) |
| probe | `gcc -std=gnu11 -O2 -Wall -Wextra -Werror -fPIC -shared -o $R8J/tools/r8j_trim.so r8j_trim.c -lpthread`; the same with `clang` (Clang 22) | 0, 0 |
| unit tests | `python3 -I docs/modernization-results/R8j-tools/test_analyze_cycles.py` (17 tests) | 0 |
| pilots | six sessions, listed under Pilot runs | not counted |
| formal runs | `setsid -f nohup bash docs/modernization-results/R8j-tools/run_scenarios.sh $R8J $R8J/tools/r8j_trim.so $R8J/home` (status in `$R8J/runs/status.txt`) | see below |
| formal I | `session.py --scenario I --out $R8J/runs/I --template $R8J/home --probe $R8J/tools/r8j_trim.so`, then `analyze_cycles.py $R8J/runs/I` (started 03:29:16, both done 03:50:10 +08:00) | session 0, analysis 0 |
| formal W | the same with `--scenario W` (done 04:11:04) | session 0, analysis 0 |
| formal A | the same with `--scenario A` (done 04:31:58) | session 0, analysis 0 |
| run driver | `run_scenarios.sh` writes these lines to `$R8J/runs/status.txt`: `session I/W/A exit=0`, `analysis I/W/A exit=0`, `done at 2026-11-10T04:31:58+08:00` | 0 |
| dock exit | each formal session sends SIGTERM to the nested dock; the dock exits with status 0 in all three (`summary.json`, `dock_status`); KWin's recorded status is 15; no new latte-dock-ng coredump (`coredumps_new` empty) | 0 |
| data copy | `cp` of `samples.csv`, `events.jsonl`, `summary.json` and `session/` into `R8j-data/<scenario>/` | 0 |
| analysis from the committed copy | `python3 -I docs/modernization-results/R8j-tools/analyze_cycles.py docs/modernization-results/R8j-data/<scenario>` for I, W and A; the `analysis.md` differs from the run directory's only in its `Run:` path line | 0, 0, 0 |
| unit tests after the runs | `python3 -I docs/modernization-results/R8j-tools/test_analyze_cycles.py` | 0 |

## Results

Units: MiB of Pss_Anon, from `smaps_rollup`, and glibc's `mallinfo2` counters (`uordblks`
in use, `fordblks` free). Every number is from the committed `R8j-data/<scenario>/analysis.json`
and the sample file beside it.

### Answer

Under the rule fixed before the runs, the window and application cycles are
**inconclusive**, and the idle control is **not reproduced**. The three runs show:

1. The first window or application start after warm-up adds 4.9 to 5.2 MiB (cycle 1). It
   is the only step of at least 1 MiB in ten identical cycles. Each later cycle moves the
   level by about 1 MiB at most, and the later levels stay within a band of about 1.5 MiB
   (120.0 to 121.8 MiB for both scenarios).
2. Most of that growth is glibc's free memory. Free bytes rise by 4.2 to 4.4 MiB, while
   in-use bytes rise by only 0.8 to 1.0 MiB. Between cycles 1 and 10 the in-use bytes change
   by 0.5 MiB at most, rising and falling without a steady climb.
3. `malloc_trim(0)` after cycle 10 returns 1.9 MiB (window) and 2.4 MiB (application),
   which is 34 % and 43 % of the growth. The next start takes back 1.7 and 2.3 MiB within
   one 5-second sample. The fixed rule's "allocator retention" needs at least 50 %, so it
   does not decide, and neither does its leak branch (which needs in-use growth of at least
   half the growth, and a trim return below a quarter).
4. The baseline trim at the warm-up's end also returned 2.2 to 2.9 MiB, so the dock's
   startup leaves free memory of the same kind.

Reading: the data fit allocator retention of the first-use allocations, which are arenas and
free chunks made by the first window or application start, kept and reused by the allocator.
They do not fit live growth that accumulates per use within these cycles. This reading is not
the verdict. The rule's verdict is "inconclusive", and telling the first-use free memory apart
from live memory needs an attribution run (step 3 below). The step is also much smaller than
the desktop's: 5 MiB here, once, against 27 to 66 MiB on the desktop in R8d's series.

### Verdict by the fixed rule

| scenario | G = A_10 - A_0 | TR = A_10 - T | TR / G | dU (in use) | dF (free) | cycles with a growth of at least 0.5 MiB | late growth, cycles 4 to 10 | cycles with a step of at least 1 MiB | verdict |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---|
| I, idle control | 0.00 | 0.00 | - | 0.00 | 0.00 | 0 | 0.00 | 0 | not reproduced |
| W, window | 5.57 | 1.91 | 0.34 | 0.98 | 4.24 | 5 | -0.05 | 1 | inconclusive |
| A, application | 5.54 | 2.36 | 0.43 | 0.81 | 4.40 | 5 | -0.26 | 1 | inconclusive |

The rule's rows, in order: row 1 (G below 5 MiB) gives I. Rows 2 and 3 need TR of at least
0.5 G, or TR below 0.25 G with in-use growth of at least 0.5 G. Neither W nor A meets them.
Row 4 needs TR below 0.25 G, which neither meets. Row 5 gives W and A: "inconclusive".

Supporting check, not part of the rule: after the trim, cycle 11 takes back 2.00 MiB (W)
and 2.66 MiB (A), and cycle 12 takes back 1.41 and 1.96 MiB. Retention predicts at least
half of the trim's return in cycle 11, and both scenarios meet it.

### Per-cycle levels (Pss_Anon, MiB; step from the previous level in brackets)

| cycle | I, idle | W, window | A, application |
|---:|---:|---:|---:|
| 0, post-trim baseline | 115.25 | 115.05 | 115.15 |
| 1 | 115.25 [0.00] | 119.95 [+4.91] | 120.32 [+5.17] |
| 2 | 115.25 [0.00] | 120.03 [+0.08] | 120.33 [+0.01] |
| 3 | 115.25 [0.00] | 120.67 [+0.64] | 120.95 [+0.61] |
| 4 | 115.25 [0.00] | 121.37 [+0.70] | 121.61 [+0.66] |
| 5 | 115.25 [0.00] | 120.39 [-0.98] | 120.56 [-1.05] |
| 6 | 115.25 [0.00] | 120.42 [+0.02] | 120.56 [0.00] |
| 7 | 115.25 [0.00] | 120.96 [+0.55] | 121.16 [+0.59] |
| 8 | 115.25 [0.00] | 121.59 [+0.62] | 121.77 [+0.62] |
| 9 | 115.25 [0.00] | 120.62 [-0.97] | 120.69 [-1.09] |
| 10 | 115.25 [0.00] | 120.62 [0.00] | 120.69 [0.00] |
| after the trim T | 115.25 | 118.71 [-1.91] | 118.33 [-2.36] |
| 11, post-trim | 115.25 | 120.71 [+2.00 from T] | 120.99 [+2.66 from T] |
| 12, post-trim | 115.25 | 120.12 [+1.41 from T] | 120.29 [+1.96 from T] |

The `dF` column in the verdict table is the free-byte growth from the baseline to cycle 10;
the per-cycle counters are in each scenario's `analysis.md`.

### glibc counters (MiB, `uordblks` in use / `fordblks` free)

| point | I, idle | W, window | A, application |
|---|---:|---:|---:|
| post-trim baseline (after the warm-up trim) | 83.76 / 3.60 | 83.69 / 3.47 | 83.76 / 3.97 |
| after cycle 1 | 83.76 / 3.60 | 84.15 / 8.22 | 84.34 / 8.60 |
| after cycle 10 | 83.76 / 3.60 | 84.67 / 7.71 | 84.57 / 8.37 |
| after the trim T | 83.76 / 3.60 | 84.67 / 6.30 | 84.57 / 7.09 |

The trim leaves the in-use bytes unchanged and lowers the free bytes by 1.41 MiB (W) and
1.28 MiB (A). Pss_Anon falls by 1.91 and 2.36 MiB, so its drop is larger than the free-byte
drop by 0.5 and 1.1 MiB. These counters do not explain that difference.

The samples around the trims confirm that the measurement windows were valid. The release
is complete within 5 seconds, and the level stays flat until the next start: the window's
run and the application's run both regrow within one sample after the next start
(`samples.csv`, 5-second rows).

Threads: the idle control peaks at 8 threads in every cycle. The window cycles peak at 10
(9 in cycle 6). The application cycles peak at 10 in cycle 1 and at 8 from cycle 2.

### Limits of this result

- **Synthetic cycles.** The harness starts the windows and KCalc and holds them for 20 s.
  No pointer, keyboard or hover reaches the dock, so the user's `hoverAction=PreviewWindows`
  is not exercised. The nested session runs no PipeWire daemon, so no window thumbnail is
  captured (`kwin_screencast: Failed to connect PipeWire context`). The preview helper is a
  separate process that this study does not measure.
- **Size and duration.** Ten identical cycles after twelve minutes of warm-up do not repeat
  the desktop's hours of mixed use. The desktop's largest steps (27 to 66 MiB) are not
  reproduced; the largest nested step is 5 MiB, and it occurs once.
- **The trim's reach.** Both scenarios sit between the two trim thresholds (34 % and 43 %).
  malloc_trim releases only the free space that covers whole pages. The free bytes it
  leaves are interleaved with live chunks, so the 50 % line may be out of reach even for
  pure retention. The rule was fixed on TR relative to G, and it is not changed here. A new
  measure would need its own pre-registration.
- **Attribution.** glibc's counters do not say which code made the first-use allocations.
  heaptrack and valgrind are not installed here, and installing them needs root.
- **Outside the malloc counters.** About 0.3 MiB of each scenario's growth (5.57 - 4.24 -
  0.98 = 0.35 for W; 5.54 - 4.40 - 0.81 = 0.33 for A) is anonymous memory outside the glibc
  arenas, such as the QML memfd heaps or thread stacks.
- **One host and one configuration.** The host is the same as the 12-hour series, with a
  fresh scratch HOME, the default layout, and the locale inherited from the desktop (the menus
  are in Chinese). Levels differ from the desktop (idle 115 MiB here, 160 to 283 MiB on the
  desktop); compare slopes and steps, not levels.
- **Log levels.** Without `--debug` the dock writes no diagnostic lines for tasks, so the
  task-list evidence is the screenshots of the pilots (`R8j-data/pilot/`), not a log.

### Provenance

- Tools and design: `R8j-tools/` and this record are committed in `2e7325481` (design, before
  any run) and `b9ef20acf` (harness, probe, pilots and amendments, before the formal runs).
  The formal runs used the `b9ef20acf` version of `session.py` and `run_scenarios.sh`.
- Build: `main` at `04589b1e9`, Release, user-mode install into the scratch HOME; the
  dock's binary SHA-256 is in the environment table.
- Data: `R8j-data/I`, `R8j-data/W`, `R8j-data/A` (samples, events, summaries, the probe log,
  the logs and the analysis), and `R8j-data/pilot/` (three screenshots and two summaries).
  The analysis reproduces from the committed copies (command in the table above).

### Next exact action

The answer is open under the fixed rule, so the next step is step 3 of the plan, an
attribution run with heaptrack or valgrind. Neither is installed, and installing either needs
root, which is yours to run. For example, after finding the package name for your system:

```bash
emerge -s heaptrack
```

After it is installed, a heaptrack run of the same window cycle would show where cycle 1's
free bytes were allocated. That run is an attribution, so it needs a new pre-registered
design before it starts. A cheaper follow-up without root would trim after every cycle, to
see whether the free bytes release at all; that also needs its own design. If you prefer,
R8j can close here with the verdict "inconclusive; first-use retention likely" for the
window and application cycles.

## Step 3: attribution with heaptrack (design fixed before the run)

Status: the design below is fixed and committed before the formal attribution run. The
run and its results are added after it.

Why: the formal window run (`R8j-data/W`) has a first-cycle step of +4.91 MiB of Pss_Anon,
with +4.24 MiB of glibc free bytes and +0.98 MiB in use, and the trim returns 1.91 MiB
of it. The verdict rule cannot say what allocates that memory. Step 3 names the call
sites, so that the next decision (a leak candidate, or retention of a first-use burst)
rests on code, not on counters.

Tool. heaptrack from the Gentoo package `dev-util/heaptrack-1.5.80_pre20251017` (installed
on this host by the user; `heaptrack --version` prints 1.6.80), its preload
`/usr/lib64/heaptrack/libheaptrack_preload.so` and its reader
`/usr/lib64/heaptrack/libexec/heaptrack_interpret`. Valgrind is not installed and is not
used. The reader's output is read by `R8j-tools/heaptrack_attr.py`, which is checked
against a real recording of a program with known allocations (1 MiB freed, 3 MiB kept,
2 MiB freed; `test_heaptrack_attr.py`, ten tests, and the CLI on that recording).

Run. Scenario W, the same timeline and options as the formal W run (warm-up 720 s, the
baseline trim, cycles 1 to 10, the trim, post-trim cycles 11 and 12; hold 20 s; rest 20 s;
sampling every 5 s), the same build (`main`, Release, the scratch HOME copy) and the same
probe. The only addition is `session.py --heaptrack`: the dock is started with heaptrack's
preload before the probe, and its raw data go to a FIFO in the run directory, read by the
reader and compressed by zstd (the wrapper's pipeline, without the wrapper, so that the
dock keeps the pid that the harness signals). Output: `runs/WH`, copied to `R8j-data/WH`.

Controls, fixed in advance.

- C1 (overhead): the run is representative if its cycle-1 step (A_1 - A_0) is at least
  1 MiB and within 2 MiB of the formal W run's 4.91 MiB. Otherwise the attribution is
  reported and labeled "not representative of the formal run".
- C2 (heaptrack's own memory): the probe's counters and Pss_Anon include heaptrack's own
  buffers. Its effect is reported, not corrected.

Measurement. Each cycle k has a window [cycle start, rest end] in harness time; heaptrack's
time is taken as harness time from the dock's start (offset 0; the uncertainty is about
0.5 s, far below a 40 s window). For each site and window the tool reports:

- allocated bytes (allocations x size of the allocation's info) and freed bytes (frees x
  size), both exact per info and window;
- transient bytes, estimated as min(allocations, frees) x size inside the window, a lower
  bound per info, because heaptrack's format counts frees per info and does not match them to
  single allocations;
- net bytes = allocated - freed (the live change of the window, exact).

A site is the first three named frames of the allocation's stack, innermost first, with the
file and line of the first two, so that two calls of one function from different lines stay
apart.

Rules, fixed in advance.

- P1 (the source of window 1's churn). The attribution set is the smallest set of sites,
  by transient bytes in window 1, that covers at least 50 % of that window's transient bytes.
  The churn accounts for the first-cycle free growth if the attribution set exists and its
  transient bytes are at least 2 MiB (half of the formal run's 4.24 MiB of free growth).
- P2 (the live growth of window 1). The live sites are those with a positive net byte
  change in window 1. The live growth is accounted for if their total is at least 0.5 MiB
  (half of the formal run's 0.98 MiB in use).
- P3 (reuse in windows 2 to 10). Reuse holds when, in each window k from 2 to 10, the
  window-1 attribution set supplies at least 80 % of window k's transient bytes. New live
  growth is the positive net bytes of sites that are not in the window-1 set, summed over
  windows 2 to 10; it is a per-use candidate if that sum is at least 1 MiB.

Reading, fixed in advance.

- P1 holds, P3 holds and the new live growth is below 1 MiB: the first-cycle free growth comes
  from a fixed set of call sites that churn in every cycle and reuse the freed memory. That
  is the retention pattern, and the record names the sites. The rule of the verdict stays as
  it was: the step does not change the window and application verdicts.
- P3 fails, or the new live growth is at least 1 MiB: a per-use source exists. The record
  names those sites as leak candidates, and a further run needs its own design.
- P1 fails: the first-cycle free growth is not explained by transient churn of these sites.
  The record reports the sites that were found and stops there.
- Whatever the outcome, the record gives the top sites with their frames, the controls and
  the limits. The step proposes no production change; a fix would be a separate slice.

Limits, fixed in advance. One scenario and one run under heaptrack. The frees are counted
per info, so the transient bytes are an estimate. Heaptrack adds its own overhead to the
dock. The window is 40 s, so an allocation that starts in one cycle and is freed later is
counted as freed in the later cycle.
