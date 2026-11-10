# R7h: dock template names in the running dock and at start-up

Status: complete. The fix is `d2bc32e71` on branch `claude/cool-goldwasser-3086cd`. The zh_CN
desktop check passed on 2026-11-10, and this record is the last commit of the fast-forward
merge into `main`, which was `311c93ff5`.

## Defect

In a translated session, `latte-dock-ng --add-dock "<name>"` silently added nothing when
`<name>` was the name that `--available-dock-templates` prints. Reported on 2026-11-10 in the
user's zh_CN Plasma session with a user-mode Debug build of `311c93ff5`: `"Default Dock"`
exited 0 and added nothing, and the dock log had no line about it. `"默认停靠栏"` added a dock.

Trace, from the code and not from a run:

| Path | Name used | Result under zh_CN |
| --- | --- | --- |
| `--available-dock-templates` | base name `Default Dock` (`Importer::availableViewTemplates()`) | prints `Default Dock` |
| `--add-dock` with a running dock | D-Bus `addView(0, name)` → `Corona::addView()` → `GenericLayout::newView()` → `Templates::Manager::hasViewTemplate()` | the table holds only shown names; the shell template is shown as `默认停靠栏` (`po/zh_CN/latte-dock.po`), so `Default Dock` returns `false` with no log line |
| `--add-dock` at start-up | `applyPostLockOptions()` checks the base-name list, then `addView(0, name)` after the start-up sequence | a base name passes the check, imports the Empty layout under that name, and then fails in `newView()` the same way; a shown name fails the check with no message |
| `--add-dock` with a containment id above 0 (D-Bus) | `Corona::addView()` passes the name to `View::newView(templateFile)` | `Storage::views(name)` reads no template, so nothing is added and nothing is logged |

The same code is in `v1.3.0` (`GenericLayout::newView()` checks `hasViewTemplate()`, the
containment branch passes the name, and `applyPostLockOptions()` compares with
`viewTemplates.contains()`), so this is not a regression from the R7f split of `main()`.

A second derivation of the same file name also differed. `Manager::initViewTemplates()` used
`QFileInfo::baseName()`, which stops at the first dot, so a user template `My.Dock.view.latte`
was shown as `My`, while `--available-dock-templates` listed `My.Dock`.

## Decisions

- **Two names select a dock template.** The shown name is matched first, in table order,
  exactly as before; the untranslated base name is the fallback. Every name that selected a
  template before still selects the same template. The reverse order was rejected: a base
  name equal to another template's shown name would then pick a different template than the
  UI shows. The rule lives in `app/templates/viewtemplatenames.{h,cpp}` (a pure unit that the
  test compiles directly).
- **`--available-dock-templates` keeps printing base names.** They do not change with the UI
  language, so scripts can rely on them, and the running dock and the start-up check accept
  them. The shown name is accepted too but not printed. The listed names are now the names
  the dock accepts (before, the running dock rejected them under a translated UI).
- **The D-Bus method `addView(uint, QString)` keeps its signature.** A containment id of 0
  takes a name, as before. A containment id above 0 takes a name now; an absolute path to an
  existing file is still accepted, as before. Any other value is logged, not read as a
  relative file name.
- **The start-up check uses the running dock's rule.** `Importer::hasViewTemplate()` builds the
  same entries as `Templates::Manager` (`viewTemplateEntries()`: the shell folder first, then
  the user folder) and selects with `selectViewTemplateId()`. The layout import happens only
  after the check passes.
- **An unknown name is logged, not silent.** `GenericLayout::newView()` and the containment
  branch of `Corona::addView()` log a `qCWarning(latteApp)` with the requested name and the
  known names (`Manager::knownViewTemplateNames()`, for example `Default Dock (默认停靠栏)`).
  The start-up check logs the same warning. It runs before `installDebugMessageHandler()` in
  `main()`, so the `--debug` filter does not discard it. An empty template file is logged in
  `GenericLayout::newView()` and `View::newView()`.

## Side effects

