# T4e: break the generic record/table include cycle

Baseline: `6f9757229d894381dbbb41de00b2bac68313926d` (T4d). The R8c `misc-header-include-cycle` finding identified a two-way include between `genericdata.h` and `generictable.h`.

`Generic` has no dependency on `GenericTable`, so `genericdata.h` no longer includes the table header. The table continues to include its record. `viewdata.h` and `errordata.h`, which embed tables by value, now include `generictable.h` explicitly. A nearby comment records the dependency direction and the cycle that returns if reversed. Template definitions, explicit instantiations, layouts, metatypes, serialization and runtime behavior are unchanged.

The new guarded self-including fixture deliberately triggers `misc-header-include-cycle`; its guard keeps recursion from producing duplicate definitions. It was rejected by clang-tidy 18.1.3 and 22.1.8 using the production `app/lattecorona.cpp` compilation flags, with the expected named finding and no compiler error. The check is registered in both `.clang-tidy` and `GATED_CHECKS`; a parity check confirms 51 enabled and gated checks match.

Focused clang-tidy runs with only `misc-header-include-cycle` completed successfully against `genericdata.cpp`, `generictable.cpp`, and `errordata.cpp` using the current Clang Debug compilation database, on both clang-tidy 18.1.3 and 22.1.8. The coordinator subsequently completed the full gates and build/test matrix.

## Coordinator acceptance

- GCC/Clang Debug/Release: four zero-warning builds, each passing 64/64 tests
  in a private D-Bus/offscreen environment. All 64 registered tests remain.
- clang-tidy 22.1.8 and isolated 18.1.3: all 51 negative fixtures rejected under
  production flags; 156 production sources have no findings from the gate.
  Four-worker timings were 76.087 and 107.020 seconds respectively.
- Focused clazy 1.17 / LLVM 19.1.1: `genericdata.cpp`, `generictable.cpp`,
  `errordata.cpp` and `viewdata.cpp` have no gated findings or analysis errors.
- Gate comparison against the published source confirms only the new check
  was added; no previous check was removed.

[Build/test results](T4e-data/validation-summary.json) and
[full gate evidence](T4e-data/tidy18-final.log) retain the results. No dependency,
minimum-version or installation change is needed. No desktop restart was
performed for this include-only repair. No build-time or CPU/PSS gain is
claimed without measurement. This slice is prepared for publication; its new remote acceptance
is separate from the preceding T4d CI run.

The preceding T4d source `6f9757229` passed all 23 jobs in
[run 43432385377](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/43432385377).
[Recorded acceptance](T4e-data/ci-6f9757229.json) covers that 50-check source,
not this new 51-check gate.
