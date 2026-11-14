# Layout scanner contract scope

The layout scanner source contract now examines the bodies of
`Importer::availableLayouts()` and `Synchronizer::initLayouts()` separately.
Previously the importer check searched the whole source file, where the same
filter expression also appears in another function; a regression in the
intended scanner could therefore go unnoticed.

The actual C++ source-contract test passed against an isolated baseline. In
separate temporary source trees, replacing `QDir::Files` with
`QDir::AllEntries` in each target function made its own assertion fail. The
production working tree was not mutated. GCC and Clang Debug builds and focused
CTest passed. This is a source-boundary check, not a live directory enumeration
test. No production behavior changed; remote CI is pending.
