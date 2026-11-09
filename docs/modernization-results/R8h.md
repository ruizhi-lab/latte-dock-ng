# R8h: ContainmentInterface uses Latte::PluginId for its separator ids

Status: Complete. The contract mutations pass on the branch. On the merge commit
`ed5d0b3c3` the four presets, the clang-tidy and clazy gates and the user's desktop check of
the boundary separators (2026-11-10) pass.

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
record. Merged as `ed5d0b3c3` on top of `main` `e769cc2c2` and the R8d record commit
`dc423e091` (documents only), in the worktree `.claude/worktrees/r8d-desktop-12h`.

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

### The merged tree (`ed5d0b3c3`)

Run by Haiku sub-agents; the main session checked their logs.

| Check | Result |
|---|---|
| `cmake --preset P`, `cmake --build --preset P -j8`, `ctest --preset P` for `gcc-debug`, `clang-debug`, `gcc-release` and `clang-release` (GCC 16.2.1, Clang 22.1.8; fresh trees, `TMPDIR` in the worktree's `build/tmp`) | Exit 0 each; 0 compiler `warning:` lines and 0 `CMake Warning`; 61/61 tests passed in each |
| `latte-r2c-clazy:noble`, `git archive` export of `ed5d0b3c3` with a fresh build tree: `cmake --preset clang-debug -DVERSION=1.2.59`, `cmake --build --preset clang-debug -j6` | Exit 0; 0 compiler warnings |
| Same container: `scripts/test-clang-tidy.py --jobs 4` (the R8i gate, clang-tidy 18.1.3) | Exit 0: 155 production sources, both negative fixtures rejected, no findings |
| Same container: `scripts/test-clazy.py --jobs 6` (clazy 1.17, LLVM 19.1.1) | Exit 0: 155 production sources, 49 negative fixtures rejected, no findings from 49 checks |
| `bash install.sh --user --jobs 8 Debug` | Exit 0; 0 compiler warnings; the running dock's build ID `36e61da38f923cb00a52df1fcfc2506ea4450fb5` equals the build output's |

## Desktop check (2026-11-10)

It waited for the R8d 12-hour desktop series to end, because restarting the dock would have
ended the series. The user-mode Debug install of `ed5d0b3c3` was started as AGENTS.md
describes (pid 264714, 00:24:38, `--replace --debug`), and the user checked a horizontal dock
whose task manager has pinned applications:

1. With a widget directly left of the task icons: right-click the left-most pinned task
   icon → 添加 左侧分隔线. A separator appears between the widget and the first task icon
   (`addInternalSeparatorAtLeftBoundaryOfApplet()`).
2. Right-click the same task icon → 移除 左侧分隔线. The separator disappears
   (`removeInternalSeparatorAtLeftBoundaryOfApplet()`).
3. With a widget directly right of the task icons: the same on the right-most pinned task
   icon with 添加 右侧分隔线 and 移除 右侧分隔线.
4. The separators the layout already has keep their places after the restart.

Result: the user reported all four steps as passed. The log of the run (348 lines when
reviewed) has no warning, error, fatal, assert or critical line. It shows the four edits on
containment 1 (screen DP-2), whose order was `43, 2, 3` (the task manager is applet 2):

| Time | Log | Order after |
|---|---|---|
| 00:52:34 | separator 47 created (`org.kde.latte.separator`) | `43, 47, 2, 3` |
| 00:52:37 | separator 48 created | `43, 47, 2, 48, 3` |
| 00:52:48 | `removeApplet` 47 | `43, 2, 48, 3` |
| 00:53:00 | `removeApplet` 48 | `43, 2, 3` |

So each separator was placed at its boundary and removed again, and the order returned to
the one before. `coredumpctl` lists no coredump since the launch. Other tasks' 添加/移除
entries use the task manager's own separators (`launchers.addInternalSeparatorAtPos()`),
which this slice does not touch.

## Not verified

- The CI Build run on the push to `main` had not been observed when this record was
  written.
- The desktop check covered the four steps above on one dock; vertical docks and the
  legacy `audoban.applet.separator` were not checked.
- The Debian 13 container build: deferred by the user until the refactor is complete.
