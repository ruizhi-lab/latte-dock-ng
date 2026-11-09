# R8h: ContainmentInterface uses Latte::PluginId for its separator ids

Status: Pending validation: the user's desktop check of the boundary separators, which
has to wait until the R8d 12-hour series ends (about 23:59 on 2026-11-09). The contract
mutations, the GCC Debug build and tests and the clazy gate pass.

Batch / slice and selected scope: R8h, the first follow-up of the
[R8f record](R8f.md). `app/view/containmentinterface.cpp` drops its own copies of the
splitter and separator plugin ids and of `isSeparatorPluginId()`, and uses
`Latte::PluginId` (`app/pluginids.h`). Row 105 of the [R8a inventory](R8a-inventory.md)
(`containmentInterfaceHasSeparatorPluginConstantsAndHelper`, class K) becomes a no-copy
contract. Behaviour unchanged.

Source baseline and start HEAD: `83a03c2c5` (local `main`). Branch
`claude/r8h-containment-plugin-ids`. It reuses the worktree
`.claude/worktrees/r8e-stale-copies`: the R8e follow-up branch there is merged, and its
GCC Debug tree saved a full build. Commit `e070d4d66` and the commit that adds this
record.

## The copies

`21ee748c0` (2026-05-14, "fix(separators): stabilize task widget boundaries") added an
anonymous-namespace block to `containmentinterface.cpp`:
`kInternalViewSplitterPluginId` (`org.kde.latte.splitter`), `kLatteSeparatorPluginId`
(`org.kde.latte.separator`), `kLegacySeparatorPluginId` (`audoban.applet.separator`)
and an inline `isSeparatorPluginId()`. A month later `50f8f7ad6` ("centralize plugin
name strings in shared header") added `app/pluginids.h` with the same three ids
(`kSplitter`, `kSeparator`, `kLegacySeparator`) and the same helper, and moved the
file's other ids to it, but left this block. `dataunittest` pins the shared values:
`isSeparatorPluginId()` must accept both separator ids and reject the spacer, and
`isLatteBuiltIn()` must accept the splitter id.

## Change

| Old | New | Uses |
|---|---|---:|
| `QLatin1String(kInternalViewSplitterPluginId)` | `QLatin1String(Latte::PluginId::kSplitter)` | 6 |
| `QLatin1String(kLatteSeparatorPluginId)` | `QLatin1String(Latte::PluginId::kSeparator)` | 1 |
| local `isSeparatorPluginId(...)` | `Latte::PluginId::isSeparatorPluginId(...)` | 10 |
| `pluginId != kLatteSeparatorPluginId && pluginId != kLegacySeparatorPluginId && pluginId != kInternalViewSplitterPluginId` (`onAppletAdded()`) | `!Latte::PluginId::isSeparatorPluginId(pluginId) && pluginId != kSplitter` | 1 |

The values and the helper are identical, and the last row is the same condition (the
helper is exactly the first two comparisons). The uses are in `sanitizeSeparatorOrder()`
(drops leading, doubled and trailing separators), `addInternalSeparatorBeforeApplet()`
and the four `Q_INVOKABLE` boundary functions behind the task menu's separator entries,
`cleanupInvalidSeparatorApplets()`, and the default placement of new applets in
`onAppletAdded()`. `git clang-format` reformatted the touched lines (two brace
placements, one braced list, one joined condition).

The contract is renamed `containmentInterfaceUsesSharedSeparatorPluginIds`: the file
must not contain the three id literals and must call
`Latte::PluginId::isSeparatorPluginId(` and use `Latte::PluginId::kSplitter`. Its
comment names the code paths and the failure mode a copy would bring back: a separator
id added to the shared list would be treated as an ordinary applet here.

## Contract mutations

Text-only scratch mutations of `containmentinterface.cpp` (the contract reads the source,
so nothing was rebuilt while one was applied), each run with
`sourcecontracttest containmentInterfaceUsesSharedSeparatorPluginIds` and restored from
`HEAD` before the next. Run by a Haiku sub-agent; the main session checked the logs.

| ID | Mutation | Result | Failing check |
|---|---|---|---|
| H0 | none | PASS | — |
| H1 | the splitter test in `sanitizeSeparatorOrder()` uses the literal again | FAIL | `!src.contains("\"org.kde.latte.splitter\"")` (line 3397) |
| H2 | a local `kLegacySeparatorPluginId` copy is back in the anonymous namespace | FAIL | `!src.contains("\"audoban.applet.separator\"")` (line 3396) |
| H3 | the 11 calls use an unqualified `isSeparatorPluginId(` | FAIL | `src.contains("Latte::PluginId::isSeparatorPluginId(")` (line 3398) |
| H4 | the 7 splitter uses name a local `kSplitterCopy` | FAIL | `src.contains("Latte::PluginId::kSplitter")` (line 3399) |

After the restore the contract passes again and the worktree is clean.

## Evidence

- GCC Debug (`gcc-debug` preset, GCC 16.2.1, incremental): exit 0, four units compiled
  (`containmentinterface.cpp`, `sourcecontracttest.cpp`, and two test units that changed
  on `main` since the tree was last built), 0 compiler warnings; CTest 61 of 61 passed.
- clazy gate (`scripts/test-clazy.py`) in the local `latte-r2c-clazy:noble` container, on a
  `git archive` export of `e070d4d66` with its own build tree, run by a Haiku sub-agent:
  configure, the full `clang-debug` build (Clang 18.1.3, 433 compile steps, 0 compiler
  warnings) and the gate all exit 0. clazy 1.17 (LLVM 19.1.1): "49 negative fixtures:
  each is rejected by its own gated check" and "155 production sources: no findings from
  49 gated clazy checks". `containmentinterface.cpp` has one production compile command,
  the executable's, so it is in the analysed set. The export has no `build/` directory,
  so the sub-agent created an empty one for the read-only mount to take the build tree.

## Desktop check (after the R8d series ends)

Not run yet. Restarting the dock ends the R8d 12-hour desktop series, which runs until
about 23:59 on 2026-11-09, and installing a user-mode build while the packaged dock runs
would let it load the new QML packages from `~/.local` later in the series. After the
series: `bash install.sh --user Debug` from this worktree, start the dock as AGENTS.md
describes, and check on a horizontal dock whose task manager has pinned applications:

1. With a widget directly left of the task icons: right-click the left-most pinned task
   icon → 添加 左侧分隔线. A separator appears between the widget and the first task icon
   (`addInternalSeparatorAtLeftBoundaryOfApplet()`).
2. Right-click the same task icon → 移除 左侧分隔线. The separator disappears
   (`removeInternalSeparatorAtLeftBoundaryOfApplet()`).
3. With a widget directly right of the task icons: the same on the right-most pinned task
   icon with 添加 右侧分隔线 and 移除 右侧分隔线.
4. The separators the layout already has keep their places after the restart.

Then the log (`/tmp/latte-ng.log`) is reviewed for warnings and errors and the coredump
count is compared with the one before the launch. Other tasks' 添加/移除 entries use the
task manager's own separators (`launchers.addInternalSeparatorAtPos()`), which this slice
does not touch.

## Not verified

- The desktop check above: pending, after the R8d series.
- Clang Debug on this host and the Release presets: run on the merged `main` after the
  desktop check.
- The Debian 13 container build: deferred by the user until the refactor is complete.
