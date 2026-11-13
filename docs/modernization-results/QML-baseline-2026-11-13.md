# QML baseline review: 2026-11-13

## Decision

The coordinator accepted the reviewed GCC Release candidate after the independent
artifact audit and source-boundary review below. It is complete and comparable
at the diagnostic-tool and environment level, but **not identical** to the
previous baseline. This is a reviewed data refresh, not automatic regeneration:
there are 9 candidate-added
normalized identities (35 occurrences) and 11 candidate-removed identities
(35 occurrences). The total remains 7,035 occurrences. The two newly covered
QML files produce no diagnostics.

## Provenance and completeness

- GitHub Actions run `43397964476` is `Build 7963444958bc8cfc4dc51a6788cb42e455c4a459`,
  on `main`, event `push`, head SHA `7963444958bc8cfc4dc51a6788cb42e455c4a459`.
- Job `131793522320` is **Build (GCC Release)**. Its build, strict-warning,
  test, and retained-evidence steps succeeded; only **Deep QML lint** failed,
  at the baseline comparison step. The job's artifact upload succeeded.
- Artifact metadata ties `qmllint-baseline-gcc-release` (artifact
  `14023963568`) to the same run and head SHA. The reviewed extracted files are
  in `/tmp/latte-modernization-t4a/ci-qml-gcc-release/qmllint-baseline/`.
- Both reports use qmllint 6.11.1, JSON revision 4, baseline schema 1, and the
  same Linux x86_64 / Ubuntu 24.04 environment fingerprint and import roots.
  Candidate inventory has 243 files versus 241 in the accepted baseline.
