# R7g: Dependency floor audit (Qt 6.6, KF6 6.0, Plasma 6.3)

Status: Complete (audit only). No tracked source, CMake, packaging or doc file was changed; this record is the only output.
Date: 2026-11-06. Branch: `claude/r7g-floor-audit-4da23c`. Start HEAD: `e18a0f9a7` (release v1.3.0).

**Applied.** Option A of section 6 was applied on 2026-11-06 in `072cbcb76` (merge `23cc38eb4`). The text below is otherwise unchanged; only this note is new.

## Verdict

- **Not reachable.** Qt 6.6 and KF6 6.0 cannot be built together with Plasma 6.3: libplasma 6.3.0 requires Qt 6.7.0 and KF6 6.10.0 (libplasma v6.3.0 `CMakeLists.txt:6, 8`). The declared pair is the floor of Plasma 6.0.0, not 6.3.
- **Effective floor: Qt 6.7.0, KF6 6.10.0, Plasma 6.3.0.** Latte's own code also needs KF6 6.8 (`KF6::I18nQml` in the preview helper, not guarded) and PlasmaWaylandProtocols 1.10.0 (`kde-output-order-v1.xml`); the declared 1.6 is too low. No Qt API above 6.6 was found in the checked set (section 3).
- **Actually tested: only Debian 13** (Qt 6.8.2, KF6 6.13.0, libplasma 6.3.5, plasma-workspace 6.3.6), enforced by `docker/verify-install.sh:38-53`. The GCC/Clang build jobs install unpinned KDE Neon packages (Qt 6.11.1, Plasma 6.8.2, KF6 6.31 in the 2026-11-06 index). No job builds Qt 6.6, KF6 6.0, Plasma 6.3.0, Qt 6.7 or KF6 6.10.
- **No known image provides Qt 6.6 with Plasma 6.3.** Upstream rejects that pair at configure time, and Debian 13's libplasma package depends on Qt >= 6.7, so option B has no image to add. A Qt 6.7 / KF6 6.10 / Plasma 6.3 environment is not in the CI matrix.
- **Recommendation (A):** set `QT_MIN_VERSION` 6.7.0 and `KF6_MIN_VERSION` 6.10.0, check KWayland against `PLASMA_MIN_VERSION`, and raise PlasmaWaylandProtocols to 1.10.0. No distro in the repo's matrix falls below these values (section 4). The diff is in section 6 and was not applied.

## 1. What Plasma 6.3.0 and its neighbours require

Source: upstream `CMakeLists.txt` at tag `v6.3.0` unless stated. Line numbers refer to those files.

