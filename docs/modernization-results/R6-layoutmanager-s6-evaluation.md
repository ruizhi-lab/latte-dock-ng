# R6-S6: typed layout-item base type — evaluation

Status: **Evaluated; implemented as the cheaper alternative (diagnostics), not
as the typed base class.**

## What S6 proposes

A C++ `QQuickItem` base type for layout slots with typed members replacing the
name-based reads the containment manager performs on layout children:

- `isInternalViewSplitter` — 7 reads in `containment/plugin/layoutmanager.cpp`;
- `isParabolicEdgeSpacer` — 3 reads;
- `backendAppletRef` — 6 reads;
- `applet`.

`AppletItem.qml` (2077 lines) and `layouts/ParabolicEdgeSpacer.qml` would derive
from it, and the manager would `qobject_cast` instead of reading properties by
name.

## Where the reads actually happen

Each read sits in a structural function, not a per-frame path:

| Function | Trigger |
| --- | --- |
| `appletItemInLayout()` | applet lookup during repair/add |
| `isJustifySplitter()` | layout-edge insertion |
| `dndSpacerIndex()` | once per drop |
| `save()` | once per order change |
| `hideAppletItem()` / `showAppletItem()` | user action or restore |
| `destroyAppletContainer()` / `destroyJustifySplitters()` | removal |

So, like S5, S6 has **no measured performance case**. Its justification is type
safety: a renamed QML property currently degrades to `QVariant::toBool()`
returning false, i.e. a silent wrong answer.

## What S1 already covers, and what it does not

S1 added diagnostics for the *containment root's functions*
(`createAppletItem`, `initAppletContainer`) — those now warn when missing. The
layout-child property reads S6 targets are **not** covered: a renamed
`isInternalViewSplitter` still fails silently.

## Cost of the slice

| Item | Size |
| --- | --- |
| New C++ QML type registered in `lattecontainmentplugin` | new registration and loading contract |
| `AppletItem.qml` root type change | 2077-line file, all bindings on the root re-checked |
| `ParabolicEdgeSpacer.qml` root type change | second root, plus `isInternalViewSplitter: false` override |
| qmllint baseline | **60 entries** for `AppletItem.qml` are invalidated and must be re-reviewed one by one |
| `applet` / `backendAppletRef` | point at Plasma-owned applet objects in part, so a name-based fallback likely has to stay (design section 5, family 2) |

Design §7.6 itself rates S6 "large" and requires the 60 baseline entries to be
reviewed, the bindings re-checked and performance verified.

## Assessment

S6 delivers no measurable runtime benefit and touches the most binding-dense
QML file in the repository plus 60 lint baselines. Against that, the concrete
gain over today's state is that a renamed layout-child property would fail at
compile/registration time instead of silently returning false.

That is a real defect class — but it is the same class S1 addressed for the root
functions, and it can be closed much more cheaply than by restructuring two root
types: the manager can detect and warn once when one of these property names is
absent from a layout child, exactly as S1 did for the root functions. A warning
costs nothing at runtime, needs no QML root change, and turns the silent wrong
answer into a logged one.

Recommendation: **do not start S6 as written**. Either
(a) add the missing-property diagnostics for these names (small, same shape as
S1), or (b) leave the reads as they are; the type-safety gap is closed by (a)
without the 60-baseline churn.

## What was implemented instead

Option (a). `LayoutManager::noteLayoutChildRoles()` inspects each layout child on
first contact in `save()` and reports a role the child stopped declaring through
`warnLayoutChildRoleMissingOnce()`, once per role name:

- checked on every layout child: `isInternalViewSplitter`, `isParabolicEdgeSpacer`;
- checked additionally on applet containers: `backendAppletRef`;
- `applet` is not checked: Plasma supplies it on backend applet objects, not in
  this package.

The first draft demanded all three roles of every child and produced two false
warnings on a healthy desktop (`ParabolicEdgeSpacer_QMLTYPE_105` has no
backendAppletRef by design). The per-class role list above is the correction, and
`edgeSpacerNeedsNoBackendAppletRole` in `containmentlayoutunittest` pins it.

No QML changed, no root type changed, no qmllint baseline entry moved.

Verification: GCC and Clang Debug build with zero compiler warnings and pass
65/65 CTest each; `qmllint` gate passes on 244 files; a user-mode Debug run logs
both containments with **0** role warnings, confirming no false report on a real
layout; removing the call makes the new test fail, so it is a genuine lock.

No measurement in this record was taken from a profile: it is a code-shape and
cost evaluation, and says nothing about runtime behaviour.
