# R8b: typed inputs for the containment debug window

Status: Pending validation: the CI deep QML lint on the push, which compares the
reviewed 6.11.1 baseline. The same-host lint A/B, the install and the user's desktop
check of the window pass.

Batch / slice and selected scope: R8b, QML typing of one component,
`containment/package/contents/ui/debugger/DebugWindow.qml` (201 unqualified
diagnostics in the reviewed baseline), behaviour unchanged. The only other
production QML touched is the `Loader` in `containment/package/contents/ui/main.qml`
that creates the window; the reviewed baseline `docs/qmllint-baseline.json` drops
the file's entry.

Source baseline and start HEAD: `79881e5ac` (local and remote `main`). Branch
`claude/r8b-debugwindow-typing`, worktree
`.claude/worktrees/r8b-debugwindow-typing`. Commits `ea1dad3a4` (the change) and the
commit that adds this record.

## What the window read before

The window is created by a `Loader` in the containment's `main.qml` when the dock
runs with the hidden `--with-window` option. Its bindings found thirteen names
through the context of `main.qml`, which qmllint cannot see from the window's own
file. Counted with qmllint 6.11.3 on this host (the identities match the reviewed
CI baseline for this file, 201 unqualified accesses):

| Name | Accesses | Owner in main.qml |
|---|---:|---|
| `latteView` | 85 | `root.latteView`, alias of `Interfaces.view` (a `QObject` from private.app) |
| `space` | 66 | the window's own property, read from nested items without an id |
| `root` | 10 | the containment root (`ContainmentItem`) |
| `layouter` | 9 | `root.layouter`, `Ability.Layouter` |
| `metrics` | 8 | `root.metrics`, `Ability.Metrics` |
| `plasmoid` | 7 | `root.plasmoid`, `Plasma::Containment` |
| `background` | 5 | `root.background`, `Background.MultiLayered` |
| `animations` | 4 | `root.animations`, `Ability.Animations` |
| `layoutsContainer` | 2 | id of a `Layouts.LayoutsContainer` |
| `autosize` | 2 | `root.autosize`, `Ability.AutoSize` |
| `parabolic` | 1 | `root.parabolic`, `Ability.ParabolicEffect` |
| `visibilityManager` | 1 | id of a `VisibilityManager` |
| `indexer` | 1 | `root.indexer`, `Ability.Indexer` |

## Change

- The window has the id `debugWindow` and qualifies every access with it.
- Ten required inputs. Eight carry their QML type (`Ability.Animations`,
  `Ability.AutoSize`, `Background.MultiLayered`, `Ability.Indexer`,
  `Layouts.LayoutsContainer`, `Ability.Metrics`, `Ability.ParabolicEffect`,
  `ContainmentUi.VisibilityManager`), so qmllint now checks the members the window
  reads. Two stay `var`, as the comment beside them says: the containment root
  declares its properties in `main.qml`, which is not an importable type, and the
  layouter publishes its three layouts as `Item` (typing it would add nine
  missing-property identities).
- `latteView` and `plasmoid` are read-only bindings through `containmentRoot`, so
  they follow the root: `Interfaces` resolves the view after the containment loads
  (a 100 ms retry, up to 50 times) and clears it when the view is destroyed.
  Neither has a QML type name.
- `main.qml` sets the inputs with `Loader.setSource()` in
  `Component.onCompleted`. A first version set them in a nested
  `Debugger.DebugWindow { ... }` declaration; qmllint then reported every use of
  `root` inside the nested component (12 new unqualified accesses in `main.qml`),
  because the file is not `pragma ComponentBehavior: Bound`, and binding the whole
  file is not in this slice. The `import "./debugger" as Debugger` became unused
  and is removed.

State authority and lifetime: every input except the root is a child of the
containment root that lives as long as the root, so an initial value stays
current. The root is the authority for the view and the plasmoid. One timing
difference: the window is now created when `main.qml` completes, not while it is
being created.

## Evidence

Deep qmllint A/B on this host (Gentoo, qmllint 6.11.3), both runs with
`scripts/qmllint-deep.sh` against the same freshly built GCC Debug tree (the R8f
worktree's, whose QML files equal `main`'s), run by a Haiku sub-agent and checked by the
main session against the saved reports:

| Run | Tree | Files attempted | Complete | Diagnostics |
|---|---|---:|---|---:|
| before | `main` QML (`79881e5ac`) | 241 | yes | 7328 |
| after | this branch (`ea1dad3a4`) | 241 | yes | 7127 |

`scripts/qmllint-baseline.py compare --baseline before --current after`: exit 0,
`added: []`, `removed`: one identity, `unqualified` "Unqualified access" in
`DebugWindow.qml` with count 201. `main.qml` has the same 32 identities and counts in both
reports. Both runs exit 1 only at the final comparison with the committed baseline,
which fails closed with "tool fingerprint changed" (the reviewed baseline is qmllint
6.11.1 on Ubuntu 24.04, as `docs/qmllint-backlog-plan.md` requires); the CI QML lint job
makes that comparison on the push.

The committed baseline loses exactly that identity: comparing `main`'s
`docs/qmllint-baseline.json` with this branch's gives `added: []` and the same single
removal. The file's identity matched the baseline before the change: 201 unqualified
accesses both in the reviewed baseline and on this host.

Other checks:

- `scripts/qmllint.sh` (syntax gate, 241 files): exit 0, "qmllint: OK".
- `bash install.sh --user --jobs 8 Debug` from the worktree: exit 0, 0 `warning:` lines;
  the installed `DebugWindow.qml` and `main.qml` under
  `~/.local/share/plasma/plasmoids/org.kde.latte.containment/` are byte-identical to the
  branch (`cmp`).
- Launch for the desktop check, as AGENTS.md describes, with the hidden option added:
  `~/.local/bin/latte-dock-ng --replace --debug --with-window` (pid 35808). The log
  (`/tmp/latte-ng.log`, 324 lines a few seconds after start) has no line with "warning"
  or "error" and none naming the debug window, a required property, an assignment,
  `TypeError` or `ReferenceError`.

## Desktop check

The user checked the windows on the live desktop (Gentoo, Plasma 6.7.6, Wayland) and
reported every item as expected:

- one "#debugwindow#" window per dock;
- its rows filled with current values (screen, geometries, location, alignment,
  visibility, icon size, background length, the applets shown in the main layout);
- "Contains Mouse" follows the pointer over the dock;
- right-click on the dock → 编辑停靠栏 (&E)... switches "Edit Mode" on and back;
- the last active window row follows the focused window.

After the check, with the dock still running (pid 35808): the log has 398 lines and
none contains "warning", "error", "fatal", "assert", `TypeError`, `ReferenceError`,
"Unable to assign", "required property", "undefined" or "failed". The 74 lines
written after the first look at the log are debug messages of the window tracker,
the view ordering and the configuration view that edit mode opens. `coredumpctl list latte-dock-ng` still lists
106 entries, the count recorded before the launch.

The dock had loaded the context menu plugin of this install
(`~/.local/lib64/plugins/plasma/containmentactions/org.kde.latte.contextmenu.so`, per
`/proc/35808/maps`), built from a tree that contains R8e (`18489612e`, `50ba3074b`), so
the same right-click is the user check that R8e's record left open.

## Not verified

- No A/B against the old window on the desktop: the check shows that the new window
  reads live values, not that every row equals what the old window showed.
- The CI deep lint against the reviewed 6.11.1 baseline: it runs on the push.
- The Debian 13 container build: deferred by the user until the refactor is complete.