- A user template whose base name equals a shipped template's base name is now renamed
  `… - 2` when it is saved. `Manager::hasViewTemplate()` also matches base names, so the
  uniqueness check (`uniqueViewTemplateName()`) avoids the names that the CLI would confuse.
  Before, this check compared shown names only, so under zh_CN `Default Dock` was free.
- A user template whose file name contains a dot is now shown with the whole name (`My.Dock`)
  instead of its first part (`My`). The GUI list changes for such templates only.
- `--available-dock-templates` and the start-up check use the template manager's file filter,
  which includes hidden files. A hidden `*.view.latte` file is now listed, as the GUI already
  showed it. No other output changes.

## Changes

- `app/templates/viewtemplatenames.{h,cpp}` (new): `ViewTemplateEntry`,
  `viewTemplateBaseName()` (suffix removal only), `selectViewTemplateId()` (two passes).
- `app/templates/templatesmanager.{h,cpp}`: `viewTemplateEntries()` (one listing rule for the
  manager and the CLI), `hasViewTemplate()` and `viewTemplateFilePath()` use the resolver,
  `knownViewTemplateNames()`.
- `app/layouts/importer.{h,cpp}`: `hasViewTemplate()` for the start-up check;
  `availableViewTemplates()` lists base names through the same listing.
- `app/main.cpp`: `applyPostLockOptions()` uses `Importer::hasViewTemplate()` and warns.
- `app/layout/genericlayout.cpp`: `newView()` warns on an unknown name and on an empty file.
- `app/lattecorona.cpp`: the containment branch of `addView()` resolves names.
- `app/view/view.cpp`: `newView()` warns on an empty template file.
- `autotests/viewtemplatenamestest.cpp` (new, nine cases) and its registration in
  `autotests/CMakeLists.txt`; three source contracts in `autotests/sourcecontracttest.cpp`
  (`genericLayoutNewViewWarnsOnUnknownTemplate`, `coronaAddViewResolvesTemplateNames`,
  `startupAddDockUsesTheDockNameRule`).

## Tests

`autotests/viewtemplatenamestest.cpp` has nine cases and compiles `app/templates/viewtemplatenames.cpp` directly, so the test runs the production resolver.

- Old lookup (shown names only, `QFileInfo::baseName()`), built against the same test: six passed, three failed. `baseNameKeepsDots` returned `"My"` for `My.Dock.view.latte`; `untranslatedBaseNameSelectsSystemTemplate` returned `""` for `Default Dock`, which is the reported defect; `userTemplateSelectsByItsName` returned `""` for `My.Dock`.
- With the fix: nine passed, none failed.
- Scratch mutation, shown names matched after base names: `shownNameWinsOverAnotherTemplatesBaseName` fails (eight passed, one failed), so the order is covered. The file was restored from a backup and checked with `cmp`.
- Source contracts on the base commit `311c93ff5` (`git show 311c93ff5:<file>`): `genericLayoutNewViewWarnsOnUnknownTemplate`, `coronaAddViewResolvesTemplateNames` and `startupAddDockUsesTheDockNameRule` all fail. On the fixed sources all three pass. The file has 212 contract functions before this change and 215 now, counted by definitions.
- Logs are in `/data/tmp/r7h` and are not committed; the lines above are copied from them.

## Build, CTest and gates

Commit `d2bc32e71` holds the code and tests; the record and the plan rows are committed after it.

- gcc-debug, first build of the change (`cmake --build --preset gcc-debug -j8`): exit 0, zero `warning:` lines. `ctest --preset gcc-debug`: exit 0, 62/62 passed (the base had 61; the new test adds one).
- gcc-debug after the final wrapping and comment edits: exit 0, five units compiled, zero `warning:` lines; CTest 62/62 passed, exit 0.
Four presets and the CI-image gates, run on `d2bc32e71` (the container exports that commit with `git archive`; the host runs built the working tree, which differs from it only in the two docs files below):

