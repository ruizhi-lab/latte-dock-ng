# P6 scratch Release build feasibility

This preflight checks whether a local, isolated build measurement is possible;
it is not a P6 implementation or a runtime QML import benchmark. A fresh
`git archive` of `5e490945f8ff4c75e796d1f73521a15836e82af4` had SHA-256
`2126759b1c171991fd162b289ffc7b64a68d3f71c3cdc98feb759ac76e38aa0b`.
Source, Ninja build tree and install prefix were all under `/tmp`; the build
was not installed and no Dock or desktop session was started.

On the host with Qt 6.11.3, Plasma/libtaskmanager 6.7.6 and KF 6.31, the
Release configuration completed in 34.895 seconds, the full `-j8` build in
46.853 seconds, and an immediate no-change rebuild in 0.022 seconds. Each
command exited successfully; Python's monotonic clock measured whole command
wall time. These are single observations, not paired performance estimates.
[Commands and raw logs](P6-baseline-data/) record the local run.

The build had no compiler warnings or errors. QML type registration reported
that `PlasmaQuick::Dialog` could not be found as a base type. Configuration
also reported that the experiment's `FETCHCONTENT_FULLY_DISCONNECTED` variable
was unused; the project did not show a dependency download during the run.
Neither warning is claimed to be new or resolved.

The no-op rebuild is not a warm QML runtime import, and the full build cannot
attribute cost to the components module. A useful P6 decision still needs a
module-attributable cold/warm Release experiment and a controlled prototype
that preserves both installation roots and user override precedence. No
performance improvement is claimed.
