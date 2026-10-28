# R2c-non-pod-global-static: Namespace-Scope QStrings Become Accessors

Status: Pending validation. The evidence section below lists what was run; CI
and the desktop smoke were not run here.
Date: 2026-10-29. Branch: `claude/r2c-batch-f-e6a6ee`. Start HEAD: `085fbd190`
(the `R2c-connect-3arg-lambda` commit on `95e342f72`).
Implementation commit: the commit that adds this report. Part of [R2c](R2c.md),
fix batch 3 (batch F, the second slice).

## Defect

clazy `non-pod-global-static` reported 31 namespace-scope objects of non-POD type
in 6 files. The 18 in `app/data/contextmenudata.h` are 13 `QString` constants and
5 `QStringList` lists; the other 13 are in the table below. Each object runs a
constructor at program start and a destructor at exit, and a header constant
gives every translation unit its own copy. A global initializer in another
translation unit that read one of these during static initialization could see
an empty value, because the order of static initializers across translation
units is not defined.

| File | Findings | Names |
| --- | ---: | --- |
| [contextmenudata.h](../../app/data/contextmenudata.h) | 18 | `ADDVIEWACTION` ... `SEPARATOR1ACTION` (13 `QString`), `ACTIONSEDITORDER`, `ACTIONSALWAYSVISIBLE`, `ACTIONSALWAYSHIDDEN`, `ACTIONSVISIBLEONLYINEDIT`, `ACTIONSSPECIAL` (5 `QStringList`) |
| [abstractlayout.h](../../app/layout/abstractlayout.h) | 1 | `Latte::Layout::MULTIPLELAYOUTSHIDDENNAME` |
| [viewstable.cpp](../../app/data/viewstable.cpp) | 1 | `Latte::Data::TEMPIDPREFIX` |
| [appletids.cpp](../../app/layouts/appletids.cpp) | 2 | `APPLETSGROUP`, `GENERALGROUP` (anonymous namespace) |
| [plasmapaneltracker.cpp](../../app/wm/plasmapaneltracker.cpp) | 2 | `service`, `path` (anonymous namespace) |
| [previewprocess.cpp](../../plasmoid/plugin/previewprocess.cpp) | 7 | `TypeShow`, `TypeMove`, `TypeHide`, `TypeHeartbeat`, `TypeActivate`, `TypeClose`, `TypeClosed` (anonymous namespace) |

