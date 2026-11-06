# R6: executable-hosted `org.kde.latte.private.app`

Design review and feasibility prototype for ledger row R6, candidate 1. No tracked
source changed. This record is the only committed output.

**Base:** `e18a0f9a7` (main, v1.3.0). **Branch:** `claude/r6-privateapp-design-e58ec7`.
**Host:** Gentoo, GCC 16.2.1, Clang 22.1.8, Qt 6.11.2, Plasma 6.7.5.

## Summary

**Recommendation: conditional go, as one coordinated slice.** The plugin, its install
rules, the `check-modules` validator, the presets and the registration order must
change together.

- **Works on the real tree:** `qt_add_qml_module(latte-dock-ng …)` generates a qmldir with no
  `plugin` line, and a qmltypes file byte-identical to the installed
  `latteprivateappplugin.qmltypes` on this host. The executable builds with zero compiler
  warnings and passes the host smoke, provided the fixes below are in.
- **Four changes are mandatory** (three were found by a failing build or smoke run, one by the checker):
  1. Use the keyword form for every `target_link_libraries(latte-dock-ng …)`; `qt_add_qml_module`
     makes a keyword call on the target (`Qt6QmlMacros.cmake:693`). Plain calls at
     `app/CMakeLists.txt:100` and `:136` abort configure.
  2. Add `declarativeimports/` and `plasma/extended/` to the executable's include path; the
     generated registration source includes headers by basename.
  3. Call the generated `qml_register_types_org_kde_latte_private_app()` before the runtime
     `Settings` registration. `Corona` registers `Settings` into this URI at
     `app/lattecorona.cpp:1690`; without the explicit call the smoke fails with
     `LatteApp.ContextMenuLayer is not a type` (real binary, rc=1; rc=0 with the call).
  4. `scripts/qmllint-baseline.py:495-520` requires a plugin library for `private/app`.
     It rejects the executable-hosted module today.
- **Open risk: the Qt floor.** All results are on Qt 6.11.2. Qt 6.6 (`CMakeLists.txt:15`) was
  not testable here, and Debian 13 (Qt 6.8.2, `docs/modernization-results/debian13.md:8`) is
  deferred by instruction. This gate must pass before merge.
- **Benefits (structural, not measured):** one compile of `interfaces.cpp` instead of two,
  no `-Wl,--no-undefined` stripping, no dlopen-time symbol resolution from the executable, and
  a host smoke that depends only on `latte-dock-ng`.
- **Not verified:** desktop behaviour, Clang, ASan/UBSan, `ctest`, full `qmllint-deep`,
  `cmake --install`, Qt 6.6/6.8, Plasma 6.3. See section 6.

## 1. Inventory of the current shape

### 1.1 Build and install

| Item | Where | Note |
| --- | --- | --- |
| Plugin target | `app/declarativeimports/CMakeLists.txt:18-26` | `qt_add_qml_module(latteprivateappplugin …)`, URI `org.kde.latte.private.app` 0.1, `PLUGIN_TARGET`, `NO_PLUGIN_OPTIONAL` |
| Plugin output | same file, `:10`, `:24` | `${CMAKE_BINARY_DIR}/qml/org/kde/latte/private/app` |
| Linker workaround | same file, `:12-16` | strips `-Wl,--no-undefined` from `CMAKE_MODULE_LINKER_FLAGS` |
| Visibility | `app/declarativeimports/CMakeLists.txt:30-33`; `app/CMakeLists.txt:84-89` | plugin: `CXX_VISIBILITY_PRESET default`; executable: `ENABLE_EXPORTS TRUE` and default visibility |
| Plugin sources | `app/declarativeimports/CMakeLists.txt:2-7` | `contextmenulayerquickitem.{h,cpp}`, `interfaces.{h,cpp}`, `../plasma/extended/backgroundtracker.{h,cpp}` |
| Executable sources | `app/CMakeLists.txt:7` | `declarativeimports/interfaces.cpp`, a second compile of the same file |
| Plugin link set | `app/declarativeimports/CMakeLists.txt:35-50` | Qt Quick, Qml, KF6 and Plasma libraries; the plugin does not link the executable |
| Install rules | `app/declarativeimports/CMakeLists.txt:58-73`; `CMakeLists.txt:72-74` | plugin `.so`, `qmldir` and `latteprivateappplugin.qmltypes` into `${KDE_INSTALL_QMLDIR}` and, when different, `${LATTE_QT6_COMPAT_QMLDIR}` (`lib/qt6/qml`) |
| Plugin inclusion | `app/CMakeLists.txt:16` | `add_subdirectory(declarativeimports)` runs before `add_executable` (`:69`) |
| Executable install | `app/CMakeLists.txt:150` | `install(TARGETS latte-dock-ng …)`; the executable has no QML installs of its own |
| Plugin-only file | `app/plasma/extended/CMakeLists.txt:1-8` | the executable list does not include `backgroundtracker.cpp` |

