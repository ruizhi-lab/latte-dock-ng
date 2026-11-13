# T4c: gate `bugprone-parent-virtual-call`

Source baseline: `main` commit `64a7f1ae5dbccf5e23c88c8e88816b701f92930a`.
The slice adds the check as the forty-ninth named clang-tidy check, with one
three-level negative fixture. `ViewsTableView::selectionChanged()` already calls
its immediate `QTableView` base after the earlier R8c fix.

## Intentional `View::resizeEvent()` ancestor call

The actual guarded source was analyzed on Arch Linux against libplasma 6.8.2 and
Qt 6.12.0 using clang-tidy 23.1.4 and the fresh Clang Debug compile database.
The Plasma 6.8 version header enabled `LATTE_PLASMA_STORED_CONTAINMENT_SCREEN`
naturally; no feature macro was forced. The check reported
`app/view/view.cpp:1801`:

> qualified name `QQuickWindow::resizeEvent` refers to a member overridden in
> subclass; did you mean `PlasmaQuick::ContainmentView`?

This call is intentional only while `rootObject()` is null. In libplasma 6.8,
`ContainmentView::resizeEvent()` resizes the root item without checking for a
null root; Latte creates and positions the platform window before loading the
view QML. Calling the `QQuickWindow` handler avoids that startup dereference.
The line has a check-specific `NOLINT(bugprone-parent-virtual-call)`. Remove it with the bypass
once every supported libplasma version has the missing root guard; the existing
root-null branch and call should then be reviewed against that API change.

## Fixture proof

`autotests/clang-tidy/bugprone-parent-virtual-call.cpp` contains a virtual method
overridden in an intermediate class and again in a derived class. The derived
override calls the grandparent implementation, bypassing the intermediate
override. The fixture was compiled with the existing production Clang Debug
flags and rejected as an error by the named check with host clang-tidy 22.1.8,
isolated clang-tidy 18.1.3, and Arch clang-tidy 23.1.4.

The gate configuration and runner list retain the previous forty-eight checks
and add this one in alphabetical order. The change adds no runtime behavior.
Both host full gates passed: 49 fixtures were rejected and 156 production
sources had no findings on clang-tidy 22.1.8 and isolated 18.1.3. All four
GCC/Clang Debug/Release builds had zero compiler warnings and passed 64/64
tests, including the Qt test API compatibility adapter.
[Recorded results](T4c-data/validation-summary.json).
Arch and Debian VM results remain distinct from this host proof.

On the Arch VM, libplasma 6.8.2 and Qt 6.12.0 enabled the guarded code from the
installed version header. GCC 16.2.1 and clang 23.1.4 Debug builds completed all
targets with zero compiler warning lines, and full CTest passed 64/64 under each
compiler in a private D-Bus/offscreen session with isolated XDG directories.
After the final Qt 6.8 test updates, both `persistentmenutest` and
`settingsviewunittest` were rebuilt and passed again (2/2) under each compiler.
The focused `View::resizeEvent()` check and the three-level fixture behaved as
described above with clang-tidy 23.1.4. Its complete runner could not start
fixture analysis because this tool deprecates the previously gated
`performance-faster-string-find` check; the repository's `WarningsAsErrors: '*'`
promotes that configuration diagnostic to an error. The same Arch Clang Debug
database passed the complete 49-check runner with the isolated CI-version
clang-tidy 18.1.3 wrapper: 49/49 fixtures rejected and no findings across 156
production sources. See the [Arch tool-version evidence](T4c-data/arch-clang-tidy23-fixture-setup.txt).
The existing VM checkout and settings were left untouched; validation used a
staged `/tmp` source and build tree.
