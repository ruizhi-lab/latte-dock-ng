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

The minimum supported executable, CMake 3.20, is not installed in this
environment; only 4.3.4 is available. The [official CMake 3.20 preset
documentation](https://cmake.org/cmake/help/v3.20/manual/cmake-presets.7.html)
defines schema version 2 and the configure/build/test fields used here, but
that specification review does not replace an actual 3.20 parser/configure
run. Keep M1a **Pending validation** for that check; do not report minimum
version compatibility as tested.

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

Next action: implement **M1b only**. Update the GCC/Clang jobs in
`.github/workflows/build.yml` to use the corresponding configure/build/test
presets and eight build jobs. Preserve syntax/deep QML lint and distro/package
checks, validate workflow syntax and job-to-preset mapping, then record remote
CI as Pending validation until the affected jobs run. M1b can use the local
M1a evidence; keep the CMake 3.20 execution item open until a 3.20 binary is
available. The user authorized automatic commits and pushes on this branch;
never push another branch or a tag.
