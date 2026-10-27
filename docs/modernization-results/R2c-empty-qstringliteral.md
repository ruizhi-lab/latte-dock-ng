# R2c-empty-qstringliteral: an Empty `QStringLiteral` as the Separator Icon

Status: Pending validation. The fix, the fixture and the gate pass locally with
Clang and GCC; the CI run and the combined desktop smoke of R2c batches A and B
are not done.
Date: 2026-10-27. Branch: `claude/r2c-batch-b-c113e5`. Start HEAD: `09b030e61`.
Implementation commit: the commit that adds this report. Part of [R2c](R2c.md),
the first slice of batch B (then `R2c-use-static-qregularexpression` and
`R2c-connect-not-normalized`).

## Defect

`ActionsHandler::initItems()` in `app/settings/actionsdialog/actionshandler.cpp`
gave the separator row of the context menu actions editor its icon with

```cpp
new ActionsDialog::ActionListWidgetItem(QIcon::fromTheme(QStringLiteral("")),
    i18n(" --- separator --- "),
```

clazy reports `Use QLatin1String("") or QString() instead of an empty
QStringLiteral` at `actionshandler.cpp:82:80`. There is no user-visible defect:
the row is meant to have no icon, and the code obtained that by asking the
platform icon engine for a theme icon named "". The finding is about intent and
cost: an empty `QStringLiteral` is a static, non-null empty string, and the
lookup ends in an icon that has no pixmaps.

## Equivalence of `QIcon()`

`QIcon::fromTheme("")` is not the same object as `QIcon()`. It returns an icon
that holds an icon engine, which reports itself null; `QIcon()` holds none.
Replacing one with the other is only safe if nothing in the dialog can tell them
apart, so both were compared with the real widget. The scratch probe (not part of
the repository) builds a `KActionSelector`, puts three rows into its "available"
list (Layouts, the separator row under test, Add Widgets) and compares the
separator row for four icons: the old call, `QIcon()`, `QIcon::fromTheme(QString())`
and two controls that must differ (`QIcon::fromTheme("list-add")` and a
`QIcon(QPixmap)`).

Properties of the two icons, Qt 6.11.2 (Qt 6.4.2 prints the same values):

| Property | `QIcon::fromTheme(QStringLiteral(""))` | `QIcon()` |
| --- | --- | --- |
| `isNull()` | `true` | `true` |
| `name()` | empty | empty |
| `availableSizes()` | empty | empty |
| `pixmap(16x16).isNull()` | `true` | `true` |
| `QVariant::fromValue(icon).isNull()` | `false` | `false` |
| `actualSize(16x16)` | `0x0` | `-1x-1` |
| `cacheKey()` | not 0 (it holds an engine) | `0` |

The last two rows are the only differences. `QListWidgetItem` stores the icon in
its item data, `ActionListWidgetItem` adds an id and an order, and
`ActionsHandler` reads only those two roles; nothing in `app/settings/actionsdialog`
calls `icon()`. The icon is therefore consumed only by Qt's item view code, which
asks it for `actualSize()` when it lays the row out; the rendering comparison below
runs that code with both icons. (`QStringLiteral("")` is a non-null empty string
and `QString()` a null one; they compare equal.)

Rendering of the separator row in the `KActionSelector` list (600x300 window):

| Environment | Row rectangle, size hint | List image, normal and current row | Controls change the image |
| --- | --- | --- | --- |
| Qt 6.11.2, Breeze style, generic platform theme, `breeze` icon theme | `(0,24 274x22)`, 22 | identical, identical | yes (row 24 high) |
| Qt 6.11.2, Plasma platform theme (`KDEPlasmaPlatformTheme6`, so `KIconEngine`), Klassy style, Colloid icons | `(0,24 274x22)`, 22 | identical, identical | yes (row 24 high) |
| Qt 6.11.2, Fusion, no icon theme | `(0,16 280x16)`, 16 | identical, identical | only the pixmap icon (row 18 high); the theme icon does not resolve here |
| Qt 6.4.2 (Ubuntu 24.04 container), plain `QListWidget`, Fusion, `breeze` icon theme | `(0,14 598x14)`, 14 | identical, identical | only the pixmap icon (row 18 high); the theme icon does not resolve here |

`QIcon::fromTheme(QString())` matches the old call in every row and property,
including `actualSize()` and `cacheKey()`. `QIcon()` was chosen because it states
the intent (a row without an icon) and does not consult the platform icon engine.
Qt 6.6 itself was not run; the oldest Qt available was 6.4.2, the newest 6.11.2.

## Fix

`QIcon()` in the same place. The gate side is one gate entry and one fixture, as
in every R2c slice: the name in `GATED_CHECKS` and
`autotests/clazy/empty-qstringliteral.cpp`, which holds the smallest violation
(`return QStringLiteral("");`). The gate now has 32 entries and 32 fixtures.

## Evidence

Container commands are those of [R2c](R2c.md#reproducing-the-environment), with a
private build directory (`~/r2c-build-batch-b`) and `--jobs 6` because another
session ran at the same time; the host was loaded (load average 15 to 24), so the
times are longer than an idle run.

| Command | Result |
| --- | --- |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 6 --inventory empty-qstringliteral --report FILE` (with the two other batch B checks, before the fix) | 1 finding: `app/settings/actionsdialog/actionshandler.cpp:82:80`, as in R2c |
| `test-clazy.py ... --inventory empty-qstringliteral` after the fix | Exit 0 in 232 s: 0 findings in 153 sources |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 6` | **Exit 0** in 182 s: 32 fixtures rejected by their own checks, 153 sources with no finding from 32 checks |
| Same, with the previous `actionshandler.cpp` mounted over the fixed file, no rebuild | **Exit 1** in 141 s: `app/settings/actionsdialog/actionshandler.cpp:82:80: [empty-qstringliteral]` |
| Host `cmake --preset gcc-debug`, `cmake --build --preset gcc-debug -j6`, `ctest --preset gcc-debug` (GCC 16.2.1, fresh tree) | Exit 0 in 516 s, 0 compiler warnings, 57/57 passed |
| Host `cmake --preset clang-debug`, `cmake --build --preset clang-debug -j6`, `ctest --preset clang-debug` (Clang 22.1.8, fresh tree) | Exit 0 in 449 s, 0 compiler warnings, 57/57 passed |
| Probe on Qt 6.11.2 (three environments above) and Qt 6.4.2 | Tables above |

The container build of the changed file produced 0 compiler warnings. The host
build logs hold thousands of `Warning:` lines from the qmllint step; they are the
known QML warnings tracked by the baseline gate, not compiler output, and this
change touches no QML.

## Handoff

- **Files and behavior changed:** `app/settings/actionsdialog/actionshandler.cpp`
  (one argument), `scripts/test-clazy.py` (one name),
  `autotests/clazy/empty-qstringliteral.cpp`. The separator row renders
  identically; `ActionsHandler` reads only the id and order roles.
- **State authority / lifetime constraints:** none; `QListWidgetItem` keeps its
  own copy of the icon.
- **Tests:** no test added or changed; `ctest` 57/57 on Clang and GCC Debug. The
  fixture and the pre-fix gate run are the negative evidence; the rendering probe
  is not in the repository.
- **Not run:** the CI job, Release presets, the desktop retest of AGENTS.md (the
  actions editor was not opened in a running dock; the main session runs one
  combined smoke after both batches merge), Qt 6.6.
- **Next exact action:** review, then the combined smoke. Do not start another
  batch implicitly.
- **Commit approval status / push approval status:** committed on the feature
  branch as the session brief allows; not pushed; `main` untouched.
