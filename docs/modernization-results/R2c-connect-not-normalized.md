# R2c-connect-not-normalized: Unnormalized Type Names in `Q_ARG`

Status: Pending validation. Merged into `main` in `fc8b803d0`; the checks on
`main` and the combined desktop smoke passed ([R2c fix batch 1](R2c.md#fix-batch-1)).
CI run 40508173790 stopped its Build jobs at the KDE Neon key download, so the
clazy gate has not run there yet. No automated test reaches the ten calls.
Date: 2026-10-27. Branch: `claude/r2c-batch-b-c113e5`. Start HEAD: `09b030e61`.
Implementation commit: the commit that adds this report. Part of [R2c](R2c.md),
the third slice of batch B (after `R2c-empty-qstringliteral` and
`R2c-use-static-qregularexpression`).

## Defect

The ten findings are not `connect()` strings. All are `Q_ARG(<pointer type> *, value)`
arguments of `QMetaObject::invokeMethod()` or `QMetaMethod::invoke()`, spelled with
a space before the `*`. clazy reports `Signature is not normalized. Use QObject*
instead of QObject *` (or `QQuickItem*`). `Q_ARG` turns its type argument into the
name string that `invokeMethod()` uses to build the method signature, so the old
spelling produces `hideAppletItem(QObject *)` while moc stores the normalized
`hideAppletItem(QObject*)`. The first `indexOfMethod()` lookup with the written name
fails (-1 in the table below on Qt 6.4.2 and 6.11.2) and Qt needs a second step to
find the method. The calls work, so nothing is broken; they depend on that second
step on every call.

| File | Lines (before) | Type | Method |
| --- | --- | --- | --- |
| `app/declarativeimports/contextmenulayerquickitem.cpp` | 548, 562, 577 | `QObject *` | `hideAppletItem`, `showAppletItem`, `removeAppletItem` on the layout manager (the applet context menu's Remove and its Undo) |
| `app/view/containmentinterface.cpp` | 1129 | `QObject *` | `processMimeData(QObject*,int,int)` of the containment, in `addApplet()` |
| `app/view/containmentinterface.cpp` | 2293, 2308 | `QObject *` | `addAppletItem(applet, index)` in `onAppletAdded()` |
| `app/view/containmentinterface.cpp` | 2472 | `QObject *` | `showAppletItem` when the removal is undone, in `onAppletAdded()` |
| `app/view/view.cpp` | 1577, 1614 | `QQuickItem *` | `insertAtCoordinates` in `updatePlasmoidDrag()` and `handlePlasmoidDrop()` |
| `containment/plugin/layoutmanager.cpp` | 731 | `QObject *` | the containment's `itemFor(QObject*)` in `resolveAppletQuickItemObjectInternal()` |

## Fix

Ten one-character edits: `Q_ARG(QObject *, x)` became `Q_ARG(QObject*, x)` and
`Q_ARG(QQuickItem *, x)` became `Q_ARG(QQuickItem*, x)` on exactly the ten reported
lines. Nothing else on those lines or in those files changed. The gate side is one
gate entry and one fixture, as in every R2c slice: the name in `GATED_CHECKS` and
`autotests/clazy/connect-not-normalized.cpp`, which holds the smallest violation
(`QMetaObject::invokeMethod(target, "take", Q_ARG(QObject *, argument))`). The gate
now has 34 entries and 34 fixtures.

Left unchanged: three `Q_RETURN_ARG(<T> *, ...)` arguments carry the same spelling
(`containment/plugin/layoutmanager.cpp:730` and `:736`, `app/alternativeshelper.cpp:115`).
clazy 1.17 does not report them, so the gate does not need them; the last one is
outside this slice's file list. If a later clazy starts to inspect `Q_RETURN_ARG`, the
same edit applies. One call in `layoutmanager.cpp` now mixes both spellings
(`Q_RETURN_ARG(QObject *, ...)` next to `Q_ARG(QObject*, ...)`), which is harmless.

## Proof that the new strings are the normalized old ones

A wrong string can break a call without a compile error, so each of the ten
replacements was checked three ways.

1. **The edit is mechanical.** A script compared the old and new text of the four
   files: the changed line numbers equal the ten clazy locations, and each changed
   line differs from the old one only by the space before the `*`
   (`git diff --stat`: 10 insertions, 10 deletions in 4 files).
2. **The compiler sees the same program.** The four translation units were
   preprocessed from `HEAD` and from the working tree with the flags of the host
   Clang Debug build (Clang 22.1.8, Qt 6.11.2). The outputs have the same number of
   lines and differ in exactly ten of them, one per site, and each difference is
   `argument<QObject *>("QObject *", x)` against `argument<QObject*>("QObject*", x)`
   (`QQuickItem` in `view.cpp`). The template argument is the same type; only the
   name string differs.
3. **Qt resolves the same method.** The scratch program `proof.cpp` (not in the
   repository) uses the real `Q_ARG` macros in the ten old and new spellings and
   prints the strings they produce, what `QMetaObject::normalizedType()` and
   `normalizedSignature()` make of the old ones, and which method `indexOfMethod()`
   finds, then calls the stand-in method both ways. Its output follows.

### Output of the proof program, Qt 6.11.2

The stand-in class declares the invoked methods as `layoutmanager.h` does:
`insertAtCoordinates(QQuickItem*, const int&, const int&)` is stored by moc as
`insertAtCoordinates(QQuickItem*,int,int)`, and both overloads of `addAppletItem` are
present, so the right one must be chosen. The first table is the type string of each
`Q_ARG`; the second is the method signature `invokeMethod()` builds from the strings
of one call. In `indexOfMethod`, -1 means not found.

| Site | Old type string | `normalizedType(old)` | New type string | Equal |
| --- | --- | --- | --- | :---: |
| `contextmenulayerquickitem.cpp:548` | `QObject *` | `QObject*` | `QObject*` | yes |
| `contextmenulayerquickitem.cpp:562` | `QObject *` | `QObject*` | `QObject*` | yes |
| `contextmenulayerquickitem.cpp:577` | `QObject *` | `QObject*` | `QObject*` | yes |
| `containmentinterface.cpp:1129` | `QObject *,int,int` | `QObject*,int,int` | `QObject*,int,int` | yes |
| `containmentinterface.cpp:2293` | `QObject *,int` | `QObject*,int` | `QObject*,int` | yes |
| `containmentinterface.cpp:2308` | `QObject *,int` | `QObject*,int` | `QObject*,int` | yes |
| `containmentinterface.cpp:2472` | `QObject *` | `QObject*` | `QObject*` | yes |
| `view.cpp:1577` | `QQuickItem *,int,int` | `QQuickItem*,int,int` | `QQuickItem*,int,int` | yes |
| `view.cpp:1614` | `QQuickItem *,int,int` | `QQuickItem*,int,int` | `QQuickItem*,int,int` | yes |
| `layoutmanager.cpp:731` | `QObject *` | `QObject*` | `QObject*` | yes |

| Site | API | Signature built from the old names | `normalizedSignature(old)` | Signature built from the new names | Equal | `indexOfMethod`: old / normalized / new | Same call |
| --- | --- | --- | --- | --- | :---: | --- | --- |
| `contextmenulayerquickitem.cpp:548` | `invokeMethod` | `hideAppletItem(QObject *)` | `hideAppletItem(QObject*)` | `hideAppletItem(QObject*)` | yes | -1 / 7 / 7 | `hideAppletItem(applet)` |
| `contextmenulayerquickitem.cpp:562` | `invokeMethod` | `showAppletItem(QObject *)` | `showAppletItem(QObject*)` | `showAppletItem(QObject*)` | yes | -1 / 8 / 8 | `showAppletItem(applet)` |
| `contextmenulayerquickitem.cpp:577` | `invokeMethod` | `removeAppletItem(QObject *)` | `removeAppletItem(QObject*)` | `removeAppletItem(QObject*)` | yes | -1 / 6 / 6 | `removeAppletItem(applet)` |
| `containmentinterface.cpp:1129` | `QMetaMethod::invoke` | `processMimeData(QObject *,int,int)` | `processMimeData(QObject*,int,int)` | `processMimeData(QObject*,int,int)` | yes | -1 / 11 / 11 | `processMimeData(applet,11,12)` |
| `containmentinterface.cpp:2293` | `invokeMethod` | `addAppletItem(QObject *,int)` | `addAppletItem(QObject*,int)` | `addAppletItem(QObject*,int)` | yes | -1 / 5 / 5 | `addAppletItem(applet,3)` |
| `containmentinterface.cpp:2308` | `invokeMethod` | `addAppletItem(QObject *,int)` | `addAppletItem(QObject*,int)` | `addAppletItem(QObject*,int)` | yes | -1 / 5 / 5 | `addAppletItem(applet,3)` |
| `containmentinterface.cpp:2472` | `invokeMethod` | `showAppletItem(QObject *)` | `showAppletItem(QObject*)` | `showAppletItem(QObject*)` | yes | -1 / 8 / 8 | `showAppletItem(applet)` |
| `view.cpp:1577` | `invokeMethod` | `insertAtCoordinates(QQuickItem *,int,int)` | `insertAtCoordinates(QQuickItem*,int,int)` | `insertAtCoordinates(QQuickItem*,int,int)` | yes | -1 / 9 / 9 | `insertAtCoordinates(applet,7,9)` |
| `view.cpp:1614` | `invokeMethod` | `insertAtCoordinates(QQuickItem *,int,int)` | `insertAtCoordinates(QQuickItem*,int,int)` | `insertAtCoordinates(QQuickItem*,int,int)` | yes | -1 / 9 / 9 | `insertAtCoordinates(applet,7,9)` |
| `layoutmanager.cpp:731` | `invokeMethod` | `itemFor(QObject *)` | `itemFor(QObject*)` | `itemFor(QObject*)` | yes | -1 / 10 / 10 | `itemFor(applet)` |

`containmentinterface.cpp:1129` finds its method first with the already normalized
string `processMimeData(QObject*,int,int)` and then calls `QMetaMethod::invoke()` on
that method, so no lookup there uses the `Q_ARG` name; its signatures in the table are
what `invokeMethod()` would build, and the last column shows that `invoke()` accepts
both spellings.

The same program on the Ubuntu 24.04 container's Qt 6.4.2 prints the same ten rows
(only the method indices differ) and `result: all ten sites equivalent`. Qt 6.6, the
declared floor, was not run.

Negative controls with the same program: `addAppletItem(QObjectt*,int)` (a misspelled
type name that still compiles through a `using`) and `addAppletItem(QObject**,int)`
are not found by `indexOfMethod()`, and on Qt 6.4.2 `invokeMethod()` fails for both
(`No such method`, no call reaches the stand-in). On Qt 6.11.2 the misspelled-name
call still succeeds, because the macro also passes the C++ type (the expansion above),
while the `QObject**` call fails. So a wrong string is visible to the table and breaks
a call on a Qt 6.4-style lookup without any compile error.

## Evidence

Container commands are those of [R2c](R2c.md#reproducing-the-environment), with a
private build directory (`~/r2c-build-batch-b`) and `--jobs 6` because another
session ran at the same time.

| Command | Result |
| --- | --- |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 6 --inventory connect-not-normalized` before the fix | 10 findings in 4 files, as in R2c |
| Same after the fix | Exit 0 in 105 s: 0 findings in 153 sources |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 6` | **Exit 0** in 124 s: 34 fixtures rejected by their own checks, 153 sources with no finding from 34 checks |
| Same, with the previous versions of the four files mounted over the fixed ones, no rebuild | **Exit 1** in 124 s: the ten findings, `contextmenulayerquickitem.cpp` 548:43, 562:51, 577:51, `containmentinterface.cpp` 1129:26, 2293:39, 2308:47, 2472:47, `view.cpp` 1577:31, 1614:39 and `layoutmanager.cpp` 731:36 |
| Host `cmake --build --preset gcc-debug -j6`, `ctest --preset gcc-debug` (GCC 16.2.1) | Exit 0, 0 compiler warnings, 57/57 passed; the four files were rebuilt in the application, the plugins and `containmentlayoutunittest` |
| Host `cmake --build --preset clang-debug -j6`, `ctest --preset clang-debug` (Clang 22.1.8) | Exit 0, 0 compiler warnings, 57/57 passed; same objects rebuilt |
| Mechanical diff check, preprocessed diff of the four translation units, proof program on Qt 6.11.2 and 6.4.2 | As described above |

The build logs hold thousands of `Warning:` lines from the qmllint step; they are
the known QML warnings tracked by the baseline gate, not compiler output, and this
change touches no QML.

## Handoff

- **Files and behavior changed:** `app/declarativeimports/contextmenulayerquickitem.cpp`
  (3 lines), `app/view/containmentinterface.cpp` (4), `app/view/view.cpp` (2),
  `containment/plugin/layoutmanager.cpp` (1), `scripts/test-clazy.py` (one name),
  `autotests/clazy/connect-not-normalized.cpp`. The same methods are invoked with the
  same arguments; the first lookup now succeeds.
- **State authority / lifetime constraints:** none; the strings are literals.
- **Tests:** no test added or changed; `ctest` 57/57 on Clang and GCC Debug. No
  automated test reaches these ten calls (they run inside the live dock), so the
  evidence is the proof above plus the fixture and the pre-fix gate run.
- **Smoke plan:** these actions reach the ten sites. Remove a
  widget from its context menu and press Undo in the Plasma notification, then
  remove one for good (`contextmenulayerquickitem.cpp` 548, 562, 577 and
  `containmentinterface.cpp` 2472). Drag a widget from the widget explorer onto the
  dock and drop it, once between two widgets and once on an empty part of the dock
  (`view.cpp` 1577, 1614; `containmentinterface.cpp` 1129, 2293, 2308). Add a widget
  without a drag, for example from the Add Widgets dialog (2308, the default index).
  `layoutmanager.cpp:731` is the last fallback of the applet item lookup and runs only
  when every earlier lookup fails, so the smoke may not reach it. The debug log should
  hold no `QMetaObject::invokeMethod: No such method` line.
- **Desktop smoke result** ([R2c fix batch 1](R2c.md#fix-batch-1)): the user
  ran these actions on the host and in the Fedora VM and reported that all
  passed. The host log shows a removal undone, which passes 548 and then 562
  and 2472 through `destroyedChanged(false)`, and a removal that expired, which
  passes 548 and then 577. It also shows one widget added at index 1. That is
  the default index of that dock too, so the log does not tell which of the
  drop and add sites ran. The Fedora log shows no widget event. Neither log has
  a `No such method` line or any warning from these calls; nothing shows
  whether `layoutmanager.cpp:731` ran.
- **Combined validation:** on `main` at `fc8b803d0`, GCC and Clang Debug and
  Release with 0 warnings and 57/57 tests each, the 37-check gate and the
  Debian 13 build on Qt 6.8.2 passed.
- **Not run:** the CI Build jobs, which hold the clazy gate (run 40508173790 at
  `eba91a3e6` stopped them at the KDE Neon key download, before compiling), Qt
  6.6.
- **Next exact action:** confirm the Build jobs of the next CI run, then mark
  this slice Complete.
- **Commit approval status / push approval status:** committed on the feature
  branch; merged into `main` in `fc8b803d0` with the user's approval; not
  pushed.
