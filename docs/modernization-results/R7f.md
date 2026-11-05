# R7f: Split main() into named start-up steps

Status: Pending validation. The split is complete on branch
`claude/r7f-main-split-9cf795` and was built and tested here with GCC and Clang
Debug, the source contracts, the logout simulation and five mutation checks.
Not run: the real-desktop checks under "Not verified", and the Debian 13
verification, which the user deferred until the refactor is complete.

Date: 2026-11-06. Start HEAD: `e18a0f9a7` (main, release v1.3.0). Implementation
HEAD: `5a34f2d10`. This record is the commit after it.

## Scope and result

Only `app/main.cpp` changed, plus the source contracts that read it in
`autotests/sourcecontracttest.cpp`. No file was added, no CMake or QML file
changed, and the ledger and `docs/qmllint-baseline.json` were not touched.

`int main()` was lines 91-721 (631 lines). It is now 187 lines, from the
`int main` line through its closing brace, and reads as the start-up sequence.
Each step is a helper defined after `main()` with a one-line `//!` comment that
says what it owns:

| Helper | Owns | Returns |
| --- | --- | --- |
| `configureProcessEnvironment` | HighDPI policy, environment sanitising, user-local paths, the QPA platform and the KDE session variables, before `QApplication` exists | whether the user had set `QT_QPA_PLATFORM` |
| `printVersion` | `--version` and `-v` output | exit code |
| `runPrivateAppHostSmoke` | `LATTE_PRIVATE_APP_HOST_SMOKE=1` load of private.app | exit code |
| `prepareApplication` | translation domain, QML cache policy, KNS compat, window icon, quit-on-last-window policy | void |
| `addCommandLineOptions` | help, version and the public options | void |
| `addHiddenDeveloperOptions` | the hidden developer options | void |
| `runOneShotCommand` | autostart toggles and the `--available-*` listings | true when one ran |
| `sessionEndingMarker` | the session-ending flag (app property and `LATTE_SESSION_ENDING`) | `std::function<void()>` |
| `watchShutdownSignals` | KSignalHandler for SIGTERM and SIGHUP; quit lock disabled | void |
| `connectSignalShutdown` | KSignalHandler's signal to the marker and `quit()` | void |
| `connectSessionManagement` | commitData and saveState requests | void |
| `chooseStartupLayout` | `--default-layout` and `--layout`, before the lock | false when the named layout is missing |
| `singleInstanceLockPath` | the per-user lock file name | path |
| `stopReplacedInstance` | `--replace` and `--import-full` stop request and lock timeout | timeout in ms |
| `forwardToRunningInstance` | D-Bus forwarding and messages of a second start | void |
| `applyPostLockOptions` | clear-cache, import-full, import-layout, memory mode, add-dock | false when an import failed |
| `installDebugMessageHandler` | `--debug`, `--mask`, `--debug-text`, `--log-file` | void |
| `sessionShutdownHooks` | the hooks of `Session::ShutdownMonitor` | the hooks |

`struct StartupLayoutOptions` replaces four locals. The Corona constructor
receives its four values from it.

Still in `main()`, as rule 1 requires: `QApplication app`, the self-pipe
`static int sigPipe[2]` with its handler, `sharedEngine` and the shared-engine
branch, the `Latte::Corona corona` / `ShutdownMonitor` / `KDBusService` block
with its declaration order, the post-`exec()` detach, DeferredDelete flush and
engine reset, the single-instance `QLockFile`, the Wayland check and the
command-line parser.

## Commits

| Commit | Change |
| --- | --- |
| `198bd5d43` | `--version` output and private.app smoke load |
| `de8ff36f1` | process environment before `QApplication` |
| `59f128f9f` | command-line option tables |
| `6e1acbc1e` | `runOneShotCommand` |
| `099cd69d4` | `prepareApplication` |
| `6c22237d6` | quit-signal and session-management setup |
| `8dc99793b` | startup layout, lock and post-lock options (two planned steps) |
| `403b9fecf` | `sessionShutdownHooks` and the declaration order |
| `c4a840770` | `sessionEndingMarker` |
| `5a34f2d10` | `mainCppMainFunctionStaysWithinLineLimit` |

The startup-layout step and the post-lock step went into one commit. The
post-lock options read the layout locals that the first step removes, so the
first step alone did not compile. That intermediate state was built, failed
with five errors, and was never committed.

## Behaviour and state authority

- Every call keeps its order, its arguments and its log text. The moved blocks
  are verbatim; the changed statements are listed under "Changed-but-not-moved
  lines".
- Early exits keep their codes: `--version` and the smoke load return from
  `main()` as before; each helper that used to `return 0` now returns to
  `main()`, which returns 0 in the same cases; a missing layout and a failed
  import still exit 0 without touching a running instance.
- Lifetime: `app`, `lockFile`, `startupLayout`, `sharedEngine`, the self-pipe
  array and the marker live in `main()`, as before. The Corona, the monitor and
  `KDBusService` stay in one block, so the monitor is still destroyed before the
  Corona.
