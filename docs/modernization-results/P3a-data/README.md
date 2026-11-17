# P3a private Fedora probe

## Result

On 2026-11-17, a Fedora 45 private session proved that the temporary task QML
overlay loaded and that its warning markers reached the Dock log. The session
used a private HOME, XDG config/data/cache/state directories, runtime directory,
D-Bus session, and cache, plus a nested virtual KWin. It imported the existing
`two-trash.layout.latte` fixture, which starts the Dock at BottomEdge with a
Tasks applet.

The disposable containment overlay scheduled two calls through the same
`Positioner::setNextLocation()` API used by the Dock's BehaviorConfig edge
buttons:

| Request | Plasma enum value | Observed result |
| --- | ---: | --- |
| TopEdge | 3 | Driver request → `locationChanged value=3` → `refreshTaskLayoutPass(reason=locationChanged, pass=0)`; passes 1–8 followed. Geometry log and view map reported TopEdge. |
| BottomEdge | 4 | Driver request → `locationChanged value=4` → `refreshTaskLayoutPass(reason=locationChanged, pass=0)`; passes 1–8 followed. Geometry log and view map reported BottomEdge. |

The markers were emitted by temporary QML copies under
`/tmp/P3a-private-6qmA4G/session/home/.local/share/plasma/plasmoids/`. The
`P3A_SOURCE_LOADED` URL identified the task overlay's `main.qml`. The
containment driver markers confirm execution of its overlay. The matching
installed task and containment source hashes before and after the probe were:

```text
task main.qml:        7b62d4994622b12ab1f1bf5d96c3f660c4ea110746086ecc89535d6e9dea48b4
containment main.qml: a9b75ffa2810d57c6bed22970811c7fb2bd393abd848754782e721e0525bbe20
```

The successful runner output and raw logs were retained on the VM at
`/tmp/P3a-probe-run2.log` and
`/tmp/P3a-private-6qmA4G/session/{latte.log,latte.stdout,kwin.log,activity.log}`.
The transient systemd user unit exited successfully after 40.435 seconds and
cleaned up the nested KWin, Dock and activity service.

## Interpretation and limits

This was an isolated diagnostic route through `Positioner::setNextLocation()`;
it was not a manual click test of the edit UI. It proves that the selected QML
source loaded, that Qt warning messages reached `--log-file`, and that these
two real edge relocations notified the task applet and invoked the existing
refresh passes. It does not prove that KWin or TaskManager accepted geometry
for every live delegate, does not expose latest-generation readiness, and does
not justify reducing the immediate pass, eight-pass bound or fallback. No
performance measurement was collected.

The isolated logs include expected environment warnings: the private bus has
no desktop portal service; nested KWin reports unavailable PipeWire context,
unknown XKB symbols and Mesa EGL/Zink virtual-GPU setup warnings. The Dock log
also reports the unavailable portal service. These did not prevent the
transitions. The first runner attempt did not start the Dock because it copied
the fixture to a filename without Latte's required `.layout.latte` suffix; that
harness issue was fixed before the successful run. No normal desktop files or
system-installed files were touched.

## Reproduction

The tools are [`p3a-fedora-private-probe.sh`](tools/p3a-fedora-private-probe.sh)
and [`instrument_p3a_qml.py`](tools/instrument_p3a_qml.py). The runner requires
`P3A_PREFIX` to point at the isolated Fedora install and `P3A_LAYOUT` to point
at a disposable `.layout.latte` fixture containing the Tasks applet. It stages
private package overlays, uses `systemd-run --user` and a private D-Bus session,
then records source, edge-signal and pass markers in the private Dock log.
`bash -n`, Python AST parsing, and `qmllint` on temporary complete package
overlays passed; `qmllint` reported existing non-syntax warnings. The tools
contain no host credentials or private keys.
