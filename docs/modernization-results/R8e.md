# R8e: CMake structure without changing the build

Batch / slice: R8e (wave 2, CMake structure). Parts A, B and C, in that order.

Status: Complete. Parts A and B are merged (`60ca9429c`), with the follow-up
`d8d76944b`; Part C is an inventory with no file change. The Release presets, CI and
the runtime check passed after the merge; see "Validation after the merge". The
sections before it describe the branch as it was before the merge.

Source baseline / actual start HEAD: `6bdd43086f14ff680a844d47061143d7bfdc90ec`
(local `main`, documentation on top of the Qt 6.7.0 and KF6 6.10.0 floor raise).
Implementation HEAD: `b0cf2c2ae23e8a1d3b2e012b1b5390e5f6f51943`, plus the record commit.

Branch / existing user edits preserved: `claude/r8e-cmake-structure-67c5b2` in the
worktree `r8e-cmake-structure-67c5b2`. The worktree was clean at start. Nothing was
pushed, merged or changed on `main`. The memory sampler and other worktrees were not
touched.

Commits (in order):

- `18489612e3442f4675ae52260326411d19283e24` build: compile the containment plugin's
  generictools from the app tree (Part A)
- `b0cf2c2ae23e8a1d3b2e012b1b5390e5f6f51943` build: add app sources with
  target_sources() instead of PARENT_SCOPE lists (Part B)
- record commit: this file, `docs/modernization-results/R8e-tools/compat_probe.py`
  (Part C)
- `828874e03` test: untrack the bytecode cache of the comparison tool (a Part B
  side effect of running its unit tests; `R8e-tools/.gitignore` now ignores `__pycache__`)

Dependencies and evidence links: frozen baseline in `/tmp/r8e-baseline/` (read-only):
`gcc-debug-compile_commands.json`, `gcc-debug-latte-dock-ng.dynsyms`,
`gcc-debug-contextmenu-plugin.dynsyms`. Comparison tool and its tests:
`docs/modernization-results/R8e-tools/`.

## Part A: the plugin compiles the app's generictools.cpp

Files changed:

- `CMakeLists.txt`: the comment and the three `configure_file()` copies of
  `generictools.h`, `generictools.cpp` and `contextmenudata.h` are removed. The
  `configure_file()` calls that generate real templates are untouched.
