# T4d: gate `performance-unnecessary-copy-initialization`

Source baseline: `main` at `1d36b229bdda2f4cde70eeb71d17cbf0e42559a8`.

R8c's clang-tidy 18.1.3 inventory recorded two findings, and clang-tidy 22.1.8 recorded those plus a third. The current source still contained all three:

| Source location | Audit | Change |
|---|---|---|
| `app/settings/settingsdialog/delegates/backgrounddelegate.cpp:32` | `myOptions` was never read or modified; the draw calls use `option` directly. | Removed the unused local copy. |
| `app/settings/viewsdialog/delegates/custommenuitemwidget.cpp:83` | `screenOpt` was passed only to `Latte::screenMaxLength(const QStyleOption &, ...)`, which reads `option.rect.height()` and does not mutate the option. | Pass `opt` directly. |
| `app/settings/settingsdialog/layoutscontroller.cpp:1099` | `layoutCurrentData` was read only for `isLocked` and `id`. This finding appeared in the R8c clang-tidy 22 inventory. | Bind a const reference through `alteredLayouts.at(i)`. |

The `alteredLayouts` list itself remains a by-value Apply-time snapshot. `Layouts::alteredLayouts()` builds it from the current rows, while the remainder of Apply may rewrite the live model's ids. The later lock step must read the original snapshot id and map it through `newFileForOldId`, preserving the behavior that locks a successfully renamed file at its final path. The const `at()` accessor avoids detaching the snapshot's implicitly shared list. Existing Layouts Apply source-contract checks and `storagelogictest::renamedLockedLayoutIsLockedAtItsNewPath` cover the lock target and ordering. `ToolsUnitTest::screenMaxLengthUsesOddAspectLength` covers the read-only geometry helper.

Added `performance-unnecessary-copy-initialization` as the 50th named check in `.clang-tidy` and `GATED_CHECKS`, with a negative fixture that copies a `std::string` from a const reference and then reads the copy. The fixture is deliberately defective and is not production code.

## Focused validation

| Tool | Fixture proof | Three changed production sources |
|---|---|---|
| clang-tidy 22.1.8 | Rejected by `performance-unnecessary-copy-initialization` with `WarningsAsErrors: '*'`. | Exit 0; no findings from the selected check. |
| clang-tidy 18.1.3 | Rejected by `performance-unnecessary-copy-initialization` with `WarningsAsErrors: '*'`. | Exit 0; no findings from the selected check. |

The focused production scans used `build/modernization/clang-debug/compile_commands.json`. The fixture proof used the named check and `-std=c++20`; it proves the fixture's diagnostic, while the coordinator's complete production gate uses the repository runner and production compile flags. The gate configuration and fixture registry were checked: all 50 names match exactly. `git diff --check` passed.

## Coordinator acceptance

All four GCC/Clang Debug/Release builds completed with zero compiler warnings
and passed 64/64 tests each in an isolated D-Bus/offscreen environment. Test
registration stayed at 64; no test or prior check was removed.

The full repository runners on clang-tidy 22.1.8 and isolated 18.1.3 rejected
all 50 fixtures under the production compile flags and reported zero findings
across 156 production sources. Timings with four workers were 73.036 seconds
and 105.133 seconds respectively. Focused clazy 1.17 / LLVM 19.1.1 analysis of
all three changed production sources also had no findings or analysis errors.
[Machine-readable build/test results](T4d-data/validation-summary.json) and
[full gate output](T4d-data/tidy18-final.log) retain the evidence.

No dependency, minimum-version, QML, ownership or persisted-config contract
changed. The call paths retain the same rendering inputs and original-ID lock
targets; no desktop install/restart was performed for these copy-only changes.
No CPU or memory reduction is claimed without measurement. Remote acceptance
is pending publication of this slice; CI for the preceding T4c batch is separate.

The preceding T4c batch passed all 23 jobs in
[run 43429056818](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/43429056818).
That run validates the 49-check source `1d36b229b`, not this fiftieth check;
[machine-readable acceptance](T4d-data/ci-1d36b229b.json) records the distinction.
