# R8c: clang-tidy check inventory of the production code

Status: Complete (report only). No source, gate, CI or `.clang-tidy` change: the slice
adds a tool, its tests, the two inventories and this record.

Batch / slice and selected scope: R8c, "clang-tidy check inventory on production code,
report only and no gate change", with the acceptance "report with counts per check and
a proposed gate order".

Source baseline and start HEAD: `705a7b94c` (local and remote `main`). Branch
`claude/r8c-tidy-inventory`, worktree `.claude/worktrees/r8c-tidy-inventory`. Commits
`d376a70d4` (tool and tests), `db2d59e74` (data, summary script, per-check timing) and
the commit that adds this record.

## A correction to the slice definition

The plan says CI's clang-tidy 18 "cannot be mirrored locally". It can: the local image
`latte-r2c-clazy:noble` (Ubuntu 24.04, the recipe in the [R2c record](R2c.md)) ships
`clang-tidy` 18.1.3 from the same Ubuntu archive that the CI Clang Debug job installs
it from. The CI-version inventory below ran there.

## Method

`R8c-tools/tidy_inventory.py` runs clang-tidy once per production source and counts
the findings per check:

- The source set is the clazy gate's: the tool imports `scripts/test-clazy.py` and uses
  its selection (one compile command per source, preferring the production target;
  `autotests/` and generated sources in the build directory skipped) and its header
  de-duplication. Both runs selected 155 sources and skipped 55 autotest sources, 137
  duplicate compile commands and 86 generated sources.
- Each source gets its own clang-tidy process from a database that holds only the
  selected commands; `clang-tidy -p` on the full database analyzes every compile command
  of a file (four for `app/data/errordata.cpp`).
- A complete `--config` replaces the repository's `.clang-tidy`, so the gate's file (two
  checks, `WarningsAsErrors: '*'`, headers of `app/data` only) is never read and the run
  never fails on findings. `HeaderFilterRegex` admits every header of the checkout;
  findings in generated code and in `autotests/` are counted apart and left out.
- A finding is a unique location per check name. clang-tidy prints aliases that report
  the same diagnostic in one bracket list; each name is counted, so family totals count
  such a diagnostic once per name. Aliases with other default options report different
  locations and are separate counts anyway.
- Compiler errors would mark a source as not analyzed; the run fails then, after writing
  the report. None occurred.
- Families: `bugprone`, `cert`, `clang-analyzer`, `concurrency`, `cppcoreguidelines`,
  `misc`, `modernize`, `performance`, `portability`, `readability`. Left out: the
  families for other code bases (abseil, altera, android, boost, darwin, fuchsia,
  google, linuxkernel, llvm, llvmlibc, mpi, objc, openmp, zircon) and `hicpp`, which is
  mostly aliases.

The two inventories, run by Haiku sub-agents and checked by the main session:

| Inventory | clang-tidy | Tree | Data |
|---|---|---|---|
| CI version | 18.1.3 (Ubuntu) in `latte-r2c-clazy:noble` | `git archive` of `d376a70d4` (= `main` `705a7b94c` plus the tool), fresh `clang-debug` build with Clang 18.1.3: 429 units, 0 compiler warnings | `R8c-data/ci-clang-tidy-18.json.gz`, `.tsv` |
| Host | 22.1.8 (Gentoo) | `main` checkout `build/modernization/clang-debug` at `705a7b94c` (Clang 22.1.8), up to date | `R8c-data/host-clang-tidy-22.json.gz`, `.tsv` |

The tool's ten unit tests pass on the host and in the container (by hand, as R8d's do):
the output parser, the counts, the configuration and the per-check profile, and one
end-to-end case on a fake checkout with a real compile database. A scratch mutation that
lets the analysis read `.clang-tidy` fails that case.

Reproduce the tables below:

```bash
python3 docs/modernization-results/R8c-tools/summarize.py \
    docs/modernization-results/R8c-data/ci-clang-tidy-18.json.gz \
    docs/modernization-results/R8c-data/host-clang-tidy-22.json.gz
```

## Results

### Overview

