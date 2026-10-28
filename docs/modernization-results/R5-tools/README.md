# R5 tools

Scripts used for [R5](../R5.md). They were written for one task in one Fedora 44 VM
(user `fedora`, work directory `~/latte-r5` in the guest) and are kept so that the
numbers can be reproduced or challenged; they are not general tooling and several
paths and the layout name are specific to that guest. "Guest" scripts run inside the
VM, "host" scripts on the developer machine.

| Script | Where | Purpose |
| --- | --- | --- |
| `r5-scenario.py` | guest | Runner: N independent dock starts per scenario with the P0 collector and the per-thread sampler over the same window; launchers `systemd` and `p0-ssh`, stimuli, temporary layout edits, KWin probe, strip screenshots |
| `r5-thread-sampler.py` | guest | Per-thread CPU and timeslices from `schedstat`, machine totals, top processes |
| `kwin-probe.js`, `kwin-probe.sh` | guest | Read-only KWin script: pointer position and window classes and geometry, reported over D-Bus |
| `launch-dock.sh`, `launch-p0style.sh` | guest | The session launcher and the launcher pattern P0 used from SSH |
| `r5-stim.sh` | guest | Stimuli: Zenity windows, title churn, KSycoca storm, one rebuild, attention, ksmserver stall |
| `r5-sycoca-watch.py` | guest | Records every `kbuildsycoca6` process (parent, environment) and the mtimes of the sycoca files |
| `build-only.sh`, `build-install.sh`, `swap-install.sh`, `build-all.sh` (inline in the batches) | guest | Build a revision with its own `install.sh` steps, install a built revision after uninstalling the current one |
| `ab-batch.sh`, `stim-batch.sh`, `phase2-batch.sh` … `phase8-batch.sh`, `stack-storm.sh` | guest | The unattended batches in the order they ran (A/B idle series, stimuli, applet bisect, variants, environment bisect, frame and QML diagnostics, stack samples) |
| `qml-profile.sh`, `gdb-sampler.py`, `stack-sample.sh`, `eu-stack-sample.sh`, `launch-for-sampling.py`, `stop-dock.sh`, `dbus-rate.sh` | guest | Diagnostics helpers (qmlprofiler in attach mode; gdb sampling, which froze the dock and is kept as a negative result; eu-stack sampling) |
| `host-monitor.py` | host | Host load, CPU, steal and the CPU of the guest's QEMU process every 5 s |
| `host-pointer.sh`, `host-sweep.py`, `host-hover-run.sh` | host | Place or sweep the guest pointer through the QEMU absolute tablet (the sweep scripts were prepared but not used for a result) |
| `r5-summarize.py`, `r5-tables.py`, `r5-trim-host.py`, `r5-stacks-summary.py`, `r5-qtd-summary.py`, `r5-wayland-frames.py` | host | Tables and summaries from the raw records |
| `r5-collect.sh` | host | Copies the raw records into `../R5-data` and writes `SHA256SUMS` |

`../scripts/measure-process-family.py` (P0's collector, unchanged) is copied to the
guest next to these scripts. Never launch the dock from a bare SSH shell for a
measurement; see `docs/development-testing-guide.md` and R5.