- `containmentactions/contextmenu/CMakeLists.txt`: the plugin lists
  `../../app/settings/generic/generictools.cpp` with a one-sentence note on why the
  directory's `TRANSLATION_DOMAIN` still reaches it, and
  `target_include_directories(... PRIVATE)` adds `app/settings/generic` and `app/data`
  (the plugin's quote includes `"generictools.h"` and `"contextmenudata.h"`).
- `scripts/test-clazy.py`: the docstring's "configure_file() copies" item is removed
  because the copies no longer exist. The filter logic is unchanged.

Behaviour changed: the plugin's compile input is now the app's own file, not a
build-tree copy. The plugin's exported symbols and its runtime behaviour are not
intended to change; the plugin was not loaded in a Plasma session (see "Not verified").

TRANSLATION_DOMAIN check, from `build/modernization/gcc-debug/compile_commands.json`:

- Baseline plugin unit (`containmentactions/contextmenu/generictools.cpp.o`): source
  is the build-tree copy, and `-DTRANSLATION_DOMAIN="plasma_containmentactions_lattecontextmenu"`
  is present.
- Candidate plugin unit (`containmentactions/contextmenu/CMakeFiles/plasma_containmentactions_lattecontextmenu.dir/__/__/app/settings/generic/generictools.cpp.o`):
  source is `app/settings/generic/generictools.cpp`, the same define is present, and
  the two app directories are on the include path.
- Candidate executable unit (`app/CMakeFiles/latte-dock-ng.dir/settings/generic/generictools.cpp.o`):
  no `TRANSLATION_DOMAIN`, as in the baseline.

Consequence the acceptance text did not anticipate: a target-scoped include directory
applies to every source of that target, so the plugin's `layoutmenuitemwidget.cpp`,
`menu.cpp` and its AUTOGEN unit also gain the same two `-I` entries. The task required
`target_include_directories(... PRIVATE ...)`, so this was kept. Those units gain only
the two directories; the headers they include resolve to the same files.

Acceptance A evidence:

- `org.kde.latte.contextmenu.so` exports exactly the baseline symbols: 6 lines, `cmp`
  identical to `gcc-debug-contextmenu-plugin.dynsyms`.
- Full compile database against the baseline (all units): 433 and 433 units. 432 units
  match. One unit key changed (the plugin's `generictools.cpp`, whose object path gained
  `__/__/app/settings/generic`). Its source changed from the build-tree copy to the app
  file. Three plugin units gained the two `-I` entries. No other unit differs.
- The executable's 142 units are identical (see Part B).
- `contextmenuunittest`, `containmentactionmenuunittest`, `toolsunittest` and
  `sourcecontracttest` passed under both presets (see "Tests").

## Part B: target_sources() for the app sources

Design:

- `latte-dock-ng` is created first in `app/CMakeLists.txt`, with the same 11 top-level
  sources as before. The 29 subdirectories that used `PARENT_SCOPE` lists now call
  `target_sources(latte-dock-ng PRIVATE <files relative to that directory>)`. Each file
  keeps its original order (checked per file: 126 entries before and after).
- Generated sources stay generated in `app/`: `qt_add_dbus_adaptor`, six
  `ki18n_wrap_ui` and two `ecm_add_qtwayland_client_protocol` calls append to the local
  variable `latte_generated_SRCS`, which one `target_sources()` call adds after the
  subdirectories. Their outputs go to `CMAKE_CURRENT_BINARY_DIR` of `app/` as before
  (checked in `build/modernization/gcc-debug/app/`: `lattedockadaptor.h/.cpp`,
  `qwayland-kde-primary-output-v1.h/.cpp`, `ui_actionsdialog.h`).
- Unchanged: `qt_add_resources`, the GCC 16 `set_source_files_properties` workaround for
  `mocs_compilation.cpp`, `ENABLE_EXPORTS` and `CXX_VISIBILITY_PRESET`, every
  `target_link_libraries`, `app/declarativeimports/CMakeLists.txt` and `autotests/`.
- Why the early creation does not change flags: a target copies its directory's
  include, define and option properties at `add_executable()` time. Nothing between the
  new position and the former one changes those properties in `app/`. The compile
  database confirms it.
- Documentation: the comment at `app/CMakeLists.txt` records why creation comes first
  (`target_sources()` fails at configure time for a missing target) and why the
  generators stay in `app/` (their headers are found through this directory's binary
  dir). `docs/modernization-results/R6-privateapp-design.md` line 296 names
  `target_sources(latte-dock-ng PRIVATE ...)` instead of the removed variable. No other
  line of that file changed.
- `sourcecontracttest` reads `app/CMakeLists.txt`. Its contracts are unchanged and it
  passed under both presets.

Counts, before and after:

| Item | Before (baseline) | After (A and B) |
|---|---|---|
| `latte-dock-ng` translation units (compare tool, latte-dock-ng filter) | 142 | 142; 0 source, 0 flag and 0 order differences; also identical with `--strict-order` |
| All compile units (`--all-units`) | 433 | 433; only the Part A plugin differences |
| `bin/latte-dock-ng` exported dynamic symbols (`nm -D --defined-only`, sorted) | 5628 | 5628; `cmp` identical |
| `org.kde.latte.contextmenu.so` exported dynamic symbols | 6 | 6; `cmp` identical |
| Executable top-level CMake sources | 11 | 11 |
| Source entries in the 29 subdirectory lists | 126 | 126, same order per file |
| Generator calls kept in `app/` | 9 (1 D-Bus, 6 ki18n, 2 Wayland) | 9 |

Compare tool, with the exact commands:

```bash
python3 -I docs/modernization-results/R8e-tools/compare_compile_commands.py /tmp/r8e-baseline/gcc-debug-compile_commands.json build/modernization/gcc-debug/compile_commands.json --baseline-source-root /data/projects/latte-dock-ng --baseline-build-root /data/projects/latte-dock-ng/build/modernization/gcc-debug --candidate-source-root "$PWD" --candidate-build-root "$PWD/build/modernization/gcc-debug"
python3 -I docs/modernization-results/R8e-tools/test_compare_compile_commands.py -v
```

The tool normalizes both roots (build root first, then source root, on whole path
components), keys units by object path, reports unit, source and flag differences,
ignores entry order and treats flag-order-only differences as notes unless
`--strict-order` is given. It exits 0 when identical, 1 when different, 2 on bad input.
The unit tests are standard-library `unittest`, are not registered in CTest, and cover
normalization, filtering, malformed input, order handling and exit status. A mutation
check (a copy with the source-root replacement removed, outside the worktree) failed 9
of the 13 tests, showing the tests detect a broken normalizer.

## Part C: compat/ inventory (list only, no file changes)

Method. `git ls-files compat` gives 26 headers and the `compat/qml` module (4 files).
For each header the `#include` / `__has_include` chain was read. Users are the
`git grep -l -F` matches for `<Name>` and `"Name"` outside `compat/` and `docs/`. Host
resolution comes from `docs/modernization-results/R8e-tools/compat_probe.py`, which runs
`g++ -M` on each named include with the executable's `-isystem` directories and without
any project `-I`, so `compat/` cannot answer for itself. Host: Gentoo, Plasma 6.7.5
(`/usr/include/Plasma/plasma_version.h`), KF6 module headers under `/usr/include/KF6/`,
Qt under `/usr/include/qt6/`. Results are for this host only, not the Plasma 6.3 floor
and not Debian 13.

"Active" is the branch the `#if` chain selects here. Comment and string-only mentions
are not users: `CMakeLists.txt:162` (a comment about `KIconThemes/KIconLoader`) and
`autotests/sourcecontracttest.cpp` (it reads the text of two compat files).

| compat file | Contents (active branch on this host) | Users outside compat/ | Class | Removal note |
|---|---|---|---|---|
| `KActivities/Consumer` | Forwards to `PlasmaActivities/PlasmaActivities/Consumer` (present). `#else` holds a stub `KActivities::Consumer` (inactive here) | 7: `app/lattecorona.cpp`, `app/layout/centrallayout.cpp`, `app/layouts/synchronizer.cpp`, `app/settings/settingsdialog/layoutsmodel.cpp`, `app/settings/universalsettings.cpp`, `app/view/view.cpp`, `app/wm/abstractwindowinterface.h` | pure forwarder (stub fallback) | needs a Debian 13 check; the stub is the fallback where PlasmaActivities is missing |
| `KActivities/Controller` | Forwards to `PlasmaActivities/PlasmaActivities/Controller` (present) | 3: `app/layouts/synchronizer.cpp`, `app/settings/settingsdialog/tablayoutshandler.cpp`, `app/wm/abstractwindowinterface.cpp` | pure forwarder | needs a Debian 13 check |
| `KActivities/Info` | Forwards to `PlasmaActivities/PlasmaActivities/Info` (present). `#else` holds a stub `KActivities::Info` (inactive here) | 2: `app/settings/settingsdialog/layoutsmodel.cpp`, `layoutsmodel.h` | pure forwarder (stub fallback) | needs a Debian 13 check; the Consumer stub includes this header, so the two stubs must go together |
| `KActivities/ResourceInstance` | Forwards to `PlasmaActivities/PlasmaActivities/ResourceInstance` (present) | 1: `app/wm/tasktools.cpp` | pure forwarder | needs a Debian 13 check |
| `KArchive/KArchiveDirectory` | Forwards to `KF6/KArchive/KArchiveDirectory` (present) | 3: `app/indicator/factory.cpp`, `app/layouts/importer.cpp`, `app/settings/settingsdialog/layoutscontroller.cpp` | pure forwarder | needs a Debian 13 check |
| `KArchive/KArchiveEntry` | Forwards to `KF6/KArchive/KArchiveEntry` (present) | same 3 as above | pure forwarder | needs a Debian 13 check |
| `KArchive/KTar` | Forwards to `KF6/KArchive/KTar` (present) | same 3 as above | pure forwarder | needs a Debian 13 check |
| `KArchive/KZip` | Forwards to `KF6/KArchive/KZip` (present); comment cites NixOS store paths | 2: `app/indicator/factory.cpp`, `autotests/indicatorunittest.cpp` | pure forwarder | needs a Debian 13 check and the NixOS CI check |
| `KCoreAddons/KSignalHandler` | Forwards to `KF6/KCoreAddons/KSignalHandler` (present) | 1: `app/main.cpp` | pure forwarder | needs a Debian 13 check |
| `KDeclarative/ConfigPropertyMap` | Forwards `KConfigPropertyMap` to `KF6/KConfigQml/KConfigPropertyMap` (present). Declares `KDeclarative::ConfigPropertyMap` after `#endif`, so it is compiled on every host | 3: `app/view/containmentinterface.cpp`, `app/view/indicator/indicator.cpp`, `autotests/indicatorunittest.cpp` | replacement implementation | keep; production code constructs the Latte-owned class |
| `KDeclarative/QmlObjectSharedEngine` | Upstream branch active here: `PlasmaQuick/SharedQmlEngine` is present, so the class subclasses it. Replacement (`KLocalizedQmlContext`) is in the `#else` branch, inactive here | 4: `app/lattecorona.cpp`, `app/view/indicator/indicator.cpp`, `app/view/settings/indicatoruimanager.cpp`, `autotests/qmlobjectsharedenginetest.cpp` | replacement implementation | keep: needed where libplasma removed `SharedQmlEngine` (Plasma 6.8 and later, see `app/CMakeLists.txt`) |
| `KGuiAddons/KIconUtils` | Forwards to `KF6/KGuiAddons/KIconUtils` (present) | 1: `declarativeimports/core/iconitem.cpp` | pure forwarder | needs a Debian 13 check |
| `KI18n/KLocalizedString` | Unguarded `#include <KLocalizedString>`, which resolves to `KF6/KI18n/KLocalizedString` (present) | 0 | unused | removal candidate; needs a Debian 13 check |
| `KIconThemes/KIconEffect` | Forwards to `KF6/KIconThemes/KIconEffect` (present) | 1: `declarativeimports/core/iconitem.cpp` | pure forwarder | needs a Debian 13 check |
| `KIconThemes/KIconLoader` | Forwards `<KIconLoader>` to `KF6/KIconThemes/KIconLoader` (present); comment cites NixOS | 3: `containment/plugin/layoutmanager.cpp`, `declarativeimports/core/environment.cpp`, `declarativeimports/core/iconitem.cpp` | pure forwarder | needs a Debian 13 check and the NixOS CI check |
| `KIconThemes/KIconTheme` | Forwards to `KF6/KIconThemes/KIconTheme` (present) | 2: `app/view/containmentinterface.cpp`, `declarativeimports/core/environment.cpp` (plus the contract test reading its text) | pure forwarder | needs a Debian 13 check; the contract on its text must follow any change |
| `KNewStuff3/KNS3/QtQuickDialogWrapper` | All five upstream spellings are absent here. The active branch is the `#else` `KNS3::QtQuickDialogWrapper` QDialog stub | 0 | unused | removal candidate; needs a Debian 13 check |
| `KPackage/Package` | Forwards to `KF6/KPackage/kpackage/package.h` (present) | 8: `app/alternativeshelper.cpp`, `app/infoview.cpp`, `app/lattecorona.cpp`, `app/layouts/manager.cpp`, `app/layouts/storage.cpp`, `app/settings/settingsdialog/layoutscontroller.cpp`, `app/settings/universalsettings.cpp`, `autotests/packageunittest.cpp` | pure forwarder | needs a Debian 13 check |
| `KPackage/PackageLoader` | Forwards to `KF6/KPackage/kpackage/packageloader.h` (present) | 5: `app/lattecorona.cpp`, `app/layouts/storage.cpp`, `app/package/lattepackage.cpp`, `app/packageplugins/indicator/indicatorpackage.cpp`, `autotests/packageunittest.cpp` | pure forwarder | needs a Debian 13 check |
| `KPackage/PackageStructure` | Forwards to `KF6/KPackage/kpackage/packagestructure.h` (present) | 2: `app/package/lattepackage.h`, `app/packageplugins/indicator/indicatorpackage.h` | pure forwarder | needs a Debian 13 check |
| `KQuickAddons/quickviewsharedengine.h` | Active branch: `plasmaquick/quickviewsharedengine.h` (present) | 0 | unused | removal candidate; needs a Debian 13 check |
| `Plasma/Package` | `namespace Plasma { using Package = KPackage::Package; }`, resolving through `KF6/KPackage/KPackage/Package` | 0 | unused | removal candidate; needs a Debian 13 check |
| `Plasma/Plasma` | Forwards to `Plasma/plasma/plasma.h` (present); `Plasma/Theme` also present | 22 (`git grep -l -F '<Plasma/Plasma>' -- ':!compat' ':!docs'`) | pure forwarder | needs a Debian 13 check; the widest fan-out, remove last |
| `Plasma/package.h` | Forwards to `KF6/KPackage/KPackage/Package` (present) | 3: `app/view/settings/canvasconfigview.h`, `primaryconfigview.h`, `widgetexplorerview.h` | pure forwarder | needs a Debian 13 check |
| `Plasma/version.h` | Forwards to `Plasma/plasma/plasma.h` and `Plasma/plasma_version.h` (both present) | 4: `app/package/lattepackage.cpp`, `app/plasma/plasmaapicompat.h`, `autotests/packageunittest.cpp`, `autotests/qmlobjectsharedenginetest.cpp` | pure forwarder | needs a Debian 13 check |
| `QuickAddons/ManagedTextureNode` | Real code: a texture-node class over `QSGSimpleTextureNode`; Qt headers resolve here | 1: `declarativeimports/core/iconitem.cpp` | replacement implementation | keep until `iconitem.cpp` no longer uses it; needs a Debian 13 check |
| `compat/qml/org/kde/latte/compat/taskmanager` (`Backend.qml`, `SmartLauncherItem.qml`, `qmldir`, `compat/qml/CMakeLists.txt`) | QML fallback for `org.kde.plasma.private.taskmanager`, which some distributions omit; installed by `install.sh` | 2 QML imports: `plasmoid/package/contents/ui/main.qml`, `plasmoid/package/contents/ui/task/TaskIcon.qml` | replacement implementation | keep; needs a Debian 13 check of the private module |

Summary: of the 26 headers, 19 are pure forwarders (two of them, `KActivities/Consumer`
and `KActivities/Info`, also carry an inactive stub fallback), 3 are replacement
implementations (`KDeclarative/ConfigPropertyMap`, `KDeclarative/QmlObjectSharedEngine`,
`QuickAddons/ManagedTextureNode`), and 4 are unused (`KI18n/KLocalizedString`,
`KNewStuff3/KNS3/QtQuickDialogWrapper`, `KQuickAddons/quickviewsharedengine.h`,
`Plasma/Package`). The `compat/qml` taskmanager module is a replacement.

Proposed removal order. NOT executed. Every step needs a Debian 13 check, and CI's NixOS
release check must also pass, because the KArchive, KIconLoader and KZip shims record
NixOS store-path layouts.

1. Unused shims (0 users): `KI18n/KLocalizedString`, `KQuickAddons/quickviewsharedengine.h`,
   `Plasma/Package`, `KNewStuff3/KNS3/QtQuickDialogWrapper`. Delete them in one change.
2. One- and two-user forwarders: point each user at the upstream spelling, build on
   Debian 13, then delete the shim. Files: `KCoreAddons/KSignalHandler`,
   `KGuiAddons/KIconUtils`, `KIconThemes/KIconEffect`, `KActivities/ResourceInstance`,
   `KPackage/PackageStructure`, `KArchive/KZip`, `KIconThemes/KIconTheme`.
3. Wider forwarders, one shim per change: `KActivities/Controller`,
   `KArchive/KArchiveDirectory`, `KArchive/KArchiveEntry`, `KArchive/KTar`,
   `KIconThemes/KIconLoader`, `Plasma/package.h`, `Plasma/version.h`, `KPackage/Package`,
   `KPackage/PackageLoader`, and last `Plasma/Plasma` (22 users).
4. Keep until the supported distributions and the Plasma floor are decided:
   `KDeclarative/QmlObjectSharedEngine`, `KDeclarative/ConfigPropertyMap`,
   `QuickAddons/ManagedTextureNode`, the `KActivities/Consumer` and `Info` stubs, and the
   `compat/qml` taskmanager module.

## Commands executed and exit status

Run from the worktree root. Logs are in `/tmp/r8e-scratch/` (scratch, not durable); the
commands reproduce them.

| # | Command | Exit | Result |
|---|---|---|---|
| 1 | `cmake --preset gcc-debug` (after Part A) | 0 | configured; "Strict compiler warnings enabled for 63 first-party compile targets" |
| 2 | `cmake --build --preset gcc-debug --parallel 8` (A) | 0 | 0 `warning:` lines |
| 3 | `ctest --preset gcc-debug --output-on-failure` (A) | 0 | 61 of 61 passed |
| 4 | `nm -D --defined-only` on `org.kde.latte.contextmenu.so`, `awk '{print $2, $3}' \| sort`, `cmp` with baseline | 0 | identical (6 lines) |
| 5 | `cmake --preset clang-debug` (A) | 0 | configured |
| 6 | `cmake --build --preset clang-debug --parallel 8` (A) | 0 | 0 `warning:` lines |
| 7 | `ctest --preset clang-debug --output-on-failure` (A) | 0 | 61 of 61 passed |
| 8 | `compare_compile_commands.py` (both databases) with `--all-units` (A) | 1 | expected: plugin `generictools.cpp` key changed, three plugin units gained two `-I` |
| 9 | `compare_compile_commands.py` (latte-dock-ng filter) (A) | 0 | identical, 142 units |
| 10 | `python3 -I -c "ast.parse(...)"` on `scripts/test-clazy.py` | 0 | parses |
| 11 | `cmake --preset gcc-debug` (after Part B) | 0 | configured |
| 12 | `cmake --build --preset gcc-debug --parallel 8` (B) | 0 | 0 `warning:` lines |
| 13 | `ctest --preset gcc-debug --output-on-failure` (B) | 0 | 61 of 61 passed |
| 14 | `nm -D` comparisons of `bin/latte-dock-ng` (5628) and the plugin (6) | 0 | both identical |
| 15 | `compare_compile_commands.py` (latte-dock-ng filter), default and `--strict-order` (B) | 0, 0 | identical, 142 units |
| 16 | `compare_compile_commands.py --all-units --strict-order` (B) | 1 | only the Part A plugin differences |
| 17 | `cmake --preset clang-debug` (after Part B) | 0 | configured |
| 18 | `cmake --build --preset clang-debug --parallel 8` (B) | 0 | 0 `warning:` lines |
| 19 | `ctest --preset clang-debug --output-on-failure` (B) | 0 | 61 of 61 passed |
| 20 | `python3 -I docs/modernization-results/R8e-tools/test_compare_compile_commands.py -v` | 0 | 13 tests, OK |
| 21 | Mutation: the same tests against a copy with the source-root replacement removed (in `/tmp`) | 1 | 9 failures, as expected |
| 22 | `python3 -I docs/modernization-results/R8e-tools/compat_probe.py . build/modernization/gcc-debug/compile_commands.json` | 0 | output identical to the first run |
| 23 | `git grep` for users and `git ls-files` for the compat list (read-only) | 0 | lists in Part C |

Earlier in the session one unit-test invocation (`python3 -I -m unittest`) failed to
import the module because `-I` removes the current directory from `sys.path`. The test
file's docstring now gives the working command. That failure was in the invocation, not
in the tool, and was fixed before run 20. One unit assertion expected `-c` to be
excluded; it is a compile-mode flag and is kept on both sides. The assertion was
corrected and the run above passed.

## Warnings and errors

- GCC Debug (Parts A and B): 0 `warning:` lines in each build log. Configure reports
  strict warnings for 63 first-party compile targets.
- Clang Debug (Parts A and B): 0 `warning:` lines in each build log.
- The `error:` pattern matches four lines per build log. All four are QML source text
  (`console.warn("ContextMenu component load error:", ...)`) echoed in the log, not
  compiler diagnostics. Both builds exited 0.

## Tests

- `ctest --preset gcc-debug`: 61 registered, 61 passed, after Part A and after Part B.
- `ctest --preset clang-debug`: 61 registered, 61 passed, after Part A and after Part B.
- Named targets passed under both presets: `contextmenuunittest`,
  `containmentactionmenuunittest`, `toolsunittest`, `sourcecontracttest`. The
  source-contract checks on `app/CMakeLists.txt` and the plugin passed.
- `logoutsimulationtest` ran `autotests/test_logout_simulation.py` (under
  `python3.11 -I`) and passed in 0.17 s. As its docstring states, it checks the parsers
  and safety guards of `scripts/test-logout-simulation.py` and needs no session;
  the nested-session run itself was not performed.
- `clazyrunnertest` and `qmllintbaselinetest` passed. These are the runner and baseline
  tests, not the clazy 49-check gate or the deep qmllint gate.
- Comparison tool unit tests: 13 of 13 passed (not registered in CTest).

## Follow-up: stale copies in reused build trees (found 2026-11-09)

Part A removed the three `configure_file()` copies from the root `CMakeLists.txt`, but
nothing deleted the copies that earlier configures had written to
`<build>/containmentactions/contextmenu/`. `CMAKE_INCLUDE_CURRENT_DIR`, which
KDECMakeSettings turns on, puts that binary directory first on the plugin's include
path (the generated `flags.make` lists it before `app/settings/generic` and `app/data`),
so a copy shadows the real `contextmenudata.h` and `generictools.h`.

- Found when a sub-agent rebuilt this checkout's `build/modernization/gcc-debug`, last
  configured on 2026-10-25: `menu.cpp` failed with "no match for call to
  '(const QString) ()'" at `menu.cpp:115`, because the copy from that date still declares
  `static const QString ADDVIEWACTION` instead of the accessor that 50ba3074b
  introduced. The default `install.sh --user` tree (`build-user-<user>`, here last
  configured on 2026-08-05) and the other three `build/modernization/*` trees hold
  copies of the same age. Fresh trees, CI and package builds are not affected. Trees
  configured between 50ba3074b and Part A compile, but the plugin's `menu.cpp` reads the
  copy, so the next change of either header would compile it against old declarations.
- Fix: the plugin's `CMakeLists.txt` removes the three copies with `file(REMOVE)` at
  configure time, with the reason and the condition for dropping it beside the rule.
- Red/green on one fresh GCC Debug tree of the fix branch: without the fix, copying the
  three files from `build-user-ruizhi` into the plugin's binary directory and
  recompiling `menu.cpp` fails (exit 2, 82 ` error: ` lines, the same message); with the
  fix applied, the build re-runs CMake, the three copies are gone, `menu.cpp` recompiles
  and the plugin builds (exit 0, 0 warnings). The full GCC Debug build of the branch has
  0 warnings and passes 61 of 61 tests. A first attempt planted the copies after
  `menu.cpp` was compiled, so nothing recompiled and it proved nothing; that run is not
  counted.

## Validation after the merge (2026-11-09)

- Presets: the merged `main` built in the GCC and Clang Debug and Release presets with
  0 compiler warnings and passed 61 of 61 tests in each, after the merge and again after
  the follow-up (see the R track ledger).
- CI: Build run 42545506041 on `e92a534a2`, which contains Parts A and B and the
  follow-up, passed all 23 jobs. Among them are the Clang Debug job's targeted
  clang-tidy and gated clazy checks, the deep QML lint in the four presets, the GCC
  Debug job's targeted ASan/UBSan tests, the Debian 13 install and package checks, and
  the NixOS release check.
- Runtime: in the R8b desktop check ([R8b](R8b.md)) the user right-clicked a dock that
  had loaded `org.kde.latte.contextmenu.so` from a user-mode install
  (`/proc/<pid>/maps`). The plugin was built in a fresh tree from a branch that contains
  Parts A and B; the installed file has the same build ID as the tree's. The context
  menu opened and its 编辑停靠栏 (&E)... entry switched edit mode. The dock quit cleanly
  and `coredumpctl list latte-dock-ng` stayed at 106 entries.

## Not verified

Written before the merge; the section above settles the Release presets, clazy, the
deep qmllint gate, the other CI jobs and the runtime check. Still not done:

- The `gcc-asan-ubsan` preset was not built here; CI runs only its targeted subset, in
  the GCC Debug job.
- Debian 13 container build here: deferred by the user until the refactor is complete.
  CI's Debian 13 install and package checks passed.
- Which of the two production compile commands of `generictools.cpp` (executable or
  plugin) the clazy gate analysed was not checked. `scripts/test-clazy.py` takes the
  first in database order; the two differ only by `TRANSLATION_DOMAIN` and the two `-I`
  entries.
- `scripts/test-logout-simulation.py` (nested session): not run.
- The compat results are for this host (Plasma 6.7.5). They say nothing about the
  Plasma 6.3 floor or about Debian 13.
- No help documentation changed (AGENTS.md rule 9 is not triggered).

## Next exact action

None. Commit approval status: merged into `main` and pushed under the standing approval
for verified work.