- Candidate manifest is complete: both qmllint chunk processes exited 0; all
  243 file exit codes are 0. Both raw chunk JSON files parse (200 + 43 file
  results), and their result count matches `expected-files.json` and the
  candidate report. Both stderr files are empty. The manifest matches the
  report's attempted file list, completion flag, per-file exits, and process
  exits (the report intentionally omits the manifest's `schemaVersion`).
- The added files are `containment/package/contents/ui/colorizer/BackgroundColorPolicy.qml`
  and `shell/package/contents/controls/ColorButton.qml`; neither has a
  diagnostic in the candidate report.
- `python3 scripts/qmllint-baseline.py check-imports --report <candidate-current.json>`
  exited 0: no failed imports of build-owned Latte modules. This is an import
  report check, not the generated-module artifact check.

## Comparison

The prescribed compare command exited 2 with `QML file coverage changed;
baseline comparison is not comparable`, correctly refusing to compare unequal
inventories. A manual comparison of every normalized key
`(file, category, severity, message)` and count gives:

| Direction, accepted baseline → candidate | Identities | Occurrences |
| --- | ---: | ---: |
| Candidate additions | 9 | 35 |
| Candidate removals | 11 | 35 |

Candidate-added warning families, with occurrence counts:

| File | Diagnostic | Count |
| --- | --- | ---: |
| `containment/package/contents/ui/BindingsExternal.qml` | `unqualified` | 1 |
| `containment/package/contents/ui/colorizer/Manager.qml` | `unqualified` | 6 |
| `indicators/default/package/config/config.qml` | `unqualified` | 7 |
| `indicators/default/package/ui/main.qml` | missing `configuration` on `QQuickItem` | 3 |
| `indicators/default/package/ui/main.qml` | missing `iconBackgroundColor` on `QQuickItem` | 1 |
| `indicators/default/package/ui/main.qml` | missing `isTask` on `QQuickItem` | 1 |
| `shell/package/contents/configuration/pages/AppearanceConfig.qml` | `unqualified` | 12 |
| `shell/package/contents/configuration/pages/EffectsConfig.qml` | `unqualified` | 3 |
| `shell/package/contents/controls/IndicatorConfigUiManager.qml` | missing `isModernDockStyle` on `QQuickItem` | 1 |

The normalized `unqualified` identity does not retain member names or locations.
Raw qmllint JSON retains line/column and suggestions; inspect the candidate raw
file for the exact occurrences before attributing a count to a particular new
expression. These are source-boundary review points:

- `BindingsExternal.qml` and `colorizer/Manager.qml` use properties supplied by
  the containment/QML context. Confirm each newly counted access is an
  intentional owner/context property; the two new helper components themselves
  have no warning.
- The indicator UI warnings are dynamic indicator/task API accesses (`configuration`,
  `iconBackgroundColor`, `isTask`) on an inferred `QQuickItem`. Confirm the
  runtime `LatteComponents.IndicatorItem` contract supplies them and icon color
  reads stay guarded by task/enabled state. `IndicatorConfigUiManager.qml`'s
  missing `isModernDockStyle` is on its dynamically loaded page/dialog bridge;
  confirm that dialog contract supplies the property in the host.
- Appearance and Effects page `unqualified` warnings are parent IDs/context
  used by nested controls. Check against the intended component ownership and
  `ComponentBehavior` semantics; do not blanket-accept solely by category.

The 11 candidate removals are pre-existing diagnostics reduced or eliminated
by source changes, in `ItemWrapper.qml` (6), both `ShortcutBadge.qml` files (1
each), `MultiLayered.qml` (1), containment `main.qml` (1), `TaskMouseArea.qml` (3),
`CanvasConfiguration.qml` (3), `LatteDockConfiguration.qml` (3), and
`IndicatorConfigUiManager.qml` (16). They balance the 35 new occurrences but
do not cancel the need to review the candidate additions individually.

## Coordinator source-boundary review and accepted refresh

The coordinator reviewed the changed source lines against the raw warning
locations, rather than accepting the unchanged total as evidence of equivalence.
The [changed-code diagnostics](QML-baseline-2026-11-13-data/changed-code-diagnostics.txt)
also contain existing identities whose locations moved; the normalized
[bidirectional count delta](QML-baseline-2026-11-13-data/identity-delta.json)
is the authority for the 35 added and 35 removed occurrences.

| Added family | Authoritative source / disposition |
| --- | --- |
| BindingsExternal `plasmoid` access | The host containment configuration map owns `windowGap`, declared in containment `config/main.xml`. Existing host context access is preserved; the nearby comment separates work-area struts from surface/input geometry. |
| Colorizer Manager context accesses | `plasmoid.configuration` owns custom colors; containment `main.qml` owns the `root` popup/expanded state and `themeExtended` alias. The new policy receives the configuration explicitly. These are existing containment context contracts. |
| Indicator config context accesses | `dialog` is injected by `app/view/settings/indicatoruimanager.cpp` into cached option pages. The hidden host forwards the live style, with `typeof` guards and the legacy indicator fallback. `i18n` is a localized host-context function. |
| Indicator `configuration`, `isTask`, `iconBackgroundColor` | `declarativeimports/components/IndicatorItem.qml` intentionally exposes a common Item through a bridge or public API. `declarativeimports/abilities/items/IndicatorObject.qml` declares these properties and forwards their public aliases. Icon-color use is gated by the configured request and task state; the QQuickItem tooling type does not describe that runtime API. |
| Appearance context accesses | `plasmoid` is the live containment configuration source. `units` is the page's readonly input derived from `page.dialog.units`; the remaining additions call localized `i18n`. The controls preserve those host/page inputs. |
| Effects context accesses | The three style comparisons read the same live `plasmoid.configuration.dockStyle`, explicitly replacing the stale CONSTANT snapshot for cached option pages. |
| Cached UI `isModernDockStyle` | EffectsConfig declares the property on the actual StackView; IndicatorConfigUiManager intentionally retains a generic Item reference and forwards its runtime field to the hidden dialog host. Existing nearby comments explain this ownership and cache constraint. |

These accesses have identified owners and remain visible warning identities.
No fake property, whole-file suppression, error-category demotion or automatic
baseline generation was introduced. The two new files themselves are clean in
the comparable report. The coordinator accepted the exact CI candidate after
this review, preserving its tool/environment fingerprint and all successful
process/file exits, and replacing only the diagnostic/manifest data that changed.

Evidence: [previous baseline](QML-baseline-2026-11-13-data/baseline-before.json),
[raw CI artifact contents](QML-baseline-2026-11-13-data/ci-report-evidence.tar.gz),
[comparison after acceptance](QML-baseline-2026-11-13-data/comparison-after-review.json),
and [19 passing parser tests](QML-baseline-2026-11-13-data/parser-tests.txt).
The accepted baseline compares with the reviewed CI report with no additions
or removals. Four current-HEAD builds/tests and syntax/module checks are
recorded in [T4a](T4a.md). The local 6.11.3 deep run remains incomparable to
this 6.11.1 baseline; it was not substituted as a passing comparison.

Two additional CI artifacts, GCC Debug and Clang Release from the same run,
independently match the accepted GCC Release candidate byte-for-byte (SHA-256
`87124c9926578772fc49ea9556e3489b2e4d2d90ed69ae063b865bb86fadfea3`).
All three normal comparisons return no added or removed identities;
[receipt](QML-baseline-2026-11-13-data/independent-ci-comparisons.json).

## Next action

Publish the reviewed refresh only with main-branch authorization, then record
the new CI comparisons. The failing run for `796344495` tested the previous
241-file baseline; its failure is not retroactively a pass. No category is
promoted to an error by this refresh, and the two-consecutive-zero-runs rule
still applies to future promotions.
