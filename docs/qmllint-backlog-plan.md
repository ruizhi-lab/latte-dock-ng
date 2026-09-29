# QML lint backlog and baseline governance

Date: 2026-09-30

The reviewed machine baseline is [`qmllint-baseline.json`](qmllint-baseline.json).
It records all 236 tracked QML files, the qmllint JSON revision, the tool and
host fingerprint, import roots, successful process exit codes, and normalized
diagnostic identities with multiplicities. The initial measurement used
qmllint 6.11.2, Qt 6.11.2 and the GCC Debug build tree on Gentoo 2.18. It is a
local candidate baseline; CI must establish a matching Ubuntu 24.04 KDE Neon
baseline before this fingerprint can pass in the CI container. A fingerprint
mismatch fails closed and is not permission to overwrite this file.

## Initial measured backlog

Counts are diagnostic occurrences across all QML files, not unique files.
Repeated warnings in one file are counted separately. The machine report is the
authoritative source for per-file identities and duplicate multiplicity.

| Category | Occurrences | Initial handling |
| --- | ---: | --- |
| `unqualified` | 6,025 | Preserve while context-property and delegate interfaces are inventoried; fix only with an identified authoritative owner. |
| `missing-property` | 1,274 | Preserve dynamic Plasma/QObject properties as explicit exceptions; resolve statically typed project-owned cases separately. |
| `Quick.anchor-combinations` | 18 | Triage by layout owner and geometry; no blanket waiver. |
| `incompatible-type` | 6 | Triage as likely actionable type mismatch; keep existing identities visible. |
| `unused-imports` | 5 | Candidate for early cleanup after a second comparable run. |
| `Quick.property-changes-parsed` | 4 | Triage for bindings changed imperatively; preserve timing-sensitive behavior. |
| `unresolved-type` | 4 | Record host-registered `org.kde.latte.private.app` separately; other unresolved project types remain actionable. |
| `stale-property-read` | 3 | Triage for misspelled or obsolete properties. |
| `import` | 2 | Failed imports of build-owned modules are fatal regardless of the warning baseline. |
| `Quick.layout-positioning` | 2 | Triage for anchors/layout ownership conflicts. |

The measured total is **7,343 occurrences** across **10 categories**. The
historical “7k+” comment is removed from the runner; future totals come from
the machine report, not a copied hand-maintained count.

## Dynamic interface exceptions

The following patterns explain why some warnings need a host-aware decision;
they do not waive every diagnostic in a listed file or category:

- `unqualified`: Plasma applet context properties, QML delegate roles and
  aliases supplied by a containing component. Add explicit IDs or typed inputs
  when they preserve the owning component's contract.
- `missing-property`: properties on objects exposed as `QObject` or
  `QQuickItem`, including Plasma's runtime applet/context interfaces. Confirm
  the owning runtime type before converting these accesses; do not add fake
  declarations solely to silence lint.
- `unresolved-type`: `org.kde.latte.private.app` types are exposed through the
  application host and are not independently loadable by standalone qmllint.
  The module's qmldir, referenced plugin library and typeinfo metadata are
  checked from the build tree. A real host smoke test remains a separate
  requirement; a loose-file or standalone import is not evidence of it.

All other identities remain visible and count toward the same baseline. No
whole-file `missing-property` suppression is permitted.

## Promotion and refresh rules

1. A category can be promoted to an error only after two consecutive runs with
   the same tool/environment/import fingerprint, zero warnings in that
   category, and green CI for both runs.
2. Baseline refresh is a reviewed change. Capture the proposed report, compare
   it against the current baseline, inspect both `added` and `removed` identity
   lists, explain intentional additions, and run the full validation matrix.
   Never generate or accept a baseline automatically in CI.
3. An environment, qmllint format/revision, or QML file inventory change is
   incomparable. Establish a fresh candidate and review its full delta rather
   than suppressing the mismatch.
4. Syntax errors, failed qmllint processes, incomplete coverage and failed
   imports of build-owned modules remain unconditional failures.
5. Raw JSON chunks, stderr, manifest, normalized current report, comparator
   output and tool version are retained under each build directory and uploaded
   as CI evidence.

The CI Ubuntu/Neon candidate has not yet been collected. The current local
baseline intentionally makes that environment difference explicit. Replace it
only after a CI run provides the corresponding complete report and raw
evidence; until then, CI baseline comparison remains Pending validation.
