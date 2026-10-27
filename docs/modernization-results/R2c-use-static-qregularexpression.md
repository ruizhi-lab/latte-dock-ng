# R2c-use-static-qregularexpression: Constant Patterns Compiled on Every Call

Status: Pending validation. The fix, the fixture and the gate pass locally with
Clang and GCC; the CI run and the combined desktop smoke of R2c batches A and B
are not done.
Date: 2026-10-27. Branch: `claude/r2c-batch-b-c113e5`. Start HEAD: `09b030e61`.
Implementation commit: the commit that adds this report. Part of [R2c](R2c.md),
the second slice of batch B (after `R2c-empty-qstringliteral`, before
`R2c-connect-not-normalized`).

## Defect

Six `QRegularExpression` temporaries were built from constant pattern strings
and compiled again on every call:

| Location (before) | Pattern (C++ literal) | Option | Now |
| --- | --- | --- | --- |
| `app/knscompat.cpp:459` | `^prefer .*\n?` | `MultilineOption` | `preferPattern` in `templatesQmldirForSystem()` |
| `app/knscompat.cpp:460` | `^linktarget .*\n?` | `MultilineOption` | `linktargetPattern`, same function |
| `app/knscompat.cpp:461` | `^optional plugin .*\n?` | `MultilineOption` | `optionalPluginPattern`, same function |
| `app/knscompat.cpp:462` | `^classname .*\n?` | `MultilineOption` | `classnamePattern`, same function |
| `app/knscompat.cpp:642` | `^prefer .*\n?` | `MultilineOption` | `preferPattern` in `ensureKnsCompat()` (legacy controls block) |
| `app/wm/tasktools.cpp:41` | `\\s+` | none | `whitespacePattern` in `serviceFromIdOrDesktopEntry()` |

clazy reports `Don't create temporary QRegularExpression objects. Use a static
QRegularExpression object instead` for each. There is no defect to observe and
neither path is hot. The `knscompat.cpp` code runs once per start
(`ensureKnsCompat()` from `main.cpp`); `serviceFromIdOrDesktopEntry()` runs when
`defaultApplication()` resolves a `preferred://` launcher (mailer, terminal,
system settings), so it is the one that can repeat.

## Fix

Each pattern became a function-local `static const QRegularExpression`, declared
next to its use (after the early returns, or inside the `if` that opens the
file), so a path that did not compile the pattern before still does not. The
names follow `suffixPattern` in `app/tools/commontools.cpp`, which already does
this. The pattern strings and the option are copied unchanged.

Why this is safe:

- A function-local static is initialized exactly once, thread-safely, on first
  use, and `QRegularExpression` is reentrant, so calling `match()`, `split()` or
  `remove()` with a shared const instance is fine. Neither file starts a thread
  or hands the expression to one.
- The objects are destroyed at process exit. `QRegularExpression` needs no event
  loop or application object for that, and the project's strict warning set has no
  exit-time-destructor warning (both compilers built with `-Werror`).
- The match state lives in the `QRegularExpressionMatch` and the string, not in
  the expression, so reusing one instance carries nothing from call to call.

Left unchanged on purpose: `app/wm/tasktools.cpp:400`,
`QRegularExpression regExp(ruleGroup.readEntry(QStringLiteral("Match")))`. Its
pattern is read from the user's service-rule configuration at run time, so it is
not constant, clazy does not report it, and a static would be wrong.

The gate side is one gate entry and one fixture, as in every R2c slice: the name
in `GATED_CHECKS` and `autotests/clazy/use-static-qregularexpression.cpp`, which
holds the smallest violation
(`text.contains(QRegularExpression(QStringLiteral("^a+$")))`). The gate now has
33 entries and 33 fixtures.

## Equivalence

A scratch probe (not in the repository) cuts the real `templatesQmldirForSystem()`
out of the old and the new `knscompat.cpp`, and the `split()` line out of both
`tasktools.cpp` versions, and runs them side by side on Qt 6.11.2:

- The pattern literals and options of the old and the new file are the same
  multiset (5 in `knscompat.cpp`, 1 in `tasktools.cpp`).