| | clang-tidy 18.1.3 | clang-tidy 22.1.8 |
|---|---:|---:|
| Checks enabled | 410 | 470 |
| Production sources | 155 | 155 |
| Sources that failed | 0 | 0 |
| Findings (first-party, unique) | 12317 | 14358 |
| Checks with findings | 91 | 110 |
| Checks without findings | 319 | 360 |
| Not counted (generated or test code) | 1152 | 2166 |
| Wall time (s) / jobs | 830 / 6 | 408 / 4 |
| Sum of unit times (s) | 4882 | 1601 |
| Median / slowest unit (s) | 30.5 / 84.2 | 7.7 / 61.4 |

### Families

| Family | clang-tidy 18.1.3: findings / checks with / without | clang-tidy 22.1.8: findings / checks with / without |
|---|---:|---:|
| bugprone | 303 / 8 / 72 | 300 / 9 / 94 |
| cert | 2 / 2 / 37 | 4 / 3 / 38 |
| clang-analyzer | 33 / 5 / 114 | 32 / 5 / 123 |
| concurrency | 0 / 0 / 2 | 0 / 0 / 2 |
| misc | 3359 / 7 / 15 | 3601 / 10 / 17 |
| performance | 246 / 5 / 14 | 247 / 5 / 14 |
| portability | 0 / 0 / 3 | 25 / 1 / 4 |
| cppcoreguidelines | 1276 / 24 / 16 | 2556 / 26 / 16 |
| modernize | 4206 / 15 / 22 | 4673 / 20 / 26 |
| readability | 2892 / 25 / 24 | 2920 / 31 / 26 |

### The gate's checks over all production sources (clang-tidy 18.1.3)

| Check | Findings | Files | clang-tidy 22.1.8 |
|---|---:|---:|---:|
| `bugprone-unused-return-value` | 0 | 0 | 0 |
| `bugprone-use-after-move` | 0 | 0 | 0 |

### Checks without findings (clang-tidy 18.1.3)

| Family | Checks | Of them also without findings in clang-tidy 22.1.8 |
|---|---:|---:|
| bugprone | 72 | 72 |
| cert | 37 | 37 |
| clang-analyzer | 114 | 114 |
| concurrency | 2 | 2 |
| misc | 15 | 15 |
| performance | 14 | 14 |
| portability | 3 | 3 |
| cppcoreguidelines | 16 | 16 |
| modernize | 22 | 21 |
| readability | 24 | 24 |

### Defect families: checks with 1 to 10 findings (clang-tidy 18.1.3)

| Check | Findings | Files | clang-tidy 22.1.8 |
|---|---:|---:|---:|
| `bugprone-branch-clone` | 10 | 7 | 10 |
| `bugprone-switch-missing-default-case` | 8 | 6 | 8 |
| `clang-analyzer-optin.core.EnumCastOutOfRange` | 6 | 1 | 6 |
| `performance-no-automatic-move` | 6 | 5 | 6 |
| `bugprone-forward-declaration-namespace` | 3 | 3 | 3 |
| `bugprone-suspicious-include` | 3 | 3 | 3 |
| `clang-analyzer-cplusplus.NewDeleteLeaks` | 3 | 2 | 2 |
| `clang-analyzer-deadcode.DeadStores` | 3 | 2 | 3 |
| `bugprone-integer-division` | 2 | 1 | 2 |
| `bugprone-parent-virtual-call` | 2 | 2 | 1 |
| `misc-header-include-cycle` | 2 | 2 | 2 |
| `performance-unnecessary-copy-initialization` | 2 | 2 | 3 |
| `cert-oop11-cpp` | 1 | 1 | 1 |
| `cert-oop54-cpp` | 1 | 1 | 1 |
| `clang-analyzer-cplusplus.PureVirtualCall` | 1 | 1 | 1 |
| `misc-redundant-expression` | 1 | 1 | 1 |
| `performance-move-constructor-init` | 1 | 1 | 1 |

### Defect families: checks with more than 10 findings (clang-tidy 18.1.3)

| Check | Findings | Files | clang-tidy 22.1.8 |
|---|---:|---:|---:|
| `misc-include-cleaner` | 1851 | 151 | 1900 |
| `misc-const-correctness` | 1344 | 111 | 1466 |
| `bugprone-narrowing-conversions` | 230 | 42 | 222 |
| `performance-unnecessary-value-param` | 200 | 40 | 200 |
| `misc-non-private-member-variables-in-classes` | 130 | 24 | 130 |
| `bugprone-easily-swappable-parameters` | 45 | 22 | 45 |
| `performance-enum-size` | 37 | 19 | 37 |
| `clang-analyzer-optin.cplusplus.VirtualCall` | 20 | 7 | 20 |
| `misc-no-recursion` | 16 | 9 | 16 |
| `misc-use-anonymous-namespace` | 15 | 4 | 15 |

