# R8d: Idle memory growth of the dock (150-minute nested session)

Status: Complete (measurement). The 150-minute nested run and the read-only desktop
series both ran to their duration, from 2026-11-06 10:06:31 to 12:36 (+08:00). The
verdicts are in the Results section and the data are in `R8d-data/`. Nothing is pushed.

Batch / slice and selected scope: R8d, measurement only. No production source, CMake,
ledger or qmllint baseline change. Deliverables: the tools in `R8d-tools/`, the data in
`R8d-data/` and this record.

Source baseline and start HEAD: `6bdd43086` (local `main`, documentation on top of the
Qt 6.7.0 / KF6 6.10.0 floor raise). Branch: `claude/r8d-idle-memory-ee7a3e`, worktree
`/data/projects/latte-dock-ng/.claude/worktrees/r8d-idle-memory-ee7a3e`. Commits:
`cc87be538` (tools), `7f14dbcb1` (analysis fixes), `f51279666` (draft record), and the
commit that adds this completed record and the data in `R8d-data/`. Nothing is pushed.

Dependencies and evidence: `docs/architecture-assessment-2026-10.md`, Performance
findings, "Memory" row (567 MiB RSS and 265 MiB anonymous after 58 minutes on hardware);
`docs/modernization-results/P0.md` (PSS rising during its samples). Neither measured
growth over a long idle period. This run does, for one scenario.

## Question and scenario

Does the dock's memory keep growing while it idles? One run: a Release dock idles for
150 minutes in a nested, headless KWin session with three idle qml6 windows open. Whether the dock's
task list shows them was not checked. There is no user input and no click. The desktop's own dock (`/usr/bin/latte-dock-ng`,
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
  every nested process, as the logout simulation keeps them. install.sh's comment on
  dev-env.sh says that a shorter value made the dock and KWin rewrite the shared KSycoca
  database against each other, so every nested process gets the same value. No desktop
  display, bus or session manager reaches the session.
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

On this host the nested KWin does not render in software. `nvidia-smi` lists the
nested `kwin_wayland` (29 MiB) and the nested dock (28 MiB) as GPU clients, so the
both render on the NVIDIA GPU. The task's premise that the nested KWin "renders without
a real GPU driver" does not hold here. The consequences are recorded in the Limitations:
the dock's RSS includes the driver's user-mode libraries (`libnvidia-glcore` and
`libnvidia-gpucomp` in the mapping tables), video memory is not in RSS, and it is
sampled only at the four snapshot times.

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
| run | `launch.py --root $R8D --max-min 170` (detached) | 0 (`session ended with status 0` at 12:36:36, after the stop file) |
| run | `memsample.py --out $R8D/series --watch-file nested-dock=… --watch-file nested-kwin=… --interval-s 30 --duration-min 150 --snapshot-min 0,15,75 --stop-file $R8D/stop --finish-touch $R8D/stop` (detached) | not captured (started detached); `summary.json` reason "duration reached", 301 ticks, `done` written, no process left |
| run | `memsample.py --out $R8D/desktop-series --watch desktop-dock=4760 --interval-s 30 --duration-min 150 --snapshot-min 0,15,75` (detached, read-only) | not captured (started detached); `summary.json` reason "duration reached", 301 ticks, `done` written, pid 4760 alive throughout |
| analysis | `python3 -I docs/modernization-results/R8d-tools/analyze.py --nested docs/modernization-results/R8d-data/nested --desktop docs/modernization-results/R8d-data/desktop --out docs/modernization-results/R8d-data --warmup-min 15` | 0 (the run directory and the committed copy give identical `analysis.md`; `analysis.json` differs only in directory paths) |

## Results

**Answer.** The nested dock does not grow while it idles. Its anonymous memory, RSS and
CPU counter were constant from the first minute to the end of the 150-minute run. The
desktop dock, in a live session without controlled input, rose by 2.45 MiB after warm-up
in three steps. The fixed rule classifies that series as "no growth", but its second-half
slope exceeds the 1 MiB/h bound, so it is recorded as a watch item, not as a finding of
growth and not as a clean bill of health.

Window: the first 15 minutes are dropped; 27 five-minute buckets cover the 2.25 h
post-warm-up window. Slopes are in KiB/h with 95 % intervals (Student t). The full tables
are in `R8d-data/analysis.md`, generated by `analyze.py` from the committed CSVs and
snapshots.

### Verdicts (Pss_Anon, fixed rule)

| series | whole-run slope, KiB/h (MiB/h) | 95 % CI, KiB/h | first half, MiB/h | second half, MiB/h | verdict |
|---|---|---|---|---|---|
| nested-dock (dock under test) | 0 (0.000) | [0, 0] | 0.000 | 0.000 | no growth |
| nested-kwin (control; RssAnon, see below) | 0 (0.000) | [0, 0] | 0.000 | 0.000 | no growth |
| desktop-dock (live session) | 704 (0.687) | [454, 954] | 0.898 | 1.397 | no growth by the rule; second half above 1 MiB/h |

