# R6 applet factory profile and identity lookup

Base: `4a2f294a8` on `main`. The preceding
[dual-dock Release A/B](R6-layoutmanager-startup-ab.md) identified container
creation and attachment as the largest measured startup phase. This slice
uses its Gentoo two-output fixture and Release executable and instruments
the installed QML in a scratch prefix. No instrumentation ships.

## Factory profile

Two runs using separate nested KWin and D-Bus sessions each produced five
samples after discarding their first launch. Each sample restores two docks,
six applets per dock, with the expected saved order and no KSycoca rewrite.
The QML probes use `Date.now()` and log with `--debug`; the data locate
millisecond-scale work under this fixture and do not provide a startup A/B
comparison.

Sum each stage over the twelve containers in a launch, then take the median
over five launches. For the factory, creating the `AppletItem` wrapper took
41 ms, its initial identity assignments 5 ms, and initialization 188 ms.
The final opacity binding was below the timer's millisecond resolution in
four launches and took 1 ms in one launch.

The second probe splits initialization into identity and resolution
(both medians below resolution), assigning the resolved item (5 ms),
reparenting it (86 ms), and filling its anchors (95 ms). The raw
reparenting/anchor pairs in milliseconds are 86/97, 90/94, 90/95, 85/91, and
84/97. The launcher and memory monitor account for most of those operations.
The cost is incurred synchronously across the Plasma and Qt Quick boundary.
Moving the QML factory to C++ would still have to attach the existing applet
and preserve the live size and visibility bindings.

## Change and validation

The factory now delegates initial identity assignment to
`initAppletContainer()`. It used to assign the same reference and resolve
the plugin name immediately before calling that method, which repeated both
operations before reaching the same readiness check. Identity is still
published before resolving the visual item so pending containers remain
addressable to repair and removal. The QML method signatures and
retry/opacity paths are kept.

An independent before/after counter instrumented every entry to
`appletPluginName()`, including recursion into the backend object. Each
version restored the same twelve applets with the same names and saved
orders and quit cleanly. Lookup entries dropped from 48 to 24 (four to two
per container). There was one instrumented launch per variant, so its wall
timing is not a performance result.

GCC and Clang Debug and Release built with zero compiler warnings, and each
preset passed 68/68 CTest targets. The syntax lint passed all 244 QML files.
Deep lint matched the previous local Qt 6.11.3 report with zero diagnostics
added or removed. The checked-in Qt 6.11.1 fingerprint comparison remains
open.

## Desktop and clone-order follow-up

The user-mode Debug install restarted in the graphical session. The primary
Dock preserved its prior order. The desktop clone published a different ID
sequence after restart, with its plugin order matching the primary Dock. The
fresh log contained no warning, error, assertion or factory failure.

A controlled comparison used the same cloned-layout snapshot and isolated
dual-output KWin sessions. Both QML variants started and restored twelve
applets. Three repeated baseline launches with identical input also produced
three different clone orders while preserving the primary order. An
additional before/after pair still reported different clone orders after
holding each session for 3.5 seconds beyond the factory markers. This
reproduces the clone-order instability with the old factory and keeps its
diagnosis separate from the identity lookup change. The performance fixture
above uses independent docks.

## Task reorder and separator retest

A follow-up desktop retest reported a brief disappearance of some task
icons when inserting separators or dragging to reorder. An isolated nested
session traced the row moves followed by Plasma model resets during
`syncLaunchers()`. The fix keeps the base icon visible underneath the drag
highlight and skips entrance animation for delegates recreated while the
launcher order is being saved. The isolated before/after trace retained the
same final order, kept the dragged icon at opacity 1, and confirmed that
delegates recreated during order synchronization were not set to opacity 0.
The user confirmed both operations on the desktop. The desktop log recorded
one invalid-context `QQmlExpression` warning, also present in the pre-fix
session.

GCC and Clang Debug and Release each passed all 68 tests, including the
launcher order synchronization smoke check and the handbook check. The
244-file syntax lint passed, and the local Qt 6.11.3 deep lint comparison
showed no diagnostic changes.

## Repeated-applet clone identity validation

The remaining repeated-plugin case was validated on `main` at
`bde6d626c97cf31fc8140f0521da655ff932b950`, using the current GCC Release
build in an isolated prefix and two virtual outputs. The fixture started from
the working cloned desktop layout and placed two `binaryclock` instances at
adjacent logical positions. Original IDs 65 and 77 carried the locked-zoom and
disabled-coloring markers respectively. The input was regenerated for each of
three launches so no run inherited an earlier write-back.

Clone IDs were recreated in every launch. The complete six-applet clone order
matched the source order after translation in all three runs:

| Run | First binary clock | Second binary clock | Clone locked zoom | Clone disabled coloring |
| --- | --- | --- | --- | --- |
| 1 | `65 -> 44` | `77 -> 45` | 44 | 45 |
| 2 | `65 -> 46` | `77 -> 47` | 46 | 47 |
| 3 | `65 -> 42` | `77 -> 46` | 42 | 46 |

An automated check also verified the binary-clock plugin at both mapped IDs,
all six mapped order entries and both preference identities. KSycoca remained
unchanged and every dock exited through its signal-driven teardown. The
private sessions emitted the expected portal and activity-service warnings;
no Fatal, ASSERT, ReferenceError or TypeError was found. Harness files and
logs are outside the repository under `/tmp/latte-repeated-clone`; production
QML and C++ were not instrumented or changed.

This closes the repeated-plugin identity validation. It provides no evidence
of a remaining mapping defect or a justification for another startup timing
change.
