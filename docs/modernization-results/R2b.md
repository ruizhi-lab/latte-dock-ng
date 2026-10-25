# R2b: Context-less Connections

Status: Complete for the code and build gates. Desktop retest pending.
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

## Pending

The desktop smoke of the touched paths was not run. It covers the alternatives
dialog (`alternativeshelper.cpp`), the applet close/undo notification
(`contextmenulayerquickitem.cpp`), the settings tabs for activities, parabolic
spread and screen tracking, the indicator removal confirmation, the settings
and widget-explorer Ready path, and the containment context menu. Record the
result in this section before treating the desktop gate as passed.
