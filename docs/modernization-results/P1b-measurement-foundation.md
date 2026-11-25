# P1b measurement foundation: KSycoca validity rule in the collector

Status: implemented and locally verified. This closes the launch-environment half
of P1b's correction; the event-correlated latency half remains open and P1b stays
Deferred.

Date: 2026-11-21. Base: local `main` at `4b9f6ef36`.

## Why this exists

P1b's Release A/B result could not be read as a CPU or memory comparison. Its own
erratum points at the cause: the pairs were launched by the same SSH harness as
P0's, and R5 then showed that an SSH launch with a shorter `XDG_CONFIG_DIRS` than
the Plasma session puts the dock in a KSycoca rewrite loop worth about 18 % of one
core. The collector recorded the load average but nothing about KSycoca, so a
contaminated window looked exactly like a clean one.

The performance protocol already required this check in prose ("treat a window in
which `~/.cache/ksycoca6_*` changes as invalid"). Nothing enforced it, so a later
run could repeat P1b's ambiguity. This slice moves the rule from prose into the
collector, where it cannot be forgotten.

## What changed

| File | Change |
| --- | --- |
| `scripts/measure-process-family.py` | Snapshots `~/.cache/ksycoca6_*` mtimes before each window and re-checks them every sampled second; a changed, appeared or vanished cache file marks the window `"valid": false` with a `KSycoca rewritten at second N: <files>` reason. `.lock` files are excluded. Each window reports `sycoca_cache_versions_seen`. |
| `autotests/test_measure_process_family.py` | New: the rule in isolation, six cases (no change, changed mtime, vanished, appeared, sorted/deduplicated, `.lock` excluded). |
| `autotests/CMakeLists.txt` | Registers `measurefamilyrulestest`, 60 s timeout. |
| `docs/performance-validation-guide.md` | Rule 7 names the in-collector check, its output shape and its test. |

Re-baselining after a reported rewrite keeps one rebuild from being reported once
per later second; the reason text still names the second it was first seen.

## Verification

| Check | Result |
| --- | --- |
| `python3 -I autotests/test_measure_process_family.py` | 6/6 pass |
| Mutation: `sycoca_rewrites` returns no change always | 3 failures (changed, vanished, appeared) |
| Mutation: `.lock` exclusion removed | 1 failure |
| End-to-end run against a real probe process | `"valid": true`, `"sycoca_cache_versions_seen": 3`, `invalid_reasons: []` |
| Four presets, `cmake --preset <p>` then `cmake --build --preset <p> -j8` | built, zero compiler warnings |
| Four presets, `ctest` | 66/66 in each (was 65; this adds `measurefamilyrulestest`) |
| Live positive case, Arch Plasma 6.8.3 VM | `kbuildsycoca6 --noincremental` inside a window gave `invalid_reasons: ["KSycoca rewritten at second 4: ksycoca6_en_V_...="]`, `"valid": false` |

The collector's process-sampling path needs a live process family; only the rule is
unit-tested. The live positive case was produced on a user-provided Arch Plasma
6.8.3 VM under `systemd-run --user`: forcing a KSycoca rebuild inside a sampling
window marked it invalid with the exact file and second, and the CPU reading in that
window more than doubled (0.5 % to 1.0 % of one core), which is the error the rule
keeps out of a comparison. Full detail:
[VM validation](VM-validation-2026-11-21.md). Two setups that did **not** trigger a
rewrite are recorded there too, so they are not retried: changing only the
collector's own `XDG_CONFIG_DIRS` does not affect an already running dock, and a
fresh `konsole` launched with a short path did not rebuild the cache within its
window.

## What this does and does not unlock

- It does: a future P1b (or P0/P4/P5/P6) A/B run can now prove its windows were
  not measuring KSycoca churn, which was the un-diagnosable part of the earlier
  outlier, and the rule is confirmed live against a real Plasma session.
- It does not: supply event-correlated latency, which P1b names as its second
  missing measurement. The collector reports CPU, memory, context switches and
  validity; it has no per-event timestamp seam, and P1b's own handoff records why
  production tracing would perturb the Release comparison.
- It does not change P1b's status. P1b needs both a clean-environment rerun **and**
  an event-latency seam before a CPU/PSS or latency claim can be made.

## Environment rerun result

The five-pair Release rerun under `systemd-run --user` is complete. Every
accepted window reported `"valid": true`; one baseline attempt was rejected
after a KSycoca rewrite and rerun successfully. The +144,763 KiB PSS outlier
did not reproduce. Median B−A deltas were +0.4613 CPU percentage points and
+194 KiB PSS, so no resource saving is claimed. See the valid rerun section in
[P1b](P1b.md#valid-release-rerun-on-the-session-desktop--2026-11-25).

Only the environment half is closed. P1b stays Deferred until a bounded,
event-correlated latency seam exists.
