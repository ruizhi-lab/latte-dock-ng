# QML lint backlog and baseline governance

Date: 2026-09-30

The reviewed machine baseline is [`qmllint-baseline.json`](qmllint-baseline.json).
It records all 236 tracked QML files, the qmllint JSON revision, the tool and
host fingerprint, import roots, successful process exit codes, and normalized
diagnostic identities with multiplicities. The accepted fingerprint is
qmllint 6.11.1 on Ubuntu 24.04 with the KDE Neon build stack, using the GCC
Debug build tree and the installed `plasma-pa` QML module. A fingerprint
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

The initial Gentoo candidate used qmllint 6.11.2 and had the same 724
diagnostic identities and multiplicities as the accepted Ubuntu/Neon report.
The four Ubuntu/Neon reports from run
[36621406790](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/36621406790)
were byte-identical (SHA-256
`cd460339154a1c2dacc7c1ed7ec07d6b6f9c4a03502c34130501b80025043bee`), covered
all 236 files with zero failed exits, and contained no additions or removals
against the Gentoo candidate. Installing `plasma-pa` supplies the expected
`org.kde.plasma.private.volume` module and keeps the comparison free of
environment-only import warnings. The selected report was reviewed and copied
into the machine baseline; a green CI comparison against it remains pending.
