# M2a: Strict compiler-warning gate

Date: 2026-09-30

Branch: `codex/modernization-m0-baseline`

Status: Pending validation until the pushed feature-branch CI matrix passes.
The complete local acceptance matrix passed.

Source baseline: `e3ef1ddf001aa032cffa62db21cfd97196174746`.
M2a start HEAD: `4c6fa1ca4`.

## Implementation

Added `LATTE_STRICT_WARNINGS`, enabled by default and explicitly enabled in all
four CMake presets. The project applies the gate after subdirectories have
created their targets, so first-party application, plugin, helper, logging and
`EXCLUDE_FROM_ALL` autotest executables receive it. Imported targets,
interface-only targets and utility targets are excluded. CMake 3.24 and newer
use the target `COMPILE_WARNING_AS_ERROR` property; older versions receive a
target-scoped GCC/Clang `-Werror` option. Disabling the option prints a status
message that the build does not satisfy the policy; it is a downstream
compatibility escape hatch only.

The workflow runs a disposable negative fixture in each GCC/Clang Debug/Release
job. Its intentional unused-variable warning must fail, then its corrected
source must build successfully. No production source behavior changed.

## Validation

- Host toolchains: GCC 16.2.1, Clang 22.1.8, CMake 4.3.4, Qt 6.10/KF 6.29.
  Fresh Debug and Release configure/builds passed for both compilers. All 53
  first-party compile targets were listed and every target present in the
  compilation databases had `-Werror`. Compiler-warning scans were empty.
- CTest passed 44/44 for all four host compiler/mode combinations. The one
  registration difference from the Debian 13 environment is `appstreamtest`,
  provided by that host's newer ECM `KDECMakeSettings`; it passed on the host.
- Debian 13 container: CMake 3.20.6, GCC 14.2, Qt 6.8.2, KF 6.13 and Plasma
  6.3.6. The older-CMake `-Werror` path configured and built the application
  plus autotests with zero compiler warnings; CTest passed 43/43. This CMake
  3.20 registration did not include host-only `appstreamtest`.
- The negative fixture failed as expected and its corrected version passed for
  GCC Debug/Release and Clang Debug/Release on the host. GCC Debug/Release
  passed again under CMake 3.20.6.
- `LATTE_STRICT_WARNINGS=OFF` configured successfully and printed the explicit
  downstream-only status message.
- `git diff --check` — passed.
- The first integrated CI run [36632244809](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/36632244809) stopped all build jobs during dependency installation because Ubuntu/Neon does not publish the Debian-specific `qt6-svg-plugins` name; the Ubuntu dependency is corrected to `libqt6svg6`. Strict-warning CI after that package fix — Pending validation.

One new GCC 16 diagnostic was found in Qt 6's intentional incomplete-type
SFINAE probe while the generated `latte-dock-ng` moc aggregation includes
`Latte::View`. It is handled in the separate M2b-GCC16 slice and documented
beside the narrowly scoped generated-file option. QML diagnostics remain a
separate lint category and were not treated as compiler warnings.

## Commands

Host fresh builds used:

```bash
cmake -S . -B /tmp/latte-m2a-final/<preset> \
  -DCMAKE_C_COMPILER=<gcc-or-clang> -DCMAKE_CXX_COMPILER=<g++-or-clang++> \
  -DCMAKE_BUILD_TYPE=<Debug-or-Release> -DBUILD_TESTING=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DLATTE_STRICT_WARNINGS=ON
cmake --build /tmp/latte-m2a-final/<preset> --parallel 8
cmake --build /tmp/latte-m2a-final/<preset> --target latte-autotests --parallel 8
ctest --test-dir /tmp/latte-m2a-final/<preset> --output-on-failure
python3 scripts/test-strict-warnings.py --compiler <g++-or-clang++> --build-type <Debug-or-Release>
```

Logs are in `/tmp/latte-m2a-final-*`; the Debian 13 CMake 3.20.6 run is in
`/tmp/latte-m2a-logs/cmake320-{configure,build,tests-build,ctest}.log`.

## Handoff

The strict-warning CI run including M3b's application-host smoke is in progress.
M4a's local static-analysis slice is now prepared independently while that
remote result is pending.

Next action: record the current CI run; if it passes, mark M2a and M3b Complete,
then push M4a for its own remote validation. The user authorized automatic
commits and pushes on this branch only.
