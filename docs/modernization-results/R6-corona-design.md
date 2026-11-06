# R6: Corona service-locator decomposition (design study)

Status: design only. No source, test or build file was changed, and nothing was
compiled or run. Branch `claude/r6-corona-design-26a365`, cut from `main` at
`e18a0f9a7` (v1.3.0).

Context: the [assessment](../architecture-assessment-2026-10.md) (service
locator, lines 33-36) and the R6 row of the
[R track ledger](../architecture-modernization-plan.md#assessment-refresh-r-track)
(line 214). The ledger keeps R6 deferred until one candidate has a measured
benefit (line 245). This study is the separate design review that the ledger
requires for each candidate.

How to read the counts: they come from `git grep` over tracked production
sources (autotests and docs excluded). Receivers are matched by name
(`m_corona`, `corona()`, `latteCorona`, `corona`, `c`), not by type, so expect a
few sites of error. The raw call-site list is in the shared notes folder,
outside the repository.

## Summary

1. **Corona has three jobs.** It is the composition root
   (`app/lattecorona.cpp:356-416`), the Plasma, D-Bus and QML facade
   (`app/lattecorona.h:93`, `106-112`, `139`, `187`), and a service locator
   with 13 accessors (`app/lattecorona.h:152-169`). The locator job is what
   blocks isolated tests.
2. **Most consumers need a value or one call, not the Corona.** About 110 of
   the 334 accessor call sites read a plain value, and about 30 `connect()`
   lines name a service as sender. `wm()` is already typed as the abstract
   `WindowSystem::AbstractWindowInterface`.
3. **Decompose one family at a time.** For each family, move the logic into a
   pure header first, as `app/view/viewgeometryhelpers.h` already does. Then
   hand the narrow dependency to its consumers through constructors, and remove
   the accessor when its last caller has moved. Corona ends as composition root
   and facade.
4. **First slice: the available-screen area.** Move the reduction loops of
   `availableScreenRectWithCriteria` and `availableScreenRegionWithCriteria`
   (`app/lattecorona.cpp:870-1133`) into a pure header, and add a standalone
   behavioural test in the style of `autotests/positionergeometrytest.cpp`. No
   signature, ownership, QML, D-Bus or teardown change. The reduced rect and
   region are published to plasmashell when the broadcast setting is on.
5. **Leave alone in that slice:** the `~Corona()` deletion list, the D-Bus
   adaptor, the Plasma virtual overrides, the `_latte_*` QML property names and
   `qmlRegisterTypes()`. The dead surface found on the way (`plasmaScreenPool()`,
   `Corona::unload()`, and a cache flag that is never set) gets its own small
   slices.

## 1. Consumer matrix

### 1.1 The 13 accessors

Sites are `git grep` counts, with the number of distinct files in brackets.
"Need" says whether the caller needs the service object or a value derived
from it.

| Accessor (declared in `lattecorona.h`) | Sites [files] | Top consumers (sites) | Second hop used (count) | Need |
| --- | --- | --- | --- | --- |
| `layoutsManager()` (159) | 87 [19] | `layoutscontroller.cpp` 21, `genericlayout.cpp` 12, `globalshortcuts.cpp` 9, `layoutsmodel.cpp` 8, `visibilitymanager.cpp` 7 | `synchronizer()` 42, `memoryUsage()` 22, `importer()` 4, `currentLayoutsNames()` 3, `switchToLayout()` 3, `moveView()` 3 | Service through `synchronizer()`; value through `memoryUsage()` |
| `wm()` (167) | 55 [11] | `positioner.cpp` 11, `subconfigview.cpp` 10, `infoview.cpp` 10, `subwindow.cpp` 7, `primaryconfigview.cpp` 6, `view.cpp` 5 | `setViewExtraFlags` 10, `unregisterIgnoredWindow` 8, `winIdFor` 5, `setWindowOnActivities` 5, `slideWindow` 4, `schemesTracker()` 4, `registerIgnoredWindow` 4; 6 `connect()` senders | Service; the type is `WindowSystem::AbstractWindowInterface` |
| `universalSettings()` (157) | 55 [14] | `tabpreferenceshandler.cpp` 18, `manager.cpp` 7, `screengeometries.cpp` 6, `synchronizer.cpp` 5, `primaryconfigview.cpp` 3, `positioner.cpp` 3, `layoutsmodel.cpp` 3 | 24 getters, 16 setters, `syncSettings()` 2, about 7 `connect()` senders | Value; the setters come from the settings UI |
| `screenPool()` (156) | 54 [13] | `genericlayout.cpp` 22, `positioner.cpp` 7, `viewsmodel.cpp` 4, `screengeometries.cpp` 4, `storage.cpp` 3 | `primaryScreen()` 12, `primaryScreenId()` 10, `hasScreenId()` 5, `isScreenActive()` 5, `id()` 4, `connector()` 4, `screenForId()` 4, `secondaryScreenIds()` 3 | Value: screen ids, names and the primary screen |
| `activitiesConsumer()` (154) | 24 [9] | `synchronizer.cpp` 8, `layoutsmodel.cpp` 4, `genericlayout.cpp` 4, `view.cpp` 3 | `currentActivity()` 5, `activities()` 4, about 15 `connect()` senders (`currentActivityChanged`, `activitiesChanged`, `activityAdded`, `activityRemoved`) | Value, plus a signal source |
| `templatesManager()` (160) | 19 [6] | `manager.cpp` 6, `exporttemplatehandler.cpp` 4, `tablayoutshandler.cpp` 3, `viewshandler.cpp` 2, `synchronizer.cpp` 2 | `newLayout()` 3, `viewTemplates()` 3, `proposedTemplateAbsolutePath()` 3, `importSystemLayouts()` 2, others 1 each | Service |
| `indicatorFactory()` (162) | 18 [3] | `indicator.cpp` 12, `indicatoruimanager.cpp` 4, `view.cpp` 2 | `isCustomType()` 4, `metadata()` 3, plugin-list queries 5, actions 3 (`removeIndicator`, `importIndicatorFile`, `downloadIndicator`) | Service: plugin catalogue and actions |
| `viewSettingsFactory()` (158) | 6 [1] | `view.cpp` 6 | `primaryConfigView()` 2, `widgetExplorerView()` 1, `lastContainment()`, `hasVisibleSettings()`, `hasOrphanSettings()` | Service; only `View` uses it |
| `dialogShadows()` (169) | 5 [4] | `subconfigview.cpp` 2, `widgetexplorerview.cpp`, `primaryconfigview.cpp`, `canvasconfigview.cpp` | `addWindow()` 4, `removeWindow()` 1 | Service |
| `themeExtended()` (165) | 4 [3] | `effects.cpp` 2, `view.cpp` 1, `subconfigview.cpp` 1 | `cornersMask()` 2; published to QML as `_latte_themeExtended_object` (`view.cpp:496`) and as a root input (`subconfigview.cpp:143`) | Value (`QRegion`), QML-exposed |
| `waylandCoronaInterface()` (152) | 5 [5] | `infoview.cpp`, `subwindow.cpp`, `subconfigview.cpp`, `view.cpp`, `wm/waylandinterface.cpp` (1 each) | the `PlasmaShell` pointer itself | Service; `WaylandInterface` forwards it back to Corona (`app/wm/waylandinterface.cpp:419-421`) |
| `globalShortcuts()` (155) | 2 [2] | `view.cpp` 1, `genericlayout.cpp` 1 | `shortcutsTracker()`, published to QML (`view.cpp:494`) | Object for QML |
| `plasmaScreenPool()` (164) | 0 | none | none | None. Created at `lattecorona.cpp:371`, deleted at `:451`, never read through Corona. Dead accessor. |

Totals: 334 accessor call sites in 37 distinct files.

Value or service: about 110 sites read a plain value. They are the
`universalSettings()` getters (24), `screenPool()` values and lookups (47),
`memoryUsage()` (22), the activity queries (9), `cornersMask()` (2),
`shortcutsTracker()` (2) and the view-settings queries (3). About 30
`connect()` lines use a service as sender. The rest call service methods.

### 1.2 Other Corona members that consumers use

| Member (`lattecorona.h`) | Kind | Sites |
| --- | --- | --- |
| `inQuit()` (103) | value: quit state | 5: `synchronizer.cpp:531`, `:538`, `:1044`, `:1051` (through `m_manager->corona()`), `view.cpp:329` |
| `syncContainmentScreen()` (144) | action | 3: `genericlayout.cpp:1544`, `positioner.cpp:364`, `view.cpp:273` |
| `availableScreenRectWithCriteria()` (118) | value: geometry | 3: `screengeometries.cpp:234`, `primaryconfigview.cpp:294`, `widgetexplorerview.cpp:176` |
| `availableScreenRegionWithCriteria()` (125) | value: geometry | 2: `screengeometries.cpp:241`, `positioner.cpp:562` |
| `quitApplication()` (176), `importFullConfiguration()` (173) | lifecycle | 1 each: `settingsdialog.cpp:151`, `:319` |
| `aboutApplication()` (185) | UI action | 1: `manager.cpp:440` |
| `createContainmentForImport()` (149) | action, libplasma 6.8 branch | 1: `storage.cpp:678` |
| `containmentsIds()`, `appletsIds()` (242-243) | value; private, reached through `friend` | 3: `storage.cpp:368-369`, `:696` |
| Plasma base API through the pointer (`Plasma::Corona`, not declared in `lattecorona.h`): `kPackage()` 14, `containments()` 9, `config()` 5, `setImmutability()` 2, `requestConfigSync()` 1, `immutability()` 1, `loadLayout()` 1 | inherited | for example `manager.cpp:306`, `importedcontainments.cpp:42`, `storage.cpp:245`, `:794` |

Corona's own signals (`lattecorona.h:212-218`) that consumers use:
`availableScreenRectChangedFrom` (about 6 lines), `viewLocationChanged` (3),
`availableScreenRegionChangedFrom` (3), `verticalUnityViewHasFocus` (2) and
`raiseViewsTemporaryChanged` (1). The Plasma base signal `containmentAdded` has
one use, `genericlayout.cpp:958`.

### 1.3 How a consumer obtains the Corona pointer

26 classes store a Corona pointer as a member: 18 raw (`Latte::Corona *m_corona`)
and 8 `QPointer<Latte::Corona>`. The guard follows the owner, not a shared
policy. The view family uses `QPointer` (`app/view/view.h:359`,
`effects.h:195`, `positioner.h:198`, `containmentinterface.h:217`,
`indicator/indicator.h:159`, `helpers/subwindow.h:105`), and so do
`settings/universalsettings.h:195` and `shortcuts/globalshortcuts.h:91`.

There are four ways a class gets the pointer:

| Mechanism | Examples (file:line) | Contract that is not in the type |
| --- | --- | --- |
| QObject parent, then `qobject_cast` (20 `qobject_cast` sites in total, across all four mechanisms) | `Layouts::Manager` (`app/layouts/manager.cpp:47`), `UniversalSettings` (`app/settings/universalsettings.cpp:39`), `GlobalShortcuts` (`app/shortcuts/globalshortcuts.cpp:48`), `PlasmaExtended::Theme` (`app/plasma/extended/theme.cpp:50`), `AbstractWindowInterface` (`app/wm/abstractwindowinterface.cpp:44`), `WaylandInterface` (`app/wm/waylandinterface.cpp:301`) | Silently null when the parent is not a Corona. `GlobalShortcuts` skips `init()` (`globalshortcuts.cpp:52-53`). `Layouts::Manager::memoryUsage()` dereferences without a check (`manager.cpp:155-158`). `Synchronizer` dereferences in its constructor (`synchronizer.cpp:50-60`). |
| Through a view or a dialog | `View` from its `Plasma::Corona *` argument (`view.cpp:139`); `Positioner` (`positioner.cpp:80`), `VisibilityManager` (`visibilitymanager.cpp:56`), `Effects` (`effects.cpp:32`), `Indicator` (`indicator.cpp:43`), `SubWindow` (`subwindow.cpp:30`), `ContainmentInterface` (`containmentinterface.cpp:344`), `SubConfigView` (`subconfigview.cpp:62`). Dialogs copy `parent->corona()`: `tabpreferenceshandler.cpp:28`, `tablayoutshandler.cpp:52`, `screensdialog.cpp:26`, `exporttemplatedialog.cpp:33`. | The Plasma base type is narrowed by a cast in each class. |
| Constructor injection | `InfoView` (`infoview.cpp:40`), `SettingsDialog` (`settingsdialog.cpp:50`), layouts model (`layoutsmodel.cpp:33`), views model (`viewsmodel.cpp:25`), `TemplatesKeeper` (`templateskeeper.cpp:21`), `Templates::Manager` (`templatesmanager.cpp:31`), `ScreenGeometries` (`screengeometries.cpp:37`) | The preferred shape. No cast. |
| Setter after construction | `GenericLayout::setCorona()` (`genericlayout.cpp:167-169`), called from `synchronizer.cpp:670` and `:981` | Two-phase: "this function needs the layout to have first set the corona" (`genericlayout.h:113`). |

`OriginalView` and `ClonedView` take a `Plasma::Corona *` and pass it to
`View` (`originalview.cpp:17-18`, `clonedview.cpp:40-41`). `ClonedView` never
uses it.

### 1.4 Corona used only as a QObject parent, or only for signals

Parent only: the child's own source never names Corona.

| Child | Created at | Child source (no Corona type) |
| --- | --- | --- |
| `Latte::ScreenPool` | `lattecorona.cpp:363` | `app/screenpool.cpp:26`; one comment at `:69` |
| `PlasmaExtended::ScreenPool` | `:371` | `app/plasma/extended/screenpool.cpp:29` |
| `ViewSettingsFactory` | `:365` | `app/view/settings/viewsettingsfactory.cpp:18` |
| `Indicator::Factory` | `:367` | `app/indicator/factory.cpp:64` |
| `PanelShadows` | `:373` | `app/view/panelshadows.cpp:50` |
| `KActivities::Consumer` | `:362` | external class |
| KWayland `Registry`, `PlasmaShell`, `PlasmaWindowManagement`, `PlasmaVirtualDesktopManagement` | `:632`, `:637`, `:642`, `:654` | external class; parent is `this` |
| `KActionCollection` (two), the shortcut owners | `globalshortcuts.cpp:77`, `:107` | external class; parent is `m_corona` |

Signal only: none found in the consumer files read. Every consumer of a Corona
signal also calls a service: `genericlayout.cpp:958`,
`screengeometries.cpp:107-108`, `positioner.cpp:94`, `primaryconfigview.cpp:63-64`,
`view.cpp:280`, `:451-452`, `:549-550`, `:1423-1424`.

The signals form a hub. Views emit `availableScreenRectChangedFrom` and
`availableScreenRegionChangedFrom` into Corona (`view.cpp:449-450`). Corona
clears its cache flag and re-emits to every view (`lattecorona.cpp:536-538`).

## 2. Construction and destruction order today

### 2.1 Construction

Members are initialised in declaration order (`app/lattecorona.h:250-297`).
The initializer list (`app/lattecorona.cpp:357-373`) follows the same relative
order, so `-Wreorder` stays quiet.

| Step | What | Where | Dependency |
| --- | --- | --- | --- |
| 1 | `Plasma::Corona(parent)` | `cpp:357` | none |
| 2 | `m_activitiesConsumer` | `:362` | none |
| 3 | `m_screenPool` (`Latte::ScreenPool`) | `:363` | none |
| 4 | `m_universalSettings` | `:364` | parent cast (`universalsettings.cpp:39`) |
| 5 | `m_viewSettingsFactory` (QPointer) | `:365` | none |
| 6 | `m_globalShortcuts` | `:366` | `init()` runs only if the parent is a Corona (`globalshortcuts.cpp:52-53`) |
| 7 | `m_indicatorFactory` | `:367` | none |
| 8 | `m_layoutsManager` | `:368` | creates `Synchronizer`, which needs `m_manager->corona()->activitiesConsumer()` in its constructor (`manager.cpp:42-55`, `synchronizer.cpp:41-50`) |
| 9 | `m_templatesManager` | `:369` | stores the Corona (`templatesmanager.cpp:29-31`) |
| 10 | `m_plasmaGeometries` (`ScreenGeometries`) | `:370` | starts a 2.5 s init timer and a D-Bus service watcher (`screengeometries.cpp:35-42`) |
| 11 | `m_plasmaScreenPool` | `:371` | none |
| 12 | `m_themeExtended` | `:372` | parent cast (`theme.cpp:50`) |
| 13 | `m_dialogShadows` | `:373` | none |

Then the constructor body (`cpp:375-415`):

| Step | What | Where |
| --- | --- | --- |
| a | `connect(qApp, aboutToQuit, onAboutToQuit)` | `:375` |
| b | `m_wm = new WaylandInterface(this)`. Not in the initializer list. | `:378` |
| c | `setupWaylandIntegration()`: creates the `Registry`, sets `m_waylandCorona`, calls `initWindowManagement()`, then a synchronous `roundtrip()` | `:380`, `:622-666` (roundtrip at `:665`) |
| d | `KPackage::Package package(new Latte::Package(this))` | `:382` |
| e | `m_screenPool->load()` | `:384` |
| f | **Early return** when the package is invalid. Steps g to k are skipped. | `:386-389` |
| g | `setKPackage()`, then `m_universalSettings->load()` and `m_themeExtended->load()`. The comment says they "must be loaded after the package has been set". | `:395-398` |
| h | `qmlRegisterTypes()` | `:400`, `:1688-1705` |
| i | `serviceStatusChanged` connected to `load()`, then `load()` | `:402-403` |
| j | screen-sync timer | `:405-410` |
| k | `new LatteDockAdaptor(this)`; `registerObject("/Latte", this)` | `:413-415` |

Step f means a half-built Corona can exist and is then destroyed normally. Not
verified at runtime.

### 2.2 `load()` (`lattecorona.cpp:520-599`)

`load()` runs once, guarded by `m_activitiesStarting` (`:522-527`). Inside, the
order is: `m_templatesManager->init()` (`:533`) before `m_layoutsManager->init()`
(`:534`); the templates init connects to the layouts manager
(`templatesmanager.cpp:43-45`). Then the cache-invalidation lambdas (`:536-538`),
the command-line memory override (`:544-547`), default-layout creation
(`:549-575`), `loadLayoutOnStartup()` (`:577`), `onScreenAdded()` for each screen
(`:581-583`), and the optional `addView` after `initializationFinished`
(`:585-592`).

### 2.3 Quit paths

- **Normal quit** (menu, settings window, D-Bus `quitApplication`).
  `quitApplication()` sets `m_inQuit`, hides the views after 300 ms with `this`
  as context, and quits the application after 800 ms with the application as
  context (`lattecorona.cpp:1196-1212`).
- **`aboutToQuit` leads to `onAboutToQuit()`** (`:469-518`). It bounds the
  ksmserver query (`:476-480`; default timeout 1 s, `app/session/shutdownmonitor.h:25`).
  On the normal path it hides the views (`:499`), calls
  `m_viewSettingsFactory->deleteLater()` (`:504`), stops the sync timer
  (`:506`), runs `cleanConfig()` in single-layout mode (`:508-510`), pauses the
  import in multiple-layout mode (`:512-514`), and unloads the layouts (`:517`).
- **Session-ending path** (`LATTE_SESSION_ENDING`, the `latte_session_ending`
  property, or ksmserver shutting down). It hides the views and returns
  (`:482-495`). It does not unload the layouts; `~Corona()` does (`:422-424`).
- **`exec()` returns.** Qt flushes the factory's `deleteLater()` (`lattecorona.h:278-281`; R1b).
- **After `exec()`** (`app/main.cpp:649-720`). The block that holds the Corona
  exits and destroys, in order, the `KDBusService`, the `ShutdownMonitor` and the
  `Corona`. The monitor is declared after the Corona, so it dies first
  (`main.cpp:652-656`). Then five passes of `DeferredDelete` (`:707-712`), and
  the shared QML engine is reset (`:718`).

### 2.4 `~Corona()` (`lattecorona.cpp:418-467`)

1. `m_layoutsManager->unload()` (`:423`) calls `Synchronizer::unloadLayouts()`
   (`manager.cpp:130-133`). That unloads every central layout, and each one
   unloads its views and containments before it is deleted
   (`app/layouts/synchronizer.cpp:524-541`, `:599-611`).
2. Detach every child with `setParent(nullptr)` (`:430-434`), so Qt's
   `deleteChildren()` finds nothing.
3. Delete explicitly, in this order (`:442-453`), each through
   `Session::deleteNow()` (`app/session/teardown.h:25-33`): `layoutsManager`,
   `templatesManager`, `viewSettingsFactory` (QPointer, skipped when null),
   `indicatorFactory`, `globalShortcuts`, `dialogShadows`, `wm`, `plasmaGeometries`,
   `themeExtended`, `plasmaScreenPool`, `screenPool`, `universalSettings`.
4. Disconnect and `delete m_activitiesConsumer` (`:455-456`), then flush deferred
   deletes (`:458`).
5. If an import is pending, start a detached `--import-full` process
   (`:462-466`).

Order constraints visible in the code:

- `Layouts::Manager::~Manager()` calls `memoryUsage()` (`manager.cpp:59`), which
  reads `universalSettings()` through Corona (`:157`). Universal settings must
  outlive the manager, and it is deleted last (`lattecorona.cpp:453`).
- `View::disconnectSensitiveSignals()` disconnects from Corona's signals
  (`view.cpp:543-553`). Views must be gone before the hub is destroyed, and
  step 1 ensures that.

### 2.5 Children that `~Corona()` does not delete

Step 2 detaches all children. An object that step 3 and step 4 do not name
survives `~Corona()`:

- KWayland `Registry` (`lattecorona.cpp:632`), `PlasmaShell` (`:637`, held in
  `m_waylandCorona`), `PlasmaWindowManagement` (`:642`) and
  `PlasmaVirtualDesktopManagement` (`:654`).
- `LatteDockAdaptor` (`:413`).
- The two `KActionCollection` objects that own the global shortcuts
  (`globalshortcuts.cpp:77`, `:107`). `~GlobalShortcuts()` deletes only its two
  trackers (`:64-72`).
- `QmlObjectSharedEngine` objects created with `this` as parent
  (`lattecorona.cpp:1312`). They leave the list only when destroyed
  (`:1325-1327`).

This comes from reading the code, not from a runtime check. The process exits
soon afterwards. Whether the detached `--import-full` relaunch (`:462-466`)
starts while these shortcuts are still registered is not verified.

### 2.6 Comments and invariants a decomposition must keep

Quoted from the code, because a redesign must not break them:

- `app/lattecorona.cpp:420-421`: "Tear down containments first — same pattern as
  Plasma's CoronaPrivate::~CoronaPrivate (takeAt before delete)."
- `app/lattecorona.cpp:426-429`: "Detach all children from Corona so Qt's
  deleteChildren() ... finds an empty list. ... LayoutsManager needs dependencies
  that were registered BEFORE it — they'd be deleted first and crash."
- `app/lattecorona.cpp:436-441`: "Delete in dependency order using deleteLater() +
  immediate flush ... Session::deleteNow() skips a service that a normal quit
  already deleted (the view settings factory, see onAboutToQuit())."
- `app/lattecorona.h:278-281`: "Guarded because a normal quit deletes the factory
  before ~Corona() ... The other services are deleted only in ~Corona() and stay
  valid until then."
- `app/lattecorona.cpp:500-503`: "... ~Corona() must not touch the freed factory
  again (the quit used to hang there)."
- `app/lattecorona.cpp:476-478`: "Bounded query: during a session teardown
  ksmserver can stop answering, and the 25 s default D-Bus timeout kept the dock
  mapped long enough to block KWin's closeWaylandWindows step."
- `app/lattecorona.cpp:485-489`: "... Proactively hide all views to decouple window
  tracking before the compositor goes away. This prevents the \"unexpectedly
  stopped\" notification from Plasma."
- `app/lattecorona.cpp:1208-1209`: "The application is the context: the quit is
  process-wide and must not be cancelled when the Corona is destroyed before the
  timer fires."
- `app/lattecorona.cpp:1449`: "Give the compositor enough time to update the
  active window id." (the 200 ms timer is at `:1448`)
- `app/main.cpp:652-656`: "... it is declared after the Corona so it is destroyed
  first and its hooks never outlive the views."
- `app/layouts/manager.cpp:48`: "needs to be created AFTER corona assignment"
- `app/layout/genericlayout.h:113`: "this function needs the layout to have first
  set the corona through setCorona() function"
- `app/lattecorona.cpp:1322-1324` and `:1330-1335`: the alternatives dialogs
  belong to their view. "Bound to Corona, it ran on a destroyed view or
  dereferenced a null one."

Invariants that any slice must keep:

1. `m_viewSettingsFactory` stays a QPointer, and `deleteNow()` keeps skipping a
   null service (`lattecorona.h:278`, `teardown.h:25-33`).
2. The `deleteNow` list keeps its order: `layoutsManager` first and
   `universalSettings` last (`lattecorona.cpp:442-453`).
3. Views are unloaded before services are deleted, and the session-ending path
   unloads in the destructor (`:422-424`, `:490-495`).
4. The monitor is created after, and destroyed before, the Corona
   (`main.cpp:649-657`).
5. `setupWaylandIntegration()` completes its roundtrip before the package loads
   (`:380`, `:665`).
6. `m_universalSettings->load()` runs after `setKPackage()` (`:395-397`).
7. `qmlRegisterTypes()` runs before `load()` (`:400`, `:403`).
8. `load()` initialises templates before layouts (`:533-534`).
9. Corona stays the object exported at `/Latte` (`:415`) and the parent of the
   adaptor (`:413`).
10. All Corona objects stay on the main thread. The assessment found no worker
    threads (`docs/architecture-assessment-2026-10.md:81-83`).

## 3. Options

| Option | What changes | Effort | Risk | Test gain | Verdict |
| --- | --- | --- | --- | --- | --- |
| 0. Do nothing; extract pure helpers only when a change touches the code | nothing | none | none | none beyond ad hoc | Baseline. The 279 two-hop chains stay. |
| 1. `Services` struct passed by reference | One struct of pointers, owned by `main()` or Corona, handed to services and views. The 334 accessor sites become field reads. | medium, mechanical | low to medium. Its lifetime must cover the monitor hooks, which capture `&corona` (`main.cpp:669-682`). It still exposes every service to every consumer. | none by itself | Transitional only |
| 2. Narrow interface per family, injected through constructors | Each family gets the interface it needs (screen lookup, memory mode, window commands through `AbstractWindowInterface`, and so on). The accessor goes when its last caller moves. | high in total: 26 storing classes, 11 `Latte::Corona *` parameters in 9 headers, 7 `Plasma::Corona *` parameters | medium. Construction order (two-phase `setCorona`; `Synchronizer` needs the Corona when it is built), QML publication of the `_latte_*` objects, `friend` declarations, signal senders | high per family, because consumers can be built with fakes | Target for consumers, one family per slice |
| 3. Corona as composition root and facade only | Corona creates and wires the services and keeps the Plasma virtuals, D-Bus slots, QML registration and lifecycle. It exposes no accessor for other classes. | high, after option 2 has covered every family | medium to high in teardown: §2.4 and §2.5 must become explicit owners; the adaptor's parent class is set in `app/CMakeLists.txt:47` | high for the services; Corona itself stays a thin facade | End state |
| 4. Pure logic first, per family | A header of pure functions next to the family, plus one standalone test target. The signatures do not change. | low per family | low; the code is copied verbatim | real behavioural tests now | First step of options 2 and 3 |

Recommended path: option 4 for each family, then option 2 for that family,
converging on option 3. Option 1 is useful only to move the lambdas in
`main()` without changing consumers.

## 4. Recommended first slice: available-screen area

### 4.1 Why this family

- **It is visible on the desktop.** The arithmetic decides how much of each
  screen is free. `ScreenGeometries` publishes the result to plasmashell:
  `org.kde.PlasmaShell.StrutManager` `setAvailableScreenRect` and
  `setAvailableScreenRegion` (`app/plasma/extended/screengeometries.cpp:26-28`,
  `:146-192`, `:214-260`). Two settings views and the positioner read it too
  (`primaryconfigview.cpp:294`, `widgetexplorerview.cpp:176`, `positioner.cpp:562`).
- **No test exercises it.** The only checks are text. `lattecoronaScreenForContainmentHasDepthGuard`
  covers the depth guard (`autotests/sourcecontracttest.cpp:4882-4890`). The
  only other mention is a comment in a contract (`sourcecontracttest.cpp:1483`).
- **The inputs are pure.** The reduction needs two geometries, plain values from
  each view, and two filter lists.
- **There is a precedent.** `app/view/viewgeometryhelpers.h` already holds tested
  pure helpers for the same kind of question, such as
  `verticalDockExternalPanelGeometry` (`:118-166`), tested by
  `autotests/positionergeometrytest.cpp`.
- **It is behaviour-neutral by construction.** No consumer and no Corona
  signature changes, and the arithmetic is copied verbatim.

### 4.2 Scope

In: the two `WithCriteria` reductions, their snapshot type, the test and the
CMake stanza.

Out: `availableScreenRect()` and `availableScreenRegion()`, the Plasma overrides
(`lattecorona.cpp:856`, `:1029`), which stay as thin callers; the cache members
and their branches (§5, item 9); the signal hub; every other accessor. The
adapter keeps the reads of `m_screenPool`, `m_layoutsManager` and
`m_activitiesConsumer`.

### 4.3 Files

- **New** `app/view/screenareahelpers.h`: header-only, `inline` functions, with
  the license header and includes that `viewgeometryhelpers.h` uses. It includes
  the generated `coretypes.h` (generated from `declarativeimports/coretypes.h.in`,
  `CMakeLists.txt:320`), `QList`, `QRect`, `QRegion` and `Plasma/Plasma`. The
  namespace is `Latte::ViewPart`, as in `viewgeometryhelpers.h`, or a new
  `Latte::ScreenArea`; the implementer chooses.
- **Changed** `app/lattecorona.cpp`: the bodies of `availableScreenRegionWithCriteria`
  (`:870-1027`) and `availableScreenRectWithCriteria` (`:1042-1133`) become
  adapters. The cache lines stay as they are.
- **Unchanged** `app/lattecorona.h`. No signature changes.
- **New** `autotests/screenareatest.cpp`.
- **Changed** `autotests/CMakeLists.txt`: a stanza modelled on
  `positionergeometrytest` (`:927-931`). It sets `screenareatest_SRCS`, calls
  `add_executable(... EXCLUDE_FROM_ALL ...)`, adds the include directories
  `${CMAKE_BINARY_DIR}/app` and `${CMAKE_CURRENT_SOURCE_DIR}/../app`, links
  `Qt6::Core Qt6::Gui Qt6::Test Plasma::Plasma`, and calls
  `latte_add_test(screenareatest)`. It is not the offscreen variant, because the
  test needs no `QGuiApplication`.

### 4.4 Steps

1. Add a snapshot struct: a plain value type with one entry per view. Each field
   replaces one Corona expression:
   - `onScreen`: `view->screen() == screen` (`lattecorona.cpp:922`, `:1094`).
   - `hasContainment`: `view->containment()` (`:922`, `:1094`).
   - `offScreenStartup`: `desktopUse && view->positioner() && view->positioner()->isOffScreen()`
     (`:920`, `:1092`).
   - `hasVisibility` and `mode`: `view->visibility()->mode()` (`:924`, `:1096`).
   - `location`: `view->location()` (`:923`, `:982`, `:1104`).
   - `formFactor`: `view->formFactor()` (`:929`).
   - `alignment` (`int alignment()`, `app/view/view.h:188`), `offset`
     (`float offset()`, `view.h:194`), `maxLength` (`float maxLength()`, `view.h:191`),
     `normalThickness` (`int`, `view.h:176`), `geometry`, `x`, `y`, `width`, `height`.
   - Keep these types exactly. `w = view->maxLength() * view->width()` converts a
     float to an int (`:931`; the vertical case is `:953`), and a test locks that
     truncation.
2. Add `reduceRect()` and `reduceRegion()` as pure functions. Copy the arithmetic
   verbatim: the rect loop is `:1091-1126`, and the region loop is `:919-1012`.
   Keep the `+ 1` terms at `:941`, `:945`, `:963`, `:990` and `:1002`, which follow
   QRect's inclusive edges.
3. Keep the `ignoreModes` augmentation (`:1080-1087` for rects, `:908-915` for
   regions) and the `allEdges` rule (`:1089`, `:917`). They can stay in the adapter
   or move into a criteria struct. The behaviour must match either way.
4. Make each adapter read every `view->` getter once, in the order it is read today.
5. Write `autotests/screenareatest.cpp` (§4.5) and the CMake stanza.
6. Review the equivalence field by field, as R3 did
   ([R3 report](R3.md)).

### 4.5 Tests

These are pure-function tests. The expected values come from the current
formulas. The author must recompute each one from the code, because no Corona
instance can be built to check them.

Rect reduction, `reduceRect()`:

1. `emptyViewListKeepsAvailableRect`: an empty list returns the input.
2. `topDockRaisesTopEdge`: a TopEdge view leaves `top() == max(input.top(), view.y + thickness)` (`:1106`).
3. `bottomDockLowersBottomEdge`: a BottomEdge view leaves `bottom() == min(input.bottom(), view.y + view.height - thickness)` (`:1110`).
4. `leftAndRightDocksMoveTheirEdges`: `:1114` and `:1118`.
5. `defaultPlasmaIgnoreListSkipsAutoHideAndSidebars`: with the Plasma list
   (`:1033-1035`), an AutoHide view changes nothing. With an empty list it counts.
6. `noneAndNormalWindowModesAreAlwaysIgnored`: `None` (-1) and `NormalWindow` (20)
   never reduce, even with an empty list (`:1081-1087`; enum values in
   `declarativeimports/coretypes.h.in:32-46`).
7. `ignoredEdgeLeavesThatEdgeAlone`: an `ignoreEdges` list that contains TopEdge
   leaves `top()` unchanged (`:1095`).
8. `viewOnAnotherScreenIsIgnored`: `onScreen == false` (`:1094`).
9. `offScreenStartupViewIgnoredOnlyForDesktopUse`: ignored when `desktopUse` is
   true, counted when it is false (`:1092`).
10. `viewWithoutContainmentOrVisibilityIsIgnored` (`:1094-1096`).

Region reduction, `reduceRegion()`:

11. `topDockSubtractsStripAtViewY`: a left-aligned top dock on a 1920x1080 screen,
    with maxLength 1.0, offset 0 and thickness 48, leaves
    `boundingRect().top() == 48` (`:985`, `:1002`).
12. `justifiedDockUsesCenterFormula`: Justify computes
    `x = geometry().center().x() - w / 2 + 1 + offset` (`:941`). Derive the expected
    x by hand, because `QRect::center()` is `(left + right) / 2`, which is not
    `left + width / 2`.
13. `zeroLengthDockSubtractsNothing`: `maxLength == 0` leaves the region unchanged (`:985`).
14. `floatingFormFactorIsSkipped`: the `default: continue` branch (`:974-975`).
15. `maxLengthTruncatesToWholePixels`: width 1000 and maxLength 0.5005 give `w == 500` (`:931`).

Not unit-tested: the adapter, the cache, and the `ignoreExternalPanels` choice of
base geometry (`:1066`). The desktop check in §4.6 covers them.

### 4.6 Acceptance gates

1. **Scope.** `git diff --stat` shows only the files in §4.3. No signature changes
   in `app/lattecorona.h`.
2. **Equivalence.** A field-by-field review of the two reductions, with the int and
   float conversions and the `+ 1` terms unchanged.
3. **Builds.** GCC and Clang Debug with zero warnings and zero errors (AGENTS rule 4).
4. **Tests.** `ctest --output-on-failure` in `build/`, including the new
   `screenareatest` and `sourcecontracttest` (AGENTS rule 6). The source contracts
   name no function that this slice moves; confirm with grep.
5. **Desktop A/B** (AGENTS runtime retest workflow). Use the same layout and screens,
   with `isAvailableGeometryBroadcastedToPlasma` enabled (`screengeometries.cpp:216`).
   Compare the `AVAILABLE RECT` debug line and the `StrutManager` calls (`:254-260`)
   before and after; they must match. Check top, bottom, left and right docks, and an
   auto-hide dock. Check the debug log for new warnings and errors.
6. **Deferred.** Debian 13 is outside this slice, as the user asked.

## 5. Risks

| # | Risk | Where | Handling |
| --- | --- | --- | --- |
| 1 | QML reads Corona-owned objects by name | `view.cpp:494-498` sets `_latte_globalShortcuts_object`, `_latte_layoutsManager_object`, `_latte_themeExtended_object` and `_latte_universalSettings_object`; `declarativeimports/interfaces.cpp:228-240` and `:289-292` read them; `subconfigview.cpp:141-143` publishes root inputs | Swapping a service changes what QML sees, with no compile error, and qmllint cannot see it (`docs/architecture-assessment-2026-10.md:37-40`). Keep the names and the object identity. |
| 2 | D-Bus adaptor and exported object | `lattecorona.h:93`; `app/CMakeLists.txt:46-47` generates the adaptor for `Latte::Corona`; `lattecorona.cpp:413-415`; 21 methods in `app/dbus/org.kde.LatteDock.xml`; in-app caller `app/launcherhelper.cpp:42` | Moving a method changes the adaptor's parent class and the exported object. Check `busctl introspect org.kde.lattedock /Latte` before and after any such slice. Not run here. |
| 3 | `friend` declarations | `lattecorona.h:299-301` | `Layouts::Storage` needs the private `containmentsIds()` and `appletsIds()` (`storage.cpp:368-369`, `:696`). `GlobalShortcuts` and `Layouts::Manager` show no private access by grep. Removing a friend is a compile test. |
| 4 | Qt parent ownership is not the teardown contract | §2.5 | An object parented to Corona survives `~Corona()` unless `deleteNow` lists it. Every new owner must be explicit. `docs/architecture-overview.md:72-73` says the same. |
| 5 | Plasma base virtuals | `lattecorona.h:106-112`, `:139`, `:187`; installed `/usr/include/Plasma/plasma/corona.h:149`, `:157`, `:167`, `:180`, `:240`, `:400` | They must stay overridden on Corona or forward to a helper. The installed header's version was not compared with the 6.3 floor. Whether libplasma calls the availability virtuals internally is not verified. |
| 6 | libplasma version branches | `LATTE_PLASMA_STORED_CONTAINMENT_SCREEN`: 20 uses in 8 files (grep) | The pre-6.8 branch calls `importLayout()` (`storage.cpp:750`), which libplasma 6.8 removed (`storage.cpp:644-646`). Both branches must compile, and this sandbox builds neither. |
| 7 | Host-symbol QML plugin | `app/declarativeimports/CMakeLists.txt:12-14`: the plugin resolves `View`, `Storage` and `BackgroundCache` from the executable at load time | Moving or hiding those symbols breaks plugin loading. These are not Corona symbols, but they are in the same executable. |
| 8 | Timing workarounds | 200 ms (`lattecorona.cpp:1448`); 300 ms and 800 ms (`:1202`, `:1210`); the screen-sync timer (`:405-410`); the ScreenGeometries 2.5 s timer (`screengeometries.cpp:42`); the ksmserver bounded query (`:476-480`) | Each is a documented workaround. A move must keep the timer contexts, as `:1208-1209` requires. |
| 9 | Dead cache | `m_availableCacheValid` is only ever written `false` (`lattecorona.cpp:536`, `:538`, `:1177`). The read and write branches (`:878-885`, `:1022-1024`, `:1051-1057`, `:1128-1130`) never run. | Do not wire it by accident. A real cache must set the flag only after a full recompute, and invalidate on every hub signal (§1.4). This is grep evidence only; nothing was run. |
| 10 | Dead surface | `plasmaScreenPool()` (`lattecorona.h:164`, `lattecorona.cpp:794-797`) has no callers. `Corona::unload()` (`lattecorona.h:210`, `lattecorona.cpp:601-620`) has no callers, but `sourcecontracttest.cpp:4868-4880` reads its text. | Do not carry these into new interfaces. Remove each in its own slice after tracing consumers (AGENTS rule 5). The C++, string and QML greps found no caller. |
| 11 | Null-Corona contracts differ | `manager.cpp:51` guards the constructor, but `memoryUsage()` (`:157`) does not; `synchronizer.cpp:50-60` dereferences; `globalshortcuts.cpp:52-53` skips `init()` | A typed, non-null constructor parameter changes behaviour for any caller that passes null today. Grep shows that every production parent is a Corona, but not every caller was traced. |
| 12 | Two-phase initialisation | `genericlayout.cpp:167-169`; `synchronizer.cpp:670`, `:981`; `genericlayout.h:113` | Replace it with constructor injection in that family's slice. |
| 13 | Destruction-time reads | `manager.cpp:59`, then `:157`, then `universalSettings()`; `view.cpp:543-553` | Keep the order in §2.4. |
| 14 | `main()` lambdas capture `&corona` | `main.cpp:669-682` | A Services struct or narrower handles need the same lifetime. The monitor must still die before the Corona (`main.cpp:652-656`). |
| 15 | Detached relaunch | `lattecorona.cpp:462-466` starts a process while this one is in its destructor | Its interaction with the shortcut owners that are not deleted (§2.5) is not verified. |

## 6. Evidence that would change the recommendation

- **A profile that puts `availableScreenRect*` on a hot path** (per frame or per
  pointer event). Then caching, and the dead flag in §5 item 9, becomes the first
  topic.
- **Snapshot inputs that need live window state**, for example
  `positioner()->isOffScreen()` or `geometry()` during a move. Then the reduction
  needs more inputs than a snapshot can hold cleanly, and the launcher family
  becomes the better first slice.
- **A launcher bug, or a second external user** of the D-Bus launcher methods
  (`app/dbus/org.kde.LatteDock.xml`, `app/launcherhelper.cpp:42`). The launcher
  routing (`app/lattecorona.cpp:91-352` and `:292-350`) is equally pure, and its
  only narrow dependency is `sortedCurrentViews()`. It would then be the better
  first slice.
- **Evidence that libplasma calls `availableScreenRect()` once per containment in
  a loop.** Then measure the cost of building snapshots before merging.
- **Compile evidence** that `GlobalShortcuts` or `Layouts::Manager` needs its
  friend declaration. Keep it if so. If not, remove it in a separate slice.
- **A runtime path that sets `m_availableCacheValid` to true**, through a macro, a
  QML call, or code that the grep missed. Then the dead-cache conclusion is wrong,
  and the cache needs its own review.
- **Evidence that the libplasma 6.8 branch changes the available area.** Then add a
  version-specific test case.
- **A decision to change the D-Bus API.** Then the adaptor work moves ahead of the
  geometry slice.

## 7. Not verified

- No build, test or run; the task was a design. Counts are `git grep` inventories,
  with receivers matched by name. Expect a few sites of error.
- The generated D-Bus adaptor (`qt_add_dbus_adaptor`) was not inspected. The
  worktree has no build tree.
- libplasma's `Plasma::Corona` implementation and destructor were not inspected.
  The installed header `/usr/include/Plasma/plasma/corona.h` was read only for its
  virtuals, and its version was not compared with the 6.3 floor.
- Whether libplasma calls the availability virtuals internally.
- KWayland and KActivities internals: child ownership, and
  `ConnectionThread::fromApplication`.
- Whether any child listed in §2.5 is deleted somewhere else. Reading found none.
- The runtime effect of §2.5 and of the detached relaunch.
- The expected values in §4.5 come from reading the code. They are unverified until
  the test is written and run.
- Desktop, VM and Debian 13 behaviour. None of it was run, and Debian 13 is deferred
  by the user.
- Whether sharing `KSharedConfig::openConfig()` between services is safe.

## 8. Method

- Read in full: `app/lattecorona.h` (306 lines), `app/lattecorona.cpp` (1707
  lines), `app/session/teardown.h`, `app/main.cpp:645-720`,
  `docs/architecture-assessment-2026-10.md`, `docs/modernization-results/R1a.md`,
  `R1b.md`, and the R-track rows of the ledger.
- Read the cited lines of every consumer named in §1, plus
  `app/layouts/manager.cpp`, `app/layouts/synchronizer.cpp`,
  `app/plasma/extended/screengeometries.cpp`,
  `app/shortcuts/globalshortcuts.cpp`, the signal relays in `app/view/view.cpp`,
  the friend uses in `app/layouts/storage.cpp`, and
  `app/declarativeimports/interfaces.cpp`.
- Two-hop chains: 279, matched by `\b(m_corona|corona\(\)|latteCorona|corona)->\w+\(\)->\w+\(`.
  The assessment's 271 used a different pattern, which was not reproduced.
- Files naming `Latte::Corona` (`*.cpp`, `*.h`, `*.qml`, autotests excluded): 63.
  Files containing the word `Corona`, including `Plasma::Corona` and comments: 86.
  The assessment's 77 was not reproduced.
- `qobject_cast<Latte::Corona *>`: 20 sites, as the assessment says.
- The raw call-site inventory is in the shared notes folder, outside the repository.
