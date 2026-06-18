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
- source-level UI/runtime regression contracts
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

On `gentoo-bull`, use `--parallel 8` or `./install.sh --user --jobs 8`.

After tests pass, verify the regular user install path:

```bash
./install.sh --user --jobs 8
```

## Adding Tests

Prefer narrow tests that exercise production code directly. Use temporary directories for config, package, archive, and install-path tests so the suite never modifies the developer's real Latte or Plasma data.

When a production function writes to a standard location, isolate it with test-local stubs or environment-controlled paths. Do not copy artifacts into `/usr`, overwrite system Plasma files, or depend on a user's live desktop configuration.

For QML and plugin behavior, prefer smoke tests that load the built plugin or package structure from the build tree. These catch runtime discovery regressions that pure C++ tests miss.

If a test exposes a production bug, keep the regression test and make the smallest fix needed to satisfy the documented behavior.

## Architecture Debt Tracked Deliberately

Some Plasma 6 compatibility work is intentionally conservative because broad cleanup can regress user-visible shell behavior:

- `app/knscompat.cpp` still creates user-local QML overrides for the KNS dialog, but the source QML root must be resolved explicitly and the override can be disabled with `LATTE_DISABLE_KNS_COMPAT=1` for diagnosis.
- QML smoke tests may use source-level regression locks when a behavior depends on a live Plasma shell, KWin, or third-party applet. Prefer real `QQmlComponent` tests when practical, but keep source locks for previously fixed regressions that are hard to exercise headlessly.
- Broad QML import modernization should be done only in touched files. Do not churn all `QtQuick 2.x` imports in one pass.
- Private Plasma imports and `Latte::Corona` shutdown ordering are known maintenance risks. Change them only with focused runtime reproduction and GCC/Clang test coverage.

## Coverage Estimate

The project currently tracks a coarse file-level coverage estimate: count production `.cpp` files referenced by autotest targets, plus runtime smoke targets that load production plugin entry points, then divide by all tracked production `.cpp` files.

Use this quick estimate from the repository root:

```bash
python3 - <<'PY'
import pathlib, re, subprocess

all_cpp = subprocess.check_output(["git", "ls-files", "*.cpp"], text=True).splitlines()
prod_cpp = [p for p in all_cpp if not p.startswith("autotests/")]
cmake = pathlib.Path("autotests/CMakeLists.txt").read_text()

covered = set()
for match in re.finditer(r"\.\./([^\s)]+\.cpp)", cmake):
    path = match.group(1)
    if pathlib.Path(path).exists():
        covered.add(path)

if pathlib.Path("declarativeimports/core/lattecoreplugin.cpp").exists():
    covered.add("declarativeimports/core/lattecoreplugin.cpp")

print(f"{len(covered)}/{len(prod_cpp)} = {len(covered) / len(prod_cpp) * 100:.1f}%")
PY
```

Report this estimate after each test commit. It is not a line or branch coverage metric, but it is useful for tracking which production compilation units now have direct regression coverage.
