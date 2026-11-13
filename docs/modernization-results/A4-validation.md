# A4 Mapping Test Fixture Investigation

Status: A4-1 design investigation complete. This record describes the initial
investigation, which changed no production source. The coordinator subsequently
selected candidate 1; implementation and current validation are tracked in
[A4-resolver](A4-resolver.md). A4 remains Pending validation for the full chain.
Date: 2026-11-13.

## Production chain and existing coverage

`Corona::screenForContainment()` first walks nested applet containments to the
outer containment. It then calls `Layouts::Synchronizer::screenForContainment()`
and uses Plasma's persisted last screen when no layout returns a nonnegative
screen. The synchronizer searches its active `CentralLayout` objects for the
containing layout. `GenericLayout::screenForContainment()` resolves pending
updates first (including `onPrimary`), then live or waiting views through
`ScreenPool` connector identity, and finally the persisted last screen.

The current `corecomponentstest` target covers `ScreenPool` mapping and primary
fallback directly, but compiles none of `Corona`, `Synchronizer`, or
`GenericLayout`. `importedcontainmentsunittest` demonstrates construction of a
lightweight `Plasma::Corona` and real `Plasma::Containment` objects, but does
not exercise Latte's screen mapping chain. The R7 desktop report covers
two-output hotplug behavior; it does not prove pending-update precedence,
primary-not-zero, or nested containment behavior through the full chain.

The chain cannot be assembled from existing public test APIs. The pending
`ViewsTable` is private to `GenericLayout`; `Synchronizer`'s active layout list
is private and populated by layout initialization; `Corona` constructs its
runtime services and performs Wayland, package, QML, and activity integration
during startup. Constructing `Corona` solely for these assertions would make
the autotest dependent on the user's desktop session and configuration unless
the runtime is isolated and fully initialized.

## Dedicated app-runtime target assessment

A dedicated `screenmappingruntimeunittest` could copy the `latte-dock-ng`
executable's sources, include paths, definitions, options, and link libraries,
filter out `main.cpp`, and add a QtTest entry point. This would compile the
actual production classes and allow a fixture to use real Plasma containments.
It would also create a second app-sized target: the executable source graph
contains more than 150 translation units, generated sources, and target-specific
settings. Maintaining a parallel source/property clone for a handful of mapping
cases would impose a large build and integration cost and could silently drift
from the executable. This design is rejected for the current A4 scope.

No friend accessors, test hooks, or copied mapping algorithms were added. A
standalone `GenericLayout` test with a friend fixture could cover local branches,
but it would not establish the Corona-to-Synchronizer path that A4 is intended
to validate.

## Candidate designs for separate review

1. **Testable production mapping resolver with integration guards.** Extract
   the deterministic mapping decision into a production unit with explicit
   inputs for pending updates, view connector, primary screen ID, containment
   membership, and persisted last screen. Keep thin production guards around
   QObject ownership, nested applet traversal, and ScreenPool access. Unit tests
   can exercise decision precedence without copying the algorithm; a focused
   integration check must still prove that the guards feed authoritative live
   state into the resolver. Review whether this boundary preserves the intended
   ownership and synchronization semantics before implementation.
2. **Reusable app object target.** Refactor app source ownership into a reusable
   object/library target consumed by both the executable and a runtime test.
   This avoids duplicating the source graph but changes the main build boundary
   and requires auditing generated sources, exported symbols, target options,
   QML host dependencies, and initialization behavior. Review it as a broader
   architecture/build change, not as a local autotest edit.

## Decision and next action

A4 remains Pending validation. A4-1 established why the current test targets
cannot reach the production chain and rejected full source-graph duplication as
disproportionate. A4-2 initially required selecting and reviewing a testability
design. The coordinator has now selected and implemented the bounded production
resolver; see [A4-resolver](A4-resolver.md) for tests and desktop acceptance.
Full-chain primary-not-zero and nested containment coverage remain unproven;
existing R7 hotplug observations remain separate desktop evidence.
