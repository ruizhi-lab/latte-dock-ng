# Qt 6 Syntax Modernization: Implementation and Fedora VM Validation

Status: Planned; no implementation or VM acceptance claimed.
Date: 2026-10-01. Planning baseline: `2c404cde6` on
`codex/architecture-followup`. Reconcile actual HEAD before implementation.

## Objective and boundaries

Modernize confirmed obsolete spellings and implicit QML signal arguments,
then make selected dynamic interfaces easier to understand without changing
features, event handling, persisted configuration or object lifetime.

Follow [AGENTS.md](../AGENTS.md), the [architecture map](architecture-overview.md)
and [testing guide](development-testing-guide.md). This is a separate S track
from the [A follow-up](architecture-followup-plan.md); it does not mark A2-A5
complete or reopen splitter, screen mapping or log-filter behavior changes.
The user has offered their Fedora VM for functional validation. Resolve the
actual SSH alias/address, login user and checkout from local configuration and
existing access; do not assume that historical VM versions or addresses apply.

Keep Qt 6.6, KF6 6.0, Plasma 6.3 and C++20 floors. Use the existing branch unless
the user requests another branch. The user authorized automatic commits per
completed batch. Commit a coherent accepted slice; leave a runtime slice
Pending validation while its required desktop feedback or checks are missing.
Record investigation and partial changes honestly instead of calling a
documentation-only handoff an implemented syntax change.

## Ordered work packages

| Batch | Prerequisite | Implementation scope | Acceptance |
| --- | --- | --- | --- |
| S0 | None | Verify branch/HEAD and VM access. Record current toolchain, active Plasma session and installed Dock identity. Capture the visible pre-change dock state, prepare isolated config/layout backups and a known-good user-mode reinstall path. Capture interaction-specific baselines immediately before each runtime slice. | Reproducible environment baseline for S1/S2, named build presets, VM record and restoration procedure. This is evidence collection, not a production refactor. |
| S1 | S0 | Replace the two `Q_DECL_OVERRIDE` uses in `declarativeimports/core/iconitem.h` with `override`; replace `Q_NULLPTR` in `app/alternativeshelper.cpp` and `app/shortcuts/globalshortcuts.cpp` with `nullptr`. | GCC/Clang application and affected core plugin compile with zero warnings; relevant existing tests pass. Review preprocessor equivalence. No new tests that merely check keyword spelling. |
| S2 | S0, S1 | In `declarativeimports/components/ScrollArea.qml` and the two wheel handlers in `ComboBox.qml`, change implicit `wheel` injection to `onWheel: function(wheel) { ... }`. Preserve handler bodies and event propagation. | Syntax/import checks, comparable deep-lint delta, focused real-component event checks and Fedora VM wheel/selection retest. Explicit argument access adds no runtime QML error. |
| S3 | S2 | Document and verify the two preview QML-to-C++ signal connections in `plasmoid/preview/main.cpp` and the D-Bus slot connection in `app/wm/abstractwindowinterface.cpp`. Inspect existing failure handling before proposing any change. | Evidence table of signature, owner, lifetime, connection failure behavior and consumer. Fedora preview activate/close and desktop-wrap checks. Keep string signatures where required; any discovered behavior fix becomes a separately scoped implementation slice. |
| S4a | S2 | Inventory unqualified access in exactly one S2 component. For each candidate, record whether it is a local property, ancestor id, delegate role or host context. Select only accesses with a proven owner. | Per-access owner table and focused interaction baseline. Existing lint output identifies exact diagnostics, not just category totals. |
| S4b | S4a | Qualify only the selected stable accesses, one component per slice. Preserve binding dependencies and avoid changing implicit signal parameters and delegate roles in the same patch. | Real component creation, property-update and interaction checks; Fedora settings/scroll behavior matches baseline; comparable lint adds no unexplained diagnostic. |
| S5a | S3, S4b | Inventory `property var` for one component touched by this track. Record actual assigned values, nullability, host provenance and loading lifecycle. | A concrete type contract or a justified dynamic-interface exception beside the owner. An absence of candidates is a valid evaluation outcome, not a claimed implementation. |
| S5b | S5a, conditional | Type only a stable project-owned input with demonstrated benefit. Preserve dynamically heterogeneous Plasma interfaces; adding `required` needs its own creation-path analysis. | All creation/Loader/delegate paths accept the input; null and teardown cases work; Fedora behavior and QML checks pass. |
| S6 | Accepted implementation slices | Review accumulated diff, remaining exceptions and validation coverage. Run final relevant regression suite and Fedora interaction smoke. | Accurate completion ledger, no unresolved regression, clean worktree after commits, and explicit remaining limitations. |