- `templatesQmldirForSystem()` on the six Kirigami `qmldir` files of the host
  (`templates`, `controls`, the module root, `dialogs`, `primitives`, `layouts`;
  together they hold the lines of all four prefixes) and on a missing file (the
  fallback text): old result == new result == new result on a second call, so the
  reused statics do not change anything. For `templates/qmldir`, 822 bytes become
  663 characters.
- `split()` on `kontact`, `firefox --new-window %u`, `  konsole   -e   fish  `,
  `a<TAB>b<LF>c`, `single`, three spaces, `x` and the empty string: old == new ==
  new again in all eight cases.

The tree has a test for part of this code: `knscompatunittest` checks that the
generated templates and controls `qmldir` files contain no `prefer ` line. It
covers the `prefer` pattern at both `knscompat.cpp` sites; nothing in the tree
tests the `linktarget`, `optional plugin` and `classname` patterns, which is why
the probe above matters. The test was rebuilt with the change and passes.

## Evidence

Container commands are those of [R2c](R2c.md#reproducing-the-environment), with a
private build directory (`~/r2c-build-batch-b`) and `--jobs 6` because another
session ran at the same time; the host was loaded (load average 13 to 24), so the
times are longer than an idle run.

| Command | Result |
| --- | --- |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 6 --inventory use-static-qregularexpression` before the fix | 6 findings: `app/knscompat.cpp` 459, 460, 461, 462, 642 and `app/wm/tasktools.cpp` 41, as in R2c |
| Same after the fix | Exit 0 in 188 s: 0 findings in 153 sources |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 6` | **Exit 0** in 173 s: 33 fixtures rejected by their own checks, 153 sources with no finding from 33 checks |
| Same, with the previous `knscompat.cpp` and `tasktools.cpp` mounted over the fixed files, no rebuild | **Exit 1** in 141 s: the six findings, `app/knscompat.cpp:459:21`, `:460:21`, `:461:21`, `:462:21`, `:642:28` and `app/wm/tasktools.cpp:41:50` |
| Host `cmake --build --preset gcc-debug -j6`, `ctest --preset gcc-debug` (GCC 16.2.1) | Exit 0, 0 compiler warnings, 57/57 passed; `knscompat.cpp` and `tasktools.cpp` were rebuilt in the application, `knscompatunittest` and `wmunittest` |
| Host `cmake --build --preset clang-debug -j6`, `ctest --preset clang-debug` (Clang 22.1.8) | Exit 0, 0 compiler warnings, 57/57 passed; same four objects rebuilt |
| Equivalence probe above | old == new on every input |

The build logs hold thousands of `Warning:` lines from the qmllint step; they are
the known QML warnings tracked by the baseline gate, not compiler output, and
this change touches no QML.

## Handoff

- **Files and behavior changed:** `app/knscompat.cpp` (five patterns in two
  functions), `app/wm/tasktools.cpp` (one pattern), `scripts/test-clazy.py` (one
  name), `autotests/clazy/use-static-qregularexpression.cpp`. No observable change:
  same patterns, same results; each pattern is compiled once per process instead
  of once per call.
- **State authority / lifetime constraints:** none. The statics hold only
  compiled constant patterns; nothing depends on them being destroyed in a
  particular order.
- **Tests:** no test added or changed; `ctest` 57/57 on Clang and GCC Debug. The
  fixture and the pre-fix gate run are the negative evidence. A possible
  follow-up outside this slice's file list: extend `knscompatunittest` with
  `linktarget`, `optional plugin` and `classname` lines in the templates `qmldir`.
- **Not run:** the CI job, Release presets, the desktop retest of AGENTS.md (the
  main session runs one combined smoke after both batches merge), a launcher
  lookup through `serviceFromIdOrDesktopEntry()` in a running dock.
- **Next exact action:** review, then `R2c-connect-not-normalized` and the
  combined smoke. Do not start another batch implicitly.
- **Commit approval status / push approval status:** committed on the feature
  branch as the session brief allows; not pushed; `main` untouched.