### 1.2 Tooling, tests and presets

| Consumer | Where | Dependency |
| --- | --- | --- |
| `qmllint-deep.sh` | `scripts/qmllint-deep.sh:23-25`, `:68`, `:78` | needs `build/qml/org/kde/latte/core` and `--build-qml build/qml`; runs `check-modules` after the lint |
| `check-modules` | `scripts/qmllint-baseline.py:495-520` (module tuple at `:497`) | requires exactly one `plugin` line and a `lib<plugin>.so` for `private/app` |
| Checker unit test | `autotests/test_qmllint_baseline.py:290-311` | writes a plugin and `.so` for all four modules |
| Host smoke | `autotests/CMakeLists.txt:1088-1098` | `LATTE_PRIVATE_APP_HOST_SMOKE=1`, offscreen, runs `$<TARGET_FILE:latte-dock-ng>` |
| ASan exception | `autotests/CMakeLists.txt:1099-1106`; `docs/development-testing-guide.md:556-563` | disables only `detect_odr_violation` for the duplicated `Interfaces` meta-object |
| Direct compiles | `autotests/CMakeLists.txt:136` (`backgroundtracker.cpp`), `:591` (`interfaces.cpp`) | compile the sources into test binaries; unaffected by an owner change |
| CMake presets | `CMakePresets.json:110` (build targets of `gcc-asan-ubsan`), `:164` (test filter) | `:110` names `latteprivateappplugin`; `:164` names `privateapphostsmoketest` |
| Uninstall | `uninstall.sh:538` | removes `${qml_dir}/org/kde/latte/private/app` |
| User install paths | `install.sh:170-189`, `:244-245` | derives `KDE_INSTALL_QMLDIR` |
| CI | `.github/workflows/build.yml:226-227` | runs `qmllint-deep.sh` on each preset build directory |

### 1.3 Runtime registration and import paths

- `Corona::qmlRegisterTypes()` is at `app/lattecorona.cpp:1688-1704`. The call is at `:400`,
  inside the constructor that starts at `:356`.
  - Into the private URI: `Latte::Settings` as an uncreatable metaobject, version 0.1
    (`:1690-1694`; constant `app/apptypes.h:71`).
  - Into `"latte-dock"` version 1, which is **not** the private URI: eight anonymous types
    (`:1696-1703`; constant `app/apptypes.h:70`). Three more are registered the same way: two in
    `app/plasma/extended/theme.cpp:551-552` and one in `app/view/settings/indicatoruimanager.cpp:35`.
  - The task brief said the anonymous types share the private URI. They do not.
- Smoke path: `app/main.cpp:130-151`. The env switch is at `:130`, the import path
  `<appdir>/../qml` at `:133`, and the QML import at `:137`.
