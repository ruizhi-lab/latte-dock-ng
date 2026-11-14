# QML baseline review: 2026-11-14

## Decision

The reviewed GCC Release candidate from run `43603278974` is accepted. It is
complete and comparable at the diagnostic-tool and environment level, but the
tracked QML inventory grew by one file, so it is **not identical** to the
previous baseline. This is a reviewed data refresh, not automatic regeneration:
3 candidate-added normalized identities (3 occurrences) and 11 candidate-removed
identities (24 occurrences). Visible occurrences move from 7,035 to 7,014. The
newly covered QML file produces no diagnostics.

## Provenance and completeness

- GitHub Actions run `43603278974` is `Build f784986c70b188cb1e86a86db04ccfe5ad7de008`,
  on `main`, event `push`, head SHA `f784986c70b188cb1e86a86db04ccfe5ad7de008`.
- All four build jobs (**GCC Debug**, **GCC Release**, **Clang Debug**,
  **Clang Release**) completed build, strict-warning verification, the test
  suite (63/63 tests on GCC Release), the targeted ASan/UBSan tree and the
  sanitizer fixtures successfully. The **only** failing step in each is
  **Deep QML lint**, and only at the baseline comparison step; **Retain QML
  baseline evidence** and **Upload build diagnostics on failure** succeeded.
- The four `qmllint-baseline-*` artifacts are **byte-identical**
  (SHA-256 `2a566003a04ebd9329fa3d4bfd7582e8c8ead41255f5f1996df83e5e9e8cedcc`).
  The reviewed GCC Release artifact is preserved as
  [`ci-report-evidence.tar.gz`](QML-baseline-2026-11-14-data/ci-report-evidence.tar.gz);
  the extracted normalized report is
  [`candidate-current.json`](QML-baseline-2026-11-14-data/candidate-current.json).
- Tool and environment fingerprints are unchanged from the accepted baseline:
  qmllint 6.11.1, JSON revision 4, report schema 1, Linux x86_64 /
  `host=ubuntu-24.04`, import roots `<build-qml>`, `<staged-latte-qml>`,
  `<system-qml>`.
- Candidate inventory is 244 files versus 243 in the accepted baseline. The one
  added file is `plasmoid/package/contents/ui/task/AudioPlaybackState.qml`.
- The candidate manifest is complete: both qmllint chunk processes exited 0 and
  all 244 per-file exit codes are 0. `fileExitCodes` keys equal
  `attemptedFiles`, which is sorted and duplicate-free.
- `python3 scripts/qmllint-baseline.py check-imports --report <candidate-current.json>`
  exited 0: no failed imports of build-owned Latte modules.

## Comparison

The prescribed compare command exited 2 with `QML file coverage changed;
baseline comparison is not comparable`, correctly refusing to compare unequal
inventories. A manual comparison of every normalized key
`(file, category, severity, message)` and count gives:

| Direction, accepted baseline → candidate | Identities | Occurrences |
| --- | ---: | ---: |
| Candidate additions | 3 | 3 |
| Candidate removals | 11 | 24 |

Candidate-added occurrences, with the exact raw location each one occupies:

| File:line:col | Diagnostic | Count |
| --- | --- | ---: |
| `plasmoid/package/contents/ui/PulseAudio.qml:75:34` | missing `isWindow` on `QObject` | 1 |
| `plasmoid/package/contents/ui/PulseAudio.qml:75:62` | missing `identity` on `QObject` | 1 |
| `plasmoid/package/contents/ui/PulseAudio.qml:166:26` | `incompatible-type`: `const QMetaObject` binding to `PulseAudioQt::VolumeObject` | 1 |

All three sit in the rewritten `PulseAudio.qml` of the audio-ownership fix, and
all three are qmllint inference limits rather than source defects:

- **`PulseAudio.qml:75`** (`if (task && task.isWindow) { tasks.push(task.identity); }`).
  `task` is `taskIdentities.objectAt(i)`, whose static type is `QObject`
  although the `Instantiator` delegate at line 33 is a `QtObject` declaring
  `readonly property bool isWindow` (line 35) and `readonly property var
  identity` (line 43). The delegate is the authoritative owner of both members;
  qmllint cannot see through `Instantiator.objectAt()`'s `QObject` return type.
  The access is guarded by `task &&`.
- **`PulseAudio.qml:166`** (`target: ... && typeof PulseObject !== "undefined" ? PulseObject : null`).
  `PulseObject` is the Plasma `org.kde.plasma.private.volume` singleton handling
  a `VolumeMonitor.target` assignment; the guard is deliberately `typeof`-based
  because that module is not guaranteed present. This is the same dynamic
  volume-module interface already carried as a visible identity elsewhere in
  the file.

The candidate removals are pre-existing diagnostics reduced by the same audio
refactor, not made comparable-file suppressions:

| File | Category | Occurrences |
| --- | --- | ---: |
| `PulseAudio.qml` | missing `appId`, `appName`, `binaryName`, `iconName`, `parentPid`, `pid`, `portalAppId` on `QObject` | 14 |
| `PulseAudio.qml` | `unqualified` | 7 |
| `task/AudioStream.qml`, `task/ProgressOverlay.qml`, `task/TaskIcon.qml` | `unqualified` | 1 each |

The `task/*` removals correspond to one-line changes replacing
`taskItem.playingAudio || taskItem.muted` with `taskItem.playingAudio`, which
also removes an unqualified `muted` read at each site. The `PulseAudio.qml`
removals come from moving per-stream metadata matching into
`plasmoid/package/contents/code/AudioStreamMatch.js` and from the new typed
`taskIdentities` delegate properties.

The normalized `unqualified` identity does not retain member names or locations;
[`changed-code-diagnostics.txt`](QML-baseline-2026-11-14-data/changed-code-diagnostics.txt)
holds the raw candidate diagnostics for every file whose identity counts changed.

## Accepted refresh

The exact reviewed CI candidate was copied into `docs/qmllint-baseline.json`,
preserving its tool/environment fingerprint, import roots, 244-file coverage and
all successful process/file exits, and replacing only the diagnostic and
manifest data that changed. No fake property, whole-file suppression,
error-category demotion or automatic CI generation was introduced. The two
consecutive-zero-runs rule still applies to any future category promotion; no
category is promoted here.

After acceptance, each of the four artifacts compares against the new baseline
with no added and no removed identities; see
[`independent-ci-comparisons.json`](QML-baseline-2026-11-14-data/independent-ci-comparisons.json).
The 19 parser tests pass; see
[`parser-tests.txt`](QML-baseline-2026-11-14-data/parser-tests.txt). The
[previous baseline](QML-baseline-2026-11-14-data/baseline-before.json) and the
[bidirectional count delta](QML-baseline-2026-11-14-data/identity-delta.json)
are retained beside this record.

## Next action

Fresh CI against the published refresh is required. The failing run for
`f784986c7` tested the previous 243-file baseline; its coverage failure is not
retroactively reclassified as a pass.
