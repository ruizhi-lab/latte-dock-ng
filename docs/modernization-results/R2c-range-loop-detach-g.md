# R2c-range-loop-detach (group G): Range Loops Iterate a Const View

Status: Complete (CI run 41452548625 and the host desktop smoke passed). Date: 2026-11-02.
Branch: `claude/r2c-batch-g-83e14a`.
Start HEAD: `95e342f72`. Implementation commit: the commit that adds this report. It amends
`d21913d34` (70 loops) and, before that, `26de6e1a2` (41 wraps).
Part of [R2c](R2c.md), fix batch 3: group G of the `range-loop-detach` slice. Batch G
changes the twelve files of this group, and one test file that the main session allowed:
`autotests/sourcecontracttest.cpp`, the text the `~View` order test searches for. Group H
covers the other files of the slice. Batches E and F change other checks and may touch
some files of this group. I did not look at their branches, so the merge is for the main
session.

All 74 findings in the twelve files are resolved, with the inventory at 0 for these files:

- 42 range expressions are wrapped in `std::as_const(...)` (table A), including
  `view.cpp:298`.
- 29 range expressions that call a function returning the container by value now hold the
  call's result in a const local (table B).
- One loop, `originalview.cpp:225`, is replaced by `QList::removeIf`. This is the only
  deliberate behavior change of the group; it has its own section.
- Two loops keep the live container and carry a same-line `clazy:exclude` with the path that
  can change it: `genericlayout.cpp:1364` and `genericlayout.cpp:1337`.

`range-loop-detach` is still not gated: 200 findings in 33 files of the other groups remain.

## Defect

clazy reports "c++11 range-loop might detach Qt container" for a range-based `for` over a
non-const Qt container. Such a loop calls the non-const `begin()` and `end()`. On a shared
container that deep-copies the container at the start of the loop. The finding is a cost
finding: I did not find a symptom, and the copy is only needed when the container is shared.

## Fix

- **42 wraps** (table A). The range expression became `std::as_const(...)`, for example
  `for (const auto view : m_latteViews)` became `for (const auto view : std::as_const(m_latteViews))`.
  Nothing else in these loops changed. No include was added: the codebase already uses
  `std::as_const` without `<utility>` (`app/layouts/storage.cpp` does).
- **29 rewrites** (table B). `std::as_const()` cannot bind a temporary, so a range that is a
  call returning the container by value cannot be wrapped. The call moves into a const local
  in the C++20 range-for init-statement:
  `for (const auto applets = m_view->containment()->applets(); const auto applet : applets)`.
  The local is const, so the loop iterates it through `begin() const` and nothing detaches.
  The local lives exactly as long as the loop, as the temporary did, and the call runs once,
  at the same point. The loop variable declaration and the body are unchanged. Each local is
  named after what the call returns; each name was checked against the enclosing function and
  the class header, and no name is used in a loop body or hides a member.
