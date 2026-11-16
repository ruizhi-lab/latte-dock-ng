# P4 Fedora observer continuation

Status: In progress. This continuation replaces the unavailable Arch guest for
the first live observer sequence and A/A preflight; it does not change or
supersede the historical Arch results in
[P4 Arch synthetic preflight](P4-arch-synthetic-preflight.md).

## Entry and current environment

The user selected Fedora after Arch SSH account authentication failed. The
Fedora libvirt domain is running at `192.168.122.128`, MAC
`52:54:00:23:b1:15`; its current SSH host key matches the existing host entry.
The configured route is `fedora@192.168.122.128`. A read-only check reports
Fedora 45, Plasma 6.8.2 and system CMake 4.3.0. The older local Fedora memory
file describes a different version, so this run uses the observed guest
versions.

At handoff, `HEAD` is `0a6974f5c` with concurrent T4k CMake changes, the P4
observer tools, and modernization ledger updates in the working tree. Do not
identify a Release artifact only by `HEAD`: record the exact source archive and
patch hashes used for the guest build. The modernization worker's CMake 4.5
download stalled, so its separate install-preparation check remains pending;
it is proceeding with a staged Release build using Fedora's system CMake 4.3.0
to supply the P4 artifact. No P4 desktop action has started.

The worker froze the guest build input as `0a6974f5cd1b13116764a2320e3d882674f5fed3`
plus four CMakeLists overrides, archive SHA-256
`92607311df689f379ea27a56dcfcd9d7180db9131b80d2a97a797dc8c3665fc9`. The
Fedora Release build completed with CMake 4.3.0, GCC 16.2.1, `BUILD_TESTING=OFF`,
and two build jobs. Configure, build and staged install returned 0; the build log
has zero warning/error lines. The 699-entry install manifest SHA-256 is
`5c91bfd3e032d69930f71c09720bae24146323f1fa30f17d6b178313d6e13ff9`. The
installed executable is
`/tmp/T4k-fedora-20261116/prefix/bin/latte-dock-ng`, SHA-256
`a86461b57ea69b4c2be145f4612a483e25fac329c11db6b3f978e44aaa675d60`; its
build-tree counterpart SHA-256 is
`11e1c689f1fa562565f034acfaefb3592ee5e8d795adea4fcce6f2f4633a7c64`. They
share ELF Build ID `439139dd54b5b67bd8fdedf877f31436817b2dc9`; the difference is
the expected CMake install RUNPATH rewrite to the staged prefix. The two staged
QML roots and all eight plugin/typeinfo pairs were independently verified.
Use the installed-prefix executable and its hash for runtime identity.

One preliminary `--version` invocation from a bare SSH environment aborted
before producing version output. Coredump metadata records Qt's Wayland startup
failure (`Failed to create wl_display`, followed by failure to initialize the
Wayland platform plugin); the SSH environment had no valid Wayland socket. This
was an invalid launcher-environment probe, not a P4 runtime result. No coredump
baseline was captured before that probe, so this record does not claim whether
the core was new. Do not repeat GUI launches from the bare SSH shell; subsequent
runtime work must use the isolated nested session and session-bound launcher.

## Observer and measurement gate

The external observer and launcher are shared with the Arch preflight under
[P4 observer tools](P4-arch-preflight-data/tools/README.md); do not copy or
fork them here. Use only a private nested KWin/session with scratch HOME,
XDG paths, D-Bus and `TMPDIR`. The user's Fedora desktop, saved layouts and
settings remain outside the test. Launch the observer only in that private
session with a unique synthetic Dolphin title and exact `org.kde.dolphin` app
ID.

Before any live event or sample, record the frozen source identity, Release
build options, executable SHA-256/path, staged install manifest, and hashes of
the loaded Latte QML and project plugin files. Confirm the running process
resolves only to that staged prefix, with no `~/.local` overrides. Record the
PID, `/proc/<pid>/stat` start time and executable before each run and recheck
them at every sample to reject restart or PID reuse. Keep build activity out of
all idle and sampling windows. Use the nested session's own KWin, Activity
Manager and virtual desktop IDs; never sample if the observer reports anything
but exit 0 and a complete same-window activate/minimize/restore sequence.

The P4 workload precondition must also be re-established on Fedora before
external observer or A/A evidence is accepted: two real Trash applets must be
present in the same containment, both internal tracked-window models must be
active, and a bounded diagnostic must establish their activity/desktop/screen
context and role parity for the controlled Dolphin window. The historical
Arch fixture hash `0b0a09fa82db4301b2326cb62a905aa7614b188535a760ae5cfb2ccef86aca25`
is a reference only; it does not establish the Fedora fixture. Keep the
diagnostic separate from the uninstrumented Release samples. If the two-model
workload or its state parity cannot be verified, record no-go and collect no
A/A resource data.

A fresh Fedora fixture has been prepared from the built-in `Default.layout.latte`
template. It contains two real `org.kde.plasma.trash` applets (IDs 3 and 4) in
the same containment; its SHA-256 is
`90b974f266dbee2228c10315f0b86191cb42a7887da2ee7289979425aa07f506`. The
layout file is in the [Fedora data directory](P4-fedora-observer-data/README.md).
This hash records the planned fixture only; applet activation and tracker
creation remain to be verified in the nested session.

After the internal workload gate passes, run the observer once to prove the
live event sequence. If both gates pass, collect five same-build A/A runs with
the same fixture and event operation, with a 30-second warmup and a 60-second
fixed sampling interval. Preserve all runs,
CPU ticks, `smaps_rollup`/PSS, process identity, load average, and whether the
KSycoca cache changed. Report the observed spread only; these runs characterize
the observer and baseline and make no CPU/PSS saving claim. A failed sequence,
invalid sample or unavailable signal remains Pending validation; do not start
a shared-query prototype from this evidence alone.

