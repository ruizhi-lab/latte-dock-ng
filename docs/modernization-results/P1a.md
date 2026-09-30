# P1a: Characterize title-event routing

Status: Complete; production behavior is unchanged. The final desktop retest is
still pending after all implementation batches.

Recorded main baseline: `e3ef1ddf001aa032cffa62db21cfd97196174746`.
Measurement source revision: `20a83a13a` plus the P1a-only trace markers in
`app/wm/tracker/windowstracker.cpp`.

## Current route

`WaylandInterface::trackWindow()` connects `PlasmaWindow::titleChanged` to
`updateWindowCache()`. That callback emits `AbstractWindowInterface::windowChanged`
for a valid window. `Tracker::Windows::init()` refreshes its cached
`WindowInfoWrap` through `requestInfo()`, restarts the single-shot 300 ms hint
timer, and emits its own `windowChanged`. `LastActiveWindow` listens to that
tracker signal and refreshes the selected window's display metadata when the
changed ID is in its active history.

Geometry, maximize/minimize, active, fullscreen, shaded, activity and virtual
desktop events use `updateWindowGeometry()` and
`AbstractWindowInterface::considerWindowChanged()`, whose 150 ms timer batches
general window changes before they enter the same full tracker path. Keep this
path unchanged. `skipTaskbarChanged`, `onAllDesktopsChanged` and
`parentWindowChanged` currently use `updateWindowCache()` too; they can affect
tracking eligibility, desktop visibility or parent-window grouping, so P1b
must leave them on the full path. The current connection list does not connect a
`skipSwitcherChanged` signal; P1b does not change that behavior.

`requestInfo()` can emit `windowRemoved()` while rejecting a blocked Plasma
window. A future title-only metadata handler must not insert an ID that was
removed during the refresh. It must re-check that the ID remains tracked after
the request, preserve immediate removal, and keep the full update path as the
fallback for invalid/stale windows and every non-title state change. A title
event must not cancel an already scheduled geometry update.

## Instrumented baseline

The opt-in `[perf-trace]` markers count calls to `updateAllHintsAfterTimer()` and
`updateAllHints()` without including window IDs, titles or geometry. They are
enabled through the `latteWm` logging category and must remain separate from
uninstrumented Release CPU/latency measurements. The reusable native Wayland
fixture is [windowtitleprobe.cpp](../../autotests/performance/windowtitleprobe.cpp).

On Fedora 44 (Qt 6.11.2, Plasma 6.7.5), a user-mode Debug build ran the fixture
with its window settled before measurement. Each trial changed only the
synthetic title ten times at 500 ms intervals; the window was closed after the
counter window. Five trials used a five-second quiet interval between windows.
All five trials requested ten title changes and produced ten schedules. Executions
included unrelated tracker work in two trials; preserve every run:

| Run | Title changes | Hint schedules | Hint executions |
| --- | ---: | ---: | ---: |
| 1 | 10 | 10 | 4 |
| 2 | 10 | 10 | 4 |
| 3 | 10 | 10 | 4 |
| 4 | 10 | 10 | 11 |
| 5 | 10 | 10 | 11 |

Raw numeric records are in [P1a-title-trace-2026-09-30.jsonl](P1a-title-trace-2026-09-30.jsonl).
The schedule count was stable at one per delivered title event. Execution count
had a median of 4, mean of 6.8 and range of 4–11; the two higher runs are not
excluded because the global trace has no event ID with which to separate
concurrent tracker work. A separate smoke trial observed 10 schedules and 3
executions. These Debug work counts do not imply a Release CPU percentage.

## P1b design and guards

Use a dedicated title-only signal from the Wayland adapter into the existing
tracker. Its production handler should refresh metadata for an already-tracked
ID and notify `LastActiveWindow` through the existing tracker notification,
without scheduling hint scans. Keep the production dispatch itself as the test
seam: a focused fake adapter should emit the title-only and full signals into
the real tracker handler; do not reproduce its routing algorithm in the test.
If that host fixture cannot be kept lightweight, use the narrow source contract
as a wiring check and keep the live window-state matrix explicitly pending.

Predeclared P1b work target: ten title-only notifications schedule zero hint
updates, down from ten in each characterized run. Preserve title freshness,
the 150/300 ms geometry debounce behavior, full updates for eligibility and
window state, queued geometry work, active-window immediacy, stale/deleted IDs
and `requestInfo()` removal side effects. Compare five same-sequence Release
runs for CPU/latency separately; if those values stay inside measurement noise,
report only the confirmed work-count reduction.

## Validation

- GCC and Clang Debug builds of `latte-dock-ng` and `sourcecontracttest` passed.
- `ctest --test-dir build/modernization/gcc-debug --output-on-failure -R '^sourcecontracttest$'` — passed (1/1).
- `ctest --test-dir build/modernization/clang-debug --output-on-failure -R '^sourcecontracttest$'` — passed (1/1).
- The C++ compiler emitted no lowercase `warning:` or `error:` diagnostics in the targeted build logs. CMake's build-aware QML lint emitted its existing dynamic Plasma-interface and unqualified-access diagnostics in unrelated QML files.
- The full runtime blocker matrix and final user feature retest remain pending; see [M5b](M5b.md).

## Handoff

Proceed to P1b using the title-only route and work target above. Keep
`updateWindowCache()` for all non-title cache changes. P0's hidden/hover/edge/
multi-window scenario evidence remains tracked separately in [P0](P0.md).
