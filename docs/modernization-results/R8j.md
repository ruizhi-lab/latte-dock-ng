# R8j: allocator retention or leak in the dock after desktop activity

Status: In progress. The design in this section was fixed and committed before the first
run of any cycle. The results and the verdict are added after the runs.

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
