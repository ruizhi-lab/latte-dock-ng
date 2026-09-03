# Development Testing Guide

This project uses automated tests as a regression safety net for core Latte Dock NG behavior. New fixes and feature changes should add focused tests for the behavior they touch, especially when the code handles layout data, indicator packages, QML plugins, window metadata, settings models, or import/export paths.

## Test Targets

Autotests live under `autotests/` and are built only when `BUILD_TESTING=ON`.

The current suite covers:

- data containers and table behavior
- settings and screen models
- declarative core helper objects
- QML plugin loading for `org.kde.latte.core`
- enum and task plugin guard behavior
- package structure and bundled indicator package resolution
- indicator metadata and archive import paths
- abstract layout configuration behavior
- scheme color parsing
- window-system helper logic
- selected settings delegates and widgets
- source-level UI/runtime regression contracts (170+ checks across the GCC
  and Clang suites) covering widget-specific special handling: digital clock
  sizing, systray guards, volume/appmenu popups, separator/spacer behavior,
  drag-and-drop, and scroll/wheel actions
- install, uninstall, Docker, and packaging contracts

Test executables are intentionally marked `EXCLUDE_FROM_ALL` so normal application builds are not slowed by test-only targets.

## Required Verification

Both GCC and Clang builds must remain error-free. Use separate build directories so compiler configuration and generated files do not contaminate each other:

```bash
cmake -S . -B build-autotests-gcc -DBUILD_TESTING=ON
cmake --build build-autotests-gcc --target latte-autotests --parallel 8
ctest --test-dir build-autotests-gcc --output-on-failure

CC=clang CXX=clang++ cmake -S . -B build-autotests-clang -DBUILD_TESTING=ON
cmake --build build-autotests-clang --target latte-autotests --parallel 8
ctest --test-dir build-autotests-clang --output-on-failure
```

Tune the parallelism to the host's core count; keep it below the core count so the build does not starve the desktop.

After tests pass, verify the regular user install path:

```bash
./install.sh --user --jobs 8
```

## Runtime Retest Workflow

Automated tests cannot reproduce shell-integration bugs (window lifecycle,
shutdown teardown, Wayland focus, compositor interaction). Before committing a
risky fix, retest the live dock with the user-mode Debug build and verify a
clean quit:

1. Install the modified code for the current user (Debug keeps symbols for
   later core analysis):

   ```bash
   ./install.sh --user Debug
   ```

2. Stop the running dock and launch the user-mode binary with a fresh debug
   log from a terminal. Do not use `pkill -f` on a command line that also
   contains "latte-dock-ng" (it matches the shell itself); use the exact
   process name:

   ```bash
   pkill -x latte-dock-ng || true
   rm -f /tmp/latte-ng.log
   source ~/.config/latte-dock-ng/dev-env.sh
   nohup ~/.local/bin/latte-dock-ng --replace --debug > /tmp/latte-ng.log 2>&1 &
   ```

3. Exercise the changed behavior, then drive the exact scenario the fix
   targets (dock restart, applet popups and submenus, KDE logout or reboot).

4. Verify a clean teardown instead of trusting the exit status:
   - `coredumpctl list` must not gain a `latte-dock-ng` entry. Record the
     newest entry id before the test as a baseline and compare afterwards.
   - The debug log must not contain `Fatal`, `ASSERT`, `Segmentation` or
     `KCrash`, and it should end at the expected teardown markers (for a full
     quit: `Latte Corona - deleted...`, `QuickWindowSystem destructed`).
   - On a signal-driven quit, the log should show the shutdown handler path
     (`KSignalHandler received signal 15` -> `calling quit()`).

5. Analyze the log for new warnings/errors and file follow-ups before the
   next risky change.

For crash fixes, run an A/B check against the previous binary: reproduce the
crash with the old build and confirm a fresh core, rebuild with the fix,
repeat the scenario, and confirm no new core is produced.

## Adding Tests

Prefer narrow tests that exercise production code directly. Use temporary directories for config, package, archive, and install-path tests so the suite never modifies the developer's real Latte or Plasma data.

When a production function writes to a standard location, isolate it with test-local stubs or environment-controlled paths. Do not copy artifacts into `/usr`, overwrite system Plasma files, or depend on a user's live desktop configuration.

For QML and plugin behavior, prefer smoke tests that load the built plugin or package structure from the build tree. These catch runtime discovery regressions that pure C++ tests miss.

If a test exposes a production bug, keep the regression test and make the smallest fix needed to satisfy the documented behavior.

## Architecture Debt Tracked Deliberately

Some Plasma 6 compatibility work is intentionally conservative because broad cleanup can regress user-visible shell behavior:

- `app/knscompat.cpp` still creates user-local QML overrides for the KNS dialog, but the source QML root must be resolved explicitly and the override can be disabled with `LATTE_DISABLE_KNS_COMPAT=1` for diagnosis. Use `LATTE_KNS_COMPAT_SYSTEM_QML_ROOTS` and `LATTE_KNS_COMPAT_USER_QML_ROOT` for isolated cross-distro path tests instead of touching the real user QML tree.
- QML smoke tests may use source-level regression locks when a behavior depends on a live Plasma shell, KWin, or third-party applet. Prefer real `QQmlComponent` tests when practical, but keep source locks for previously fixed regressions that are hard to exercise headlessly.
- CMake helper modules keep target resolution, compiler warning relaxation, and packaging metadata out of the top-level build file.
- Broad QML import modernization should be done only in touched files. Do not churn all `QtQuick 2.x` imports in one pass.
- Warning cleanup should remove one relaxed compiler flag at a time, with GCC and Clang autotests passing before the next flag is tightened.
- Private Plasma imports and `Latte::Corona` shutdown ordering are known maintenance risks. Change them only with focused runtime reproduction and GCC/Clang test coverage.

## Coverage Estimate

The project currently tracks a coarse file-level coverage estimate: count production `.cpp` files referenced by autotest targets, plus runtime smoke targets that load production plugin entry points, then divide by all tracked production `.cpp` files.

Use this quick estimate from the repository root:

```bash
python3 autotests/coverageestimate.py
```

Report this estimate after each test commit. It is not a line or branch coverage metric, but it is useful for tracking which production compilation units now have direct regression coverage.
