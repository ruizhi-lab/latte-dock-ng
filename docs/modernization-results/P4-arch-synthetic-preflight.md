# P4 Arch synthetic preflight

On 2026-11-14 an isolated preflight tested whether a synthetic workload could
support the deferred P4 measurement. It is not a reconstruction of the
historical Fedora P4 layout: neither the host nor the Arch VM had a matching
saved layout with verifiable configuration. The synthetic layout contained
App Menu, Kicker Dashboard, Kicker, Trash and a CPU System Monitor in one
containment. No performance data were collected.

The Arch guest used Plasma and libplasma 6.8.2 and Qt 6.12.0. A fresh archive
of commit `5ed12c7222aa1b744efa9049dc55163a582a879d` was checked against all
1,944 tracked blobs, with none missing or mismatched. A user-mode Debug build
and temporary diagnostic QML were installed into a scratch HOME under `/tmp`;
the instrumented build succeeded with no compiler warning. The synthetic
layout hash was `242ddb6190613850852ecddd89f0db929fc792b1a3dbf8b8ea577fcfc81ba951`;
the instrumented `AppletItem.qml` hash was
`1139744e350632b92c163cf5e07197b5b5ad8d25ddf82846dd569fba2f66b13e`.

The nested KWin session used its own Wayland socket, private D-Bus and scratch
ActivityManager. Activity and virtual desktop IDs were available, and an
independent filtered TaskManager probe saw the controlled Dolphin and QML
windows. The active Arch session Dock retained its PID throughout the test.
The nested rendering log also contained software-rendering, portal and
PipeWire warnings; these were not treated as clean desktop evidence.

The predeclared requirement of at least two per-applet tracker models was not
met. Only Trash supplied a tracked application ID (`org.kde.dolphin`), loaded
the model and matched the controlled Dolphin window. App Menu, Kicker
Dashboard, Kicker and System Monitor had empty tracked IDs and their loaders
were inactive. The current fallback maps Trash and Folder to Dolphin; the
remaining synthetic applet configuration did not supply launcher IDs. This is
consistent with the current gate and does not prove the missing historical
layout had the same configuration.

A second synthetic fixture added a standard Folder applet with its default
`desktop:/` URL and no injected tracked ID. Its hash was
`2bf118968a2430471af6b680bbec6f561e4e4544c6f153293c89cdacd6e976df`.
Trash and Folder were in the same containment; both independently loaded a
tracker model and repeatedly matched the controlled Dolphin window. The
private processes were cleaned up and the active Dock retained its PID. This
establishes the two-model precondition for this synthetic workload, not for
the historical Fedora layout.

The second run also emitted a Folder View `isScreenUiReady` TypeError and an
integer-comparison error in the temporary harness. The latter arose when a
missing log made the model count empty before numeric comparison; the loop
continued and the later model messages independently showed both trackers.
The Folder error comes from the Plasma applet calling a ShellCorona-specific
API absent from Latte Corona. [Plasma's implementation](https://raw.githubusercontent.com/KDE/plasma-workspace/Plasma/6.8/shell/shellcorona.cpp#L2125-L2147)
depends on desktop and panel state that Latte does not own. Folder's widget
loader does not use that readiness gate, but full Folder rendering and
interaction were not verified. A future
experiment must correct the harness and account for the compatibility error.
P4 remains **no-go for comparative resource measurement** until workload validity and
task/filter parity are established. Debug instrumentation and preflight
liveness are not Release A/B results and imply no CPU or memory savings.

A scratch-only fail-closed getter experiment on Arch eliminated the missing
method TypeError and observed the Folder widget's `FolderViewLayer` loader
starting with readiness false. It did not verify the rendered file list or
interaction: the model count was unavailable and a temporary QML observer had
its own import error. No desktop containment was exercised. The scratch code
used no Plasma 6.8-only C++ API, but it was not built against the 6.3 floor.
No compatibility implementation was added to the production tree; the Folder
error and full behavior remain open.

As a possible warning-free synthetic alternative, the Arch Trash plugin's
embedded metadata was inspected and did not declare a single-instance limit.
That alone did not establish runtime support for two instances. An initial
request to transfer and extract the source archive in the VM was rejected by
automated approval review. After the user clarified that the VM is a dedicated
test environment, the same operation was approved and the preflight ran.

The later synthetic fixture used two real Trash instances (IDs 6 and 9) in one
containment, with no manually injected tracked IDs or Folder applet. Its
SHA-256 was `0b0a09fa82db4301b2326cb62a905aa7614b188535a760ae5cfb2ccef86aca25`.
Both instances independently loaded tracker models and matched a controlled
Dolphin window. The private session still emitted portal, SQL query and virtual
graphics warnings, so this is not a clean performance baseline. No CPU or PSS
measurement was performed. The SQL warnings have no recorded query or stack,
and the log also says the KActivities service was not running. Both trackers
matched the controlled window, but full activity, configuration and indicator
state parity was not established.

A later bounded diagnostic run waited for the actual `KActivities::Consumer`
to reach Running and for both tracker contexts to become available. It first
observed undefined virtual-desktop values; about 62 ms later both received
change notifications and agreed on nonempty activity and desktop IDs and the
same screen rectangle. Once ready, both models matched the controlled Dolphin
window with the same `IsWindow`, `IsStartup`, `IsActive` and `IsMinimized`
roles for its foreground, non-minimized state. This establishes that limited
state parity for one instrumented Debug run, not minimized, activity-switch or
full lifecycle parity. The private session logged an unavailable portal and
virtual graphics warnings. It reported no SQL warnings in that run. No Release
resource measurements were made in that diagnostic run. The [context comparison](P4-arch-preflight-data/context-validation.log)
and [window role comparison](P4-arch-preflight-data/row-validation.log) record
the validator results.

One subsequent uninstrumented Release feasibility run used the same two-Trash
fixture in a fresh private session. An independent TaskManager observer confirmed
the controlled Dolphin window, then exited before a 20-second warmup and a
30-second external `/proc` sampling window. The [31 one-second samples](P4-arch-preflight-data/release-aa1-samples.csv)
showed no Dock CPU tick increase and PSS between 168,222 and 168,286 KiB. The
[observer output](P4-arch-preflight-data/release-aa1-observer.log) records its
window check. The Release binary SHA-256 was
`1032b0f24498e881e372ca7fea924b22896ef5f3ef38fb03fd0f58295c7c879e`.
Mesa/EGL and Zink diagnostics remained in the private virtual graphics stack.
This single idle observation establishes only that the external sampling chain
ran; it is not an A/A variability estimate, cannot verify both trackers inside
the uninstrumented process, and does not establish a CPU or PSS saving.

A subsequent bounded event-control pilot attempted to observe activation,
minimization and restoration through an external TaskManager observer. The
observer started, initially reported an empty activity and undefined desktop,
then timed out without completing its three-stage check. The [observer log](P4-arch-preflight-data/release-event-pilot-observer.log)
is retained. The run is no-go: it did not establish the event sequence or
collect performance samples. The private processes were cleaned up, and the
active Arch Dock PID remained unchanged. The cause of the observer timeout was
not established.

The first attempt shared the normal temporary lock path and exited before the
test; the process used a private session bus and scratch HOME, and source review
shows the lock-conflict path returns before applying import options. Its actual
exit code and a pre-run active-layout hash were not captured. A subsequent run
isolated `TMPDIR` as well as HOME, XDG paths, D-Bus and Wayland. One private
D-Bus daemon from the first attempt required manual PID-verified cleanup; the
active Dock remained running. The temporary diagnostic QML was not committed.
