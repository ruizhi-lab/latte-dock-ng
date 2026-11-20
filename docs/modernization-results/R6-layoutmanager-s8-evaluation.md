# R6-S8: parabolic per-event invocation — measured, not implemented

Status: **Measurement only, no source change.** S8 was conditional on a profile
showing that the per-event name lookup matters. The profile shows the opposite:
the lookup is 0.59 % of what one move event costs.

## What S8 proposed

Cache the `QMetaMethod` per parabolic item so the hover path stops resolving
`parabolicMove(qreal,qreal)` by name on every pointer move
(`app/view/parabolic.cpp:114`), and the same for `parabolicEntered`
(`:173`, `:212`).

## Measurement

A temporary probe split one invocation into its parts and the whole call, on a
Debug build with `--debug`, while the user swept the pointer across the dock
icons for about ten seconds.

| Stage | p50 | p90 | mean | max |
| --- | --- | --- | --- | --- |
| `QMetaObject::indexOfMethod("parabolicMove(qreal,qreal)")` | 1.12 µs | 1.70 µs | 1.11 µs | 5.16 µs |
| `QMetaObject::method(index)` | 0.05 µs | 0.09 µs | 0.055 µs | — |
| whole invocation incl. the QML handler | 193.7 µs | 349.3 µs | 196.1 µs | 5.77 ms |

n = 7831 samples. Name resolution is **0.59 %** of one move event.

Caching the method would remove roughly 1.2 µs from a 196 µs call: 0.006 % of a
16.7 ms frame at 60 fps. That is below run-to-run noise and cannot be attributed
to the cache in any paired run.

The probes were removed; `git diff app/view/parabolic.cpp` is empty.

## Verdict

Do not implement S8. The measured cost of the thing it would cache is three
orders of magnitude below the event it sits in. Design §9 named exactly this
outcome as the reason S8 stays conditional ("only if the profile shows the
lookups matter"); they do not.

## What the measurement does point at

The 196 µs is almost entirely the QML handler that `parabolicMove` runs — the
parabolic scaling computation itself. Moving that computation to C++ is a
different candidate, listed separately in the R6 row of the plan
("parabolic scale computation in C++"), and it would be judged on its own
profile: this record only establishes that the dispatch around it is irrelevant.

Note that the numbers come from a Debug build, where the QML handler is
unoptimized; a Release run would shrink the total and the lookup proportion would
rise. Since the absolute saving stays around 1 µs either way, the verdict does
not depend on the build type, but a Release profile is what any future parabolic
work should start from.
