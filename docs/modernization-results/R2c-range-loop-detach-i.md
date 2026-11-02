# R2c-range-loop-detach-i: Range Loops over Layout Containers Do Not Detach

Status: Complete (CI run 41452548625 and the host desktop smoke passed). Part of
[R2c](R2c.md) fix batch 3, group I: the
`range-loop-detach` slice for one file, `app/layouts/storage.cpp`, which had 63 of
the 209 findings the tree had at the start of this branch. One commit on
`claude/r2c-batch-i-eb861d`, on top of `e62767af2` (the merge of batch H). Not
pushed. The gate does not include `range-loop-detach` yet; the main session adds
it after all groups are merged. Date: 2026-11-02.

## Defect

clazy 1.17 reports "c++11 range-loop might detach Qt container" for a range-based
`for` over a non-const Qt container. The loop calls the non-const `begin()`, which
copies the container when its data is shared with another owner. The finding is
about that possibility: no copy was observed and no functional defect is known.
The 63 loops in this file are of two kinds:

- **8 loops over an lvalue** (a local) that the body only reads.
- **55 loops over a call that returns a container by value:**
  `KConfigGroup::groupList()` (42), `Plasma::Containment::applets()` (8),
  `Plasma::Corona::containments()` (3), `AppletIds::appletIdListEntries()` (1)
  and `corona->containmentsIds() + corona->appletsIds()` (1). `std::as_const()`
  cannot bind such a temporary. `groupList() const` returns `QStringList` (KF6);
  `applets() const` and `containments() const` return `QList` by value (Plasma
  6.7.5 headers on the host, 6.8.1 in the container); `appletIdListEntries()` and
  `containmentsIds()` return `QStringList` (`app/layouts/appletids.h`,
  `app/lattecorona.h`).

## Fix

Two forms, chosen per loop from its range expression. No loop body, loop variable
declaration or other line changed.

- **Wrapped (8 loops).** `for (DECL : NAME)` became
  `for (DECL : std::as_const(NAME))`. Each body was read with its callees, and none
  modifies the container it walks (table below). Each loop variable is a const
  reference or a copy, so the loop compiles with const elements.
- **Hoisted (55 loops).** `for (DECL : CALL)` became
  `for (const auto NAME = CALL; DECL : NAME)`, a C++20 range-for init-statement:

  ```cpp
  for (const auto &cId : containments.groupList()) {
  for (const auto containmentIds = containments.groupList(); const auto &cId : containmentIds) {
  ```

  The local is const, so the loop uses `begin() const`. It lives as long as the
  loop, as the temporary did, and the call runs once, at the same point, before the
  first iteration. A result that shares data with an object shares it implicitly:
  a later change to that object detaches the object, not the local, so the loop
  walks the values it started with, as it did over the temporary. A freshly built
  result is not copied at all (C++17 initializes the local from the prvalue).

The local is named after what the call returns: `containmentIds` (group names of
a containments group), `appletIds` (group names of an applets group), `configIds`
(the config groups of one applet), `applets` and `containments` (the objects a
containment or the corona returns), `coronaIds` (the concatenated ids) and
`idListEntries` (the entry names of `appletIdListEntries()`). Nested loops with an
inner call got their own init-statement; `appletIds` inside `containmentIds` is an
example.

Checks on the diff:

- **Nothing else changed.** `loop-diff-check.py` prints `63 loops changed as
  expected; 0 lines to review`, and `git clang-format --diff HEAD` reports no hunk.
  No include was added: `std::as_const` was already used in this file.
- **Names.** No hoisted name is declared in an enclosing scope or used by its own
  loop body (`name-check.py`, over each enclosing top-level function). Sibling loops
  in one function reuse a name when they hoist the same kind of call, as batch H's
  `knscompat.cpp` does. No nested pair shares a name. The one word match is
  `applets` in two comments of `hasAppletsAndContainmentsWithSameId` (lines 1152 and
  1211), not in code.
- **Not findings, left alone.** 20 range-for loops are not flagged: 157 (a const
  member), 299 (a const parameter), 559, 563, 675, 709 and 740 (already
  `std::as_const`), and 593, 1055, 1068, 1142, 1153, 1166, 1267, 1338, 1471, 1486,
  1615, 1689 and 2005, which range over `*layout->containments()`, a
  `const QList<Plasma::Containment *> *` (`app/layout/genericlayout.h:79`).

### Wrapped loops (8)

