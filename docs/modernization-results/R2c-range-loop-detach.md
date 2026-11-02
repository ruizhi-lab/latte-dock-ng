# R2c-range-loop-detach: Range Loops Do Not Detach Qt Containers

Status: Complete. Merged into `main` (fast-forward to `6b51b43a6`, pushed on 2026-11-02);
the checks passed, CI run 41452548625 passed all 23 jobs and the host desktop smoke
passed ([R2c fix batch 3](R2c.md#fix-batch-3)). Date: 2026-11-02. The check joined the
gate in `8a30d3fa9`.

This is the umbrella report of the largest slice of R2c: all 274 findings of
`range-loop-detach` that were left after fix batch 2, in 45 files, fixed by four
groups of fix batch 3. Each group has its own report with one row per loop; this
report holds the totals, the forms of the fix, the exceptions and the gate.

## Defect

clazy 1.17 reports "c++11 range-loop might detach Qt container" for a range-based
`for` over a non-const Qt container, and over a call that returns a container by
value. Such a loop calls the non-const `begin()`, which deep-copies a container
whose data is shared with another owner. The finding is a cost finding: no symptom
is known. The inventory counted 305 findings; fix batch 2 cleared 31 of them as a
side effect of `R2c-container-anti-pattern` and `R2c-detaching-temporary`.

## Fix

Three forms, chosen per loop from its range expression and body:

- **Wrap (107 loops).** The range is an lvalue container whose body does not modify
  it: `for (DECL : container)` became `for (DECL : std::as_const(container))`.
  `std::as_const` is the form the coding standard asks for; `qAsConst` stays banned.
- **Hoist (164 loops).** The range is a call that returns a container by value
  (`QStandardPaths::standardLocations()`, `KConfigGroup::groupList()`,
  `Containment::applets()`, `QDir::entryList()`, `qGuiApp->screens()` ...).
  `std::as_const` cannot bind a temporary, so the result moved into a const local
  held by a C++20 range-for init-statement:
  `for (const auto screens = qGuiApp->screens(); const auto &screen : screens)`.
  The local is const, so the loop uses `begin() const`; its lifetime is the loop's,
  as the temporary's was; the call runs once at the same point; and a loop whose
  body changes the object the call read from keeps iterating its snapshot.
- **Other (3 loops).** `OriginalView::syncClonesToScreens()` erased elements from the
  list it iterated and now uses `QList::removeIf` (the only deliberate behavior
  change, below). Two loops of `GenericLayout::syncLatteViewsToScreens()` keep their
  live iteration under a documented `clazy:exclude` (below).

| Group | Batch | Files | Findings | Wraps | Hoists | Other | Commits | Report |
| --- | --- | ---: | ---: | ---: | ---: | --- | --- | --- |
| G | layout and view code | 12 | 74 | 42 | 29 | 1 `removeIf`, 2 exceptions | `a18a905bc` | [report](R2c-range-loop-detach-g.md) |
| H | services | 20 | 65 | 23 | 42 | none | `d8b07b428`, `6f42e5ed2` | [report](R2c-range-loop-detach-h.md) |
| I | `app/layouts/storage.cpp` | 1 | 63 | 8 | 55 | none | `8a998f1b8` | [report](R2c-range-loop-detach-i.md) |
| J | layouts, settings, window code | 12 | 72 | 34 | 38 | none | `4116888bb` | [report](R2c-range-loop-detach-j.md) |
| | | 45 | 274 | 107 | 164 | 3 | | |

## The one behavior change

`OriginalView::syncClonesToScreens()` removed the ids that are waiting for a clone
with `secondaryscreens.removeAll(scrid)` inside a range-for over the same list.
After each removal the loop moves past the id that slid into the erased slot, so a
waiting id that directly follows a removed one stayed in the list, and the code
below then moved a removable clone onto a screen whose clone was still being
created, or called `createClone()` for it. `removeIf` removes every waiting id in
one pass. The old and new results differ only for two adjacent waiting ids; the
table of examples and the reading of the code that follows are in
[the group G report](R2c-range-loop-detach-g.md#behavior-change-originalviewcpp225).
It needs a dock in "All secondary screens" mode with two secondary screens whose
clones are created at the same time, so a two-screen desktop cannot reach it.

## Exceptions

Two loops keep their live iteration, each with `// clazy:exclude=range-loop-detach`
on the `for` line and the traced reason above it:

- `app/layout/genericlayout.cpp`, `syncLatteViewsToScreens()`, loop over
  `m_latteViews`: `reconsiderScreen()` of an original view runs
  `OriginalView::syncClonesToScreens()`, which calls `removeClone()` for a clone on
  a screen that is no longer used, and that reaches `m_latteViews.take()`.
- Same function, loop over `m_containments`: `addView()` emits `layoutChanged`
  through `View::setLayout()`; the original view's `syncClonesToScreens()` can call
  `createClone()`, then `GenericLayout::newView()`, `Storage::newView()`,
  `Storage::importLayoutFile()` and `ImportedContainments::restore()`, which emits
  `Corona::containmentAdded` and reaches `GenericLayout::addContainment()` and
  `m_containments.append()`.

Both are mutation during iteration that existed before the slice and that a
`const` view or a snapshot would change. They are follow-ups, not part of this
slice (see [R2c](R2c.md#follow-ups)).

## Left alone, not findings

clazy reports nothing for these, so they were not edited: loops over local copies
and by-value parameters in `synchronizer.cpp:456`, `:798`, `waylandinterface.cpp:108`
and `manager.cpp:451`; two loops that write their elements through a non-const
reference (`appletids.cpp:91`, `layoutscontroller.cpp:142`); and a loop in
`tasktools.cpp:291` that prepends to the list it iterates and returns at once.

## Gate

`range-loop-detach` is in `GATED_CHECKS` with `autotests/clazy/range-loop-detach.cpp`
(a non-const member `QList` walked by a range-for). The gate has 49 checks and 49
fixtures. On the merged tree it exits 0; mounting the pre-fix `screenpool.cpp` and
`globalshortcuts.cpp` over the fixed files makes it exit 1 with the 14 findings of
those two files (5 and 9).

## How the diffs were checked

Every group reported its loops one by one. The main session also checked all of it
mechanically: a script compared each changed line with the base and accepted only
`std::as_const(...)` wraps and `for (const auto NAME = CALL; DECL : NAME)` hoists
whose reconstruction equals the old line ignoring whitespace. 268 of the 274 loops
matched exactly; the other six were read by hand: the two exceptions, the
`removeIf` loop and three hoists whose line was split or sits inside a one-line
lambda. A second script scanned the body of every wrapped loop for a direct write
to its container (109 matches, one of them a test string); the only hit iterates a
local list while it changes a member list, which is safe.

## Handoff

- **Files and behavior changed:** 45 production files and `autotests/sourcecontracttest.cpp`
  (the search text of `viewDestructorDropsPointerWindowTrackingConnections`). No
  observable change except the `removeIf` case above.
- **Tests:** no test added or changed apart from the source-contract text.
- **CI and desktop smoke:** CI run 41452548625 passed all 23 jobs; `Build (Clang Debug)` ran
  the gate with `range-loop-detach` and found nothing in 153 production sources. The host
  smoke on `6b51b43a6` passed with no warning from Latte's code and no core. It reached
  `GenericLayout::syncLatteViewsToScreens()` at startup and in two layout switches (eight
  calls in the log), but not the paths that change the iterated containers (clones, a screen
  added or removed), and not the `removeIf` case of `OriginalView::syncClonesToScreens()`,
  which needs two secondary screens. See [R2c fix batch 3](R2c.md#fix-batch-3).
- **Next exact action:** none. The decisions that remain are items 1 and 2 of the
  [R2c follow-ups](R2c.md#follow-ups).
- **Commit approval status / push approval status:** the group commits were made on
  session branches and merged into the integration branch by the main session (G
  `c0f64b9ee`, H `e62767af2`, I `98b00c657`, J `b7fc199db`). `main` was fast-forwarded to
  `6b51b43a6` and pushed on 2026-11-02 with the user's approval.
