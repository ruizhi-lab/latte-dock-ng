# R2c-range-loop-reference: Range Loops Bind Their Element by `const &`

Status: Pending validation. The fix, the fixture and the gate pass locally with
Clang and GCC; the CI run and the combined desktop smoke are not done.
Date: 2026-10-27. Branch: `claude/r2c-batch-a-a5a9fa`. Start HEAD: `4a08e472e`
(the `R2c-qfileinfo-exists` commit on top of `09b030e61`). Implementation
commit: the commit that adds this report. Part of [R2c](R2c.md); the second
slice of batch A.

## Defect

15 range-based `for` loops in 6 files declared their variable as `auto`, so each
iteration copied a non-trivial element: 12 `QString`, 2 `QRegion` and 1
`Latte::Layouts::SubContaimentIdentityData` (two `QString` members). clazy
reports "Missing reference in range-for with non trivial type". The copies are
implicitly shared (an atomic reference count increment and decrement per
iteration; two `QString` copies for the identity record), so the cost is small
and there is no functional defect or known symptom. The finding is also a
statement of intent: no loop assigns to its variable.

## Fix

`for (auto x : range)` became `for (const auto &x : range)` at the 15 flagged
locations, the form already used in `Synchronizer::freeRunningActivities()`
(`for (const auto &activity : runningActivities())`). If a body had assigned to
its variable the compiler would reject the change; none did, so no copy had to
stay and nothing needed `// clazy:exclude=range-loop-reference`.

A reference into the container is only safe while the sequence is not modified
and the storage lives. Each loop was read for both:

| Location | Element | Range | Why the reference stays valid |
| --- | --- | --- | --- |
| `app/layout/genericlayout.cpp:1319` | `QString` | local `clearpendings` | The body removes the id from `m_pendingContainmentUpdates`, a different object |
| `app/layouts/storage.cpp:157` | `SubContaimentIdentityData` | member `m_subIdentities` | `subContainmentId() const`; the body reads two members and returns an int |
| `app/layouts/synchronizer.cpp:97` | `QString` | temporary `m_assignedLayouts.keys()` | Range-for keeps the temporary alive for the loop; the body only reads `m_assignedLayouts` (a `const` method) |
| `app/layouts/synchronizer.cpp:139` | `QString` | temporary `m_assignedLayouts.keys()` | The body edits the local `frees` |
| `app/layouts/synchronizer.cpp:169` | `QString` | parameter `const QStringList &layoutActivities` | The body appends to the local `valids` |
| `app/layouts/synchronizer.cpp:456`, `:472` | `QString` | locals `explicits`, `freelayoutnames` | Local copies of entries of `m_assignedLayouts` that no other code refers to, so the signals the body emits cannot modify them |
| `app/layouts/synchronizer.cpp:636` | `QString` | local `preloadednames` | The body moves a layout file by name |
| `app/layouts/synchronizer.cpp:827`, `:844` | `QString` | locals `explicits`, `freelayoutnames` | As 456 and 472 |
| `app/settings/universalsettings.cpp:81` | `QString` | local `colorsScriptPaths` | The body registers each directory with `KDirWatch` |
| `app/view/effects.cpp:446`, `:450` | `QRegion` | members `m_subtractedMaskRegions`, `m_unitedMaskRegions` | The body combines into the local `region` |
| `containmentactions/contextmenu/menu.cpp:318`, `:346` | `QString` | temporary `m_actions.keys()` | The body changes the actions, not the key list |

11 of these 15 loops are also `range-loop-detach` findings (a non-const range
over a container that may be shared); the exceptions are `storage.cpp:157`,
`synchronizer.cpp:169`, `:456` and `:827`. That check has its own slice,
`R2c-range-loop-detach`, and still reports its 305 findings after this slice.
This slice does not add `std::as_const`, so the `begin()` call, and with it
any detach, is exactly what it was.

The gate side is one gate entry and one fixture, as in every R2c slice: the name
in `GATED_CHECKS` and `autotests/clazy/range-loop-reference.cpp`, which holds
the smallest violation (`for (auto name : names)` over a `QStringList`). The
gate has 33 entries and 33 fixtures on this branch.

## Evidence

Container commands are those of [R2c](R2c.md#reproducing-the-environment) with
the checkout mounted read-only, a build tree of its own and `-j6`/`--jobs 6`,
because a second session builds at the same time.

| Command | Result |
| --- | --- |
| Baseline `test-clazy.py --build-dir build/modernization/clang-debug --jobs 6 --inventory qfileinfo-exists,range-loop-reference,no-module-include` on an export of `09b030e61` | Exit 0 in 161 s: `range-loop-reference` 15 findings in 6 files |
| `test-clazy.py ... --inventory range-loop-reference` on the fixed tree | Exit 0 in 148 s: **0 findings** in 153 sources |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 6` | **Exit 0** in 216 s: 33 fixtures rejected by their own checks, 153 sources with no finding from 33 checks |
| Same, with the previous `app/layouts/synchronizer.cpp` and `app/view/effects.cpp` mounted over the fixed files | **Exit 1** in 171 s: 10 `[range-loop-reference]` findings (8 and 2) in those two files, nothing else |
| `test-clazy.py ... --inventory range-loop-detach` on the fixed tree | Exit 0 in 128 s: 305 findings in 47 files, the count of [R2c](R2c.md); 11 of the 15 fixed loops are among them, at the same lines |
| Container `cmake --build ... -j6` (Clang 18.1.3, Qt 6.11.1, strict warnings) | Exit 0 in 26 s, 6 objects rebuilt, 0 compiler warnings |
| Host `cmake --build --preset gcc-debug -j6`, `ctest --preset gcc-debug` (GCC 16.2.1) | Exit 0 in 68 s (configure, incremental build, ctest), 7 objects rebuilt, 0 compiler warnings, 57/57 passed |
| Host `cmake --build --preset clang-debug -j6`, `ctest --preset clang-debug` (Clang 22.1.8) | Exit 0 in 54 s (configure, incremental build, ctest), 7 objects rebuilt, 0 compiler warnings, 57/57 passed |

The build logs hold thousands of `Warning:` lines from the qmllint step; they
are the known QML warnings tracked by the baseline gate, not compiler output,
and this change touches no QML.

## Handoff

- **Files and behavior changed:** the 6 production sources of the table (15
  loops), `scripts/test-clazy.py` (one name),
  `autotests/clazy/range-loop-reference.cpp`. No observable change.
- **State authority / lifetime constraints:** each loop variable now refers to
  the element in its range instead of a copy. Every range is a local, a
  parameter, a temporary kept alive by the loop, or a member that the body does
  not modify (table above). A later edit that mutates the range inside one of
  these loops must copy the element first.
- **Tests:** no test added or changed; `ctest` 57/57 on GCC and Clang Debug.
  The fixture and the mutation run are the negative evidence.
- **Not run:** the CI job, Release presets, the desktop retest of AGENTS.md
  (there is no behavior change; the main session runs one combined smoke after
  both batches merge).
- **Next exact action:** review, then the next slice of this batch,
  `R2c-no-module-include`.
- **Commit approval status / push approval status:** committed on the feature
  branch as the batch brief allows; not pushed; `main` untouched.
