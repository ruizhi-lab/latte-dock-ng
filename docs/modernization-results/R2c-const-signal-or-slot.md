# R2c-const-signal-or-slot: Const Getters Declared as Slots

Status: Complete. Merged into `main` in `382e55cd2`; the checks on `main` and
the host desktop smoke passed ([R2c fix batch 2](R2c.md#fix-batch-2)). CI run
40683601580 at `14d705a5d` passed all 23 jobs, including the 43-check clazy gate
([CI validation](R2c.md#ci-validation)). No automated test calls the ten
methods.
Date: 2026-10-28. Branch: `claude/r2c-batch-d-a6a9f7`. Start HEAD: `eba91a3e6`.
Implementation commit: the commit that adds this report. Part of [R2c](R2c.md),
the first slice of batch D (before `R2c-incorrect-emit` and
`R2c-fully-qualified-moc-types`).

## Defect

clazy's `const-signal-or-slot` reports a non-void `const` method in a slots
section as "getter ... possibly mismarked as a slot". A getter is not a slot:
the section offers it to signal connections it has no use for and hides the
real reason it is in the meta-object, which is lookup by name. The ten
findings are:

| Header:line | Method | Section | Callers |
| --- | --- | --- | --- |
| `app/layout/genericlayout.h:150` | `GenericLayout::qmlFreeEdges(int) const` | `public Q_SLOTS`, written `virtual Q_INVOKABLE` | none in C++, QML or JS |
| `app/plasma/extended/screengeometries.h:47` | `ScreenGeometries::screenIsActive(const QString &) const` | `private Q_SLOTS` | one direct call (`screengeometries.cpp:273`) |
| `app/view/visibilitymanager.h:211` | `VisibilityManager::isValidMode() const` | `private Q_SLOTS` | none |
| `containment/plugin/layoutmanager.h:157` | `LayoutManager::isModernDockStyle() const` | `private Q_SLOTS` | none (see below) |
| `containment/plugin/layoutmanager.h:158` | `LayoutManager::isTrashWidget(QObject *) const` | `private Q_SLOTS` | two direct calls |
| `containment/plugin/layoutmanager.h:161` | `LayoutManager::usesLegacyJustifySplitters() const` | `private Q_SLOTS` | direct calls |
| `containment/plugin/layoutmanager.h:162` | `LayoutManager::readConfigValue(const QString &, const QVariant & = QVariant()) const` | `private Q_SLOTS` | direct calls |
| `containment/plugin/layoutmanager.h:164` | `LayoutManager::containmentObject() const` | `private Q_SLOTS` | direct calls |
| `containment/plugin/layoutmanager.h:165` | `LayoutManager::resolveAppletQuickItemObject(QObject *) const` | `private Q_SLOTS` | direct calls |
| `containment/plugin/layoutmanager.h:166` | `LayoutManager::plasmoidApplets() const` | `private Q_SLOTS` | direct calls |

Why only `qmlFreeEdges` of the public ones: clazy 1.17 sees `Q_INVOKABLE` only
when it is the first token of the declaration. A scratch fixture in the
container gave a finding for `virtual Q_INVOKABLE int f() const;` and for a
plain `int f() const;`, and none for `Q_INVOKABLE int f() const;` or
`Q_INVOKABLE virtual int f() const;` in the same slots section. So
`GenericLayout::viewsWithTasks() const`, which is `Q_INVOKABLE` in the same
section, is not reported, and it is not touched here. Moving `qmlFreeEdges`
out of the section, as asked, is the real fix; reordering its keywords would
only have silenced the check.

## Fix

Each getter left its slots section and became `Q_INVOKABLE`, in an access section
that is not a slots section and with the access level it had:

- `genericlayout.h`: `qmlFreeEdges` moved from `public Q_SLOTS` into the `public:`
  section, below the `freeEdges()` overloads it mirrors. Its signature and
  trailing comment are unchanged.
- `screengeometries.h`, `visibilitymanager.h`: the getter moved from `private Q_SLOTS`
  into the next `private:` section.
- `layoutmanager.h`: the seven getters moved from `private Q_SLOTS` into the
  `private:` section that follows it. The other private slots stay slots in
  their original order.

A comment beside each group says why they are not slots. No `.cpp` file, QML
file or test changed, and nothing was renamed. The gate side is one entry in
`GATED_CHECKS` and `autotests/clazy/const-signal-or-slot.cpp` (a `const` getter
in a `public Q_SLOTS` section); the gate now has 38 entries and 38 fixtures.

## Consumers checked

The change turns a meta-object `Slot` into a `Method`: same name, same
signature, same access level, same return type. Only something that treats a
slot differently from a method can notice, so these were searched in the whole
tracked tree (`git grep`, including QML, JS, XML and tests):

- **QML and JS.** No file names `qmlFreeEdges`, `screenIsActive`, `isValidMode`,
  `isTrashWidget`, `usesLegacyJustifySplitters`, `readConfigValue`,
  `containmentObject`, `resolveAppletQuickItemObject` or `plasmoidApplets`. The
  QML hits for `isModernDockStyle` read `root.isModernDockStyle`, a property of the
  containment root item (`main.qml:75`), and `indicator.isModernDockStyle`, the
  `Q_PROPERTY` of `ViewPart::Indicator`. What QML calls on the layout manager is
  public (for example `resolveAppletQuickItem`, `addAppletItem`, `save`). The
  nine private getters cannot be called from QML at run time: the QML engine does
  not expose private meta-methods (probe below), so the move cannot change what
  QML sees.
- **Lookups by name.** `QMetaObject::invokeMethod`, `indexOfMethod`,
  `indexOfSlot`, `QMetaMethod` and `SLOT()`/`SIGNAL()` string forms: none
  targets one of the ten. The only quoted occurrence is
  `m_rootItem->property("isModernDockStyle")` in `layoutmanager.cpp:506`, which
  reads the QML root item. The app's by-name calls into the layout manager are
  `hideAppletItem`, `showAppletItem`, `removeAppletItem`, `addAppletItem`,
  `insertAtCoordinates`, `dndSpacerIndex`, `requestApplets*` and
  `setAppletInScheduledDestruction`, all `public` and untouched.
  `containmentlayoutunittest.cpp` calls the private slots
  `rememberAppletRemovalIndex` and `restoreAppletOrderForApplet` by name; they
  are not among the ten.
- **D-Bus.** The only production `registerObject()` with `ExportAllSlots` is
  `PlasmaPanelTracker` (`app/wm/plasmapaneltracker.cpp:32`), which is none of the
  four classes.
- **Source contracts.** `sourcecontracttest.cpp:2486` requires `layoutmanager.h`
  to contain `isTrashWidget(QObject *applet) const`. The string is still in the
  header (`Q_INVOKABLE bool isTrashWidget(QObject *applet) const;`) and
  `sourcecontracttest` passes.
- **No caller at all.** `qmlFreeEdges`, `isValidMode` and `LayoutManager::isModernDockStyle`
  have no caller in C++, QML or JS. They stay (a check slice does not remove dead
  code), still invokable; deleting them is a separate decision.

## Proof that only the kind of ten methods changes in the meta-object

1. **moc output of the build.** The Clang Debug container tree was built before and
   after, and the 119 `moc_*.cpp` files compared with
   `moc_signatures.py`, a scratch parser of the Qt 6.11 moc format (kind, access,
   flags, name, runtime signature, return type, parameter types and names, C++
   signature): 4 of 119 files differ and 11 records change from `Slot` to `Method`.
   Everything else is identical, and the number of records is 976 on both sides.
   The 11 records are the ten findings plus the cloned `readConfigValue(QString)`
   overload that the default argument adds:

   ```text
   GenericLayout        Slot -> Method  public   qmlFreeEdges(int)                  -> QList<int>
   ScreenGeometries     Slot -> Method  private  screenIsActive(QString)            -> bool
   VisibilityManager    Slot -> Method  private  isValidMode()                      -> bool
   LayoutManager        Slot -> Method  private  isModernDockStyle()                -> bool
                                                 isTrashWidget(QObject*)            -> bool
                                                 usesLegacyJustifySplitters()       -> bool
                                                 readConfigValue(QString,QVariant)  -> QVariant
                                                 readConfigValue(QString) (cloned)  -> QVariant
                                                 containmentObject()                -> Plasma::Containment*
                                                 resolveAppletQuickItemObject(QObject*) -> QObject*
                                                 plasmoidApplets()                  -> QList<QObject*>
   ```

2. **moc JSON of the headers.** `moc --output-json` of each pre-fix header (`eba91a3e6`)
   and post-fix header, host moc 6.11.2: 11 records move from `slots` to `methods`;
   access, arguments, return type and constness are equal.
3. **QML tooling metadata.** `LayoutManager` is the only one of the four classes
   that is a QML type, in `lattecontainmentplugin`. The container's moc (the
   build's exact AUTOMOC flags) and `qmltyperegistrar` regenerated the plugin's
   `.qmltypes` and `*_qmltyperegistrations.cpp` from the pre-fix and the post-fix
   header: the post-fix output equals what the build wrote, the registrations
   file is identical before and after, and the `.qmltypes` file keeps the same
   `Method` entries (the registrar lists the private getters in both versions)
   with only their `lineNumber` values and the order of the three non-const
   private methods changed.
4. **The compiler sees the same calls.** No `.cpp` changed. Direct callers compile
   unchanged because the C++ signature and access level are the same.
5. **Reachability probe.** A scratch program (Qt 6.11.2) declares a public and a
   private slot and a public and a private `Q_INVOKABLE`. `QMetaObject::invokeMethod`
   reaches all four by name, the meta-object lists the private ones as `Slot Private`
   and `Method Private`, and a `QJSEngine` object wrapper calls the two public ones but
   reports "is not a function" for both private ones. So a private `Q_INVOKABLE`
   keeps exactly the by-name reachability the private slot had and gains none in QML.

## Evidence

Container commands are those of [R2c](R2c.md#reproducing-the-environment), with a
private build directory (`~/r2c-build-batch-d`) and `--jobs 6`.

| Command | Result |
| --- | --- |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 6 --inventory const-signal-or-slot --report build/const-signal-or-slot.json` before the fix | 10 findings in 4 files, as in R2c |
| Same after the fix | Exit 0 in 179 s: 0 findings in 153 sources |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 6` | **Exit 0** in 187 s: 38 fixtures rejected by their own checks, 153 sources with no finding from 38 checks |
| Same, with the previous versions of the four headers mounted over the fixed ones, no rebuild | **Exit 1** in 117 s: the ten findings at `genericlayout.h:150:5`, `screengeometries.h:47:5`, `visibilitymanager.h:211:5` and `layoutmanager.h` 157:5, 158:5, 161:5, 162:5, 164:5, 165:5, 166:5 |
| Container `cmake --build build/modernization/clang-debug -j6` (Clang 18.1.3, Qt 6.11.1) after the change | Exit 0 in 121 s, 0 compiler warnings |
| Host `cmake --preset gcc-debug`, `cmake --build --preset gcc-debug -j6`, `ctest --preset gcc-debug` (GCC 16.2.1, Qt 6.11.2) | Exit 0 each; fresh tree built in 385 s, 0 compiler warnings, 0 CMake warnings; 57/57 tests passed |
| Host `cmake --preset clang-debug`, `cmake --build --preset clang-debug -j6`, `ctest --preset clang-debug` (Clang 22.1.8) | Exit 0 each; fresh tree built in 254 s, 0 compiler warnings, 0 CMake warnings; 57/57 tests passed |
| moc output comparison, header moc JSON diff, qmltypes regeneration, reachability probe | As described above |

The build logs hold thousands of `Warning:` lines from the qmllint step; they are the
known QML warnings tracked by the baseline gate, not compiler output, and this
change touches no QML.

## Handoff

- **Files and behavior changed:** `app/layout/genericlayout.h`,
  `app/plasma/extended/screengeometries.h`, `app/view/visibilitymanager.h`,
  `containment/plugin/layoutmanager.h` (declarations only), `scripts/test-clazy.py`
  (one name), `autotests/clazy/const-signal-or-slot.cpp`. The same methods exist with
  the same names, signatures and access levels; the meta-object lists them as methods,
  not slots.
- **State authority / lifetime constraints:** none; declarations only.
- **Tests:** no test added or changed; `ctest` 57/57 on GCC and Clang Debug. No automated
  test calls the ten methods by name.
- **Formatting:** `formatter.sh` was inspected, not applied. `clang-format` would re-indent 23 to
  91 unrelated lines per header (the repository's 2-space member indent against the 4 spaces
  these files use); the moved lines keep the surrounding style.
- **Smoke plan:** none specific. `usesLegacyJustifySplitters`, `readConfigValue`,
  `containmentObject`, `resolveAppletQuickItemObject` and `plasmoidApplets` run during dock
  startup and when widgets are added, removed or reordered, `isTrashWidget` when a widget is
  added and in the one-time startup migration, `screenIsActive` when the screen geometries are
  published to Plasma (`PLASMA SCREEN GEOMETRIES` lines in the debug log). The log
  should hold no `QMetaObject::invokeMethod: No such method` line.
- **Decision for later:** nine of the ten are private and none is reached by name, so
  plain private methods would also satisfy the check and shrink the meta-object.
  `Q_INVOKABLE` was chosen as the equivalent change. The three methods with no
  caller (`qmlFreeEdges`, `isValidMode`, `LayoutManager::isModernDockStyle`) are
  candidates for a separate dead-code review.
- **Combined validation:** on `main` at `382e55cd2`, GCC and Clang Debug and
  Release with 0 warnings and 57/57 tests each, the 43-check gate and the
  Debian 13 container build on Qt 6.8.2 passed; see [R2c fix batch 2](R2c.md#fix-batch-2).
- **Desktop smoke result:** passed on the host ([R2c fix batch
  2](R2c.md#fix-batch-2)). The log holds 40 `PLASMA SCREEN GEOMETRIES` lines
  (`screenIsActive`) and no `No such method` line.
- **CI:** run 40683601580 at `14d705a5d` passed all 23 jobs, among them
  `Build (Clang Debug)` with the 43-check clazy gate and `Install verify
  (debian13)` on Qt 6.8.2; see [CI validation](R2c.md#ci-validation).
- **Not run:** a build on Qt 6.6 or 6.7.
- **Next exact action:** none.
- **Commit approval status / push approval status:** committed on the feature
  branch; merged into `main` in `382e55cd2` with the user's approval; pushed
  with `14d705a5d`.