| File:line | Container | Why the body does not modify it |
| --- | --- | --- |
| `app/layouts/storage.cpp:423` | local `toInvestigateContainmentIds` | Reads `allIds` and `assignedIds`, calls `availableId()` (const references), writes `assignedIds` and `assigned`. The list is not passed on. |
| `app/layouts/storage.cpp:436` | local `toInvestigateAppletIds` | As line 423, for applet ids. |
| `app/layouts/storage.cpp:452` | local `toInvestigateContainmentIds` | Reads and writes `assigned` (`operator[]` may insert into it, not into the list) and logs. |
| `app/layouts/storage.cpp:466` | local `toInvestigateAppletIds` | As line 452, for applet ids. |
| `app/layouts/storage.cpp:511` | local `toInvestigateSubContIds` | Writes `subParentContainmentIds`, `subAppletIds` and `assigned` (through `operator[]`) and config groups; calls `subIdentityIndex()` (const, takes a `KConfigGroup`). The list is not passed on. |
| `app/layouts/storage.cpp:762` | local `newContainments` (the result of `importContainmentsToCorona()` or `importLayout()`) | Appends to `importedViews`; calls `isLatteContainment()` (const). |
| `app/layouts/storage.cpp:1675` | local `clones` | Calls `removeView(filepath, clonedata)`, which edits config groups of `filepath`; it cannot reach the local. |
| `app/layouts/storage.cpp:1928` | local `subconts` (the result of `layout->subContainmentsOf()`) | Calls `syncContainmentConfig()` (config only) and writes `destinationContainments`; it cannot reach the local. |

### Hoisted loops (55)

| File:line | Call | Local |
| --- | --- | --- |
| `app/layouts/storage.cpp:133` | `corona->containments()` | `containments` |
| `app/layouts/storage.cpp:214` | `corona->containments()` | `containments` |
| `app/layouts/storage.cpp:329` | `containments.groupList()` | `containmentIds` |
| `app/layouts/storage.cpp:343` | `containments.groupList()` | `containmentIds` |
| `app/layouts/storage.cpp:402` | `investigate_conts.groupList()` | `containmentIds` |
| `app/layouts/storage.cpp:408` | `appletsEntries.groupList()` | `appletIds` |
| `app/layouts/storage.cpp:483` | `investigate_conts.groupList()` | `containmentIds` |
| `app/layouts/storage.cpp:485` | `AppletIds::appletIdListEntries()` | `idListEntries` |
| `app/layouts/storage.cpp:535` | `investigate_conts.groupList()` | `containmentIds` |
| `app/layouts/storage.cpp:544` | `investigate_conts.group(contId).group(QStringLiteral("Applets")).groupList()` | `appletIds` |
| `app/layouts/storage.cpp:569` | `fixedNewContainmets.groupList()` | `containmentIds` |
| `app/layouts/storage.cpp:632` | `multiContainments.groupList()` | `containmentIds` |
| `app/layouts/storage.cpp:703` | `corona->containmentsIds() + corona->appletsIds()` | `coronaIds` |
| `app/layouts/storage.cpp:716` | `applets.groupList()` | `appletIds` |
| `app/layouts/storage.cpp:783` | `originContainments.groupList()` | `containmentIds` |
| `app/layouts/storage.cpp:823` | `containments.groupList()` | `containmentIds` |
| `app/layouts/storage.cpp:891` | `containments.groupList()` | `containmentIds` |
| `app/layouts/storage.cpp:902` | `applets.groupList()` | `appletIds` |
| `app/layouts/storage.cpp:908` | `applets.group(aId).groupList()` | `configIds` |
| `app/layouts/storage.cpp:920` | `containments.groupList()` | `containmentIds` |
| `app/layouts/storage.cpp:961` | `applets.groupList()` | `appletIds` |
| `app/layouts/storage.cpp:976` | `layout->corona()->containments()` | `containments` |
| `app/layouts/storage.cpp:995` | `copied_conts.groupList()` | `containmentIds` |
| `app/layouts/storage.cpp:1006` | `applets.groupList()` | `appletIds` |
| `app/layouts/storage.cpp:1012` | `applets.group(aId).groupList()` | `configIds` |
| `app/layouts/storage.cpp:1024` | `copied_conts.groupList()` | `containmentIds` |
| `app/layouts/storage.cpp:1056` | `containment->applets()` | `applets` |
| `app/layouts/storage.cpp:1071` | `containment->applets()` | `applets` |
| `app/layouts/storage.cpp:1096` | `containmentsEntries.groupList()` | `containmentIds` |
| `app/layouts/storage.cpp:1097` | `containmentsEntries.group(cid).group(QStringLiteral("Applets")).groupList()` | `appletIds` |
| `app/layouts/storage.cpp:1107` | `containmentsEntries.groupList()` | `containmentIds` |
| `app/layouts/storage.cpp:1108` | `containmentsEntries.group(cid).group(QStringLiteral("Applets")).groupList()` | `appletIds` |
| `app/layouts/storage.cpp:1154` | `containment->applets()` | `applets` |
| `app/layouts/storage.cpp:1178` | `containment->applets()` | `applets` |
| `app/layouts/storage.cpp:1203` | `containmentsEntries.groupList()` | `containmentIds` |
| `app/layouts/storage.cpp:1212` | `containmentsEntries.groupList()` | `containmentIds` |
| `app/layouts/storage.cpp:1213` | `containmentsEntries.group(cid).group(QStringLiteral("Applets")).groupList()` | `appletIds` |
| `app/layouts/storage.cpp:1223` | `containmentsEntries.groupList()` | `containmentIds` |
| `app/layouts/storage.cpp:1233` | `containmentsEntries.group(cid).group(QStringLiteral("Applets")).groupList()` | `appletIds` |
| `app/layouts/storage.cpp:1270` | `containment->applets()` | `applets` |
| `app/layouts/storage.cpp:1295` | `containmentsEntries.groupList()` | `containmentIds` |
| `app/layouts/storage.cpp:1296` | `containmentsEntries.group(cid).group(QStringLiteral("Applets")).groupList()` | `appletIds` |
| `app/layouts/storage.cpp:1360` | `containmentsEntries.groupList()` | `containmentIds` |
| `app/layouts/storage.cpp:1477` | `containment->applets()` | `applets` |
| `app/layouts/storage.cpp:1492` | `containment->applets()` | `applets` |
| `app/layouts/storage.cpp:1531` | `containmentGroups.groupList()` | `containmentIds` |
| `app/layouts/storage.cpp:1539` | `appletGroups.groupList()` | `appletIds` |
| `app/layouts/storage.cpp:1550` | `containmentGroups.groupList()` | `containmentIds` |
| `app/layouts/storage.cpp:1558` | `appletGroups.groupList()` | `appletIds` |
| `app/layouts/storage.cpp:1587` | `containment->applets()` | `applets` |
| `app/layouts/storage.cpp:1625` | `containmentsEntries.groupList()` | `containmentIds` |
| `app/layouts/storage.cpp:1661` | `containmentGroups.groupList()` | `containmentIds` |
| `app/layouts/storage.cpp:1717` | `applets.groupList()` | `appletIds` |
| `app/layouts/storage.cpp:1874` | `linkedContainments.groupList()` | `containmentIds` |
| `app/layouts/storage.cpp:2029` | `containmentGroups.groupList()` | `containmentIds` |