The desktop slope is statistically positive (its CI excludes 0), but the upper bound
(0.93 MiB/h) is below the rule's 1 MiB/h, so the rule says "no growth". At the whole-run
point slope, +100 MiB would take about 145 h. A straight line describes this series
poorly (next section).

Other metrics: for the nested dock, Rss and RssAnon are flat; Pss_File moves by about
0.08 MiB/h (CI [0.02, 0.13]). Its RSS is constant, so the dock's own mappings did not change; the movement is
consistent with the PSS share of shared-library pages being redistributed as other
processes map them. The desktop Pss_File has the same slope (0.08 MiB/h, CI [0.02, 0.13]) and its
Rss moves with Pss_Anon (same slope, 704 KiB/h).

### Levels and CPU (MiB; bucket medians)

| series | Pss_Anon first to last (min, max) | Rss first to last | mean CPU over the post-warm-up window |
|---|---|---|---|
| nested-dock | 71.48 to 71.48 (71.48, 71.48) | 272.35 to 272.35 | 0.00 % |
| nested-kwin | RssAnon 77.32 to 77.32 | VmRSS 362.81 to 362.81 | 0.16 % |
| desktop-dock | 416.71 to 419.16 (416.71, 419.16) | 695.21 to 697.66 | 0.24 % |

The nested dock's RSS rose from 170.6 MiB at its first sample to 272.4 MiB within 30 s and
then stayed there. Its CPU counter was 48 ticks at 30 s and at every later sample, so it
used no measurable CPU for the remaining 2.5 h. That is consistent with no KSycoca rebuild
loop, but it also means the nested dock may be lighter than a user's dock (see Limitations).

### Desktop steps (raw Pss_Anon samples of the desktop series)

| wall time (+08:00) | elapsed | change | inside the rule's window |
|---|---|---|---|
| 10:11:01 | 4.5 min | +6.30 MiB (410.34 to 416.64) | no (warm-up) |
| 10:13:01 and 10:14:31 | 6.5 and 8.0 min | +0.26 MiB, then -0.19 MiB | no (warm-up) |
| 10:30:01 | 23.5 min | +0.98 MiB (416.71 to 417.69), then +0.02 | yes |
| 10:53:31 and 10:55:31 | 47 and 49 min | +0.38 MiB, then -0.31 MiB (net +0.13) | yes, transient |
| 12:14:31 | 128 min | +1.32 MiB (417.84 to 419.16) | yes |

Between the steps the value is flat. The whole post-warm-up rise is +2.45 MiB, so the
linear slope is an average over discrete events. The largest event, at 12:14, falls in
the second half and is the main reason its slope exceeds 1 MiB/h. The series does not
show what caused the steps. It had no controlled input, and the session was shared with
other applications, so the steps are not attributed.

### Mappings, 15-minute snapshot to end (MiB)

| series | total Pss | Anonymous delta | largest contributors (delta Pss) |
|---|---|---|---|
| desktop-dock | 502.53 to 505.37 (+2.84) | +2.45 | `[heap]` +1.32 (1 mapping); `[anon rw-p]` +0.81 (107 mappings before and after); `/memfd:JSGCHeap:QtQml (deleted)` +0.31 (31 to 27 mappings); file-backed PSS shares: libnvidia-gpucomp +0.07, libnvidia-glcore +0.05, Qt and KF6 libraries +0.01 to +0.02 each |
| nested-dock | 124.66 to 125.06 (+0.40) | +0.00 | file-backed PSS shares only: libnvidia-gpucomp +0.08, libnvidia-glcore +0.05, Qt libraries +0.01 to +0.02 each |
| nested-kwin | no mapping table | n/a | smaps is hidden from this user (non-dumpable process), see below |

So the desktop dock's anonymous growth sits in the brk heap (`[heap]`) and in anonymous rw-p mappings (typically malloc arenas), with a
small part in the QML JS heap, and none of it in the file-backed mappings. The nested dock's
anonymous mappings did not change at all between 15 minutes and the end.

**KWin control.** Its smaps is hidden from this user, so it has no mapping table. Its status
fields over the same window are flat: VmRSS 371516 KiB, RssAnon 79176 KiB, RssFile 232568
KiB, RssShmem 59772 KiB, VmData 327952 KiB and 12 threads at both 15 minutes and the end.
Its CPU counter rose by 1261 ticks (about 12.6 s over 2.25 h, 0.16 % mean). The nested dock's
status fields over the same window are also flat: VmRSS 291976 KiB, RssAnon 73192 KiB, RssFile
181724 KiB, RssShmem 37060 KiB, VmData 177976 KiB, 5 threads, 56 descriptors and 48 CPU ticks.

