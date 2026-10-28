# R2c-incorrect-emit: Signals Emitted Before Anyone Can Listen

Status: Pending validation. Merged into `main` in `382e55cd2`; the checks on
`main` and the host desktop smoke passed ([R2c fix batch 2](R2c.md#fix-batch-2)). The CI run
waits for a push.
Date: 2026-10-28. Branch: `claude/r2c-batch-d-a6a9f7`. Start HEAD: `eba91a3e6`
(the start of batch D; this slice follows `R2c-const-signal-or-slot`). Implementation
commit: the commit that adds this report. Part of [R2c](R2c.md), the second slice of
batch D.

## Defect

clazy's `incorrect-emit` reports two different mistakes. Three findings are emits
in a constructor ("Emitting inside constructor probably has no effect"): clazy
assumes the object is not reachable yet, so nothing can have connected. One
is a signal called as a plain function ("Missing emit keyword"). The four
findings are:

| File:line (before) | Class | Code |
| --- | --- | --- |
| `app/view/windowstracker/windowstracker.cpp:40` | `ViewPart::WindowsTracker` | `Q_EMIT allScreensChanged();` |
| `app/view/windowstracker/windowstracker.cpp:41` | `ViewPart::WindowsTracker` | `Q_EMIT currentScreenChanged();` |
| `app/wm/tracker/trackedgeneralinfo.cpp:31` | `WindowSystem::Tracker::TrackedGeneralInfo` | `Q_EMIT lastActiveWindowChanged();` |
| `app/wm/waylandinterface.cpp:838` | `WaylandInterface::requestInfo()` | `windowRemoved(w->uuid());` |

## Fix

- `windowstracker.cpp`: the two emits are gone from the constructor.
- `trackedgeneralinfo.cpp`: the emit is gone from the constructor.
- `waylandinterface.cpp:838`: `Q_EMIT` added; the call is the signal of
  `AbstractWindowInterface`, so the line is otherwise unchanged.

A comment at each removed emit states why it is absent (no receiver can exist and
the getter already reports the state), so nobody adds it back. The gate side is
one entry in `GATED_CHECKS` and `autotests/clazy/incorrect-emit.cpp`, which holds both
shapes (an emit in a constructor and a signal called without `Q_EMIT`); the gate now has
39 entries and 39 fixtures. No other file changed.

## Why no receiver can exist

An emit in a constructor reaches only connections made before it, and only code that
ran earlier in the same constructor, or in something it called, can have made them.

**`ViewPart::WindowsTracker`** (`allScreensChanged`, `currentScreenChanged`):

- Before the emits the constructor creates `AllScreensTracker` and `CurrentScreenTracker`
  with the tracker as their parent, connects `Windows::enabledChanged` to a lambda whose
  receiver is the tracker, and calls `Windows::addView()`. The two child constructors
  connect `Latte::View` and `Windows` signals to lambdas that have the child as receiver;
  `addView()` creates a `TrackedViewInfo` and connects signals of the `View` to `Windows`.
  None of them takes a signal of `ViewPart::WindowsTracker` as sender.
- In the whole tracked tree the two signals appear only as the `NOTIFY` of the properties
  `allScreens` and `currentScreen`, in the declarations and in these two emits. There is no
  `connect()`, `SIGNAL()`, `QSignalSpy`, `Connections` or `on...Changed` handler. The
  other hits for `currentScreenChanged` are `Positioner::currentScreenChanged`, another class,
  and its only QML handler (`LatteDockConfiguration.qml:143`, a `Connections` on
  `latteView.positioner`); the search was case-insensitive, so `onAllScreensChanged` and
  `onCurrentScreenChanged` handlers would have shown.
- QML reaches the tracker only through `View::windowsTracker`, and `View` assigns it
  after the constructor returns (`view.cpp:221`: `m_windowsTracker = new ViewPart::WindowsTracker(this);
  Q_EMIT windowsTrackerChanged();`). The member is a `QPointer`, so the getter returns null
  while the constructor runs, and a binding that evaluates later reads `allScreens` and
  `currentScreen` fresh after `windowsTrackerChanged()`. The trackers are assigned only in the
  constructor, so the getters return the same objects for the tracker's whole life.

**`TrackedGeneralInfo`** (`lastActiveWindowChanged`):

- The only earlier call is `new LastActiveWindow(this)`; its constructor connects
  `Schemes`, `Windows` and `AbstractWindowInterface` signals to `LastActiveWindow` receivers
  and never connects to the `TrackedGeneralInfo`. The derived classes `TrackedViewInfo` and
  `TrackedLayoutInfo` run after this constructor.
- `lastActiveWindowChanged` is the `NOTIFY` of the property `activeWindow` and is
  connected nowhere; no QML or C++ code reads that property, and the objects are owned by
  `Windows` and not exposed to QML.
- The same signal is emitted again in `~TrackedGeneralInfo()` after the pointer is cleared.
  clazy does not report destructors and the slice does not change it.

**`WaylandInterface::requestInfo()`**: `Q_EMIT` expands to nothing, so adding it cannot change
the call. The signal still reaches `Windows::init()`'s lambda and `windowRemovedSlot()`.

## Proof

1. **Preprocessed and compiled units.** The scratch script `tu_compare.py` takes the pre-fix
   file from `eba91a3e6` and the working tree file, with the compile flags of the host Clang Debug
   build (Clang 22.1.8, Qt 6.11.2), and `-fmacro-prefix-map` so `__FILE__` is equal on both sides.
   - `waylandinterface.cpp`: the preprocessed text differs only in whitespace (the
     `Q_EMIT` token vanishes), and the object files compiled with `-g0` are byte-identical
     (781680 bytes each).
   - `windowstracker.cpp`: the preprocessed text differs in the two removed statements and in
     the line number of one `qCDebug` in the destructor (46 became 49 because the comment is
     longer than the two lines it replaces). The constructor of `ViewPart::WindowsTracker`
     calls 27 functions before and 25 after; the two that disappear are `allScreensChanged()`
     and `currentScreenChanged()`.
   - `trackedgeneralinfo.cpp`: the preprocessed text differs in the removed statement only. The
     constructor calls 15 functions before and 14 after; the one that disappears is
     `lastActiveWindowChanged()`.
2. **Source contracts.** `sourcecontracttest` inspects `requestInfo()` for `trackWindow(w);` and its
   condition, which did not change; it passes.

## Evidence

Container commands are those of [R2c](R2c.md#reproducing-the-environment), with a
private build directory (`~/r2c-build-batch-d`) and `--jobs 6`.

| Command | Result |
| --- | --- |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 6 --inventory incorrect-emit --report build/incorrect-emit.json` before the fix | 4 findings in 3 files, as in R2c |
| Same after the fix | Exit 0 in 143 s: 0 findings in 153 sources |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 6` | **Exit 0** in 190 s: 39 fixtures rejected by their own checks, 153 sources with no finding from 39 checks |
| Same, with the previous versions of the three sources mounted over the fixed ones, no rebuild | **Exit 1** in 138 s: the four findings, `windowstracker.cpp` 40:12 and 41:12, `trackedgeneralinfo.cpp` 31:12 and `waylandinterface.cpp` 838:9 |
| Container `cmake --build build/modernization/clang-debug -j6` after the change | Exit 0 in 18 s, 0 compiler warnings |
| Host `cmake --preset gcc-debug`, `cmake --build --preset gcc-debug -j6`, `ctest --preset gcc-debug` (GCC 16.2.1, Qt 6.11.2) | Exit 0 each; incremental build in 19 s, 0 compiler warnings, 0 CMake warnings; 57/57 tests passed |
| Host `cmake --preset clang-debug`, `cmake --build --preset clang-debug -j6`, `ctest --preset clang-debug` (Clang 22.1.8) | Exit 0 each; incremental build in 17 s, 0 compiler warnings, 0 CMake warnings; 57/57 tests passed |
| `tu_compare.py` on the three sources | As described above |

The build logs hold thousands of `Warning:` lines from the qmllint step; they are the
known QML warnings tracked by the baseline gate, not compiler output, and this
change touches no QML.

## Handoff

- **Files and behavior changed:** `app/view/windowstracker/windowstracker.cpp`,
  `app/wm/tracker/trackedgeneralinfo.cpp`, `app/wm/waylandinterface.cpp`,
  `scripts/test-clazy.py` (one name), `autotests/clazy/incorrect-emit.cpp`. Three emits that
  no receiver could observe are gone and one signal call has its `Q_EMIT`; the
  program behaves the same.
- **State authority / lifetime constraints:** `View::m_windowsTracker` is null until
  `View` assigns it, and `View` emits `windowsTrackerChanged()` afterwards; that, and the
  trackers being created once, is why `allScreensChanged()` and `currentScreenChanged()` have no
  initial emit. `allScreens` and `currentScreen` never change after construction, so the
  two properties could be `CONSTANT`; that is a design change outside this slice.
- **Tests:** no test added or changed; `ctest` 57/57 on GCC and Clang Debug. No automated test
  constructs these classes.
- **Smoke plan:** start the dock and check what the window tracker drives: the dodge
  visibility modes (`dodgeActive`, `dodgeMaximized`, `dodgeAllWindows`), which hide and show the
  dock as windows move or maximize (`VisibilityManager` connects to `currentScreen()`),
  dock colors that follow the active or touching window (`colorizer/Manager.qml`), and the
  scroll action that switches desktops or toggles the maximized state of the last active
  window (`layouts/EnvironmentActions.qml`). All three read `latteView.windowsTracker` after
  `windowsTrackerChanged()`. The debug log should hold no new warning.
- **Combined validation:** on `main` at `382e55cd2`, GCC and Clang Debug and
  Release with 0 warnings and 57/57 tests each, the 43-check gate and the
  Debian 13 container build on Qt 6.8.2 passed; see [R2c fix batch 2](R2c.md#fix-batch-2).
- **Desktop smoke result:** passed on the host ([R2c fix batch
  2](R2c.md#fix-batch-2)). The window trackers started without the removed emits.
  Window-tracker hint updates ran all day, an AlwaysVisible dock kept maximized
  windows at its edge, the `Default` layout's DodgeActive dock ran for a minute
  and a half, and the log holds no new warning. The dodge-maximized walk-through
  passed by the user's report; no switch to that mode appears in the log.
- **Not run:** the CI job (`main` is not pushed).
- **Next exact action:** after a push is approved, confirm the CI run and mark
  this slice Complete.
- **Commit approval status / push approval status:** committed on the feature
  branch; merged into `main` in `382e55cd2` with the user's approval; not
  pushed.
