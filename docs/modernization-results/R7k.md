# R7k: the docks dialog's template and import entries

Status: fix committed as `bd045d9ee` on branch `claude/fervent-sinoussi-f8ffb2`, based on `main` at `99a09d1f2`. Not merged into `main`. The zh_CN desktop check has not run; it needs the user's approval, because it installs a build and replaces the running dock. The packaged dock is still stopped; the R7i session owns its restore.

## Report

On the user's zh_CN desktop, with the R7i Debug build (pid 70050), single-layout mode and the layout 我的布局 (two bottom docks): right-click a dock → 编辑布局 → 布局编辑器 → 我的布局 → 停靠栏… → 新建 → a dock template. The report says nothing appeared in the dock list, a file selection window opened, and clicking in that window did not import anything. The Import… entry was expected to share the defect.

## Which menu item opened the file window

The 新建 menu lists the dock templates (the shipped one shows as 默认停靠栏), a separator, then 模板… (`Templates...`). Two items open a file window:

- **模板…** (`initViewTemplatesSubMenu()` in `app/settings/viewsdialog/viewshandler.cpp`) called `KIO::highlightInFileManager()` on `~/.config/latte/templates/Dock.view.latte`. That file does not exist. The shipped template is `Default Dock.view.latte` in the shell package, and the user's folder holds only layouts. The file manager therefore opened the folder with nothing highlighted, which is a file window that imports nothing.
- **导入…** (`importView()`) opens a Qt file dialog, 导入停靠栏, for `*.view.latte`.

Only 模板… is reached from the 新建 menu and opens a file manager. The record cannot tell which item the user clicked. The desktop check asks the user to confirm it (step 1 below).

## What the code does on base (99a09d1f2)

- **Template entry.** `ViewsHandler::newView(templateData)` calls `Layouts::Storage::views(templateData.id)`. The id is the template's file path (`Templates::Manager::viewTemplateEntries()`), and `views(file)` reads that file. The rows go to `Views::appendViewFromViewTemplate()` and then `Model::Views::appendTemporaryView()`, which inserts a row. Nothing in the proxy hides it.
- **Import.** `importView()` passes the picked path to the same `newView()`.
- **Probe.** `R7k-tools/probe-views.cpp` makes the same KConfig calls as `Storage::views(file)` and `isLatteContainment()`. On the installed `Default Dock.view.latte` it returns one containment with `plugin=org.kde.latte.containment` (`R7k-data/probe-views-output.txt`). The probe is a copy of those calls, not the Latte code, so it shows that the file parses, not that the dialog appends the row.
- **Silent no-op.** `newView()` returned without a word when `views(file)` gave no rows. That happens for a file that holds no dock, such as a non-dock file typed into the Import dialog or an empty template. The shipped template does not take that path.
- **Apply.** `Views::save()` calls `central->newView()`, which ends in `Storage::newView()`. For an active layout, the import can yield no containment, and `Storage::newView()` then returns an invalid view. `save()` stored that view over the pending row with `setOriginalView()`. The row stayed in the table as a blank row, with no message.

Ruled out by reading and by the probe: the template id (the file path), the name translation, the KConfig read of the shipped template, a proxy filter that could hide the row. Not ruled out: the user's click target, and whether a message appeared. The desktop check settles both.

## Change

- `app/settings/viewsdialog/viewshandler.cpp`
  - 模板… sits below a separator and opens the templates folder (`~/.config/latte/templates`, created when missing) with `KIO::OpenUrlJob`. The file it named before is gone.
  - `newView()` logs a warning and shows an inline warning, "<b>%1</b> has no dock, so no dock was added.", when the file holds no dock. The success path is unchanged.
- `app/settings/viewsdialog/viewscontroller.cpp`, `Views::save()`
  - A new dock that the layout refuses (an invalid view from `central->newView()`) is no longer stored over its row. Its row is removed, and an error says "Could not add <b>%1</b> to the layout."
  - A cut or paste whose destination refused the dock keeps its origin view. The origin removal skips it, so the dock is not lost.
- `autotests/sourcecontracttest.cpp`: three contracts, `viewsTemplatesMenuOpensTemplatesFolder`, `viewsDialogNewViewReportsTemplateWithoutDock` and `viewsDialogSaveReportsRefusedDock`.

The user-visible strings are new msgids. They fall back to English until the translators update the catalogues.

## Red and green

The contracts read the source text, so a red run swaps the base app files in, runs the test binary, and restores the fix. The restored files match `bd045d9ee`.

| Run | Source | Command | Exit | Result |
| --- | --- | --- | --- | --- |
| red | `main` (99a09d1f2) app files | `QT_QPA_PLATFORM=offscreen build/modernization/gcc-debug/bin/sourcecontracttest viewsTemplatesMenuOpensTemplatesFolder viewsDialogNewViewReportsTemplateWithoutDock viewsDialogSaveReportsRefusedDock` | 3 | 2 passed, 3 failed |
| green | `bd045d9ee` | the same command | 0 | 5 passed (with the initTestCase and cleanupTestCase rows) |