S1 and S2 are concrete implementation batches. S3/S4a/S5a are discovery and
contract work; do not invent a rewrite merely to produce code changes. Missing
test evidence is a task to collect through S0 and focused fixtures. Defer only
when a concrete unavailable environment, unresolved contract or lack of benefit
is documented after investigation.

## Implementation details and regression constraints

For S2, retain wheel delta thresholds, pixel-versus-angle choice, delay timer,
blocked-wheel state, current-index clamping, activation signal count and the
existing `accepted` behavior. Do not change ordinary parameter-free handlers:
`onClicked: { ... }` is not inherently obsolete. Prefer a named function
parameter here rather than introducing arrow-function `this` differences.

For S3, the preview sender's signals are declared in QML and discovered at
runtime; an ordinary C++ member-pointer connection is not a mechanical
replacement. The D-Bus connection API consumes a slot signature. Keep both
forms unless a separately justified adapter reduces a demonstrated problem.
Check existing return handling and receiver context without adding broad
fallbacks or changing helper startup/shutdown policy.

For S4/S5, inspect name shadowing before adding `root.` or `control.`. Delegate
roles and Plasma context objects are not necessarily properties of those ids.
Changing `var` to a QObject/value type can alter conversion and null handling;
adding `required` changes construction and delegate injection contracts. Keep
the original binding intact and preserve updates after Component.onCompleted.

Keep `QLatin1String`, supported import versions, raw pointers with guaranteed
lifetimes and necessary dynamic properties when there is no demonstrated
defect. No global formatter pass, warning suppression or automatic lint
baseline replacement belongs to this track.

## Fedora VM procedure

1. Read only the relevant SSH/VM configuration to resolve the user's VM.
   Record OS, Qt, Plasma, compiler versions, session type and active user.
   Confirm connectivity and the existing checkout before copying anything.
2. Save the relevant Dock config/layout files with original metadata and hashes.
   Record the installed executable and original source revision. Do not replace
   unknown VM edits or assume the installed binary matches the host checkout.
3. Use a dedicated VM checkout or an explicit source transfer that excludes
   `.git`, build products and user config. Record the source revision plus any
   uncommitted patch hash, so pre-commit VM validation is traceable. Preserve
   the baseline source for reinstalling after a regression.
4. Capture the active Plasma environment with
   `systemctl --user show-environment`. Apply its DISPLAY, WAYLAND_DISPLAY,
   XDG_SESSION_TYPE, XDG_RUNTIME_DIR, DBUS_SESSION_BUS_ADDRESS and optional
   XAUTHORITY exactly as described in the testing guide.
5. In the VM checkout run `bash install.sh --user Debug` with eight build jobs
   where supported. Capture the install result before stopping the old Dock.
   Source generated `~/.config/latte-dock-ng/dev-env.sh`; launch
   `~/.local/bin/latte-dock-ng --replace --debug --log-file /tmp/latte-ng.log`
   through a saved launcher with `setsid -f nohup`. Confirm `/proc/<pid>/exe`.
   Use the normal user prefix because KWin protocol authorization may reject a
   separately launched temporary-prefix executable.
