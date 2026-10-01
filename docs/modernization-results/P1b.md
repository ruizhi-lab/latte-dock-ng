# P1b: Title-only fast path

Status: Deferred for long-term observation. Implementation and title-work
target are verified; the user reports functional acceptance passed. Exact
response latency and a measurable performance benefit remain unverified.

Recorded main baseline: `e3ef1ddf001aa032cffa62db21cfd97196174746`.
Implementation base: P1a commit `ec8a504db`.

## Change

Wayland title changes now emit a dedicated `windowTitleChanged` signal. The
window tracker refreshes its cached `WindowInfoWrap` and emits the existing
`windowChanged` notification so active-window metadata remains fresh. It
schedules the normal debounced hint scan only if a non-display field changed.
The comparison includes identity, parent, geometry, validity, active/minimized/
maximized/fullscreen/shaded and stacking state, task eligibility, abilities,
application name, icon cache key, desktop and activity membership.

The title handler checks tracking membership both before and after
`requestInfo()`. The second check is required because `requestInfo()` can emit
`windowRemoved()` synchronously for blocked windows; the handler must not
reinsert an ID after that authoritative removal. Non-title changes continue
through the existing full path. A title event does not stop or restart pending
geometry work.

The focused unit test covers title-only equality and independently changes
every non-display `WindowInfoWrap` field to verify that each requires the full
fallback. The source contract checks adapter-to-
tracker wiring, metadata notification, guarded removal and conditional hint
scheduling. It is a wiring check, not a live compositor dispatch test.

## Validation

- `cmake --build build-autotests-gcc -j8 --target wmunittest sourcecontracttest` — passed.
- `ctest --test-dir build-autotests-gcc --output-on-failure -R '^(sourcecontracttest|wmunittest)$'` — passed (2/2).
- `cmake --build build-autotests-clang -j8 --target latte-dock-ng sourcecontracttest wmunittest` — passed. The C++ build completed; build-aware QML lint emitted the repository's existing dynamic Plasma-interface and unqualified-access diagnostics.
- `ctest --test-dir build-autotests-clang --output-on-failure -R '^(sourcecontracttest|wmunittest)$'` — passed (2/2).
- The initial `build-autotests-gcc` directory was stale: it cached GCC 15.3, but `/usr/bin/g++` had since moved to GCC 16.2, so its generated Makefile omitted the GCC-16-only MOC suppression. A fresh configure with `cmake -S . -B /tmp/latte-p1b-gcc-debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBUILD_TESTING=ON -DLATTE_STRICT_WARNINGS=ON` detected GCC 16.2.1 and applied `-Wno-sfinae-incomplete` only to the generated MOC aggregate.
- `cmake --build /tmp/latte-p1b-gcc-debug -j8 --target latte-dock-ng sourcecontracttest wmunittest` — passed. The build-aware QML lint reported existing dynamic Plasma-interface and unqualified-access warnings; the C++ build completed without compiler warnings/errors.
- `ctest --test-dir /tmp/latte-p1b-gcc-debug --output-on-failure -R '^(sourcecontracttest|wmunittest)$'` — passed (2/2).
- `git diff --check` — passed. `formatter.sh` was applied to only the changed C++ ranges and the diff was reviewed; its generated formatting around existing function definitions was normalized back to the surrounding file style.

## Runtime work-count result

Five Fedora Wayland Debug trials each changed the title of a synthetic window
ten times. In every trial, the title-only measurement window produced zero new
hint schedules and zero executions; all probes exited successfully. The
window-added scan settled before counting, and the synthetic window closed
after counting. This meets the predeclared work target of zero schedules for
ten title-only changes, down from ten schedules per trial on P1a.

The canonical Debug dock then ran a separate synthetic QWidget case requesting
one resize followed by eleven title updates. Its global trace showed 12 hint
schedules and 10 executions; a separate ten-second idle interval showed 0/0.
This confirms that tracker hint work remained active during the mixed sequence,
but these markers have no event IDs and cannot attribute each schedule or prove
the exact pending-timer deadline. The source contract verifies that the
unchanged-title branch does not call the scheduling helper and the production
handler does not stop its timer. Individual non-title state transitions remain
in the focused `WindowInfoWrap` comparison test and the live retest matrix.

## Release A/B result

Five alternating Release A/B pairs used separate matching install prefixes.
A is `ec8a504db` (P1a) and B is `f6ebf3432`. Both were built with GCC 16.2.1
on Fedora 44 using Release, Qt 6.11.2, KDE Frameworks 6.30, Plasma 6.7.5 and
kernel 7.2.7. Each executable loaded its own prefix's QML/plugin files, with
the same user layout and Plasma environment. Each run used a 30-second warmup
and the same synthetic ten-title sequence; `/proc/<pid>/stat` and
`smaps_rollup` were sampled over a 7.5-second interval spanning that sequence.
All ten probes exited successfully. The raw marker-free records are in
[P1b-release-ab-2026-09-30.jsonl](P1b-release-ab-2026-09-30.jsonl).

The median paired B−A CPU delta was 0 seconds (0 percentage points of one
core), with a range of −0.01 to +0.24 seconds (−0.134 to +3.197 percentage
points). The median paired PSS delta was +2,094 KiB, ranging from −300 to
+144,763 KiB. The high value comes from the first B run (382,728 KiB versus
237,965 KiB for its paired A); it is retained with unknown cause. The other
four B runs ranged from 234,628 to 245,455 KiB, and their A partners ranged
from 234,213 to 237,582 KiB. The short CPU window had no repeatable change
outside its observed variability. These measurements do not support a CPU or
memory saving claim. Each Release run printed the VM's two Mesa EGL
`failed to create dri2 screen` warnings, matching the existing Fedora graphics
environment warning. The canonical user-mode Debug process was restored after
the Release trials.

Title response latency was not measured: the fixture does not expose a
trustworthy timestamp for the tracker receiving each title update. The field
comparison unit test covers every non-display `WindowInfoWrap` field, and the
source contract checks wiring. No fake-adapter host fixture was added; the live
Wayland title trace verifies the real dispatch path, while individual non-title
state and exact pending-geometry timing checks remain pending. The live global
schedule/execute markers carry no window or event IDs, so they cannot correlate
a resize with its debounce deadline. Adding per-event production tracing would
change the measured path and undermine the uninstrumented Release comparison;
pending-timer preservation is instead protected by the source contract and the
production handler's unchanged timer path. Do not add instrumentation unless a
bounded, event-correlated test seam becomes available.
The user-confirmed Fedora task indicators and preview are recovery evidence,
not completion of the runtime feature matrix.

The final manual matrix must test the task-icon hover settings independently:
window preview only, highlight only, and preview plus highlight. Verify each
mode follows its setting and preserves tooltips, grouped-window actions,
hide/dodge behavior and preview-helper failure fallback.

On 2026-09-30, the user reported that the remaining title/window-state and task
interaction checks passed. Exact response latency remains unmeasured, and the
short Release A/B samples still do not support a CPU or memory saving claim.

## Handoff

No bounded event-correlated pending-geometry check is currently available:
the live markers have no event identity, and production tracing would perturb
the Release measurement. Keep exact title-response latency unmeasured unless a
lightweight event-correlated seam is later identified. Retain the user's
functional acceptance while keeping the response-latency and performance
benefit limits explicit.
