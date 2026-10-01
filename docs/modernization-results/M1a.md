# M1a: Reproducible CMake presets

Date: 2026-09-29

Branch: `codex/modernization-m0-baseline`

Starting revision: `407e9528af8a61302502ba2b6a76e437e69a6654`

Scope: shared configure/build/test presets, local preset ignore rule, and
developer testing instructions. No CI, dependency, install, or production code
changed.

## Implementation

Added four CMake schema-v2 presets: `gcc-debug`, `gcc-release`, `clang-debug`
and `clang-release`. Each sets explicit C and C++ compiler names, a distinct
`build/modernization/<preset>` directory, build type, `BUILD_TESTING=ON`, and
`CMAKE_EXPORT_COMPILE_COMMANDS=ON`. The explicit `Unix Makefiles` generator is
already used by the M0 baseline. Build presets build both `all` and
`latte-autotests` with eight jobs. Test presets enable output-on-failure and
fail when CTest finds no tests. `CMakeUserPresets.json` is ignored. `.clangd`
continues to select the existing `build` compilation database; preset database
selection remains a local editor preference.

## Validation

- `cmake --version`: 4.3.4. `cmake --list-presets`: listed all four configure
  presets successfully.
- `python3 -m json.tool CMakePresets.json`: passed.
- Each preset configured in a clean, independent directory, then
  `cmake --build --preset <name>` built the application/plugin targets and
  `latte-autotests`; all four commands passed. Full logs are retained locally
  under `build-modernization-baseline/logs/<name>-preset-{configure,build}.log`.
- `ctest --preset <name>` passed 43/43 in every preset directory. The suite
  required host permission because the restricted sandbox prevents creation of
  the isolated D-Bus socket used by `windowviewbackendtest`. Logs are
  `build-modernization-baseline/logs/<name>-fresh-ctest.log`.
- CTest names matched the M0 baseline in all four fresh preset directories:
  43 each, sorted-name SHA-256
  `5a379c7ee7b6c098981b0a45acfd72792bd9e69e8f5e27374b1c9d4c0f703558`.
  Required test binaries were built and all tests executed.
- `git check-ignore CMakeUserPresets.json`: confirmed ignored.
- `git diff --check`: passed.

The official CMake 3.20.6 binary was used to validate the minimum on Debian 13.
`cmake --list-presets`, `cmake --preset gcc-debug`, and
`cmake --build --preset gcc-debug` all passed with Debian's Qt 6.8.2, KF 6.13
and Plasma 6.3.6 development stack. The first configure exposed that Debian
exports the KWayland target as `Plasma::KWaylandClient`; adding that explicit
candidate lets older CMake resolve it without relying on imported-target
enumeration. A source contract protects the candidate list.

The first minimal-image CTest run exposed two `coreunittest` assertions that
depend on named theme icons. The test now explicitly selects the standard
Breeze theme and theme search path, while Debian/Ubuntu test images and CI
install Breeze plus Qt's SVG icon engine (`qt6-svg-plugins` on Debian and on
Ubuntu 26.04; `libqt6svg6` provides it on Ubuntu 24.04/Neon). The Ubuntu 26.04
developer image installs both its SVG library and plugin package, and Debian
testing carries the equivalent package. Debian/Ubuntu build images also install
`qt6-declarative-private-dev`; without the package GCC reports missing
Qt private include directories exported by Debian's Qt development packages.
With those deterministic test dependencies, the CMake 3.20.6 Debian 13
configure and build passed with zero compiler warnings/errors, and all 43 CTest
targets passed. The [official CMake 3.20 preset documentation](https://cmake.org/cmake/help/v3.20/manual/cmake-presets.7.html)
also confirms the schema-v2 fields used by the presets.

The oldest supported distro acceptance target is Debian 13.7's packaged stack:
CMake 3.31.6-2, Qt 6.8.2, KF 6.13 and Plasma desktop 6.3.6. Debian ships
`libplasma-dev` 6.3.5-1 and `plasma-workspace-dev` 4:6.3.6-2; the local
Debian 13 container passed both package-floor checks and the complete
system/user install-uninstall verifier. Feature-branch CI run
[36623951814](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/36623951814)
passed all four build/test/lint presets and every distro install/package job.
Gentoo, Arch, Fedora, openSUSE, Mageia, Ubuntu and NixOS forward-stack checks
are recorded in M3b.

## Exact commands

From the repository root:

```bash
cmake --list-presets
cmake --preset gcc-debug
cmake --build --preset gcc-debug
ctest --preset gcc-debug
```

Repeat the configure, build, and test commands with `gcc-release`, `clang-debug`
and `clang-release`. Registration comparison used:

```bash
ctest --test-dir "build/modernization/<preset>" --show-only=json-v1
```

## Handoff

Next action: proceed to M2a. CMake 3.20.6 Debian configure, build and all 43
CTest targets now pass. The user authorized automatic commits and pushes on
this branch; never push another branch or a tag.
