# R6 partial containment restore retry and startup pilot

Base revision `93a8c0eab`. The
[previous readiness fix](R6-layoutmanager-restore-readiness.md) guards
completed restores until QML readiness is published. A factory call can
also fail partway through a restore. The earlier successful calls have
already attached their containers, but the readiness timer has not
started. The old retry recreated the successful containers and the
duplicate IDs could hide the wrong visual order during save.

The retry now reuses live containers already attached to the main layout.
It inserts the missing items in saved order and avoids a lookup pass on
the initial attempt. While a restore retry is pending, the independent
repair timer leaves missing containers to restore so it cannot overwrite
their positions using a partial visual order. Repair remains available once
the retry limit is reached. The production-path regression fails one factory
call at the first, middle or last position, invokes repair before the retry,
and checks saved order, container count and reuse of the original items.
The middle-position case reproduced a saved order of `10;30;20` on the old
code after five containers had been created for three applets.

## Isolated retry comparison

The GCC Release containment plugin was built before and after the change
with the same build options. Only this plugin was swapped in the scratch
prefix. The probe returned no container once for a middle applet on each
dock, then allowed the production retry to run. Both variants started from
an independently regenerated copy of the same two-output fixture. One
earlier pair was discarded because the second variant inherited the layout
written by the baseline.

| Variant | Created containers per six-applet dock | Saved order on first dock | Saved order on second dock |
| --- | ---: | --- | --- |
| Before | 11 | `2,3,5,6,7,4` | `21,22,24,25,26,23` |
| After | 6 | `2,3,4,5,6,7` | `21,22,23,24,25,26` |

Both variants exited cleanly. The count compares work under an injected
failure. It does not measure how frequently such retries occur on desktops.

## Release startup pilot

The GCC 16.2.1 Release reuse build, before the repair guard, ran in a private D-Bus session with
KWin 6.7.6, Qt 6.11.3, software Qt Quick rendering and the QML disk cache
disabled. The synthetic layout used two 1600 × 900 virtual outputs, six
applets per output and independent docks. Five launches followed one
warm-up launch. Each completed a graceful exit, kept KSycoca unchanged
and restored the expected six IDs on each dock.

The staging prefix instrumented applet reparenting and `anchors.fill`
with millisecond-resolution timestamps. The restore marker means the
expected layout order has been published. The frame marker is the first
Qt Quick `frameSwapped` after that marker on each dock. This is a
client-side frame marker. The instrumentation and the reduced services
in the private session make these characterization runs unsuitable for
comparing desktop startup latency. The private bus had no Plasma Activities
service, so some applets logged missing-activity-database warnings.

All values are milliseconds.

| Run | First dock reparent | First dock anchors | Second dock reparent | Second dock anchors | Launch to both restores | Launch to both post-restore frames |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 163 | 74 | 27 | 4 | 1449 | 1538 |
| 2 | 156 | 74 | 29 | 6 | 1504 | 1593 |
| 3 | 158 | 76 | 28 | 5 | 1445 | 1534 |
| 4 | 156 | 77 | 30 | 3 | 1475 | 1564 |
| 5 | 152 | 77 | 31 | 3 | 1448 | 1534 |
| Median | 156 | 76 | 29 | 4 | 1449 | 1538 |

The first dock consistently paid most of the attachment cost. This pilot
uses one revision and does not establish a normal-startup speedup.

## Per-plugin attachment and reversed initialization

A follow-up probe logged each container's plugin, reparent interval and
anchor interval. It retained the restore and frame markers above. Five
launches followed one warm-up in each of two series. The reversed series
swapped both containment and applet IDs, so the original second dock and
screen were initialized first. Merely reordering the configuration sections
or swapping only `lastScreen` did not change initialization order and those
runs are excluded.

The table reports median reparent/anchor milliseconds. The first and second
columns refer to initialization order, not a physical output.