- **One replacement, with a behavior change**: [Behavior change](#behavior-change-originalviewcpp225).
- **Two exclusions** with their traced paths: [Exclusions](#exclusions).
- **One test text**: `autotests/sourcecontracttest.cpp:1401` searches for the wrapped header
  `for (QWindow *window : std::as_const(m_pointerWindows)) {` instead of the old one. The
  order checks of `viewDestructorDropsPointerWindowTrackingConnections` are unchanged.
- Every loop was read: its body, and each callee that touches the same container. A loop
  changes only when neither the body nor a callee can change the container it iterates.

## Table A: wrapped in `std::as_const` (42)

Line numbers are those of the start HEAD `95e342f72`.

| File:line | Container | Why the body does not modify it |
| --- | --- | --- |
| `app/layout/genericlayout.cpp:69` | `m_latteViews` (member) | `disconnectSensitiveSignals()` stops a timer, disconnects four signals and calls `setLayout(nullptr)`, which only disconnects the view's own connections and clears its pointer and activities. No view class overrides it. No layout hash is touched |
| `app/layout/genericlayout.cpp:73` | `m_waitingLatteViews` (member) | same body |
| `app/layout/genericlayout.cpp:82` | `m_containments` (member) | body reads `parent()` and appends to the local `subcontainments` |
| `app/layout/genericlayout.cpp:120` | `m_latteViews` (member) | same body as `:69` (`unloadLatteViews()`) |
| `app/layout/genericlayout.cpp:124` | `m_waitingLatteViews` (member) | same body as `:69` |
| `app/layout/genericlayout.cpp:462` | `m_latteViews` (member) | body reads `isOriginal()` and appends to the local `viewslist` |
| `app/layout/genericlayout.cpp:672` | `appletsWithShortcuts` (local) | body only fills the local `views`, through a nested loop |
| `app/layout/genericlayout.cpp:673` | `m_latteViews` (member) | body fills the local `views`; its nested loop at `:676` is a rewrite (table B) |
| `app/layout/genericlayout.cpp:837` | `m_containments` (member) | body writes the `layoutId` entry of each containment's config and logs |
| `app/layout/genericlayout.cpp:1059` | `containments` (by-value parameter) | body writes config and connects signals; a parameter is not reachable from a callee |
| `app/layout/genericlayout.cpp:1093` | `m_containments` (member) | body appends to the local `containments` and disconnects signals |
| `app/layout/genericlayout.cpp:1107` | `containments` (local, `:1085`) | body removes from `m_containments`, a different container |
| `app/layout/genericlayout.cpp:1270` | `m_containments` (member) | body only reads: `isLatteContainment()`, `hasLatteView()`, `Storage::view()` (reads the config and `layout->containments()`), `expectedViewScreenId()`; it fills the local `map` |
| `app/layout/genericlayout.cpp:1319` | `clearpendings` (local, `:1305`) | body removes ids from `m_pendingContainmentUpdates`, a different container |
| `app/layout/genericlayout.cpp:1347` | `m_latteViews` (member; the element is copied by `auto view`) | body reads `containment()`, `isOriginal()` and `mapContainsId()` and appends to the local `viewsToDelete`. The loop variable keeps `auto` |
| `app/view/containmentinterface.cpp:154` | `searchPaths` (local) | body builds `QDir` and `QDirIterator` objects and returns |
| `app/view/containmentinterface.cpp:196` | `candidates` (local) | body calls `iconExistsInCurrentTheme()`, which cannot reach the local |
| `app/view/containmentinterface.cpp:880` | `m_appletOrder` (member) | body reads and writes `m_appletData[id]` only; `appletConfiguration()` (`:2219`) does not touch `m_appletOrder`; no call to `setAppletsOrder()`, `removeApplet()` or `updateAppletsOrder()` |
| `app/view/containmentinterface.cpp:1900` | `m_appletOrder` (member) | body reads `indexOf()` and writes `m_appletData[id].lastValidIndex`, a different container |
| `containment/plugin/layoutmanager.cpp:967` | `applets` (local, `:958`) | body reads trash state (`isTrashWidget()`, `appletId()`) and fills the local `trashIds` |
| `containment/plugin/layoutmanager.cpp:980` | `trashIds` (local, `:957`) | body fills `applets`, a different container |
| `containment/plugin/layoutmanager.cpp:1193` | `appletIdsOrder` (local, `:1156`) | body calls `isValidApplet()` only |
| `containment/plugin/layoutmanager.cpp:1207` | `applets` (local, `:1166`) | body fills `appletIdsOrder`, a different container |
| `containment/plugin/layoutmanager.cpp:2103` | `applets` (local, `:2097`) | body calls `appletId()`, `appletItem()`, `addAppletItem()` and a QML method; none can reach the local |
| `containment/plugin/layoutmanager.cpp:2802` | `splittersToDelete` (local, `:2780`) | body calls `deleteLater()` on each item |
| `containment/plugin/layoutmanager.cpp:2907` | `items` (local, `:2904`) | body fills the local `list` |
| `app/view/originalview.cpp:171` | `m_clones` (member) | body calls `Positioner::setNextLocation()` on the clone. Its relocation signals reach positioner slots, the config-window hide and QML visibility handlers. A non-animated move calls `LayoutsManager::moveView()`, which unassigns the clone's containment from one layout and assigns the same view object to another (`assignToLayout()` creates no view). `locationChanged` reaches `Corona::viewLocationChanged`, whose receivers only start a timer or re-emit `View::dockLocationChanged`, which has no receiver. None of this reaches `addClone()`, `removeClone()`, `cleanClones()`, `syncClonesToScreens()`, `reconsiderScreen()` of this view or the destructor |
| `app/view/originalview.cpp:186` | `m_clones` (member) | body calls `ContainmentInterface::addApplet()` of each clone, which creates an applet. Its signals reach `ClonedView`'s handlers, which call `OriginalView::addApplet()` again (this only reads `m_clones`) and `onOriginalAppletsOrderChanged()`, the layout's `appletCreated()`, which only appends containments, and `onAppletAdded()`, which connects signals and fills the task models. None reaches the mutators |
| `app/view/originalview.cpp:206` | `m_clones` (member) | same as `:186` |
| `app/view/originalview.cpp:238` | `m_clones` (member) | body reads `currentScreenId()` (a getter) and fills the local `removable` |
| `app/view/originalview.cpp:248` | `secondaryscreens` (local) | body calls `setScreenToFollow()` or `createClone()`; no callee can reach the local |
| `app/view/originalview.cpp:259` | `removable` (local) | body calls `removeClone()`, which changes `m_clones`, not `removable` |
| `app/view/clonedview.cpp:345` | `originalids` (local, `:327`) | body fills the locals `nextAppletIds` and `usedClonedIds` |
| `app/view/clonedview.cpp:363` | `originalids` (local, `:327`) | same body |
| `app/view/clonedview.cpp:375` | `clonedids` (local, `:328`) | body fills the same two locals and breaks out of the inner loop |
| `app/view/settings/subconfigview.cpp:120` | `connections` (member) | body disconnects each connection |
| `app/view/settings/subconfigview.cpp:124` | `viewconnections` (member) | same body |
| `app/view/settings/subconfigview.cpp:222` | `viewconnections` (member) | body disconnects each connection; the list is refilled after the loop |
| `app/view/view.cpp:298` | `m_pointerWindows` (member, `QSet`, in `View::~View()`) | body only disconnects each window; `clear()` follows the loop. The pinned test text is updated (see Fix) |
| `app/view/effects.cpp:446` | `m_subtractedMaskRegions` (member, `QHash<QString, QRegion>`) | body combines into the local `region` |
| `app/view/effects.cpp:450` | `m_unitedMaskRegions` (member, `QHash<QString, QRegion>`) | same body |
| `app/view/indicator/indicatorresources.cpp:49` | `paths` (by-value parameter) | body creates `KSvg::Svg` objects and appends them to the member `m_svgs`, a different container |

## Table B: rewritten to a const local in the range-for init-statement (29)

The range expression is a call that returns the container by value. The call is held in a
const local named after what it returns; the loop variable declaration and the body are
unchanged. The local holds the same values in the same order as the temporary did, so the
loop visits the same elements. Only the constness of the iteration changes.

| File:line (base) | Call | Local |
| --- | --- | --- |
| `app/layout/genericlayout.cpp:676` | `view->containment()->applets()` | `applets` |
| `app/layout/genericlayout.cpp:751` | `m_corona->containments()` | `coronaContainments` (`containments` is a member function of this class) |
| `app/layout/genericlayout.cpp:950` | `latteViews()` | `views` |
| `app/layout/genericlayout.cpp:959` | `latteViews()` | `views` |
| `app/layout/genericlayout.cpp:1004` | `m_corona->containments()` | `coronaContainments` |
| `app/layout/genericlayout.cpp:1388` | `applets.groupList()` | `groups` |
| `app/layout/genericlayout.cpp:1410` | `applets.groupList()` | `groups` |
| `app/view/containmentinterface.cpp:148` | `QStandardPaths::standardLocations(...)` | `dataPaths` (the head breaks after the `;`) |
| `app/view/containmentinterface.cpp:582` | `v.toArray()` (inside a one-line lambda) | `entries` |
| `app/view/containmentinterface.cpp:629` | `v.toArray()` (inside a one-line lambda) | `entries` |
| `app/view/containmentinterface.cpp:773` | `m_view->containment()->applets()` | `applets` |
| `app/view/containmentinterface.cpp:788` | `m_view->containment()->applets()` | `applets` |
| `app/view/containmentinterface.cpp:822` | `m_view->containment()->applets()` | `applets` |
| `app/view/containmentinterface.cpp:1964` | `m_view->containment()->applets()` | `applets` |
| `app/view/containmentinterface.cpp:2025` | `m_view->containment()->applets()` | `applets` |
| `app/view/containmentinterface.cpp:2160` | `m_view->containment()->applets()` | `applets` |
| `app/view/containmentinterface.cpp:2383` | `subContainment->applets()` | `subApplets` |
| `containment/plugin/layoutmanager.cpp:960` | `containment->applets()` | `containmentApplets` (`applets` is declared in the same function) |
| `containment/plugin/layoutmanager.cpp:1060` | `containment->applets()` | `applets` |
| `containment/plugin/layoutmanager.cpp:1546` | `containment->applets()` | `applets` |
| `containment/plugin/layoutmanager.cpp:2224` | `containment->applets()` | `applets` |
| `app/view/positioner.cpp:415` | `qGuiApp->screens()` | `screens` |
| `app/view/positioner.cpp:423` | `qGuiApp->screens()` | `screens` |
| `app/view/positioner.cpp:447` | `qGuiApp->screens()` | `screens` |
| `app/view/positioner.cpp:1039` | `qGuiApp->screens()` | `screens` |
| `app/view/view.cpp:890` | `qApp->topLevelWindows()` | `windows` |
| `app/view/view.cpp:2054` | `plugin->contextualActions()` | `contextualActions` |
| `app/view/eventssink.cpp:287` | `m_originParentItem->childItems()` | `childItems` |
| `app/view/visualitemsearch.h:48` | `item->childItems()` | `childItems` |

## Behavior change: `originalview.cpp:225`

This is the only deliberate behavior change of group G. The loop was replaced by one call.

**Old behavior** (base `95e342f72`, `originalview.cpp:225-229`):

```cpp
for (const auto scrid : secondaryscreens) {
    if (m_waitingCreation.contains(scrid)) {
        secondaryscreens.removeAll(scrid);
    }
}
```

The loop erases from the list it iterates. An erase shifts the ids after it one place
forward, while the range-for has already moved on to the next position. So the id that
moved into the erased slot is not examined, and if it is waiting it stays in
`secondaryscreens`. The erase also leaves stale copies of the last ids past the end of the
shrunk list. The loop does visit those positions, so a skipped waiting id is sometimes
caught there and sometimes not.

**New behavior**:

```cpp
//! Removes every waiting screen in one pass; erasing inside a range-for skips the id after each erased one.
secondaryscreens.removeIf([this](int scrid) { return m_waitingCreation.contains(scrid); });
```

`removeIf` erases every id that is in `m_waitingCreation` in one pass, so the list holds only
ids that are not waiting. `m_waitingCreation` is a `QList<int>` (`originalview.h:67`), and
`QList::removeIf` is in the installed Qt 6.11 headers (`qlist.h`); the brief gives Qt 6.1 as the first version with it, so
the Qt 6.6 floor is covered.

**When they differ.** The old loop keeps a waiting id when it directly follows an id that the
loop erased, and no later stale copy visits it. The new code never keeps one. Examples, with
the ids in list order and W for waiting:

| List | Waiting | Old result | New result |
| --- | --- | --- | --- |
| `[A, B, C, D]` | A, B | `[B, C, D]` | `[C, D]` |
| `[A, B, C]` | A, B | `[B, C]` | `[C]` |
| `[A, B, C, D]` | B, C | `[A, C, D]` | `[A, D]` |
| `[A, B, C]` | B, C | `[A]` | `[A]`: the stale copy of C catches it, no difference |
| `[A, C, B, D]` | A, B | `[C, D]` | `[C, D]`: no two waiting ids are adjacent, no difference |

Where the two differ, the old result keeps a screen that is waiting for its own clone.

**What follows in `syncClonesToScreens()`.** The remaining ids are used by the loop at
`originalview.cpp:248-257`:

- If a removable clone is waiting to be reused (`removable` is not empty), the old code moves
  that clone onto the leftover waiting screen (`setScreenToFollow()`). The new code does not
  move it there. A removable clone with no other target is removed, as before, by the loop at
  `originalview.cpp:259-262`.
- Otherwise the old code calls `createClone()` for the leftover id. `createClone()` returns at
  once, because `m_waitingCreation` already holds the id (`originalview.cpp:138-141`), so no
  view is created. There is no difference in that branch, except that the template is read.

The new code does what the loop was written to do: a screen that is waiting for its clone is
not given another one. I did not run the old code, so the consequence of moving a clone onto a
screen whose clone is still being created is read from the code, not observed.

**Desktop smoke for this change.** Use a dock in "All secondary screens" mode and two
secondary screens that get a clone at the same time (for example, both connected together,
or a layout change while a clone is still being created). Check that each secondary screen
ends with exactly one clone, and that no clone is moved onto a screen whose clone is still
being created.

## Exclusions

Both exclusions use the same-line form `// clazy:exclude=range-loop-detach`, with the path
and the reason in `//!` lines above the loop. Each path was traced by reading the code; the
line numbers below are those of the base `95e342f72`.

### `genericlayout.cpp:1364`: `syncLatteViewsToScreens()`, the loop over `m_latteViews`

The body calls `view->reconsiderScreen()` for original views. The verified path:

1. `reconsiderScreen()` of an original view is `OriginalView::reconsiderScreen()`
   (`originalview.cpp:159-163`), which calls `syncClonesToScreens()`.
2. `syncClonesToScreens()` calls `removeClone()` for a clone on a screen that is no longer
   used (`originalview.cpp:259-262`).
3. `removeClone()` calls `cloned->layout()->removeViewWithoutDestroyingContainment(...)`
   (`originalview.cpp:108`) when the clone has a layout.
4. `removeViewWithoutDestroyingContainment()` does `m_latteViews.take(viewcontainment)` for an
   active layout (`genericlayout.cpp:1597`).

So the body can change the hash it iterates. The loop iterates the live hash, as before. A
snapshot or a restart would change what it visits, which is a behavior decision for a later
change.

### `genericlayout.cpp:1337`: `syncLatteViewsToScreens()`, the loop over `m_containments`

The body calls `addView(containment)`, and I traced the path to its end. Each link was read
in the code:

1. `addView()` calls `latteView->setLayout(this)` (`genericlayout.cpp:928`).
2. `View::setLayout()` emits `layoutChanged` (`view.cpp:1512`).
3. `OriginalView::syncClonesToScreens()` is connected to `View::layoutChanged` in the
   `OriginalView` constructor (`originalview.cpp:29`).
4. `syncClonesToScreens()` calls `createClone(scrid)` for a secondary screen that has no clone
   and is not waiting (`originalview.cpp:255`), when the screens group is not single
   (`originalview.cpp:218`).
5. `createClone()` calls `layout()->newView(nextdata)` (`originalview.cpp:140`).
6. `GenericLayout::newView()` calls `Layouts::Storage::self()->newView(this, ...)`
   (`genericlayout.cpp:1491`).
7. `Storage::newView()` calls `importLayoutFile(destinationLayout, temp2File)` for an active
   layout (`storage.cpp:840-842`).
8. `Storage::importLayoutFile()` calls `importContainmentsToCorona()` (`storage.cpp:751-755`),
   which ends with `ImportedContainments::restore()` (`storage.cpp:747`).
9. `restore()` emits `Corona::containmentAdded` for each imported containment
   (`importedcontainments.cpp:44`).
10. `containmentAdded` is connected to `GenericLayout::addContainment()` in `initCorona()`
    (`genericlayout.cpp:975`).
11. `addContainment()` appends the containment to `m_containments` (`genericlayout.cpp:706`
    in single-layout memory; `:712` in multiple-layouts memory when the containment's
    `layoutId` is this layout's name). `newUniqueIdsFile()` writes that `layoutId` for the
    destination (`storage.cpp:503`).

**Preconditions**, each read in the code: the layout is active (`hasCorona()`); a
containment of an active layout has a non-empty `storedView()` (`storage.cpp:1893-1898`), so
`createClone()` gets past its guard; the screens group is not single; and a secondary screen
has no clone and is not waiting.

**Every change to `m_containments`** in `genericlayout.cpp`, from grep: lines 92 and 99
(`unloadContainments`), 706 and 712 (`addContainment`), 773 (`containmentDestroyed`), 1057
(`assignToLayout`), 1108 (`unassignFromLayout`) and 1598 (`removeViewWithoutDestroyingContainment`).
`m_containments` is a protected member of `GenericLayout` (`genericlayout.h:174`). In `app/`, `containment/`, `containmentactions/`, `plasmoid/`, `declarativeimports/` and `autotests/`, only `genericlayout.cpp` names it, and the only subclass, `CentralLayout` (`centrallayout.h:36`), does not. So these functions are the only places that change it; other files reach them through calls. The callers of
`addContainment()` are `appletCreated()` (`:756`), `initContainments()` (`:1015`, `:1020`) and
the Corona connection (`:975`). Nothing else in the grep touches `m_containments`.

**Not checked.** I did not reproduce the path in a running session, and I did not read Plasma's
internals. The exclusion covers every change to the list, not only this path.

## Notes for the main session

1. **The twelve files are at 0; the gate still cannot take the check.** The final inventory
   shows 200 findings in 33 files, all in the other groups. `range-loop-detach` can be gated
   when those are done.
2. **Review the three non-transform changes**: the `removeIf` replacement at
   `originalview.cpp:225` (the behavior change above) and the two exclusions at
   `genericlayout.cpp:1337` and `:1364`. The exclusion comments are the reasons on record.
3. **The one extra file**, `autotests/sourcecontracttest.cpp`, changes only the header text of
   `viewDestructorDropsPointerWindowTrackingConnections` (line 1401). Its order checks are
   unchanged, and the test passes on both presets.
4. **Desktop smoke**: the scenarios for the behavior change are in its section. The rest
   of the smoke is as before: startup, adding, moving and removing widgets, a layout switch,
   an original dock with a clone, and a clone relocation (the signal chains behind
   `originalview.cpp:171`, `:186` and `:206` were traced in C++ only; no QML file or
   `Q_INVOKABLE` reaches the four clone functions, which I checked by grep).

## Evidence

Container commands are those of [R2c](R2c.md#reproducing-the-environment), with the checkout
mounted read-only, the build tree on `$HOME/r2c-build-batch-g` and `-j4` / `--jobs 4`. The
container image is `latte-r2c-clazy:noble`, ID `8e9528e34f50`, the image of R2c (clazy 1.17 on
LLVM 19.1.1, Ubuntu clang 18.1.3, Qt 6.11.1, [recorded in R2c](R2c.md#environment)). The host
runs CMake 4.3.4, GCC 16.2.1, Clang 22.1.8 and Qt 6.11.2. Logs are in `$HOME/r2c-build-batch-g/`
and the baseline tree in `$HOME/r2c-build-batch-g-baseline/`, outside the repository. The scripts
are in `$HOME/r2c-build-batch-g/scripts-batch-g/`.

| Command | Exit | Result |
| --- | ---: | --- |
| Baseline re-run from a clean `git archive 95e342f72` export (not the worktree): configure, build `-j4`, `test-clazy.py --inventory range-loop-detach` | 0 | 0 `warning:` lines. **274 findings in 45 files**; in the twelve files 74, per file as before. Log: `$HOME/r2c-build-batch-g-baseline/baseline-recheck.log` |
| The five edits of this step, reviewed with `git diff HEAD` | — | Only the intended lines changed: `view.cpp:298` wrapped; the test header text at `sourcecontracttest.cpp:1401`; `originalview.cpp:225-229` replaced by the one-line `removeIf` and a one-line `//!` comment; the two exclusions with their `//!` reasons at `genericlayout.cpp:1337` and `:1364` |
| `verify_transforms.py` against `95e342f72` (wraps, rewrites, unexplained hunks) | 0 | 73 hunks: 15 wraps, 55 rewrites, **3 unexplained, as intended**: the `removeIf` and the two exclusion hunks |
| Container, final state: `cmake --build --preset clang-debug -j4` | 0 | 4 objects compiled (the four changed sources); **0 `warning:` lines** |
| Container, final state: `test-clazy.py --inventory range-loop-detach --report build/rld-final2.json` | 0 | **200 findings in 33 files** (274 − 74). **In the twelve files: 0** |
| Container, final state: `test-clazy.py --build-dir build/modernization/clang-debug --jobs 4` (the full gate) | 0 | "43 negative fixtures: each is rejected by its own gated check"; "153 production sources: no findings from 43 gated clazy checks" |
| Host GCC Debug, final state: `cmake --preset gcc-debug`, `cmake --build --preset gcc-debug -j4`, `ctest --preset gcc-debug` | 0 / 0 / 0 | **0 `warning:` lines**, 0 CMake warnings, 4 objects compiled. **58 of 58 passed**, including `sourcecontracttest` |
| Host Clang Debug, final state: `cmake --preset clang-debug`, `cmake --build --preset clang-debug -j4`, `ctest --preset clang-debug` | 0 / 0 / 0 | **0 `warning:` lines**, 0 CMake warnings, 10 objects compiled. **58 of 58 passed**, including `sourcecontracttest` |

The capitalised `Warning:` lines in the host build logs are qmllint output for QML files, the
known QML warnings of the baseline gate, not compiler output. The four `error:` matches in the
container build are the QML text `console.warn("ContextMenu component load error:")`. The 58
tests include `appstreamtest`, which ECM adds when `appstreamcli` is installed.

## Not run

- CI (`.github/workflows/build.yml`).
- The desktop smoke (note 4). The brief assigns it to the main session after the merge.
- The GCC and Clang Release presets; the brief asks for Debug.
- The Qt 6.6 and 6.7 floor, and the distribution package installs. `removeIf` exists from Qt
  6.1 (the brief's figure; the installed Qt 6.11 header has it), and `std::as_const()` and the C++20 init-statement
  are standard, but the builds here used Qt 6.11.1 (container) and 6.11.2 (host) only.

## Handoff

- **Batch / slice and selected scope:** R2c fix batch 3, group G of `range-loop-detach`: twelve
  files and 74 findings. 74 resolved in the twelve files (42 wraps, 29 rewrites, 1 replacement,
  2 exclusions), and one extra file, `autotests/sourcecontracttest.cpp` (test text only).
- **Status:** Complete; CI run 41452548625 and the host desktop smoke passed
  ([R2c](R2c.md#fix-batch-3)).
- **Source baseline / actual start HEAD / implementation HEAD or uncommitted diff:**
  `95e342f72` / `95e342f72` / the commit that amends `d21913d34`, with this report.
- **Branch / existing user edits preserved:** `claude/r2c-batch-g-83e14a` in its own worktree.
  The worktree had only the commit `d21913d34` before this step. The `build/` mount point is
  git-ignored.
- **Dependencies and evidence links:** [R2c](R2c.md), row `R2c-range-loop-detach`;
  [R2c-range-loop-reference](R2c-range-loop-reference.md) and
  [R2c-container-anti-pattern](R2c-container-anti-pattern.md).
- **Files and externally observable behavior changed:** twelve production sources:
  `app/layout/genericlayout.cpp`, `app/view/containmentinterface.cpp`,
  `containment/plugin/layoutmanager.cpp`, `app/view/originalview.cpp`,
  `app/view/clonedview.cpp`, `app/view/settings/subconfigview.cpp`, `app/view/effects.cpp`,
  `app/view/indicator/indicatorresources.cpp`, `app/view/positioner.cpp`, `app/view/view.cpp`,
  `app/view/eventssink.cpp` and `app/view/visualitemsearch.h`; the test
  `autotests/sourcecontracttest.cpp`; and this report. **One behavior change**:
  `originalview.cpp:225`, with its section above. The other changes keep behavior.
- **State authority / invalidation / lifetime / fallback constraints:** `secondaryscreens` is a
  local; `m_waitingCreation` is a member read by the predicate. The exclusions keep the live
  iteration of `m_latteViews` and `m_containments`.
- **Toolchain, import environment and artifact identity:** the container and host toolchains
  above; container image ID `8e9528e34f50`.
- **Commands executed with exit status and durable log/artifact locations:** see Evidence.
- **GCC/Clang Debug/Release warning/error results:** 0 compiler warnings on GCC Debug and Clang
  Debug (host), and 0 on the container's Clang Debug. Release presets were not built.
- **Tests registered / run / passed / skipped / failed and name deltas:** 58 registered on both
  host presets; 58 run, 58 passed, 0 skipped, 0 failed on GCC and Clang Debug.
  `sourcecontracttest` passes on both. No test was added or removed; one test's searched text
  changed (`sourcecontracttest.cpp:1401`).
- **Production behavior vs copied logic vs source-contract evidence:** production code. The gate
  runs clazy on the production sources. One source-contract test matches an exact loop text, and
  its text was updated.
- **QML diagnostics added/removed and environment compatibility:** none. No QML changed.
- **Negative fixtures or controlled mutation evidence:** the 43 gate fixtures are unchanged and each
  is rejected. The `view.cpp:298` wrap was tested in the first host run (the test failed), then
  updated.
- **Desktop user feedback / log review / clean-quit and coredump evidence:** the host smoke on
  `6b51b43a6` passed with no warning from Latte's code and no core ([R2c fix batch
  3](R2c.md#fix-batch-3)). It reached startup with two docks (`positioner.cpp`, `view.cpp`,
  `containmentinterface.cpp`, `layoutmanager.cpp`), two layout switches that destroyed and
  recreated the docks (`~View`, `syncLatteViewsToScreens()`) and the dock settings window in
  edit mode (`subconfigview.cpp`). It did not reach the `removeIf` change in
  `originalview.cpp:225`, which needs a dock in "All secondary screens" mode on a desktop
  with two secondary screens, so that change is still read from the code and not observed.
- **Performance paired samples / variability / work counts / tradeoffs:** not measured. Each changed
  loop no longer detaches a shared container at its start.
- **CI, distribution and Nix checks (passed / failed / unavailable):** run 41452548625 at
  `6b51b43a6` passed all 23 jobs after one rerun of `Install verify (mageia)`, which had failed
  on the Mageia Cauldron mirror. `Build (Clang Debug)` ran the 49-check gate and 57 tests, and
  the Debian 13 (Qt 6.8.2) install job and the Nix check passed. See
  [CI validation](R2c.md#ci-validation).
- **Unresolved failures, review assumptions and unavailable checks:** the 200 findings in the other
  groups; the `removeIf` consequence is read from the code, not observed; the exclusion paths were
  traced in code, not reproduced; the QML handlers behind `originalview.cpp:171`, `:186` and `:206`
  were not read.
- **Next exact action, file/symbol or command; do not start another batch implicitly:** none.
  The main session reviewed the three non-transform changes and merged the group, and
  `range-loop-detach` joined the gate in `8a30d3fa9`.
- **Commit approval status / push approval status:** amended into one commit on
  `claude/r2c-batch-g-83e14a`; merged into the integration branch in `c0f64b9ee` by the main
  session. `main` was fast-forwarded to `6b51b43a6` and pushed on 2026-11-02 with the user's
  approval.
