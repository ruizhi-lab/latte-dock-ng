# Parabolic scale computation in C++ — measured, not implemented

Status: **Evaluation only, no source change.** The candidate listed in the R6 row
("parabolic scale computation in C++") was profiled and is not worth building.
The measurement is also worth recording for a mistake made during it, see the
last section.

## What the candidate proposes

Move the per-move parabolic scale computation out of
`declarativeimports/abilities/definition/ParabolicEffect.qml` into C++. Each
pointer move over the dock reaches it through:

1. `Parabolic::onEvent()` invoking `parabolicMove(qreal, qreal)` on the hovered
   item (`app/view/parabolic.cpp:114`);
2. `ParabolicArea.qml` `onParabolicMove` → `calculateParabolicScales()`;
3. `ParabolicEffect::applyParabolicEffect()` — two loops over `_spreadSteps`,
   each calling `scaleForItem()` → `scaleLinear()`, building two JavaScript
   arrays and returning a JavaScript object;
4. two signals broadcast to the neighbouring items with those arrays.

With the default `spread = 19`, step 3 costs roughly twenty JS calls plus two
array and one object allocation per move.

## Measurement

A temporary `QElapsedTimer` probe around the `invokeMethod()` call in
`parabolic.cpp` recorded the whole QML handler. Pointer sweeps across the dock
produced:

| Build | n | p50 | p90 | p99 |
| --- | --- | --- | --- | --- |
| Debug (gcc-debug) | 958 | 196 µs | 349 µs | 608 µs |
| **Release (gcc-release)** | **958** | **184 µs** | **320 µs** | — |

A separate earlier probe split the dispatch itself: `indexOfMethod()` 1.11 µs and
`method()` 0.055 µs per call, i.e. about 1.2 µs of the total (recorded in
[R6-S8](R6-layoutmanager-s8-evaluation.md)).

## Verdict

Release is only about 6 % faster than Debug. Compiled optimisation therefore
barely touches this path, which rules out the premise of the candidate: the cost
is not in C++-side arithmetic or dispatch that a compiler could improve, it is in
QML interpretation, JavaScript allocation and the variant broadcasts to the
neighbouring items.

Moving the computation into C++ would have to replace the QML signals, the two
arrays and the object return with something the QML side still consumes, i.e. the
same engine boundary the current code crosses — the situation
[R6-S7](R6-layoutmanager-s7-evaluation.md) rejected for the creation path. It
would also rewrite the most visible interaction of the shell, which AGENTS.md
lists among the fragile areas.

If this path is ever optimised, the work should reduce what happens per move —
fewer broadcasts, a lower recomputation rate, or reusing the scale arrays —
rather than change the language the arithmetic runs in. That needs its own design
and a visual acceptance pass; it is not covered here.

## A mistake made during this measurement

While profiling the Release build, the dock was started from
`build/modernization/gcc-release/bin/latte-dock-ng` together with the QML modules
`dev-env.sh` points at, which belong to the Debug user installation. That mix left
window previews non-functional for the user. The run also showed the QML-side
probe never executing (`QML 样本: 0` against 11 643 C++ samples), which was itself
evidence that the QML layer was not healthy, and it should have stopped the run.

Recovery: the probe files were restored from their backups, the Release dock was
stopped, and the packaged `/usr/bin/latte-dock-ng` was started again through a
transient user unit. The user confirmed previews were back. Leftover state that
the mixed run had created was then removed:

- `~/.local/share/latte-dock-ng` (452 K, containing `kns-compat.stamp` and
  `qml-kns-compat`) — left behind by the user installation and still present after
  the earlier cleanup pass;
- test caches `~/.cache/qmlsmoketest` (16 M), `~/.cache/qmlplugindump`,
  `~/.cache/qmlobjectsharedenginetest`, `~/.cache/helpmenuqmltest`.

The lesson for any future profiling here: a Release binary must be paired with
its own QML module install, never with the Debug `dev-env.sh` overlay, and a probe
that records zero samples on one side of a boundary means the run is invalid
rather than that the path is idle.
