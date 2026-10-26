# R2b: Context-less Connections

Status: Complete. The code, build and desktop checks of every touched path
pass.
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

**Later site.** The desktop smoke reached `Corona::showAlternativesForApplet()`
in [lattecorona.cpp](../../app/lattecorona.cpp). The scan could not flag it,
because its connections had a context object, just the wrong one. The flag
reset after the alternatives dialog closes was bound to Corona but only drove
the view, captured as a raw pointer, while the dialog's QML object is a child
of that view. Removing the dock with the dialog open therefore ran it on a
destroyed view, and a dialog without a view dereferenced a null pointer. It now
uses the view as the context and connects only when a view exists. The
`m_alternativesObjects` list also kept dialogs that were deleted with their
view; a dialog now leaves the list when it is destroyed. Commit `56c34148c`;
`SourceContractTest::lattecoronaAlternativesCallbacksFollowTheirOwners()` locks
both.

## Evidence

Fresh preset trees on the review host: GCC 16.2.1, Clang 22.1.8, Qt 6.11.2.

| Check | Result |
| --- | --- |
| Scan for three-argument `connect()` calls, comments stripped | 24 in the parent of `484a8d59b`, 0 now; the installed KF6 and Plasma headers contain none |
| Fixture with a context-less lambda `connect()` and the application's flags | Rejected by GCC and Clang; the same call with a context object compiles |
| `QT_NO_CONTEXTLESS_CONNECT` in `compile_commands.json` | Present in 427 of 427 translation units in both Debug trees |
| GCC and Clang Debug/Release presets, `LATTE_STRICT_WARNINGS=ON` | 0 compiler warnings; 56/56 CTest in all four |
| CI run 40180566388 at `3023bddc3` | Passed, including the Debian 13 build with Qt 6.8.2, which also enforces the macro |
| GCC and Clang Debug/Release presets at `56c34148c`, after both fix merges | 0 compiler warnings; 57/57 CTest in all four; the new source contract fails on the previous code and passes now |

The first version of this report counted seventeen sites and described the two
`[&]` captures as dangling references. Neither was accurate; the table above
replaces it.

## Desktop smoke

Run on 2026-10-25 by the user with the user-mode Debug build of `e4c24e773`
(`install.sh --user Debug`, launched with `--debug`) on Gentoo with Plasma
6.7.5, Wayland and Qt 6.11.2. The debug log has no errors and one warning
(see the environment note below), and no latte-dock-ng coredump was written.

A second round on 2026-10-26 ran the branch `claude/nice-merkle-7b9fb9`, which
contains R2b and the edit-mode menu fixes now merged in `f634322d4`. The user
confirmed the results; that log had no warnings and no coredump was written.

A third round on 2026-10-26 ran `main` at `ed5579e4c`, which includes both
merges and `56c34148c`. The dock was started through `systemd-run --user`, so it
had the session environment rather than the agent shell's. The log has no
errors and one QML deprecation warning, listed below, and no coredump was
written.

| Touched path | Check | Result |
| --- | --- | --- |
| `contextmenulayerquickitem.cpp` | Remove a widget and undo it from the notification; remove one and let the notification expire | Passed |
| `tablayoutshandler.cpp`, `tabpreferenceshandler.cpp`, `activitiesdelegate.cpp` | Layouts mode switch, activities menu OK/Reset/Cancel, parabolic spread, thickness margin influence and screen delay; apply and reopen | Passed |
| `primaryconfigview.cpp`, `widgetexplorerview.cpp` | Open Edit Dock and Add Widgets, check their effects, reopen | Passed |
| `visibilitymanager.cpp` | Always Visible dock, change its size, maximize a window | Passed: the window stays clear of the dock |
| `lattecorona.cpp` | Startup and three layout reloads during the settings checks | Passed: windows are tracked and the layouts load |
| `menu.cpp`, actions | Configure Latte... from the dock menu | Passed |
| `indicator.cpp`, `indicatorPluginChanged` | Install a local test indicator while the dock runs, select it, switch back to Latte | Passed: it appears in the custom list without a restart and the dock follows the selection |
| `menu.cpp`, `userConfiguringChanged` | Enter edit mode and right-click the empty dock area, then leave edit mode | Passed in the second round: the edit-only dock actions appear and Edit Dock... is hidden; outside edit mode the menu is unchanged |
| `indicator/factory.cpp` confirmation, `indicator.cpp` `indicatorPluginRemoved` | Select the local test indicator on a dock, then remove it with the remove icon at the left of its row in the custom list and confirm | Passed in the third round: the confirmation dialog appears, the log records "Trying to remove indicator", the package directory is removed, and the dock that used it falls back to Latte |
| `alternativeshelper.cpp` | Show Alternatives on an applet in edit mode and choose an alternative | Passed in the second round: the dialog opens and the chosen alternative replaces the applet in place |
| `lattecorona.cpp`, alternatives dialog callbacks (`56c34148c`) | Open Show Alternatives in edit mode, close it, then click outside the edit window | Passed in the third round: the dialog closes and the edit window closes on focus loss as usual, so the view's flag was reset |

Findings that predate R2b:

- In edit mode, a `MouseArea` in `containment/package/contents/ui/main.qml`
  accepts every press outside the tasks plasmoid. It was added on 2026-07-03
  and is in v1.2.62. The dock menu therefore never opens in edit mode, and
  "Show Alternatives..." and the edit-only dock actions, which only appear in
  edit mode, cannot be reached. Fixed by `2a6015796` and `50b920180`, merged in
  `f634322d4`.
- On Plasma 6.5 to 6.7, choosing "Show Alternatives..." did nothing: below
  Plasma 6.8 Latte used plasma-desktop's `AppletAlternatives.qml`, which imports
  the plasmashell-only `org.kde.plasma.shell` module since Plasma 6.5. Fixed by
  `f7bcc824b`, merged in `f634322d4`.
- In the Effects page, switching from Latte to an indicator without a config
  UI and back leaves the options area blank, because
  `IndicatorConfigUiManager.qml` keeps the previous page. Fixed by `58182590b`,
  merged in `12f063d93`.
- The test indicator was a copy of Plasma Tab Style, which only frames the
  active task, so the dock showed no dots or lines until the user switched back
  to Latte. That is expected.
- In the first two rounds the dock was launched from an agent shell and
  inherited its environment; when an application was started from the dock,
  KIO warned that `SENTRY-TRACE` cannot be passed to systemd. The third round
  used the session environment and did not show it.
- Clicking the remove icon logged a Qt deprecation warning:
  `declarativeimports/components/ComboBox.qml` uses the injected `index`
  parameter in `onIconClicked` instead of a formal parameter. It is unrelated
  to the connection changes and is left for a QML fix.

## Pending

None for R2b.
