# A/A run 1 — invalid before sampling

Status: **invalid**. The observer did not emit a final verdict, so the run
stopped before the 30-second settle period and before any sampling. No CPU or
memory measurements were produced.

The reviewed runner invoked the staged Release binary in a fresh private
session. The source archive, executable, fixture and install-manifest hashes
matched the frozen values. Process identity pointed to the staged executable,
and teardown completed with `dock-exited-after-SIGTERM`.

The observer runner returned 72 with `Observer exited without a final verdict;
no performance sample is valid.` Its Popen-captured JSONL contains only five
raw Mesa initialization messages and no parsed event records. Root's read-only
review of the user-unit journal found all 11 QML `P4_EVENT` records there,
including readiness and a complete `valid-sequence` result for the same
Dolphin window. Qt routed the observer messages to journald instead of the
runner pipe, so the Python runner could not accept that sequence. The journal
is retained as [`user-unit-journal.log`](user-unit-journal.log).

The same startup output also appears in KWin and Latte stderr:

- `MESA: error: ZINK: failed to choose pdev`
- `MESA-EGL: warning: egl: failed to create dri2 screen`

Those Mesa messages are retained context, not a proven cause of the runner
failure. The Dolphin output is empty. Run 1 remains invalid because the runner
returned 72; it produced no performance sample. No resource result is inferred.
The uploaded observer QML used `console.log` (SHA-256
`0522a4b2d6d272df8ec6bfb730c2846b8213a75a3c1207d58772d13a5f6f0265`). The
approved later `console.warn` change is a helper-version change and does not
retroactively make run 1 an accepted metric run.

The parent `ssh` invocation returned exit code 72. The transient unit reported
`Service runtime: 23.192s`, `CPU time consumed: 5.690s`, and `Memory peak:
504.9M`; these are launcher-level unit statistics, not dock metrics. No
`samples.csv` or post-run artifact hash file exists.

Compact retained evidence is in this directory: observer JSONL, KWin/Latte/
Dolphin/Activity Manager logs, process identity, frozen hashes, staged
QML/plugin/type hash list, and teardown marker. The remote run directory remains
at `/tmp/P4-fedora-20261116/aa-run-1/` for audit.
