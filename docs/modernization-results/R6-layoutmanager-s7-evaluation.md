# R6-S7: creation path in C++ — evaluation

Status: **Evaluated, not started. Recommended against.**

## What S7 proposes

Move the applet-container creation out of the containment QML into C++: replace
the root functions `createAppletItem(QVariant)` and
`initAppletContainer(QVariant,QVariant)` with C++ creation from a typed
`QQmlComponent*`, keep the C++ applet resolver, and replace the JavaScript retry
timer (`Qt.createQmlObject` in `main.qml`) with a C++ timer.

The manager already reaches these two functions by name in six places
(`layoutmanager.cpp:1250`, `:1911`, `:2050`, `:2120`, `:2131` plus the cached
method lookup in `onRootItemChanged()`), so the call is typed only if the
function bodies move too.

## What the QML bodies do today

`main.qml` `createAppletItem()` is not a thin factory. It has to keep, in one
place, all of the following:

1. **`Qt.binding()` for opacity.** Two `Qt.binding()` calls set
   `appletContainer.opacity` to `appletContainerShouldBeVisible()`'s result. The
   repository calls the opacity-based visibility deliberate: `visible:false`
   triggers applet-internal element state resets.
2. **A live visibility predicate over three reactive properties.**
   `appletContainerShouldBeVisible()` reads `applet` (a Plasma-owned backend
   object), `status` and `keepVisibleOnHiddenStatus` and `isHidden` from the
   container. `AppletItem.qml:379` resolves `status` through
   `readDynamicProperty(statusSource, "status")`, i.e. the value is not even a
   static property of the container; `isHidden` (`:224`) in turn depends on
   applet state and container properties.
3. **A binding that must survive later state changes.** The opacity binding is
   re-evaluated by the QML engine whenever any upstream property changes. C++ has
   no equivalent: assigning `setOpacity()` once, or connecting every dependency
   by hand, does not reproduce a declarative binding over a property graph that
   the QML engine currently maintains for free.
4. **A retry timer tied to the container's lifetime.** Each attempt is a `Timer`
   parented to the container, destroyed on success or after ~4 s (80 × 50 ms),
   and the container is destroyed if it never succeeds.

## Why this is not a mechanical refactor

The typed-interface argument (S2) worked because the application called the
plugin through a name; the *bodies* stayed where they belonged. S7 moves bodies,
and those bodies are QML bindings over QML-owned state. Reimplementing (1)–(3) in
C++ means either

- keeping `Qt.binding()` through a `QJSValue` created in C++ (i.e. still calling
  into the QML engine, now from a less readable place), or
- replacing the binding with explicit connections on every dependency — which the
  next change to `status`/`isHidden` silently breaks, because nothing in C++ knows
  that `status` is a dynamic property resolved at run time.

Both outcomes are worse than the current code for the same behaviour.

## Cost and risk

| Item | Assessment |
| --- | --- |
| QML root contract | `createAppletItem`/`initAppletContainer` are a compatibility surface for separately installed plugins; removing them needs a migration window |
| Opacity invariant | must be preserved exactly (repository calls it deliberate) |
| Retry timing | 80 × 50 ms limit and per-container timer lifetime must be reproduced |
| Test coverage | the retry path is only exercised by desktop runs; a C++ rewrite would start without one |

Design §7.6 itself rates S7 "large, last" and lists the opacity visibility as
something that "must be kept". Nothing in this evaluation challenges that
ordering; it challenges whether the slice pays for itself.

## Verdict

**Do not start S7.** It moves QML to C++ without removing a boundary — the
creation still has to reach the QML engine for bindings — while putting a
deliberate, subtly-coupled visibility binding on the other side of a rewrite that
no autotest covers.

If the motivation is to stop reading these two functions by name, S1 already
made their absence loud, and the remaining name dispatch is a single cached
`QMetaMethod::invoke()` per applet creation — a structural, not per-frame, cost.

## Measurement note

No profile was taken for S7, for the same reason S5's profile did not help: the
accesses are structural (once per applet creation or repair), not per frame. This
evaluation is a code-shape and risk assessment and claims nothing about runtime
behaviour.
