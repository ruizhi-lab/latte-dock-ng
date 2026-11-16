# Fedora environment comparability review — 2026-11-17

## Version change between run groups

The previous [Fedora continuation record](../../P4-fedora-observer-continuation.md)
reports Fedora 45, Plasma 6.8.2 and CMake 4.3.0, but preserves neither the
version-detection command nor its raw output. Run 2's CSV spans
2026-11-16 10:32:34–10:33:34 CST and run 3 spans 10:37:10–10:38:10 CST. Both
unit journals record boot ID `83dfbae7e37247c08c9aa33ce45541cc`.

DNF transaction 21 began at `2026-11-16 13:50:48` and ended at `13:53:39`; its
history output has no timezone suffix. RPM records Plasma 6.8.3 and KWin 6.8.3
installed at 21:52 CST on Nov 16, after runs 2 and 3. DNF explicitly shows
the 6.8.2 Plasma/KWin packages being replaced by 6.8.3. It also shows Qt
Declarative 6.11.2-2 replaced by 6.11.2-3, and Breeze 6.8.2 replaced by 6.8.3.
Qt Base remained 6.11.2-3, installed on Nov 4.

Transaction 21 installed kernel packages for 7.3.3, but the guest did not
reboot: the current boot ID matches runs 2 and 3, uptime at the check was about
81,247 seconds, and the running kernel remains 7.2.15-300.fc45. The package
update therefore changed the desktop/QML runtime without changing the running
kernel. Runs 2 and 3 are retained as a historical 6.8.2 group and are not
pooled with run 4 onward.

Run 4's wall-clock interval was 2026-11-17 07:24:43–07:25:43 CST. The host
CMake download ended at 07:23:20.617 and unpack ended at 07:23:34.507, before
the run unit began at 07:23:49; it did not overlap the sample interval. Run 4
used Plasma/KWin 6.8.3, Qt Declarative 6.11.2-3, Qt Base 6.11.2-3 and the same
running kernel. It passed the observer, sampling, identity/family, KSycoca,
artifact/helper hash and teardown checks. Runs 4–8 form the five-run valid group in the distinct 6.8.3 environment.

The old record's 6.8.2 package version matches the versions at the run 2/3
timestamps, and the DNF transaction proves the later update. The executable's
own version string could not be checked: `plasmashell --version` from a bare
SSH shell aborted before printing a version. `coredumpctl` identifies PID
24305 as `/usr/bin/plasmashell`, signal 6 (ABRT), package
`plasma-workspace/6.8.3-1.fc45`; it is not a Latte process. The coredump remains
on the guest and was not removed. Its metadata is in
[`plasmashell-coredump-info.txt`](plasmashell-coredump-info.txt), and the exact
probe result is in [`plasmashell-version-probe.txt`](plasmashell-version-probe.txt).
Current RPM/DNF evidence is in [`guest-rpm-dnf.txt`](guest-rpm-dnf.txt). Run 2
and run 3 unit journals are retained beside their raw data.

Across runs 4–8, the source archive, executable, fixture, install manifest,
QML/plugin/type payload and all four control-helper identities match. The build
artifact is unchanged; the Plasma/KWin and Qt Declarative runtime is the
documented environment boundary. Runs 2–3 and 4–8 are separate groups and
their metrics must not be pooled. The current 6.8.3 group has five valid
execution samples; see the baseline report for its descriptive summary and
measurement limits.
