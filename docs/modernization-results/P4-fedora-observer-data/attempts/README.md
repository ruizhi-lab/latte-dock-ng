# Fedora P4 preflight attempts

The five attempts ran only in scratch HOME/XDG directories on the Fedora guest.
Each used `systemd-run --user --collect --wait --pipe` with
`PartOf=graphical-session.target` and `After=graphical-session.target`, then a
private `dbus-run-session`, virtual nested KWin, scratch Activity Manager,
staged Release dock, and synthetic Dolphin window. No real desktop layout or
configuration was changed.

The retained launchers and per-process logs are grouped by attempt. The tar
bundle is the original compact transfer from Fedora; extracted copies are
provided for direct review. Attempt 4's exact launcher SHA-256 is
`d186ac88234b978f26a24c5fbbd27efec528e9061b104f3065c7b7d9d3e34bcf` and is
the hash of the retained
[`attempt-4-launcher.sh`](attempt-4-launcher.sh), not the subsequently repaired
current launcher.
The generated scratch AppletItem source SHA-256 is
`66f297e51373742bb1e60f710fd273e48a12207b6c5324f6a7329a315e7aacea`; the
unchanged installed source SHA-256 is
`d4a98dcb6abaf3f03b45282344b39dd005b178234e7c582fc099a1939aa1ddaa`.

Attempts 2 through 4 fail inside the diagnostic QML instrumentation before
tracker rows can be evaluated. Attempt 4's generated logger still references
the nullable `applet.id`; the attempted string replacement ran before insertion
of those strings. No external observer verdict, A/A sample, or performance
claim is present in these records.

Attempt 5 used the locally tested generator and reached active loader IDs 3
and 4 with Dolphin tracked IDs, but no matching internal row. It exited 20
before the observer, so it is a workload no-go rather than performance data.
Its [launcher](attempt-5-launcher.sh), [generator](attempt-5-generator.py),
[runner log](attempt-5-runner.log), [status](attempt-5-status.txt) and generated
QML/process logs under `attempt-5-session/` are retained. The coordinator
verified the captured generated QML against the retained attempt-5 generator.
The current launcher's ordered cleanup change postdates attempt 5 and was
not run; the retained pipe warning does not prove a clean quit.