### Style families: the 15 largest (clang-tidy 18.1.3)

| Check | Findings | Files | clang-tidy 22.1.8 |
|---|---:|---:|---:|
| `modernize-use-trailing-return-type` | 2794 | 285 | 3200 |
| `readability-implicit-bool-conversion` | 924 | 70 | 863 |
| `modernize-use-nodiscard` | 705 | 103 | 702 |
| `readability-identifier-length` | 633 | 123 | 633 |
| `readability-qualified-auto` | 308 | 42 | 308 |
| `cppcoreguidelines-avoid-magic-numbers` | 306 | 76 | 306 |
| `readability-magic-numbers` | 306 | 76 | 306 |
| `modernize-concat-nested-namespaces` | 280 | 240 | 280 |
| `cppcoreguidelines-narrowing-conversions` | 230 | 42 | 222 |
| `cppcoreguidelines-owning-memory` | 207 | 57 | 207 |
| `modernize-use-auto` | 172 | 49 | 172 |
| `readability-redundant-access-specifiers` | 171 | 84 | 171 |
| `readability-convert-member-functions-to-static` | 113 | 42 | 114 |
| `cppcoreguidelines-special-member-functions` | 92 | 92 | 90 |
| `readability-function-cognitive-complexity` | 90 | 41 | 90 |

### Names that report the same diagnostic (clang-tidy 18.1.3)

| Names | Diagnostics |
|---|---:|
| `cppcoreguidelines-avoid-magic-numbers` + `readability-magic-numbers` | 306 |
| `bugprone-narrowing-conversions` + `cppcoreguidelines-narrowing-conversions` | 230 |
| `cppcoreguidelines-non-private-member-variables-in-classes` + `misc-non-private-member-variables-in-classes` | 72 |
| `cppcoreguidelines-avoid-c-arrays` + `modernize-avoid-c-arrays` | 40 |
| `cppcoreguidelines-macro-to-enum` + `modernize-macro-to-enum` | 20 |
| `cppcoreguidelines-explicit-virtual-functions` + `modernize-use-override` | 18 |
| `cppcoreguidelines-use-default-member-init` + `modernize-use-default-member-init` | 11 |
| `cert-oop11-cpp` + `performance-move-constructor-init` | 1 |

### clang-tidy 22.1.8 against clang-tidy 18.1.3

- Checks only in clang-tidy 22.1.8: 70, 18 of them with findings: `bugprone-derived-method-shadowing-base-method` 6, `cert-int09-c` 2, `cppcoreguidelines-pro-bounds-avoid-unchecked-container-access` 1254, `cppcoreguidelines-use-enum-class` 35, `misc-multiple-inheritance` 3, `misc-override-with-different-visibility` 27, `misc-use-internal-linkage` 41, `modernize-avoid-c-style-cast` 50, `modernize-use-designated-initializers` 3, `modernize-use-integer-sign-comparison` 4, `modernize-use-ranges` 6, `portability-avoid-pragma-once` 25, `readability-enum-initial-value` 2, `readability-math-missing-parentheses` 72, `readability-redundant-parentheses` 6, `readability-redundant-typename` 1, `readability-use-concise-preprocessor-directives` 4, `readability-use-std-min-max` 5.
- Checks only in clang-tidy 18.1.3: 10: `cert-dcl21-cpp`, `clang-analyzer-core.CallAndMessageModeling`, `clang-analyzer-core.StackAddrEscapeBase`, `clang-analyzer-cplusplus.VirtualCallModeling`, `clang-analyzer-nullability.NullabilityBase`, `clang-analyzer-osx.NSOrCFErrorDerefChecker`, `clang-analyzer-valist.CopyToSelf`, `clang-analyzer-valist.Uninitialized`, `clang-analyzer-valist.Unterminated`, `clang-analyzer-valist.ValistBase`.
- Checks in both whose count differs: 16.

