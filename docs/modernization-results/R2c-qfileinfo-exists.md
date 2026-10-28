# R2c-qfileinfo-exists: Static `QFileInfo::exists()`

Status: Complete. Merged into `main` in `1dd485bc1`; the checks on `main` and
the combined desktop smoke passed ([R2c fix batch 1](R2c.md#fix-batch-1)). CI
run 40683601580 at `14d705a5d` passed all 23 jobs, including the 43-check clazy
gate ([CI validation](R2c.md#ci-validation)).
Date: 2026-10-27. Branch: `claude/r2c-batch-a-a5a9fa`. Start HEAD: `09b030e61`.
Implementation commit: the commit that adds this report. Part of
[R2c](R2c.md); the first slice of batch A.

## Defect

35 call sites in 11 files asked whether a path exists through a temporary
object, `QFileInfo(path).exists()`. `QFileInfo` has a static overload,
`QFileInfo::exists(const QString &)`, that answers the same question without
building the object; clazy reports the member form with "Use the static
QFileInfo::exists() instead. It's documented to be faster."

There is no functional defect and no known symptom: the finding is a cost and
hygiene finding in the scheme-color, indicator discovery, layout import, theme
and path-lookup code.

## Fix

`QFileInfo(x).exists()` became `QFileInfo::exists(x)` at every flagged site,
mechanically and with balanced parentheses. The inventory counts and the edits
match per file:

| File | Sites |
| --- | ---: |
| `app/wm/schemecolors.cpp` | 10 |
| `app/indicator/factory.cpp` | 7 |
| `app/layouts/importer.cpp` | 4 |
| `app/layouts/storage.cpp` | 3 |
| `app/plasma/extended/theme.cpp` | 3 |
| `app/tools/commontools.cpp` | 3 |
| `app/lattecorona.cpp` | 1 |
| `app/layout/abstractlayout.cpp` | 1 |
| `app/plasma/extended/backgroundcache.cpp` | 1 |
| `app/settings/settingsdialog/layoutscontroller.cpp` | 1 |
| `app/view/indicator/indicatorresources.cpp` | 1 |

Two sites pass a `QStringBuilder` (`QLatin1String(...) + subPath`, in
`app/layouts/importer.cpp` and `app/tools/commontools.cpp`); it converts to the
`const QString &` parameter exactly as it converted for the `QFileInfo`
constructor. Other uses of `QFileInfo` stay: `app/indicator/factory.cpp` keeps
`QFileInfo(uiFile).absolutePath()` next to the changed `exists()` call because
it asks a different question, and clazy does not flag it. No comment was added
at the sites: the constraint is carried by the gate.

The gate side is one gate entry and one fixture, as in every R2c slice: the name
in `GATED_CHECKS` and `autotests/clazy/qfileinfo-exists.cpp`, which holds the
smallest violation (`return QFileInfo(path).exists();`). The gate has 32 entries
and 32 fixtures on this branch.

## Behavior check

A probe program, not committed, compares the temporary form with the static
form on the inputs the production code can pass. It was built with
`-DQT_USE_QSTRINGBUILDER` like the product, g++ 16.2.1 and Qt 6.11.2:

| Input | `QFileInfo(p).exists()` | `QFileInfo::exists(p)` |
| --- | --- | --- |
| existing file, directory, directory with trailing `/`, `.`, `/` | true | true |
| file with a trailing `/`, missing file, missing relative path | false | false |
| empty path | false | false |
| symlink to a file | true | true |
| dangling symlink | false | false |
| missing `:/` resource path | false | false |
| `QStringBuilder` argument, missing and existing path | false, true | false, true |

14 cases, 0 mismatches.

## Evidence

Container commands are those of [R2c](R2c.md#reproducing-the-environment) with
the checkout mounted read-only, a build tree of its own and `-j6`/`--jobs 6`,
because a second session builds at the same time (load average 14 to 16 during
the runs, so the clazy times are longer than the 97 s of an idle host).

| Command | Result |
| --- | --- |
| Baseline `test-clazy.py --build-dir build/modernization/clang-debug --jobs 6 --inventory qfileinfo-exists,range-loop-reference,no-module-include` on an export of `09b030e61` | Exit 0 in 161 s: `qfileinfo-exists` 35 findings in 11 files (the other two checks: 15 and 14) |
| `test-clazy.py ... --inventory qfileinfo-exists` on the fixed tree | Exit 0 in 186 s: **0 findings** in 153 sources |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 6` | **Exit 0** in 170 s: 32 fixtures rejected by their own checks, 153 sources with no finding from 32 checks |
| Same, with the previous `app/wm/schemecolors.cpp` and `app/indicator/factory.cpp` mounted over the fixed files | **Exit 1**: 17 `[qfileinfo-exists]` findings (10 and 7) in those two files, nothing else |
| Container `cmake --build ... -j6` (Clang 18.1.3, Qt 6.11.1, strict warnings) | Exit 0, 181 objects, 0 compiler warnings; the 11 files were compiled after the edit |
| Host `cmake --preset gcc-debug`, `cmake --build --preset gcc-debug -j6`, `ctest --preset gcc-debug` (GCC 16.2.1, fresh tree) | Exit 0 in 448 s, 425 objects, 0 compiler warnings, 57/57 passed |
| Host `cmake --preset clang-debug`, `cmake --build --preset clang-debug -j6`, `ctest --preset clang-debug` (Clang 22.1.8, fresh tree) | Exit 0 in 494 s, 425 objects, 0 compiler warnings, 57/57 passed |
| Probe program above | 14 cases, 0 mismatches |

The build logs hold thousands of `Warning:` lines from the qmllint step; they
are the known QML warnings tracked by the baseline gate, not compiler output,
and this change touches no QML.

## Handoff

- **Files and behavior changed:** the 11 production sources of the table (35
  call sites), `scripts/test-clazy.py` (one name),
  `autotests/clazy/qfileinfo-exists.cpp`. No observable change.
- **State authority / lifetime constraints:** none. The removed `QFileInfo`
  temporaries were only asked `exists()` and shared no cached state; the
  filesystem is the authority, queried once per call as before.
- **Tests:** no test added or changed; `ctest` 57/57 on GCC and Clang Debug.
  The fixture and the mutation run are the negative evidence.
- **Combined validation:** on `main` at `fc8b803d0`, GCC and Clang Debug and
  Release with 0 warnings and 57/57 tests each, the 37-check gate, the Debian 13
  build on Qt 6.8.2 and the desktop smoke on the host and in the Fedora VM
  passed; see [R2c fix batch 1](R2c.md#fix-batch-1).
- **CI:** run 40683601580 at `14d705a5d` passed all 23 jobs, among them
  `Build (Clang Debug)` with the 43-check clazy gate and `Install verify
  (debian13)` on Qt 6.8.2; see [CI validation](R2c.md#ci-validation). The
  earlier run 40508173790 at `eba91a3e6` stopped its Build jobs at the KDE Neon
  key download before compiling, so it says nothing about this slice.
- **Not run:** nothing outstanding.
- **Next exact action:** none.
- **Commit approval status / push approval status:** committed on the feature
  branch; merged into `main` in `1dd485bc1` with the user's approval; pushed
  with `eba91a3e6`.