| Run | Exit codes | Compiler `warning:` lines | CTest |
| --- | --- | --- | --- |
| host `gcc-debug` | configure 0, build 0 | 0 (no unit compiled in this run; the two earlier runs above compiled 5 and 6 units, with 0 warnings) | 62/62 |
| host `gcc-release` | configure 0, build 0 (402 units) | 0 | 62/62 |
| host `clang-debug` | configure 0, build 0 (402 units) | 0 | 62/62 |
| host `clang-release` | configure 0, build 0 (402 units) | 0 | 62/62 |
| CI image `latte-r2c-clazy:noble`, `clang-debug` | configure 0, build 0 | 0 | 61/61 (the image has no `appstreamcli`, so `appstreamtest` is not registered; the host has 62) |

The CI-image gates, with the image's own clang-tidy 18.1.3:

- clazy gate (`scripts/test-clazy.py`): exit 0. 156 production sources (155 before, plus `viewtemplatenames.cpp`), no finding from the 49 gated checks; the 49 negative fixtures are each rejected.
- clang-tidy gate (`scripts/test-clang-tidy.py`): exit 0. 156 production sources, no finding from the 2 gated checks; the 2 fixtures are each rejected.

Other output: qmllint prints lines that start with a capital `Warning:` into the build logs. These are not compiler warnings. The counts are 0 for host `gcc-debug`, 1 for `gcc-release`, 1 for `clang-release` and 5769 for `clang-debug`, which runs the deep QML lint. No QML file changed, the build exit codes are 0, and `qmllintbaselinetest` passes in all four presets.

Commands (run in the worktree; the container runs from the export described above):

```bash
cmake --preset gcc-debug && cmake --build --preset gcc-debug -j8 && ctest --preset gcc-debug
# the same three steps for gcc-release, clang-debug and clang-release
git archive d2bc32e71bf730c5abb887dcdbe823bb0989e635 | tar -x -C /data/tmp/r7h/export && mkdir -p /data/tmp/r7h/export/build
docker run --rm -v /data/tmp/r7h/export:/src:ro -v /data/tmp/r7h/container-build:/src/build -w /src latte-r2c-clazy:noble bash -c '
  export LATTE_RELEASE_VERSION="$(python3 scripts/release-version.py resolve)"
  cmake --preset clang-debug -DVERSION="$LATTE_RELEASE_VERSION" && cmake --build --preset clang-debug -j8 && ctest --preset clang-debug
  python3 scripts/test-clazy.py --build-dir build/modernization/clang-debug --jobs 8
  python3 scripts/test-clang-tidy.py --build-dir build/modernization/clang-debug --jobs 4'
```

Each step's exit code is recorded above; the logs are in `/data/tmp/r7h` (not committed).

## Desktop check (zh_CN)

Run on 2026-11-10 after the user's go-ahead, while the R7 desktop checklist was done with its dock (pid 572691, unit `latte-dock-ng-debug-r7-…`), which was stopped first. The check used a user-mode Debug build of `d2bc32e71`, the steps below:

1. From the worktree: `bash install.sh --user Debug`, then `pkill -x latte-dock-ng || true` and `rm -f /tmp/latte-ng.log`.
2. `source ~/.config/latte-dock-ng/dev-env.sh`, then launch the dock as AGENTS.md step 5 says for AI agents (`systemd-run --user` with the launcher script, which sources `dev-env.sh`).
3. `~/.local/bin/latte-dock-ng --add-dock "Default Dock"`: one dock is added.
4. `~/.local/bin/latte-dock-ng --add-dock "默认停靠栏"`: a second dock is added.
5. Remove both new docks by right-clicking each and choosing 移除停靠栏(&R).
6. Optional: `~/.local/bin/latte-dock-ng --add-dock "No Such Dock"` adds nothing, and `/tmp/latte-ng.log` has the line `No dock is added: the dock template "No Such Dock" is unknown. Known dock templates: ...`.
7. Read `/tmp/latte-ng.log`: steps 3 to 5 must add no warning or error line.

Result: passed for the two adds, and the user confirmed that both docks show on the screen.

