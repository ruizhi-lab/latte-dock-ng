# Qt test API compatibility: 2026-11-13

Source baseline: `64a7f1ae5dbccf5e23c88c8e88816b701f92930a`.
The user supplied Arch (Plasma 6.8) and Debian 13 (Plasma 6.3) VMs for genuine
SDK validation. Existing VM source snapshots and Dock installations were not
reused or changed; fresh source/build trees were used under `/tmp`.
Arch lacked compiler tooling, so clang, clang-libs, LLVM and llvm-lit 23.1.4
were installed for validation. The mirror override used only temporary files;
`/etc/pacman.conf` and the system mirror list were unchanged.

## API repairs

`settingsviewunittest.cpp` used `QTestAccessibility::containsEventOfType()`,
which the Debian Qt 6.8.2 header does not provide. The test now checks the
existing event list for the same event types. The helper remains guarded by
`QT_CONFIG(accessibility)` and does not skip or relax the selection assertions.

Qt 6.12 deprecates the `QPoint` overload of `QDragEnterEvent`, while Qt 6.8.2
only has that overload. A dependent constructor-capability probe selects the
available API without raising the dependency floor or disabling warnings.

Accessibility is activated and verified after window exposure, immediately
before selection. The authoritative state is `QAccessible::isActive()`; mapping
the window can change platform activation. Qt 6.8.2 offscreen still refuses
activation, while the actual Debian Wayland session accepts it and passes the
focused test. A failed platform precondition remains visible as a failure.

## Validation boundaries

Host GCC/Clang Debug/Release builds have zero compiler warnings and each passes
64/64 tests. Arch libplasma 6.8.2 / Qt 6.12.0 builds with GCC 16.2.1 and Clang
23.1.4 have zero compiler warnings; both Debug suites pass 64/64 offscreen.

Debian libplasma 6.3.5 / Qt 6.8.2 builds with GCC 14.2.0 and Clang 19.1.7
have zero compiler warnings. Both final offscreen suites pass 63/64; the
remaining `settingsviewunittest` requires the actual accessibility-capable
session and passes there under both compilers. The original suites passed
62/64 because `persistentmenutest` used insufficient pointer movement for the
accidental-click guard in Qt 6.8. A plain-QMenu / PersistentMenu A/B showed that
both reject the original input and accept a deliberate nine-move row traversal.
The fixture now uses that traversal and retains all toggle/visibility assertions.
The adjusted menu test also passes in all four host configurations, with zero
compiler warnings. The initial failures are not counted as offscreen passes;
[final VM focused results](T4c-data/debian/README.md) record the real-session
settings test and corrected menu test passing with both compilers.
The SDK-dependent QML tooling warnings are not a comparable deep baseline and
are not included in the zero C++ compiler-warning claim.

No dependency change, install, Dock restart or persisted user-config change
is required by these test-only API repairs.
