# R2b: Context-less Lambda Connections

Status: Complete for the code and build gates. Desktop retest pending.
Date: 2026-10-26. Branch: `main`. Implementation commit: see
`git log --grep 'fix: give every connection a context object'`.

## Defect

Seventeen `QObject::connect()` calls passed a functor without a context object.
Such a connection is only disconnected when the sender dies, so the lambda kept
running after the objects it captured were destroyed. Two sites captured by
reference (`[&]`) in a constructor, which is a dangling reference from the first
emission onwards. The rest captured raw pointers and read them after their
lifetime. Qt 6.7 and newer turn this into a compile error through
`QT_NO_CONTEXTLESS_CONNECT`; the declared Qt 6.6 floor does not, so the macro
was not defined and nothing enforced the rule.

## Fix

**Macro.** `CMakeLists.txt` defines `QT_NO_CONTEXTLESS_CONNECT`. It is enforced
from Qt 6.7, which the review host (6.11.2) provides, and ignored by the Qt 6.6
floor, so the floor still configures.

**Sites.** Each connection now names a context object whose lifetime matches the
captures:

| Site | Context | Why |
| --- | --- | --- |
| [alternativeshelper.cpp](../../app/alternativeshelper.cpp) | `contItem` | The containment item receives the replacement applet. The helper itself is not the context: the alternatives dialog can delete it before the old applet finishes its deferred destruction. |
| [contextmenulayerquickitem.cpp](../../app/declarativeimports/contextmenulayerquickitem.cpp) | `layoutMgr` (guard hoisted) | The undo and destroy follow-ups only drive the layout manager, so they must end with it instead of outliving it. |
| [lattecorona.cpp](../../app/lattecorona.cpp) | `this` | `Corona` owns the synchronizer, registry and the objects the lambda creates. |
| [indicator/factory.cpp](../../app/indicator/factory.cpp) | `this` | The factory outlives the confirmation dialog it creates. |
| [activitiesdelegate.cpp](../../app/settings/settingsdialog/delegates/activitiesdelegate.cpp) | `menu` | The lambda only manipulates the menu; the button that sends the signal can be replaced by the delegate. |
| [tablayoutshandler.cpp](../../app/settings/settingsdialog/tablayoutshandler.cpp), [tabpreferenceshandler.cpp](../../app/settings/settingsdialog/tabpreferenceshandler.cpp) | `this` | The handler owns the preferences it updates. |
| [indicator.cpp](../../app/view/indicator/indicator.cpp) | `this` | The indicator owns the state it re-emits. |
| [primaryconfigview.cpp](../../app/view/settings/primaryconfigview.cpp), [widgetexplorerview.cpp](../../app/view/settings/widgetexplorerview.cpp) | `this`, `[&]` → `[this]` | The views are the sender and the receiver of their own status change; capturing by reference in a constructor was a dangling reference. |
| [visibilitymanager.cpp](../../app/view/visibilitymanager.cpp) | `this` | The manager owns the struts timer connection it replaces. |
| [menu.cpp](../../containmentactions/contextmenu/menu.cpp) | `this` | The actions plugin owns the actions and outlives the containment signal. |

No behavior was changed beyond the lifetime scope of the callbacks.

## Evidence

| Check | Result |
| --- | --- |
| Context-less connection scan over all production `.cpp` | 0 live sites (one match was inside a commented-out block in `menu.cpp`) |
| Qt version on the review host | 6.11.2, so the macro is enforced |
| GCC Debug, `LATTE_STRICT_WARNINGS=ON` | Build succeeded, 0 compiler warnings, 56/56 CTest |
| Clang 22 Debug, `LATTE_STRICT_WARNINGS=ON` | Build succeeded, 0 compiler warnings, 56/56 CTest |

## Pending

The desktop smoke of the touched paths was not run in this pass. It covers the
alternatives dialog (`alternativeshelper.cpp`), the applet close/undo
notification (`contextmenulayerquickitem.cpp`), the settings tabs for activities,
parabolic spread and screen tracking, the indicator removal confirmation, the
settings and widget-explorer Ready path, and the containment context menu.
Record the result in this section before treating the desktop gate as passed.