| Check | clang-tidy 18.1.3 | clang-tidy 22.1.8 |
|---|---:|---:|
| `modernize-use-trailing-return-type` | 2794 | 3200 |
| `misc-const-correctness` | 1344 | 1466 |
| `readability-implicit-bool-conversion` | 924 | 863 |
| `misc-include-cleaner` | 1851 | 1900 |
| `bugprone-narrowing-conversions` | 230 | 222 |
| `cppcoreguidelines-narrowing-conversions` | 230 | 222 |
| `modernize-use-nodiscard` | 705 | 702 |
| `cppcoreguidelines-special-member-functions` | 92 | 90 |
| `bugprone-parent-virtual-call` | 2 | 1 |
| `clang-analyzer-cplusplus.NewDeleteLeaks` | 3 | 2 |
| `cppcoreguidelines-pro-bounds-pointer-arithmetic` | 36 | 37 |
| `modernize-make-unique` | 0 | 1 |
| `performance-unnecessary-copy-initialization` | 2 | 3 |
| `readability-convert-member-functions-to-static` | 113 | 114 |
| `readability-else-after-return` | 64 | 63 |
| `readability-redundant-member-init` | 13 | 12 |

### Slowest units (clang-tidy 18.1.3)

| Source | Seconds |
|---|---:|
| `app/view/containmentinterface.cpp` | 84.2 |
| `app/lattecorona.cpp` | 80.2 |
| `app/layouts/storage.cpp` | 72.0 |
| `app/layout/genericlayout.cpp` | 67.7 |
| `app/wm/waylandinterface.cpp` | 56.9 |
| `app/layouts/importer.cpp` | 55.0 |
| `app/view/view.cpp` | 54.8 |
| `app/layouts/synchronizer.cpp` | 54.2 |

## Cost of a gate