## Needs a decision

None. Each of the 63 loops is either wrapped, and its body was checked (table
above), or hoisted, and its range is a by-value call, so the body cannot change
what the loop walks. No `clazy:exclude` was added.

## Evidence

Container: image `latte-r2c-clazy:noble` (ID `8e9528e34f50`): Ubuntu clang 18.1.3
(LLVM 19.1.1), clazy 1.17, Qt 6.11.1, libplasma 6.8.1 (`PLASMA_VERSION` `0x60801`).
Host: GCC 16.2.1 (Gentoo), Clang 22.1.8, CMake 4.3.4, Qt 6.11.2, KF6 6.29.0,
libplasma 6.7.5 (`PLASMA_VERSION` `0x60705`).

`app/plasma/plasmaapicompat.h` defines `LATTE_PLASMA_STORED_CONTAINMENT_SCREEN` for
libplasma 6.8 and later. In `storage.cpp` the block is lines 650 to 749, and it
holds lines 703 and 716. The container compiles the block and the host does not.

| Command (container unless marked host) | Result |
| --- | --- |
| Before the fix, on `e62767af2`: `cmake --preset clang-debug -DVERSION=…`, `cmake --build --preset clang-debug -j4`, `python3 scripts/test-clazy.py --build-dir build/modernization/clang-debug --jobs 4 --inventory range-loop-detach --report build/rld.json` | Exit 0. `build.log`: 0 `warning:` lines. **209 findings in 25 files; 63 in `app/layouts/storage.cpp`** |
| `python3 -I "$HOME/r2c-build-batch-i/evidence/loop-diff-check.py" "$START" app/layouts/storage.cpp`, `START` = `e62767af2` | `63 loops changed as expected; 0 lines to review` |
| `git clang-format --style=file --diff HEAD -- app/layouts/storage.cpp` | `clang-format did not modify any files` |
| `name-check.py` over the 55 hoisted names, each against its enclosing top-level function | 55 headers checked; no nested pair with the same name; one word match (`applets`, in two comments) |
| After the fix, the same build with `--report build/rld-after.json` | Exit 0. One object rebuilt (`storage.cpp.o`); `build-post-fix.log`: 0 `warning:` lines. **146 findings in 24 files; 0 in `app/layouts/storage.cpp`.** The other 146 are identical to the baseline by file, line and message |
| Full gate: the same build, then `test-clazy.py --build-dir build/modernization/clang-debug --jobs 4` | **Exit 0**: 48 negative fixtures, each rejected by its own gated check; 153 production sources with no finding from 48 gated checks; `build-gate.log`: 0 `warning:` lines |
| Host GCC Debug on a fresh tree: `cmake --preset gcc-debug`, `cmake --build --preset gcc-debug -j4`, `ctest --preset gcc-debug --output-on-failure` | Configure, build and CTest exit 0. 425 objects built; 0 `warning:` lines; 0 CMake warnings. The build log has 4 `error` matches, all in QML `console.warn` text. **CTest: 100% passed, 0 failed out of 58** |
| Host Clang Debug on a fresh tree: the same with `clang-debug` | Configure, build and CTest exit 0. 425 objects built; 0 `warning:` lines; 0 CMake warnings; the same 4 QML matches. **CTest: 100% passed, 0 failed out of 58** |