| Plugin | Normal first | Normal second | Reversed first | Reversed second |
| --- | ---: | ---: | ---: | ---: |
| `org.kde.plasma.kickerdash` | 156 / 0 | 27 / 0 | 151 / 0 | 27 / 0 |
| `org.kde.plasma.systemmonitor.memory` | 0 / 71 | 0 / 2 | 0 / 72 | 0 / 3 |

Every other plugin operation took at most 1 ms. Both docks restored their
expected order in every included run. Median launch-to-both-restores was
1433 ms in normal order and 1438 ms in reversed order; launch-to-both-frames
was 1515 ms and 1514 ms respectively. These are characterization runs, not
an optimization A/B.

The cost follows the first initialized instance of the two applets, not a
particular dock or screen. The probe found no redundant LayoutManager
reparent or anchor operation to remove: the intervals are dominated by
work triggered inside the hosted applet when it acquires its required
parent and anchors. Consequently this slice makes no normal-startup QML
lifecycle change. The shipped optimization is limited to avoiding duplicate
container creation on the verified partial-retry path.

## Final validation

GCC and Clang Debug and Release completed full builds with no compiler
warnings. Each preset passed all 68 CTest targets. The production-path unit
test covers first, middle and last factory failures, repair interleaving,
container identity and count, saved order and the final visual child order.

The user-mode Debug installation restarted through the graphical-session
user manager. DP-1 restored order `43,58,71,65,63,61,40,56,41`; HDMI-A-1
restored `42,45,46,49,51,50,44,48,47` with the complete nine-entry clone
mapping. There was no restore retry, factory failure, fatal message or
assertion. The log retained two known Plasma brightness warnings and emitted
one KWin panel-tracker geometry warning outside this restore path. Production
QML did not change for the restore fix, so no new deep-lint baseline
comparison was run for that change.

## Context-menu separator follow-up

The user's desktop retest passed the restore cases but found that adding a
launcher-internal separator briefly moved the following two icons far to the
right. Geometry probes ruled out a full-width separator delegate and recorded
parabolic expansion, including a 76 to 113 px task slot before recovery.

Two attempted fixes targeted external drag cleanup and scale suppression.
They did not remove the symptom. A final state probe recorded no drag-enter,
drop or `containsDrag` transitions at all, establishing that the user's action
uses the task context menu instead. Those drag changes were reverted in full.

Both menu actions call `addInternalSeparatorAtPos()` through head/tail helpers.
An open menu sets `restoreZoomIsBlocked`, so the usual `invkClearZoom()` entry
point intentionally returns without resetting the enlarged task slots. The
new `addLauncherSeparatorAtPos()` wrapper emits the local `sglClearZoom()`
directly before inserting the model row. This resets the menu-pinned geometry
without changing global menu or hover policy.

The QML smoke regression executes the production wrapper, verifies the clear
signal precedes insertion, and checks that both head and tail actions use it.
The older source contract was updated to the wrapper as well. Focused QML
smoke, source-contract and errors-only lint checks pass. All four build
presets completed with zero C/C++ compiler warnings and passed 68/68 CTest
targets. The user-mode Debug installation restarted with eight mapped applet
rows on both screens and no QML or restore errors; only the known KWin
panel-tracker warning was present. The menu-driven visual retest is pending.

## Positioned separator insertion follow-up

The menu retest exposed a second transient state: adding a separator beside a
non-trailing pinned launcher first displayed it at the pinned group's far
edge, then moved it to the requested position. The old path appended through
`requestAddLauncher()` and relied on `TaskItem` to call `tasksModel.move()`
about 50 ms later.

The new positioned path publishes the requested index in the same model
update. Independent `addLauncherAtPosition()` and
`addLauncherToModelAtPosition()` functions leave every legacy QML signature
and the ordinary pin/trailing-separator behavior unchanged. A matching
`SyncedLaunchers::addLauncherAtPosition()` slot forwards the index to every
client; older containment hosts use the original delayed-move fallback.

