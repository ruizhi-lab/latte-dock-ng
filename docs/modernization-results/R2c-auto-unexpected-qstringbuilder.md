# R2c-auto-unexpected-qstringbuilder: `auto` Kept a QStringBuilder

Status: Complete. CI run 40345972905 at `09b030e61` passed with the check on
the gate. No Qt 6.6 or older system is available for a desktop check; the
oldest supported stack here is Debian 13 with Qt 6.8.2.
Date: 2026-10-26. Branch: `claude/r2c-clazy-inventory-3a703c`. Start HEAD:
`771e74c3d`. Implementation commit: `eb83f088c`. Part of [R2c](R2c.md), the
first of its `R2c-<check>` slices.

## Defect

`BackgroundCache::BackgroundCache()` in `app/plasma/extended/backgroundcache.cpp`
declared

```cpp
const auto configFile = QStandardPaths::writableLocation(
                            QStandardPaths::GenericConfigLocation) +
                        QLatin1Char('/') + PLASMACONFIG;
```

The project defines `QT_USE_QSTRINGBUILDER`, so the sum is a `QStringBuilder`
and `auto` kept the builder instead of a `QString`. `KDirWatch::addFile()` then
converted it. Whether the builder's operands are still alive at that point
depends on the Qt version:

| Qt | Members of `QStringBuilder` | Effect of this line |
| --- | --- | --- |
| 6.4, 6.5, 6.6 | `const A &a; const B &b;` | The operands are the `QString` from `writableLocation()`, the `QLatin1Char`, the intermediate builder and the `QStringLiteral` string. All are temporaries destroyed at the end of the declaration, so the conversion read dead stack objects |
| 6.7 and later | `A a; B b;` with rvalue operands moved in | Harmless |

The members come from `src/corelib/text/qstringbuilder.h` in the qtbase `6.4` to
`6.8` branches; the installed Qt 6.11.1 header has the by-value form. The
declared floor is Qt 6.6 (`QT_MIN_VERSION`), so a supported system could hit
it. Debian 13 (Qt 6.8.2), the CI image (Qt 6.11.1) and the review host (Qt 6.11.2)
are not affected.
The result on an affected system is undefined: a wrong or empty watched path, or
a crash. No such report exists; the defect was found by clazy.

R2c first described this from the clazy message alone ("possible crash"). The Qt
headers and the sanitizer run below replace that with the version dependency.

## Reproduction

The same expression with `QT_USE_QSTRINGBUILDER`, g++ 13.3, `-fsanitize=address`:

```cpp
#include <QStandardPaths>
#include <QString>

int main()
{
    const auto configFile = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
        + QLatin1Char('/') + QStringLiteral("plasma-org.kde.plasma.desktop-appletsrc");

    const QString converted = configFile; // the operands are gone here on Qt <= 6.6
    return converted.isEmpty() ? 1 : 0;
}
```

| Qt | Result |
| --- | --- |
| 6.4.2 (`qt6-base-dev 6.4.2+dfsg-21.1build5`, Ubuntu 24.04 archive) | `AddressSanitizer: stack-use-after-scope`, read in `QConcatenable<QStringBuilder<QString, QLatin1Char>>::size()` called from `QStringBuilder::operator QString()` |
| 6.11.1 (`qt6-base-dev 6.11.1-0zneon+24.04+noble+release+build162`) | No report, exit 0 |

Qt 6.6 itself was not installed; its header declares the same reference members
as 6.4, so the 6.4 result stands for it.

## Fix

`const QString configFile = ...` in the same place. A comment at the site records
the trigger (`QT_USE_QSTRINGBUILDER`), the version dependency, the failure and
the gate that keeps `auto` out. Behavior on Qt 6.7 and later is unchanged: the
watched path is the same string.

The gate side is one gate entry and one fixture, as every R2c slice: the name in
`GATED_CHECKS` and `autotests/clazy/auto-unexpected-qstringbuilder.cpp`, which
holds the smallest violation (`const auto path = name() + QLatin1Char('/') +
name();`). The gate now has 31 entries and 31 fixtures.

## Evidence

Container commands are those of [R2c](R2c.md#reproducing-the-environment). The
host was busy with the GCC build during the three clazy runs, so their times are
longer than the 97 s of an idle run.

| Command | Result |
| --- | --- |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 8 --inventory auto-unexpected-qstringbuilder` | Exit 0 in 115 s: 0 findings in 153 sources (1 before the fix) |
| `test-clazy.py --build-dir build/modernization/clang-debug --jobs 8` | **Exit 0** in 159 s: 31 fixtures rejected by their own checks, 153 sources with no finding from 31 checks |
| Same, with the previous `backgroundcache.cpp` mounted over the fixed file | **Exit 1** in 152 s: `app/plasma/extended/backgroundcache.cpp:44:5: [auto-unexpected-qstringbuilder]` |
| Host `cmake --build --preset clang-debug` (Clang 22.1.8), `ctest --preset clang-debug` | Exit 0, 0 compiler warnings, 57/57 passed; `backgroundcache.cpp` was rebuilt in the application and in `corecomponentstest` |
| Host `cmake --preset gcc-debug`, `cmake --build --preset gcc-debug`, `ctest --preset gcc-debug` (GCC 16.2.1, fresh tree) | Exit 0, 0 compiler warnings, 57/57 passed |
| AddressSanitizer program above on Qt 6.4.2 and 6.11.1 | stack-use-after-scope and clean, as in the table |

The build logs hold thousands of `Warning:` lines from the qmllint step; they
are the known QML warnings tracked by the baseline gate, not compiler output,
and this change touches no QML.

## Handoff

- **Files and behavior changed:** `app/plasma/extended/backgroundcache.cpp` (one
  declaration and its comment), `scripts/test-clazy.py` (one name),
  `autotests/clazy/auto-unexpected-qstringbuilder.cpp`. No observable change on
  Qt 6.7 and later.
- **State authority / lifetime constraints:** the Qt version decides how long a
  `QStringBuilder` operand must live; the code now never keeps a builder, and
  the clazy gate rejects a new `auto` over a builder.
- **Tests:** no test added or changed; `ctest` 57/57 on Clang and GCC Debug. The
  fixture and the pre-fix run are the negative evidence.
- **CI:** run 40345972905 at `09b030e61` passed every job; the Clang Debug job
  ran the gate with this check. The Release presets and the desktop smoke ran
  later on `main` with [R2c fix batch 1](R2c.md#fix-batch-1).
- **Not run:** any run on a real Qt 6.6 system (none is available).
- **Next exact action:** none.
- **Commit approval status / push approval status:** committed on the feature
  branch at the user's request; merged into `main` in `8d1fe1568` and pushed
  with `09b030e61`.