- `addLatteQmlImportPaths()` (`app/main.cpp:775-793`) adds only user-local and KNS roots. Its
  callers are `app/main.cpp:645`, `app/infoview.cpp:81`, `app/view/view.cpp:283` and
  `app/view/settings/subconfigview.cpp:133`. It does not add the private module.
- The QML_* macros in `app/` are three: `interfaces.h:24`, `contextmenulayerquickitem.h:38`,
  `backgroundtracker.h:21`. Only `Interfaces` is compiled into the executable today.

### 1.4 QML files that import `org.kde.latte.private.app`

| File | Use |
| --- | --- |
| `containment/package/contents/ui/abilities/Animations.qml:8` | import only |
| `containment/package/contents/ui/colorizer/Manager.qml:9` | `LatteApp.BackgroundTracker` (`:163`) |
| `containment/package/contents/ui/layouts/LayoutsContainer.qml:11` | `LatteApp.ContextMenuLayer` (`:299`) |
| `containment/package/contents/ui/main.qml:18` | `LatteApp.Interfaces` (`:1754`); a comment at `:1497` |
| `shell/package/contents/configuration/CanvasConfiguration.qml:10` | `LatteApp.ContextMenuLayer` (`:96`) |
| `plasmoid/package` | none; the plasmoid imports only `org.kde.latte.private.tasks` |

All five use the alias `LatteApp`. No QML in the tree reads `LatteApp.Settings`. No QML
imports `latte-dock` (the anonymous-type URI).

### 1.5 Packaging and documentation

- RPM: `latte-dock-ng.spec:96-111` builds `%files` from the CMake install manifest, so a removed
  `.so` drops out automatically. There is no hard-coded `private/app` path.
- Nix, Docker and scripts: `default.nix`, `flake.nix:28-111`, `docker/*`,
  `scripts/prepare-gentoo-overlay.sh` and `scripts/gentoo_manifest.py` mention QML paths in
  general terms only (`git grep`).
- Arch and Debian: `git ls-files` shows no `PKGBUILD` or `debian/` directory. The Gentoo
  ebuild lives in the external overlay named in `AGENTS.md`, which this review did not check.
- Documents that describe the plugin and would need updating in the implementation:
  `docs/architecture-overview.md:37-40,56`; `docs/development-testing-guide.md:132-135,556-563`;
  `docs/qmllint-backlog-plan.md:27,48-52`; `docs/architecture-modernization-plan.md:48,117`;
  `docs/architecture-modernization-implementation.md:235,452`;
  `docs/modernization-results/P0.md:544-552`.
- `docs/modernization-results/debian13.md:16-18` says the plugin is "excluded from the ordinary
  host build". The current CMake has no `EXCLUDE_FROM_ALL` on that target
  (`app/declarativeimports/CMakeLists.txt:18-26`), so I read the note as "not a dependency of
  `latte-dock-ng`, so built only when requested". Not reconciled here.
- `docs/qmllint-baseline.json` has no string containing `private.app` (checked by walking the
  JSON, read only).

### 1.6 Host state (read only)

This host has an installed Latte: `/usr/lib64/qt6/qml/org/kde/latte/private/app/` holds
`qmldir`, `liblatteprivateappplugin.so` and `latteprivateappplugin.qmltypes`, and
`/usr/bin/latte-dock-ng` exists. I did not run the installed binary or change those files.
The prototype runs were isolated from them (section 5.4).

## 2. Qt facts verified on this host

`qmake6 -query QT_VERSION` returns `6.11.2`. Sources read:
`/usr/lib64/cmake/Qt6Qml/Qt6QmlMacros.cmake`, `/usr/lib64/cmake/Qt6Core/Qt6CoreMacros.cmake`,
`/usr/include/qt6/QtQml/qqmlmoduleregistration.h`, `/usr/include/qt6/QtQml/qqml.h`. Tools:
`/usr/lib64/qt6/libexec/qmltyperegistrar`, `/usr/lib64/qt6/bin/qmllint`.

