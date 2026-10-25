# R2b: Context-less Connections

Status: Pending validation. The code, build and most desktop checks pass; the
indicator removal confirmation and the alternatives dialog are not exercised.
Date: 2026-10-25. Branch: `main`. Implementation commit: `484a8d59b`.

## Defect

Twenty-four `QObject::connect()` calls had no context object: 23 lambdas (17 in
`app/` and 6 in `containmentactions/contextmenu/menu.cpp`) and one
member-function connection in `visibilitymanager.cpp`. Such a connection is
only disconnected when the sender is destroyed, so a callback that uses an
object the sender does not own can run after that object is gone. Qt 6.7 and
newer reject the three-argument form under `QT_NO_CONTEXTLESS_CONNECT`; the
declared Qt 6.6 floor does not, so the macro was not defined and nothing
enforced the rule.

Three sites were not bounded by their sender: the alternatives helper, the
indicator removal confirmation and the containment `userConfiguringChanged`
handler (see the table). At every other site the sender already bounded the
callback, because it is the receiver itself, a descendant of the context or
owned by the receiver. Naming the context there makes that lifetime explicit
and lets the macro be defined.

## Fix

**Macro.** `CMakeLists.txt` defines `QT_NO_CONTEXTLESS_CONNECT`. It is enforced
from Qt 6.7 and ignored by the Qt 6.6 floor, so the floor still configures.

**Sites.** Each connection now names a context object whose lifetime matches the
captures:

| Site | Sender | Context | Lifetime before the change |
| --- | --- | --- | --- |
| [alternativeshelper.cpp](../../app/alternativeshelper.cpp) | Old applet, `destroyed` | `contItem` | Not bounded: the containment item that receives the replacement could be gone before the old applet's deferred destruction ends. The helper is not the context because the alternatives dialog can delete it first. |
| [contextmenulayerquickitem.cpp](../../app/declarativeimports/contextmenulayerquickitem.cpp) (2) | Applet | `layoutMgr`, with an early return when it is gone | Safe through the `QPointer` check, but the connections stayed as no-ops until the applet died; they now end with the layout manager. |
| [indicator/factory.cpp](../../app/indicator/factory.cpp) | Button box of the removal confirmation | `this` | Not bounded: the dialog has no parent, so nothing tied the accept handler to the factory it calls. |
| [lattecorona.cpp](../../app/lattecorona.cpp) (3) | Layout synchronizer, Wayland registry | `this` | Bounded: `Corona` owns both senders. |
| [activitiesdelegate.cpp](../../app/settings/settingsdialog/delegates/activitiesdelegate.cpp) (2) | OK and Reset buttons inside the menu | `menu` | Bounded: the buttons are descendants of `menu`, which is a child of the editor button that the OK handler also uses. |
| [tablayoutshandler.cpp](../../app/settings/settingsdialog/tablayoutshandler.cpp) (1), [tabpreferenceshandler.cpp](../../app/settings/settingsdialog/tabpreferenceshandler.cpp) (3) | Button groups, screen-delay spin box | `this` | Bounded: the handler owns the button groups; the spin box belongs to the dialog that owns the handler. |
| [indicator.cpp](../../app/view/indicator/indicator.cpp) (2) | Parent view | `this` | Bounded: the view is the indicator's parent. |
| [primaryconfigview.cpp](../../app/view/settings/primaryconfigview.cpp), [widgetexplorerview.cpp](../../app/view/settings/widgetexplorerview.cpp) | The view itself | `this`; `[&]` became `[this]` | Bounded: sender and receiver are the same object. The `[&]` lambdas only used `this`, so they held no dangling reference; `[this]` states the capture. |
| [visibilitymanager.cpp](../../app/view/visibilitymanager.cpp) | The manager itself | `this` | Unchanged: the three-argument member-function form already used the sender as the receiver. |
| [menu.cpp](../../containmentactions/contextmenu/menu.cpp) (5 actions) | Actions owned by the plugin | `this` | Bounded: the plugin owns the actions. |
| [menu.cpp](../../containmentactions/contextmenu/menu.cpp) (`userConfiguringChanged`) | Containment | `this` | Not bounded: the containment outlives the actions plugin when its actions are reconfigured. |

No behavior was changed beyond the lifetime scope of the callbacks.

