# R2c-detaching-temporary: Read Elements of a Returned List Without Copying It

Status: Complete. Merged into `main` in `181b7f871`; the checks on `main` and
the host desktop smoke passed ([R2c fix batch 2](R2c.md#fix-batch-2)). CI run
40683601580 at `14d705a5d` passed all 23 jobs, including the 43-check clazy gate
([CI validation](R2c.md#ci-validation)).
Date: 2026-10-28. Branch: `claude/r2c-batch-c-bca59f`. Start HEAD: `cec126a31`
(the `R2c-unused-non-trivial-variable` commit on top of `eba91a3e6`).
Implementation commit: the commit that adds this report. Part of [R2c](R2c.md);
the second slice of batch C (fix batch 2).

## Defect

43 call sites in 6 files called a non-const member of a `QList` that a function
had just returned by value: 38 `operator[]` and 5 `first()`. The functions are
`QQuickItem::childItems()`, `QGuiApplication::screens()`,
`QDBusMessage::arguments()` and `Plasma::Corona::containments()`. Each hands out
its list by value, so the temporary shares its buffer with the owner, and a
non-const member on it detaches: the temporary allocates a private buffer and
copies every element, only for the caller to read one. clazy reports "Don't call
QList::operator[]() on temporary" (and the same for `first()`).

In `containment/plugin/layoutmanager.cpp` (36 of the 43) the calls sit in loops
that ask `childItems()` on every pass, some of them twice per pass, so each read
of one child copied the child list of a layout. There is no functional defect and
no known symptom; the finding is a cost finding on the applet-ordering paths.

## Fix

`x()[i]` became `x().at(i)` and `x().first()` became `x().constFirst()` at every
flagged location. The edit was applied by a script from the positions clazy
reported, and each of the 43 changed lines was then compared with its old line
under the same rewrite: 43 of 43 are equal, so nothing else on those lines
changed. No loop was restructured: each iteration still asks the layout for its
current children, so a loop whose body moves items still sees the change;
hoisting `childItems()` out of the loops would be a separate change.

| File | Sites | Call |
| --- | ---: | --- |
| `containment/plugin/layoutmanager.cpp` | 36 | `childItems()[i]` of `layout`, `m_startLayout`, `m_mainLayout` and `m_endLayout`, in `save` 1, `insertAtLayoutTail` 3, `insertAtLayoutHead` 3, `insertAtLayoutIndex` 1, `insertAtLayoutCoordinates` 2, `firstSplitter` 3, `lastSplitter` 3, `appletItemInLayout` 1, `dndSpacerIndex` 3, `hideAppletItem` 1, `showAppletItem` 1, `destroyAppletContainer` 1, `reorderSplitterInStartLayout` 2, `reorderSplitterInEndLayout` 2, `addJustifySplittersInMainLayout` 4, `destroyJustifySplitters` 1 and `joinLayoutsToMainLayout` 4 |
| `app/layout/genericlayout.cpp` | 2 | `qGuiApp->screens()[i]` in `GenericLayout::viewAtLowerScreenPriority` |
| `containmentactions/contextmenu/menu.cpp` | 2 | `reply.arguments().first()` for `contextMenuData` and `viewTemplatesData` |
| `app/lattecorona.cpp` | 1 | `containments().first()` in `Corona::unload` |
| `app/plasma/extended/screengeometries.cpp` | 1 | `reply.arguments().first()` of the `NameHasOwner` reply |
| `app/wm/abstractwindowinterface.cpp` | 1 | `reply.arguments().first()` of the KWin wrapping-around reply |

What stays the same:

- **Element and bounds.** `at(i)` and `operator[]` read the same slot, assert the
  same range in a debug build and check nothing in a release build. No index
  expression and no bounds check changed: the `count()`, `size` and `isEmpty()`
  guards around the sites, including the two `count() > 0 ? ... : nullptr`
  conditions in `joinLayoutsToMainLayout`, are untouched.
- **Reference type.** `operator[]` returned `T &` into the detached copy;
  `at()` and `constFirst()` return `const T &` into the shared buffer. Every site
  copies the element (`QQuickItem *item = ...`, `QVariant arg = ...`), passes it
  by value or calls a const member on it, and none writes through the reference.
  The reference is used inside the full expression that creates the temporary,
  and the temporary keeps the buffer alive until that expression ends.
- **`Corona::unload()`.** `delete containments().constFirst()` still takes a fresh
  list on every pass, which is the design the comment above the loop describes.
  The temporary shares its buffer with Plasma's list. As that comment says,
  Plasma removes a destroyed containment from its own list through
  `QObject::destroyed`; that removal detaches Plasma's list while the temporary
  keeps the old buffer until the delete expression ends. I found no caller of
  `Corona::unload()` in the tree: it is a public slot, it is not in the D-Bus
  interface and the destructor and layout unloading go through
  `m_layoutsManager->unload()`. A short comment records why `constFirst()` is safe.
- **Formatting.** `git clang-format` over the changed lines asked for two edits:
  `//we need` to `// we need` on the changed line at `layoutmanager.cpp:2769`,
  which is applied, and a join of a three-line condition in `screengeometries.cpp`
  into one line, which would rewrite two lines the change does not touch and only
  reflects the difference between `.clang-format` and the older layout, so it was
  left.

The gate side is one gate entry and one fixture, as in every R2c slice: the name
in `GATED_CHECKS` (after `connect-not-normalized`) and
`autotests/clazy/detaching-temporary.cpp`, which holds the smallest violation
(`return makeList().first();` on a function that returns a `QList<QString>`). The
gate has 39 entries and 39 fixtures on this branch.

## Behavior check

A probe program, not committed, mirrors two of the accessors: an owner with a
`QList<int *>` returned by value like `childItems()`, and a `QList<QVariant>`
returned by value like `arguments()` (`screens()` and `containments()` return
their lists by value in the same way). It was built with g++ 16.2.1 on Qt 6.11.2
and with g++ 13.2.0 on Qt 6.4.2. For lists of 1 to 5 elements and every index it
checks that `owner.childItems()[i]` and `owner.childItems().at(i)` return the
same element, that a temporary bound to a variable shares the owner's buffer, that
`at()` and `constFirst()` leave it shared and that `operator[]` and `first()`
detach it:

| Result | Qt 6.11.2 | Qt 6.4.2 |
| --- | ---: | ---: |
| index cases where `[]` and `at()` return the same element | 15 of 15 | 15 of 15 |
| `at()` left the buffer shared with the owner | 15 of 15 | 15 of 15 |
| `operator[]` replaced the buffer with a private copy | 15 of 15 | 15 of 15 |
| `first()` and `constFirst()` equal, `constFirst()` shared, `first()` detached (5 lists) | 5 of 5 | 5 of 5 |

## Evidence

Container commands are those of [R2c](R2c.md#reproducing-the-environment) with
the checkout mounted read-only, a build tree of its own (`$HOME/r2c-build-batch-c`)
and `-j6`/`--jobs 6`, because an idle-CPU measurement runs in the Fedora VM and
a second session builds at the same time.

| Command | Result |
| --- | --- |
| Baseline `test-clazy.py --build-dir build/modernization/clang-debug --jobs 6 --inventory unused-non-trivial-variable,detaching-temporary,container-anti-pattern --report ...` on `eba91a3e6` | Exit 0 in 146 s: `detaching-temporary` 43 findings in 6 files, 36 of them in `layoutmanager.cpp` (the other two checks: 14 and 38) |
| Container `cmake --build ... -j6` after the fix (Clang 18.1.3, Qt 6.11.1, strict warnings) | Exit 0 in 22 s, 6 objects, 0 compiler warnings |
| `test-clazy.py ... --inventory detaching-temporary` on the fixed tree | Exit 0 in 93 s: **0 findings** in 153 sources |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 6` | **Exit 0** in 129 s: 39 fixtures rejected by their own checks, 153 sources with no finding from 39 checks |
| Same, with the previous versions of the 6 files mounted over the fixed ones | **Exit 1** in 216 s: 43 `[detaching-temporary]` findings at the baseline positions (36, 2, 2, 1, 1 and 1 per file), nothing else |
| Host `cmake --preset gcc-debug`, `cmake --build --preset gcc-debug -j6`, `ctest --preset gcc-debug` (GCC 16.2.1, incremental) | Exit 0 in 48 s, 8 objects rebuilt (the 6 sources and the two test targets that compile `layoutmanager.cpp` and `menu.cpp`), 0 compiler warnings, 57/57 passed |
| Host `cmake --preset clang-debug`, `cmake --build --preset clang-debug -j6`, `ctest --preset clang-debug` (Clang 22.1.8, incremental) | Exit 0 in 49 s, 8 objects rebuilt, 0 compiler warnings, 57/57 passed |
| Probe program above (host Qt 6.11.2; Ubuntu 24.04 container with Qt 6.4.2) | 15 of 15 index cases equal on both, 0 failures |

The build logs hold thousands of `Warning:` lines from the qmllint step; they
are the known QML warnings tracked by the baseline gate, not compiler output,
and this change touches no QML.

`containmentlayoutunittest` compiles `layoutmanager.cpp` and drives it on real
`QQuickItem`s (restore, saving the options, scheduled destruction and the applet
lookup `appletItemInLayout`, which holds one of the 36 sites); it passes on both
compilers. Which of the 36 sites its restore path reaches was not measured. The
insertion helpers, the Justify splitters and the drag and drop spacer index are
the paths I expect only the desktop to exercise.

## Handoff

- **Batch / slice and selected scope:** R2c fix batch 2, batch C, slice 2 of 3:
  `detaching-temporary`, 43 locations in 6 files.
- **Status:** Complete; CI run 40683601580 passed.
- **Source baseline / start HEAD / implementation HEAD:** `eba91a3e6` /
  `cec126a31` / the commit that adds this report.
- **Branch / existing user edits preserved:** `claude/r2c-batch-c-bca59f` in its
  own worktree; the tree was clean at the start of the slice.
- **Dependencies and evidence links:** [R2c](R2c.md),
  [R2c-unused-non-trivial-variable](R2c-unused-non-trivial-variable.md) (the
  slice before it on the branch).
- **Files and behavior changed:** the 6 production sources of the table (43 call
  sites), `scripts/test-clazy.py` (one name),
  `autotests/clazy/detaching-temporary.cpp`. No observable change.
- **State authority / lifetime constraints:** none new. The lists are returned by
  value and owned by their sources; the temporaries only share a buffer for the
  length of one expression. The only lifetime-sensitive site is the `delete` in
  `Corona::unload()`, described above.
- **Toolchain and artifact identity:** container image `latte-r2c-clazy:noble`
  (ID `8e9528e34f50`; clazy 1.17 on LLVM 19.1.1, Ubuntu clang 18.1.3, Qt 6.11.1,
  CMake 3.30.5); host GCC 16.2.1, Clang 22.1.8, CMake 4.3.4, Qt 6.11.2. Raw logs
  and the probe are in the session notes folder of the batch (outside the
  repository).
- **Commands executed:** the Evidence table.
- **GCC/Clang Debug warning/error results:** 0 compiler warnings and 0 errors on
  both; Release presets were not built.
- **Tests registered / run / passed:** 57 / 57 / 57 on GCC and Clang Debug; no
  test added or changed.
- **Production behavior vs copied logic vs source-contract evidence:** the gate
  runs clazy on the production sources; the fixture holds a copy of the pattern;
  the probe reproduces the accessors and was built against real Qt, not against
  the production functions. `containmentlayoutunittest` runs production code of
  `layoutmanager.cpp`, but no test was written for the changed sites.
- **QML diagnostics added/removed:** none; no QML changed.
- **Negative fixtures / mutation evidence:** the fixture above and the run with
  the 6 pre-fix files mounted over the fixed ones (43 findings, exit 1).
- **Desktop feedback / log review / clean-quit evidence:** the host smoke on
  `main` passed with no warning from Latte's code and no core ([R2c fix batch
  2](R2c.md#fix-batch-2)). It reached startup on two screens, the context menu's
  layout and template lists and a layout switch. Dragging, adding and removing
  widgets passed by the user's report without a log entry. Justify alignment and
  undoing a removal were not part of it. The paths listed before the smoke: add,
  drag and drop and remove widgets in the dock, including a widget dropped between
  two others (`insertAt*`, `dndSpacerIndex`); a dock with Justify alignment, which
  adds and moves the splitters (`addJustifySplittersInMainLayout`,
  `joinLayoutsToMainLayout`, `reorderSplitterIn*Layout`); undoing a widget removal
  (`hideAppletItem`, `showAppletItem`); a two-screen setup for
  `GenericLayout::viewAtLowerScreenPriority`; the dock context menu, which reads
  `contextMenuData` and `viewTemplatesData`; startup, which asks the Plasma
  service for its name and KWin for the virtual-desktop wrapping.
- **Performance:** not measured. Each of the 43 reads no longer allocates and
  copies a list; the change cannot add work.
- **Combined validation:** on `main` at `382e55cd2`, GCC and Clang Debug and
  Release with 0 warnings and 57/57 tests each, the 43-check gate and the
  Debian 13 container build on Qt 6.8.2 passed; see [R2c fix batch 2](R2c.md#fix-batch-2).
- **CI, distribution and Nix checks:** run 40683601580 at `14d705a5d` passed all
  23 jobs, among them `Build (Clang Debug)` with the 43-check clazy gate and
  `Install verify (debian13)` on Qt 6.8.2; see
  [CI validation](R2c.md#ci-validation).
- **Unresolved failures, review assumptions and unavailable checks:** none
  failed. `Corona::unload()` has no caller I could find, so its changed line is
  not reachable today; whether the function is dead code is a question for the
  maintainers and is outside this slice. A unit test for the insertion helpers
  and the splitter functions of `LayoutManager` would cover the sites that only
  the desktop reaches; adding it was outside the files this slice may touch.
- **Next exact action:** none.
- **Commit approval status / push approval status:** committed on the feature
  branch; merged into `main` in `181b7f871` with the user's approval; pushed
  with `14d705a5d`.
