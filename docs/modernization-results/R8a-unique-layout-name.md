# Layout name collision behavior

`Importer::uniqueLayoutName()` retains its public interface and delegates to
a small internal implementation. The importer still supplies its existing
layout-existence lookup. Suffix stripping, numbered candidate order, the
10,000 iteration bound and the final random hexadecimal fallback are unchanged.
The two callers continue to use the importer interface; this does not change
layout paths or persisted formats.

The importer logic test now calls the production naming implementation with
real files in `QTemporaryDir`. It checks a sequence of occupied names and an
existing numbered suffix. The test supplies its own directory lookup and does
not exercise the importer's XDG path wrapper or the random exhaustion case.
In isolated source copies, a reduced iteration bound and removal of suffix
stripping each caused the corresponding test to fail. Existing source
contracts for the bound and fallback were moved to the implementation and the
importer delegation is checked.

GCC and Clang Debug and Release builds passed with no compiler warnings, and
each configuration passed all 64 CTest targets. Clang-tidy 18.1.3 and 22.1.8
passed all 55 gated checks and fixtures over 157 production sources; the LLVM
18 run excluded one diagnostic outside first-party production scope. Focused
clazy passed for the importer and naming implementation. QML syntax passed for
243 files and module/import metadata checks passed. A deep QML baseline
comparison could not pass locally because the tool fingerprint differs from
the reviewed baseline; its diagnostics were not classified as new or existing.
The importer test links the project's existing Qt Gui dependency; no minimum
version or external dependency changed. Remote CI is pending.

On 2026-11-14, the user confirmed that creating layouts with duplicate names in
the settings dialog produced distinct names. That interaction uses the settings
controller's separate naming implementation, so it is a desktop smoke test and
does not directly exercise the importer helper. After the Debug restart, three
`libpng error: Read Error` lines appeared during startup, before the layout
interaction; the log did not identify an image file or stack. The naming code
does not read images, but the origin of those startup errors remains unverified
and should be investigated separately.