## Results

The frozen Release artifact and staged plugin/QML identity are recorded above.
The bounded Fedora attempts reached a private nested KWin, imported the scratch
fixture, and started the staged dock and a synthetic Dolphin process. They did
not establish the internal tracker gate, so the external observer was not run
to a valid verdict and no A/A samples were collected.

| Attempt | Result |
| --- | --- |
| 1 | The launcher used `env -i`, which removed the user manager's graphical/config environment. KActivityManager aborted because it selected XCB without a display; the observer produced no final verdict. The launcher also expected `rg`, unavailable in the stripped PATH. This attempt is invalid environment-control evidence. |
| 2 | The graphical config paths and PATH were preserved, Activity Manager selected the nested Wayland display, and the staged layout loaded. The scratch QML diagnostic failed component creation because it added a second `Component.onCompleted` handler (`Property value set multiple times` at AppletItem.qml:1414). No tracker result is inferred. |
| 3 | The duplicate handler was merged into the existing handler. The diagnostic then raised TypeErrors because it dereferenced the nullable `applet` context object while logging IDs. The gate remained unobserved. |
| 4 | A replacement intended to use `backendAppletId` ran before the diagnostic strings were inserted, so the generated scratch QML still dereferenced `applet.id`. It emitted the same TypeErrors; the runner returned 20 (`internal-tracker-rows-not-observed`). The actual generated AppletItem QML SHA-256 is `66f297e51373742bb1e60f710fd273e48a12207b6c5324f6a7329a315e7aacea`; the staged original is `d4a98dcb6abaf3f03b45282344b39dd005b178234e7c582fc099a1939aa1ddaa`. |

These are failures in the diagnostic harness, not evidence that the production
tracker models are absent or broken. The live internal context/role parity and
external activation/minimize/restore sequence remain unverified. The A/A
preconditions are not satisfied, so sampling remains pending. The retained launchers and
logs are in [attempt evidence](P4-fedora-observer-data/attempts/README.md).

The coordinator rejected the known-broken attempt-4 generator as a final tool.
A separate narrow repair now adds only three diagnostic lines, using the
nullable-safe `backendAppletId`; it never globally replaces production
expressions. The [local generator check](P4-fedora-observer-data/tools/test_p4_diagnostics.py)
recovers the original production QML byte-for-byte after removing those lines
and checks unique insertion points and unchanged completion-handler count.
The coordinator independently ran that check and verified that attempt 5's
captured generated QML exactly matches the retained attempt-5 generator
applied to the current production source. The following snapshot-only
diagnostic revision is separate from that acceptance.

Attempt 5 exited 20 after 22.320 seconds with
`internal-tracker-rows-not-observed`. Both Trash loader IDs 3 and 4 became
active and reported `org.kde.dolphin`, without the earlier diagnostic QML
errors, but no matching-row record was observed. This proves the loader gate
only, not ready internal models or window-role parity. The external observer
was not run and no A/A sample was collected. At that point the log did not distinguish an empty model, an unready
filter context, or different task application IDs.

The retained attempt-5 shutdown also emitted a pipe warning because its
launcher terminated the dock and compositor together. The current launcher
now waits for the dock before stopping its supporting processes; it was not
rerun. No clean-quit or no-new-core acceptance is inferred from attempt 5.
Portal/PipeWire and Mesa/Zink warnings remain in the nested environment and
are not claimed resolved.

Attempt 6 added a source-preserving diagnostic capped at 64 snapshots per
applet and 16 raw rows per snapshot. After Dolphin appeared, both applets'
models contained one real non-startup window with raw app ID
`org.kde.dolphin`, title `window-title — Dolphin`, and the same window UUID.
Their activity/desktop IDs were nonzero and equal; both screens were
`(0, 0, 1280, 800)`. This establishes two internal model instances seeing the
same window, but the snapshot did not include active/minimized roles. The
`console.log` matching-row marker was still absent while `console.warn`
snapshots were visible, and the script returned 20 without invoking the
observer. This is a logging/gate no-go, not missing production windows.

The next narrow attempt uses a warning-level matching-row diagnostic and
explicit `qml.debug=true` in the private diagnostic environment. It must prove
initial role parity and a complete external observer sequence before any A/A
capture. All previous attempts remain invalid for resource measurement;
no shared-query implementation is justified.
No commit or push was requested or performed.

## Attempt 7 independent evidence review

Warning-level row diagnostics exposed matching rows for both Trash applets,
and the external observer completed its ordered activate/minimize/restore
sequence with exit 0. The launcher's selector nevertheless overwrote
`latte.log` while deriving its output pathname, so the final comparator could
not find `internal-parity.json` and the overall launcher exited 1. The original
full Latte log was lost; the retained runner contains both initial active,
shown, non-startup internal rows, and the overwritten log's first line
contains the selected parity JSON.

The coordinator independently ran the repaired selector on those retained
runner rows, recovered the same parity JSON, and applied the actual observer
sequence validator to the retained JSONL. Activity/desktop/screen and window
identity all match. The [review record](P4-fedora-observer-data/attempts/attempt-7-coordinator-review.json)
accepts only this live role/context/event evidence; no performance sample is
accepted and the launcher's failed status is unchanged. The selector now has
an explicit output pathname and a local regression check that arbitrary log
names cannot be overwritten by filename substitution.

The [A/A baseline slice](P4-fedora-aa-baseline.md) is preparing a clean,
uninstrumented workload from the unchanged staged prefix. It must retain five
valid same-build runs and reject process restarts, invalid observer sequences
and changing KSycoca caches. No resource saving or shared-query design is
established by this preflight.
