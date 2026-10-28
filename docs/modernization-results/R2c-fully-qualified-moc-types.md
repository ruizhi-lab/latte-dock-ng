# R2c-fully-qualified-moc-types: Signal and Slot Parameter Types Spelled in Full

Status: Pending validation. Merged into `main` in `382e55cd2`; the checks on
`main` and the host desktop smoke passed ([R2c fix batch 2](R2c.md#fix-batch-2)). The CI run
waits for a push. No automated test looks up one of the 21 signatures.
Date: 2026-10-28. Branch: `claude/r2c-batch-d-a6a9f7`. Start HEAD: `eba91a3e6`
(the start of batch D; this slice follows `R2c-const-signal-or-slot` and
`R2c-incorrect-emit`). Implementation commit: the commit that adds this report.
Part of [R2c](R2c.md), the third slice of batch D.

## Defect

moc stores a parameter type as it is written, and it matches that text, not the type,
against `Q_DECLARE_METATYPE` and against the signatures that string-based lookups
build. clazy's `fully-qualified-moc-types` reports a signal or slot whose parameter
type is written relative to the enclosing namespace. The 21 findings are in nine
headers and have three shapes:

| Header | Declarations (before) | Spelling to |
| --- | --- | --- |
| `app/wm/abstractwindowinterface.h` | signals `activeWindowChanged`, `windowChanged`, `windowTitleChanged`, `windowAdded`, `windowRemoved` and slot `windowRemovedSlot`, all `WindowId` (6) | `Latte::WindowSystem::WindowId` |
| `app/wm/tracker/windowstracker.h` | signals `activeWindowChanged`, `windowChanged`, `windowRemoved`, `applicationDataChanged`, `const WindowId &` (4) | `const Latte::WindowSystem::WindowId &` |
| `app/wm/tracker/lastactivewindow.h` | slots `applicationDataChanged`, `windowChanged`, `windowRemoved`, `const WindowId &` (3) | same |
| `app/wm/tracker/schemes.h` | signal `colorSchemeChanged(const WindowId &)` (1) | same |
| `app/layouts/synchronizer.h` | signals `newLayoutAdded`, `layoutActivitiesChanged`, `const Data::Layout &` (2) | `const Latte::Data::Layout &` |
| `app/settings/settingsdialog/layoutscontroller.h` | slots `onLayoutAddedExternally`, `onLayoutActivitiesChangedExternally`, `const Data::Layout &` (2) | same |
| `app/settings/viewsdialog/viewshandler.h` | slot `newView(const Data::Generic &)` (1) | `const Latte::Data::Generic &` |
| `app/view/settings/primaryconfigview.h` | slot `updateAvailableScreenGeometry(View *origin = nullptr)` (1) | `Latte::View *origin = nullptr` |
| `app/view/view.h` | slot `availableScreenRectChangedFromSlot(View *origin)` (1) | `Latte::View *origin` |

`WindowId` is `using WindowId = QByteArray` in `app/wm/windowinfowrap.h`, so inside
`Latte::WindowSystem` the short name looks complete but the meta-object keeps the
alias name. Fourteen findings are `WindowId`, five are `Data::...` and two are `View *`.

## Fix

The 21 declarations got the fully qualified spelling in the table, and nothing else
on those lines changed: same C++ types, names, constness and default argument. The
definitions in the `.cpp` files stay as they are (they name the same types). One
comment in `abstractwindowinterface.h`, above the first signal, says why the short alias
must not come back. The gate side is one entry in `GATED_CHECKS` and
`autotests/clazy/fully-qualified-moc-types.cpp` (an alias, a `Data::Layout`-style
name and a slot); the gate now has 40 entries and 40 fixtures. Several of the headers
already spell other parameters in full (`Latte::View *`,
`const Latte::Layout::GenericLayout *`).

## What the text feeds, and who reads it

moc's output for the same declaration, before and after, for `Synchronizer::newLayoutAdded`:

```text
string table   "Data::Layout"                       "Latte::Data::Layout"
signal         SignalData<void(const Data::Layout &)>      SignalData<void(const Latte::Data::Layout &)>
dispatcher     std::add_pointer_t<Data::Layout>            std::add_pointer_t<Latte::Data::Layout>
```

and `QMetaMethod::methodSignature()` changes from `newLayoutAdded(Data::Layout)` to
`newLayoutAdded(Latte::Data::Layout)`; for the alias from `windowAdded(WindowId)` to
`windowAdded(Latte::WindowSystem::WindowId)`. Only a consumer that reads that text, or
the metatype that moc derives from it, can notice. They were searched in the whole
tracked tree (`git grep`, including QML, JS, XML and tests):

- **String connections.** The six lines with `SIGNAL()` or `SLOT()` in the tree name other
  signals: `sourceChanged()`, `guideRequested(QString)`, `scrollRequested(int)` in tests,
  `activate(QString)` and `close(QString)` in `plasmoid/preview/main.cpp`, and the
  `QDBusConnection::connect()` of `onVirtualDesktopNavigationWrappingAroundChanged(bool)`
  in `abstractwindowinterface.cpp:302`, whose parameter is a `bool` that is not one
  of the 21. Every connection to one of the 21 is written with pointers to members (for
  example `connect(m_wm, &AbstractWindowInterface::windowAdded, ...)`); `ViewsHandler::newView`
  is only called.
- **Lookups by name.** `invokeMethod`, `indexOfSignal`, `indexOfSlot`, `indexOfMethod`,
  `fromSignal`, `QMetaMethod::methodSignature()` and `normalizedSignature()`: none names
  one of the 21. The signature strings the app builds are for `appletContainsPos(QVariant,QVariant)`,
  `addSyncedLauncher(QVariant,QVariant)` and its siblings, `processMimeData(QObject*,int,int)`,
  `itemFor(QObject*)` and similar methods of QML objects and of the layout manager. No test
  looks up a signature either.
- **Metatype registrations.** `WindowId` has none (it is `QByteArray`). The `Latte::Data::*`
  types are declared with their qualified names (`Q_DECLARE_METATYPE(Latte::Data::Layout)`
  in `app/data/layoutdata.h`), which is why moc now matches them (see the result below).
- **QML.** Signal handlers bind by the signal name and the argument names, which did not
  change. No QML or JS file names any of the 21 signals or slots as a handler or a call
  (the hits for `windowChanged` and `windowAdded` are Qt's `QQuickItem::windowChanged` and KWin's
  own signal in `plasmapaneltracker.js`).
- **D-Bus.** None of the nine classes is registered on the bus or carries a `D-Bus Interface` class info
  (the only ones are `Corona` and `PlasmaPanelTracker`).
- **Source contracts.** `sourcecontracttest` reads these classes through their `.cpp`
  files (`Q_EMIT windowChanged(pW->uuid())`, `connect(m_wm, &AbstractWindowInterface::windowTitleChanged, ...)`
  and similar); the two headers it reads (`abstractwindowinterface.h`, `view.h`) are
  matched for lines this slice does not touch. It passes.

## Proof

1. **moc output of the build, whole tree.** The Clang Debug container tree was built before
   (after `R2c-incorrect-emit`) and after, and the 119 `moc_*.cpp` files compared with the
   scratch parser `moc_signatures.py` (kind, access, flags, name, runtime signature, return
   type, parameter type and name strings, C++ signature of all 976 records): 9 of 119 files
   differ, and exactly 21 records are removed and 21 added, one pair per finding. Every pair
   has the same kind, access, name, return type, parameter names and arity; only the type
   spelling differs. The number of records is 976 on both sides.
2. **moc JSON of the headers.** `moc --output-json` of the pre-fix and post-fix headers (host
   moc 6.11.2), compared key by key for every class (signals, slots, methods, properties,
   enums, class infos, super classes; line numbers left out): 42 entries differ, the 21 old and
   21 new entries, each with an equal `index`, `access`, `name`, `returnType` and argument
   names. The other entries are identical (for example 130 in `view.h`).
3. **Differences in the moc text beyond the spelling.** Six of the nine files differ only in the
   spelling. In `moc_view.cpp` the string `"View*"` is gone because it now equals the existing
   `"Latte::View*"`, which renumbers the later string indexes by one. In `moc_synchronizer.cpp`
   and `moc_layoutscontroller.cpp` moc adds a `RegisterMethodArgumentMetaType` branch that returns
   `QMetaType::fromType<Latte::Data::Layout>()` for the four `Latte::Data::Layout` arguments, and
   `qt_metacall` forwards that request to it instead of answering with an invalid type. moc
   generates it because the qualified name now equals the `Q_DECLARE_METATYPE` text, which the
   short `Data::Layout` never did (the probe below shows the same with an equivalent declaration).
   `Latte::Data::Generic` and the `WindowId` arguments get no such branch, before or after.
4. **The compiler checks the type identity.** The unchanged `.cpp` definitions still match the
   declarations in the GCC and Clang builds, so every spelling names the same type as before.
5. **Consumer probe.** A scratch program (Qt 6.11.2) declares the same signals with the short
   and the full spelling next to a `Q_DECLARE_METATYPE`. The meta-object signatures read
   `layoutPartial(Data::Layout)` and `layoutFull(Outer::Data::Layout)`, `idPartial(Id)` and
   `idFull(Outer::Id)`. `parameterMetaType()` is valid for all four and queued pointer-to-member
   connections deliver the argument for all four; a string connection with the other spelling
   fails with "No such signal"; `invokeMethod` by name works with either `Q_ARG` spelling; and
   `RegisterMethodArgumentMetaType` answers only for the fully qualified, declared class type.
   So the change is visible to string lookups and to that lazy registration only, and the
   tree has no string lookup of the 21.

## Evidence

Container commands are those of [R2c](R2c.md#reproducing-the-environment), with a
private build directory (`~/r2c-build-batch-d`) and `--jobs 6`.

| Command | Result |
| --- | --- |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 6 --inventory fully-qualified-moc-types --report build/fully-qualified-moc-types.json` before the fix | 21 findings in 9 files, as in R2c |
| Same after the fix | Exit 0 in 154 s: 0 findings in 153 sources |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 6` | **Exit 0** in 193 s: 40 fixtures rejected by their own checks, 153 sources with no finding from 40 checks |
| Same, with the previous versions of the nine headers mounted over the fixed ones, no rebuild | **Exit 1** in 147 s: the 21 findings, at `synchronizer.h` 130:31 and 131:40, `layoutscontroller.h` 123:40 and 124:52, `viewshandler.h` 91:24, `primaryconfigview.h` 120:40, `view.h` 355:45, `abstractwindowinterface.h` 141:30 to 145:24 and 192:28, `lastactivewindow.h` 123:39, 125:30 and 126:30, `schemes.h` 43:35 and `windowstracker.h` 110:36, 111:30, 112:30 and 114:39 |
| Container `cmake --build build/modernization/clang-debug -j6` (Clang 18.1.3, Qt 6.11.1) after the change | Exit 0 in 119 s, 0 compiler warnings |
| Host `cmake --preset gcc-debug`, `cmake --build --preset gcc-debug -j6`, `ctest --preset gcc-debug` (GCC 16.2.1, Qt 6.11.2) | Exit 0 each; incremental build in 115 s (60 objects rebuilt), 0 compiler warnings, 0 CMake warnings; 57/57 tests passed |
| Host `cmake --preset clang-debug`, `cmake --build --preset clang-debug -j6`, `ctest --preset clang-debug` (Clang 22.1.8) | Exit 0 each; incremental build in 69 s (60 objects rebuilt), 0 compiler warnings, 0 CMake warnings; 57/57 tests passed |
| moc output comparison, header moc JSON comparison, consumer probe | As described above |

The build logs hold thousands of `Warning:` lines from the qmllint step; they are the
known QML warnings tracked by the baseline gate, not compiler output, and this
change touches no QML.

## Handoff

- **Files and behavior changed:** `app/wm/abstractwindowinterface.h`, `app/wm/tracker/windowstracker.h`,
  `app/wm/tracker/lastactivewindow.h`, `app/wm/tracker/schemes.h`, `app/layouts/synchronizer.h`,
  `app/settings/settingsdialog/layoutscontroller.h`, `app/settings/viewsdialog/viewshandler.h`,
  `app/view/settings/primaryconfigview.h`, `app/view/view.h` (declarations only, one comment),
  `scripts/test-clazy.py` (one name), `autotests/clazy/fully-qualified-moc-types.cpp`. The signal
  and slot signature text of 21 methods changes; the C++ types do not, and neither does any
  connection. The generated meta-object registers the metatype of `Latte::Data::Layout` for four
  arguments on request, which only a lookup by the signature text would use, and the tree has none.
- **State authority / lifetime constraints:** none; declarations only.
- **Tests:** no test added or changed; `ctest` 57/57 on GCC and Clang Debug.
- **Smoke plan:** these actions run through the 21 methods, reached by pointers to members or direct calls.
  Window tracking: focus, move, maximize and close a window and switch the color scheme with a
  dock that follows window colors or uses a dodge visibility mode (`AbstractWindowInterface`,
  `Tracker::Windows`, `LastActiveWindow`, `Schemes`). Layouts dialog: add a layout, change its
  activities, switch layouts (`Synchronizer::newLayoutAdded`, `layoutActivitiesChanged` and the two
  `Layouts` controller slots). Views dialog: create a dock from a template (`ViewsHandler::newView`).
  Dock configuration: open the dock settings while moving the dock to another edge or screen
  (`PrimaryConfigView::updateAvailableScreenGeometry`, `View::availableScreenRectChangedFromSlot`).
  The debug log should hold no `QObject::connect` warning.
- **Combined validation:** on `main` at `382e55cd2`, GCC and Clang Debug and
  Release with 0 warnings and 57/57 tests each, the 43-check gate and the
  Debian 13 container build on Qt 6.8.2 passed; see [R2c fix batch 2](R2c.md#fix-batch-2).
- **Desktop smoke result:** passed on the host ([R2c fix batch
  2](R2c.md#fix-batch-2)): window tracking through a day of window changes, a
  layout switch and back, and the Docks dialog, with no `QObject::connect` warning
  in the log. Adding a layout, changing its activities, creating a dock from a
  template and moving a dock to another edge were not part of it.
- **Not run:** the CI job (`main` is not pushed), a build on Qt 6.6 or 6.7.
- **Next exact action:** after a push is approved, confirm the CI run and mark
  this slice Complete.
- **Commit approval status / push approval status:** committed on the feature
  branch; merged into `main` in `382e55cd2` with the user's approval; not
  pushed.
