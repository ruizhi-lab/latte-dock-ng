# R7b: Snapshot iteration in syncLatteViewsToScreens

Status: Complete. The code, the source contract, the GCC and Clang Debug builds and
ctest runs are done. The desktop hot-plug retest ran on 2026-11-10 in a two-output
variant (see [Desktop validation](#desktop-validation)).
Date: 2026-11-06. Branch: `claude/r7b-sync-views-93477d`. Start HEAD: `e18a0f9a7`
(main, release v1.3.0). Commits: `1df198b38` (fix), `72b624e28` (contract),
and the last commit on the branch, which adds this record. Nothing is pushed.

Scope: [R2c follow-up 1](R2c.md#follow-ups) and the two loops listed under
Exceptions in [R2c-range-loop-detach.md](R2c-range-loop-detach.md#exceptions).
`OriginalView::syncClonesToScreens()` (follow-up 2) is not changed.

## Defect

`GenericLayout::syncLatteViewsToScreens()` ran two range-for loops over live
containers. Each loop body could change the container it was reading.

- Loop over `m_containments`: `addView()` can append to the list. The chain is
  `View::setLayout()` (layoutChanged) -> `OriginalView::syncClonesToScreens()` ->
  `createClone()` -> `GenericLayout::newView()` -> `Storage::newView()` ->
  `Storage::importLayoutFile()` -> `ImportedContainments::restore()` ->
  `Corona::containmentAdded` -> `GenericLayout::addContainment()` ->
  `m_containments.append()`. An append that reallocates the buffer invalidates the
  loop's iterator. A containment removed during the loop would be dereferenced
  afterwards.
- Loop over `m_latteViews` (a `QHash`): `reconsiderScreen()` of an original view
  runs `OriginalView::syncClonesToScreens()`, which calls `removeClone()` and then
  `removeViewWithoutDestroyingContainment()`, which calls `m_latteViews.take()`.
  Changing a `QHash` during its own iteration is undefined.

Both loops carried `clazy:exclude=range-loop-detach` comments that recorded this
as a known exception. The path needs a dock in "All secondary screens" mode with
two secondary outputs, which the R2c host smoke could not reach. No user report or
log shows a failure here. The defect is established from the code.

## Fix

Each loop walks a copy and checks every entry against the live container right
before using it. Only `app/layout/genericlayout.cpp` changed in the fix commit.

- Containments: `const auto containmentsSnapshot = m_containments;`. The loop skips
  an entry with `!m_containments.contains(containment)` before any dereference.
- Views: `QList<QPointer<Latte::View>> reconsiderSnapshot`, built from
  `m_latteViews`. The loop skips an entry with `!view || m_latteViews.value(view->containment()) != view`.
  The original condition and `reconsiderScreen()` follow unchanged.
- Both `clazy:exclude` lines are removed. The comments now give the trigger, why
  the copy is safe, and the failure mode, as AGENTS.md rule 7 asks.
- Entries added during a loop are not visited. Containments appended by the first
  loop are created after `validViewsMap()` has run, so their ids are not in
  `viewsMap` and the body would do nothing for them. Clones created during the
  second loop fail `isOriginal()`. The C++ range-for also evaluates `end()` once,
  so an append that did not reallocate was not visited before either.

## Behaviour changed

- Entries present when a loop starts are visited in the same order with the same
  body.
- An entry removed during a loop is skipped. Before, the result was undefined.
- An entry added during a loop is not visited. Before, the result was undefined
  for a reallocating append and no visit for the rest.
- No other behaviour changes. `OriginalView::syncClonesToScreens()`,
  `addView()`, `reconsiderScreen()`, the storage path and QML are not touched.

## State authority, lifetime and invalidation

- `m_containments` decides which containments belong to the layout. It is changed
  by `addContainment()` and `assignToLayout()` (appends), and by
  `containmentDestroyed()` (connected to `QObject::destroyed`),
  `unassignFromLayout()` and `removeViewWithoutDestroyingContainment()` (removals).
  The membership test compares pointers only, so a destroyed containment that was
  already removed is never dereferenced.
- `m_latteViews`, keyed by containment, decides which view is registered for a
  containment. It is changed in several places, among them `addView()`,
  `assignToLayout()`, `destroyedChanged()`, `recreateView()`, `unassignFromLayout()`,
  `removeViewWithoutDestroyingContainment()` and the deletion loop of this function.
  The registration check requires that the value for the view's own containment is
  that view.
- `viewsMap` is still computed once by `validViewsMap()` before the loops, as
  before. It is not refreshed during the loops.
- `QPointer<Latte::View>` covers the view's lifetime. The deletion loop and
  `removeViewWithoutDestroyingContainment()` call `deleteLater()`, so a removed
  view stays alive until the event loop runs. The null check alone would not skip
  it. The registration check does.
- Threading is unchanged: everything runs on the GUI thread. The copies are locals
  and end with the function.
- Re-entrancy was not traced. A loop body could reach this function again through
  a signal (`Positioner::syncLatteViews`). A nested call uses its own copies and
  each outer entry is still checked before use, so the outer loop stays
  well-defined either way.

Failure modes this prevents: iterator invalidation after an append (undefined
behaviour, typically a crash inside Qt's list code); dereference of a containment
removed or destroyed during the loop; `QHash` iteration after `take()`; and
`reconsiderScreen()` on a view that was removed or deleted.

## Evidence

Worktree: `/data/projects/latte-dock-ng/.claude/worktrees/r7b-sync-views-93477d`.
Logs: `build/modernization/r7b-logs/` (git-ignored, under the build tree).

| Step | Command | Exit status | Result |
| --- | --- | --- | --- |
| GCC configure | `cmake --preset gcc-debug` | 0 | Configured |
| GCC build 1 | `cmake --build --preset gcc-debug --parallel 6` (started before the edits) | 0 | 0 `warning:` lines |
| GCC build 2 | same, after the fix and the contract | 0 | 0 `warning:` lines |
| GCC ctest 1 | `ctest --preset gcc-debug` | 8 | 60 of 61 passed. `sourcecontracttest` failed on my own ordering assertion (`guardedCopy > viewsCopy`). The declaration precedes the fill loop, so the assertion was wrong. Fixed in the test; production code unchanged |
| Contract rebuild | `cmake --build --preset gcc-debug --parallel 6 --target sourcecontracttest` | 0 | 0 `warning:` lines |
| Contract alone | `sourcecontracttest genericLayoutSyncLatteViewsToScreensIteratesSnapshots` | 0 | PASS |
| GCC final build | `cmake --build --preset gcc-debug --parallel 6` | 0 | 0 `warning:` lines; recompiled `genericlayout.cpp` after the mutation restore |
| GCC final ctest | `ctest --preset gcc-debug` | 0 | 61 of 61 passed, 32.97 s |
| Clang configure | `cmake --preset clang-debug` | 0 | Configured with `clang++` 22.1.8 in a fresh build directory |
| Clang build | `cmake --build --preset clang-debug --parallel 6` | 0 | 0 `warning:` lines; compiled `genericlayout.cpp` and `sourcecontracttest.cpp` from scratch |
| Clang ctest | `ctest --preset clang-debug` | 0 | 61 of 61 passed, 33.00 s |

Each build log has 4 ` error:` matches, all QML `console.warn("ContextMenu component load error:")` text. None is a compiler error, and the build exit statuses are 0.

### Mutation evidence

The contract reads `app/layout/genericlayout.cpp` at run time. I replaced the file
with each variant, ran the contract, and restored the committed copy. `cmp`
confirmed the restore.

| Variant | Exit | First failing assertion |
| --- | --- | --- |
| M1: the pre-R7b file (`git show e18a0f9a7:app/layout/genericlayout.cpp`) | 1 | `!body.contains("clazy:exclude")` |
| M2: membership test disabled (`if (false) {`) | 1 | `containmentsCheck > containmentsLoop` |
| M3: registration check reduced to the null check | 1 | `viewsCheck > viewsLoop` |

### Other checks

- **Production-level test: not feasible.** `GenericLayout` is compiled only into the
  application target (`app/layout/CMakeLists.txt`). No autotest links it or builds a
  `Corona`. `syncLatteViewsToScreens()` returns at once without `m_corona`. The
  source contract is therefore a text lock only, not behaviour evidence.
- **clazy:** not installed here, and CI runs the 49-check gate. I checked the
  gated rules that could apply by hand. No range-for over a non-const member
  remains in this function (`range-loop-detach`). The loops over the copies bind a
  `const` local or a reference (`range-loop-reference`). No `keys().contains()`
  pattern is used (`container-anti-pattern`). Both copies are used
  (`unused-non-trivial-variable`). No `clazy:exclude` was added.
- **qmllint:** no QML file changed, so no `scripts/qmllint-deep.sh` run and no
  baseline comparison were made.
- **Debian 13 verification: deferred by the user until the refactor is complete.**
- **Warnings:** GCC and Clang Debug have 0 `warning:` lines in the logs above.

## Desktop retest

Not run. Only the user can run it: it needs the user's session and two secondary
outputs, and the dock must not be started against the desktop from here. The steps
follow the user-mode workflow in `docs/development-testing-guide.md` (Runtime Retest
Workflow).

1. Set the dock to "All secondary screens" with two secondary outputs connected
   (three outputs in total). The dock should show an original on one secondary
   output and a clone on the other.
2. Install this branch as user-mode Debug and start it with a fresh log:

   ```bash
   cd /path/to/latte-dock-ng
   git switch claude/r7b-sync-views-93477d
   bash install.sh --user Debug
   pkill -x latte-dock-ng || true
   rm -f /tmp/latte-ng.log
   source ~/.config/latte-dock-ng/dev-env.sh
   nohup ~/.local/bin/latte-dock-ng --replace --debug > /tmp/latte-ng.log 2>&1 &
   ```

3. Record a core-dump baseline: `coredumpctl list latte-dock-ng | tail -n 3`.
4. **Scenario A (clone's output):** unplug the secondary output that shows the
   clone, wait 10 seconds, replug it, wait 10 seconds. Repeat three times. Then
   check the log:

   ```bash
   grep -c "START of SyncLatteViewsToScreens" /tmp/latte-ng.log
   grep -c "end of, syncLatteViewsToScreens" /tmp/latte-ng.log
   grep "view must consider its screen" /tmp/latte-ng.log | tail -n 10
   grep "IsClonedFrom" /tmp/latte-ng.log | tail -n 5
   grep -E "Fatal|ASSERT|Segmentation|KCrash|QList::|QHash::" /tmp/latte-ng.log || echo "no fatal markers"
   coredumpctl list latte-dock-ng | tail -n 3
   ```

   Expected:
   - The START and end counts are equal, with one pair per layout per sync.
   - Each sync logs "view must consider its screen" once per original dock,
     naming the original's containment id. The clone's id never appears there.
   - After replug, a clone window appears on the output within a few seconds, and
     the new clone shows up in an `IsClonedFrom` line for the original.
   - No fatal marker, and no new `latte-dock-ng` core dump.
5. **Scenario B (original's output):** unplug the output that shows the original,
   wait 10 seconds, replug it, wait 10 seconds. Run the same checks. If the dock
   ends in a different place from v1.3.0, record where, and compare with the
   v1.3.0 package.
6. A clean-quit check is not needed for this change, because it does not touch
   quit or teardown. A restart of the dock is still a useful smoke test.

## Not verified

- The desktop hot-plug retest above, which only the user can run.
- A production-level test of the sync path (see [Other checks](#other-checks)).
- The clazy gate and qmllint, which CI runs.
- Debian 13 verification, deferred by the user until the refactor is complete.
- A comparison against a pre-fix binary on the desktop. No crash is claimed, so no
  A/B crash check applies.

## Superseded records

`R2c.md` (Follow-ups, item 1) and the Exceptions list in `R2c-range-loop-detach.md`
describe these two loops as live iterations. This change closes item 1. Those
records were not edited here. The main session decides whether to update them.

## Next exact action

Run Scenario A (step 4 above) on a checkout of `claude/r7b-sync-views-93477d` and
send back the output of the five `grep` and `coredumpctl` commands.

Commit approval: the branch commits above are made, and the work is on this branch
only. Push approval: not requested, so nothing is pushed.

## Desktop validation

Checked on 2026-11-10 and 2026-11-11 on the user's desktop. Two-output variant: in two HDMI-A-1 off/on cycles, the clone of an all-screens test dock was removed and created again. Every sync START had its end, and only originals were reconsidered. There was no fatal marker and no coredump. The three-output scenario and scenario B were not run. Full record: [R7-desktop-check](R7-desktop-check.md).
