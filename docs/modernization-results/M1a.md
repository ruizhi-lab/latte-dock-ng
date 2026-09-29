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

The same CMake 3.20.6 build ran 43 CTest targets: 42 passed and `coreunittest`
failed two icon-source assertions because the minimal build image has no icon
theme assets. This is an environment limitation, not a CMake configure/build
failure; keep that test result visible until the test image supplies its
required icons. The [official CMake 3.20 preset documentation](https://cmake.org/cmake/help/v3.20/manual/cmake-presets.7.html)
also confirms the schema-v2 fields used by the presets.

The oldest supported distro acceptance target is Debian 13.7's packaged stack:
CMake 3.31.6-2, Qt 6.8.2, KF 6.13 and Plasma desktop 6.3.6. Debian ships
`libplasma-dev` 6.3.5-1 and `plasma-workspace-dev` 4:6.3.6-2; the local
Debian 13 container passed both package-floor checks and the complete
system/user install-uninstall verifier. The feature-branch CI rerun remains
pending. Gentoo, Arch, Fedora, openSUSE, Mageia, Ubuntu and NixOS forward-stack
checks are recorded in M3b.

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

Next action: push the corrected CI and package-verification slice, then record
the fresh workflow result. Keep the missing-icon CTest failure visible; do not
claim that every CMake 3.20 test passed. The user authorized automatic commits
and pushes on this branch; never push another branch or a tag.