Logs. The container build logs and inventories are in `$HOME/r2c-build-batch-i/`
(`build.log` and `rld.json` before the fix, `build-post-fix.log`, `build-gate.log`,
`rld-after.json`). The run logs, the host logs, the diff and format checks and the
scripts are in `$HOME/r2c-build-batch-i/evidence/`, outside the repository.

## Not run or not verified

- **Lines 703 and 716 are compiled only in the container.** The host's libplasma
  6.7.5 leaves the Plasma 6.8 block out, so the GCC and Clang host builds do not
  compile them. The container's Clang 18 build (Plasma 6.8.1) compiled them with
  0 warnings and clazy analysed them. Their form is the same as the other hoisted
  loops.
- **No test runs these loops.** No autotest compiles `app/layouts/storage.cpp` or
  references `Layouts::Storage`. `autotests/storagelogictest.cpp` says it
  "replicates core logic" and does not call the production file.
  `privateapphostsmoketest` runs the executable in a QML-only mode that returns
  before the corona starts (`app/main.cpp:130`). The 58 passing CTest tests show
  that the code compiles and the existing tests pass; they do not exercise these
  loops. The claims about the bodies rest on reading them and their callees.
- **Desktop smoke and CI: not run on the group branch;** both ran after the merge (see the
  Handoff). Where to look, by path and line: startup and layout load
  (`containmentsIds()` 329, `appletsIds()` 343, `newUniqueIdsFile()` 402 to 569,
  the errors and warnings checks 1041 to 1374, `plugins()` 1455 to 1577, `views()`
  2029, `removeAllClonedViews()` 1661); layout switch and import (`importLayoutFile()`
  762, `importContainments()` 783, `moveToLayoutFile()` 632, and on Plasma 6.8
  `importContainmentsToCorona()` 703 and 716); export, and copy and paste of docks
  (`exportTemplate()` 891 to 1024, `storedView()` 1928, `newView()` 823); adding and
  removing a dock (`newView()` 823, `removeView()` and `removeAllClonedViews()` 1661
  to 1675).
- Release presets were not built. Debian 13 and Nix were not checked.
- Performance was not measured. The change adds no copy and no work; a wrap or a
  hoist only removes a potential detach.

## Handoff

- **Batch / slice and selected scope:** R2c fix batch 3, group I:
  `range-loop-detach`, `app/layouts/storage.cpp` only, 63 findings. 8 wrapped in
  `std::as_const()`, 55 hoisted into a const local, 0 left.
