# R6 LayoutManager restore index

Base: `main` at `9cf6c8059a83c069fdd882cf02ccbe29ccdb5d82`, with a
clean worktree and local and tracking branch references aligned at the start.

The R6 typed interface and insertion protocol are already in place. The
S5–S8 evaluations give little reason to expand the QML/C++ boundary to
optimize drag or hover. This follow-up removes repeat work from the
containment's startup restore path, which is distinct from those paths.

## Priorities

1. Close the LayoutManager integration checks and measure multi-dock
   startup in paired Release launches. The saved-order and late-item path
   can be measured separately from QML container creation.
2. Keep the P4 shared-query proposal conditional on repeatable query-work
   and CPU/PSS evidence. R5 already traced the large idle CPU observation to
   an incomplete session environment. P3a, P5 and P6 still have their own
   readiness or cost prerequisites.
3. For the next architecture-only change, review the proposed pure
   available-screen-area calculation in [the Corona design](R6-corona-design.md).
   The executable-hosted private app module crosses registration, packaging
   and host-test boundaries, with a higher integration cost.

## Change

Each restore attempt enumerates and resolves its live applets once. An
attempt-local ID index serves both the missing-Quick-item check and ordered
container creation. Previously, checking a saved ID repeated the whole
`plasmoidApplets()` traversal, and constructing the ordered list scanned
the applets again for each ID. The indexing and matching after enumeration
now use expected linear work in the live and stored list sizes. The cost of
normalizing the initial enumeration itself is separate.

The index expires before the next retry, preserving the chance for a late
Quick item to appear. The first object for a matching ID wins the saved
slot, remaining objects follow enumeration order, and saved legacy
splitter positions remain authoritative. No QML API or persisted format
changes. The existing persisted-map test now covers a late item, a live
order different from the saved order, and a live item absent from the saved
order.

This establishes reduced work by inspection. Startup latency, process CPU
and memory savings require paired Release measurements.

## Verification and handoff

- The GCC and Clang Debug and Release presets built with zero compiler
  warnings and errors. The same 68 CTest targets passed in each preset. A
  sandboxed run hung in the indicator QML smoke case. The host retry of that
  case and all four suites passed. The Debian 13 floor was not rebuilt.
- LLVM 23.1.6 clang-tidy rejected 56 negative fixtures and reported no
  findings across 157 production translation units. The local
  `clazy-standalone` executable was unavailable.
- `bash scripts/qmllint.sh` passed on 244 files. Deep lint generated a report
  and checked module metadata, but its comparison rejected the local tool
  fingerprint (local Qt 6.11.3, reviewed Qt 6.11.1).
- The user-mode Debug install completed. The transient user unit bound to
  the graphical session loaded the installed user-mode executable and
  containment plugin. The log reports identical applet order before and
  after restart on DP-1 (`43, 40, 41`) and HDMI-A-1 (`44, 46, 45`). The user
  confirmed both screens displayed the expected order. No restore warning,
  error or assertion appeared in the subsequent log review.
- If quantifying the startup effect, compare paired Release launches with
  the same multi-dock layout and separately time applet enumeration and
  container creation. The continuous hover and idle profiles do not measure
  this path.
- Main branch commit and push both require user authorization.
