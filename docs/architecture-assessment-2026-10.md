# Architecture, Performance and Code-Modernity Assessment: 2026-10-25

This is the evidence record for the
[R track](architecture-modernization-plan.md#assessment-refresh-r-track) of the
modernization plan. It records what the review observed and measured. It does
not authorize implementation, commits or releases by itself.

## Scope and method

- Source: `main` at `cb94d71c62e37f35e6b6a4a7dafd16a380142efb` (tag `v1.2.62`).
- Static review with CodeGraph, focused reads and pattern counts over tracked
  production sources (autotests excluded). The counts come from `grep` and are
  inventory figures, not lint results.
- Idle samples were read from `/proc` for a running dock on the developer
  desktop (Gentoo, Plasma 6.7.5, Wayland). The VM figures quoted below come from
  [P0](modernization-results/P0.md) and were not re-measured.
- No profiler session of interactive paths (parabolic hover, drag) was run.

## Summary

| Area | Assessment | Evidence |
| --- | --- | --- |
| Build, CI and tooling | Strong | Schema-v2 presets, strict `-Werror` gate with negative fixtures on GCC and Clang, targeted ASan/UBSan, deep qmllint baseline gate, distribution install matrix, Nix checks |
| Qt 6 / KF6 API idioms | Good | No `foreach`, `qAsConst`, `Q_DECL_*` or `Q_NULLPTR`; 3 of 792 connections use string signatures, each for a runtime-resolved signature; `QT_NO_KEYWORDS`, `QT_NO_CAST_FROM_ASCII`, `QT_STRICT_ITERATORS` and `QT_USE_QSTRINGBUILDER` are active |
| C++17/20 language features | Limited | No `std::optional`, `std::span`, `std::string_view`, `std::variant`, ranges or structured bindings in production code; 2 `enum class`; 62 hand-written copy/move member declarations in `app/data` whose move constructors copy (removed in R3) |
| Component boundaries | Moderate | `Corona` acts as a service locator; C++ reads and drives QML objects by name; the `org.kde.latte.private.app` plugin resolves symbols from the executable |
| QML modernity | Surface ported, typing incomplete | Unversioned imports, Plasma 6 roots, `MultiEffect` and `function onX()` handlers throughout; 6,048 `unqualified` and 1,144 `missing-property` diagnostics in the reviewed baseline |
| Runtime performance | Low idle CPU on hardware; avoidable wakeups | Before R1a: 0.27 % CPU, 16 main-thread and 8 D-Bus-thread wakeups per second at idle |
| Tests | Broad, shallow in places | `sourcecontracttest` asserts source text in several hundred cases; two QtQuickTest files; `parabolicmathtest` copies QML math |

## Architecture findings

- **Service locator.** 77 production files reference `Latte::Corona`; there are
  271 two-hop accessor chains such as `m_corona->layoutsManager()->synchronizer()`
  and 20 `qobject_cast<Latte::Corona *>`. Components cannot be constructed in
  isolation, which pushes tests towards source contracts.
- **Name-based C++/QML coupling.** 114 `property("…")` reads, 40
  `setProperty()` calls and 23 `invokeMethod()` calls by name; 61 of them are
  in [layoutmanager.cpp](../containment/plugin/layoutmanager.cpp). A renamed
  QML property fails silently at runtime and qmllint cannot see it.
- **Host-symbol plugin.** `latte-dock-ng` sets `ENABLE_EXPORTS` and default
  visibility, and [app/declarativeimports/CMakeLists.txt](../app/declarativeimports/CMakeLists.txt)
  strips `-Wl,--no-undefined` so `org.kde.latte.private.app` can resolve
  application symbols at load time.
- **Context properties.** 22 `setContextProperty()` calls in the settings views
  feed unqualified QML lookups. Replaced in [R4](modernization-results/R4.md)
  by root properties, except `indicator` and `dialog`, which third-party
  indicator configuration UIs read.
- **Startup and shutdown in `main()`.** The function spans about 680 lines of
  CLI handling, single-instance D-Bus, signal handling and session shutdown.
- **Compatibility shims.** `compat/` forwards KF5-era include paths through a
  global `include_directories(BEFORE compat)`, although the floors are KF6 6.0
  and Plasma 6.3.

Strengths to preserve: the isolated preview process with watchdog and fallback,
`View` composed of focused parts, the QML abilities interfaces
(definition/host/client/bridge), declarative registration through
`qt_add_qml_module`, and invariants documented beside the code.

## Performance findings

| Finding | Evidence | Status |
| --- | --- | --- |
| Session shutdown poller | A 500 ms timer ran for the whole session and made two blocking D-Bus calls per tick (`isShuttingDown`, `isServiceRegistered`) | Fixed in [R1a](modernization-results/R1a.md) |
| Original-icon fallback poll | A 100 ms timer per applet with "keep original icon colors" resolved the icon through C++ on every tick | Fixed in R1a |
| Unbounded quit-time query | `Corona::onAboutToQuit()` called ksmserver with the 25 s default timeout | Fixed in R1a |
| VM idle CPU | P0 measured about 20 % of one core and about 390 context switches per second at idle in the Fedora VM; hardware shows 0.27 % | Unexplained; R5 |
| Memory | 567 MiB RSS, 265 MiB anonymous after 58 minutes on hardware; P0 saw PSS rise during samples | Needs a long-run growth check |
| Heavy task delegates | Each task instantiates roughly 100+ QML objects and 300+ bindings; unqualified lookups keep most bindings out of compiled code | Strategic; R6 |
| Parabolic zoom | Scales propagate between neighbours in JavaScript (`sltUpdateItemScale` relay) | Strategic; R6 |

Measured idle wakeups before and after R1a (30 s windows, same desktop, HEAD
`v1.2.62` installed system binary vs the user-mode Debug build with R1a):

| Metric | Before | After |
| --- | ---: | ---: |
| Main-thread voluntary context switches per second | 16.0 | 0.16 |
| QDBusConnection thread voluntary context switches per second | 8.0 | 0 |
| CPU, percent of one core | 0.27 | below sampling resolution |

Already sound and not to be reworked without new evidence: window-tracker
debouncing and inactive-view filtering, lazy preview helper, OpenGL RHI choice
(matches plasmashell), and the absence of worker threads.

## C++ and Qt findings

- Hand-written copy and move members in `app/data` (62 declarations in 16
  classes; the first grep inventory reported 43) duplicated the defaults; the
  move constructors copied (no `std::move` in `app/data`), and a field added
  later could be silently dropped by one of them. Removed in
  [R3](modernization-results/R3.md), which adds a source contract against new
  ones.
- 24 connections had no context object (23 lambdas, 17 of them in `app/`, and
  one member-function connection), for example in
  [alternativeshelper.cpp](../app/alternativeshelper.cpp). Fixed in
  [R2b](modernization-results/R2b.md) and enforced by
  `QT_NO_CONTEXTLESS_CONNECT`; the desktop retest passed.
- No deprecation gate: `ecm_set_disabled_deprecation_versions()` is not used,
  so APIs deprecated at or below the Qt 6.6 / KF 6.0 floors are not compiled
  out. Fixed in [R2a](modernization-results/R2a.md).
- Raw pointers to QML-owned items without `QPointer` and a non-close-on-exec
  self-pipe. Fixed in R1a.
- A startup Wayland retry loop that could never succeed after `QApplication`
  construction. Removed in R1a.
- A quit-path use-after-free: `~Corona()` called `deleteLater()` on the view
  settings factory that `QCoreApplication::exec()` had already freed. Fixed in
  [R1b](modernization-results/R1b.md).
- A `const auto` in `BackgroundCache` held a `QStringBuilder` of temporaries,
  a dangling reference on Qt 6.6 and older; the clazy inventory found it. Fixed
  in [R2c-auto-unexpected-qstringbuilder](modernization-results/R2c-auto-unexpected-qstringbuilder.md).

## QML findings

- 720 uses of the lowercase `plasmoid.` context path against 60 uses of the
  Plasma 6 `Plasmoid` attached object.
- JavaScript uses `var` 1,570 times and `let`/`const` 44 times.
- `required property` appears 91 times and `pragma ComponentBehavior: Bound` in
  five files; most components still depend on outer ids.
- Plasma packages and pure QML modules are installed as loose files without
  qmlcachegen. The installed libplasma 6.7.5 provides `plasma_add_applet()` with
  `QML_SOURCES`; availability at the Plasma 6.3 floor is unverified.
- `Qt.createQmlObject()` compiles a timer from a string during applet startup
  retries in the containment `main.qml`.

## Build and test findings

- `.clang-tidy` enables two checks. clazy was not used in CI; since
  [R2c](modernization-results/R2c.md) the Clang Debug job gates the clazy
  checks that have zero findings and a fixture that fires. The inventory found
  570 findings from 19 checks; after the first slice and fix batch 1, 37 checks
  are gated and 488 findings from 12 checks wait as `R2c-<check>` slices.
- Application sources are collected through `PARENT_SCOPE` lists in about 30
  subdirectories; `configure_file()` copies `.cpp` files between targets; some
  directories use `add_definitions()`.
- `sourcecontracttest` locks implementation text rather than behaviour in many
  cases, which makes refactoring expensive. New behavioural tests from R1a
  (`shutdownmonitortest`) and R1b (`teardowntest`) show the preferred shape.

## Roadmap

The ordered slices, dependencies and acceptance gates are tracked in the
[R track ledger](architecture-modernization-plan.md#assessment-refresh-r-track).