The QML smoke case checks immediate order and no delayed-move signal, while
the C++ launcher test checks positioned dispatch to both clients. A real
Plasma `TasksModel` probe also preserved `Dolphin, Kate, Separator, Konsole`
immediately and after `syncLaunchers()`. Focused tests and four-file QML lint
pass. All four build presets passed 68/68 CTest targets with no C/C++
compiler warnings. The user-mode Debug installation restarted with eight
mapped rows on both screens and no QML, positioned-dispatch or restore errors.
The desktop retest is pending.

## Crash and complete separator revert

The positioned candidate crashed during the desktop retest at 00:13:02 on
2026-11-25 (PID 261635, SIGSEGV). The preserved core's batch GDB backtrace
starts at `QSortFilterProxyModel::rowCount()` and enters
`TaskManager::TasksModel::data()` from QML. This is consistent with unsafe
reentrant model access when `launcherList` is reset synchronously on every
synced client. The old delayed move also exists to avoid multi-dock model
reentrancy, so passing unit tests and the single-model probe did not establish
that immediate replacement was safe.

Following the two-failure stop rule, all separator candidates were reverted:
menu zoom clearing, positioned insertion, sync dispatch, and their new tests.
The rejected diff is retained outside the repository as
`/tmp/latte-positioned-separator-rejected.patch`. LayoutManager restore-retry
work and the new AGENTS.md rules are unaffected.

Focused QML smoke, launcher, source-contract and containment-layout tests pass
on the reverted sources. The user-mode Debug dock restarted successfully with
eight mapped rows on both screens and no startup warnings or errors. After the
revert, all four build presets completed without C/C++ compiler warnings and
passed 68/68 CTest targets. The earlier separator candidates remain reverted;
the later desktop request resumed that work with the delayed-placement
approach below.

## Drag-completion launcher order regression

Desktop acceptance preserved startup order, ordinary applet edits and delayed
placement, but the primary and secondary task launchers diverged. The primary
saved `Dolphin, Code` while the secondary retained `Code, Dolphin`; no final
sync followed that primary change. Intermediate model notifications publish
order only during `inDraggingPhase`, while drag completion saved the committed
order without publishing it. A notification arriving after the dragging state
ended could therefore leave the peer at its previous order.

Drag completion now publishes the committed order immediately after saving it.
The production QML change is one call and does not alter launcher-list resets,
separator insertion or model moves. A source contract for the drag-completion
call failed before the change and passes after it; the existing QML smoke test
verifies the published model order and activity filtering. Focused QML smoke,
source-contract and launcher tests pass, as does errors-only lint for the
changed QML file. All four build presets completed without C/C++ compiler
warnings and passed 68/68 CTest targets. The user-mode Debug dock restarted
with seven mapped applet pairs and no startup warnings or errors. The user
confirmed both synchronization directions, repeated moves and persistence
across restart.

## Separator delayed placement

The remaining visual defect came from publishing the appended separator's
layout slot before the existing 50 ms delayed move placed it at the requested
index. The centered task list temporarily expanded at the wrong edge and then
returned to the correct final order. The fix forwards the requested index to
all synced clients and suppresses only the pending separator's layout slot
until that delayed move completes. It preserves the legacy `addLauncher()`
entry point and never assigns a replacement `launcherList`, avoiding the model
reentrancy of the rejected immediate-position candidate.

Regression checks cover positioned dispatch to both clients, forwarding from
the production launcher ability, the syncer's delayed-move request, zero slot
width while the position is pending and restoration after the move. Focused
QML smoke, source-contract and launcher tests pass, as does errors-only lint
for all five changed QML files. All four build presets completed without
C/C++ compiler warnings and passed 68/68 CTest targets. The user-mode Debug
dock restarted with seven mapped applet pairs and no startup warnings or
errors.

The final desktop revision replaces pending arrays instead of mutating them
in place, retains the pending state through `tasksModel.move()`, avoids
publishing the temporary append order and disables the displaced transition
while launcher moves are pending. The user confirmed continuous insertion on
both screens, single insertion, deletion and restart persistence. No temporary
diagnostic logging remains in the production changes.
