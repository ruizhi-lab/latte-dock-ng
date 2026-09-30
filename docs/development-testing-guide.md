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
- KWin WindowView D-Bus group selection, unavailable-effect fallback and retry
  behavior (`windowviewbackendtest`, isolated with `dbus-run-session`)
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

The shared schema-v2 presets provide four equivalent configurations. They keep
their compile databases in separate directories and build both the application
and `latte-autotests` with eight jobs. `.clangd` continues to select the
existing `build` database; selecting a preset database is a local editor choice
and should not replace or symlink the shared default.

```bash
cmake --list-presets
cmake --preset gcc-debug
cmake --build --preset gcc-debug
ctest --preset gcc-debug
```

Use `gcc-release`, `clang-debug` or `clang-release` to select the other
compiler/configuration. `CMakeUserPresets.json` is intentionally ignored for
machine-local choices. The project presets require CMake 3.20 schema-v2
support. Keep CMake 3.20 as the source compatibility floor. The Debian 13.7
container separately verifies its packaged minimum stack: CMake 3.31.6, Qt
6.8.2, KDE Frameworks 6.13 and Plasma 6.3.6. Gentoo, Arch and Fedora checks
record their current stable versions as forward-compatibility evidence.

The initial clang-tidy gate selects two bug-prone checks for the small
`app/data/errordata.cpp` component. Run it from a Clang Debug compile database;
the disposable fixture confirms an ignored `std::string::empty()` result is
reported as an error.

```bash
python3 scripts/test-clang-tidy.py --build-dir build/modernization/clang-debug
```

## Deep QML lint and baseline review

The syntax-only check and the build-aware deep check serve different purposes.
The latter records raw JSON chunks, stderr, complete file coverage and process
exit codes under the selected build directory. It checks each generated Latte
`qmldir` against its declared plugin library and typeinfo file, and compares
diagnostics with the reviewed baseline when fingerprints match.

```bash
bash scripts/qmllint.sh
bash scripts/qmllint-deep.sh build/modernization/gcc-debug
python3 scripts/qmllint-baseline.py compare \
    --baseline docs/qmllint-baseline.json \
    --current build/modernization/gcc-debug/qmllint-baseline/current.json
```

The full category inventory, dynamic-interface exceptions, promotion criteria
and review workflow are in [qmllint-backlog-plan.md](qmllint-backlog-plan.md).
Do not copy a current report over the baseline to clear a failed comparison.
Inspect the environment fingerprint and additions/removals first. Standalone
lint cannot validate `org.kde.latte.private.app`, whose plugin resolves symbols
from the Latte application host; its metadata and build artifacts are checked
separately, and host behavior requires an application-level smoke/retest.

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

## Present Windows Retest (Wayland)

After a user-mode Debug install, open two windows of the same application and
set its Latte task action to **Present Windows**. Clicking the group should
display KWin's window selector, not cycle focus. Check both the visible result
and `org.kde.kwin.Effects.activeEffects` on `/Effects`: an accepted D-Bus reply
alone does not prove that KWin displayed any windows.

Also verify that a minimized group member is included, a single real window
still activates normally, and temporarily unloading `windowview` causes group
cycling. Restore the effect immediately; the next click should present again
without restarting Latte. The backend test covers unavailable interfaces on a
private bus, while the QML tests protect click routing and phantom filtering.
Hover-thumbnail previews remain disabled and are outside this test's scope.

## Remote VM GUI Testing

An SSH shell does not inherit the active Plasma GUI session. Launching Latte
from that shell without the session's display and D-Bus variables can make the
isolated preview helper fail to connect to the display; the helper failure
fallback then disables previews after repeated attempts. Read the environment
from the active Plasma user service before a remote GUI test:

```bash
systemctl --user show-environment
```

Pass through the reported `DISPLAY`, `WAYLAND_DISPLAY`, `XDG_SESSION_TYPE`,
`XDG_RUNTIME_DIR`, `DBUS_SESSION_BUS_ADDRESS` and, when present, `XAUTHORITY`.
For a development Debug install, also source the generated
`~/.config/latte-dock-ng/dev-env.sh` in the launch script. Do not construct
`QML_IMPORT_PATH`, `QML2_IMPORT_PATH` or `XDG_DATA_DIRS` by hand: the user
installer selects the QML and plugin roots for its install prefix. Verify the
launch log and the user-mode executable path before judging shell behavior.

Use the standard per-user install path for both Debug retests and Release
measurements. Debug uses `bash install.sh --user Debug`; Release measurement
builds can use `bash install.sh --user Release --jobs 8 --no-clean`. Launch
`~/.local/bin/latte-dock-ng` from the captured Plasma session. The Debug
developer environment is generated for local module overrides; Release's
installed binary detects its own QML prefix. Do not launch a separate `/tmp`
prefix for compositor integration tests: Fedora 44 KWin denied the
PlasmaWindowManagement protocol to a directly launched `/tmp` binary even
though the process started, which invalidated that run. KWin matches privileged
Plasma interfaces against the first executable token in the registered desktop
entry; the temporary prefix did not match it.

For detached SSH launches, save the session variables in a small launcher,
source `dev-env.sh` only for Debug, and use `setsid -f nohup` so the VM process
survives the command session. Start with a fresh log and inspect it after the
test. Check the exact executable with `/proc/<pid>/exe`; `pgrep -x
latte-dock-ng` is safe for identifying the Dock. Never use `pkill -f` in a
command that also contains `latte-dock-ng`.

Wayland pointer injection needs a different path from an SSH X11 command.
Fedora 44's `ydotoold` was inactive and `/dev/uinput` was root-only. A
temporary daemon can be started with a user-owned socket, then used for
relative pointer movement:

