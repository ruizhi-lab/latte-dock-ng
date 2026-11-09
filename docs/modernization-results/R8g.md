# R8g: a behavioural case for Package::pathChanged()

Status: Complete on its branch: the case passes, all five scratch mutations fail it, and
the GCC and Clang Debug presets build with 0 warnings and pass 61 of 61 tests. The merge
and the four presets on the merged `main` are recorded in the R track ledger.

Batch / slice and selected scope: R8g, the second follow-up of the
[R8f record](R8f.md): row 107 of the [R8a inventory](R8a-inventory.md)
(`lattePackageShellPluginNameBranchingIsCorrect`, class B1) becomes a behavioural case.
Tests only; no production source changes.

Source baseline and start HEAD: `83a03c2c5` (local `main`, one documentation commit
ahead of `origin/main` `4741deeab`). Branch `claude/r8g-package-fallback-test`. It
reuses the worktree `.claude/worktrees/r8f-duplicate-contracts`: R8f's branch there is
merged, and its GCC and Clang Debug trees saved two full builds. Commit `a28a31f6c` and
the commit that adds this record.

## Why the existing case missed R8f's mutation

`packageunittest` already had `packagePathChangesHandleForeignMetadata`: it pointed a
`Latte::Package` at a directory whose `metadata.json` has the plugin id
`org.example.foreign` and then back at Latte's shell package. It compared only the
plugin ids, never the fallback package that `Package::pathChanged()` sets, so R8f's
mutation M106 (the gate compares with `org.kde.latte.shellX`) passed it.

`Package::pathChanged()` (`app/package/lattepackage.cpp`) gives every shell package
other than `org.kde.latte.shell` the installed Latte shell package as fallback and
clears the fallback for Latte's own package. Production builds the package once, in
`Corona` (`app/lattecorona.cpp`), and `initPackage()` sets the `org.kde.plasma.desktop`
fallback after its own `setPath()`, so the branches only show when a package is
re-pointed, as the test does.

## Change

`pathChangeGivesForeignShellsTheLatteFallback` replaces the old case:

- After pointing the package at the foreign directory, the fallback is valid, its
  plugin id is `org.kde.latte.shell`, and its path resolves (canonically) to this source
  tree's `shell/package`.
- After pointing it back at `shell/package`, the plugin id is `org.kde.latte.shell` and
  the fallback is invalid.
- For the case, `XDG_DATA_DIRS` holds only a temporary directory with
  `plasma/shells/org.kde.latte.shell` linked to the source tree, and `XDG_DATA_HOME` an
  empty one, so an installed Latte (here the distribution package in `/usr/share`, or a
  user-mode install) cannot answer. A small RAII helper restores both variables, so the
  later cases see the original environment.
- The fallback is loaded through libplasma's `Plasma/Shell` package structure. CTest
  sets `QT_PLUGIN_PATH` to the build's `bin` directory only, so the structure is found
  where libplasma's plugins are in Qt's own plugin directory (this host, the CI images)
  but not in the Nix check phase. The case skips with a message where the structure is
  missing, instead of failing.

The source contract `lattePackageShellPluginNameBranchingIsCorrect` is removed: it only
checked that the string `"org.kde.latte.shell"` appears in `lattepackage.cpp`, which
stays true under every mutation below. `sourcecontracttest` goes from 223 to 222
functions, above the 170 that AGENTS.md rule 6 expects. With R8a's classes this leaves
B1 with one row. `git clang-format` formatted the touched lines.

## Mutations

Scratch mutations of `app/package/lattepackage.cpp`, one exact text each
(`/data/tmp/r8g/mutate.py`, not committed). Each mutant was built, run with
`packageunittest`, and restored from `HEAD` before the next. Run by a Haiku sub-agent;
the main session checked the logs.

| ID | Mutation | Shell id literals left | Result | Failing check |
|---|---|---:|---|---|
| G1 | the gate compares with `org.kde.latte.shellX` (R8f's M106) | 3 | FAIL | `!package.fallbackPackage().isValid()` (line 188): Latte's own package gets a fallback |
| G2 | a foreign package gets `org.kde.plasma.desktop` as fallback | 3 | FAIL | `fallback.isValid()` (line 182): not in the redirected data directories |
| G3 | the foreign branch never calls `setFallbackPackage()` | 4 | FAIL | `fallback.isValid()` (line 182) |
| G4 | the else branch no longer clears the fallback | 4 | FAIL | `!package.fallbackPackage().isValid()` (line 188) |
| G5 | the else-if compares with `org.kde.latte.shellX` | 3 | FAIL | `!package.fallbackPackage().isValid()` (line 188) |

Every mutant built (exit 0) and failed only this case (6 passed, 1 failed). After the
restore the case passes again (7 of 7) and the worktree is clean. The "Shell id
literals left" column is the count of `"org.kde.latte.shell"` in the mutated file: the
removed contract would have passed every mutant.

## Evidence

- Host: Gentoo, Qt 6.11.3, KPackage 6.31.0, libplasma 6.7.6. Before the mutations
  `packageunittest` passed 7 of 7, with
  `PASS   : PackageUnitTest::pathChangeGivesForeignShellsTheLatteFallback()` (not
  skipped).
- `gcc-debug` (GCC 16.2.1) and `clang-debug` (Clang 22.1.8), incremental, run by a
  Haiku sub-agent: exit 0, 0 compiler warnings (2 and 4 units recompiled), CTest 61 of
  61 in each.
- `sourcecontracttest`: "Totals: 224 passed" on the branch against 225 on `main`
  (each with `initTestCase` and `cleanupTestCase`).

## Follow-ups

- `packageunittest` logs a KCoreAddons warning for the indicator package structure:
  `app/packageplugins/indicator/latte-packagestructure-indicator.json` states the Id
  `org.kde.latte.indicator`, while the plugin file is `latte_indicator.so`, and KF6
  wants the Id derived from the file name. It predates this slice. Changing either side
  changes the plugin's id, so the consumers of that id have to be traced first.

## Not verified

- The Nix check phase: the case is expected to skip there (see above); not run here.
- Older Plasma releases (the 6.3 floor): the case runs only where `Plasma/Shell` loads;
  checked on this host and by CI on the push.
- The Debian 13 container build: deferred by the user until the refactor is complete.
