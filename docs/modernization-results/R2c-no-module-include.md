# R2c-no-module-include: No Qt Module Headers in Includes

Status: Pending validation. Merged into `main` in `1dd485bc1`; the checks on
`main`, including a Debian 13 build on Qt 6.8.2, and the combined desktop smoke
passed ([R2c fix batch 1](R2c.md#fix-batch-1)). The CI run waits for a push.
Date: 2026-10-27. Branch: `claude/r2c-batch-a-a5a9fa`. Start HEAD: `e9bf4acf5`
(the `R2c-range-loop-reference` commit on top of `4a08e472e` and `09b030e61`).
Implementation commit: the commit that adds this report. Part of
[R2c](R2c.md); the third slice of batch A.

## Defect

14 files included a whole Qt module header, `#include <QtQml>` in 11 headers and
`#include <QtDBus>` in 3 sources. clazy reports "Module QtQml should not be
included directly". A module header pulls in every public header of the module
and of the modules it depends on: in Qt 6.11 `QtQml` includes `QtCore`,
`QtQmlIntegration`, `QtNetwork` and 35 QtQml headers, and `QtDBus` includes
`QtCore` and 20 D-Bus headers. Each file used a handful of them, so every
translation unit that included one of these headers parsed the whole set, and
none of the files said which Qt classes it depends on.

There is no functional defect and no known symptom; the finding is a build cost
and include hygiene finding.

## Fix

- **11 headers.** `#include <QtQml>` became `#include <QtQml/qqmlregistration.h>`.
  Every one of them used the module for the registration macros only
  (`QML_NAMED_ELEMENT`, plus `QML_SINGLETON` or `QML_UNCREATABLE`), which this
  header defines. It exists in every Qt 6 release (the floor is Qt 6.6) and the
  repository already used it in `plasmoid/plugin/windowviewbackend.h` and
  `plasmoid/plugin/previewprocess.h`.
- **3 sources.** `#include <QtDBus>` became the headers of the D-Bus classes the
  file names:

| File | Headers |
| --- | --- |
| `app/plasma/extended/screengeometries.cpp` | `QDBusConnection`, `QDBusMessage`, `QDBusMetaType`, `QDBusPendingCallWatcher`, `QDBusServiceWatcher` |
| `app/wm/abstractwindowinterface.cpp` | `QDBusConnection`, `QDBusMessage`, `QDBusPendingCallWatcher`, `QDBusServiceWatcher`, `QDBusVariant` |
| `containmentactions/contextmenu/menu.cpp` | `QDBusConnection`, `QDBusInterface`, `QDBusMessage` |

The 11 headers are `app/declarativeimports/contextmenulayerquickitem.h`,
`app/declarativeimports/interfaces.h`, `app/plasma/extended/backgroundtracker.h`,
`containment/plugin/layoutmanager.h`, `containment/plugin/types.h`,
`declarativeimports/core/dialog.h`, `declarativeimports/core/environment.h`,
`declarativeimports/core/quickwindowsystem.h`, `declarativeimports/core/tools.h`,
`plasmoid/plugin/contextmenuactionsbackend.h` and `plasmoid/plugin/types.h`.

Each header replacement keeps its line, so no line number in the QML metadata
moved. The module headers also carried all of `QtCore`, so the change could have
exposed files that used a Qt class without including it. It did not:
GCC and Clang built every target and test without any other include being
added, and a syntax-only pass of the 342 non-generated translation units of the
GCC database against the edited tree reported no error before the real builds
ran. No file outside the 14 changed.

The gate side is one gate entry and one fixture, as in every R2c slice: the name
in `GATED_CHECKS` and `autotests/clazy/no-module-include.cpp`, which holds the
smallest violation (`#include <QtDBus>`). The gate has 34 entries and 34 fixtures
on this branch.

## Effect

Preprocessed size of translation units that include one of the headers, with the
project flags (`g++ 16.2.1 -E -H`), on an export of `09b030e61` against the tree
after this change:

| Translation unit | Header inclusions | Preprocessed lines |
| --- | --- | --- |
| `declarativeimports/core/tools.cpp` | 1162 to 617 (-47%) | 286 698 to 157 230 (-45%) |
| `declarativeimports/core/quickwindowsystem.cpp` | 1152 to 634 (-45%) | 286 018 to 167 943 (-41%) |
| `declarativeimports/core/environment.cpp` | 1262 to 816 (-35%) | 296 004 to 194 282 (-34%) |
| `plasmoid/plugin/types.cpp` | 1151 to 638 (-45%) | 286 009 to 169 286 (-41%) |
| `plasmoid/plugin/contextmenuactionsbackend.cpp` | 1311 to 846 (-35%) | 296 862 to 192 472 (-35%) |
| `containment/plugin/layoutmanager.cpp` | 1308 to 877 (-33%) | 303 166 to 203 999 (-33%) |
| `app/plasma/extended/backgroundtracker.cpp` | 1257 to 809 (-36%) | 296 540 to 194 565 (-34%) |
| `app/wm/abstractwindowinterface.cpp` | 1384 to 1107 (-20%) | 302 529 to 222 636 (-26%) |

The eight units are a sample of the 14 files and the translation units that
include their headers. Preprocessing is only part of a compile and the host was
shared with another build, so no wall-clock saving is claimed.

## Behavior check

The QML types must still register as before, so the generated registration
files of the four QML plugins (`*_qmltyperegistrations.cpp`, `*_metatypes.json`,
`*.qmltypes` and `qmldir`; 20 files) were copied from the Clang Debug tree before
this change and compared with the tree rebuilt after it: 0 of 20 differ. The
four `*_metatypes.json.gen` files that the rebuild regenerated after the edit
equal the earlier `*_metatypes.json` byte for byte, so the registrar input did
not change, and the generated registration sources, `.qmltypes` and `qmldir`
files are identical too.

## Evidence

Container commands are those of [R2c](R2c.md#reproducing-the-environment) with
the checkout mounted read-only, a build tree of its own and `-j6`/`--jobs 6`,
because a second session builds at the same time.

| Command | Result |
| --- | --- |
| Baseline `test-clazy.py --build-dir build/modernization/clang-debug --jobs 6 --inventory qfileinfo-exists,range-loop-reference,no-module-include` on an export of `09b030e61` | Exit 0 in 161 s: `no-module-include` 14 findings in 14 files |
| `test-clazy.py ... --inventory no-module-include` on the fixed tree | Exit 0 in 161 s: **0 findings** in 153 sources |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 6` | **Exit 0** in 164 s: 34 fixtures rejected by their own checks, 153 sources with no finding from 34 checks |
| Same, with the previous `declarativeimports/core/tools.h` and `app/wm/abstractwindowinterface.cpp` mounted over the fixed files | **Exit 1** in 149 s: `[no-module-include]` at `app/wm/abstractwindowinterface.cpp:18` and `declarativeimports/core/tools.h:12`, nothing else |
| Container `cmake --build ... -j6` (Clang 18.1.3, Qt 6.11.1, strict warnings) | Exit 0 in 74 s, 61 objects rebuilt, 0 compiler warnings |
| Host `cmake --build --preset gcc-debug -j6`, `ctest --preset gcc-debug` (GCC 16.2.1) | Exit 0 in 137 s (configure, incremental build, ctest), 89 objects rebuilt, 0 compiler warnings, 57/57 passed |
| Host `cmake --build --preset clang-debug -j6`, `ctest --preset clang-debug` (Clang 22.1.8) | Exit 0 in 164 s (configure, incremental build, ctest), 89 objects rebuilt, 0 compiler warnings, 57/57 passed |
| Syntax-only pass (`g++ -fsyntax-only`) of the 342 non-generated translation units of the GCC database against a scratch copy with these edits | 0 failures; run before the real builds |

The build logs hold thousands of `Warning:` lines from the qmllint step; they
are the known QML warnings tracked by the baseline gate, not compiler output,
and this change touches no QML.

## Handoff

- **Files and behavior changed:** the 14 files above (one include line each,
  five lines for `screengeometries.cpp` and `abstractwindowinterface.cpp`, three
  for `menu.cpp`), `scripts/test-clazy.py` (one name),
  `autotests/clazy/no-module-include.cpp`. No observable change; the QML
  registration artifacts are identical.
- **Include contracts:** the headers declare the registration macros through
  `<QtQml/qqmlregistration.h>`, and the sources name the D-Bus classes they
  use. A file that starts using another QtQml or QtCore class includes that
  class's header; the module headers stay out and the gate rejects them. No
  dependency changed: every file already built against Qt6::Qml or Qt6::DBus.
- **Tests:** no test added or changed; `ctest` 57/57 on GCC and Clang Debug.
  The fixture and the mutation run are the negative evidence.
- **Combined validation:** on `main` at `fc8b803d0`, GCC and Clang Debug and
  Release with 0 warnings and 57/57 tests each, the 37-check gate and the
  desktop smoke passed; see [R2c fix batch 1](R2c.md#fix-batch-1). The Debian 13
  container (`verify-install.sh debian13`: GCC 14.2.0, Qt 6.8.2, KF 6.13.0,
  Plasma 6.3) built, installed and uninstalled both modes with 0 compiler
  warnings, the first build of these includes on a Qt older than 6.11. The
  D-Bus screen geometry publication in `screengeometries.cpp` logged its
  rectangles on the host and in the Fedora VM.
- **Not run:** the CI job (`main` is not pushed), and a build against Qt 6.6,
  6.7, 6.9 or 6.10 (the registration header and the D-Bus headers exist in all
  of them; the repository already includes `<QtQml/qqmlregistration.h>`
  elsewhere).
- **Next exact action:** after a push is approved, confirm the CI run and mark
  this slice Complete.
- **Commit approval status / push approval status:** committed on the feature
  branch; merged into `main` in `1dd485bc1` with the user's approval; not
  pushed.