- Layout: both views went into `R7f test`, the active single-mode layout (`singleModeLayoutName=R7f test` in `lattedockrc`). Every `Called for layout:` line in the log names it. As the R7 checklist session reports, `R7f test` is a copy of 我的布局 that the checklist imported at 20:26 (its F3 step, through `--import-layout`). These checks did not use 我的布局: its file `我的布局.layout.latte` is still dated 00:53 and holds containments 1 and 13.
- Install: `bash install.sh --user Debug` exit 0; log in `/data/tmp/r7h/install-user.log`. No privilege escalation (`sudo_cmd` is empty in user mode).
- The new dock started at 21:01:23 (pid 613162, unit `latte-dock-ng-debug-20261110-210124`, exe `~/.local/bin/latte-dock-ng`). Its locale is `zh_CN.utf8`, the same as plasmashell's.
- `--add-dock "Default Dock"`: exit 0. The log has `Adding View: 12 - Passed ALL checks` at 21:01:58 on the primary screen DP-2.
- `--add-dock "默认停靠栏"`: exit 0. The log has `Adding View: 14 - Passed ALL checks` at 21:02:16.
- Removal: the two test docks were removed over D-Bus, `removeView` for 12 and then 14, both exit 0. The dock logged `dock containment destroyed changed` twice at 21:04:44. `Corona::removeView()` ends in `View::removeView()`, the same action as the context menu. The right-click path itself was not exercised, and the user's docks (containments 1 and 13) were left alone.
- Log, from 21:01:23 to 21:05:43: no `No dock is added` line and no fatal, assert or crash line. One `Warning:` line at 21:04:44.081, `Failed to notify "Created too many similar notifications in quick succession"`, came from the notification service right after the second removal. The add path logged nothing like it, and the removal path is not changed by this commit.

- Later, at 21:05:43, `R7f test.layout.latte` was saved after view 12 was removed. Its `[Containments][14]` section, the second test dock removed over D-Bus, is still in the file. No save followed before the abort below.
- Abort at 21:07:40. Two removals ran from the Layouts Editor's docks dialog (`ViewsHandler::removeSelectedViews()`, log lines 21:07:36 and 21:07:39). The R7 session reports that the user started them with 移除所选视图 on docks 1 and 13. The log does not record the view ids. The second removal emptied the table. Then `rowCount() - 1` is -1, so `viewscontroller.cpp:371`, `m_view->selectRow(qBound(0, selectionheadrow, m_model->rowCount() - 1))`, asks `qBound` for a range from 0 to -1. Qt's `qBound` asserts `!(max < min)` (`qminmax.h`), and this Debug build aborted with SIGABRT; a core dump is present. The log ends there, and the dock was not running afterwards. This commit does not change `viewscontroller.cpp`, so the defect predates it and shows only where `Q_ASSERT` is enabled. The R7 session's backtrace (frames #7 to #9) shows the same call. The release tree's compile flags define `QT_NO_DEBUG`, which compiles the assert out.
- Resolved: the dock restarted at 21:08:54 (pid 615700) and rewrote `R7f test.layout.latte` at 21:08:56 with containments 1 and 13 only, so the stale `[Containments][14]` section is gone.

The scope of this record is closed by the merge into `main`. The abort is a separate defect for a follow-up, not part of this slice.

## Limits

- The start-up check and the `--available-dock-templates` listing read the packaged shell through `Importer::systemShellDataPath()`, the last XDG data directory. The running dock reads the shell through KPackage, which finds a user-mode shell first. The two differ only if a user-mode shell adds or renames a template. This listing rule was already in place and is not changed here.
- The start-up check translates shown names in the CLI process's own locale. Run under another locale than the dock's, it rejects a shown name with the warning; base names are accepted in every locale.
- A user template whose base name equals a shipped template's base name now gets the ` - 2` suffix when it is saved. The old check compared shown names only, so the collision was not seen under zh_CN.
- No unit test reaches the live dock or the Qt message routing; the desktop check covers those.