| Behaviour | Evidence (Qt 6.11.2) | Minimum Qt |
| --- | --- | --- |
| An executable target is a valid backing target; no plugin is created | `Qt6QmlMacros.cmake:505-521`; plugin step guarded at `:983` | not verified |
| `PLUGIN_TARGET` with an executable is a fatal error | `Qt6QmlMacros.cmake:505-515` | not verified |
| Executable output directory gets the target path appended; an explicit `OUTPUT_DIRECTORY` is used as given | `Qt6QmlMacros.cmake:615-628` | not verified |
| qmldir has a `plugin` line only when a plugin target exists | `Qt6QmlMacros.cmake:2021-2022` | not verified |
| Resource prefix is `:/qt/qml/<uri path>/` under policy QTP0001 NEW | `Qt6QmlMacros.cmake:816-824`; repo sets it at `CMakeLists.txt:89` | QTP0001 introduced 6.5.0 (`:816`) |
| Registration: qmltyperegistrar writes `<target>.qmltypes` by default (`:4200`) and `<target>_qmltyperegistrations.cpp` (`:4282`), added to the target's sources (`:4374`) | `Qt6QmlMacros.cmake:4172-4417` | not verified |
| Generated code: `qml_register_types_<uri>()` and a static `QQmlModuleRegistration` | observed output (T3, section 5.4); class at `qqmlmoduleregistration.h:13-22` | not verified |
| Generated code includes types by header basename (`__has_include`) | observed compile errors without header directories (T6) | not verified |
| A keyword `target_link_libraries(<target> PRIVATE …)` call on the target | `Qt6QmlMacros.cmake:693`; mixing with plain calls fails configure (T1) | not verified |
| Policies that default to NEW only when introduced at or before the check version; otherwise a warning | `Qt6CoreMacros.cmake:3559-3576` | — |
| QTP0004 and QTP0005 are 6.8.0 | `Qt6QmlMacros.cmake:706`, `:4040`; not used by this design | 6.8.0 |
| `qmlRegisterUncreatableMetaObject` and `qmlRegisterAnonymousType` signatures | `/usr/include/qt6/QtQml/qqml.h:60`, `:94`, `:301` | — |
| `QML_NAMED_ELEMENT` in a namespace | compile error (S8) | not viable |

**Floor verdict:** not verified for Qt 6.6 or 6.7. The repo's floors are `QT_MIN_VERSION 6.6.0`,
`KF6 6.0.0` and `PLASMA 6.3.0` (`CMakeLists.txt:15-17`). Debian 13 records Qt 6.8.2 and
Plasma 6.3.5 (`docs/modernization-results/debian13.md:8`). `QQmlModuleRegistration` exists in
6.11.2, and its introduction version is unknown here. The header marks the old majorVersion
overload `QT_DEPRECATED_X` (`qqmlmoduleregistration.h:21-22`), which suggests the class predates
6.6. That is an inference, not a verified fact.

## 3. Design

### 3.1 Sources and targets

- Move `declarativeimports/contextmenulayerquickitem.{h,cpp}` and
  `plasma/extended/backgroundtracker.{h,cpp}` into the executable list in `app/CMakeLists.txt`.
  Both are plugin-only today.
- Keep `declarativeimports/interfaces.{h,cpp}` in the executable list (`app/CMakeLists.txt:7`).
- Delete `app/declarativeimports/CMakeLists.txt` and its `add_subdirectory` (`app/CMakeLists.txt:16`).
- Call `qt_add_qml_module(latte-dock-ng …)` after `add_executable` (`app/CMakeLists.txt:69`).
- Add `target_include_directories(latte-dock-ng PRIVATE declarativeimports plasma/extended)`,
  following `containment/CMakeLists.txt:44-47`.
