# T5: unchecked optional access evaluation

This report evaluates `bugprone-unchecked-optional-access`; it does not enable
the check. On the Clang Debug compile database after `e8ba4ad70`, the R8c
inventory selected 156 production sources and ran only this check with four
workers. LLVM 18.1.3 found no first-party diagnostics in 61.498 seconds; LLVM
22.1.8 found none in 60.238 seconds. Wall time was measured around the complete
inventory subprocess with Python's monotonic `perf_counter()`. The
[LLVM 18 log](T5-optional-data/llvm18-scan.log) and
[LLVM 22 log](T5-optional-data/llvm22-scan.log) show selection and scope.

Using the production compile command for `app/session/shutdownmonitor.cpp`,
with the source and output redirected to isolated fixtures, both versions
rejected an [unchecked dereference](T5-optional-data/negative.cpp) with this
check under `WarningsAsErrors=*`, while an [explicitly guarded access](T5-optional-data/safe.cpp)
passed. The respective fixture logs are in the adjacent data directory.

The earlier R8c cost inventory recorded 37.3 seconds for the individual check
in its CI-version environment. That is an isolated historical measurement,
not the incremental cost of adding it to today's 55-check gate. Three paired
local full-gate runs on LLVM 22, using the same database and four workers,
alternated execution order. The 55-check mean was 71.308 seconds and the
56-check mean was 71.473 seconds, with paired differences of +0.078, +0.105
and +0.310 seconds. All six runs passed. Load varied during the experiment and
the differences are within observed timing variation; this does not establish
an exact cost or predict CI performance. [Raw runs](T5-optional-data/full-gate-runs.json)
and individual gate logs are retained beside this report.

No CI cost budget has yet been established for Band B, and this local test does
not substitute for a comparison on the CI runner. No gate, build dependency,
or runtime behavior changed in this evaluation.