- Lambdas that hold references: `sessionEndingMarker` captures `&app`, and
  `sessionShutdownHooks` captures `&app` and `&corona`. The referents are
  `main()` objects that outlive every connection and the monitor, so the
  captures are valid. `connectSignalShutdown` captures a copy of the marker
  (which holds `&app`), not a reference to a helper local. The self-pipe
  handler is still a captureless lambda that reaches the static in `main()`
  scope, which is why the self-pipe stayed in `main()`.

## Source-contract changes

No assertion was deleted, and each restated one keeps the same property.

1. `qtQuickGpuPreferenceKeepsSoftwareFallbackAvailable` anchored on the forward
   declaration of `configureQtQuickGraphicsPreference()`, so it passed even
   without the call. It now anchors on the call site in `main()` and checks the
   call inside `configureProcessEnvironment`.
2. `enableAutostartExitsImmediately` anchored on the option name, which also
   appears in the option table. It now anchors on the branch, requires the
   `return true;` inside that branch, and requires `main()` to return 0 on it.
3. `knsCompatImportsAreAvailableForSystemInstall`: `ensureKnsCompat()` moved into
   `prepareApplication()`. The ordering is restated: `prepareApplication(app)` is
   called after `QApplication` exists and before the shared engine is created,
   and `ensureKnsCompat()` is in its body.
4. `sessionShutdownHandlingMatchesStableWaylandPath`: the monitor-after-session
   order is checked against the call site of `connectSessionManagement(app)`.
   The save-state block runs to the end of that function. The hook checks read
   `sessionShutdownHooks()`.
5. `duplicateInstanceExitsWithoutQGuiAppExit`: the tryLock block runs to the blank
   line after it, because the clear-cache comment that used to end it moved. The
   message and the no-exit, no-engine checks moved to the body of
   `forwardToRunningInstance()`.
6. New `mainCppMainFunctionStaysWithinLineLimit`: `main()` must stay within 200
   lines. The test counts newlines from the `int main` line to the closing brace,
   which gives 186 for this code; the physical length is 187. The limit leaves
   room for a short new call. What it rules out is a block of more than about a
   dozen lines in `main()`.

The limit is 200, not the 150 the brief suggested. The rule-1 objects, which must
stay in `main()`, take about 110 lines (the self-pipe with its comments, the
shared-engine branch, the monitor block, and the post-exec sequence), and the
sequence itself needs the rest.

## Verification

| Command or check | Result |
| --- | --- |
| `cmake --preset gcc-debug`, then `cmake --build --preset gcc-debug --parallel 6`, at `e18a0f9a7` | exit 0; 0 compiler warnings |
| `ctest --preset gcc-debug`, at `e18a0f9a7` | 61/61 passed, exit 0 |
| Incremental GCC build and full `ctest` after each commit above | exit 0, 0 compiler warnings, 61/61 passed; the one failed build is the uncommitted intermediate state |
| `ctest --preset gcc-debug` at `5a34f2d10` (final GCC state) | 61/61 passed, exit 0 |
| `cmake --preset clang-debug` (configure); `cmake --build --preset clang-debug --parallel 6`; `ctest --preset clang-debug` at `5a34f2d10` | configure exit 0; build exit 0 with 0 compiler warnings and 0 compiler errors (the four lines matching `error:` in the log are QML source text); 61/61 passed, exit 0 |
| `python3 scripts/test-logout-simulation.py --install --jobs 6` at `de8ff36f1` | exit 0; run `20261106-005530`; confirm-cancel-complete, blocker and sigterm passed |
| same command at `6e1acbc1e` | exit 0; run `20261106-010329`; all three passed |
| same command at `5a34f2d10` (final) | exit 0; run `20261106-051937`; all three passed, 17 PASS lines, 0 FAIL |
| `build/modernization/gcc-debug/bin/latte-dock-ng --version` | exit 0; prints `lattedock 1.2.59`. Run with `QT_QPA_PLATFORM=offscreen`, so no Wayland connection was made |
| `privateapphostsmoketest`, `releaseversiontest`, `packagingcontracttest`, `sourcecontracttest` | passed in every ctest run above |

Toolchain: GCC 16.2.1 (Gentoo), Clang 22.1.8, Qt 6.11.2, kernel 6.18.57-gentoo-dist.
The logout simulation used its own scratch HOME, a nested `kwin_wayland --virtual`
and a private bus, as the testing guide describes. Its work directory was
`$XDG_CACHE_HOME/latte-logout-simulation` (here `/run/user/1000/cache/latte-logout-simulation`),
and the `--install` build ran there, not in the worktree.

Cadence: the brief asked for a simulation run every two or three extractions.
Runs were made after the second and the fourth extraction and at the final
commit. The intermediate commits from the fifth to the ninth extraction were
built and passed the full CTest suite, but were not run through the simulation.

Mutation checks. Each mutation was applied to `app/main.cpp`, the
`sourcecontracttest` entry was run, and the file was restored from a backup. The
worktree was clean afterwards.

