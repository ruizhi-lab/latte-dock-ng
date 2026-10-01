# M4b: ASan/UBSan configuration

Status: Pending validation.

## Scope and design

This slice adds an opt-in `gcc-asan-ubsan` preset with a dedicated build tree.
When enabled, target-scoped flags instrument first-party C++ targets, shared
plugins, executables, and helper binaries. Normal presets and install rules do
not enable sanitizers. The focused preset builds `latte-dock-ng` with its QML
plugins plus `dataunittest`, `coreunittest`, and `previewprocessunittest`; it
runs those tests and `privateapphostsmoketest`. The preview test's fake helper
is instrumented as a dependency without changing the production process
boundary.

The fault fixture checks that an ASan heap-buffer-overflow and UBSan signed
integer overflow produce their expected diagnostics. External Qt/KDE libraries
are not rebuilt or instrumented by this configuration.

## Validation

Passed locally:

- `cmake --list-presets`, `cmake --preset gcc-asan-ubsan` — exit 0; CMake
  4.3.4/GCC 16.2.1 configured 53 first-party compile targets.
- Debian 13.7 container — CMake `3.31.6-2`, Qt `6.8.2`, KF `6.13.0`, Plasma
  workspace `6.3.6`; `cmake --list-presets=test` and a fresh strict ASan/UBSan
  configure both exited 0 and registered 53 sanitizer targets. This was a
  configure-only compatibility check; compiler validation remains on host CI.
- `cmake --build --preset gcc-asan-ubsan` — exit 0 after expanding the target
  set to `latte-dock-ng`, its required plugins, three selected QtTest
  executables and the instrumented preview fake helper. C++ compilation emitted
  no compiler warnings; the normal build-time QML lint printed its existing
  baseline warnings.
- `ctest --preset gcc-asan-ubsan` on the local Gentoo host — exit 8 after all
  QtTest assertions passed; each process then hit the host's LeakSanitizer
  ptrace limitation.
- Refreshed Ubuntu 26.04 container, after adding `dbus-run-session` to the
  disposable container: all four selected tests passed, including the host
  smoke, with LeakSanitizer active. The host smoke initially exposed ASan's ODR
  report for the intentionally shared `Interfaces::staticMetaObject` compiled
  into both the exporting host and `private.app` plugin. Disabling only
  `detect_odr_violation` for this one sanitizer smoke process resolved that
  expected DSO duplicate; ASan address/leak checks and UBSan remain enabled.
- `python3 scripts/test-sanitizers.py --compiler g++` and
  `python3 scripts/test-sanitizers.py --compiler clang++` — both exit 0; ASan
  reported `heap-buffer-overflow`, UBSan reported signed integer overflow. An initial
  fixture used a stack array and UBSan intercepted it first; the fixture was
  changed to a heap allocation to exercise ASan specifically.
- `git diff --check` and `python3 -m py_compile scripts/test-sanitizers.py` —
  exit 0.

Pending:

- CI sanitizer execution of the current slice is pending. CI must pass with
  leak detection enabled before completion.

Runtime shutdown findings still require the documented desktop path; this
configuration has not been installed or launched as the user-mode dock.

## Handoff

Push the implementation, then record the sanitizer preset's CI result. Keep
leak detection enabled; the only runtime allowance is the documented ODR
subcheck for this host/plugin smoke.
