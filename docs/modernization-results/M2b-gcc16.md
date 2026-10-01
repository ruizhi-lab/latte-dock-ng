# M2b-GCC16: Qt incomplete-SFINAE warning

Date: 2026-09-30

Branch: `codex/modernization-m0-baseline`

Status: Complete locally; this slice is included in M2a's pending CI run.

Starting revision: `4c6fa1ca4`.

## Finding and change

GCC 16.2 emits `-Wsfinae-incomplete` while compiling the generated
`app/latte-dock-ng_autogen/mocs_compilation.cpp`: Qt 6 `QMetaType` probes the
incomplete `Latte::View` type through SFINAE before the same moc aggregation
includes its definition. GCC's [diagnostic discussion](https://gcc.gnu.org/pipermail/gcc-bugs/2025-July/923779.html)
records false positives in this new warning family.

The suppression applies only to that generated moc translation unit and only
for GNU C++ 16 or newer. All application sources and all other targets retain
the full `-Werror` gate. Remove the workaround once an updated Qt probe or GCC
diagnostic no longer reports this warning.

## Validation

- Fresh GCC 16.2 Debug/Release application and autotest builds passed with
  strict warnings. No compiler warning/error lines remained after the scoped
  suppression.
- CTest passed 44/44 in both GCC modes.
- Clang 22.1 Debug/Release builds and CTest passed 44/44 with no compiler
  warnings/errors; the GCC-specific option was not applied.
- Debian 13 CMake 3.20.6 / GCC 14.2 strict build and CTest passed 43/43; the
  GCC 16-only suppression was not applied.
- `git diff --check` — passed.

## Handoff

The workaround is limited to this compiler-generated file and is covered by
the full compiler matrix. Recheck it when the Qt version or GCC warning family
changes.