- **Status:** Complete. The container inventory and gate and both host builds with CTest
  pass; CI run 41452548625 and the host desktop smoke passed ([R2c](R2c.md#fix-batch-3)).
- **Source baseline / start HEAD / implementation HEAD:** `e62767af2`
  (`claude/r2c-batch-i-eb861d`, the merge of batch H) / `e62767af2` / the one commit
  that adds the change and this report.
- **Branch / existing user edits preserved:** `claude/r2c-batch-i-eb861d` in its own
  worktree, clean at the start; no other edits.
- **Dependencies and evidence links:** [R2c](R2c.md),
  [R2c-range-loop-detach-h](R2c-range-loop-detach-h.md) (the same kind of work on
  other files) and [R2c-container-anti-pattern](R2c-container-anti-pattern.md). The
  other groups of this check work on other files; the tree has 146 findings left
  in them.
- **Files and externally observable behavior changed:**
  `app/layouts/storage.cpp` (63 `for` headers, one line each; no other line) and
  this report. Not changed: `scripts/test-clazy.py`, `autotests/clazy/`, `R2c.md`,
  the ledger and other shared docs. No observable change is intended: the same
  elements, in the same order, from the same snapshot as before.
- **State authority / invalidation / lifetime / fallback constraints:** a wrapped
  loop reads the live container, as before, and its body does not write it (table).
  A hoisted loop walks the result of its call as it was when the loop started, so a
  body that needs to see its own changes must read the source again. A later edit
  that inserts into a wrapped container inside its loop must copy the container
  first, as the constraint in [R2c-range-loop-detach-h](R2c-range-loop-detach-h.md)
  says. No code comment was added: the brief asked for no other change and no
  invariant changed. The main session may add a note, as it may for batch H.
- **Toolchain, import environment and artifact identity:** see Evidence.
- **Commands executed with exit status and durable log/artifact locations:** see
  Evidence.
- **GCC/Clang Debug/Release warning/error results:** host GCC 16.2.1 and Clang
  22.1.8, Debug: 0 `warning:` lines and 0 CMake warnings each. Container Clang 18
  Debug: 0 `warning:` lines in the baseline, post-fix and gate builds. Release
  presets were not built.
- **Tests registered / run / passed / skipped / failed and name deltas:** 58
  registered; 58 run and 58 passed on GCC and on Clang Debug; none skipped. No test
  added, changed or removed; no gate name added.
- **Production behavior vs copied logic vs source-contract evidence:** production
  code is compiled and analysed by clazy. The wrapped loops rest on reading the
  bodies and callees; the hoisted loops rest on the argument in Fix. No test covers
  the file (see above).
- **QML diagnostics added/removed and environment compatibility:** none; no QML
  changed.
- **Negative fixtures or controlled mutation evidence:** none for this check yet.
  `autotests/clazy/range-loop-detach.cpp` is added by the main session with the gate
  entry.
- **Desktop user feedback / log review / clean-quit and coredump evidence:** the host smoke on
  `6b51b43a6` passed with no warning from Latte's code and no core ([R2c fix batch
  3](R2c.md#fix-batch-3)). It reached startup and the layout load with two docks, two layout
  switches, the Docks dialog and the layout exports of check 11. It did not reach a layout
  import (`importLayoutFile()`), a new dock from a template or the removal of cloned views
  (`removeAllClonedViews()`).
- **Performance paired samples / variability / work counts / tradeoffs:** not
  measured. No copy is added: the local is initialized from the call result.
- **CI, distribution and Nix checks (passed / failed / unavailable):** run 41452548625 at
  `6b51b43a6` passed all 23 jobs after one rerun of `Install verify (mageia)`, which had failed
  on the Mageia Cauldron mirror. `Build (Clang Debug)` ran the 49-check gate and 57 tests, and
  the Debian 13 (Qt 6.8.2) install job and the Nix check passed. See
  [CI validation](R2c.md#ci-validation).
- **Unresolved failures, review assumptions and unavailable checks:** no failure.
  Assumptions: the body reading for the wrapped loops; the snapshot argument for the
  hoisted loops. Lines 703 and 716 are checked only by the container build. No test
  reaches the file. Not run on the group branch: CI, the desktop smoke and the Release
  presets; all three ran after the merge ([R2c fix batch 3](R2c.md#fix-batch-3)).
- **Next exact action:** none. `range-loop-detach` joined `GATED_CHECKS` with
  `autotests/clazy/range-loop-detach.cpp` in `8a30d3fa9`, once the tree inventory was zero.
- **Commit approval status / push approval status:** one commit on
  `claude/r2c-batch-i-eb861d`; merged into the integration branch in `98b00c657` by the main
  session. `main` was fast-forwarded to `6b51b43a6` and pushed on 2026-11-02 with the user's
  approval.
