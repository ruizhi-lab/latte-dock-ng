# Task Preview Selection Lifetime

Status: Complete. Fedora interaction retest completed without the reported warning.
Date: 2026-10-01. Base: `bb4a0fc06`.

## Observed failure

The Fedora retest log reported an invalid QML context followed by
`main.qml:247: TypeError: Property 'moveIsolatedPreview' ... is not a function`.
The root retains the selected TaskItem and calls its QML method each frame.
The task previously did not release that selection on ListView removal or
component destruction. QObject existence does not guarantee that its QML
context is still valid.

## Fix

The delegate stops its delayed preview request and clears the root selection
at removal and destruction, only if it still owns that selection. The delayed
and immediate hover paths reject invalid model indices and removing tasks.
The root frame handler rejects a disabled preview or removing task before
invoking its geometry method. Local comments describe the lifetime boundary;
source contracts protect release ordering and selection ownership.

## Verification

GCC and Clang Debug `latte-autotests` builds completed without compiler
warnings. All 46 CTest targets passed with each compiler: 45 inside the
sandbox, followed by the isolated D-Bus target outside it because the sandbox
blocked socket creation. These source contracts do not prove live QML teardown
behavior. Fedora user-mode installation and hover/close retest are required
before accepting and committing the runtime fix.

Fedora `bash install.sh --user Debug --jobs 8` completed. Syntax checks for
both changed QML files passed. The user confirmed the desktop retest. The
user-mode Debug Dock remained running as PID 35843 from
`/home/fedora/.local/bin/latte-dock-ng`; the new `/tmp/latte-ng.log` has no
invalid-context, `moveIsolatedPreview`, warning, error, fatal or assertion
entries. The earlier log is preserved as `/tmp/latte-preview-lifetime-before.log`
in the VM.
