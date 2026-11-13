# T4f: gate `bugprone-forward-declaration-namespace`

Source baseline: `main` commit `bbc8f678270e612495d74318b009eb5ddd3fe7b0`.
The current Clang Debug inventory reports three findings in three production
headers. A focused current-source scan selected 156 production sources and
found the same three declarations recorded by R8c.

## Findings and consumers

| Header | Unused declaration | Existing declaration/type with the same name |
| --- | --- | --- |
| `app/settings/settingsdialog/layoutscontroller.h` | `Latte::SettingsDialog` | `Ui::SettingsDialog`; the application dialog is `Latte::Settings::Dialog::SettingsDialog` |
| `app/settings/viewsdialog/viewscontroller.h` | `Latte::ViewsDialog` | `Ui::ViewsDialog`; the application dialog is `Latte::Settings::Dialog::ViewsDialog` |
| `app/view/view.h` | `Latte::GenericLayout` | The actual class is `Latte::Layout::GenericLayout` |

The two controller headers do not reference their outer-namespace dialog
declarations. Their implementation consumers use the corresponding handler and
controller types; the concrete windows live in `Settings::Dialog`, and their
generated form classes remain forward-declared in `Ui`. No `Latte::SettingsDialog`
or `Latte::ViewsDialog` consumer exists in the tracked code.

`view.h` includes `../layout/genericlayout.h`, which defines
`Latte::Layout::GenericLayout`. Its `layout` Q_PROPERTY, getter, setter and
member all name that type. The removed `Latte::GenericLayout` declaration was
not used. The actual class, include, property type, QML registration and module
URI remain unchanged. These are application-internal headers with no install
rule; this change removes no supported public C++ or QML type.

The implementation deletes only those three unused declarations. The negative
fixture defines `generated_ui::SettingsDialog` and adds an unused
`application::SettingsDialog` forward declaration. It uses custom namespaces
and ordinary C++ types, so it exercises the namespace mismatch without relying
on an SDK declaration.

## Fixture and consumer proof

Using the production Clang Debug compile database, clang-tidy 22.1.8 and
isolated clang-tidy 18.1.3 each enabled 52 names matching `GATED_CHECKS` and
rejected all 52 negative fixtures with their own named checks.

The focused `bugprone-forward-declaration-namespace` check was then run on
`layoutscontroller.cpp`, `viewscontroller.cpp` and `view.cpp`, which include the
three affected headers. All six runs (three translation units under each tool)
completed without a finding. The coordinator subsequently completed the broader validation below.

## Coordinator acceptance

All four GCC/Clang Debug/Release configurations built with zero compiler
warnings and passed 64/64 tests each, in a private D-Bus/offscreen environment.
The 64 registered tests and all 51 prior gated checks remain present.

The full clang-tidy runners on 22.1.8 and isolated 18.1.3 rejected all 52
fixtures under production flags and reported no findings in 156 production
sources. Four-worker timings were 75.702 and 107.894 seconds. Focused clazy
1.17 / LLVM 19.1.1 rechecked the three consumer translation units without a
finding or analysis error. The shared runner tests passed 22/22.

LLVM 18 also reported one diagnostic outside the production scope. A separate
post-build pass over all 156 sources located it at the system Plasma header
`/usr/include/Plasma/plasma/applet.h:45`: the SDK's `Package` forward declaration
has the same name as the project's `Latte::Package`. This is not an authored
project declaration. The existing `first_party()` scope already excludes SDK
paths; no filter or suppression was added. The gate's old summary called every
excluded finding generated/test code, so its wording now accurately says
outside first-party production scope. The archived original full-run log
retains its original wording; the [scope audit](T4f-data/tidy18-full-scope-audit.json)
records the external path and unchanged classification.

[Build/test evidence](T4f-data/validation-summary.json) and
[gate output](T4f-data/tidy18-final.log) retain the results. No dependencies or
minimum versions changed. No VM build or Dock restart was repeated for this
unused-declaration cleanup; the reported matrix is native host validation.
This slice is locally accepted; publication and new remote acceptance remain
separate from the preceding T4e CI run.

The preceding T4e source `bbc8f6782` passed all 23 jobs in
[run 43436056930](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/43436056930).
[Recorded acceptance](T4f-data/ci-bbc8f6782.json) covers that 51-check source,
not this new 52-check gate.
