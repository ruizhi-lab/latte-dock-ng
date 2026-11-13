# T4g: permit automatic moves from returned locals

Baseline: `3cbbee3a8c400c31bed32c6e081030e39fce81fa`. R8c found six `performance-no-automatic-move` diagnostics across five source files. Each reported a local `const` value returned by value; constness forces a copy when the caller's return type is constructed from that local.

The patch removes only `const` from five locals covering those six return sites. Earlier reads remain unchanged, no explicit `std::move` was added, and all branch values and fallback behavior are preserved:

- `kPackageToolExecutable()` checks the `kpackagetool6` result for emptiness, returns it on success, and otherwise returns the `kpackagetool5` lookup. Its caller passes the resulting path to `QProcess` for indicator removal; no alias to the local escapes.
- `launcherUrlFromString()` inspects `parsed.scheme()` before returning the URL for `invokeLauncherOperation()`. The function takes the input by const reference, but `parsed` is an independent local value and no consumer retains it.
- `pluginIdFromMetaData()` uses `leaf` to detect metadata filenames and may return the directory name instead. Its results are copied into containment plugin maps/sets and local IDs; the metadata and local filename remain unchanged.
- `defaultApplication()` passes `terminal` to `serviceFromIdOrDesktopEntry()` for lookup, returning the configured value only when no service matches. The lookup takes a read-only value; the result is assigned to launcher task data, not aliased to the local.
- `LayoutManager::appletIconPath()` tests for a Qt resource prefix and returns a constructed `qrc` URL in that branch, otherwise returning `iconPath` to the QML `AppletItem`. The icon lookup result has no retained alias.

The negative fixture returns a const `std::string` local as `std::optional<std::string>` through two branches. The return type conversion prevents NRVO, and both clang-tidy 18.1.3 and 22.1.8 report the named check under production compile flags. The fixture uses only standard library types. Focused runs of the five changed translation units passed the new check on both tool versions. The check is registered in `.clang-tidy` and `GATED_CHECKS`; the full gate and four-configuration build/test matrix are recorded below.

## Coordinator acceptance

GCC and Clang Debug/Release builds completed with zero compiler warnings and
each passed 64/64 tests; no registered tests were removed. LLVM 18.1.3 and
22.1.8 rejected all 53 negative fixtures and found no gated findings in 156
production sources. Focused clazy 1.17 analysis of the five changed sources
was clean. The LLVM 18 scope audit recorded one diagnostic in an external
Plasma SDK header, the same outside-production diagnostic documented in T4f;
the existing filter was not changed. [Results](T4g-data/validation-summary.json)
and the [scope audit](T4g-data/tidy18-full-scope-audit.json) retain the evidence.

No dependency, minimum version, QML interface or persisted configuration
changed. No desktop restart or VM build was repeated for these value-return
changes. No CPU or memory improvement is claimed without measurement.

The preceding T4f source `d0ce8cf89` passed all 23 jobs in
[run 43440085083](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/43440085083).
That run does not validate T4g. The local source baseline additionally includes
the user's existing changelog commit `3cbbee3a8`.
