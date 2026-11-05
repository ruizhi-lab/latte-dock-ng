# R6 design study: typed interfaces for the containment `LayoutManager`

Status: **design only.** No source file was changed, nothing was built or run, the desktop was not touched, and Debian 13 was not verified (deferred by the user). Source read: `HEAD` of branch `claude/r6-layoutmgr-design-a6f582`, cut from `main` at `e18a0f9a7` (v1.3.0). This record belongs to ledger row R6 in [architecture-modernization-plan.md](../architecture-modernization-plan.md#assessment-refresh-r-track); the main session updates the ledger. Line numbers are of this checkout; `file:line` citations are plain code spans so they do not go stale in links.

## 1. Summary

1. **Why the coupling exists.** `LayoutManager` is compiled only into the `lattecontainmentplugin` QML module (`containment/CMakeLists.txt:5-26`). The application does not link it, and `ContainmentInterface` says so and calls by name (`app/view/containmentinterface.cpp:2079-2083`). Every application-side call and read is therefore a string.
2. **Recommendation (Option 1).** Keep `LayoutManager` in the plugin. Add a header-only C++ interface, `LayoutManagerApi`, declared with `Q_DECLARE_INTERFACE`, which the plugin object implements. The application obtains it with `qobject_cast` from the object it already holds. Calls become typed, and the application needs no link-time symbol from the plugin. Not built, so the link behaviour is reasoning, not verified.
3. **First slice (S1, one file plus one test): make the name contracts fail loudly.** A renamed QML function or property fails silently today. `QMetaMethod::invoke()` and `QObject::property()` return false or an invalid value without a warning, and the restore path logs only at debug level before it stops after 60 retries (`containment/plugin/layoutmanager.cpp:1087-1091`, `1235-1240`, `1282-1291`).
4. **Then, in order:** S2 typed `LayoutManagerApi` for application-side reads and calls (about 38 sites); S3 retire the legacy Justify-splitter branches, which are dead while `usesLegacyJustifySplitters()` returns `false` (after a consumer trace); S4 a typed one-shot protocol to replace two dynamic properties; S5 typed QML pushes for the drag path, only after a profile. S6 (typed layout-item base type) and S7 (C++ creation path) are large, need desktop retests, and are not started.
5. **Performance is not established.** Each drag move makes seven name-based property reads and one name-based `invokeMethod()` (`app/view/view.cpp:1560`, `:1571`, `:1576`; `containment/plugin/layoutmanager.cpp:478`, `:481`, `:1593`, `:1595`, `:1596`), and parabolic hover invokes a QML signal by name per pointer move (`app/view/parabolic.cpp:114`). Nothing was measured. The first slices are justified by correctness and maintainability; section 9 lists the measurements that would reorder them.

## 2. Scope, method and limits

- **Read in full or in the access regions:** `containment/plugin/layoutmanager.h` (all 262 lines) and the access regions of `layoutmanager.cpp` (2842 lines); access sites and their enclosing functions in `app/view/containmentinterface.cpp`, `app/view/view.cpp`, `app/declarativeimports/contextmenulayerquickitem.cpp`, `app/declarativeimports/interfaces.cpp`, `app/view/parabolic.cpp`, `app/view/effects.cpp` and `app/view/settings/subconfigview.cpp`; the containment QML that defines or consumes the names (`main.qml`, `BindingsExternal.qml`, `DragDropArea.qml`, `applet/AppletItem.qml`, `layouts/*.qml`, `abilities/PositionShortcuts.qml`, `applet/ParabolicArea.qml`, `abilities/MyView.qml`); the autotests that pin or stub these names; and the project documents named in the brief.
- **Inventory method.** The raw search is `git grep -n -E 'property\(|setProperty\(|invokeMethod\(|indexOfMethod|indexOfProperty|indexOfSlot|QMetaProperty|\.invoke\(|contextProperty\('` over the directories in the brief; the brief's pattern lacks `indexOfProperty`, `indexOfSlot`, `.invoke(` and `contextProperty(`, which were added. Autotests are excluded from the inventory and are discussed in section 8. A scratch script outside the repository generated the tables from `HEAD` (it is not committed); each row lists the line numbers that share an enclosing function, target, name and direction. Owner and hotness come from the code read for this record (dictionaries in the generator, reviewed by hand).
- **Not done:** no build, no `ctest`, no `qmllint`, no Debian or Docker work, no desktop run, and no CodeGraph query (the worktree is not indexed; `git grep` and reads were used). Qt and Plasma runtime behaviour is cited from documented API knowledge and is marked "not verified" where it matters.

## 3. Counts, discrepancies and corrections to the brief

| Measure | Value | Source and note |
| --- | --- | --- |
| Production C++ `property("` reads (excluding autotests and docs) | 112 | Matches the brief. The 2026-10 assessment says 114 (`docs/architecture-assessment-2026-10.md:37-38`); the difference is scope or date, not re-checked. |
| Production C++ `setProperty(` calls | 38 | Matches the brief; the assessment says 40. |
| Production C++ `invokeMethod(` calls | 24 | Matches the brief; the assessment says 23. |
| `layoutmanager.cpp` lowercase `property(` occurrences | 56 (54 literal, 2 variable: lines 577, 956) | The brief says 61. Not reproduced with the same pattern. |
| `layoutmanager.cpp` other name-based calls | 3 `setProperty(`, 2 `invokeMethod(`, 9 cached `QMetaMethod::invoke()`, 13 `indexOf*`/`method()` lookups | From the same grep. |
| Inventory in this record | 195 access lines in 175 table rows | Table 4.1. By file: `layoutmanager.cpp` 79, `containmentinterface.cpp` 63, `view.cpp` 24, `subconfigview.cpp` 8, `contextmenulayerquickitem.cpp` 7, `interfaces.cpp` 7, `parabolic.cpp` 4, `effects.cpp` 3. By direction: read 101, call 39, resolve 33, write 22. |
| QML references to `fastLayoutManager` | 70 lines (`main.qml` 35, `AppletItem.qml` 29, `DragDropArea.qml` 4, `BindingsExternal.qml` 2) | Table 4.2. |
| QML references to `isInternalViewSplitter` or `internalSplitterId` | 53 lines | All of `containment/package`. |
| QML `Binding { }` blocks in `BindingsExternal.qml` | 34 | Table 4.2; they set named properties on C++ objects. |

**Corrections to the brief:**

- The brief lists the root items `isModernDockStyle` and `currentDockStyleIndex` among names that C++ reads. It does not. `layoutmanager.cpp` reads only `myView` on the root (line 478). `isModernDockStyle` is read only by QML (21 lines in `containment/package`; defined at `main.qml:74-75`). `Indicator::isModernDockStyle()` in `app/view/indicator/indicator.cpp:140` is a different, KConfig-based C++ method.
- The brief's `rowSpacing` and `columnSpacing` are Qt Quick `Grid` properties on the project's `AppletsContainer` (`containment/package/contents/ui/layouts/AppletsContainer.qml:16,19` sets both to 0). The C++ reads them per drag move (`layoutmanager.cpp:1595-1596`).
- `containmentinterface.cpp:10` includes `layoutmanager.h`, but nothing in `app/` uses a `LayoutManager` symbol. The include appears unused. Not verified by compiling.

**Stale comments found (documentation drift, not blocking):**

- `app/view/view.cpp:1596-1598` says the native drop path "call[s] LayoutManager::addAppletItem() directly with the insertion index". The code sets `_latte_pendingInsertionIndex` (`view.cpp:1633-1635`) and calls `cont->createApplet()` (`view.cpp:1643-1645`).
- `containment/plugin/layoutmanager.cpp:1957-1958` reads "in javascript direct insertAtCoordinates was usedd ???", an unanswered question about the JS side.

## 4. Inventory of string-based access

Legend for the tables. **Dir:** `read` (`property()`), `write` (`setProperty()`), `call` (`invokeMethod()` or a cached `QMetaMethod::invoke()`), `resolve` (`indexOf*()`, `method()`, or a `QMetaProperty` by index). **Owner:** `P` Plasma-owned (cannot be typed from this repository), `Q` project QML, `C` project C++ (`LayoutManager` or `ContainmentInterface`), `D` dynamic property used as a channel, `Q other` QML outside `containment/`. **Hot path:** `HOT` per pointer or drag event; `warm` per item switch or geometry change; `event-driven` per signal from another component; `user action`; `structural` (startup, applet add or remove, timers); `cold`. Lines with the same enclosing function, target, name and direction share one row.

### 4.1 Production C++ (generated from `HEAD`)

The rows come from a script over the files in section 2 (autotests excluded). The script's regular expression covers `property(`, `setProperty(`, `invokeMethod(`, `indexOf{Method,Property,Slot}(`, `QMetaProperty`, `.invoke(`, `contextProperty(` and `->method(`. Line numbers are of `HEAD`. `containment/plugin/layoutmanager.h` contributes no string-based access line: its only matches are the `QMetaMethod` members (`:247-249`) and a comment that names `QMetaObject::invokeMethod()` (`:164`).

| Location | Enclosing function | Target | Name | Dir | Defined in (owner) | Hot path |
| --- | --- | --- | --- | --- | --- | --- |
| `containment/plugin/layoutmanager.cpp:318` | `setPlasmoid` | `m_plasmoid` | `configuration` | read | Plasma plasmoid attached object (dynamic) (P) | once per containment |
| `containment/plugin/layoutmanager.cpp:478` | `readConfigValue` | `m_rootItem` | `myView` | read | main.qml:335 alias of _myView (abilities/privates/MyViewPrivate.qml) (Q) | HOT when reached from insertAtCoordinates; otherwise structural |
| `containment/plugin/layoutmanager.cpp:481` | `readConfigValue` | `myView` | `alignment` | read | abilities/MyView.qml:33; config main.xml:8 (Int) (Q) | HOT when reached from insertAtCoordinates; otherwise structural |
| `containment/plugin/layoutmanager.cpp:549` | `containmentObject` | `m_plasmoid` | `plasmoid` | read | Plasma plasmoid attached object (P) | helper on structural paths |
| `containment/plugin/layoutmanager.cpp:577` | `resolveAppletQuickItemObjectInternal` | `applet` | `applet / plasmoid (resolveFromBackendProperty)` | read | applet object: AppletItem.qml:504 or Plasma object (Q/P) | structural: per applet resolution |
| `containment/plugin/layoutmanager.cpp:614, 694` | `resolveAppletQuickItemObjectInternal` | `applet` | `item` | read | applet object; not declared in containment QML (owner unverified) (P) | structural: per applet resolution |
| `containment/plugin/layoutmanager.cpp:624, 695` | `resolveAppletQuickItemObjectInternal` | `applet` | `_plasma_graphicObject` | read | Plasma (libplasmaquick); not defined in repo (P) | structural: per applet resolution |
| `containment/plugin/layoutmanager.cpp:640` | `resolveAppletQuickItemObjectInternal` | `metaObject` | `itemFor(QObject*)` | resolve | Plasma Containment/Plasmoid attached; called at main.qml:904,911 (P) | structural: per applet resolution |
| `containment/plugin/layoutmanager.cpp:641` | `resolveAppletQuickItemObjectInternal` | `metaObject` | `itemFor(QVariant)` | resolve | Plasma Containment/Plasmoid attached; called at main.qml:904,911 (P) | structural: per applet resolution |
| `containment/plugin/layoutmanager.cpp:645, 651` | `resolveAppletQuickItemObjectInternal` | `m_plasmoid` | `itemFor` | call | Plasma Containment/Plasmoid attached; called at main.qml:904,911 (P) | structural: per applet resolution |
| `containment/plugin/layoutmanager.cpp:678` | `resolveAppletQuickItemObjectInternal` | `metaObject` | `applet` | resolve | applet/AppletItem.qml:504 (property Item) on containers; Plasma applet object elsewhere (Q/P) | structural: per applet resolution |
| `containment/plugin/layoutmanager.cpp:679` | `resolveAppletQuickItemObjectInternal` | `metaObject` | `plasmoid` | resolve | Plasma plasmoid attached object (P) | structural: per applet resolution |
| `containment/plugin/layoutmanager.cpp:680` | `resolveAppletQuickItemObjectInternal` | `metaObject` | `pluginName` | resolve | applet object, read by name (layoutmanager.cpp:680) (P) | structural: per applet resolution |
| `containment/plugin/layoutmanager.cpp:681` | `resolveAppletQuickItemObjectInternal` | `metaObject` | `status` | resolve | applet object, read by name (layoutmanager.cpp:681) (P) | structural: per applet resolution |
| `containment/plugin/layoutmanager.cpp:693` | `resolveAppletQuickItemObjectInternal` | `applet` | `id` | read | applet object; not declared in containment QML (owner unverified) (P) | structural: per applet resolution |
| `containment/plugin/layoutmanager.cpp:769` | `plasmoidApplets` | `m_plasmoid` | `applets` | read | Plasma plasmoid attached object (P) | structural |
| `containment/plugin/layoutmanager.cpp:838` | `isTrashWidget` | `applet` | `applet` | read | applet/AppletItem.qml:504 (property Item) on containers; Plasma applet object elsewhere (Q/P) | structural: applet added |
| `containment/plugin/layoutmanager.cpp:938` | `appletId` | `candidate` | `id` | read | applet object; not declared in containment QML (owner unverified) (P) | structural helper; also AppletItem.qml:235 binding |
| `containment/plugin/layoutmanager.cpp:956` | `appletId` | `candidate` | `backendAppletRef / applet / plasmoid / _plasma_applet / _plasmoid / item / _plasma_graphicObject / graphicObject (backendProps)` | read | mixed: project QML names and Plasma applet names (Q/P) | structural helper; also AppletItem.qml:235 binding |
| `containment/plugin/layoutmanager.cpp:1035` | `onRootItemChanged` | `rootMetaObject` | `createAppletItem(QVariant)` | resolve | main.qml:803 (root function) (Q) | once per root item |
| `containment/plugin/layoutmanager.cpp:1038` | `onRootItemChanged` | `rootMetaObject` | `createJustifySplitter()` | resolve | main.qml:946 (root function) (Q) | once per root item |
| `containment/plugin/layoutmanager.cpp:1041` | `onRootItemChanged` | `rootMetaObject` | `initAppletContainer(QVariant,QVariant)` | resolve | main.qml:929 (root function) (Q) | once per root item |
| `containment/plugin/layoutmanager.cpp:1232, 1268` | `restore` | `m_createAppletItemMethod` | `createAppletItem(QVariant)` | call | main.qml:803 (root function) (Q) | structural: startup and bounded 150 ms retries |
| `containment/plugin/layoutmanager.cpp:1250` | `restore` | `m_createJustifySplitterMethod` | `createJustifySplitter()` | call | main.qml:946 (root function) (Q) | structural: startup and bounded 150 ms retries |
| `containment/plugin/layoutmanager.cpp:1331` | `isJustifySplitter` | `item` | `isInternalViewSplitter` | read | applet/AppletItem.qml:229; layouts/ParabolicEdgeSpacer.qml:18 (Q) | structural (justify splitters) |
| `containment/plugin/layoutmanager.cpp:1353` | `reorderParabolicSpacers` | `m_mainLayout` | `startParabolicSpacer` | read | layouts/LayoutsContainer.qml:349 (readonly alias) (Q) | structural: each save() |
| `containment/plugin/layoutmanager.cpp:1354` | `reorderParabolicSpacers` | `m_mainLayout` | `endParabolicSpacer` | read | layouts/LayoutsContainer.qml:350 (readonly alias) (Q) | structural: each save() |
| `containment/plugin/layoutmanager.cpp:1373` | `save` | `this` | `_latte_skipSpacerReorder` | read | dynamic property on LayoutManager; set and read in C++ only (D) | structural: layout changes, destruction |
| `containment/plugin/layoutmanager.cpp:1378` | `save` | `this` | `_latte_skipSpacerReorder` | write | dynamic property on LayoutManager; set and read in C++ only (D) | structural: layout changes, destruction |
| `containment/plugin/layoutmanager.cpp:1386` | `save` | `item` | `isInternalViewSplitter` | read | applet/AppletItem.qml:229; layouts/ParabolicEdgeSpacer.qml:18 (Q) | structural: layout changes, destruction |
| `containment/plugin/layoutmanager.cpp:1387` | `save` | `item` | `isParabolicEdgeSpacer` | read | applet/AppletItem.qml:75; layouts/ParabolicEdgeSpacer.qml:14 (Q) | structural: layout changes, destruction |
| `containment/plugin/layoutmanager.cpp:1390` | `save` | `item` | `backendAppletRef` | read | applet/AppletItem.qml:233 (property QtObject) (Q) | structural: layout changes, destruction |
| `containment/plugin/layoutmanager.cpp:1394` | `save` | `item` | `applet` | read | applet/AppletItem.qml:504 (property Item) on containers; Plasma applet object elsewhere (Q/P) | structural: layout changes, destruction |
| `containment/plugin/layoutmanager.cpp:1593` | `insertAtLayoutCoordinates` | `m_plasmoid` | `formFactor` | read | Plasma plasmoid attached object (P) | HOT: every drag move (called per insertAtCoordinates) |
| `containment/plugin/layoutmanager.cpp:1595` | `insertAtLayoutCoordinates` | `layout` | `rowSpacing` | read | Qt Quick Grid; layouts/AppletsContainer.qml:19 sets 0 (Qt type set by Q) | HOT: every drag move (called per insertAtCoordinates) |
| `containment/plugin/layoutmanager.cpp:1596` | `insertAtLayoutCoordinates` | `layout` | `columnSpacing` | read | Qt Quick Grid; layouts/AppletsContainer.qml:16 sets 0 (Qt type set by Q) | HOT: every drag move (called per insertAtCoordinates) |
| `containment/plugin/layoutmanager.cpp:1665, 1674, 1683` | `firstSplitter` | `item` | `isInternalViewSplitter` | read | applet/AppletItem.qml:229; layouts/ParabolicEdgeSpacer.qml:18 (Q) | structural (justify splitters) |
| `containment/plugin/layoutmanager.cpp:1697, 1706, 1715` | `lastSplitter` | `item` | `isInternalViewSplitter` | read | applet/AppletItem.qml:229; layouts/ParabolicEdgeSpacer.qml:18 (Q) | structural (justify splitters) |
| `containment/plugin/layoutmanager.cpp:1733` | `appletItemInLayout` | `item` | `isInternalViewSplitter` | read | applet/AppletItem.qml:229; layouts/ParabolicEdgeSpacer.qml:18 (Q) | structural: per applet during repair/add |
| `containment/plugin/layoutmanager.cpp:1734` | `appletItemInLayout` | `item` | `isParabolicEdgeSpacer` | read | applet/AppletItem.qml:75; layouts/ParabolicEdgeSpacer.qml:14 (Q) | structural: per applet during repair/add |
| `containment/plugin/layoutmanager.cpp:1737` | `appletItemInLayout` | `item` | `backendAppletRef` | read | applet/AppletItem.qml:233 (property QtObject) (Q) | structural: per applet during repair/add |
| `containment/plugin/layoutmanager.cpp:1741` | `appletItemInLayout` | `item` | `applet` | read | applet/AppletItem.qml:504 (property Item) on containers; Plasma applet object elsewhere (Q/P) | structural: per applet during repair/add |
| `containment/plugin/layoutmanager.cpp:1795, 1811, 1827` | `dndSpacerIndex` | `item` | `isParabolicEdgeSpacer` | read | applet/AppletItem.qml:75; layouts/ParabolicEdgeSpacer.qml:14 (Q) | cold: drop only (DragDropArea.qml:230; view.cpp:1618) |
| `containment/plugin/layoutmanager.cpp:2054` | `repairAppletContainers` | `existingItem` | `applet` | read | applet/AppletItem.qml:504 (property Item) on containers; Plasma applet object elsewhere (Q/P) | structural: 150 ms repair timer bursts (main.qml:1217-1253) |
| `containment/plugin/layoutmanager.cpp:2064` | `repairAppletContainers` | `m_initAppletContainerMethod` | `initAppletContainer(QVariant,QVariant)` | call | main.qml:929 (root function) (Q) | structural: 150 ms repair timer bursts (main.qml:1217-1253) |
| `containment/plugin/layoutmanager.cpp:2180, 2267` | `addAppletItem` | `this` | `_latte_pendingInsertionIndex` | read | dynamic property on LayoutManager; set in C++, read in C++ and main.qml:766-769 (D) | structural: applet added |
| `containment/plugin/layoutmanager.cpp:2184, 2271` | `addAppletItem` | `this` | `_latte_pendingInsertionIndex` | write | dynamic property on LayoutManager; set in C++, read in C++ and main.qml:766-769 (D) | structural: applet added |
| `containment/plugin/layoutmanager.cpp:2206, 2301` | `addAppletItem` | `m_createAppletItemMethod` | `createAppletItem(QVariant)` | call | main.qml:803 (root function) (Q) | structural: applet added |
| `containment/plugin/layoutmanager.cpp:2282` | `addAppletItem` | `applet` | `applet` | read | applet/AppletItem.qml:504 (property Item) on containers; Plasma applet object elsewhere (Q/P) | structural: applet added |
| `containment/plugin/layoutmanager.cpp:2292` | `addAppletItem` | `m_initAppletContainerMethod` | `initAppletContainer(QVariant,QVariant)` | call | main.qml:929 (root function) (Q) | structural: applet added |
| `containment/plugin/layoutmanager.cpp:2346` | `removeAppletItem` | `applet` | `applet` | read | applet/AppletItem.qml:504 (property Item) on containers; Plasma applet object elsewhere (Q/P) | structural: applet removed |
| `containment/plugin/layoutmanager.cpp:2378` | `hideAppletItem` | `item` | `isInternalViewSplitter` | read | applet/AppletItem.qml:229; layouts/ParabolicEdgeSpacer.qml:18 (Q) | structural: scheduled destruction |
| `containment/plugin/layoutmanager.cpp:2382` | `hideAppletItem` | `item` | `backendAppletRef` | read | applet/AppletItem.qml:233 (property QtObject) (Q) | structural: scheduled destruction |
| `containment/plugin/layoutmanager.cpp:2385` | `hideAppletItem` | `item` | `applet` | read | applet/AppletItem.qml:504 (property Item) on containers; Plasma applet object elsewhere (Q/P) | structural: scheduled destruction |
| `containment/plugin/layoutmanager.cpp:2421` | `showAppletItem` | `item` | `isInternalViewSplitter` | read | applet/AppletItem.qml:229; layouts/ParabolicEdgeSpacer.qml:18 (Q) | structural: scheduled destruction undo |
| `containment/plugin/layoutmanager.cpp:2425` | `showAppletItem` | `item` | `backendAppletRef` | read | applet/AppletItem.qml:233 (property QtObject) (Q) | structural: scheduled destruction undo |
| `containment/plugin/layoutmanager.cpp:2428` | `showAppletItem` | `item` | `applet` | read | applet/AppletItem.qml:504 (property Item) on containers; Plasma applet object elsewhere (Q/P) | structural: scheduled destruction undo |
| `containment/plugin/layoutmanager.cpp:2483` | `destroyAppletContainer` | `item` | `isInternalViewSplitter` | read | applet/AppletItem.qml:229; layouts/ParabolicEdgeSpacer.qml:18 (Q) | structural |
| `containment/plugin/layoutmanager.cpp:2489` | `destroyAppletContainer` | `item` | `backendAppletRef` | read | applet/AppletItem.qml:233 (property QtObject) (Q) | structural |
| `containment/plugin/layoutmanager.cpp:2492` | `destroyAppletContainer` | `item` | `applet` | read | applet/AppletItem.qml:504 (property Item) on containers; Plasma applet object elsewhere (Q/P) | structural |
| `containment/plugin/layoutmanager.cpp:2531` | `reorderSplitterInStartLayout` | `item` | `isInternalViewSplitter` | read | applet/AppletItem.qml:229; layouts/ParabolicEdgeSpacer.qml:18 (Q) | structural (justify splitters) |
| `containment/plugin/layoutmanager.cpp:2560` | `reorderSplitterInEndLayout` | `item` | `isInternalViewSplitter` | read | applet/AppletItem.qml:229; layouts/ParabolicEdgeSpacer.qml:18 (Q) | structural (justify splitters) |
| `containment/plugin/layoutmanager.cpp:2631, 2661` | `addJustifySplittersInMainLayout` | `m_createJustifySplitterMethod` | `createJustifySplitter()` | call | main.qml:946 (root function) (Q) | structural (justify splitters) |
| `containment/plugin/layoutmanager.cpp:2706` | `destroyJustifySplitters` | `item` | `isInternalViewSplitter` | read | applet/AppletItem.qml:229; layouts/ParabolicEdgeSpacer.qml:18 (Q) | structural (justify splitters) |
| `containment/plugin/layoutmanager.cpp:2792` | `moveAppletsBasedOnJustifyAlignment` | `this` | `isInternalViewSplitter` | read | applet/AppletItem.qml:229; layouts/ParabolicEdgeSpacer.qml:18 (Q) | structural (justify splitters) |
| `view/containmentinterface.cpp:391` | `identifyShortcutsHost` | `m_layoutManager` | `rootItem` | read | LayoutManager Q_PROPERTY layoutmanager.h:35; set by main.qml:1205 (C) | once per containment |
| `view/containmentinterface.cpp:395` | `identifyShortcutsHost` | `m_view->containment()` | `_plasma_graphicObject` | read | Plasma (libplasmaquick); not defined in repo (P) | once per containment |
| `view/containmentinterface.cpp:422` | `identifyMethods` | `m_shortcutsHost->metaObject()` | `activateEntryAtIndex(QVariant)` | resolve | abilities/PositionShortcuts.qml:28 (Q) | once per shortcut host |
| `view/containmentinterface.cpp:423` | `identifyMethods` | `m_shortcutsHost->metaObject()` | `newInstanceForEntryAtIndex(QVariant)` | resolve | abilities/PositionShortcuts.qml:37 (Q) | once per shortcut host |
| `view/containmentinterface.cpp:424` | `identifyMethods` | `m_shortcutsHost->metaObject()` | `setShowAppletShortcutBadges(QVariant,QVariant,QVariant,QVariant)` | resolve | abilities/PositionShortcuts.qml:20 (Q) | once per shortcut host |
| `view/containmentinterface.cpp:425` | `identifyMethods` | `m_shortcutsHost->metaObject()` | `appletIdForIndex(QVariant)` | resolve | abilities/PositionShortcuts.qml:46 (Q) | once per shortcut host |
| `view/containmentinterface.cpp:552` | `updateBadgeForLatteTask` | `metaObject` | `updateBadge(QVariant,QVariant)` | resolve | plasmoid/package/contents/ui/main.qml:1546 (Latte tasks, outside containment/) (Q other) | event-driven: task or badge change |
| `view/containmentinterface.cpp:560` | `updateBadgeForLatteTask` | `method` | `updateBadge(QVariant,QVariant)` | call | plasmoid/package/contents/ui/main.qml:1546 (Latte tasks, outside containment/) (Q other) | event-driven: task or badge change |
| `view/containmentinterface.cpp:597` | `activatePlasmaTask` | `metaObject` | `activateTaskAtIndex(QVariant)` | resolve | plasmoid/package/contents/ui/main.qml:1523 (Latte tasks, outside containment/) (Q other) | user action: shortcut or task click |
| `view/containmentinterface.cpp:605` | `activatePlasmaTask` | `method` | `activateTaskAtIndex(QVariant)` | call | plasmoid/package/contents/ui/main.qml:1523 (Latte tasks, outside containment/) (Q other) | user action: shortcut or task click |
| `view/containmentinterface.cpp:644` | `newInstanceForPlasmaTask` | `metaObject` | `newInstanceForTaskAtIndex(QVariant)` | resolve | plasmoid/package/contents/ui/main.qml:1528 (Latte tasks, outside containment/) (Q other) | user action: shortcut or task click |
| `view/containmentinterface.cpp:652` | `newInstanceForPlasmaTask` | `method` | `newInstanceForTaskAtIndex(QVariant)` | call | plasmoid/package/contents/ui/main.qml:1528 (Latte tasks, outside containment/) (Q other) | user action: shortcut or task click |
| `view/containmentinterface.cpp:674` | `activateEntry` | `m_activateEntryMethod` | `activateEntryAtIndex(QVariant)` | call | abilities/PositionShortcuts.qml:28 (Q) | user action: keyboard shortcut |
| `view/containmentinterface.cpp:685` | `newInstanceForEntry` | `m_newInstanceMethod` | `newInstanceForEntryAtIndex(QVariant)` | call | abilities/PositionShortcuts.qml:37 (Q) | user action: keyboard shortcut |
| `view/containmentinterface.cpp:696` | `hideShortcutBadges` | `m_showShortcutsMethod` | `setShowAppletShortcutBadges(QVariant,QVariant,QVariant,QVariant)` | call | abilities/PositionShortcuts.qml:20 (Q) | user action: modifier key |
| `view/containmentinterface.cpp:715` | `showShortcutBadges` | `m_showShortcutsMethod` | `setShowAppletShortcutBadges(QVariant,QVariant,QVariant,QVariant)` | call | abilities/PositionShortcuts.qml:20 (Q) | user action: modifier key |
| `view/containmentinterface.cpp:728` | `appletIdForVisualIndex` | `m_appletIdForIndexMethod` | `appletIdForIndex(QVariant)` | call | abilities/PositionShortcuts.qml:46 (Q) | user action: shortcut |
| `view/containmentinterface.cpp:754` | `appletIdForAppletIndex` | `m_layoutManager` | `order` | read | LayoutManager Q_PROPERTY layoutmanager.h:51 (READ only) (C) | user action: shortcut |
| `view/containmentinterface.cpp:910` | `configurationForAppletVisualIndex` | `ctx` | `plasmoid (context property)` | read | Plasma plasmoid attached object (P) | user action: shortcut |
| `view/containmentinterface.cpp:916` | `configurationForAppletVisualIndex` | `plasmoidObj` | `configuration` | read | Plasma plasmoid attached object (dynamic) (P) | user action: shortcut |
| `view/containmentinterface.cpp:938` | `configurationForAppletVisualIndex` | `child` | `configuration` | read | Plasma plasmoid attached object (dynamic) (P) | user action: shortcut |
| `view/containmentinterface.cpp:1000` | `setPlasmoid` | `m_plasmoid` | `configuration` | read | Plasma plasmoid attached object (dynamic) (P) | once per containment |
| `view/containmentinterface.cpp:1029` | `setLayoutManager` | `m_layoutManager->metaObject()` | `order` | resolve | LayoutManager Q_PROPERTY layoutmanager.h:51 (READ only) (C) | once per containment |
| `view/containmentinterface.cpp:1032, 1046, 1060` | `setLayoutManager` | `m_layoutManager->metaObject()` | `order / lockedZoomApplets / userBlocksColorizingApplets (by index)` | resolve | LayoutManager Q_PROPERTY via QMetaProperty index (C) | once per containment |
| `view/containmentinterface.cpp:1036` | `setLayoutManager` | `this->metaObject()` | `updateAppletsOrder()` | resolve | ContainmentInterface own private slot (containmentinterface.cpp) (C) | once per containment |
| `view/containmentinterface.cpp:1043` | `setLayoutManager` | `m_layoutManager->metaObject()` | `lockedZoomApplets` | resolve | LayoutManager Q_PROPERTY layoutmanager.h:52 (C) | once per containment |
| `view/containmentinterface.cpp:1050` | `setLayoutManager` | `this->metaObject()` | `updateAppletsInLockedZoom()` | resolve | ContainmentInterface own private slot (C) | once per containment |
| `view/containmentinterface.cpp:1057` | `setLayoutManager` | `m_layoutManager->metaObject()` | `userBlocksColorizingApplets` | resolve | LayoutManager Q_PROPERTY layoutmanager.h:53 (C) | once per containment |
| `view/containmentinterface.cpp:1064` | `setLayoutManager` | `this->metaObject()` | `updateAppletsDisabledColoring()` | resolve | ContainmentInterface own private slot (C) | once per containment |
| `view/containmentinterface.cpp:1087` | `addApplet` | `m_layoutManager` | `order` | read | LayoutManager Q_PROPERTY layoutmanager.h:51 (READ only) (C) | user action: add widget or drop |
| `view/containmentinterface.cpp:1090` | `addApplet` | `m_layoutManager` | `_latte_pendingInsertionIndex` | write | dynamic property on LayoutManager; set in C++, read in C++ and main.qml:766-769 (D) | user action: add widget or drop |
| `view/containmentinterface.cpp:1124` | `addApplet` | `m_plasmoid->metaObject()` | `processMimeData(QObject*,int,int)` | resolve | Plasma containment item; not defined in repo (DragDropArea.qml:242) (P) | user action: add widget or drop |
| `view/containmentinterface.cpp:1132` | `addApplet` | `processmethod` | `processMimeData(QObject*,int,int)` | call | Plasma containment item; not defined in repo (DragDropArea.qml:242) (P) | user action: add widget or drop |
| `view/containmentinterface.cpp:1151` | `addInternalSeparatorBeforeApplet` | `m_layoutManager` | `order` | read | LayoutManager Q_PROPERTY layoutmanager.h:51 (READ only) (C) | user action: add separator |
| `view/containmentinterface.cpp:1222, 1245` | `addInternalSeparatorBeforeApplet` | `m_layoutManager` | `_latte_pendingInsertionIndex` | write | dynamic property on LayoutManager; set in C++, read in C++ and main.qml:766-769 (D) | user action: add separator |
| `view/containmentinterface.cpp:1223, 1246` | `addInternalSeparatorBeforeApplet` | `m_layoutManager` | `_latte_skipSpacerReorder` | write | dynamic property on LayoutManager; set and read in C++ only (D) | user action: add separator |
| `view/containmentinterface.cpp:1295` | `addInternalSeparatorAtLeftBoundaryOfApplet` | `m_layoutManager` | `order` | read | LayoutManager Q_PROPERTY layoutmanager.h:51 (READ only) (C) | user action: add separator |
| `view/containmentinterface.cpp:1374` | `addInternalSeparatorAtRightBoundaryOfApplet` | `m_layoutManager` | `order` | read | LayoutManager Q_PROPERTY layoutmanager.h:51 (READ only) (C) | user action: add separator |
| `view/containmentinterface.cpp:1453` | `removeInternalSeparatorAtLeftBoundaryOfApplet` | `m_layoutManager` | `order` | read | LayoutManager Q_PROPERTY layoutmanager.h:51 (READ only) (C) | user action: remove separator |
| `view/containmentinterface.cpp:1542` | `removeInternalSeparatorAtRightBoundaryOfApplet` | `m_layoutManager` | `order` | read | LayoutManager Q_PROPERTY layoutmanager.h:51 (READ only) (C) | user action: remove separator |
| `view/containmentinterface.cpp:1708` | `cleanupInvalidSeparatorApplets` | `m_layoutManager` | `order` | read | LayoutManager Q_PROPERTY layoutmanager.h:51 (READ only) (C) | deferred cleanup |
| `view/containmentinterface.cpp:1863` | `updateAppletsOrder` | `m_layoutManager` | `order` | read | LayoutManager Q_PROPERTY layoutmanager.h:51 (READ only) (C) | structural: order-changed signal handler |
| `view/containmentinterface.cpp:1918` | `updateAppletsInLockedZoom` | `m_layoutManager` | `lockedZoomApplets` | read | LayoutManager Q_PROPERTY layoutmanager.h:52 (C) | structural: lockedZoom-changed handler |
| `view/containmentinterface.cpp:1976` | `updateAppletsDisabledColoring` | `m_layoutManager` | `userBlocksColorizingApplets` | read | LayoutManager Q_PROPERTY layoutmanager.h:53 (C) | structural: coloring-changed handler |
| `view/containmentinterface.cpp:2083` | `setAppletsOrder` | `m_layoutManager` | `requestAppletsOrder` | call | LayoutManager Q_INVOKABLE layoutmanager.h:123 (C) | structural: sync to containment config |
| `view/containmentinterface.cpp:2091` | `setAppletsInLockedZoom` | `m_layoutManager` | `requestAppletsInLockedZoom` | call | LayoutManager Q_INVOKABLE layoutmanager.h:124 (C) | user action: context menu or settings |
| `view/containmentinterface.cpp:2099` | `setAppletsDisabledColoring` | `m_layoutManager` | `requestAppletsDisabledColoring` | call | LayoutManager Q_INVOKABLE layoutmanager.h:125 (C) | user action: context menu or settings |
| `view/containmentinterface.cpp:2107` | `setAppletInScheduledDestruction` | `m_layoutManager` | `setAppletInScheduledDestruction` | call | LayoutManager Q_INVOKABLE layoutmanager.h:126 (C) | structural: removal and undo |
| `view/containmentinterface.cpp:2236` | `appletConfiguration` | `ai` | `configuration` | read | Plasma plasmoid attached object (dynamic) (P) | structural: per applet creation |
| `view/containmentinterface.cpp:2245` | `appletConfiguration` | `subcontainment` | `_plasma_graphicObject` | read | Plasma (libplasmaquick); not defined in repo (P) | structural: per applet creation |
| `view/containmentinterface.cpp:2248` | `appletConfiguration` | `subcai` | `configuration` | read | Plasma plasmoid attached object (dynamic) (P) | structural: per applet creation |
| `view/containmentinterface.cpp:2291` | `onAppletAdded` | `m_layoutManager` | `_latte_pendingInsertionIndex` | read | dynamic property on LayoutManager; set in C++, read in C++ and main.qml:766-769 (D) | structural: applet added |
| `view/containmentinterface.cpp:2295` | `onAppletAdded` | `m_layoutManager` | `_latte_pendingInsertionIndex` | write | dynamic property on LayoutManager; set in C++, read in C++ and main.qml:766-769 (D) | structural: applet added |
| `view/containmentinterface.cpp:2299, 2314` | `onAppletAdded` | `m_layoutManager` | `addAppletItem` | call | LayoutManager Q_INVOKABLE layoutmanager.h:98-99 (two overloads) (C) | structural: applet added |
| `view/containmentinterface.cpp:2307` | `onAppletAdded` | `m_layoutManager` | `order` | read | LayoutManager Q_PROPERTY layoutmanager.h:51 (READ only) (C) | structural: applet added |
| `view/containmentinterface.cpp:2428` | `onAppletAdded` | `deferredAi` | `configuration` | read | Plasma plasmoid attached object (dynamic) (P) | structural: applet added |
| `view/containmentinterface.cpp:2432` | `onAppletAdded` | `deferredAi` | `configuration` | write | Plasma plasmoid attached object (dynamic) (P) | structural: applet added |
| `view/containmentinterface.cpp:2477` | `onAppletAdded` | `m_layoutManager` | `showAppletItem` | call | LayoutManager Q_INVOKABLE layoutmanager.h:102 (C) | structural: applet added |
| `view/view.cpp:494` | `init` | `obj` | `_latte_globalShortcuts_object` | write | dynamic property set by view.cpp:494 (D) | once per view/containment setup |
| `view/view.cpp:495` | `init` | `obj` | `_latte_layoutsManager_object` | write | dynamic property set by view.cpp:495 (D) | once per view/containment setup |
| `view/view.cpp:496` | `init` | `obj` | `_latte_themeExtended_object` | write | dynamic property set by view.cpp:496 (D) | once per view/containment setup |
| `view/view.cpp:497` | `init` | `obj` | `_latte_universalSettings_object` | write | dynamic property set by view.cpp:497 (D) | once per view/containment setup |
| `view/view.cpp:498` | `init` | `obj` | `_latte_view_object` | write | dynamic property set by view.cpp:494-498 (D) | once per view/containment setup |
| `view/view.cpp:503` | `init` | `plasma_containment` | `_plasma_graphicObject` | read | Plasma (libplasmaquick); not defined in repo (P) | once per view/containment setup |
| `view/view.cpp:508` | `init` | `containmentGraphicItem` | `_latte_view_interfacesobject` | read | dynamic property set by view.cpp:1784 (D) | once per view/containment setup |
| `view/view.cpp:953` | `eventFilter` | `appletGuard.data()` | `reset` | call | applet interface object (view.cpp:953); owner unverified (P) | event-driven: applet guard and dynamic property events |
| `view/view.cpp:1560` | `updatePlasmoidDrag` | `lm` | `dndSpacerItem` | read | LayoutManager Q_PROPERTY layoutmanager.h:40; set by main.qml:1206 (C) | HOT: every drag move (0 ms timer restarted per move) |
| `view/view.cpp:1571` | `updatePlasmoidDrag` | `lm` | `rootItem` | read | LayoutManager Q_PROPERTY layoutmanager.h:35; set by main.qml:1205 (C) | HOT: every drag move (0 ms timer restarted per move) |
| `view/view.cpp:1576` | `updatePlasmoidDrag` | `lm` | `insertAtCoordinates` | call | LayoutManager Q_INVOKABLE layoutmanager.h:109 (C) | HOT: every drag move (0 ms timer restarted per move) |
| `view/view.cpp:1604` | `handlePlasmoidDrop` | `lm` | `dndSpacerItem` | read | LayoutManager Q_PROPERTY layoutmanager.h:40; set by main.qml:1206 (C) | cold: once per drop |
| `view/view.cpp:1608` | `handlePlasmoidDrop` | `lm` | `rootItem` | read | LayoutManager Q_PROPERTY layoutmanager.h:35; set by main.qml:1205 (C) | cold: once per drop |
| `view/view.cpp:1613` | `handlePlasmoidDrop` | `lm` | `insertAtCoordinates` | call | LayoutManager Q_INVOKABLE layoutmanager.h:109 (C) | cold: once per drop |
| `view/view.cpp:1618` | `handlePlasmoidDrop` | `lm` | `dndSpacerIndex` | call | LayoutManager Q_INVOKABLE layoutmanager.h:116 (C) | cold: once per drop |
| `view/view.cpp:1634` | `handlePlasmoidDrop` | `m_interface->layoutManager()` | `_latte_pendingInsertionIndex` | write | dynamic property on LayoutManager; set in C++, read in C++ and main.qml:766-769 (D) | cold: once per drop |
| `view/view.cpp:1657` | `cleanupDndSpacer` | `lm` | `dndSpacerItem` | read | LayoutManager Q_PROPERTY layoutmanager.h:40; set by main.qml:1206 (C) | cold: drag end |
| `view/view.cpp:1661` | `cleanupDndSpacer` | `lm` | `rootItem` | read | LayoutManager Q_PROPERTY layoutmanager.h:35; set by main.qml:1205 (C) | cold: drag end |
| `view/view.cpp:1781` | `setInterfacesGraphicObj` | `containment()` | `_plasma_graphicObject` | read | Plasma (libplasmaquick); not defined in repo (P) | cold: setup |
| `view/view.cpp:1784` | `setInterfacesGraphicObj` | `containmentGraphicItem` | `_latte_view_interfacesobject` | write | dynamic property set by view.cpp:1784 (D) | cold: setup |
| `view/view.cpp:1847` | `event` | `qApp` | `latte_session_ending` | read | dynamic property on qApp (app-level; not containment) (D) | cold: session end |
| `view/view.cpp:2106, 2133` | `updateSystemColorScheme` | `this` | `KDE_COLOR_SCHEME_PATH` | write | dynamic property on View (app-level; not containment) (D) | cold: color scheme change |
| `view/view.cpp:2127` | `updateSystemColorScheme` | `this` | `KDE_COLOR_SCHEME_PATH` | read | dynamic property on View (app-level; not containment) (D) | cold: color scheme change |
| `declarativeimports/contextmenulayerquickitem.cpp:270` | `mousePressEvent` | `m_appletContainsMethod` | `appletContainsPos(QVariant,QVariant)` | call | main.qml:1066 (root function) (Q) | cold: click |
| `declarativeimports/contextmenulayerquickitem.cpp:425` | `updateAppletContainsMethod` | `metaObject` | `appletContainsPos(QVariant,QVariant)` | resolve | main.qml:1066 (root function) (Q) | cold: context menu open |
| `declarativeimports/contextmenulayerquickitem.cpp:458` | `addAppletActions` | `layoutMgr` | `userBlocksColorizingApplets` | read | LayoutManager Q_PROPERTY layoutmanager.h:53 (C) | cold: context menu build |
| `declarativeimports/contextmenulayerquickitem.cpp:466` | `addAppletActions` | `layoutMgr` | `setOption` | call | LayoutManager Q_INVOKABLE layoutmanager.h:96 (C) | cold: context menu build |
| `declarativeimports/contextmenulayerquickitem.cpp:545` | `addAppletActions` | `layoutMgr` | `hideAppletItem` | call | LayoutManager Q_INVOKABLE layoutmanager.h:101 (C) | cold: context menu build |
| `declarativeimports/contextmenulayerquickitem.cpp:559` | `addAppletActions` | `layoutMgr` | `showAppletItem` | call | LayoutManager Q_INVOKABLE layoutmanager.h:102 (C) | cold: context menu build |
| `declarativeimports/contextmenulayerquickitem.cpp:574` | `addAppletActions` | `layoutMgr` | `removeAppletItem` | call | LayoutManager Q_INVOKABLE layoutmanager.h:100 (C) | cold: context menu build |
| `declarativeimports/interfaces.cpp:130` | `updateView` | `source` | `_latte_view_object` | read | dynamic property set by view.cpp:494-498 (D) | cold: view change |
| `declarativeimports/interfaces.cpp:133` | `updateView` | `m_plasmoidInterface` | `_latte_view_object` | read | dynamic property set by view.cpp:494-498 (D) | cold: view change |
| `declarativeimports/interfaces.cpp:262` | `syncPlasmoidReference` | `m_plasmoidInterface` | `_plasma_graphicObject` | read | Plasma (libplasmaquick); not defined in repo (P) | event-driven: dynamic property change |
| `declarativeimports/interfaces.cpp:289` | `syncPlasmoidObjects` | `source` | `_latte_globalShortcuts_object` | read | dynamic property set by view.cpp:494 (D) | event-driven: dynamic property change |
| `declarativeimports/interfaces.cpp:290` | `syncPlasmoidObjects` | `source` | `_latte_layoutsManager_object` | read | dynamic property set by view.cpp:495 (D) | event-driven: dynamic property change |
| `declarativeimports/interfaces.cpp:291` | `syncPlasmoidObjects` | `source` | `_latte_themeExtended_object` | read | dynamic property set by view.cpp:496 (D) | event-driven: dynamic property change |
| `declarativeimports/interfaces.cpp:292` | `syncPlasmoidObjects` | `source` | `_latte_universalSettings_object` | read | dynamic property set by view.cpp:497 (D) | event-driven: dynamic property change |
| `view/parabolic.cpp:72` | `setCurrentParabolicItem` | `m_currentParabolicItem` | `parabolicExited` | call | applet/ParabolicArea.qml:13 (signal) (Q) | warm: each parabolic item switch |
| `view/parabolic.cpp:114` | `onEvent` | `m_currentParabolicItem` | `parabolicMove` | call | applet/ParabolicArea.qml:12 (signal) (Q) | HOT: every pointer move over parabolic area (parabolic.cpp:114) |
| `view/parabolic.cpp:173` | `onEvent` | `child->metaObject()` | `parabolicEntered(real,real)` | resolve | applet/ParabolicArea.qml:11 (signal) (Q) | HOT: every pointer move over parabolic area (parabolic.cpp:114) |
| `view/parabolic.cpp:211` | `onCurrentParabolicItemChanged` | `m_currentParabolicItem` | `parabolicEntered` | call | applet/ParabolicArea.qml:11 (signal) (Q) | warm: each parabolic item switch |
| `view/effects.cpp:339` | `setAppletsLayoutGeometry` | `m_view` | `_applets_layout_geometry` | write | dynamic property on View; read by declarativeimports/core/dialog.cpp:113 (D) | warm: layout geometry change (frequency not verified) |
| `view/effects.cpp:361` | `onPopUpMarginChanged` | `m_view` | `_applets_popup_margin` | write | dynamic property on View; read by declarativeimports/core/dialog.cpp:119 (D) | cold: margin change |
| `view/effects.cpp:524` | `updateEffects` | `m_panelBackgroundSvg` | `mask` | read | Plasma Svg object m_panelBackgroundSvg (effects.cpp:524); owner Plasma/Qt (P) | warm: effects refresh (frequency not verified) |
| `view/settings/subconfigview.cpp:35` | `findGraphicContextObject` | `containmentObject` | `_plasma_graphicObject` | read | Plasma (libplasmaquick); not defined in repo (P) | cold: settings UI |
| `view/settings/subconfigview.cpp:41` | `findGraphicContextObject` | `containmentObject` | `graphicObject` | read | unknown owner; probed fallback (subconfigview.cpp:41) (P) | cold: settings UI |
| `view/settings/subconfigview.cpp:161` | `setRootInput` | `root` | `<input name from setRootInput>` | write | settings QML root input names (R4 pattern; subconfigview.cpp:147-162) (Q (settings)) | cold: settings UI |
| `view/settings/subconfigview.cpp:267` | `syncPlasmoidInput` | `metaObj` | `configuration` | resolve | Plasma plasmoid attached object (dynamic) (P) | cold: settings UI and diagnostics |
| `view/settings/subconfigview.cpp:268` | `syncPlasmoidInput` | `metaObj` | `location` | resolve | Plasma plasmoid attached object (P) | cold: settings UI and diagnostics |
| `view/settings/subconfigview.cpp:269` | `syncPlasmoidInput` | `metaObj` | `formFactor` | resolve | Plasma plasmoid attached object (P) | cold: settings UI and diagnostics |
| `view/settings/subconfigview.cpp:290` | `syncPlasmoidInput` | `metaObj` | `_plasma_graphicObject` | resolve | Plasma (libplasmaquick); not defined in repo (P) | cold: settings UI and diagnostics |
| `view/settings/subconfigview.cpp:291` | `syncPlasmoidInput` | `metaObj` | `graphicObject` | resolve | unknown owner; probed fallback (subconfigview.cpp:41) (P) | cold: settings UI and diagnostics |

### 4.2 QML side: calls and bindings that touch C++ or are touched by C++

These rows are compiled by hand from the QML files; the QML has no equivalent of the C++ script. Each row names the property or function and whether C++ reads or calls it.

| Location | QML context | Dir | Name(s) | Target | Owner | Hot path |
| --- | --- | --- | --- | --- | --- | --- |
| `containment/package/contents/ui/main.qml:1202-1215` | `LatteContainment.LayoutManager` element | write (typed) | `plasmoidObj`, `rootItem`, `dndSpacerItem`, `mainLayout`, `startLayout`, `endLayout`, `metrics`; handlers `onAppletOrderChanged`, `onSplitterPositionChanged`, `onSplitterPosition2Changed` | `LayoutManager` Q_PROPERTY (typed) | Q to C, typed | once (the pattern to extend) |
| `main.qml:766-769` | `Containment.onAppletAdded` | read, write | `_latte_pendingInsertionIndex` | `LayoutManager` dynamic property | D | structural |
| `main.qml:774`, `:778-779` | `Containment.onAppletAdded` | read, call | `order`, `defaultInsertionIndex()`, `addAppletItem(applet, index)` | `LayoutManager` | C | structural |
| `main.qml:785`, `:789-790` | `onAppletRemoved` | call | `removeAppletItem` | `LayoutManager` | C | structural |
| `main.qml:520-610` (7 places), `:547`, `:582`, `:618`, `:734`, `:747` | layout changes, restore, destruction | call | `joinLayoutsToMainLayout`, `save`, `restore` | `LayoutManager` | C | structural |
| `main.qml:897-899` | `resolveAppletItem()` | `typeof` check, call | `resolveAppletQuickItem` | `LayoutManager`; falls through silently when absent | C | structural |
| `main.qml:1050-1057` | `forceModernAppletsToMainLayout()` | `typeof` check, call | `insertBefore`, `insertAfter` | `LayoutManager`; falls through silently when absent | C | structural |
| `main.qml:1222`, `:1240` | repair timers (`repairAppletContainersTimer`, `runtimeAppletRepairTimer`) | call | `repairAppletContainers` | `LayoutManager` | C | structural (150 ms repeats) |
| `main.qml:803-806`, `:929-931` | `createAppletItem()`, `initAppletContainer()` | write | `backendAppletRef`, `sourceAppletPluginName` on the new container | `AppletItem.qml:233`, `:477` | Q | structural; also called from C++ (family 3) |
| `main.qml:938` | `initAppletContainer()` | write | `applet` | `AppletItem.qml:504` | Q | structural |
| `main.qml:948` | `createJustifySplitter()` | write | `internalSplitterId` | `AppletItem.qml:357`; legacy path | Q | dead while `usesLegacyJustifySplitters()` is false (S3) |
| `main.qml:954-968`, `:1015`, `:1093-1109` | `forceDestroyInternalSplittersNow()`, `internalViewSplittersCount()` | read | `isInternalViewSplitter`, `isParabolicEdgeSpacer`, `isParabolicEdgeSpacer` | `AppletItem.qml:229`; `layouts/ParabolicEdgeSpacer.qml:14`, `:18` | Q | cold |
| `main.qml:813-833` | `createAppletItem()` retry | string-compiled QML (`Qt.createQmlObject`), `triggered.connect` | none (creates a `Timer` from a string) | none | Q | structural (startup) |
| `main.qml:1066-1089` | `appletContainsPos()` (called from C++) | read | `child.applet.id`, `child.containsPos()` | `AppletItem.qml` | Q | cold (context menu) |
| `containment/package/contents/ui/DragDropArea.qml:171`, `:190` | `onDragEnter`, `onDragMove` | call | `insertAtCoordinates` | `LayoutManager` | C | HOT per move |
| `DragDropArea.qml:230`, `:235` | `onDrop` | call | `dndSpacerIndex`, `indexToMasquearadedPoint` | `LayoutManager` | C | cold (drop) |
| `containment/package/contents/ui/applet/AppletItem.qml:164`, `:184-188` | item bindings | read | `lockedZoomApplets`, `userBlocksColorizingApplets` (outer id) | `LayoutManager` | C | binding (on change) |
| `AppletItem.qml:216` | `isScheduledForDestruction` binding | read | `appletsInScheduledDestruction` (outer id) | `LayoutManager` | C | binding (on change) |
| `AppletItem.qml:234-255` | `backendAppletId`, `backendAppletIcon`, `backendAppletIconPath` bindings | call | `appletId`, `appletIcon`, `appletIconPath` | `LayoutManager` | C | binding (on change) |
| `AppletItem.qml:1070-1088`, `:1181-1207` | drag reorder handlers | call | `insertBefore`, `insertAfter` | `LayoutManager` | C | user action (drag end) |
| `AppletItem.qml:1243` | `finishSortDrag()` | call | `moveAppletsBasedOnJustifyAlignment` (calls `joinLayoutsToMainLayout` when the legacy path is off, `layoutmanager.cpp:2767-2774`) | `LayoutManager` | C | user action |
| `containment/package/contents/ui/layouts/ParabolicEdgeSpacer.qml:14`, `:18` | readonly properties | read by C++ | `isParabolicEdgeSpacer`, `isInternalViewSplitter` | project QML objects | Q | structural |
| `containment/package/contents/ui/layouts/LayoutsContainer.qml:349-350` | readonly aliases on `_mainLayout` | read by C++ | `startParabolicSpacer`, `endParabolicSpacer` | project QML objects (`:411-417`) | Q | structural (`layoutmanager.cpp:1353-1354`) |
| `containment/package/contents/ui/layouts/AppletsContainer.qml:16`, `:19` | `Grid` settings | read by C++ | `columnSpacing`, `rowSpacing` (set to 0) | Qt Quick `Grid` | Qt on Q | HOT per move (`layoutmanager.cpp:1595-1596`) |
| `containment/package/contents/ui/BindingsExternal.qml:24` | `required property var fastLayoutManager` | untyped input | `fastLayoutManager` | `LayoutManager` | Q | (type information lost) |
| `BindingsExternal.qml:383-386` | `Binding` | write | `plasmoid` on `latteView.extendedInterface` | `ContainmentInterface` Q_PROPERTY | C | once per containment |
| `BindingsExternal.qml:389-392` | `Binding` | write | `layoutManager` on `latteView.extendedInterface` (the bridge to family 1) | `ContainmentInterface` Q_PROPERTY (`containmentinterface.h:65`) | C | once per containment |
| `BindingsExternal.qml:43-360` (32 blocks) | `Binding` | write | `maxThickness`, `normalThickness`, `maxNormalThickness`, `headThicknessGap`, `type`, `fontPixelSize`, `maxLength`, `offset`, `screenEdgeMargin`, `screenEdgeMarginEnabled`, `alignment`, `isTouchingTopViewAndIsBusy`, `isTouchingBottomViewAndIsBusy`, `colorizer`, `metrics`, `backgroundAllCorners`, `backgroundRadius`, `backgroundRadiusEnabled`, `backgroundOpacity`, `effectiveBackgroundOpacity`, `drawEffects`, `drawShadows`, `editShadow`, `innerShadow`, `panelBackgroundSvg`, `appletsLayoutGeometry`, `isStickedOnTopEdge`, `isStickedOnBottomEdge`, `isShownFully`, `strutsThickness`, `isFloatingGapWindowEnabled`, `enabled` | `latteView`, `latteView.effects`, `.visibility`, `.positioner`, `.windowsTracker`, `.extendedInterface` (C++ Q_PROPERTYs, not inventoried one by one) | C | run time (not verified) |
| `containment/package/contents/ui/abilities/PositionShortcuts.qml:12`, `:20`, `:28`, `:37`, `:46` | shortcut host | objectName, function | `PositionShortcutsAbilityHost`, `setShowAppletShortcutBadges`, `activateEntryAtIndex`, `newInstanceForEntryAtIndex`, `appletIdForIndex` | called from `containmentinterface.cpp` (family 3) | Q | user action |
| `containment/package/contents/ui/applet/ParabolicArea.qml:11-13`, emits at `:88`, `:100`, `:180` | `ParabolicArea` signals | call | `parabolicEntered`, `parabolicMove`, `parabolicExited` | called from `app/view/parabolic.cpp:72`, `:114`, `:173`, `:211` | Q | HOT per pointer move |
| `containment/package/contents/ui/applet/communicator/Actions.qml:11` | `function setProperty(appletId, parameter, value)` | none | none | none | Q | not an access: a name collision with the grep (excluded) |

### 4.3 Hot-path summary (from 4.1 and 4.2)

- **Per drag move (HOT, 8 access lines):** `containment/plugin/layoutmanager.cpp:478`, `:481` (`readConfigValue("alignment")` reads `myView` and `alignment`), `:1593`, `:1595`, `:1596` (`formFactor`, `rowSpacing`, `columnSpacing`), reached from `app/view/view.cpp:1560`, `:1571`, `:1576` (`dndSpacerItem`, `rootItem`, `insertAtCoordinates`) on every move of a plasmoid drag. QML drags call `insertAtCoordinates` from `DragDropArea.qml:171` and `:190`.
- **Per pointer move over the parabolic area (HOT, 2 lines):** `app/view/parabolic.cpp:114` (QML signal invoked by name on every move) and `:173` (name lookup in the drag child walk, cached per drag position).
- **Warm (per item switch or geometry change):** `app/view/parabolic.cpp:72`, `:211`; `app/view/effects.cpp:339`, `:524`.
- **Event-driven:** 8 lines (dynamic-property change handlers, the applet guard and the task badge path).
- **User action:** 28 lines (shortcuts, context menu, add or remove separator, add widget).
- **Structural or cold:** 145 lines (restore, add and remove, repair timers, setup, settings UI). Restore and repair run under bounded timers (`layoutmanager.cpp:1087-1091`, `:1116-1118`, `:1283-1285`; `main.qml:1217-1253`).
- **No measurement exists** for any of these paths. The estimates in section 10 are reasoning only.

## 5. Families and typed alternatives

Five families cover the inventory. Each lists the typed alternative and what has to stay dynamic, and why. Hotness refers to the table in 4.1 (hot means per pointer or drag event).

### Family 1: `LayoutManager` API used by the application (C++ to C++ across the plugin boundary)

- **Sites:** 48 access lines: 33 in `app/view/containmentinterface.cpp` (for example `m_layoutManager->property("order")` at `:754` and `:1087`; `QMetaObject::invokeMethod(m_layoutManager, "requestAppletsOrder", …)` at `:2083`), 10 in `app/view/view.cpp` (`dndSpacerItem`, `rootItem`, `insertAtCoordinates`, `dndSpacerIndex` at `:1560-1618`, `:1657-1661`) and 5 in `app/declarativeimports/contextmenulayerquickitem.cpp` (`:458-574`). About 9 of the 33 are the notify connects at `:1029-1066`, which S2 leaves unchanged.
- **Why it is string-based:** the plugin boundary, stated at `app/view/containmentinterface.cpp:2079-2083` and `:2297-2298`. The class is compiled into `lattecontainmentplugin` only (`containment/CMakeLists.txt:5-11`, `:18-26`), so no `LayoutManager::` symbol can be referenced from `latte-dock-ng`.
- **Typed alternative:** a pure C++ interface, for example `Latte::Containment::LayoutManagerApi`, in `containment/plugin/layoutmanager.h`, declared with `Q_DECLARE_INTERFACE` and implemented by `LayoutManager` with `Q_INTERFACES`. Getters and invokables become virtual (`override`); `ContainmentInterface::setLayoutManager()` (`containmentinterface.cpp:1015-1025`) stores `qobject_cast<LayoutManagerApi *>(manager)` next to the existing `QPointer<QObject>` (`containmentinterface.h:230`), which stays the lifetime owner. Calls through a pure virtual interface need no symbol from the plugin, because the vtable comes from the object created in the plugin.
- **Signals:** the `QMetaProperty` notify connections at `containmentinterface.cpp:1029-1066` need a `QObject` signal; an interface cannot declare one. They stay name-based in S2 and can move to an observer interface later.
- **Stays dynamic:** nothing once the interface exists, except the notify connections above.

### Family 2: containment QML state and layout-item roles read by C++ (QML to C++ pulls)

- **Sites:** `myView` and `alignment` read through the root (`layoutmanager.cpp:478-481`, drag-hot via `:1941`); `formFactor`, `rowSpacing`, `columnSpacing` (`:1593-1596`, drag-hot); `isInternalViewSplitter`, `isParabolicEdgeSpacer`, `backendAppletRef`, `applet` on layout children (`:1331`, `:1386-1394`, `:1665-1741`, `:1795-1827`, `:2378-2492`, `:2531-2792`); `startParabolicSpacer` and `endParabolicSpacer` on `mainLayout` (`:1353-1354`).
- **Owners:** project QML. `alignment` is `plasmoid.configuration.alignment` through `abilities/MyView.qml:33`; `isInternalViewSplitter` is `applet/AppletItem.qml:229` and `layouts/ParabolicEdgeSpacer.qml:18`; `startParabolicSpacer` is an alias at `layouts/LayoutsContainer.qml:349`. `formFactor` is Plasma-owned (family 5).
- **Typed alternatives:**
  - *Pushed scalars and objects (S5):* typed `Q_PROPERTY` values with `NOTIFY` on `LayoutManager`, bound in the existing `LatteContainment.LayoutManager { }` block (`main.qml:1202-1215`, which already pushes `rootItem`, `mainLayout`, `startLayout`, `endLayout`, `dndSpacerItem` and `metrics` typed). This is the pattern R4 used for the settings windows.
  - *Layout-item roles (S6):* a C++ `QQuickItem` base type with typed properties; `AppletItem.qml` and `ParabolicEdgeSpacer.qml` derive from it, and the C++ `qobject_cast` replaces the reads. The QML root type changes, so this is larger.
- **Stays dynamic:** `applet` and `backendAppletRef` point at Plasma-owned applet objects in part, so the C++ may still need a name-based fallback (family 5).
- **QML-side note:** 70 QML lines mention `fastLayoutManager` (56 member accesses), mostly as an outer id. `applet/AppletItem.qml:241` says so in a comment, and `BindingsExternal.qml:24` declares it `required property var`. qmllint cannot check these members, and `docs/qmllint-baseline.json` has no entry that mentions `fastLayoutManager` (grep count 0). A typed `required property` of the registered `LayoutManager` type is the QML-side fix (not verified for qmllint).

### Family 3: QML functions and signals called by C++ (C++ to QML calls)

- **Sites:** `createAppletItem(QVariant)`, `initAppletContainer(QVariant,QVariant)` and `createJustifySplitter()` on the containment root (cached at `layoutmanager.cpp:1035-1042`, invoked at `:1232`, `:1250`, `:1268`, `:2064`, `:2206`, `:2292`, `:2301`, `:2631`, `:2661`); `appletContainsPos(QVariant,QVariant)` (`contextmenulayerquickitem.cpp:418-436`, invoked at `:270`); the shortcut host functions in `abilities/PositionShortcuts.qml:20-46` (`containmentinterface.cpp:391-430`, `:674-728`); the parabolic signals in `applet/ParabolicArea.qml:11-13` (`app/view/parabolic.cpp:72`, `:114`, `:173`, `:211`); the Latte tasks functions in the `plasmoid/` applet (`containmentinterface.cpp:540-660`, outside `containment/`).
- **Typed alternatives:**
  - *Creation trio (S7):* create `AppletItem` containers in C++ from a typed `QQmlComponent*` pushed from QML, using the C++ resolver that already exists (`layoutmanager.cpp:553-721`). The JavaScript retry (`main.qml:813-833`, `Qt.createQmlObject`) and the opacity bindings (`main.qml:841-843`) must be preserved or moved deliberately.
  - *Requests that need a result (S7 or later):* a typed C++ signal carrying a request object; delivery through a direct connection is synchronous, and the QML handler writes the result into the request.
  - *Per-event calls (S8, conditional):* cache the `QMetaMethod` per item, as the shortcut host already does (`containmentinterface.cpp:422-430`, `:674-728`). The per-move invocations in `parabolic.cpp:114-119` look up the method name on every call.
- **Stays dynamic:** `processMimeData(QObject*,int,int)` is Plasma-owned and not defined in this repository (called at `containmentinterface.cpp:1124-1132` and `containment/package/contents/ui/DragDropArea.qml:240-242`). `Containment.itemFor` and `Plasmoid.itemFor` are Plasma-owned (`main.qml:904`, `:911`).
- **Tests that already exist:** `autotests/containmentlayoutunittest.cpp:27-33` implements `createAppletItem` on a C++ root stub (`RestoreRoot`), so the restore path runs without QML. The stub does not implement `initAppletContainer` or `createJustifySplitter`, so those paths are not exercised by that test.

### Family 4: name channels in both directions (dynamic properties and QML `Binding { property: }`)

- **Pending insertion and skip flag (S4):** `_latte_pendingInsertionIndex` is set at `containmentinterface.cpp:1090`, `:1222`, `:1245`, `:2295` and `app/view/view.cpp:1634`; read and cleared at `layoutmanager.cpp:2180-2184` and `:2267-2271`; read and cleared from QML at `main.qml:766-769`. `_latte_skipSpacerReorder` is set at `containmentinterface.cpp:1223`, cleared at `:1246`, and read and cleared at `layoutmanager.cpp:1373-1378`. The protocol is a one-shot token around the synchronous `createApplet()` call (`containmentinterface.cpp:1213-1221`).
- **Typed alternative:** typed methods on `LayoutManagerApi`, for example `expectAppletInsertionAt(int)`, `takeAppletInsertionIndex()` and `expectSkipSpacerReorderOnce()`. The QML handler `main.qml:763-786` calls `takeAppletInsertionIndex()` instead of reading a dynamic property.
- **Hazard (not reproduced):** `view.cpp:1634` sets the pending index. If `createApplet()` adds no applet, no code on that path clears it; the clears are at `containmentinterface.cpp:1245-1246`, `:2295` and `layoutmanager.cpp:2184`, `:2271`. The next applet added would then be placed at the stale index. S4 clears the token after `createApplet()` returns, after verifying that `appletAdded` fires synchronously (the comment at `containmentinterface.cpp:1213-1215` says it does).
- **View handoff:** `_latte_view_object` and four siblings are set on the containment object and graphic item (`app/view/view.cpp:494-498`, `:508`, `:1784`) and read by `app/declarativeimports/interfaces.cpp:130-133`, `:289-292`, with a `QDynamicPropertyChangeEvent` filter by name (`interfaces.cpp:224-232`). This is not LayoutManager-specific; a typed handoff is a later slice (section 11).
- **`BindingsExternal.qml`:** 34 `Binding { target: …; property: "…" }` blocks write named properties into C++ objects (`latteView`, `latteView.effects`, `latteView.visibility`, `latteView.positioner`, `latteView.windowsTracker`, `latteView.extendedInterface`). Lines 383 (`plasmoid`) and 389-392 (`layoutManager`) feed `ContainmentInterface`. QML reports a missing target property at run time, but qmllint cannot check the targets because `latteView` is `required property var` (`BindingsExternal.qml:16`). The fix is the R4 pattern (typed properties) and is outside LayoutManager; it is listed in section 11.
- **Stays dynamic:** none of the LayoutManager channels need to be dynamic. Plasma-owned names are family 5.

### Family 5: Plasma-owned names (must stay dynamic; encapsulate)

- **Names:** `configuration` (`layoutmanager.cpp:318`; `containmentinterface.cpp:916`, `:938`, `:1000`, `:2236`, `:2248`, `:2428-2432`; the comment at `:2233-2234` says it is a dynamic property in Plasma 6), `plasmoid` (`layoutmanager.cpp:549`), `applets` (`:769`), `formFactor` (`:1593`), `itemFor(QObject*)` and `itemFor(QVariant)` (`:640-653`), `_plasma_graphicObject` (`:624`; `containmentinterface.cpp:395`, `:2245`; `view.cpp:503`, `:1781`; `interfaces.cpp:262`; `settings/subconfigview.cpp:35`), `graphicObject` (`subconfigview.cpp:41`), `applet`, `item`, `id`, `pluginName`, `status` (`layoutmanager.cpp:577-695`, `:938`, `:956`), the `plasmoid` context property (`containmentinterface.cpp:910`) and `reset` on the applet interface (`view.cpp:953`).
- **Typed where Plasma offers typed API:** `PlasmaQuick::AppletQuickItem::itemForApplet()` (`layoutmanager.cpp:592`, `:609`, `:670`), `Plasma::Applet::id()` (`:928`), `Plasma::Containment::applets()` (`:811`, `:976`), `Plasma::Applet::icon()` (`:978`), `Plasma::Applet::pluginMetaData()` (`:829`), `containment->config()` (`:503`, `:524`). The resolver already tries these first.
- **What has to stay dynamic and why:** the Plasma name fallbacks, because the object may be a Plasma applet, an applet QML item or a test double. Keep them in one resolver with an explicit allow-list, and test each branch with `QObject` fakes (`containmentlayoutunittest.cpp:66-80` already does).
- **Floor evidence needed before removing a fallback:** the repository documents that libplasma 6.8 does not publish `_plasma_graphicObject` (`app/alternativeshelper.cpp:26-29`; `autotests/sourcecontracttest.cpp:4272-4275`). Whether the Debian 13 floor (Plasma 6.3.6, `docs/development-testing-guide.md:33-60`) publishes it is not verified.

## 6. Whole-design options

| | Option 1: typed seams in the current module split (recommended) | Option 2: move `LayoutManager` into the executable-backed module or a shared static library | Option 3: model-driven applet containers |
| --- | --- | --- | --- |
| What changes | `LayoutManagerApi` interface; typed QML pushes; typed one-shot methods; dead branch removal; one Plasma resolver; C++ creation later | `LayoutManager` moves to `org.kde.latte.private.app` (or a static library linked into both targets); the app connects typed signals directly | C++ owns a `QAbstractListModel` of layout entries; QML `Repeater`s create the delegates; creation, repair and restore timers shrink |
| Typed signals | No (observer or name-based connect remains for notify) | Yes | Yes |
| QML import and compatibility | Unchanged URI and type names | `LatteContainment.LayoutManager` changes to the new module; `qmldir`, install paths and the qmltypes install change | Model roles become QML-facing names (new compatibility surface) |
| Test impact | Extends `containmentlayoutunittest`; pins change in `sourcecontracttest` | `containmentlayoutunittest` compiles `layoutmanager.cpp` directly today (`autotests/CMakeLists.txt:735-756`) and would relink | Largest: restore, drag, parabolic and scheduled destruction tests change |
| Main risks | Lifetime of the interface pointer after containment reload; sync of notify connects | Module ownership, export rules (`private.app` resolves executable symbols), duplicate meta-objects if a static library is linked into both targets (`qobject_cast` identity; not verified). The testing guide lists private imports as a known maintenance risk (`docs/development-testing-guide.md:527-536`) | Layout timing, opacity-based hiding (an invariant in `docs/architecture-modernization-plan.md:110-124`), the 30 ms reorder delay (`containmentinterface.cpp:1271-1274`), multi-screen clones |
| Effort (relative) | S1 small; S2 medium; S3 medium; S4 small; S5 small code and medium validation; S6 large; S7 large | Large: packaging and module changes plus a retest of every path | Very large |
| Risk (relative) | S1 very low; S2 low to medium; S3 medium; S4 medium; S5 medium; S6 and S7 high | Medium to high | High |
| Verdict | Start now, one slice at a time | Fallback only if S2 shows that name-based notify connects are the main remaining risk | Not now; reconsider only with a measured reason |

Option 1 keeps the module layout, adds one interface, and lets each slice ship on its own. Option 2 gives typed signals but changes the module and packaging surface. Option 3 removes the most name-based code but moves layout timing, which the repository already treats as fragile.

## 7. Recommended first slice and the slices after it

### 7.1 S1: fail loudly on the name contracts (first, small, independently shippable)

- **Files:** `containment/plugin/layoutmanager.cpp` only in production code (`onRootItemChanged()` at `:1028-1043`; the cached-method invocations at `:1232`, `:1250`, `:1268`, `:2064`, `:2206`, `:2292`, `:2301`, `:2631`, `:2661`, where the boolean from `QMetaMethod::invoke()` is ignored). Use the existing `latteContainment` category (`latteContainment` is used at `:691`).
- **Change:** when `indexOfMethod` returns `-1` for `createAppletItem(QVariant)`, `createJustifySplitter()` or `initAppletContainer(QVariant,QVariant)`, log one `qCWarning(latteContainment)` naming the missing function. When a cached invoke returns `false`, log a warning with the function name. Normal runs must produce no new output; `containmentlayoutunittest` gains a root stub without `initAppletContainer` and asserts the warning with `QTest::ignoreMessage`.
- **Why first:** it converts the silent failure into a logged failure, does not change dispatch, and creates the seam that later slices use to prove behaviour.
- **Tests that call production code:** `autotests/containmentlayoutunittest.cpp` (existing target, `autotests/CMakeLists.txt:735-756`) with two new cases: a root without `createAppletItem`, and a root without `initAppletContainer`.
- **Gates:** zero warnings and errors on GCC and Clang Debug (`cmake --preset gcc-debug` / `clang-debug` per `docs/development-testing-guide.md:33-60`); `ctest` for `containmentlayoutunittest`, `sourcecontracttest` and `pluginregistrationunittest`; no QML change, so no `qmllint` delta.
- **Desktop retest (user, per AGENTS.md):** install the user-mode Debug build, start the dock detached, and confirm that the log has no new `latteContainment` warnings during normal use (add, remove, restart).

### 7.2 S2: typed `LayoutManagerApi` for application-side reads and calls

- **Files:** `containment/plugin/layoutmanager.h` (interface, `Q_INTERFACES`, `override`); `containment/plugin/layoutmanager.cpp` (`override` markers only); `app/view/containmentinterface.{h,cpp}` (typed member next to `m_layoutManager`; obtained in `setLayoutManager()`; cleared when the object is destroyed; about 38 string sites replaced; the notify connects at `:1029-1066` stay); `app/view/view.cpp` (10 LayoutManager sites in the drag and drop functions); `app/declarativeimports/contextmenulayerquickitem.cpp` (5 sites, obtained through a new `ContainmentInterface::layoutManagerApi()` accessor); `autotests/sourcecontracttest.cpp:5218`, which pins `m_layoutManager->property("rootItem")` and must be updated.
- **Tests that call production code:** `containmentlayoutunittest` gains (a) `qobject_cast<LayoutManagerApi *>` returns the constructed `LayoutManager`; (b) the typed getters equal the property values after `requestAppletsOrder()` and `setOption()`; (c) the cached interface pointer in `ContainmentInterface` is cleared when the object is destroyed. A test that builds `ContainmentInterface` without a view is not verified as possible; if it is not, the check moves to the desktop retest.
- **Gates:** S1 gates; the full `ctest` list on both compilers (the testing guide asks that fresh configurations list the expected tests, `docs/architecture-modernization-plan.md:136-160`); no QML change.
- **Desktop retest:** the list in 7.4.

### 7.3 S3: retire the dead legacy Justify-splitter branches (after a consumer trace)

- **Evidence:** `usesLegacyJustifySplitters()` returns `false` (`containment/plugin/layoutmanager.cpp:463-469`, with a comment that panel mode was removed). Every `useJustifySplitters` branch is therefore dead: `:1137`, `:1789`, `:1867`, `:1942`, `:2201`. `addJustifySplittersInMainLayout()` (`:2574-2689`, invokable at `layoutmanager.h:104`) has no caller in this repository. The `createJustifySplitter` function (`containment/package/contents/ui/main.qml:946-951`) is reached only from the dead branches (`layoutmanager.cpp:1250`, `:2631`, `:2661`). The QML `internalSplitterId` is set only by that function (`main.qml:948`).
- **Live probes that need a trace before removal (AGENTS rule 5):** `isJustifySplitter()` (`layoutmanager.cpp:1329-1332`, called from `:1537` and `:1559` in the live tail and head insertion), `destroyJustifySplitters()` (`:2690-2722`, called from `joinLayoutsToMainLayout()` at `:2731`), and `moveAppletsBasedOnJustifyAlignment()` (`:2767-2774`). The last one returns early through `joinLayoutsToMainLayout()` when the legacy path is off, so `AppletItem.qml:1243` stays a live call.
- **Gates:** for Left, Center, Right and Justify with both dock styles: restore, add, remove and drag produce identical `appletOrder` and `order` values (compare before and after); the `qmllint` diagnostics count for the touched QML files does not grow (`docs/qmllint-baseline.json`); a Justify layout saved by an earlier release restores the same order.
- **Desktop retest:** the list in 7.4 plus a Justify layout restore.

### 7.4 S4: typed one-shot insertion protocol

- **Change:** replace `_latte_pendingInsertionIndex` and `_latte_skipSpacerReorder` with typed methods on `LayoutManagerApi` (family 4). Consume the token once in `addAppletItem` and `save()`. Clear it after `createApplet()` returns in `app/view/view.cpp`, which fixes the hazard in family 4.
- **Tests:** `containmentlayoutunittest` cases: a token is taken once; the skip flag is consumed by one `save()`; `addAppletItem(applet, -1)` uses the token when present.
- **Desktop retest:** add a separator; drop a Widget Explorer entry at a position; drop launchers and files on the dock; repeat after a restart.

### 7.5 S5: typed QML pushes for the drag path (start only after the profile in section 9)

- **Change:** add typed `Q_PROPERTY` values with `NOTIFY` to `LayoutManager`: `viewAlignment` (int), `formFactor` (int), `viewObject` (`QObject *`, the `myView` item), and the layout spacing. Bind them in `main.qml:1202-1215`. Use them in `readConfigValue()` (`layoutmanager.cpp:471-500`) and `insertAtLayoutCoordinates()` (`:1593-1596`). Keep the configuration fallback and `alignmentFromVariant()` for unset values. The configured `alignment` is `Int` (`containment/package/contents/config/main.xml:8`), so the string branch is defensive.
- **Constraint:** the live-first order in `readConfigValue()` is deliberate; the comment at `layoutmanager.cpp:474-476` says KConfig can still hold the previous value during an alignment switch. The typed value must keep that order.
- **Tests:** `containmentlayoutunittest` setters; a `QQmlComponent` test of the bindings is possible but not verified.
- **Desktop retest:** change alignment while dragging a plasmoid and an applet.

### 7.6 Later slices (not started)

- **S6, typed layout-item base type (large, after S3):** a C++ `QQuickItem` base for layout slots with typed `isInternalViewSplitter`, `isParabolicEdgeSpacer`, `backendAppletRef` and `applet`. `AppletItem.qml` and `layouts/ParabolicEdgeSpacer.qml` derive from it. The QML root type changes, so review the 60 `qmllint` baseline entries for `AppletItem.qml` and check bindings and performance.
- **S7, creation path in C++ (large, last):** replace `createAppletItem` and `initAppletContainer` with C++ creation from a typed `QQmlComponent*`, the C++ resolver, and a C++ retry timer in place of `Qt.createQmlObject` (`main.qml:813-833`). The opacity-based visibility (`main.qml:821-826`, `:838-843`) must be kept; the repository calls it deliberate (`docs/architecture-modernization-plan.md:122-124`).
- **S8, conditional: parabolic per-event invocation.** Cache the `QMetaMethod` per item (`parabolic.cpp:72`, `:114`, `:211`) only if the profile in section 9 shows the lookups matter.

### 7.7 Desktop retest list for S2 to S4 (run by the user, per AGENTS.md)

1. Install the user-mode Debug build and launch it detached with the session environment (AGENTS.md workflow); confirm one process.
2. Add a widget from the Widget Explorer and drop one on the dock at a position; it lands at the drop index.
3. Drag an applet to reorder it across the start, main and end areas for Left, Center and Right; the spacer follows the pointer.
4. Remove an applet and use the undo; it returns at its original index. Repeat on a clone if a second screen is available.
5. Context menu: toggle "use icon theme's original colors", lock zoom, hide and show an applet.
6. Add and remove a separator.
7. Restart the dock, then the session; the order is restored and no container is duplicated.
8. Switch alignment and dock style (Classic and Modern).
9. Position shortcuts with the modifier held (exercises the shortcut host lookups in family 3).
10. Review the log for new warnings or errors and for "No such method" messages.

## 8. Compatibility constraints and comments to preserve

### 8.1 Compatibility surfaces (must not change in any slice without a separate decision)

- **QML URI and version:** `org.kde.latte.private.containment`, version `0.1` (`containment/CMakeLists.txt:18-22`); installed to the QML directory and the Qt 6 compatibility directory (`containment/CMakeLists.txt:49-62`). Imports at `containment/package/contents/ui/main.qml:19` and six other files.
- **Registered type names:** `LayoutManager` (`containment/plugin/layoutmanager.h:32`) and `types` (`containment/plugin/types.h:20-22`, `Q_GADGET`, uncreatable). `autotests/pluginregistrationunittest.cpp:60` asserts `LayoutManager` is registered under the URI.
- **Members used from QML** (`fastLayoutManager.*`, table 4.2): properties `plasmoidObj`, `rootItem`, `dndSpacerItem`, `mainLayout`, `startLayout`, `endLayout`, `metrics`, `order`, `lockedZoomApplets`, `userBlocksColorizingApplets`, `appletsInScheduledDestruction`, `hasRestoredApplets`; invokables `restore`, `save`, `joinLayoutsToMainLayout`, `addAppletItem`, `removeAppletItem`, `defaultInsertionIndex`, `resolveAppletQuickItem`, `insertBefore`, `insertAfter`, `insertAtCoordinates`, `dndSpacerIndex`, `indexToMasquearadedPoint`, `repairAppletContainers`, `appletId`, `appletIcon`, `appletIconPath`, `moveAppletsBasedOnJustifyAlignment`; signals `appletOrderChanged`, `splitterPositionChanged`, `splitterPosition2Changed` (`main.qml:1212-1214`) and `hasRestoredAppletsChanged` (`main.qml:1797`).
- **Root functions called by name from C++:** `createAppletItem(QVariant)`, `initAppletContainer(QVariant,QVariant)`, `createJustifySplitter()` (`main.qml:803`, `:929`, `:946`) and `appletContainsPos(QVariant,QVariant)` (`main.qml:1066`).
- **Object names:** `PositionShortcutsAbilityHost` (`containment/package/contents/ui/abilities/PositionShortcuts.qml:12`, searched at `containmentinterface.cpp:398`). The root's `objectName: "containmentViewLayout"` (`main.qml:30`) has no consumer in this repository.
- **Persisted configuration keys used by LayoutManager:** `appletOrder`, `alignment`, `splitterPosition`, `lockedZoomApplets`, `userBlocksColorizingApplets`, `dockStyle` (`containment/package/contents/config/main.xml`; reads at `layoutmanager.cpp:1072`, `:1136`, `:1138`, `:1432`, `:1438`, `:1469`, `:1480`), the General-group key `trashKeepOriginalColorsDefaulted` (`:869`, `:905`, not in `main.xml`), and the numeric group names in the `Applets` group (`:1009`). The option names `lockZoom` and `userBlocksColorizing` are C++-only (`layoutmanager.cpp:156-157`, `:1475-1478`; `contextmenulayerquickitem.cpp:466-470`).
- **Dynamic-property names shared across files:** `_latte_pendingInsertionIndex`, `_latte_skipSpacerReorder`, `_latte_view_object`, `_latte_*_object`, `_latte_view_interfacesobject`, `_applets_layout_geometry`, `_applets_popup_margin`. Changing any of them changes a contract that spans C++ and QML.
- **Plasma-owned names** (family 5): these cannot be changed by this project.

### 8.2 Timing and lifetime comments to preserve (quoted from the code)

These comments describe constraints that any redesign must keep. Each one names the failure it prevents.

1. `containment/plugin/layoutmanager.h:229-232`: "The containment QML assigns these items and the plasmoid; Plasma and the QML engine own them and can destroy them before this manager during a containment reload or teardown. QPointer makes late timers and signals see null instead of a dangling object."
2. `containment/plugin/layoutmanager.h:162-164`: "Const getters are not slots (clazy const-signal-or-slot), so they are Q_INVOKABLE: the meta-object still lists them, as it listed them when they were private slots, and QMetaObject::invokeMethod() still finds them by name." An interface with virtual getters changes this, so keep the Q_INVOKABLE entries for QML until QML stops using them.
3. `containment/plugin/layoutmanager.cpp:474-476`: "Prefer live alignment from the active view first, then live plasmoid config map. KConfig can still hold the previous value in the same event turn while user switches alignment in UI."
4. `containment/plugin/layoutmanager.cpp:1369-1372`: "When a separator is being created programmatically (signalled by _latte_skipSpacerReorder), skip reorderParabolicSpacers(). It moves the parabolic edge spacers which forces a second Grid recalculation within addAppletItem, causing visible icon oscillation."
5. `containment/plugin/layoutmanager.cpp:2177-2178`: "The QML Containment.onAppletAdded handler resolves to this overload (not the (x,y) overload) because the signal passes a QRectF, not (int,int)." The two `addAppletItem` overloads are resolved by argument count in QML; a typed interface must keep both signatures distinct.
6. `containment/plugin/layoutmanager.cpp:2026-2038`: "Never repair applets that are being destroyed (e.g. during the Plasma undo-notification timeout). Re-creating a UI container for a dying applet places it at the default insertion position …" and "Additionally guard against applets whose container was already removed by destroyAppletContainer() but whose Plasma::Applet is still alive (destroyed() == true)."
7. `containment/plugin/layoutmanager.cpp:287-291`: "Immediately hide the item so that the widget disappears on ALL screens when the user removes it from any one screen. Without this, a clone (secondary screen) still shows the widget at full size until the Plasma undo timeout expires, and subsequent order syncs can re-position it to the end."
8. `containment/plugin/layoutmanager.cpp:1105-1106`: "A copied dock has independent IDs and configuration. Backend applets can exist before all their Quick items do; treating that partial UI as the saved membership drops late applets and their color/zoom preferences."
9. `containment/plugin/layoutmanager.cpp:1458-1461`: "After the bounded restore retries, render what is available, but keep the persisted order while a live backend still lacks its visual item. Otherwise the next restart cannot recover that applet's original slot."
10. `containment/plugin/layoutmanager.cpp:2134-2136`: "The containment owns membership; the visual order may still be partial during startup. Keep preferences for live backends without Quick items, including after restore's retry limit, and drop genuinely removed IDs."
11. `app/view/view.cpp:1547-1550`: "This slot is called via m_plasmoidDragTimer (0ms singleshot, deferred to next event loop iteration). Deferring avoids touching QQuickItems during Wayland DragMove processing, which would break subsequent drag event delivery."
12. `app/view/containmentinterface.cpp:1212-1221`: "Set pending insertion index so that onAppletAdded (fired synchronously during createApplet) places the separator at the correct target position immediately …" and "Also suppress the reorderParabolicSpacers() call inside save() during this operation."
13. `app/view/containmentinterface.cpp:1271-1274`: "Apply the new order once after a brief delay to give the separator QQuickItem (placed by addAppletItem via _latte_pendingInsertionIndex) time to initialize, then persist to config." The delay is `QTimer::singleShot(30, …)`.
14. `app/view/containmentinterface.cpp:2079-2083`: "The layout manager lives in the containment plugin which is not linked into latte-dock-ng, so invoke it through the meta-object system. The methods are Q_INVOKABLE (see layoutmanager.h), so this call actually dispatches instead of silently failing." This is the plugin boundary; the typed interface keeps the boundary and removes the string.
15. `containment/package/contents/ui/main.qml:809-812`: "The applet's QML graphic object may not be ready yet at startup. Defer with a Timer and retry; if it still fails after a few attempts give up silently to avoid log noise." This is the existing retry and it is silent by design; S1 must not add noise to this path.
16. `containment/package/contents/ui/main.qml:821-823`: "Use opacity-based visibility instead of visible:false to avoid triggering applet internal element state resets."
17. `app/declarativeimports/contextmenulayerquickitem.h:85-88`: "The containment root item that implements appletContainsPos(). It is recreated with the containment QML while the cached QMetaMethod stays valid, so the item must be guarded independently of the method."
18. `app/view/parabolic.cpp:112-113`: "Use DirectConnection since the QML item is in the same thread — avoids event queue overhead per mouse move frame." The direct connection must stay direct after any change of call form.
19. `app/alternativeshelper.cpp:26-29`: "Plasma 6 resolves applet items through AppletQuickItem::itemForApplet() and no longer publishes the Plasma 5 '_plasma_graphicObject' property (verified on libplasma 6.8); keep the property only as a fallback."
20. `app/view/containmentinterface.cpp:2232-2234`: "In Plasma 6 the 'configuration' property is a dynamic property set by the QML engine rather than a static Q_PROPERTY — use property() directly instead of requiring indexOfProperty >= 0."

### 8.3 Source contracts and test doubles that pin or stub these names

- **Source contracts (`autotests/sourcecontracttest.cpp`):** `:5218` pins `m_layoutManager->property("rootItem")` (updated in S2); `:4248` and `:4272-4305` pin `_plasma_graphicObject` in the view, subconfig and containment sources; `:5801-5812` pins `alternativeshelper.cpp`; `:4174` pins `createAppletItem` in the QML; `:2443-2446`, `:2600`, `:2641`, `:3596`, `:3645-3685` and `:4392` read `layoutmanager.cpp` or `.h` for structure. Each must be reviewed when its code changes. Per the plan's invariants, a contract that protects real behaviour is kept, and a behavioural test replaces it where practical (`docs/architecture-modernization-plan.md:110-124`).
- **Test double in QML:** `autotests/qmlsmoketest.cpp:1028` stubs `fastLayoutManager` as a JavaScript object with `resolveAppletQuickItem`. It does not check the real `LayoutManager` API, so a QML contract change will not be caught there. S2 and S5 must update it deliberately.
- **Test double in C++:** `autotests/containmentlayoutunittest.cpp:27-33` (`RestoreRoot::createAppletItem`), and the `QQuickItem` fixtures that use dynamic `id`, `_plasma_graphicObject` and `configuration` properties (`:66-80`).

## 9. Evidence that would change the recommendation

- **A drag profile.** If a profile of a plasmoid or applet drag shows `QObject::property`, `QMetaObject::invokeMethod` by name or `QVariant` conversion among the top samples of `updatePlasmoidDrag` (`app/view/view.cpp:1545-1580`) or `insertAtLayoutCoordinates` (`layoutmanager.cpp:1587`), move S5 ahead of S2. Measure with the performance protocol (`docs/performance-validation-guide.md:102-131`): paired runs, the same scenario, Release builds for numbers.
- **A parabolic profile.** If the hover path (`app/view/parabolic.cpp:114-119`) shows name lookup in the top samples, move S8 forward.
- **A reproduced silent failure.** If renaming `createAppletItem`, `isInternalViewSplitter` or `_latte_pendingInsertionIndex` in a test build gives a wrong layout without any log, S1 is urgent and S6 gains priority.
- **Plasma facts.** Evidence that the Debian 13 floor (Plasma 6.3.6) publishes `_plasma_graphicObject` would keep the fallback; evidence that it does not would allow removing the fallback in family 5.
- **Link evidence.** If the typed interface fails to link or to load in a clean build (Debian 13 floor, Nix or Fedora), choose Option 2 or a shared static library instead.
- **Signal fragility.** If the name-based notify connects (`containmentinterface.cpp:1029-1066`) cause a lost update or a stale connection on containment reload, move to Option 2 for typed signals.
- **Dead-path usage.** If a shipped configuration or a user layout turns out to reach a `useJustifySplitters` branch, S3 is not a removal and needs a migration first. If the trace confirms none, S3 gains priority because it removes about 25 access lines and the `createJustifySplitter` contract.
- **Lifetime across a containment reload.** If the reload can destroy the `LayoutManager` while `ContainmentInterface` still holds the interface pointer, the lifetime rule in comment 1 (section 8.2) needs a test before S2 ships.

## 10. Not verified and open questions

- **No build or run.** All compile, link and runtime statements are reasoning from the code. The desktop was not used.
- **Qt behaviour (from documented API, not re-checked here):** `QObject::property()` on a missing name returns an invalid `QVariant`; `QMetaMethod::invoke()` on an invalid method returns `false` without a warning; `QMetaObject::invokeMethod()` by name prints a warning when the method is missing. S1 relies on these facts; S1's tests will confirm them.
- **QML engine behaviour:** reading and assigning dynamic properties through QML (`main.qml:766-769`) relies on the engine seeing properties created by `QObject::setProperty`; not verified. A `Binding` to a missing property produces a run-time warning (`BindingsExternal.qml`), not verified in this checkout.
- **Plasma-owned behaviour:** `processMimeData`, `Containment.itemFor`, `Plasmoid.itemFor`, the `reset` method on the applet interface (`view.cpp:953`), the `plasmoid` context property (`containmentinterface.cpp:910`) and `applet.id` are not defined in this repository; their owners and versions were not verified in Plasma sources.
- **Floor compatibility:** the Debian 13 floor (Qt 6.8.2, Plasma 6.3.6; `docs/development-testing-guide.md:33-60`) is the minimum for any new API. Debian verification is deferred by the user, so nothing here is checked against it.
- **Link-free calls through `Q_DECLARE_INTERFACE`:** reasoning only; S2's first build is the test.
- **Hot-path cost estimates:** a `QObject::property()` lookup costs on the order of a microsecond per call, so five per drag move is small; this is an estimate, not a measurement.
- **Unused include:** `app/view/containmentinterface.cpp:10`; removal needs a compile.
- **Clones:** `app/view/clonedview.cpp` and the LayoutManager instances per view were not analysed in depth.
- **Justify usage:** whether any shipped configuration reaches the legacy branches is not verified (S3's trace answers it).

## 11. Follow-ups outside R6 (not started; each would need its own decision)

- **Latte tasks applet found by name:** `app/view/containmentinterface.cpp:540-660` walks the child items of an applet, matches the plugin ID, and invokes `activateTaskAtIndex`, `newInstanceForTaskAtIndex` and `updateBadge` by name (`plasmoid/package/contents/ui/main.qml:1523-1546`). Same family as 3, in another component.
- **`BindingsExternal.qml` name-based bindings:** 34 blocks; the R4 pattern applies (`docs/architecture-modernization-plan.md:212`).
- **`_latte_*_object` handoff:** `app/view/view.cpp:494-508` and `app/declarativeimports/interfaces.cpp:224-292`; a typed handoff through the existing `Latte::Interfaces` type.
- **Window dynamic properties:** `_applets_layout_geometry` and `_applets_popup_margin` (`app/view/effects.cpp:339`, `:361`; `app/declarativeimports/core/dialog.cpp:113-119`).
- **Plasma fallbacks:** `_plasma_graphicObject` removal (family 5) needs the floor evidence in section 9.
- **Stale comments:** `app/view/view.cpp:1596-1598` and `containment/plugin/layoutmanager.cpp:1957-1958` (section 3).