6. Run the scenarios below and retain the user feedback or observed screenshots
   and event evidence. Then inspect the new log for warnings/errors. If a
   message family is suppressed by the global handler, ordinary log silence
   does not prove it absent; use temporary bounded pre-filter observation only
   for a concrete diagnostic gap, following A2's capture constraints.
7. Restore test settings and layouts, and verify their saved hashes where an
   exact restoration is intended. Stop temporary input tools. If a regression
   occurs, reinstall the known-good revision and preserve failure evidence.
   Fix the failing slice before beginning a dependent runtime change.

## Functional acceptance matrix

| Slice | Fedora scenario | Required observation |
| --- | --- | --- |
| S1 | Start Dock; render icons; invoke the touched shortcut and applet-alternative paths where available | Same visible results, no startup or connection errors. This can share S2's install session after separate compiler checks. |
| S2 ScrollArea | Scroll both directions, exercise small deltas and rapid repeated events; use a touchpad if available | Same thresholds, throttling, event forwarding and blocked-state reset as baseline. If hardware is absent, cover event logic in the actual component and record the hardware limitation. |
| S2 ComboBox | Scroll both mouse regions, first/last item, wheel-disabled state, open/closed popup and normal click selection | Same selected index, bounds, activation count and propagation to surrounding scroll areas. A runtime fixture may exercise the actual component if an in-app consumer is unavailable. |
| S3 preview | Temporarily enable the user's supported preview mode; activate and close a selected window, then restore the original hover action | Correct target UUID and window, helper remains responsive, title-tooltip fallback and disabled-preview setting remain intact. |
| S3 D-Bus | Read desktop wrapping setting; if safe in the test VM toggle and restore it while Dock runs | State notifications reach the existing receiver and wrapping behavior matches the setting. Do not restart the whole KWin session solely for this syntax review. |
| S4/S5 | Change each selected input after component creation, open/close its UI, then destroy/recreate it | Bindings continue updating; no new type, missing-property or lifetime error. |
| S6 | Task activation/minimize, hover mode, clock/tray/volume popups, settings, separator/spacer and drag order | No new observed regression relative to S0. Repeat broader scenarios only when a changed component affects them. |

Build affected targets using existing GCC and Clang presets with `-j8`; run
relevant existing CTest targets and QML checks using the testing guide. Add
behavioral fixtures only for concrete uncovered risks, using production
components rather than copying their algorithms. Final acceptance uses the
relevant full regression suite. A Fedora pass does not prove every supported
Plasma version or physical multi-display setup; preserve existing CI/distro
gates and report unavailable checks separately.

## Handoff and continuation

Create `docs/modernization-results/S<ID>.md` for each started slice. Record its
status, base and implementation revision, changed files, exact commands and
results, diagnostic additions/removals, VM source identity, scenarios and
feedback, restored settings, unresolved limitations and one next action.

| Batch | Status | Evidence / next action |
| --- | --- | --- |
| S0 | Complete | [Fedora environment and rollback baseline](modernization-results/S0.md); capture interaction-specific before state within S1/S2. |
| S1 | Not started | Replace four C++ spellings, build GCC/Clang and check relevant behavior. |
| S2 | Not started | Capture wheel/ComboBox before state, then change three handlers. |
| S3 | Not started | Wait for S2 acceptance. |
| S4a/S4b | Not started | Wait for S2 acceptance. |
| S5a/S5b | Not started / conditional | Wait for S3 and S4 evidence. |
| S6 | Not started | Wait for accepted implementation slices. |

Update individual rows or slice entries as work proceeds; do not mark deferred
work implemented.

Reusable implementation prompt:

> Implement S0, then S1 and S2 from docs/qt6-syntax-modernization-plan.md on the
> current authorized branch. Read AGENTS.md and the linked testing guide. Use
> the user's Fedora VM for functional verification, collecting missing evidence
> rather than deferring merely because a test has not yet been written. Preserve
> all interactions and compatibility contracts. Commit each completed batch
> automatically, record pending runtime feedback honestly, and leave an exact
> handoff. Continue later S batches only after their stated prerequisites pass.
