# R2c "Lost intent" review (read-only)

Status: Findings recorded on 2026-11-03. F1 to F4 were fixed afterwards (F1 in `f03b319a3`,
F2 to F4 in `00aaf6ad5`; see [R2c, Follow-up fixes](R2c.md#follow-up-fixes)); F5 is open and
needs a decision. The review itself ran and reproduced nothing: every claim below comes from
reading the code and the upstream history, so the runtime outcomes of F1 to F4 are inferred,
and none of the four defects was reproduced on a desktop build before its fix. Part of
[R2c](R2c.md), follow-up 7. A read-only Claude Haiku 5.5 session wrote it; the main session
read it but did not re-verify its traces. Line numbers are those of `main` at `61963df10`.

Scope: the eight items in the "Lost intent" section of
`docs/modernization-results/R2c-unused-non-trivial-variable.md`, from deletion
commit `cec126a31` (main at `61963df10`). Read-only: no file changed, no branch,
no commit, no build, no app run. Every claim below is from reading the code and
git history; the runtime effects are NOT reproduced. Re-checked on 2026-11-03 at
`61963df10` (clean tree, no later commit touches the reviewed paths); see
"Verification status".

## Verdict per item

| # | Item (file) | Verdict | Evidence |
| - | ----------- | ------- | -------- |
| 1a | `knownActivities` in `Layouts::save()` | Deliberate removal; its consequence is finding F1 | filter removed in `c7d163fdb` (2020-08-26); runtime-only check added in `a02056b4c` (2020-08-31) and the comment at `synchronizer.cpp:749` |
| 1b | `switchToLayout` in `Layouts::save()` | Harmless leftover | assignment removed in `7393ac02e` (2020-08-27); post-save switch replaced by `nextSingleLayoutName` in `22fa4c8fc` (2021-02-14); `layoutscontroller.cpp:1040-1052` |
| 2 | `layoutName` in `Positioner::immediateSyncGeometry()` | Harmless leftover | the argument of `availableScreenRegionWithCriteria` became `activityid` in `3689cc406` (2020-08-25); `positioner.cpp:551-566` |
| 3 | `oldId` in `Layouts::setId()` | Harmless leftover, but it marks the lost id notification behind F2/F4 | the loop that used it updated `shares` lists; removed with the shared-layouts feature in `caf9bf7b7` (2020-08-25); no `shares` symbol remains in `app/` |
| 4 | `idForConnector` in `ScreenPool::load()` | Harmless leftover | snapshot added in `deb9c244f` (2020-04-24), never compared; `updated` depends only on `connectorForId` (`screenpool.cpp:78-86`) |
| 5 | outer `pluginId` in `ContainmentInterface::onAppletAdded()` | Harmless leftover | added in `452fa2d82` (2026-07-15), shadowed by the else-branch declaration (`containmentinterface.cpp:2309`); the sibling `addedAppletId` of the same commit was removed in the warning-cleanup commit `70103ece0`, the QString was not |
| 6 | `allactivities` in `Synchronizer::switchToLayoutInMultipleModeBasedOnActivities()` | Harmless leftover | declared in `a83d26a35` (2020-08-30), never read; `validActivities()` (`8448f36ed`, same day) does the same check |
| 7 | `activeNames` / `currentNames` in `Menu::populateMoveToLayouts()` | Lost UI indication (F5), not a functional defect | bold and "[Current]" removed in `69ec3d937` (2021-05-15); `isCurrent` dead until `4674a4e2f` (2026-07-31); `menu.cpp:436-441` |
| 8 | `cid` in the storage id-conflict checks | Harmless leftover | unused in the discovery loops since `2be67b11c` / `11aa2abd2` (2021-04-29); the report loops keep their own `cid` (`storage.cpp:1069-1081`, `1167-1188`) |

The deletion itself is behavior-neutral for all eight: every deleted initializer
is side-effect free. The real defects sit next to the deleted locals, in the
rename, lock and activity-display code.

## Findings (ranked by user impact)

### F1 - Activities column reads outside the activity table for an unknown activity id (high impact, medium likelihood, high confidence on the code path)

- Outcome: fixed in `f03b319a3` as proposed: both `GenericTable` lookups by id (the const
  and the non-const `operator[]`) return a default record, and `paint()` skips ids that have
  no row. The optional pruning in `Layouts::onActivityRemoved` was not done. `dataunittest`
  and `sourcecontracttest` cover it, and the Layouts page opened on the desktop with a
  layout that names an unknown activity id.
- Code path:
  - `app/settings/settingsdialog/delegates/activitiesdelegate.cpp:316` (in `paint()`):
    `assignedActivities << allActivitiesTable[assignedIds[i]];`, with no `containsId` guard.
    The local `allActivitiesTable` (line 313) is non-const, so the non-const overload runs.
  - `app/data/generictable.cpp:92-104` (non-const) and `:107-119` (const) both return
    `m_list[pos]` with `pos == -1` when the id is missing.
  - `/usr/include/qt6/QtCore/qlist.h:516-521`: the non-const `QList::operator[]` asserts the
    index (`Q_ASSERT_X` at 518); the const overload calls `at()` (`:511-515`), which asserts at 513.
    Without the assert the read is out of range.
  - The delegate is installed on `ACTIVITYCOLUMN` at `layoutscontroller.cpp:151`.
  - The unguarded lookup has been there since `dfbf83b4a` (2020-08-29); `containsId` does not appear in `activitiesdelegate.cpp`.
- Why item 1 leads here: the save-time filter dropped unknown ids on Apply until `c7d163fdb`.
  Since then `Layouts::save()` writes the list back unchanged (`layoutscontroller.cpp:964`).
  The only live cleanup left is `Synchronizer::onActivityRemoved()` (`synchronizer.cpp:447-462`),
  which removes a deleted activity only from layouts the synchronizer has assigned to it, while
  Latte runs. Ids from other machines and ids deleted while Latte was stopped persist, and the
  display side was never made to tolerate them.
- Triggers (state -> result):
  - (a) A layout file names an activity this machine does not have: an activity deleted while
    Latte was not running, or a layout shared from another machine (the case the comment at
    `synchronizer.cpp:749` protects). Opening Preferences > Layouts paints that row and performs the lookup.
  - (b) The Preferences dialog is a session singleton (`manager.cpp:461-470`). If it was created
    before an activity assigned to a layout was deleted while Latte runs, the synchronizer cleans
    its own table and files, but the open settings model keeps the id:
    `Layouts::onActivityRemoved` (`layoutsmodel.cpp:962-974`) drops the id from the activity table only.
  - The Activities menu has no action for an unknown id: its list is `ALLACTIVITIESSORTEDROLE`
    (pseudo ids plus `synchronizer()->activities()`, `layoutsmodel.cpp:462-468`), and the menu loop
    skips invalid entries (`activitiesdelegate.cpp:68-73`). The user cannot uncheck the id either:
    `setModelData` only removes ids of menu actions (`activitiesdelegate.cpp:275-282`).
- What the user sees: in builds with Qt assertions enabled (Debug), an assertion abort when the
  Layouts page is painted. In Release the read is undefined; a crash or garbage text is possible (not run).
- Confidence: high that the read is out of range; medium on the runtime outcome.
- Smallest fix: guard the lookup in `paint()` (`if (allActivitiesTable.containsId(assignedIds[i]))`,
  or show the raw id as "(unavailable)"), and make both `GenericTable::operator[]` overloads that take a `QString`
  return `T()` for a missing id (this fixes the root for every caller, including F4). Optionally prune
  unknown ids in `Layouts::onActivityRemoved`.
- Test: `autotests/dataunittest.cpp` (target at `autotests/CMakeLists.txt:23-44`): an
  `ActivitiesTable` with one row; a lookup of a missing id returns an empty `Activity`. A
  source-contract line in `sourcecontracttest.cpp` for the guard in `paint()`.

### F2 - Name rotation in one Apply corrupts layout ids in the Layouts model; a later Apply writes to the wrong layout file (high impact, low likelihood, high confidence on the mechanism)

- Outcome: fixed in `00aaf6ad5` as proposed: `newFileForOldId` is built first and
  `LayoutsTable::withRenamedIds()` rewrites every row from one snapshot. `dataunittest`
  covers a chain and a swap. On the desktop, a swap of two names in one Apply left the
  right files, and a later Apply in the same session used them.
- Code path: the id rewrite loop `layoutscontroller.cpp:992-1007`. For each renamed layout it calls
  `setData(IDCOLUMN, newFile)` on every row whose id equals `idInfo.oldId`, and that check reads the
  rows that earlier iterations already rewrote. `setId` (`layoutsmodel.cpp:629-640`, called from
  `:665`) changes ids in place. `alteredLayouts()` (`layoutsmodel.cpp:865-878`) looks rows up by id
  and gets the first match.
- Trigger: in one Apply, rename layout X to the current name of layout Y while Y is also renamed
  (a chain: Office->Home, then Work->Office; or a swap through a temporary name), then Apply.
- Trace (chain): before the loop, row0 = {Work-path, "Office"} and row1 = {Office-path, "Home"}.
  `idInfo0` (Work->Office-path) rewrites row0 to Office-path, so row0 and row1 now share
  Office-path. `idInfo1` (Office->Home-path) matches both rows and rewrites both to Home-path.
  The files on disk are correct after this Apply (the temp-directory passes are name based); the model is not.
- What the user sees: the first Apply looks right. The corrupt model stays until Latte restarts.
  - A later Apply in the same session that changes the first row (for example its icon) goes
    through `new CentralLayout(this, id)` (`layoutscontroller.cpp:945`) when that layout is not
    loaded (a loaded layout is found by name at `:944`), and writes the change into the other
    layout's file.
  - Edits to the second row are dropped: `alteredLayouts()` compares both rows with the first.
  - A rename of the first row also moves the other layout's file; a rename of the second row is ignored.
  - After restart a layout can carry another layout's content and name, or be missing from the list.
- Confidence: high on the mechanism (the loop and the id lookups are explicit); medium on the exact outcome (traced, not run).
- Smallest fix: build `QHash<oldId, newFile>` before mutating anything, then update each row once by
  looking up its own current id. Move the rewrite into a pure `Data::LayoutsTable` helper so it can
  be tested without a Corona. Take the synchronizer snapshot after the rewrite (see F4).
- Test: a `dataunittest` case with a two-layout rotation and a swap: ids stay unique and map to the
  expected files. Add the rotation to the manual smoke list.

### F3 - Renaming a locked layout silently unlocks it on Apply (medium impact, medium likelihood, high confidence)

- Outcome: fixed in `00aaf6ad5` as proposed: `LayoutFileLock` in `app/layouts/` locks by
  path, and `Layouts::save()` locks the file the layout has after the rename.
  `storagelogictest` covers it. On the desktop, a layout that was set read-only and renamed
  in one Apply was still read-only afterwards (mode 0444).
- Code path: the first loop unlocks a non-writable layout (`layoutscontroller.cpp:947-950`). The
  final loop re-locks it only through `layoutPtr`, which is looked up by the original name (`:1022`)
  or built from the pre-rename id (`:1025`). `Storage::lock()` (`storage.cpp:226-233`) changes
  permissions only when `layout->file()` exists.
  - Multiple-layouts mode: after `renameLayout()` (`genericlayout.cpp:825-838`) the original name no
    longer matches, so the lookup yields null, and the object built from the old id has no file.
  - Single-layout mode: `renameLayout()` returns early (`genericlayout.cpp:827-829`), so the loaded
    object still points to the moved file.
- Trigger: rename a locked layout, or lock and rename it in one Apply.
- What the user sees: after Apply the file is writable. `CentralLayout::data()` derives
  `isLocked = !isWritable()` (`centrallayout.cpp:144`), so after restart the layout is unlocked. In
  the same session the page keeps showing "locked" (`applyData()` at `layoutsmodel.cpp:195-205`
  does not recompute it), so the page disagrees with the file.
- Smallest fix: lock by the post-rename path (from `alteredIdsInfo`) through a path-based helper,
  extracted from `Storage::lock` / `unlock`, instead of object lookups.
- Test: a `storagelogictest`-style test in a `QTemporaryDir` (that file already isolates its data
  this way, `storagelogictest.cpp:108`): lock by path, rename, lock the new path, and assert that the
  new file is not writable. Add "rename a locked layout" to the smoke list.

### F4 - The synchronizer keeps pre-rename file ids; a D-Bus switch to a renamed inactive layout corrupts its table (medium-low impact, low likelihood, high confidence on the mechanism)

- Outcome: fixed in `00aaf6ad5` as proposed: the synchronizer gets the table taken after the
  rename, an empty stored layout is not written back, and the two `idForName()` lookups of
  explicit layouts are guarded with `containsName()`. Only source contracts cover it; the
  D-Bus path needs the multiple-layouts mode and was not exercised on a desktop.
- Code path: `layoutscontroller.cpp:924` takes `currentLayouts` before the rewrite, and `:1034`
  passes that copy to `Synchronizer::setLayoutsTable()` (`synchronizer.cpp:259-267`). The table keeps
  the old ids. Nothing refreshes it until restart: `initLayouts()` runs only at startup
  (`manager.cpp:127`), and `updateLayoutsTable()` matches by file path.
- Reachability: the context menu lists only loaded layouts in multiple-layouts mode
  (`lattecorona.cpp:1545-1546`), and `Positioner::setNextLocation` needs a loaded destination
  (`positioner.cpp:1181-1183`). The reachable path is the D-Bus method `switchToLayout`
  (`app/dbus/org.kde.LatteDock.xml:33-35`) -> `Corona::switchToLayout` (`lattecorona.cpp:1466`) ->
  `Synchronizer::switchToLayout` -> `switchToLayoutInMultipleMode` -> `switchToLayoutInMultipleModeBasedOnActivities`.
- Effects: `synchronizer.cpp:735-741` loads the stale path (missing, or another layout's file). Line
  740 then writes the result back into the row, even when it is empty, before the check at `:743-745`.
  A blank row later reaches `m_layouts[m_layouts.idForName(name)]` with an empty id, which is the
  index -1 read of F1, in `onActivityRemoved()` (`synchronizer.cpp:457-461`) and in the no-activities
  branch (`:799-803`).
- Smallest fix: pass `m_model->currentLayoutsData()` taken after the rewrite (depends on F2); store
  `layoutdata` at line 740 only when it is not empty; guard the `idForName` lookups with
  `containsName`; plus the F1 const-lookup fix.
- Test: source-contract guards in `sourcecontracttest.cpp`; the table part is covered by the F2 test.

### F5 - The Move To Layout menu no longer marks running layouts (low impact; decision needed)

- Outcome: open; the decision has not been made.
- Code path: `containmentactions/contextmenu/menu.cpp:436-441` (`populateMoveToLayouts`) only sets
  `setChecked(isViewCurrentLayout)`. The switch menu still checks running layouts (`populateLayouts`,
  `menu.cpp:381-388`).
- History: bold and "[Current]" were removed from the move-to menu in `69ec3d937` (2021-05-15),
  together with the switch from `activeNames` to the menu list. The `isCurrent` local stayed, unused,
  until `4674a4e2f` (2026-07-31). The commit does not say the removal was intended.
- Decision: restore the marking (bold the items in `currentNames`), or accept the current behavior.
  If restored: a source-contract check, or a helper that computes the item state.

## Adjacent observations (outside the eight items)

- `GenericTable::operator[]` (both overloads, `generictable.cpp:92-119`) reads index -1 for a missing
  id. This is the root cause of F1 and F4. Fix it at the source.
- `synchronizer.cpp:826-828`: an empty `if` with a comment about `startActivity` removed in KF6 (the
  comment is at line 827). Dead code.
- `screenpool.cpp:78`: `m_connectorForId[scrId]` default-inserts an empty connector when a config
  entry is empty or starts with ':' and the previous snapshot had the id. Minor; not visible.

## Corrections for the R2c report (`docs/modernization-results/R2c-unused-non-trivial-variable.md`)

- Line 57, "Git history starts at the repository reset, so none of this can be dated": true for
  `main` only (its single root is `8350a6b1c`). The upstream lineage (root `f9ec1fa5b`, 2016) is
  reachable from tag `v1.0.7` and dates every item: `c7d163fdb` 2020-08-26, `7393ac02e`
  2020-08-27, `22fa4c8fc` 2021-02-14, `a02056b4c` 2020-08-31, `8448f36ed` / `a83d26a35` 2020-08-30,
  `3689cc406` / `caf9bf7b7` 2020-08-25, `deb9c244f` 2020-04-24, `2be67b11c` 2021-04-29,
  `69ec3d937` 2021-05-15. The 2026 commits are on `main`.
- Lines 68-70 (`setId`) and 59-64 (`save`): the report says `Layouts::save()` diffs the tables
  instead. The diff is correct, but the snapshot at line 924 predates the id rewrite, which is the
  root of F2 and F4. The removed `oldId` notification is what those paths lack.

## Commands used (read-only)

- `git log --oneline -- docs/modernization-results/R2c-unused-non-trivial-variable.md`, `git show cec126a31`
- `git log --all -S/-G ...` for knownActivities, switchToLayout, allactivities, oldId, idForConnector, cid, activeNames/isCurrent
- `git show <commit> -- <file>` for the 2020-2021 commits listed above; `git for-each-ref --contains` for their refs
- `git log 61963df10..HEAD -- <reviewed paths>` (empty) and `git status --short` (clean), for the re-check
- Reads of the current files at the line numbers cited above

## Verification status

Re-read on 2026-11-03 at `61963df10` (tree clean; no commit after the baseline touches the reviewed paths):
- `layoutscontroller.cpp` 914-1060: snapshot 924, alteredLayouts 934, unlock 947-950, setIcon 953,
  setActivities 964, rename 967-986, id rewrite 992-1007, lock 1017-1031, setLayoutsTable 1034,
  switch 1040-1052, applyData 1054; delegate installed at 151.
- `layoutsmodel.cpp`: applyData 195-205, setId 629-640 (call at 665), alteredLayouts 865-879,
  onActivityRemoved 962-974, ALLACTIVITIESSORTEDROLE 462-468, initActivities 882-899, setActivities 614-626.
- `activitiesdelegate.cpp`: menu loop 52-73, setModelData 257-283, paint 301-317.
- `generictable.cpp` 92-119; `qlist.h` 511-521 (system Qt 6 header).
- `synchronizer.cpp`: 447-462, 728-745, 796-805, 749 (comment), 827 (empty if).
- `lattecorona.cpp` 1538-1556 (menu filter at 1545-1546); `positioner.cpp` 1176-1186 and 551/562;
  `org.kde.LatteDock.xml` 33-35.
- `storage.cpp` 226-242 (lock/unlock); `centrallayout.cpp` 135-155; `genericlayout.cpp` 825-840.
- Item anchors: `screenpool.cpp:60,78`; `containmentinterface.cpp:2279,2309`; `menu.cpp:355-388,411-441`;
  `storage.cpp:1041,1069,1081,1128,1143,1167`.
- From the first pass at the same commit, not re-read in this pass: `manager.cpp:127,461-470`,
  `storage.cpp:85-94` (isWritable), `abstractlayout.cpp:25-42` (an unloaded layout has no file),
  `autotests/CMakeLists.txt:23-44`, `storagelogictest.cpp:108`, `synchronizer.cpp:516-555` (initLayouts),
  `layoutscontroller.cpp:151`.

## Not verified

- At the time of the review: no build, no run, no Qt Release-mode check, so the runtime
  outcomes of F1-F4 were inferred from the code. The fixes were built and tested afterwards
  (see [R2c, Follow-up fixes](R2c.md#follow-up-fixes)); the defects themselves were not
  reproduced before the fixes.
- The QML side of the Layouts page was not read.
- KActivities signal timing (activity removed while Latte is stopped) was reasoned about, not tested.
- Single-layout rename: not traced whether unloading the renamed layout writes back to its old path.
  The removal at `synchronizer.cpp:659-668` (deprecated renaming) looks designed for that case.
