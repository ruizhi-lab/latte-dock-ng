# QML baseline review: 2026-11-19

## Decision

One reviewed identity count is accepted, and one source file is corrected so
its diagnostics drop below the baseline instead of growing.

- `plasmoid/package/contents/ui/task/animations/RealRemovalAnimation.qml`
  `unqualified`: 40 → **41**. The row-replacement work (`de3f70358`) added
  `!taskItem.taskRowReplacementSuccessor` to `enabledAnimation`. `taskItem` is a
  containing-context object in every animation file, so each bare reference is
  counted separately; the guard is required and cannot be expressed without one
  more reference.
- `declarativeimports/abilities/items/basicitem/HiddenSpacer.qml`
  `unqualified`: 44 → **40**. The separator-retention work (`08eb33fee`,
  `de3f70358`) first raised this file from 44 to 47 by adding
  `settledSeparatorSpace` and `retainingSeparatorSpace` and reading them, plus
  `separatorSpace`, `nScale` and `maxSize`, without qualification from inside
  the component's own `Binding` expressions. Those five names are declared by
  this file; they are now read through the `hiddenSpacer` id, which is the same
  spelling the file already used at `hiddenSpacer.nScale = 0`. Behaviour is
  unchanged: `hiddenSpacer` is the root item, so each access resolves to the
  identical property and keeps its dependency. The file therefore ends at 40,
  four below the previous baseline: the five own-property accesses are gone
  while the new `itemIndex` reads that come from the containing `BasicItem`
  remain, because that is a real context dependency.

No other identity changes. The refresh is therefore a one-line count update at
an unchanged tool/environment fingerprint (qmllint 6.11.1, JSON revision 4,
report schema 1, Linux x86_64 `host=ubuntu-24.04`), unchanged import roots, and
an unchanged 244-file inventory.

## Local verification

The author's host runs qmllint 6.11.3, so the full comparison correctly fails
closed with `tool fingerprint changed`. The diagnostic data is still usable for
review because the two tool versions agree on this tree: re-linting the
untouched baseline tree reproduces the published 6.11.1 counts exactly
(44 for the abilities `HiddenSpacer`, 40 for `RealRemovalAnimation`), and 272 of
the 282 shared `(file, category)` count pairs are identical between the local
report and the accepted baseline. The ten differing pairs are the two files
above plus `PulseAudio.qml`, `main.qml`, `PlasmaCoreThumbnail.qml` and one
`TaskItem.qml` identity, all of which need Plasma and `plasma-pa` QML modules
that the author's host does not install. `RealRemovalAnimation.qml` does not
differ, so the accepted 41 is confirmed under both tool versions.

## Accepted refresh

Only the `RealRemovalAnimation.qml` `unqualified` count changed in
`docs/qmllint-baseline.json`. No fake property, whole-file suppression,
error-category demotion or automatic CI generation was introduced, and no
category is promoted.

## Next action

Fresh CI against the published refresh is required, because the reviewed
fingerprint is the CI toolchain rather than the author's host.