```bash
vm_uid="$(id -u)"
vm_gid="$(id -g)"
vm_socket="/run/user/${vm_uid}/ydotool-latte-test.sock"
sudo -n setsid -f nohup /usr/bin/ydotoold \
  --socket-path="$vm_socket" --socket-perm=0660 \
  --socket-own="${vm_uid}:${vm_gid}" >/tmp/ydotoold-latte-test.log 2>&1
until test -S "$vm_socket"; do sleep 1; done
sleep 2  # Let udev and the compositor discover the virtual input device.
export YDOTOOL_SOCKET="$vm_socket"
ydotool mousemove -x 10 -y 0
```

Fedora 44 `ydotool` 1.0.4 produced matching `libinput debug-events` pointer
deltas for relative moves. In that environment, `mousemove --absolute` also
appeared as a relative event, and `xdotool getmouselocation` did not reflect
the injected movement during the Wayland check. Do not treat an exit code or
XWayland cursor query as proof that a Latte hover action fired; verify the
actual preview/highlight behavior or helper lifecycle. `mousemove -x/-y`
accepts relative deltas, so an absolute target still needs a trustworthy
starting coordinate. Stop only the temporary `/usr/bin/ydotoold` process whose
arguments contain this test's custom socket path, then remove that socket. Do
not leave a privileged input daemon running between tests.

A Fedora hover sweep with this input harness did not start the preview helper,
even though `libinput` observed the injected motion. The task icon had not been
identified, so do not infer either a working or broken Latte hover path from a
pointer command alone. Establish a known pointer origin and task-icon target,
then verify the helper process or visible hover effect before collecting a
hover performance sample. GUI utilities such as Spectacle also need the
captured Plasma session variables; an SSH-launched Spectacle help probe without
them aborted instead of producing a diagnostic.

When visual inspection of the VM is authorized, a screenshot with the pointer
included can establish the current pointer origin and distinguish a launcher
from a running task icon. Start Spectacle with the captured Plasma session
environment, for example:

```bash
while IFS= read -r line; do export "$line"; done \
  < <(systemctl --user show-environment | grep -E '^(DISPLAY|WAYLAND_DISPLAY|XDG_SESSION_TYPE|XDG_RUNTIME_DIR|DBUS_SESSION_BUS_ADDRESS|XAUTHORITY)=')
spectacle --background --nonotify --pointer --output /tmp/latte-vm-hover.png
```

Transfer the temporary image only when authorized and remove it after
inspection. Re-capture after relative pointer moves: observed `ydotool` motion
did not always land at the arithmetic position inferred from the requested
deltas. The captured cursor over an icon is useful targeting evidence, but
preview/helper or visible highlight evidence is still required to confirm the
hover action.

For Fedora 44's 1920x1080 VM, passing pixel coordinates to
`ydotool mousemove --absolute -x/-y` did not target the corresponding pixel;
the screenshot cursor remained at the upper-left corner. A large 0–65535-style
coordinate attempt and a relative move from an unknown origin also failed to
establish a target. Do not guess the absolute-coordinate scale or use a
successful command exit as input evidence. Capture the pointer before and
after each calibration move and verify its visible position before a hover
test. Also, temporary QML source probes can be masked by the compiled cache:
the Dock clears `~/.cache/lattedock/qmlcache` only when its
`VERSION-QMLCACHEREVISION` marker changes. To test an edited installed QML file,
preserve the cache reversibly, force a fresh compile in an isolated test, then
restore both the source and cache before the ordinary Dock launch. A missing
probe log without this check does not prove that the QML event was not fired.

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
# Sanitizer checks

The opt-in `gcc-asan-ubsan` preset uses a separate build directory and applies
AddressSanitizer and UndefinedBehaviorSanitizer to first-party C++ targets,
including project plugins and helper binaries. It leaves normal Debug/Release
and install presets unchanged. The focused preset builds the application host
plus `dataunittest`, `coreunittest`, and `previewprocessunittest`; it runs those
tests and the offscreen `privateapphostsmoketest`. This compiles and links the
application plugins under sanitizers, and instruments the fake preview helper
through the test target dependency without changing the production process
boundary. The private.app host smoke disables only ASan's ODR detector for
the `Interfaces` meta-object intentionally compiled into both host and plugin;
address, leak and undefined-behavior checks stay enabled.

Run the focused local check with:

```bash
cmake --preset gcc-asan-ubsan
cmake --build --preset gcc-asan-ubsan
ctest --preset gcc-asan-ubsan
python3 scripts/test-sanitizers.py --compiler g++
```

The fixture runner deliberately triggers a heap buffer overflow and signed
integer overflow and requires the corresponding sanitizer diagnostics. Do not
install or launch this instrumented build as the user-mode dock. Record any
third-party findings with the exact target and stack before considering a
narrow suppression.

# Nix development and tests

`nix develop` exposes the release derivation's build dependencies plus GCC,
Clang, Make, D-Bus and Python for local verification. Its preset build remains
an ordinary Debug/Release build. `nix flake check --print-build-logs` builds a
separate `checks.x86_64-linux.autotests` derivation, builds the application
host and test executables, then runs CTest with software Qt rendering and a
private session bus. `nix build .#default --no-link --print-build-logs`
validates the release package independently; the check-only derivation is not
installed into the release output.

The main commands are:

```bash
nix flake check --print-build-logs
nix build .#default --no-link --print-build-logs
nix develop
cmake --preset gcc-debug
cmake --build --preset gcc-debug
```
