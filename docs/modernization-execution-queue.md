# Modernization execution queue

Status: 2026-11-25, `main` at `672d41c16178a54126ede1da8c924f4316ab3a00`.

This file is the current task index, not a history of every experiment. The
main plan and the linked batch reports remain the evidence and completion
authorities. There is no active implementation task with supporting evidence.

## Remaining candidates

| Candidate | State | Evidence required before reopening |
| --- | --- | --- |
| A5 policy extraction | Deferred | A concrete ownership defect, duplicated policy or measured maintenance/build benefit. See [evaluation](modernization-results/A5.md). |
| P1b title-only fast path | Deferred | A bounded event-correlated latency seam. The valid Release rerun closed the environment question but found no CPU/PSS saving. See [report](modernization-results/P1b.md). |
| P3a edge-layout refresh | Deferred | A production-owned latest-generation geometry publication acknowledgement. See [readiness review](modernization-results/P3a.md). |
| P4 shared task query | Deferred as unjustified | A materially different workload showing meaningful duplicate-model cost. Five model-cost pairs found zero CPU delta and −15 KiB median PSS. See [evaluation](modernization-results/P4-P6.md). |
| P5 background shadow | Deferred as unjustified | Evidence of removable allocation or frame cost. Five RHI pairs found zero texture-operation and median frame-time deltas. See [evaluation](modernization-results/P4-P6.md). |
| P6 QML module caching | Deferred as unjustified | Material, repeatable cold/warm module benefit and an override-safe prototype. Existing component-specific results are mixed-direction noise. See [baseline](modernization-results/P6-scratch-build-baseline.md). |

Deferred here means no implementation is justified, not that work is pending.
Do not reopen a candidate merely to continue modernization.

## Completed work

- A2: ScrollView/ScrollBar warnings remain visible after pre-filter evidence;
  other message filters and popup behavior are preserved.
- A3: saved Justify layouts, separators, dragging and restart pass, including
  the synchronous order-request reentrancy fix.
- A4: primary-not-zero and nested containment mapping are covered by production
  tests and mutations.
- R6 LayoutManager: S1–S4, restore retry, launcher ordering and separator
  placement are complete. S5, S7 and S8 were evaluated and not implemented.
- R7/R8: the implementation and measurement slices in the main ledger are
  complete or explicitly closed.
- T1–T5: the accepted clang-tidy/clazy gate work is complete, including the
  optional-access promotion and LLVM renamed-check compatibility.
- P0, P1a, P2a, P2b and P3b are complete. P1b and P3a remain deferred only for
  the evidence listed above. P4–P6 are closed as unjustified.

## Evidence index

- [Main modernization plan and completion ledger](architecture-modernization-plan.md)
- [A-track follow-up plan](architecture-followup-plan.md)
- [Implementation and verification rules](architecture-modernization-implementation.md)
- [Performance validation protocol](performance-validation-guide.md)
- [A2 warning-filter review](modernization-results/A2.md)
- [A3 Justify fixture and desktop acceptance](modernization-results/A3-legacy-justify-fixture.md)
- [A4 mapping resolver](modernization-results/A4-resolver.md)
- [R6 partial restore and launcher follow-up](modernization-results/R6-layoutmanager-partial-retry.md)
- [P1b measurement foundation](modernization-results/P1b-measurement-foundation.md)
- [T5 optional-access promotion](modernization-results/T5-optional-promotion.md)

## Next-candidate gate

A new task must have a concrete defect or measured benefit and a bounded scope.
Follow the implementation guide: account for the current HEAD and worktree,
trace consumers, preserve compatibility, add production-path regression checks,
and run affected GCC/Clang builds and tests. Runtime changes also require the
canonical user-mode Debug install and focused manual retest. Update the linked
evidence and this index after each accepted slice. On `main`, commit and push
require explicit user approval; task completion alone does not authorize a
release or tag.