The 18 findings of `contextmenudata.h` are counted at the original header; the
generated copy in the build directory is not counted (see [R2c](R2c.md#what-is-analyzed)).

## Fix

Each constant became an accessor. A header accessor is `inline` and returns a
`const` reference to a function-local `static const` that is built on first use;
a `.cpp` accessor is a plain function in the existing anonymous namespace (or a
`static` function where there was none). The lists are built from the accessors,
in the original order. Names and string values are unchanged. Every use gained
`()`.

| Site | Change | Uses changed |
| --- | --- | --- |
| [contextmenudata.h](../../app/data/contextmenudata.h) | 13 `QString` and 5 `QStringList` accessors; each list is the original initializer with `NAME()` | the header itself |
| [menu.cpp](../../containmentactions/contextmenu/menu.cpp) | `Latte::Data::ContextMenu::NAME` → `NAME()` | 72 lines (82 occurrences) |
| [actionshandler.cpp](../../app/settings/actionsdialog/actionshandler.cpp) | same | 22 lines |
| [preferencesdata.h](../../app/data/preferencesdata.h) | default member initializer of `contextMenuAlwaysActions` | 1 |
| [preferencesdata.cpp](../../app/data/preferencesdata.cpp) | comparison and assignment of the always-visible list | 2 |
| [universalsettings.cpp](../../app/settings/universalsettings.cpp) | default of `readEntry` | 1 |
| [dataunittest.cpp](../../autotests/dataunittest.cpp) | `QCOMPARE` (one line) | 1 |
| [abstractlayout.h](../../app/layout/abstractlayout.h) | `inline const QString &MULTIPLELAYOUTSHIDDENNAME()` | the header |
| [manager.cpp](../../app/layouts/manager.cpp) | `Layout::MULTIPLELAYOUTSHIDDENNAME` → `()` | 121, 122 |
| [importer.cpp](../../app/layouts/importer.cpp) | same | 754, 767, 787, 807 |
| [storage.cpp](../../app/layouts/storage.cpp) | same | 618, 1865 |
| [synchronizer.cpp](../../app/layouts/synchronizer.cpp) | same | 537, 697 |
| [templatesmanager.cpp](../../app/templates/templatesmanager.cpp) | same | 144 |
| [viewstable.cpp](../../app/data/viewstable.cpp) | `static const QString &TEMPIDPREFIX()` | definition, 3 uses |
| [appletids.cpp](../../app/layouts/appletids.cpp) | `APPLETSGROUP()` and `GENERALGROUP()` in the anonymous namespace | definitions, 6 uses |
| [plasmapaneltracker.cpp](../../app/wm/plasmapaneltracker.cpp) | `service()` and `path()` in the anonymous namespace | definitions, 6 uses |
| [previewprocess.cpp](../../plasmoid/plugin/previewprocess.cpp) | seven `const QString &TypeX()` in the anonymous namespace | definitions, 7 lines of uses |

Two checks were made on the `.cpp` names:

- **No collision.** `service` and `path` are not declared as a local, parameter
  or member anywhere in `plasmapaneltracker.cpp` or its header. The only other
  `service` in the file is `message().service()`, a member of `QDBusMessage` that
  is not touched. The names were kept, so no accessor was renamed.
- **Test contract.** `sourcecontracttest.cpp` checks that `previewprocess.cpp`
  contains `TypeHeartbeat`, `TypeActivate`, `TypeClose`, `TypeMove` and
  `TypeClosed`. Each still appears, as a call (`TypeHeartbeat()`).

## Static initialization

The change removes the namespace-scope objects, so no construction or
destruction happens at program start or exit for these names. Nothing reads them
during static initialization:

- Every use is inside a function body or in the default member initializer of
  `Preferences::contextMenuAlwaysActions` (`preferencesdata.h:42`). A search of the
  production and test sources for lines that start at column 0 and call an accessor
  finds only the body of `scriptingCall()` in `plasmapaneltracker.cpp`.
- No `Preferences` object exists at namespace scope (`git grep`): the default
  initializer runs only when a `Preferences` is constructed at run time.
- Even a global object constructed during static initialization would now be
  safe: the accessor builds its value on that first call, so the order of
  initializers no longer matters. Before the change the same call could read an
  empty list.

## Evidence

Container and toolchain as in [R2c-connect-3arg-lambda](R2c-connect-3arg-lambda.md#evidence):
Ubuntu clang 18.1.3, clazy 1.17 on LLVM 19.1.1, Qt 6.11.1. Host: GCC 16.2.1, Clang
22.1.8, Qt 6.11.2. The container commands are the same as for the first slice.

| Command | Result |
| --- | --- |
| `test-clazy.py --inventory connect-3arg-lambda,non-pod-global-static --report build/inventory.json`, at `085fbd190` | `non-pod-global-static` **31 findings in 6 files**: the original `contextmenudata.h` counted once, its 18 findings in the generated copy ignored; `connect-3arg-lambda` 0 |
| Same, after the change (container build of the tree first) | Exit 0: `non-pod-global-static` **0 findings in 0 files**; `connect-3arg-lambda` 0 in 0 files; 0 findings ignored as generated |
| Container `cmake --build --preset clang-debug -j4` (incremental, includes the autotests) | Exit 0; 0 compiler errors, 0 `warning:` lines |
| `diff` of `build/modernization/clang-debug/containmentactions/contextmenu/contextmenudata.h` (generated by `configure_file`) and `app/data/contextmenudata.h` | Identical: the configure step refreshed the copy |
| Full gate: `test-clazy.py --build-dir build/modernization/clang-debug --jobs 4` | **Exit 0**: 45 negative fixtures, each rejected by its own gated check; 153 production sources with no finding from 45 gated checks |
| Mutation: the pre-fix `plasmoid/plugin/previewprocess.cpp` mounted read-only over the fixed file, then the full gate | **Exit 1**: 7 findings, `previewprocess.cpp:26:1` to `32:1` (`TypeShow` ... `TypeClosed`), all `[non-pod-global-static]`; 45 fixtures were still rejected |
| `git grep` for uses without `()` of the 31 names; for `()()` | 0 bare uses of the `Latte::Data::ContextMenu` names; the other names appear only in string literals of `sourcecontracttest.cpp` (`QStringLiteral("TypeHeartbeat")`, which the accessor name still contains); 0 doubled parentheses |
| `git grep` for namespace-scope code that calls an accessor (lines starting at column 0) | Only the body of `scriptingCall()` in `plasmapaneltracker.cpp` |
| `git clang-format --style=file --diff HEAD` on the changed files | Proposals only (see Formatting); none applied |
| `git diff --check` | Clean |
| Host `cmake --preset gcc-debug` (re-run), `cmake --build --preset gcc-debug -j4`, `ctest --preset gcc-debug` | Configure 0, 0 CMake warnings; build 0 with **0 compiler warnings**; **58/58 tests passed** (26 s) |
| Host `cmake --preset clang-debug` (re-run), `cmake --build --preset clang-debug -j4`, `ctest --preset clang-debug` | Configure 0, 0 CMake warnings; build 0 with **0 compiler warnings**; **58/58 tests passed** (22 s) |

The host builds of this slice are incremental on the trees that were built from
scratch for [the first slice](R2c-connect-3arg-lambda.md#evidence). CMake's
dependency tracking rebuilds every target that includes the changed headers
(`contextmenudata.h` and `abstractlayout.h` are included by most of the
application), and the warning count is taken over all of those rebuilt files.

### Formatting

`git clang-format --style=file --diff HEAD` proposes four kinds of change, none of
which this slice applies, because each would reflow lines the change does not
touch:

- a line break after the return type of each top-level accessor (the project's
  `callLatte()` in `menu.cpp` keeps the return type on the name's line);
- spaces inside braces in the list initializers and in `QStringList x{Data::...}`
  (the surrounding code writes `{...}`);
- joining the multi-line list initializers of `contextmenudata.h` onto one line;
- rewrapping the `return (...)` expression of `Preferences::inDefaultValues()`
  (`preferencesdata.cpp`), which was already not clang-format clean.

The changed lines keep the layout of the code around them. `formatter.sh` was not
run: it rewrites whole files.

## Known gap: `app/main.cpp`

The gate does not see two namespace-scope statics in `app/main.cpp`
(lines 752-753, `static QStringList s_userLocalQmlPaths` and
`s_userLocalPluginPaths`, neither with an initializer). The file is outside this
batch's list, so both are unchanged.

The cause is in clazy 1.17 and depends on the file name. In the clazy container
with the project's flags, the same content produces these findings:

| File name (same content) | Reported |
| --- | --- |
| `main-real-copy.cpp` (a copy of `app/main.cpp`) | lines 752 and 753 |
| `mainx.cpp` (a probe with one static without and one with an initializer, twice) | all four |
| `main.cpp` (the same probe, in another directory) | only the two with an initializer |
| `main.cpp` (a copy of `app/main.cpp`, or the file itself) | nothing |

So a `static` without an initializer is skipped when the basename is exactly
`main.cpp`. The gate's fixture, `autotests/clazy/non-pod-global-static.cpp`, has
a different name and an initializer, so it cannot show this. The other tracked
`main.cpp`, `plasmoid/preview/main.cpp`, has no namespace-scope `QString` or
`QStringList`. The gate's "0 findings" for this check is therefore not complete
for `app/main.cpp`. Closing this needs a gate-side fixture, or a conversion of
the two statics by the owner of `app/main.cpp`.

## Handoff

- **Files and behavior changed:** `app/data/contextmenudata.h`,
  `app/layout/abstractlayout.h`, `app/data/viewstable.cpp`,
  `app/layouts/appletids.cpp`, `app/wm/plasmapaneltracker.cpp`,
  `plasmoid/plugin/previewprocess.cpp`, the uses in `menu.cpp`,
  `actionshandler.cpp`, `preferencesdata.h/.cpp`, `universalsettings.cpp`,
  `manager.cpp`, `importer.cpp`, `storage.cpp`, `synchronizer.cpp`,
  `templatesmanager.cpp` and `autotests/dataunittest.cpp`, the gate entry
  `non-pod-global-static` in `scripts/test-clazy.py`, the fixture
  `autotests/clazy/non-pod-global-static.cpp`, and this report. The strings and
  lists are unchanged. The only difference is when a string is built: on first
  use, not at program start.
- **State authority / lifetime constraints:** each value has one owner, the
  function-local static of its accessor. Nothing is invalidated, because the
  values are constant.
- **Tests:** no test added. Host CTest on the same tree: see Evidence. The
  `sourcecontracttest` checks that read `previewprocess.cpp` still find their text.
- **Smoke plan (for the integration):** the context-menu actions editor (its list
  order and the always-visible defaults), the dock context menu (items and their
  order, including Add view and Quit), the layout menu, the view settings (the
  "always shown" context-menu actions read from the universal settings), the
  window previews on task hover (`previewprocess.cpp`, the helper process and its
  heartbeat), a layout with the hidden multiple-layouts file (import and switch
  with multiple layouts in memory), a new view from a template (temporary ids),
  and a layout save, reload and copied containment (the applet ID groups of
  `AppletIds`). Look for `No such method` or `QDBusError` lines in the log, and for
  any `Warning:` line the log did not have before.
- **Not run:** CI, the desktop smoke, a Qt 6.6 or 6.8 build, the Release presets.
- **Next exact action:** the main session decides how the `app/main.cpp` gap is
  closed (see Known gap), then merges this branch (`085fbd190` and this commit),
  runs the combined checks and the desktop smoke, and updates `R2c.md` and the
  ledger.
- **Commit approval status / push approval status:** committed on
  `claude/r2c-batch-f-e6a6ee` (a non-`main` branch); not pushed.