| Package (6.3.0) | Qt minimum | KF6 / ECM minimum | Other declared minimums | Source |
| --- | --- | --- | --- | --- |
| libplasma | 6.7.0 (line 6; `find_package(Qt6)` line 46) | 6.10.0 (lines 8, 13, 48) | PlasmaActivities 6.3.0 (line 65); PlasmaWaylandProtocols 1.10.0 (line 68); Qt6WaylandClient 6.7.0 (line 69); Wayland 1.9 (line 70); CMake 3.16 (line 1) | [CMakeLists.txt](https://invent.kde.org/plasma/libplasma/-/raw/v6.3.0/CMakeLists.txt) |
| plasma-workspace (also builds LibTaskManager) | 6.7.0 (line 11) | 6.10.0 (lines 12, 20, 58-63) | Plasma, PlasmaQuick, KWayland, PlasmaActivities, PlasmaActivitiesStats, KSysGuard, KF6Screen, KScreenLocker, LayerShellQt all at 6.3.0 (lines 64-73; `PROJECT_DEP_VERSION` line 10); PlasmaWaylandProtocols 1.6 (line 214); Wayland Client and Server (line 215); Phonon4Qt6 4.6.60 (line 129); `add_subdirectory(libtaskmanager)` (line 405) | [CMakeLists.txt](https://invent.kde.org/plasma/plasma-workspace/-/raw/v6.3.0/CMakeLists.txt) |
| kwayland (KWayland) | 6.7.0 (line 6) | 6.10.0 (lines 7, 12) | Wayland 1.15 (line 48); WaylandProtocols 1.15 (line 55); PlasmaWaylandProtocols 1.16.0, `TYPE REQUIRED` (lines 61-62) | [CMakeLists.txt](https://invent.kde.org/plasma/kwayland/-/raw/v6.3.0/CMakeLists.txt) |
| layer-shell-qt (LayerShellQt) | 6.7.0 (line 12) | 6.10.0 (line 13) | Wayland 1.3 (line 38) | [CMakeLists.txt](https://invent.kde.org/plasma/layer-shell-qt/-/raw/v6.3.0/CMakeLists.txt) |
| plasma-activities (PlasmaActivities) | 6.7.0 (line 8) | 6.10.0 (line 9) | Qt Core and DBus (line 41) | [CMakeLists.txt](https://invent.kde.org/plasma/plasma-activities/-/raw/v6.3.0/CMakeLists.txt) |
| plasma-activities-stats | 6.7.0 (line 9) | 6.10.0 (line 10; KF6Config line 38) | PlasmaActivities 6.3.0 (line 39) | [CMakeLists.txt](https://invent.kde.org/plasma/plasma-activities-stats/-/raw/v6.3.0/CMakeLists.txt) |
| LibTaskManager | not separate | not separate | Built by plasma-workspace; its config version is the plasma-workspace version (`libtaskmanager/CMakeLists.txt:136`); `Qt6_VERSION >= 6.8.0` selects `PRIVATE_CODE` for its Wayland protocol sources (lines 46-48) | [libtaskmanager/CMakeLists.txt](https://invent.kde.org/plasma/plasma-workspace/-/raw/v6.3.0/libtaskmanager/CMakeLists.txt) |
| PlasmaWaylandProtocols (Latte's own need) | not applicable | not applicable | `kde-output-order-v1.xml` is absent at tags v1.6.0, v1.7.0, v1.8.0, v1.9.0 and present at v1.10.0 | [KDE/plasma-wayland-protocols](https://github.com/KDE/plasma-wayland-protocols) (git trees API) |
| KF6 frameworks at 6.0.0 (the 17 components listed at `CMakeLists.txt:94-96`; KF6Declarative at line 141 is one of them) | 6.5.0 (`REQUIRED_QT_VERSION`) | ECM 6.0.0; CMake 3.16 | All 17 repositories exist at v6.0.0 | e.g. [kcoreaddons v6.0.0](https://invent.kde.org/frameworks/kcoreaddons/-/raw/v6.0.0/CMakeLists.txt) |
| KF6 frameworks at 6.10.0 | 6.6.0 | ECM 6.10.0 | Same components | e.g. [kcoreaddons v6.10.0](https://invent.kde.org/frameworks/kcoreaddons/-/raw/v6.10.0/CMakeLists.txt) |
| Gentoo 6.3.0 ebuilds (bumped 2025-02-11, dropped 2025-02-18) | QTMIN 6.8.1 (libplasma line 9; plasma-workspace line 9; layer-shell-qt line 7; kwayland line 8; activities and stats line 9) | KFMIN 6.10.0 (one line above each QTMIN) | libplasma DEPEND `plasma-wayland-protocols-1.16.0` (line 49); kwayland (line 30) | e.g. [libplasma-6.3.0.ebuild](https://raw.githubusercontent.com/gentoo/gentoo/e94242f2e71d131a1549ba14360c2f7b3218b923/kde-plasma/libplasma/libplasma-6.3.0.ebuild); the local `/var/db/repos/gentoo/kde-plasma/libplasma/` holds only 6.7.5, 6.7.6 and 6.8.2 |
| Debian 13 libplasma 6.3.5-1 (`debian/control`) | qt6-base, qt6-declarative, qt6-svg, qt6-wayland dev packages >= 6.7.0~ | libkf6-*-dev >= 6.10.0~ | plasma-wayland-protocols >= 1.10.0~ | [sources.debian.org](https://sources.debian.org/src/libplasma/6.3.5-1/debian/control/) |
| Plasma 6.3 announcement | not stated | not stated | The page text gives no Qt or KF requirement | [kde.org](https://kde.org/announcements/plasma/6/6.3.0/) |
| libplasma history | 6.0.0: Qt 6.6.0, KF 6.0.0 (lines 6-7); 6.1.0: Qt 6.6.0, KF 6.2.0 (lines 6, 8); 6.2.0: Qt 6.7.0, KF 6.5.0 (lines 6, 8) | | | [v6.0.0](https://invent.kde.org/plasma/libplasma/-/raw/v6.0.0/CMakeLists.txt), [v6.1.0](https://invent.kde.org/plasma/libplasma/-/raw/v6.1.0/CMakeLists.txt), [v6.2.0](https://invent.kde.org/plasma/libplasma/-/raw/v6.2.0/CMakeLists.txt) |

## 2. Every stated minimum in the repository

Values are as written in the file. "Matches the 6.3 chain" compares them with section 1.

### 2a. Build files

| Where | Stated minimum | Matches the 6.3 chain? |
| --- | --- | --- |
| `CMakeLists.txt:15` | `QT_MIN_VERSION` 6.6.0 | No (6.7.0) |
| `CMakeLists.txt:16` | `KF6_MIN_VERSION` 6.0.0 | No (6.10.0). Also used for `find_package(KWayland)` at line 101, but KWayland is versioned with Plasma (6.3.0) |
| `CMakeLists.txt:17` | `PLASMA_MIN_VERSION` 6.3.0 | Yes |
| `CMakeLists.txt:1` | `cmake_minimum_required` 3.20 | Yes (ECM and libplasma need 3.16) |
| `CMakeLists.txt:88` (comment) | "our minimum is 6.6" | No |
| `CMakeLists.txt:148` | `find_package(PlasmaWaylandProtocols 1.6)` | No (1.10.0 is needed by `app/CMakeLists.txt:65-66`) |
| `CMakeLists.txt:18` | `KDE_COMPILERSETTINGS_LEVEL` 6.0.0 | Not a dependency floor. It selects compiler settings and must not exceed the ECM minimum (`KDECompilerSettings.cmake`, ECM v6.10.0) |
| `CMakeLists.txt:105`; `cmake/CheckLayerShellSetScreen.cpp:6-7`; `app/wm/waylandinterface.cpp:195-196` | "added in LayerShellQt 6.6 and later removed upstream" | Added in 6.6: confirmed. "Removed": not reproduced (section 3, row 15) |
| `CMakePresets.json:3-5` | `cmakeMinimumRequired` 3.20 | Yes |
| `README.md:47-48` | cmake >= 3.20; GCC 10+ / Clang 12+ | Toolchain floors; not audited |
| `cmake/LattePackaging.cmake:38-40` | glibc >= 2.34 for the RPM | Platform floor; not audited |

### 2b. User-facing text

| Where | Stated minimum | Matches the 6.3 chain? |
| --- | --- | --- |
| `README.md:38` | Plasma >= 6.3.0 ("fully verified on Plasma 6.5+") | Yes |
| `README.md:39` | Qt >= 6.6 | No |
| `README.md:53-59` | Qt6Core, Gui, DBus, Qml, Quick, Widgets, WaylandClient >= 6.6.0 | No |
| `README.md:61-66` | Plasma, PlasmaQuick, PlasmaActivities, PlasmaActivitiesStats, LibTaskManager >= 6.3.0 | Yes |
| `README.md:64` | KWayland >= 6.0.0 | No (KWayland 6.3.0 is the Plasma 6.3 release) |
| `README.md:68-84` | 17 KF6 entries >= 6.0.0 | No (6.10.0) |
| `README.md:88` | LayerShellQt >= 6.0.0 | Not enforced by CMake (`CMakeLists.txt:102` has no version). plasma-workspace 6.3.0 requires 6.3.0 |
| `README.md:3` | "Plasma 6.3 (Debian 13 trixie) is verified as the minimum supported version" | Yes for Plasma. The verified Qt and KF6 are 6.8 and 6.13 |
| `README.md:5`; `INSTALLATION.md:4`; `flake.nix:2`; `latte-dock-ng.spec:60`; `cmake/LattePackaging.cmake:15`; `app/org.kde.latte-dock.desktop.cmake:38, 47`; `app/org.kde.latte-dock.appdata.xml.cmake:42, 51, 78, 87` | "Plasma 6.3+" | Yes |
| `INSTALLATION.md:87` | "Debian 13 (trixie) is the minimum supported release" | Yes |
| `docs/debian-apt-repository.md:101` | "Plasma 6.3+ ... Ubuntu support starts at 26.04" | Yes |
| `CHANGELOG.md:58` | "Debian 13 floor (Qt 6.8, KDE Frameworks 6.13, Plasma 6.3)" | Matches the tested stack |
| `CHANGELOG.md:600-602` | v1.2.34: "Lowered the minimum Plasma version from 6.5.0 to 6.3.0" | This entry changed only Plasma; Qt and KF6 were not touched |
| `PORTING_PLASMA6_WAYLAND.md:1, 3` | "Plasma 6.5+" | Stale title |
| `app/main.cpp:999` (comment) | "this project requires Qt 6.6+" | Restates the floor (changed in the diff) |

### 2c. Packaging and verifiers

| Where | Stated minimum | Matches the 6.3 chain? |
| --- | --- | --- |
| `docker/verify-install.sh:38-42`, enforced at `:51-53` | cmake 3.31.6; qt6-base-dev 6.8.2; libkf6coreaddons-dev 6.13.0; libplasma-dev 6.3.5; plasma-workspace-dev 6.3.6 | Yes. These are the only enforced minimums |
| `docker/verify-ebuild-gentoo.sh:78` | DESCRIPTION "for Plasma 6.5+" | Inconsistent with 6.3 |
| `docker/verify-ebuild-gentoo.sh:89-91, 127` | qtbase, qtdeclarative, qtwayland >= 6.6 | No |
| `docker/verify-ebuild-gentoo.sh:92-111, 128` | 20 KF6 packages and ECM >= 6.0 | No |
| `docker/verify-ebuild-gentoo.sh:112-118` | kpipewire, kwayland, layer-shell-qt, libplasma, plasma-activities, plasma-activities-stats, plasma-workspace >= 6.5 | Stricter than 6.3. A packaging decision; not changed in the diff |
| `docker/verify-ebuild-gentoo.sh:123` | plasma-wayland-protocols >= 1.6 | No (1.10.0) |
| `latte-dock-ng.spec:14-77` | BuildRequires and Requires carry no version numbers | Not applicable |
| `containment/package/metadata.json:83` | `"X-Plasma-API-Minimum-Version": "6.0"` | Not a dependency floor; below 6.3. Its meaning for Plasma was not verified |

### 2d. Project documents (not edited)

| Where | Statement | Note |
| --- | --- | --- |
| `docs/architecture-modernization-plan.md:10, 16, 208` | Keep CMake 3.20, Qt 6.6, KF6 6.0, Plasma 6.3 | Ledger; the main session owns it. The plan's reason is source compatibility with the oldest supported distro |
| `docs/architecture-modernization-baseline.md:56-57` | Source floors and Debian 13.7 acceptance values | Same decision |
| `docs/architecture-assessment-2026-10.md:52-53, 99` | "the floors are KF6 6.0 and Plasma 6.3"; deprecated APIs at or below the floors are not compiled | Restates |
| `docs/modernization-results/A0.md:10` | "requires Qt 6.6, KF6 6.0 and Plasma 6.3" | Restates |

## 3. Version gates, probes and APIs against the floors

"Above the floor" means above Qt 6.6 or KF6 6.0 (or above the 6.3 Plasma floor where marked). Tag evidence was read from the upstream trees fetched for this audit.

| # | Item | Where | What was checked | Above the floor? |
| --- | --- | --- | --- | --- |
| 1 | `KF6::I18nQml`, `KLocalizedQmlContext` | `plasmoid/CMakeLists.txt:14` (unconditional); `plasmoid/preview/main.cpp:18, 97` | KI18n `src/CMakeLists.txt` adds `i18n-qml` only from v6.8.0 (line 7); v6.7.0 has no such entry. `src/i18n-qml/klocalizedqmlcontext.h` is present at v6.10.0 | **Yes: KF6 >= 6.8.0** |
| 2 | The same class in the application | `app/CMakeLists.txt:135-137`; `autotests/CMakeLists.txt:967-969`; `compat/KDeclarative/QmlObjectSharedEngine:18-28` | The `#else` branch compiles only where `PlasmaQuick::SharedQmlEngine` is absent | Gated (Plasma >= 6.8) |
| 3 | Plasma 6.8 screen API | `app/plasma/plasmaapicompat.h:20-30, 43-51`; `app/main.cpp:632` | libplasma v6.3.0: `int screen()` (`containment.h:190`), `int lastScreen()` (`:197`), `screenForContainment` (`corona.h:210`); v6.8.0: `uint screen()` (`containment.h:227`), `setScreen(uint)` (`:235`). `src/plasmaquick/sharedqmlengine.h` is present at v6.3.0 and absent at v6.8.0 | Gated. The 6.3 floor uses the pre-6.8 branch, which is correct |
| 4 | `KDeclarative::QmlObjectSharedEngine` | `compat/KDeclarative/QmlObjectSharedEngine:3-16` | KDeclarative v6.0.0 has no such class. The shim subclasses `PlasmaQuick::SharedQmlEngine` | No KF requirement |
| 5 | Drag-event QPointF constructors | `app/view/eventssink.cpp:103, 130` | Qt 6.6.0 and 6.11.0 `qevent.h`: QPoint constructors only (6.6.0 lines 752, 772). Qt 6.12.0 adds QPointF overloads (lines 763, 788); the QPoint ones are deprecated since 6.16 | Gated at 6.12. The QPoint branch exists in 6.6 |
| 6 | `QStringBuilder` operand storage | `app/plasma/extended/backgroundcache.cpp:44-49` | Qt 6.6.0 `text/qstringbuilder.h` stores `const A &a` (line 100) and `const B &b` (line 101); 6.7.0 stores `A a; B b;` (lines 140-141) | Behaviour changes at 6.7. The comment is correct |
| 7 | `QT_NO_CONTEXTLESS_CONNECT` | `CMakeLists.txt:304-307` | Qt 6.6.0 `qobject.h` lacks the macro; 6.7.0 `qobject.h:260` guards the contextless `connect` | Enforced from 6.7 |
| 8 | `QTemporaryDir` move assignment | `app/settings/settingsdialog/layoutscontroller.cpp:928-930` | `QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QTemporaryDir)` at `qtemporarydir.h:27` in both v6.8.2 and v6.6.0. The macro definition was not found in the headers tried | Unresolved. Probably not above 6.6. The code uses `unique_ptr` regardless |
| 9 | `QScopeGuard` | `autotests/coreunittest.cpp:21` | `src/corelib/tools/qscopeguard.h` at qtbase v6.6.0 | No |
| 10 | `QtQuick.Effects` (MultiEffect) | 17 `import QtQuick.Effects` lines in QML | `src/effects/qquickmultieffect.cpp` at qtdeclarative v6.6.0 | No |
| 11 | `KIO::OpenFileManagerWindowJob` | `app/settings/settingsdialog/settingsdialog.cpp:41` and three more files | `src/gui/openfilemanagerwindowjob.h` at kio v6.0.0 | No |
| 12 | `KNSWidgets::Dialog` | `app/settings/settingsdialog/tablayoutshandler.cpp:41, 471` | `src/widgets/dialog.h` at knewstuff v6.0.0 | No |
| 13 | `KWindowShadow`, `KWindowEffects` | `app/view/panelshadows.cpp:12`; `app/view/effects.cpp:22` | `src/kwindowshadow.h` and `src/kwindoweffects.h` at kwindowsystem v6.0.0 | No |
| 14 | `KSvg::FrameSvg`, `KSvg::Svg` | `app/infoview.h:33`; `app/plasma/extended/panelbackground.cpp:40` | `src/ksvg/framesvg.h` and `svg.h` at ksvg v6.0.0 | No |
| 15 | LayerShellQt `setScreen(QScreen*)` | `app/wm/waylandinterface.cpp:195-200`; probe `cmake/CheckLayerShellSetScreen.cpp` | `src/interfaces/window.h`: absent at v6.3.0; present at v6.6.0 (line 132) and v6.8.0 (line 132). Tags after 6.8 are not in the mirror, and the host's installed header still declares it | Probe-gated (correct). "Later removed upstream" was not reproduced |
| 16 | LayerShellQt `setDesiredSize` | probe `cmake/CheckLayerShellDesiredSize.cpp`; `CMakeLists.txt:117-128` | Absent at v6.3.0; present at v6.6.0 and v6.8.0 (line 83) | Probe-gated |
| 17 | LayerShellQt `setActivateOnShow` | probe `cmake/CheckLayerShellActivateOnShow.cpp`; `CMakeLists.txt:130-140` | Absent at v6.3.0; present at v6.6.0 and v6.8.0 (line 163) | Probe-gated |
| 18 | Debian 13 LayerShellQt | none | `liblayershellqtinterface-dev` 6.3.4-1 in trixie ([packages.debian.org](https://packages.debian.org/trixie/liblayershellqtinterface-dev)). Consistent with the probe comments at `CMakeLists.txt:118-119, 131` | Context only |
| 19 | `kde-output-order-v1.xml` | `app/CMakeLists.txt:65-66` | plasma-wayland-protocols v1.6.0 to v1.9.0: absent; v1.10.0: present | **Yes: PlasmaWaylandProtocols >= 1.10.0 (declared 1.6)** |
| 20 | `kde-primary-output-v1.xml` | `app/CMakeLists.txt:57-58` | Present at v1.6.0 and later | No |
| 21 | `#if QT_VERSION < QT_VERSION_CHECK(6, 15, 0)` | `containment/plugin/layoutmanager.cpp:787` | Selects a fallback compiled below 6.15 | No |
| 22 | `PLASMA_VERSION` integer | ECM v6.10.0 `ECMVersionHeader.h.in:10`: `(major<<16)\|(minor<<8)\|patch` | Same encoding as `QT_VERSION_CHECK`. The host's installed `/usr/include/Plasma/` header (libplasma 6.7.5) encodes `((6<<16)\|(7<<8)\|(5))` | Valid as used |
| 23 | Compiler and CMake gates | `app/CMakeLists.txt:73` (GCC >= 16); `cmake/LatteStrictWarnings.cmake:20` (CMake >= 3.24) | Read only | Not library floors |
| 24 | `__has_include` probes | `compat/**` (KActivities, KArchive, KIconThemes, KPackage, KNS, Plasma); `app/main.cpp:63, 632, 697` | Header-name probes covering KF5-era and distribution layouts | Not version gates |
| 25 | `ecm_set_disabled_deprecation_versions(QT ${QT_MIN_VERSION} KF ${KF6_MIN_VERSION})` | `CMakeLists.txt:296-302` | ECM writes `QT_DISABLE_DEPRECATED_BEFORE` for Qt and `KF_DISABLE_DEPRECATED_BEFORE_AND_AT` for KF (`ECMDeprecationSettings.cmake:82-84, 96`) | Raising the floors removes more APIs at compile time. See section 6 |

## 4. What is actually tested

| Job (`.github/workflows/build.yml`) | Environment | Versions in effect | At the floor? |
| --- | --- | --- | --- |
| build: GCC and Clang, Debug and Release (lines 42-175) | `ubuntu:24.04` container (line 47) with the KDE Neon `user noble` repository (steps at lines 77-133) | Unpinned (lines 136-148). Neon index of 2026-11-06 (`archive.neon.kde.org/user/dists/noble/main/binary-amd64/Packages.gz`): qt6-base-dev and qt6-declarative-dev 6.11.1; libplasma-dev 6.8.2; plasma-workspace-dev 4:6.8.2; liblayershellqtinterface-dev 6.8.2; plasma-activities-dev 6.8.2; libkpipewire-dev 6.8.2; libkf6*-dev 6.31.0; extra-cmake-modules 6.31.0 | No. The index is a snapshot, not a CI log |
| qml-lint (lines 254-274) | `ubuntu:24.04` without Neon | Ubuntu noble qt6-declarative-dev 6.4.2+dfsg-4build3 ([packages.ubuntu.com](https://packages.ubuntu.com/noble/qt6-declarative-dev)). A syntax-only gate (`scripts/qmllint.sh:4-8`) | No. Below the 6.6 floor |
| install-verify, debian13 (lines 301-304) | `debian:trixie` (`docker/Dockerfile.debian`, `DEBIAN_BASE=trixie`) | `verify-install.sh` fails below cmake 3.31.6, qt6-base-dev 6.8.2, libkf6coreaddons-dev 6.13.0, libplasma-dev 6.3.5, plasma-workspace-dev 6.3.6 | **Yes: the tested floor** |
| package-install-verify, debian13 (lines 367-371; packaging steps 404-456) | `debian:trixie`, Release; `cpack -G DEB` and install | Compiles the application and installs the package against the Debian 13 stack | **Yes** |
| debian-testing (lines 305-307, 372-375) | `debian:testing` | Newer than the floor | Not a floor test |
| ubuntu 26.04 (lines 294-295, 376-379) | `ubuntu:26.04` | Qt 6.10.2 and KF6 6.24.0 in the local refresh record (`docs/modernization-results/local-distro-refresh-2026-09-30.md:44`); libPlasma 6.6.6 per `docs/release-workflow.md:236-237` | No |
| fedora (default 45), opensuse (tumbleweed), arch (latest), mageia (cauldron) (lines 285-298) | rolling or release images | Qt 6.10.0 to 6.11.2 and KF6 6.22 to 6.30 in the local refresh record (`docs/modernization-results/local-distro-refresh-2026-09-30.md:39-45`) | No |
| nixos-release-check (lines 481-521) | `ubuntu-24.04` runner with Nix | nixpkgs `b5aa0fbd5389` (`flake.lock`, lastModified 2026-06-29); its Qt and Plasma versions were not examined | Unknown |
| Maintainer desktop checks | Debian 13.7 VM | Qt 6.8.2, KF6 6.13, Plasma 6.3.6 (`docs/modernization-results/debian13-runtime.md:3`) | Yes (documented; not re-run here) |

No CI job builds with Qt 6.6, KF6 6.0, Plasma 6.3.0, Qt 6.7 or KF6 6.10. `git grep` of `.github` and `docker` for `6\.6` and `6\.0` finds no build pin.

## 5. Reachability

| Stack | Reachable for Latte? | Evidence |
| --- | --- | --- |
| Qt 6.6 + KF6 6.0 + Plasma 6.3.0 (declared) | **No** | libplasma v6.3.0 lines 6 and 8 (Qt 6.7.0, KF6 6.10.0); plasma-workspace v6.3.0 lines 11-12 |
| Qt 6.6 + KF6 6.0 + Plasma 6.0.0 | No, for Latte | The Plasma floors match (libplasma v6.0.0 lines 6-7), but the preview helper needs KF6 6.8 (section 3, row 1) |
| Qt 6.6 + KF6 6.2 + Plasma 6.1.0 | No, for Latte | libplasma v6.1.0 lines 6-8; KF6 6.8 still needed |
| Qt 6.7 + KF6 6.5 + Plasma 6.2.0 | No, for Latte | libplasma v6.2.0 lines 6-8; KF6 6.8 still needed |
| Qt 6.7.0 + KF6 6.10.0 + Plasma 6.3.0 | **Yes** (upstream build floors) | libplasma v6.3.0 lines 6-8; plasma-workspace v6.3.0 lines 11-12. No CI job builds it |
| Qt 6.8.2 + KF6 6.13.0 + Plasma 6.3.5 or 6.3.6 (Debian 13) | **Yes, tested** | `docker/verify-install.sh:38-42`; Debian libplasma 6.3.5-1 control |

## 6. Recommendation and proposed diff

| Option | What it means | Consequence | Verdict |
| --- | --- | --- | --- |
| A. Raise the floors to the effective floor | Qt 6.7.0, KF6 6.10.0, KWayland checked against Plasma, PlasmaWaylandProtocols 1.10.0; docs and Gentoo verifier updated (diff below) | Declared floors become true. No distro in the matrix is excluded: Debian 13 (6.8.2 / 6.13), Ubuntu 26.04 (6.10.2 / 6.24), Mageia (6.10.0 / 6.22), Gentoo, Fedora, openSUSE, Arch are all above. The existing Debian 13 and Neon jobs compile with the new define values; Qt 6.7 headers are not exercised | **Recommended.** Conclusive for the Qt, KF6 and Plasma floors because the upstream build requirements decide them. The Qt 6.7 compile is the one gap |
| B. Keep the floors and add a CI job | A job for Qt 6.6 with Plasma 6.3 | No known image provides it. libplasma 6.3.0 rejects Qt 6.6 at configure time (section 1). A Qt 6.7 / KF6 6.10 / Plasma 6.3 image is not in the matrix | Not possible as stated. A Qt 6.7 job would need a new image |
| C. Leave as is | Keep the declared floors | The declared 6.6 and 6.0 stay false, and the preview helper already needs KF6 6.8. The plan's reason (`docs/architecture-modernization-plan.md:14-16`) is source compatibility with the oldest supported distro; raising to 6.7 and 6.10 excludes no distro in the matrix | Not recommended |

The diff below was generated from the current tree and checked with `git apply --check -p1` on this branch; it does not touch any file under autotests. Apply it only after the user decides on option A. Its build-affecting lines are the two floors, the KWayland lookup and the PlasmaWaylandProtocols floor. The rest keeps comments and docs consistent.

````diff
--- a/CMakeLists.txt
+++ b/CMakeLists.txt
@@ -14,4 +14,4 @@
 
-set(QT_MIN_VERSION "6.6.0")
-set(KF6_MIN_VERSION "6.0.0")
+set(QT_MIN_VERSION "6.7.0")
+set(KF6_MIN_VERSION "6.10.0")
 set(PLASMA_MIN_VERSION "6.3.0")
@@ -87,3 +87,3 @@
 # qt_policy comes with the Qt6::Core macros loaded by the find_package call
-# above (QTP0001 exists since Qt 6.5; our minimum is 6.6).
+# above (QTP0001 exists since Qt 6.5; our minimum is 6.7).
 qt_policy(SET QTP0001 NEW)
@@ -100,3 +100,5 @@
 find_package(PlasmaActivities ${PLASMA_MIN_VERSION} REQUIRED)
-find_package(KWayland ${KF6_MIN_VERSION} REQUIRED)
+# KWayland is released with Plasma, not with KDE Frameworks: its version
+# follows PLASMA_MIN_VERSION (6.3.0 ships with Plasma 6.3).
+find_package(KWayland ${PLASMA_MIN_VERSION} REQUIRED)
 find_package(LayerShellQt REQUIRED)
@@ -147,3 +149,5 @@
 find_package(Qt${QT_MAJOR_VERSION}WaylandClient REQUIRED)
-find_package(PlasmaWaylandProtocols 1.6 REQUIRED)
+# app/CMakeLists.txt compiles kde-output-order-v1.xml, which plasma-wayland-protocols
+# 1.9.0 does not ship and 1.10.0 does.
+find_package(PlasmaWaylandProtocols 1.10.0 REQUIRED)
 find_package(Wayland REQUIRED COMPONENTS Client)
@@ -305,3 +309,3 @@
 # running after the objects it captures are destroyed. Qt 6.7 and newer
-# enforce the macro; the Qt 6.6 floor ignores it.
+# enforce the macro; with the Qt 6.7 floor it is always active.
 add_compile_definitions(QT_NO_CONTEXTLESS_CONNECT)
--- a/README.md
+++ b/README.md
@@ -38,3 +38,3 @@
 - **Plasma >= 6.3.0** (developed and fully verified on Plasma 6.5+)
-- **Qt >= 6.6**
+- **Qt >= 6.7**
 - **KCMUtils, Kirigami and NewStuff QML modules**
@@ -52,9 +52,9 @@
 ```
- Qt6Core >= 6.6.0
- Qt6Gui >= 6.6.0
- Qt6DBus >= 6.6.0
- Qt6Qml >= 6.6.0
- Qt6Quick >= 6.6.0
- Qt6Widgets >= 6.6.0
- Qt6WaylandClient >= 6.6.0
+ Qt6Core >= 6.7.0
+ Qt6Gui >= 6.7.0
+ Qt6DBus >= 6.7.0
+ Qt6Qml >= 6.7.0
+ Qt6Quick >= 6.7.0
+ Qt6Widgets >= 6.7.0
+ Qt6WaylandClient >= 6.7.0
 
@@ -63,3 +63,3 @@
  PlasmaActivities >= 6.3.0
- KWayland >= 6.0.0
+ KWayland >= 6.3.0
  PlasmaActivitiesStats >= 6.3.0
@@ -67,19 +67,19 @@
 
- KF6Config >= 6.0.0
- KF6CoreAddons >= 6.0.0
- KF6GuiAddons >= 6.0.0
- KF6DBusAddons >= 6.0.0
- KF6Declarative >= 6.0.0
- KF6Package >= 6.0.0
- KF6XmlGui >= 6.0.0
- KF6IconThemes >= 6.0.0
- KF6KIO >= 6.0.0
- KF6I18n >= 6.0.0
- KF6Notifications >= 6.0.0
- KF6NewStuff >= 6.0.0
- KF6Archive >= 6.0.0
- KF6GlobalAccel >= 6.0.0
- KF6Crash >= 6.0.0
- KF6WindowSystem >= 6.0.0
- KF6Svg >= 6.0.0
+ KF6Config >= 6.10.0
+ KF6CoreAddons >= 6.10.0
+ KF6GuiAddons >= 6.10.0
+ KF6DBusAddons >= 6.10.0
+ KF6Declarative >= 6.10.0
+ KF6Package >= 6.10.0
+ KF6XmlGui >= 6.10.0
+ KF6IconThemes >= 6.10.0
+ KF6KIO >= 6.10.0
+ KF6I18n >= 6.10.0
+ KF6Notifications >= 6.10.0
+ KF6NewStuff >= 6.10.0
+ KF6Archive >= 6.10.0
+ KF6GlobalAccel >= 6.10.0
+ KF6Crash >= 6.10.0
+ KF6WindowSystem >= 6.10.0
+ KF6Svg >= 6.10.0
 
@@ -87,3 +87,3 @@
  Wayland::Client
- LayerShellQt >= 6.0.0
+ LayerShellQt >= 6.3.0
 ```
--- a/docker/verify-ebuild-gentoo.sh
+++ b/docker/verify-ebuild-gentoo.sh
@@ -88,25 +88,25 @@
 	>=dev-libs/wayland-1.22
-	>=dev-qt/qtbase-6.6:6[dbus,gui,widgets]
-	>=dev-qt/qtdeclarative-6.6:6
-	>=dev-qt/qtwayland-6.6:6
-	>=kde-frameworks/karchive-6.0:6
-	>=kde-frameworks/kcmutils-6.0:6
-	>=kde-frameworks/kconfig-6.0:6
-	>=kde-frameworks/kcoreaddons-6.0:6
-	>=kde-frameworks/kcrash-6.0:6
-	>=kde-frameworks/kdbusaddons-6.0:6
-	>=kde-frameworks/kdeclarative-6.0:6
-	>=kde-frameworks/kglobalaccel-6.0:6
-	>=kde-frameworks/kguiaddons-6.0:6
-	>=kde-frameworks/ki18n-6.0:6
-	>=kde-frameworks/kiconthemes-6.0:6
-	>=kde-frameworks/kio-6.0:6
-	>=kde-frameworks/kirigami-6.0:6
-	>=kde-frameworks/kitemmodels-6.0:6
-	>=kde-frameworks/knewstuff-6.0:6
-	>=kde-frameworks/knotifications-6.0:6
-	>=kde-frameworks/kpackage-6.0:6
-	>=kde-frameworks/ksvg-6.0:6
-	>=kde-frameworks/kwindowsystem-6.0:6
-	>=kde-frameworks/kxmlgui-6.0:6
+	>=dev-qt/qtbase-6.7:6[dbus,gui,widgets]
+	>=dev-qt/qtdeclarative-6.7:6
+	>=dev-qt/qtwayland-6.7:6
+	>=kde-frameworks/karchive-6.10:6
+	>=kde-frameworks/kcmutils-6.10:6
+	>=kde-frameworks/kconfig-6.10:6
+	>=kde-frameworks/kcoreaddons-6.10:6
+	>=kde-frameworks/kcrash-6.10:6
+	>=kde-frameworks/kdbusaddons-6.10:6
+	>=kde-frameworks/kdeclarative-6.10:6
+	>=kde-frameworks/kglobalaccel-6.10:6
+	>=kde-frameworks/kguiaddons-6.10:6
+	>=kde-frameworks/ki18n-6.10:6
+	>=kde-frameworks/kiconthemes-6.10:6
+	>=kde-frameworks/kio-6.10:6
+	>=kde-frameworks/kirigami-6.10:6
+	>=kde-frameworks/kitemmodels-6.10:6
+	>=kde-frameworks/knewstuff-6.10:6
+	>=kde-frameworks/knotifications-6.10:6
+	>=kde-frameworks/kpackage-6.10:6
+	>=kde-frameworks/ksvg-6.10:6
+	>=kde-frameworks/kwindowsystem-6.10:6
+	>=kde-frameworks/kxmlgui-6.10:6
 	>=kde-plasma/kpipewire-6.5:6
@@ -122,3 +122,3 @@
 DEPEND="\${COMMON_DEPEND}
-	>=dev-libs/plasma-wayland-protocols-1.6
+	>=dev-libs/plasma-wayland-protocols-1.10
 "
@@ -126,4 +126,4 @@
 ${git_bdepend}
-	>=dev-qt/qtbase-6.6:6
-	>=kde-frameworks/extra-cmake-modules-6.0
+	>=dev-qt/qtbase-6.7:6
+	>=kde-frameworks/extra-cmake-modules-6.10
 	virtual/pkgconfig
--- a/app/view/eventssink.cpp
+++ b/app/view/eventssink.cpp
@@ -90,3 +90,3 @@
     // Qt 6.12 changed drag-event constructors to QPointF; retain QPoint for the
-    // supported Qt 6.6-6.11 range and preserve fractional positions on newer Qt.
+    // supported Qt 6.7-6.11 range and preserve fractional positions on newer Qt.
     switch (e->type()) {
--- a/app/main.cpp
+++ b/app/main.cpp
@@ -998,3 +998,3 @@
         //! block warnings from dependencies that still ship legacy QML snippets.
-        //! this project requires Qt 6.6+, so warnings related to Qt < 6 fallback code are irrelevant here.
+        //! this project requires Qt 6.7+, so warnings related to Qt < 6 fallback code are irrelevant here.
         //! this also filters a known Qt/Plasma startup warning from workspace calendar internals.
--- a/app/plasma/extended/backgroundcache.cpp
+++ b/app/plasma/extended/backgroundcache.cpp
@@ -43,3 +43,3 @@
 {
-    // QT_USE_QSTRINGBUILDER makes `+` return a QStringBuilder. Up to Qt 6.6 (our floor) a
+    // QT_USE_QSTRINGBUILDER makes `+` return a QStringBuilder. Up to Qt 6.6 a
     // builder keeps references to its operands, which here are temporaries that are gone
--- a/app/settings/settingsdialog/layoutscontroller.cpp
+++ b/app/settings/settingsdialog/layoutscontroller.cpp
@@ -928,3 +928,3 @@
     //! It is held by a pointer because the fallback replaces the directory object: QTemporaryDir cannot be
-    //! copied, and its move assignment was not checked against the declared Qt minimum (6.6; it exists in 6.8.2).
+    //! copied, and its move assignment was not checked against the declared Qt minimum (6.7; it exists in 6.8.2).
     auto layoutTempDir = std::make_unique<QTemporaryDir>(Latte::Layouts::Importer::layoutUserDir() + QStringLiteral("/.apply-XXXXXX"));
````

Decisions left out of the diff:

1. The Gentoo verifier's Plasma 6.5 floors (`docker/verify-ebuild-gentoo.sh:78, 112-118`). Lowering them to 6.3 or keeping them is a packaging choice.
2. PlasmaWaylandProtocols: the diff uses 1.10.0, which is what Latte's own protocol files need. KWayland 6.3.0 itself declares 1.16.0 (KWayland v6.3.0 `CMakeLists.txt:61`). Use 1.16.0 if the floor should mirror the Plasma chain.
3. `KDE_COMPILERSETTINGS_LEVEL 6.0.0` (`CMakeLists.txt:18`): a compiler-settings level, not a floor. Changing it changes warnings and needs a warning check.
4. The ledger (`docs/architecture-modernization-plan.md:10, 16`) and `docs/architecture-modernization-baseline.md:56-57` still state the old floors. The main session owns them.
5. The "later removed upstream" claim for `setScreen` (`CMakeLists.txt:105`, `cmake/CheckLayerShellSetScreen.cpp:6-7`, `app/wm/waylandinterface.cpp:195-196`) was not reproduced and is not changed by the diff.
6. The QTemporaryDir comment (`layoutscontroller.cpp:929`) keeps its claim. Only the version number changed.
7. `CMakeLists.txt:296` says "deprecated at or before". ECM applies "before" to Qt and "before and at" to KF (`ECMDeprecationSettings.cmake:82-84`). The wording was not changed.
8. `PORTING_PLASMA6_WAYLAND.md` title "Plasma 6.5+" is stale. Not changed.
9. `containment/package/metadata.json:83` (`X-Plasma-API-Minimum-Version` "6.0") is not a dependency floor. Not changed.

## 7. Not verified, and limits

- Nothing was compiled. No Qt 6.6 or 6.7 environment and no KF6 6.0 or 6.10 environment was built, and Docker and Debian work were out of scope. "Reachable" means the upstream CMake requirements allow it; it is not a build result.
- No symbol-level audit of every Qt, KF6 or Plasma call was done. Section 3 covers the version gates, the probes and about 25 APIs found by search. A Qt 6.7 API outside that set would not be caught.
- Plasma 6.3 point releases: only the 6.3.0 CMake files and Debian's libplasma 6.3.5 control were read. 6.3.1 to 6.3.6 were not checked individually.
- Gentoo: the 6.3.0 ebuilds were read from the GitHub history. The local Gentoo tree holds no 6.3.x ebuild.
- The floors of KWayland, plasma-activities and layer-shell-qt below 6.3 were not checked. The 6.3 floor does not depend on them.
- `CMakeLists.txt:88` says QTP0001 "exists since Qt 6.5". The file was not found at qtbase v6.6.0 (`cmake/QtPolicies.cmake` returned 404), so this was not verified.
- "LayerShellQt setScreen was later removed upstream" was not reproduced. No tag after 6.8 is in the mirror, and both v6.8.0 and the host's installed header still declare it.
- QTemporaryDir (section 3, row 8): the macro is used at the same line in v6.6.0 and v6.8.2, but its definition was not located, so Qt 6.6 support was not proven.
- The preview helper's failure below KF6 6.8 (`plasmoid/CMakeLists.txt:14`) is reasoned from CMake target-resolution rules. It was not reproduced.
- The Neon index is a snapshot from 2026-11-06 of what unpinned CI would install. It is not the version list of any past CI run. The CI logs were not inspected.
- The Nix job's nixpkgs revision was not examined.
- Git history before the 2026-05-03 reset (`8350a6b1c`) is not available. The origin of the 6.6 and 6.0 values is therefore unknown.

## 8. Method and sources

- `git grep` and `Read` on the worktree at `e18a0f9a7`. CodeGraph is not indexed here, so it was not used.
- Read-only fetches, direct or through the local proxy `http://127.0.0.1:10808`: invent.kde.org raw files at tags v6.0.0 to v6.3.0, v6.7.0, v6.8.0 and v6.10.0; GitHub mirror trees and raw files under `KDE/*` and `qt/*`; Gentoo ebuilds at the version-bump commits; sources.debian.org, packages.debian.org and packages.ubuntu.com pages; the `archive.neon.kde.org` noble user `Packages.gz`; and the Plasma 6.3.0 announcement.
- Scratch files were kept outside the repository: `/tmp/r7g-src` (fetched sources) and `/tmp/r7g-proposal` (generated diff).
- No Docker image was pulled, no Debian verification was run, and no `latte-dock-ng` process was started.
