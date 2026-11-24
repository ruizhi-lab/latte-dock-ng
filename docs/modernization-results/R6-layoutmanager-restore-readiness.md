# R6 containment restore during the readiness delay

Base: `0f549ea18`. This slice follows the
[applet index refresh](R6-layoutmanager-index-refresh.md).

`LayoutManager::restore()` attaches the containers, saves the resulting order,
and connects updates before starting the two-second QML readiness timer.
The readiness property stays false during the delay so startup presentation
can settle. A retry queued before a successful restore can still fire during
that delay. Previously it attached the same applets again, reconnected save
signals, and restarted the presentation timer. Saving deduplicated persisted
IDs and could therefore hide the duplicate live containers from config checks.

The active readiness timer now guards completed restore work until
`hasRestoredApplets` takes over. Attempts that still need applets can resume
before the timer starts. The QML signal, delay and ordinary runtime applet
insertion path retain their established sequence.

The persisted legacy-Justify regression fixture now queues a retry with
one unresolved applet, then resolves it and calls restore before the pending
retry fires. The old implementation creates eight containers where four are
expected. The fixed implementation creates four, publishes readiness after
the delay, and retains the saved order and splitter positions.

## Validation

An isolated two-output KWin session called restore again 250 ms after each
six-applet Dock published its order. The readiness property was still false
on both Docks. The old plugin doubled the live container count from six to
twelve per Dock. The fixed plugin kept six, then published readiness once
per Dock. Both variants exited cleanly. A separate fixed run inserted a
separator on the first Dock during the readiness delay and reached readiness
with seven and six containers, respectively.

GCC and Clang Debug and Release built with zero compiler warnings and each
passed all 68 CTest targets. Syntax lint passed on all 244 QML files.

The installed user-mode Debug dock completed initialization on both
physical outputs. The primary order matched the previous session and the
startup log had no warnings or errors. The clone's order differed, as in
the [prior baseline reproduction](R6-layoutmanager-factory-profile.md#desktop-and-clone-order-follow-up).

The user confirmed that widgets remained normal on both screens during the
desktop retest. A temporary task launcher order mismatch was investigated
separately in the task launcher validator.

## Peer task launcher follow-up

The task launcher validator initially waited one 400 ms retry interval before
attempting the first move on the receiving Dock. Two isolated two-output
before/after runs used the same shared launchers and applied one drag-order
change on the sender. The time from receipt to the first receiver move was
373 and 374 ms on the baseline versus 5 ms in both fixed runs. The final
launcher lists matched between Docks in all four launches. The change keeps
the 400 ms interval for follow-up checks and retries a temporarily empty
receiver list for up to five intervals. The QML smoke test exercises both the
immediate move and the case where launchers arrive after validation starts.

All four GCC and Clang build presets passed 68/68 CTest targets, including
the handbook. Syntax lint passed on 244 files and the local Qt 6.11.3 deep
comparison added no diagnostics. The user-mode Debug dock initialized on
both outputs with a clean startup log.

### Separator gaps during peer order persistence

The desktop follow-up confirmed peer order synchronization and reported a
brief movement on either side when icons landed. In a nested two-Dock run
with an internal separator, the receiving Dock moved its row and settled.
The subsequent order commit reset the Plasma model twice. During the
reset, a neighboring slot narrowed from 76 px to 74 px for one sampled
frame and the following icon moved from x=76 to x=74. Both returned to
76 px after the separator index settled.

Order persistence now snapshots nonzero settled left and right separator
gaps and seeds recreated delegates only when their row and launcher
identity match. The snapshot contains values and is scoped to the
synchronous commit. The existing spacer settling timer returns ownership
to the live separator indexes. With the same repro, two model resets still
occurred and all 66 post-reset samples stayed at 76 px for both the slot
width and the following position. Both Docks retained the same final order.
With the separator between two launchers, a separate run kept both adjacent
slots at 76 px in all 67 post-reset samples. The spacer test also verifies
that a value snapshot seeds the replacement gap and yields to the live
spacing after the settling interval.


### Duplicate file-manager launchers

The desktop replica displayed two persistent Dolphin icons after a task
reorder. Its saved task list contained both `preferred://filemanager` and
`applications:org.kde.dolphin.desktop`. The task-row remapping experiment
was reverted. Restore and import now deduplicate identical normalized
records while preserving distinct activity assignments. A synchronized
add or drag-to-pin checks the same normalized identity before adding a
global launcher, and drag reorder recognizes an existing equivalent pin.

With Dolphin configured as the default file manager in an isolated dual-Dock
fixture, the old path showed two Dolphin entries after an add. The fixed
path restored the duplicate fixture to one entry and still had one visible
Dolphin on each Dock after repeating the add. The QML smoke test covers
normalizing the duplicate, rejecting a second pin and allowing a different
launcher to be added.

The desktop Debug restart migrated each Dock's saved list to one
`preferred://filemanager` record. Both Docks initialized with no startup
warning or error. GCC and Clang Debug and Release each passed 68/68 CTest
targets, and the QML syntax check passed on all 244 files.

The user confirmed normal task sorting on the desktop after the rollback
and launcher deduplication.
