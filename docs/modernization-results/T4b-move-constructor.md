# T4b: gate `performance-move-constructor-init`

Source baseline: `main` at `9e62bfaa9336c22df475e65ff22ccd6643fd708f`.
R8c's clang-tidy 18 inventory found one instance in
`app/wm/windowinfowrap.cpp:54`; clang-tidy 22 reported the same finding.

The move constructor copied `m_wid`, a `QByteArray`, while initializing the
other members. After moving it, a focused recheck exposed the same issue in
`m_parentId`, also a `QByteArray`; both members now move their values. Each
change preserves the moved-to object's value and leaves the moved-from object
valid, with the usual unspecified value of a moved-from `QByteArray`.

Added the named check to `.clang-tidy` and `GATED_CHECKS`, plus a negative
fixture whose move constructor copies a `std::string` member. The fixture is
intentionally defective and exists only to prove the gate check.

The production finding was reproduced before the fix using the Clang Debug
compile database with host clang-tidy 22.1.8. After the fix, focused analysis
of `windowinfowrap.cpp` found no instances with clang-tidy 22.1.8 or 18.1.3.
The new fixture was rejected by `performance-move-constructor-init` as an error
under the existing production flags with both versions. The coordinator subsequently ran both full gates: all 48 negative fixtures
were rejected and 156 production sources had no findings. LLVM 22 took
75.718 seconds and LLVM 18 took 100.883 seconds with four workers.

A consumer audit found no requirement that a moved-from wrapper retain its
IDs: `WaylandInterface::requestInfo()` returns its local wrapper without
reading it again, and `Windows::infoFor()` copies map-owned values. Existing
move assignment remains outside this narrowly scoped constructor fix.
No dependency or minimum-version changes are required.

The formatter was run on touched C++ files; unrelated legacy formatting churn
was removed after diff inspection. Remote CI and genuine Plasma 6.8/6.3
VM results remain separate acceptance steps.

Combined GCC/Clang Debug/Release builds completed with zero compiler warnings;
each configuration passed 64/64 tests in a private D-Bus/offscreen environment.
Test registration stayed at 64 with no removals. The 49-check clazy gate passed
before the constructor edit; a focused recheck of the final constructor also
had no clazy findings. [Machine-readable evidence](T4b-data/validation-summary.json)
and [gate logs](T4b-data/tidy18-final.log) retain the local validation results.