### GPU memory at the snapshots (nvidia-smi, MiB; 0 / 15 / 75 / end min)

| series | 0 | 15 | 75 | end |
|---|---|---|---|---|
| desktop-dock | 204 | 206 | 199 | 200 |
| nested-dock | 0 | 28 | 28 | 28 |
| nested-kwin | 29 | 29 | 29 | 29 |

The nested dock had no GPU context at its 0-minute snapshot (about 1 s after the dock started). Video memory
of both nested processes stayed constant after that. The desktop dock's video memory stayed
between 199 and 206 MiB.

### Process events

- Nested: the sampler ended at its duration with 301 ticks (10:06:35 to 12:36:35). The stop
  file tore the session down at 12:36:36. The dock exited with status 0 on SIGTERM (its
  normal quit path), the qml6 windows and nested KWin were terminated, and no process was
  left. Zero new latte-dock-ng coredumps were listed.
- Desktop: ended at its duration (10:06:31 to 12:36:31) with pid 4760 alive throughout, and
  zero new coredumps.
- Nested dock log (`R8d-data/session/dock.log`): one line, "Latte Dock requested Qt Quick
  OpenGL rendering". No warning, fatal or assertion.
- Cross-checks: an independent least-squares computation reproduces the desktop slope
  (703.6 KiB/h), and `analyze.py` reproduces the analysis from the committed copy.
  `t(0.975, 25) = 2.0595` matches the t table.

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

- The nested dock used no measurable CPU after its first minute (its CPU counter did not
  move for 2.5 h), and its visible state was not verified: the session has no plasmashell,
  the dock ran without `--debug`, and nothing confirmed that its panels were mapped on the
  virtual output. It may be lighter than a user's dock with panels and an active task list,
  so the nested verdict covers this idle state only.
- The desktop series is a live session with no controlled input. Its steps cannot be
  attributed, and it is a different build (package `RelWithDebInfo`; the running executable
  has been replaced on disk). Compare its slope and steps with the nested series, not its
  absolute level.
- The desktop's own KWin is not sampled, so a session-wide drift on the desktop cannot be
  separated from the dock's own growth.
- One scenario, one run, 150 minutes, no user input and no clicks, three idle qml6
  windows (their presence in the dock's task list was not checked). Only the dock's idle
  behaviour is measured.
- The control's smaps is hidden (non-dumpable KWin), so the KWin control has RSS fields
  from the status file (RssAnon, RssFile, VmRSS), not PSS, and no mapping table.
- Video memory is read with `nvidia-smi` at the snapshot times only (0, 15, 75 and end).
- The desktop dock was read only; nothing was signalled, attached to, traced or restarted.
- The nested session runs on the host's GPU and the desktop's theme settings
  (`XDG_CONFIG_DIRS` is inherited and only read), so it is not a clean-room measurement.
- Debian 13 verification: deferred by the user until the refactor is complete.

## Provenance of the committed data

- `R8d-data/nested` and `R8d-data/desktop` are copies of the two sampler directories under
  the run root. `summary.json` and `events.log` are unchanged. The snapshots are
  `gzip -9 -n` compressed, and `analyze.py` reads the compressed form directly.
- The sampler and analysis that produced the run are `R8d-tools/memsample.py` and
  `analyze.py` as committed in `7f14dbcb1`. The only later change to `memsample.py` is the
  line terminator: the `csv` module wrote CRLF during the run, and the sampler now writes LF.
  The two `samples.csv` copies were converted to LF with `sed -i 's/\r$//'`. No value changed.
- `analysis.md` and `analysis.json` were produced from the committed copies; the
  run-directory analysis gives the same `analysis.md`.
- `R8d-data/session` holds the launcher log, the versions, the exit record, the dock log, and
  the compressed install and KWin logs. The smoke-run data are not committed; their results
  are above.

## Next exact action

Extend the read-only desktop series to 12 hours with the same tool and rule, to see whether
the three steps recur and how often. Use the desktop dock's current pid:

```bash
python3 -I docs/modernization-results/R8d-tools/memsample.py --out /tmp/r8d-desktop-12h --watch desktop-dock=$(pgrep -x latte-dock-ng | head -n 1) --duration-min 720 --snapshot-min 0,15,75
```

Then analyze it with the same warm-up and rule:

```bash
python3 -I docs/modernization-results/R8d-tools/analyze.py --nested /tmp/r8d-desktop-12h --out /tmp/r8d-desktop-12h/analysis --warmup-min 15
```

`--nested` takes any sampler directory; here it holds the desktop series. The sampler only
reads /proc, so it must not be pointed at a nested dock or at any process that is signalled.