Logs: `R7k-data/contracts-red-base.log`, `R7k-data/contracts-green-head.log`. The full source-contract run on `main` before the change, `sourcecontracttest` with no arguments, gave 227 passed and 0 failed.

## Host builds and CTest

Each preset was configured and built with its own build tree under `build/modernization/<preset>` and then run with `ctest --preset`. Compiler warnings are the lines that contain `warning:` in the build log.

| Preset | Configure | Build | Compiler warning lines | CTest |
| --- | --- | --- | --- | --- |
| gcc-debug | 0 | 0 | 0 | 63/63 passed, exit 0 |
| clang-debug | 0 | 0 | 0 | 63/63 passed, exit 0 |
| gcc-release | 0 | 0 | 0 | 63/63 passed, exit 0 |
| clang-release | 0 | 0 | 0 | 63/63 passed, exit 0 |

The gcc-debug and clang-debug objects of the changed files were built after the last edit, which is checked by timestamps. Logs: `R7k-data/host-presets-summary.txt`, `R7k-data/ctest-totals.txt`.

## CI-image gates

Run in the local image `latte-r2c-clazy:noble` (Ubuntu 24.04, clazy 1.17, Ubuntu LLVM 19.1.1 for clazy, clang-tidy 18.1.3 for clang-tidy), on `git archive bd045d9ee` mounted read-only at `/src`, with a writable build tree at `/src/build`, as in R2c and R8i. The release version resolves to 1.2.59.

| Step | Exit | Result |
| --- | --- | --- |
| clang-debug configure and build in the container | 0 | 0 compiler warning lines |
| `python3 scripts/test-clazy.py --build-dir build/modernization/clang-debug --jobs 8` | 0 | 156 production sources, no finding from 49 gated checks; 49 negative fixtures each rejected by its own check |
| `python3 scripts/test-clang-tidy.py --build-dir build/modernization/clang-debug --jobs 8` | 0 | 156 production sources, no finding from 2 gated checks; 2 negative fixtures each rejected |

Logs: `R7k-data/ci-gates-summary.txt`, `R7k-data/ci-clazy-tail.log`, `R7k-data/ci-clang-tidy-tail.log`.

## Desktop check (not run)

The check needs the user's approval before it starts. Its steps, in zh_CN, with the Latte Debug build from this branch installed in `~/.local`:

1. On the dock: right-click → 编辑布局 → 布局编辑器 → select 我的布局 → 停靠栏… . In the 新建 menu, record which entries are there.
2. 新建 → 默认停靠栏. Expect a row in the list and the message "默认停靠栏 added successfully…". Then 立即应用. Expect the new dock on the desktop; see the follow-up below about the edge.
3. 新建 → 模板… (the item below the separator). Expect a file manager on the templates folder and no new row.
4. 导入… → pick the shipped `Default Dock.view.latte` from `~/.local/share/plasma/shells/org.kde.latte.shell/contents/templates/`. Expect a row. Then pick a file that holds no dock. Expect the warning "<b>…</b> has no dock, so no dock was added."
5. Log: `grep -E "No dock is added|refused the new dock|Could not add" <log>`.

## Follow-up (not changed here)

- The dialog's template and import rows keep the template's own edge. `GenericLayout::newView(templateName)` (`--add-dock`) and `View::newView(templateFile)` (the dock context menu) first pick a free edge on the primary screen. In 我的布局, with two bottom docks, a bottom template dock therefore overlaps the existing docks after Apply. The report did not ask for this change, and it would change the row's edge, so it is left for the user to decide after the desktop check.
- The English handbook describes New and Import, not 模板…. The release help gate in AGENTS.md covers that at release time.

## Reproduce

```bash
cd <worktree>
cmake --build build/modernization/gcc-debug --target sourcecontracttest -j8
QT_QPA_PLATFORM=offscreen build/modernization/gcc-debug/bin/sourcecontracttest viewsTemplatesMenuOpensTemplatesFolder viewsDialogNewViewReportsTemplateWithoutDock viewsDialogSaveReportsRefusedDock
g++ -std=c++20 -fPIC $(pkg-config --cflags Qt6Core) -I/usr/include/KF6 -I/usr/include/KF6/KConfigCore -I/usr/include/KF6/KConfig docs/modernization-results/R7k-tools/probe-views.cpp -o /data/tmp/dock-template-fix/probe-views $(pkg-config --libs Qt6Core) -L/usr/lib64 -lKF6ConfigCore
/data/tmp/dock-template-fix/probe-views "$HOME/.local/share/plasma/shells/org.kde.latte.shell/contents/templates/Default Dock.view.latte"
```
