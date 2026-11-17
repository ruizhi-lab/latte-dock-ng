# P3a: Edge-layout refresh evaluation

Status: Deferred. A private Fedora session now verifies the loaded task QML
source and observes edge-triggered refresh passes. The production publication
path still has no trustworthy readiness signal that can safely stop the
bounded repair timer early. No runtime behavior was changed and no reduction
in layout or geometry-publication work is claimed.

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

No P3a readiness or convergence test was run because the required latest-
generation acknowledgement is absent; the eight delayed passes and fallback
remain unchanged.

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

## Fedora VM transition and harness follow-up — 2026-10-01

The Fedora 44 Wayland VM's XWayland authority path was read from the running
Xwayland command line. A one-session KWin remote-input prompt was approved
through temporary ydotool keyboard input with “Always allow” left unchecked.
Afterward, `DISPLAY=:0 XAUTHORITY=<session-file> xdotool mousemove 1435 687`
placed the pointer at the intended settings control; a pointer-included
screenshot verified its actual position before each click.

Two UI paths moved the visible Dock from Bottom to Top and back to Bottom. The
Layout Editor's Docks table applied its Top row value, then discarding the
dialog restored the saved layout exactly. The Dock's own Edit Dock panel also
moved the visible Dock Top and back Bottom. These observations confirm that
the GUI transitions worked, but a temporary `console.log` probe in
`refreshTaskLayoutPass()` emitted no marker for either path, even after
preserving and clearing the QML cache before relaunch. This does not establish
whether the handler ran; therefore no refresh-pass count, convergence result
or P3a performance claim is made. The settings-driven route may recreate a
view rather than signal the existing QML root, and probe loading/log routing
still needs an independent check.

The exact QML source, compiled cache and saved layout were restored. The active
layout SHA-256 matches its pre-test value:
`ed6ade8626fe13cd10e93233e51cd83247faa838975d1f4c74e20b5d11e408ec`.
The user-mode Debug Dock was relaunched and its fresh log contains only the
previously observed VM EGL warnings and normal startup information. P3a stays
Deferred until the production path can report latest-generation geometry
readiness; the refresh path observability gap was closed by the isolated probe
below.

## Isolated Fedora private-session probe — 2026-11-17

The later probe resolves the earlier missing-marker uncertainty. A private
HOME, XDG tree, runtime directory, session bus, QML cache and nested virtual
KWin were used on Fedora 45. The temporary overlay ran the task QML markers
and a containment Timer driver that called the same
`Positioner::setNextLocation()` entry point used by the Dock edge buttons. It
requested TopEdge (Plasma enum value 3) and then BottomEdge (value 4). The
normal desktop, layout, user configuration and system install were not used.

The task QML emitted `P3A_SOURCE_LOADED` with a `file:///tmp/P3a-private-…/home`
URL identifying the private overlay. Both transitions then logged, in order,
the driver request, `locationChanged` with the requested edge value, and
`refreshTaskLayoutPass(reason=locationChanged, pass=0)`. Each transition also
produced passes 1 through 8 at the existing 120 ms interval. Latte's geometry
logs and view map showed TopEdge after the first move and BottomEdge after the
second. This verifies source selection and marker routing, and confirms that
this relocation route reaches the existing handler. It does not establish
geometry-publication acceptance by TaskManager/KWin or a convergence condition.

The report, marker observations, tool details and limitations are recorded in
[`P3a-data/README.md`](P3a-data/README.md). The runner and instrumenter live in
[`P3a-data/tools`](P3a-data/tools/). Their temporary overlays are hash-checked;
the installed task and containment QML hashes matched the workspace sources
before and after the run. The probe was diagnostic only: no readiness signal,
timer change, performance measurement or optimization is claimed.
