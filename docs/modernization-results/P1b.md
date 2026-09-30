# P1b: Title-only fast path

Status: Implementation complete; runtime and performance acceptance pending.

Recorded main baseline: `e3ef1ddf001aa032cffa62db21cfd97196174746`.
Implementation base: P1a commit `ec8a504db`.

## Change

Wayland title changes now emit a dedicated `windowTitleChanged` signal. The
window tracker refreshes its cached `WindowInfoWrap` and emits the existing
`windowChanged` notification so active-window metadata remains fresh. It
schedules the normal debounced hint scan only if a non-display field changed.
The comparison includes identity, parent, geometry, validity, active/minimized/
maximized/fullscreen/shaded and stacking state, task eligibility, abilities,
application name, icon cache key, desktop and activity membership.

The title handler checks tracking membership both before and after
`requestInfo()`. The second check is required because `requestInfo()` can emit
`windowRemoved()` synchronously for blocked windows; the handler must not
reinsert an ID after that authoritative removal. Non-title changes continue
through the existing full path. A title event does not stop or restart pending
geometry work.

Focused tests cover title-only equality and representative geometry,
eligibility, desktop and icon changes. The source contract checks adapter-to-
tracker wiring, metadata notification, guarded removal and conditional hint
scheduling. It is a wiring check, not a live compositor dispatch test.

## Validation

- `cmake --build build-autotests-gcc -j8 --target wmunittest sourcecontracttest` — passed.
- `ctest --test-dir build-autotests-gcc --output-on-failure -R '^(sourcecontracttest|wmunittest)$'` — passed (2/2).
- `cmake --build build-autotests-clang -j8 --target latte-dock-ng sourcecontracttest wmunittest` — passed. The C++ build completed; build-aware QML lint emitted the repository's existing dynamic Plasma-interface and unqualified-access diagnostics.
- `ctest --test-dir build-autotests-clang --output-on-failure -R '^(sourcecontracttest|wmunittest)$'` — passed (2/2).
- `cmake --build build-autotests-gcc -j8 --target latte-dock-ng sourcecontracttest wmunittest` — application target blocked by GCC's `-Werror=sfinae-incomplete` diagnostic in the existing generated MOC aggregate at `app/view/view.h:69`; the same build did compile the focused GCC test targets. This diagnostic is outside the P1b diff and is recorded as a pending GCC full-application check.
- `git diff --check` — passed. `formatter.sh` was applied to only the changed C++ ranges and the diff was reviewed; its generated formatting around existing function definitions was normalized back to the surrounding file style.

## Pending acceptance

The predeclared target is zero hint schedules for ten title-only changes (ten
schedules in each of five P1a runs). No P1b live trace or five-pair Release
CPU/latency comparison was collected. SSH access could not be established from
this host: `ssh -o BatchMode=yes -o ConnectTimeout=5 fedora 'printf fedora-ready'`
failed with `Bad owner or permissions on /etc/ssh/ssh_config.d/20-systemd-ssh-proxy.conf`,
and `/home/ruizhi/.ssh/config` is absent. The application terminal is also not
attached to this task. Do not treat the P1a Debug counts as P1b results.

The full production-dispatch fake-adapter test and runtime state matrix remain
pending as well. The user has confirmed that Fedora task indicators and window
previews are restored and Debian task indicators are restored; these recovery
checks do not validate this change. The final manual desktop retest remains
scheduled after all implementation batches. No CPU, memory or wakeup saving is
claimed until the predeclared work target and comparable performance checks are
recorded.

## Handoff

Restore a working Fedora SSH route or an attached Plasma VM terminal, then
install the P1b user-mode Debug build with its generated `dev-env.sh`, trigger
the native Wayland title probe and record marker-only schedule/execute counts.
Verify state changes still schedule the full path and an already pending
geometry scan survives a title event. If those checks pass, collect five
same-sequence Release A/B pairs for CPU and latency. Also resolve the GCC
full-application warning gate independently before marking P1b complete.
