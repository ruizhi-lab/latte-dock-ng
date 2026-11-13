# T4i Arch validation

The isolated Arch VM uses Plasma 6.8.2 and Qt 6.12.0. The Clang Debug build of
`lattecoreplugin` and `coreunittest` completed without compiler warnings;
the QML module metadata check passed. [Build output](build.log) is retained.

The existing core test executed the new popup centering cases successfully:
13 passed, zero failed and one unrelated desktop-only icon watcher skipped.
See [test output](coretest.log). LLVM 18.1.3 clang-tidy rejected all 55
negative fixtures and found no gated findings in 156 production sources;
one finding outside the first-party scope was excluded. See [gate output](tidy.log).

The test source and build were isolated under `/tmp`; no dock was installed or
restarted. Actual Wayland popup placement was not visually re-observed. The
helper uses existing Qt Core facilities and adds no dependency.