## Evidence

Fresh preset trees on the review host: GCC 16.2.1, Clang 22.1.8, Qt 6.11.2.

| Check | Result |
| --- | --- |
| Scan for three-argument `connect()` calls, comments stripped | 24 in the parent of `484a8d59b`, 0 now; the installed KF6 and Plasma headers contain none |
| Fixture with a context-less lambda `connect()` and the application's flags | Rejected by GCC and Clang; the same call with a context object compiles |
| `QT_NO_CONTEXTLESS_CONNECT` in `compile_commands.json` | Present in 427 of 427 translation units in both Debug trees |
| GCC and Clang Debug/Release presets, `LATTE_STRICT_WARNINGS=ON` | 0 compiler warnings; 56/56 CTest in all four |
| CI run 40180566388 at `3023bddc3` | Passed, including the Debian 13 build with Qt 6.8.2, which also enforces the macro |

The first version of this report counted seventeen sites and described the two
`[&]` captures as dangling references. Neither was accurate; the table above
replaces it.

## Desktop smoke

Run on 2026-10-25 by the user with the user-mode Debug build of `e4c24e773`
(`install.sh --user Debug`, launched with `--debug`) on Gentoo with Plasma
6.7.5, Wayland and Qt 6.11.2. The debug log has no errors and one warning
(see the environment note below), and no latte-dock-ng coredump was written.

| Touched path | Check | Result |
| --- | --- | --- |
| `contextmenulayerquickitem.cpp` | Remove a widget and undo it from the notification; remove one and let the notification expire | Passed |
| `tablayoutshandler.cpp`, `tabpreferenceshandler.cpp`, `activitiesdelegate.cpp` | Layouts mode switch, activities menu OK/Reset/Cancel, parabolic spread, thickness margin influence and screen delay; apply and reopen | Passed |
| `primaryconfigview.cpp`, `widgetexplorerview.cpp` | Open Edit Dock and Add Widgets, check their effects, reopen | Passed |
| `visibilitymanager.cpp` | Always Visible dock, change its size, maximize a window | Passed: the window stays clear of the dock |
| `lattecorona.cpp` | Startup and three layout reloads during the settings checks | Passed: windows are tracked and the layouts load |
| `menu.cpp`, actions | Configure Latte... from the dock menu | Passed |
| `indicator.cpp`, `indicatorPluginChanged` | Install a local test indicator while the dock runs, select it, switch back to Latte | Passed: it appears in the custom list without a restart and the dock follows the selection |
| `menu.cpp`, `userConfiguringChanged` | Enter and leave edit mode repeatedly | Ran without warnings; the effect is not visible because the dock menu cannot be opened in edit mode |
| `indicator/factory.cpp` confirmation, `indicator.cpp` `indicatorPluginRemoved` | Remove the local test indicator from the custom list | Not exercised: the confirmation handler never ran (no "Trying to remove indicator" in the log) |
| `alternativeshelper.cpp` | Show Alternatives on an applet | Not reachable: the dock menu cannot be opened in edit mode |

Findings that predate R2b:

- In edit mode, a `MouseArea` in `containment/package/contents/ui/main.qml`
  accepts every press outside the tasks plasmoid. It was added on 2026-07-03
  and is in v1.2.62. The dock menu therefore never opens in edit mode, and
  "Show Alternatives..." and the edit-only dock actions, which only appear in
  edit mode, cannot be reached. A separate task restores the menu.
- In the Effects page, switching from Latte to an indicator without a config
  UI and back leaves the options area blank, because
  `IndicatorConfigUiManager.qml` keeps the previous page. A separate task fixes
  it.
- The test indicator was a copy of Plasma Tab Style, which only frames the
  active task, so the dock showed no dots or lines until the user switched back
  to Latte. That is expected.
- The dock was launched from an agent shell and inherited its environment. When
  an application was started from the dock, KIO warned that `SENTRY-TRACE`
  cannot be passed to systemd. Start later retests from the session
  environment.

## Pending

Remove a local custom indicator through the remove icon in the custom list and
confirm the dialog, which exercises the `factory.cpp` confirmation and the
`indicatorPluginRemoved` fallback. Run Show Alternatives once the dock menu
opens in edit mode again. Record both here before treating the desktop gate as
passed.