- Use the keyword form on the executable's link calls (`app/CMakeLists.txt:100` and `:136`).
- Name the typeinfo `latteprivateapp.qmltypes` (the prototype's choice). This is a rename
  from `latteprivateappplugin.qmltypes`; the old name appears only in `app/declarativeimports/CMakeLists.txt`
  (`git grep`); tools read the typeinfo name from the qmldir.

### 3.2 The duplicate `interfaces.cpp`

Today the file is compiled twice (`app/CMakeLists.txt:7` and
`app/declarativeimports/CMakeLists.txt:4`). The plugin's `Latte::Interfaces` metaobject and the
host's copy are two definitions in two DSOs, and the plugin resolves the host's through the
executable's exported symbols. The ASan exception describes exactly this
(`autotests/CMakeLists.txt:1099-1106`). With one compile the exception should be removable. I
did not run ASan.

### 3.3 Install roots and qmldir

- Keep metadata (qmldir and typeinfo) in both roots, as the containment and core modules do, so the
  rationale in `CMakeLists.txt:68-71` still holds. Drop the `.so` installs.
- The embedded qmldir is what the engine uses at runtime (import trace of S1:
  `loaded ":/qt/qml/org/kde/latte/private/app/qmldir"`). The on-disk copy serves tools and the
  build-tree checks.
- The new qmldir differs from the installed one by three lines: `linktarget`, `plugin` and
  `classname` are gone. The typeinfo name changes if the rename is taken.
- `uninstall.sh:538` needs no change.

### 3.4 qmllint and `check-modules`

- Output stays at `build/qml/org/kde/latte/private/app/`, so `qmllint-deep.sh` needs no path change.
- qmllint resolves the module from the on-disk typeinfo: `LatteApp.Interfaces` and
  `LatteApp.ContextMenuLayer` resolved (S9). `LatteApp.Settings` stays unresolved because it
  is registered at runtime only. That matches the existing category in
  `docs/qmllint-backlog-plan.md:27,48-52`.
- `check-modules` must change. For `private/app` it should expect zero `plugin` lines and one
  `typeinfo` file. The prototype failure was `expected one plugin declaration in
  …/private/app/qmldir` (T5). Update `autotests/test_qmllint_baseline.py:290-311` to match.
- Adding `app` to the fatal import list in `_validate_import_report`
  (`scripts/qmllint-baseline.py:533`) would change lint policy. Decide it separately, with a
  baseline review.

### 3.5 Lazily registered types and ordering

This is the finding that matters most. Observed in both prototypes:

- A runtime `qmlRegisterUncreatableMetaObject(…, "org.kde.latte.private.app", 0, 1, "Settings")`
  before the first engine import makes the static types fail with `… is not a type` (S2, and
  the real binary, V1).
- Calling the generated `qml_register_types_org_kde_latte_private_app()` first makes them
  resolve (S3; real binary V2).
- Registering `Settings` after the first import also works (S4).
- An anonymous type registered under `"latte-dock"` does not interfere (S5).

The production order is the one in V1: the `Corona` constructor registers `Settings`
(`app/lattecorona.cpp:400`, `:1690`) before any QML imports the module. The fix is therefore
mandatory.

The mechanism was not read from source. The installed headers do not include the
implementation of `QQmlModuleRegistration`. The observed rule is that the static callback does
not take effect when another registration for the same URI comes first.

Options:

- **(a) Call the generated function first.** Verified in the standalone and the real binary.
  Recommended. Add a comment beside the call, and a permanent smoke fixture that runs the same
  order.
- **(b) Move `Settings` into the static registration.** Not possible as written: a
  `QML_NAMED_ELEMENT` cannot sit in a namespace (S8). A class wrapper would change the C++ shape.
- **(c) Remove the runtime `Settings` registration.** No QML in the tree reads it, but it is
  part of the URI surface, so removing it needs a decision.
- **(d) Register after the first import.** Fragile; rejected.

### 3.6 Anonymous types

They register into `"latte-dock"` version 1 (`app/apptypes.h:70`), which is not this module.
The design leaves them alone. S5 shows their registration does not disturb the static types.

### 3.7 Host smoke test and import paths

- The smoke path (`app/main.cpp:130-151`) keeps working. The executable is at
  `build/bin/latte-dock-ng` (observed), so `../qml` (`:133`) resolves to `build/qml`.
- The smoke must run the generated registration first, the same order as `Corona`. Otherwise it
  cannot catch the ordering regression. Use the V1 order as a permanent negative fixture and
  the V2 order as the positive one.
- `addLatteQmlImportPaths` (`app/main.cpp:775`) needs no change.

### 3.8 Presets and comments

- `CMakePresets.json:110` drops `latteprivateappplugin` from the `gcc-asan-ubsan` build targets.
- `autotests/CMakeLists.txt:1088-1089` comments and the ASan exception at `:1099-1106` need
  rewording, or removal after an ASan run.
- AGENTS rule 7 applies: the ordering constraint and the keyword form need comments beside the
  code (`app/lattecorona.cpp` and `app/CMakeLists.txt`).

## 4. Risks and compatibility surfaces

| ID | Risk or surface | Evidence | Mitigation |
| --- | --- | --- | --- |
| 1 | Ordering: runtime `Settings` before the generated function removes the three QML types | S2, S3; V1 rc=1, V2 rc=0 | Explicit call; permanent smoke fixture |
| 2 | Keyword/plain conflict on the executable's link calls | T1 configure error | `PRIVATE` on all `latte-dock-ng` link calls |
| 3 | Generated registration needs header directories | T6 compile errors; containment precedent | Include directories |
| 4 | Stale plugin layout on disk. A qmldir with a `plugin` line makes the engine try to load the sibling `.so` | S6: a missing `.so` is tolerated. A stale `.so` would be loaded into the new host, not tested | Installers and uninstall remove `liblatteprivateappplugin.so` and the old qmltypes name in both roots |
| 5 | Qt floor: executable branch and `QQmlModuleRegistration` on Qt 6.6/6.7/6.8 | not verified | Build and run the smoke on Qt 6.6 and Debian 13 (Qt 6.8.2) before merge |
| 6 | Checker and its test reject a plugin-less module | T5 | Update checker and test |
| 7 | User-local overrides: `LATTE_FORCE_USER_LOCAL_QML_IMPORTS` (`app/main.cpp:725-727`) and `~/.local` roots | S6 reasoning | Document; stale `~/.local` copies fall under risk 4 |
| 8 | Third-party QML importing the module | URI and versions unchanged; typeinfo byte-identical (T4). No known third-party load of the `.so` | None for the URI; the `.so` disappears |
| 9 | Executable grows by two sources; the Debug qmllint `PRE_BUILD` (`app/FakeTarget.cmake`) is pre-existing and prints many QML warnings | T8 | None; not caused by this design |
| 10 | Desktop behaviour of `ContextMenuLayer`, `BackgroundTracker` and `Interfaces` | not tested | Desktop retest per the AGENTS.md workflow (user) |
| 11 | ASan/UBSan: removing the ODR exception | not tested | Run `gcc-asan-ubsan` without `detect_odr_violation=0` |
| 12 | `ENABLE_EXPORTS` and default visibility on the executable | still needed by other plugins? not checked | Keep until checked separately |

## 5. Recommendation and prototype plan

### 5.1 Go or no-go

**Conditional go.** The mechanism works on the real tree. Implement it as one slice:
CMake, registration order, checker and test, presets, and docs. Do not merge before the Qt 6.6
and Debian 13 gate.

### 5.2 Edits tested in the scratch copy

Applied to `/tmp/privateapp-proto` (not committed; patch in the shared notes folder):

1. `app/CMakeLists.txt`: add the two sources with `target_sources(latte-dock-ng PRIVATE ...)`. Remove
   `add_subdirectory(declarativeimports)`. After `add_executable`, add the `qt_add_qml_module`
   call with `OUTPUT_DIRECTORY` and `TYPEINFO latteprivateapp.qmltypes`, and the
   `target_include_directories` block. Change the two link calls to `PRIVATE`. Install only
   `qmldir` and the typeinfo.
2. `app/declarativeimports/CMakeLists.txt`: delete (the scratch copy keeps a comment-only file).
3. `app/lattecorona.cpp`: declare `void qml_register_types_org_kde_latte_private_app();` at
   global scope, and call `::qml_register_types_org_kde_latte_private_app()` first in
   `Corona::qmlRegisterTypes()`.
4. `app/main.cpp`: smoke-only switches for the two orders. Prototype only; the implementation
   should turn them into a permanent test.

Not applied in the prototype, so they belong to the slice: `scripts/qmllint-baseline.py`,
`autotests/test_qmllint_baseline.py`, `CMakePresets.json`, the docs in section 1.5, the ASan
exception, and the compat-root installs.

### 5.3 Checks that must pass before merge

1. Build `latte-dock-ng` with GCC and Clang and `LATTE_STRICT_WARNINGS=ON`, with zero warnings
   (AGENTS rule 4). GCC: done, zero (T8). Clang: not run.
2. `ctest --output-on-failure` on the full build (AGENTS rule 6). That includes
   `privateapphostsmoketest`, which then needs only `latte-dock-ng`, and the updated
   `qmllintbaselinetest`. Not run.
3. `scripts/qmllint-deep.sh` on a full build, with four generated modules and a baseline
   comparison with no silent additions or removals. Not run.
4. The ordering fixture: the V1 order fails and the V2 order passes (both observed; the
   permanent fixture is not written).
5. ASan/UBSan preset passes without `detect_odr_violation=0`, or the exception is kept with a
   stated reason. Not run.
6. `cmake --install` into a temporary prefix: `private/app` holds `qmldir` and typeinfo in both
   roots and no `.so`; `uninstall.sh` removes the directory. Not run.
7. Desktop retest by the user: user-mode install, `ContextMenuLayer` menus, colorizer background
   tracking, the `Interfaces` link, and clean quit (`docs/development-testing-guide.md`, Runtime
   Retest Workflow). Not run.
8. Qt 6.6 and Debian 13 (Qt 6.8.2) builds of the module. Deferred by instruction.

### 5.4 Prototype results

Standalone (`/tmp/r6-qmlexe-proto`; Qt 6.11.2, GCC 16.2.1, `-Wall -Wextra`, Debug):

| ID | Experiment | Result |
| --- | --- | --- |
| S1 | Executable with `qt_add_qml_module` and two `QML_NAMED_ELEMENT` types (a QObject and a QQuickItem); no runtime registration in this run | Zero warnings. qmldir has no `plugin` line. Both types OK. The `Settings` probe fails with `Cannot read property 'On'`, as expected without the runtime registration |
| S2 | Runtime `Settings` before the engine | Enum probe OK; `Interfaces` and `ContextMenuLayer`: `is not a type` |
| S3 | As S2, plus an explicit call of the generated function first | All OK |
| S4 | Runtime `Settings` after the first import | OK in both phases |
| S5 | Anonymous type under `"latte-dock"` before the engine | OK |
| S6 | On-disk qmldir with `plugin latteprivateappplugin` and no `.so`, added via `addImportPath` | Import OK. Trace: `resolvePlugin … file does not exist`; the resource qmldir and types are used |
| S7 | On-disk qmldir without a plugin line | OK |
| S8 | `QML_NAMED_ELEMENT` inside a namespace | Compile error (`friend` outside a class). Not viable |
| S9 | `qmllint -I build/qml` on a file that imports the module | `Interfaces` and `ContextMenuLayer` resolve; `Settings` does not |

Real tree (`/tmp/privateapp-proto`; `-DBUILD_TESTING=OFF`, Debug, Ninja, `-j6`):

| ID | Step | Result |
| --- | --- | --- |
| T1 | Configure with the plugin removed and the plain link calls kept | Configure error at `app/CMakeLists.txt:108` (keyword/plain conflict) |
| T2 | `PRIVATE` on the two link calls | Configure OK; no CMake warnings |
| T3 | `latte-dock-ng_qmltyperegistration` | qmldir: `module org.kde.latte.private.app`, `typeinfo latteprivateapp.qmltypes`, `prefer :/qt/qml/org/kde/latte/private/app/`, `depends QtQuick`; no `plugin` line. Generated registration defines `qml_register_types_org_kde_latte_private_app()` and a static `QQmlModuleRegistration` |
| T4 | Generated `latteprivateapp.qmltypes` against the installed `latteprivateappplugin.qmltypes` | Byte-identical (`diff` silent; 6013 bytes) |
| T5 | `check-modules` on the partial build | `FAILED — expected one plugin declaration in …/private/app/qmldir`. Other modules fail only because the build is partial |
| T6 | Full build, first try | Compile errors in the generated registration source (no header directories) |
| T7 | Second try with header directories | Link error: `lattecorona.cpp` referenced a `Latte::`-qualified symbol. Caused by my declaration placed inside `namespace Latte`; corrected to global scope |
| T8 | Third try | Builds. `build/bin/latte-dock-ng` created. Zero compiler warnings with `-Werror` active (182 `-Werror` flags in `build.ninja`). The pre-existing Debug qmllint step prints many QML warnings |
| S-smoke V0 | `LATTE_PRIVATE_APP_HOST_SMOKE=1`, offscreen, `env -i`, scratch HOME; no runtime `Settings` | rc=0: `private.app loaded in latte-dock-ng host process` |
| V1 | Runtime `Settings` registered before the smoke component (production order) | rc=1: `LatteApp.ContextMenuLayer is not a type` |
| V2 | Explicit call first, then the same runtime `Settings` | rc=0 |

Isolation: the build directory's `qt.conf` sets `Prefix=/usr/lib64/qt6`, `Plugins=plugins`
and `Qml2Imports=/tmp/r6-qt-qml`. That directory contains symlinks to Qt's QtQuick and QtQml
only. Without it, the engine loaded the installed copy under `/usr/lib64/qt6/qml/org/kde/latte`
(`locateLocalQmldir` in the trace), which changed the results.

Not run: Clang, ASan, `ctest`, `qmllint-deep`, `cmake --install`, the desktop.

## 6. Not verified

- Qt 6.6 and 6.7 behaviour. Qt 6.8.2 on Debian 13 is deferred by instruction.
- Plasma 6.3 (the host has 6.7.5).
- Clang, ASan/UBSan, `ctest`, `qmllint-deep` on all modules, and the baseline comparison.
- `cmake --install` and the installed layout. A stale `.so` being loaded was not tested; S6
  covers only a missing `.so`.
- Desktop behaviour. The `Corona` order was reproduced by the smoke switch, not by running the dock.
- The internals of `QQmlModuleRegistration`. The behaviour is observed, not read.
- Performance. Nothing was measured.

## 7. Reproduction

- Standalone: `/tmp/r6-qmlexe-proto` (`CMakeLists.txt`, `hostdemo.{h,cpp}`, `main.cpp`).
- Real tree: `/tmp/privateapp-proto`, with edit scripts `apply_proto_edits.py`,
  `keyword_link_fix.py`, `include_dirs_fix.py` and `global_decl_fix.py`. The combined patch and
  logs are in the shared notes folder.
- Configure and build: `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=OFF`,
  then `cmake --build build --target latte-dock-ng -j6`.
- Smoke, with no desktop session:
  `env -i HOME=<scratch> PATH=/usr/bin:/bin LATTE_PRIVATE_APP_HOST_SMOKE=1 LATTE_DISABLE_USER_LOCAL_QML_IMPORTS=1 QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software ./build/bin/latte-dock-ng`.
