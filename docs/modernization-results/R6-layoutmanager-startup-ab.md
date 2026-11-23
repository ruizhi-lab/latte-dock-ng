# R6 dual-dock Release startup A/B

A is `9cf6c8059` and B is `f336632e6`. The latter is the pushed
containment restore index. The main executable, QML, resources and other
plugins within each platform are identical. Only the containment plugin
changes. Each side is rebuilt with the platform's GCC Release flags and the
same `steady_clock` probe. The baseline's private helper had no moc or layout
effect, so both variants use the same generated registration and moc objects.

## Method

Two independent docks each restore six saved applets. The fixture is derived
from a two-dock desktop layout. The clone relationship is removed for the
test, connector aliases are mapped to the two nested outputs, and both docks
restore their own saved order. Gentoo uses a fresh private D-Bus session, a
virtual KWin with two 1600 × 900 outputs, Qt Quick software rendering and
disabled QML disk cache. The debug pilot verified both outputs and saved
orders. Timed runs omit `--debug`. Each series discards the initial B/A warmup
pair and alternates AB/BA over twelve pairs.

The harness checks the loaded plugin, two distinct containment managers, six
applets per manager, successful exit and unchanged KSycoca file identity and
mtime across each run. Early pilots with mismatched KDE menu environments
rewrote KSycoca and are excluded. Matching the KDE environment for
`kbuildsycoca6` and the dock made the cache stable. No accepted sample retried
restore.

The enumeration timer includes every call to `plasmoidApplets()` during
the readiness and ordering phase. Ordering time is the rest of that phase,
including the configured count check. Creation time includes QML container
creation and attaching the items. Phase times sum both docks within each
launch. Whole-launch time starts before spawning the process and ends at the
last successful restore immediately after the saved order and options were
processed. It excludes first paint measurement and the delayed
`hasRestoredApplets` notification.

## Gentoo host

GCC 16.2.1, Qt 6.11.3 and KWin 6.7.6. 24 measured pairs across two separate
nested sessions. All durations are milliseconds. The 95% intervals are
percentile bootstrap intervals on the median paired delta (20,000 resamples);
the summary script uses a fixed seed.

| Phase | A median | B median | Paired B − A | 95% interval | B faster |
| --- | ---: | ---: | ---: | ---: | ---: |
| Applet enumeration | 0.1006 | 0.0231 | −0.0773 | −0.0782 to −0.0755 | 24/24 |
| Readiness and order outside enumeration | 0.0329 | 0.0213 | −0.0118 | −0.0129 to −0.0110 | 24/24 |
| Enumeration and ordering combined | 0.1333 | 0.0449 | −0.0887 | −0.0906 to −0.0870 | 24/24 |
| Container creation and attachment | 240.452 | 241.646 | +3.660 | −1.061 to +6.849 | 9/24 |
| Entire restore method | 251.880 | 252.461 | +4.893 | −1.828 to +6.611 | 11/24 |
| Process launch to both restores | 1,426.521 | 1,416.212 | −5.238 | −20.589 to +2.507 | 14/24 |

Enumeration went from 13 calls per dock to 1 in every pair. The combined
enumeration and ordering median fell by about 66%. Per-series paired launch
deltas were −10.31 ms and +4.01 ms, so the whole-launch improvement did not
replicate. QML container creation and attachment accounts for about 95% of the
summed restore time on this fixture. The enumeration probe incurs two clock
reads per call, so the timing of that phase includes instrumentation overhead
proportional to the call count.

The [first series](R6-layoutmanager-startup-ab-data/gentoo-1/runs.jsonl),
[replication](R6-layoutmanager-startup-ab-data/gentoo-2/runs.jsonl), and
[summary script](R6-layoutmanager-startup-ab-data/summarize.py) are retained.
The next startup investigation should profile the container creation and
attachment phase before choosing a second production change.

## VM replication

The two VM builds and measurements ran in user cache directories and used a
separate install prefix and session from each VM's desktop dock. The VM
fixture uses the same applet types, IDs and order with task launchers cleared.
Each VM ran a two-output debug pilot followed by 12 timed pairs and a
discarded warmup pair. Timed series were run serially to avoid competition
between the VMs for host CPU. Build dependencies were installed with user
authorization. The GCC Release application and plugins built with zero
compiler warnings on both VMs, and `containmentlayoutunittest` and
`sourcecontracttest` passed on both.

| VM | Toolchain | Enumeration and ordering A → B | Paired B − A (95% interval) | Launch to both restores A → B | Paired B − A (95% interval) |
| --- | --- | ---: | ---: | ---: | ---: |
| Debian 13 | GCC 14.2, Qt 6.8.2, KWin 6.3.6 | 0.1502 → 0.0569 ms | −0.0924 ms (−0.0996 to −0.0798) | 1,647.539 → 1,641.301 ms | +0.270 ms (−18.906 to +20.798) |
| Arch | GCC 16.2.1, Qt 6.12.0, KWin 6.8.3 | 0.1379 → 0.0558 ms | −0.0824 ms (−0.0869 to −0.0792) | 1,364.733 → 1,367.331 ms | +3.990 ms (−2.640 to +15.694) |

Every measured pair on both VMs had 13 enumeration calls per dock on A and
one on B. The combined phase was faster in all 24 VM pairs. Median container
creation and attachment took 296.209 ms on Debian A and 295.870 ms on B,
and 209.152 ms on Arch A and 211.725 ms on B. Both VM whole-launch intervals
span zero, consistent with the host replication. Across all three platforms
the index reproducibly cuts less than a tenth of a millisecond of restore work
for two six-applet docks. The series does not measure first paint, CPU
consumption or memory.

VM records are retained for [Debian](R6-layoutmanager-startup-ab-data/debian/runs.jsonl)
and [Arch](R6-layoutmanager-startup-ab-data/arch/runs.jsonl). Run the summary
script separately per platform to reproduce the results. Its pooled summary
is intended for repeated series on the same platform.