| Mutation | Check that failed |
| --- | --- |
| 20 statement lines added to `main()` | `mainCppMainFunctionStaysWithinLineLimit`: 206 lines; limit 200 |
| `qGuiApp->exit();` added to `forwardToRunningInstance()` | `duplicateInstanceExitsWithoutQGuiAppExit` |
| `connectSessionManagement(app);` moved after the monitor | `sessionShutdownHandlingMatchesStableWaylandPath`: `pollerStart > sessionManagementCall` |
| `ensureKnsCompat();` removed from `prepareApplication()` | `knsCompatImportsAreAvailableForSystemInstall`: `ensureCompatCall > prepareBody` |
| enable-autostart branch returns `false` | `enableAutostartExitsImmediately`: `returnZero < branchEnd` |

clazy is not installed here, so its rules were followed by hand. No Qt container
is iterated in new code. Every new `connect()` that takes a lambda has a context
object. No `clazy:exclude` was added. No namespace-scope static container was
added.

## Changed-but-not-moved lines

Method: `git diff --color-moved=dimmed-zebra e18a0f9a7 HEAD -- <file>`. A line
counts as not moved when git draws it in the plain red or green, not the dimmed
move colour. The counts came from a script over the coloured output; the totals
agree with `git diff --numstat`.

| File | Added | Removed | Not moved | Moved |
| --- | --- | --- | --- | --- |
| `app/main.cpp` | 413 | 264 | 429 (288 added, 141 removed) | 248 |
| `autotests/sourcecontracttest.cpp` | 80 | 18 | 96 (79 added, 17 removed) | 2 |

With `--color-moved-ws=ignore-all-space`, `app/main.cpp` has 241 lines not moved
(193 added, 48 removed). That number is not broken down further here.

Groups in `app/main.cpp`, the 429 default-mode lines:

- **A. Same text after trimming indentation, 226 lines (113 removed, 113 added).**
  The bodies of `printVersion` and `runPrivateAppHostSmoke` moved from an
  `if` at 8 spaces to a function body at 4. The body of `forwardToRunningInstance`
  moved out of the tryLock block, one level shallower. The hook members of
  `sessionShutdownHooks` moved from a braced argument at 12 spaces to a return at
  8. Git's default detection compares whitespace, so these show as changed. Of
  the 113 pairs, 83 are statements or declarations, 16 are braces or blank lines,
  9 are return statements and 5 are comments.
- **B. Added only, 175 lines.** 41 braces (the new function braces and the
  struct's closing line), 18 forward declarations, 18 helper headers, 5 struct
  lines and one include, 23 comments (18 are the one-line helper headers and 5
  explain the new code in place), 17 blank separators, 21 return statements of
  the new helpers, 7 `startup.` field statements, and 24 other statements. The
  other statements are the new call sites in `main()` and in helpers, the new
  lock-path lines, the three Corona arguments and the monitor call.
- **C. Removed only, 28 lines.** 17 statements that the helpers replace (the four
  layout locals and their assignments, the old lock-file expression, the old
  Corona and monitor argument lines, and the old lambda head and capture), 8
  `return 0;` lines (now `return false;`, `return true;` or `return;`), 2 old
  `QString` locals, and the closing `});` of the monitor initializer.

Every group is a direct effect of extracting a function: its declaration, braces,
header and comment; the return value that carries an early exit back to
`main()`; the struct that replaces four locals; and the call sites. Group A is
whitespace-only movement. In groups B and C no value passed or tested changes
source, and no log string changes; the renames and the return values are the
only statements whose text differs from the original.

Groups in `autotests/sourcecontracttest.cpp`, the 96 lines: 55 added statements
(the new line-limit test and the restated checks), 18 comments that explain the
restated checks, 4 blank lines and 2 braces; 16 removed statements replaced by
those checks, and 1 comment.

## Not verified

- Real Plasma desktop. Not run, by rule: logout and cancel against a real
  ksmserver, startup in the user's session, `--replace` and `--import-full`
  against a running dock, add-dock and import-layout through a live dock's
  D-Bus, the autostart toggles, and `--help` on the installed dock. Only the
  logout simulation (nested KWin, scratch HOME) and the `--version` run above
  were done. The logout simulation checks the teardown markers and the quit
  status; it does not check the window content.
- `--help` and the option parsing were not A/B compared with the base binary,
  because the base was not built. The option definitions are moved verbatim;
  the moved-line counts show them as moved.
- Debian 13 verification: deferred by the user until the refactor is complete.
- qmllint: not applicable, since no QML changed.
- clazy: not installed here; checked by hand as above.
- Observed, not fixed (out of scope): the comment block that begins "POSIX
  signal handler for SIGTERM/SIGINT/SIGHUP" sits above
  `filterDebugMessageOutput` and ends mid-sentence. It is stale.

## Next exact action

The main session reviews the branch with
`git diff --color-moved=dimmed-zebra e18a0f9a7 claude/r7f-main-split-9cf795 -- app/main.cpp`
and merges it when the user approves. Its contract changes overlap with the R7
branches already merged into main, so the main session resolves that overlap at
merge time; this branch was not rebased. The desktop checks above and the
Debian 13 verification are left to the user.

Commit status: committed on `claude/r7f-main-split-9cf795` only. Not pushed, not
merged, and main was not touched. Push is not approved.
