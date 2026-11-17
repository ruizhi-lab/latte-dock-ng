# R6 LayoutManager S1: fail loudly on root method contracts

Status: **Desktop validation complete; awaiting main-session review.** This is the first implementation slice of the
typed `LayoutManagerApi` design in [R6](R6-layoutmanager-design.md). The change
does not add the interface, alter QML or change the dispatch target. It adds
diagnostics at the existing meta-object boundary and keeps restore, repair and
fallback behavior intact.

## Change

`LayoutManager::onRootItemChanged()` now warns when the containment root lacks
`createAppletItem(QVariant)`, `createJustifySplitter()` or
`initAppletContainer(QVariant,QVariant)`. A missing method leaves an invalid
cached `QMetaMethod`; call sites skip that non-dispatchable method and retain
their existing null-result/failure handling. If a valid cached method's
`invoke()` returns false, the manager warns once per method in that root
generation. The warning set belongs to the manager and is cleared whenever the
root changes. `setRootItem()` listens for destruction of the QML-owned root,
clears the guarded pointer and emits `rootItemChanged()`; a generation check
prevents a stale destruction callback from clearing a replacement root's
cached methods. The `QMetaObject::Connection` is explicitly disconnected in
the manager destructor before `QObject` destroys any child roots; otherwise a
child root's `destroyed` signal could run a lambda after the derived manager
state has already been destroyed. This teardown hook is needed because
`QPointer` clearing does not itself emit the QML property's `rootItemChanged`
notification.

The call remains conditional on the existing Justify path. The method is
checked when a root is installed even though `createJustifySplitter()` is
currently reached only by the legacy Justify branch; a normal root supplies
the function and produces no new warning. No QML signatures, registered type,
persisted settings, retry limits, timers or ownership were changed.

## Production tests

`containmentlayoutunittest` now calls the real `LayoutManager` with QQuickItem
root stubs. It checks the three missing-method diagnostics, calls the
production restore path with a root missing `createAppletItem()`, repeats the
repair path for a root missing `initAppletContainer()`, and deletes a live root
before installing a replacement. The destruction case confirms that
`rootItemChanged()` is emitted, the pointer is cleared, and restore dispatches
through the replacement root. A manager-owned child root fixture confirms the
destruction connection is disconnected before QObject base teardown.

A valid method invocation returning false cannot be induced through the normal
single-threaded call path: dispatch uses the same live root whose meta-object
provided the cached method and supplies the matching `QVariant` arguments.
The de-duplication guard remains for that API failure result, but there is no
behavioral test for the false return. The earlier null-target fixture was
removed because it modeled stale state that the root-destruction callback now
prevents. The tested lifecycle boundary clears cached methods before another
operation can use the destroyed root.

The legacy Justify restore branch is disabled by `usesLegacyJustifySplitters()`
and is not changed by this slice beyond reporting the cached invocation result.
If re-enabled with a missing or failing `createJustifySplitter()`, its current
null result can still be dereferenced. Routing that failure through the
existing retry would also risk recreating applet containers already attached
earlier in the restore loop. This branch needs a separate design and runtime
validation before it is changed; S1 leaves both the null-dereference and
partial-retry risks unresolved.

## Verification

- GCC Debug: built `containmentlayoutunittest` and `pluginregistrationunittest`;
  `containmentlayoutunittest`, `sourcecontracttest` and
  `pluginregistrationunittest` passed.
- Clang Debug: built `containmentlayoutunittest` and
  `pluginregistrationunittest`; the same three CTest targets passed.
- `bash scripts/qmllint.sh`: passed all 244 QML files. This slice changes no
  QML.
- `git diff --check`: passed.
- The root-destruction, child-root teardown, replacement and missing
  `createAppletItem()` restore cases are covered by the production
  `containmentlayoutunittest`; the legacy splitter null-result path is not
  runtime exercised because the legacy branch is disabled and remains an
  unresolved risk.
- The full CTest suite, Release configurations, Debian 13 floor and desktop
  behavior have not been validated in this slice.

## Desktop validation and handoff

The user-mode Debug install completed and the installed binary was launched in
the graphical-session user manager using the workflow in `AGENTS.md`. The
first unprivileged install attempt could not clean a protected QML cache; the
authorized retry completed successfully. The transient unit
`latte-dock-ng-debug-20261117-215038` remained active with the user-local Debug
binary.

Desktop evidence from `/tmp/latte-ng.log`:

- Before restart, the user added System Monitor CPU Core (ID 47) and Weather
  (ID 48) to containment 12 on DP-2. The recorded order was
  `QList(43, 47, 48, 40, 41)`.
- After the user-requested restart, both IDs and the same order appeared in
  `initializationCompleted`, confirming restoration.
- The user then removed both components. The log records `appletRemoved` for
  ID 48 (`org.kde.plasma.weather`) and ID 47
  (`org.kde.plasma.systemmonitor.cpucore`); the final order is
  `QList(43, 40, 41)`.
- The Debug dock is still running. The log contains no Warning, Error, Fatal,
  ASSERT, missing-method or failed-removal entries during this sequence.

This validates ordinary root method availability and the add, restart restore,
and remove path on this desktop. Other desktop scenarios were not exercised.
The full CTest suite, Release configurations, Debian 13 dependency floor, and
the disabled legacy splitter's missing-method failure path remain unrun. The legacy branch's null-result dereference and
partial-retry risk remain explicitly unresolved; this slice did not change its
fallback or retry behavior. Build-time QML lint may report pre-existing QML
warnings; the clean-log statement above refers to runtime dock diagnostics.

No commit or push was made in this handoff because the main session requested
review first. The user previously authorized automatic commit and push after
each task completes; this authorization remains in effect once the main-session
review is complete.