Three more runs price the candidates, all with CI's clang-tidy 18.1.3 in the same
container and `--jobs 4` (CI's runners have four cores): the gate's two checks over all
production sources, every non-analyzer check with `--check-profile` (clang-tidy's
per-check timing, summed over the sources), and the static analyzer alone, which the
profile does not show. They reused the CI-version build tree with an export of
`db2d59e74`, which has the same production sources, and ran one after another on an
otherwise idle host (load 0.22 before; the desktop stayed in normal use). Data:
`R8c-data/ci-cost-gate.json.gz`, `ci-cost-noanalyzer.json.gz`, `ci-cost-analyzer.json.gz`;
the ranked candidates with their cost are `R8c-data/ci-gate-candidates.tsv`.

```bash
python3 docs/modernization-results/R8c-tools/costs.py \
    --inventory docs/modernization-results/R8c-data/ci-clang-tidy-18.json.gz \
    --gate docs/modernization-results/R8c-data/ci-cost-gate.json.gz \
    --checks docs/modernization-results/R8c-data/ci-cost-noanalyzer.json.gz \
    --analyzer docs/modernization-results/R8c-data/ci-cost-analyzer.json.gz \
    --candidates docs/modernization-results/R8c-data/ci-gate-candidates.tsv
```

### Cost runs (clang-tidy 18.1.3, 4 jobs)

| Run | Checks | Wall (s) | Sum of unit times (s) | Profiled check time (s) | Median / slowest unit (s) |
|---|---:|---:|---:|---:|---:|
| Gate's two checks | 2 | 213 | 840 | 254 | 5.1 / 10.0 |
| All non-analyzer checks | 291 | 1842 | 7247 | 6510 | 45.5 / 86.3 |
| Static analyzer only | 119 | 278 | 1098 | 0 | 4.7 / 53.1 |

Base cost (parsing and framework): 586 s over 155 sources, median 3.5 s, slowest 7.1 s. The gate's two checks add `bugprone-unused-return-value` 71 s, `bugprone-use-after-move` 182 s. The analyzer run's units sum to 1098 s, about 513 s above the base.

### Candidates: checks without findings in bugprone, concurrency, misc, performance, portability

104 checks besides the gate's two; 11 are not in the profile, so their cost is unknown: `bugprone-dynamic-static-initializers`, `bugprone-exception-escape`, `bugprone-macro-parentheses`, `bugprone-macro-repeated-side-effects`, `bugprone-no-escape`, `bugprone-signal-handler`, `bugprone-unhandled-exception-at-new`, `performance-noexcept-destructor`, `performance-noexcept-move-constructor`, `performance-noexcept-swap`, `portability-restrict-system-includes`.

| Band | Cost per check over the tree | Checks | Together (s) | Estimated wall added at 4 jobs (s) |
|---|---|---:|---:|---:|
| A | up to 10 s | 46 | 143 | 36 |
| B | 10 to 50 s | 30 | 708 | 177 |
| C | over 50 s | 17 | 1470 | 367 |

The gate's two checks over all sources took 213 s of wall time; adding band A is estimated at 246 s in all, bands A and B at 423 s.

Band C, the most expensive:

| Check | Cost (s) |
|---|---:|
| `bugprone-stringview-nullptr` | 165 |
| `bugprone-standalone-empty` | 152 |
| `misc-unused-using-decls` | 149 |
| `bugprone-reserved-identifier` | 131 |
| `bugprone-suspicious-string-compare` | 88 |
| `bugprone-infinite-loop` | 77 |
| `bugprone-assert-side-effect` | 72 |
| `performance-move-const-arg` | 71 |
| `misc-confusable-identifiers` | 70 |
| `bugprone-suspicious-semicolon` | 69 |
| `bugprone-multiple-statement-macro` | 68 |
| `bugprone-sizeof-expression` | 65 |
| `misc-coroutine-hostile-raii` | 61 |
| `portability-std-allocator-const` | 61 |
| `bugprone-unused-raii` | 59 |
| `bugprone-chained-comparison` | 57 |
| `bugprone-inc-dec-in-conditions` | 56 |

## Findings worth a look

Report only; nothing was changed. Locations are from the clang-tidy 18 inventory.

| Check | Location | Verdict |
|---|---|---|
| `misc-redundant-expression` | `app/plasma/extended/backgroundcache.cpp:269` | Defect: `inBounds` tests `bright2 <= 255` twice and never `bright1 <= 255`. The brightness values normally stay within 0 to 255, so no visible effect is known. |
| `bugprone-parent-virtual-call` | `app/settings/viewsdialog/viewstableview.cpp:35` | Defect: `selectionChanged()` calls `QAbstractItemView::selectionChanged()` and skips `QTableView`'s override, which sends the accessibility events for the selection. |
| `bugprone-parent-virtual-call` | `app/view/view.cpp:1800` | Intended: the comment above it documents the libplasma 6.8 `ContainmentView::resizeEvent()` crash it avoids before the root item exists. |
| `clang-analyzer-cplusplus.PureVirtualCall` | `app/view/settings/subconfigview.cpp:391` | Not reachable: the constructor calls `setupWaylandIntegration()`, which returns while `m_latteView` (a `QPointer`, set only in `initParentView()`) is null; the analyzer cannot see the `QPointer` state. The guard is the only thing between the constructor and a pure virtual call. |
| `clang-analyzer-cplusplus.NewDeleteLeaks` | `indicatorresources.cpp:49`, `plasmapaneltracker.cpp:141`, `:143` | Not leaks: a `QObject` parent and `deleteLater()` own them. |
| `cert-oop54-cpp` | `app/wm/windowinfowrap.cpp:127` | Safe: `operator=` copies value members only, so self-assignment does no harm. |
| `performance-move-constructor-init` | `app/wm/windowinfowrap.cpp:54` | Minor: the move constructor copies the window id instead of moving it. |
| `clang-analyzer-deadcode.DeadStores` | `app/main.cpp:691`, `:700`; `app/wm/waylandinterface.cpp:1157` | Harmless dead stores. |
| `bugprone-integer-division` | `declarativeimports/core/dialog.cpp:182`, `:186` | Popup centering halves the integer popup size, losing half a pixel for odd sizes; probably harmless. |
| `bugprone-suspicious-include` | three `#include "moc_*.cpp"` | AUTOMOC idiom, not defects. |
| `misc-header-include-cycle` | `app/data/genericdata.h` and `generictable.h` | The two headers include each other. |

## Proposed gate order

Rules, taken from the clazy gate (`scripts/test-clazy.py`, the [R2c record](R2c.md)):

- Name every gated check; no family wildcards. clang-tidy 22 enables 70 checks that 18
  lacks, 18 of them with findings here, and drops 10, so a wildcard gate changes with
  the CI image.
- Gate a check only when CI's clang-tidy reports nothing for it, or after one slice has
  fixed or suppressed (with the reason beside the code) every finding of that check. Of
  the 319 checks without findings in 18, all but `modernize-make-unique` (one finding)
  stay without findings in 22, so such a list survives an upgrade.
- One negative fixture per gated check that the runner must see rejected under the
  production flags, and a comparison of `--list-checks` with the named list. A check
  without a fixture that fires stays out.
- Gate the primary names, not their aliases: an alias reports the same diagnostics and
  runs its matchers again (`cert-dcl37-c` and `cert-dcl51-cpp` each cost as much as
  `bugprone-reserved-identifier`).
- Analyze the clazy gate's source set, as the inventory does, not one file.

The order:

1. **Scope.** The gate's two checks over all 155 production sources instead of
   `app/data/errordata.cpp`: no findings in 18 or 22, 213 s at 4 jobs here. The runner
   takes the clazy gate's source selection (as `tidy_inventory.py` does) and gets a
   fixture for `bugprone-use-after-move`; `bugprone-unused-return-value` has one.
2. **Band A.** The 46 checks without findings that cost at most 10 s each over the tree,
   together about 36 s at 4 jobs, in slices of about 15 with their fixtures. Most look
   for real defect patterns (`bugprone-empty-catch`, `performance-for-range-copy`,
   `bugprone-copy-constructor-init`, `bugprone-undefined-memory-manipulation`,
   `bugprone-unhandled-self-assignment` and others); a few target APIs this code barely
   uses (`concurrency-thread-canceltype-asynchronous`, `bugprone-bad-signal-to-kill-thread`),
   and the fixture step decides those.
3. **Fix, then gate, one check per slice**, starting with the defects:
   `misc-redundant-expression` (the `backgroundcache.cpp` defect),
   `bugprone-parent-virtual-call` (one fix, one documented suppression),
   `performance-move-constructor-init`, `performance-unnecessary-copy-initialization`,
   `misc-header-include-cycle`, `bugprone-forward-declaration-namespace`,
   `performance-no-automatic-move`, `bugprone-switch-missing-default-case` and
   `bugprone-branch-clone` (each finding reviewed), `bugprone-integer-division` (decide
   whether the half-pixel matters). `bugprone-suspicious-include` only with an exception
   for the AUTOMOC `moc_*.cpp` includes.
4. **Band B.** 30 checks at 10 to 50 s each, about 177 s together, including
   `bugprone-dangling-handle`, `bugprone-unchecked-optional-access` and
   `concurrency-mt-unsafe`, when the CI time allows.
5. **Static analyzer, as its own job.** All 119 checks took 279 s here, about 513 s of CPU
   above the base. Without the `optin` checks it reports 7 findings, all reviewed above
   (three ownership false positives, one unreachable pure virtual call, three dead
   stores); they are suppressed or removed first. The `optin` checks
   (`optin.cplusplus.VirtualCall` 20, `optin.core.EnumCastOutOfRange` 6) stay out.
6. **Band C and the unpriced checks**: 17 checks over 50 s each (about 367 s together)
   and 11 that the profile does not time (preprocessor callbacks and a few others); each
   needs its own measured budget first.

Not proposed: the style families (`cppcoreguidelines`, `modernize`, `readability`) and
the large defect-family checks (`misc-include-cleaner` 1851, `misc-const-correctness`
1344, `bugprone-narrowing-conversions` 230, `performance-unnecessary-value-param` 200,
`misc-non-private-member-variables-in-classes` 130, `bugprone-easily-swappable-parameters`
45, `performance-enum-size` 37, `misc-no-recursion` 16, `misc-use-anonymous-namespace`
15). They encode style choices or work against Qt's idioms (implicit sharing makes
by-value Qt parameters cheap, `QObject` parents own memory) and need a project decision,
not a gate slice.

## Not verified

- CI's runner is not this host: the times are this host's (16 cores, 4 jobs for the
  CI-version cost runs) and only show proportions.
- The unit times in the two inventories are not a timing reference: the CI-version
  inventory ran with 6 jobs and overlapped the host inventory for about two minutes. The
  cost runs above are the reference.
- 11 candidate checks are not in clang-tidy's profile (checks working through
  preprocessor callbacks and a few others), so their cost is unknown.
- No fixture was written for any check, so no check is proven to fire under the
  production flags yet; that is the first step of each gate slice.
- The Debian 13 container build: deferred by the user until the refactor is complete;
  this slice changes no build input.
