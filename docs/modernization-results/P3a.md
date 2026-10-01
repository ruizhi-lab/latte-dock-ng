# P3a: Edge-layout refresh evaluation

Status: Deferred after early re-evaluation. The current implementation still
has no trustworthy readiness signal that can safely stop the bounded repair
timer early. No runtime behavior was changed and no reduction in layout or
geometry-publication work is claimed.

Recorded main baseline: `e3ef1ddf001aa032cffa62db21cfd97196174746`.
Implementation branch: `codex/modernization-m0-baseline`.

## Evaluation

`plasmoid/package/contents/ui/main.qml` responds to both `locationChanged` and
`formFactorChanged` by running an immediate repair and restarting a timer with
up to eight additional passes at 120 ms intervals. Each pass resets list
scrolling, calls `icList.forceLayout()` and emits `publishTasksGeometries()`.
The timer is bounded and only runs after an edge/orientation change.

The task view is a virtualized `ListView`. Its delegates are created and moved
asynchronously, and the `TaskItem.slotPublishGeometries()` path publishes
viewport-clamped geometry only when the task belongs to the current layout and
the view is ready (or not yet ready). Hidden views intentionally publish
screen-edge geometry. Consequently, stable `width`/`height`, a stable number
of currently instantiated children, or one quiet interval cannot establish
that all current-generation delegate geometry has been published. No existing
signal reports that condition. Stopping on any of those proxies could leave a
late delegate or the latest rapid edge change with stale geometry.

Per the P3a rule, the timer, eight-pass bound, immediate pass and fallback are
left unchanged until the production layout/publication path can expose a
generation-aware readiness condition. This avoids changing behavior without a
way to verify convergence.

## Validation

- CodeGraph was queried first for edge-change, layout, force-layout and geometry-publication paths; it did not index the task `main.qml` by those names. After that, `rg` confirmed the refresh timer and signal connections in `plasmoid/package/contents/ui/main.qml`, and the publisher in `plasmoid/package/contents/ui/task/TaskItem.qml`.
- `sed -n '360,420p' plasmoid/package/contents/ui/main.qml` — confirmed location/form-factor changes start the 500 ms geometry publisher and immediate-plus-repeated layout refresh.
- `sed -n '1400,1465p' plasmoid/package/contents/ui/main.qml` — confirmed the immediate pass and eight delayed 120 ms passes, each forcing layout and publishing geometry.
- `sed -n '895,970p' plasmoid/package/contents/ui/task/TaskItem.qml` — confirmed readiness, layout-membership, viewport-clamping and hidden-view conditions in production geometry publication.
- `git diff --check` — passed for this documentation-only evaluation.
- No build or runtime test was run because production code is unchanged.

## Handoff

Do not reduce the timer based on geometry equality or elapsed time. Revisit only
after the production ListView/delegate publisher exposes a readiness signal
bound to the latest edge-change generation and current model state. Then test
late delegates, an empty task model, task arrival/removal during relocation,
rapid superseding edge changes, all four edges, horizontal/vertical layouts,
center/justify alignment, multiple screens and fractional scale before
changing the fallback.

## Early re-evaluation — 2026-10-01

The current `refreshTaskLayoutPass()` has no generation token or completion
acknowledgement. It calls `forceLayout()` and emits a broadcast; each delegate
then independently clamps and publishes geometry through the TaskManager
model. The publication API does not report that KWin/TaskManager accepted the
geometry for every live delegate in the latest generation. A QML profiler
startup trace cannot establish that contract, so the early evaluation does not
change the previous decision. A future slice would need a production
generation-aware publisher/readiness contract and focused tests before it can
reduce passes.

Evidence commands:

```bash
codegraph explore "root.locationChanged formFactorChanged refresh timer layoutPass publishTasksGeometries TaskItem.slotPublishGeometries generation ready signal ListView delegates"
rg -n "onLocationChanged|onFormFactorChanged|refresh|forceLayout|publishTasksGeometries|slotPublishGeometries|viewport|isCurrent" plasmoid/package/contents/ui/main.qml plasmoid/package/contents/ui/task/TaskItem.qml
sed -n '1390,1475p' plasmoid/package/contents/ui/main.qml
sed -n '880,985p' plasmoid/package/contents/ui/task/TaskItem.qml
```

No P3a edge-transition test was run because the required readiness signal is
absent; the eight delayed passes and fallback remain unchanged.

## Cross-version TaskManager API check — 2026-10-01

The publication boundary was checked against the installed TaskManager headers
on the Debian 13 and Fedora 44 VMs, including the newer Fedora Plasma stack.
Both versions expose `requestPublishDelegateGeometry(...)` as a `void` virtual
method on `AbstractTasksModelIface`; neither header exposes a completion signal
or generation token. The API documentation also retains a FIXME that multiple
delegates in multiple applets are not handled. This confirms the missing
acknowledgement is an upstream API limitation on both the compatibility floor
and the newer tested stack, rather than a missed Latte-side signal.

Commands and results:

```text
Fedora 44 / Plasma 6.7.5:
rpm -qf /usr/include/taskmanager/abstracttasksmodeliface.h
  plasma-workspace-devel-6.7.5-1.fc44.x86_64
grep -A22 -B4 -n "requestPublishDelegateGeometry" /usr/include/taskmanager/abstracttasksmodeliface.h
  virtual void requestPublishDelegateGeometry(...);
  header documentation includes the multiple-delegates FIXME

Debian 13 / Plasma 6.3.6:
dpkg-query -S /usr/include/taskmanager/abstracttasksmodeliface.h
  plasma-workspace-dev: /usr/include/taskmanager/abstracttasksmodeliface.h
dpkg-query -W plasma-workspace-dev
  plasma-workspace-dev 4:6.3.6-2
grep -A22 -B4 -n "requestPublishDelegateGeometry" /usr/include/taskmanager/abstracttasksmodeliface.h
  virtual void requestPublishDelegateGeometry(...);
  header documentation includes the multiple-delegates FIXME
```

This strengthens the defer decision: implementing readiness solely in Latte
would guess at acceptance by TaskManager/KWin and could stop refresh passes
before all live delegates have published. Reopen only when the upstream API or
a production-owned adapter can acknowledge the latest generation and current
delegate set. No runtime code or behavior changed; no build was required.
