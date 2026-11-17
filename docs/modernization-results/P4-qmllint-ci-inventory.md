# P4 QML lint inventory repair

## CI evidence

Build workflow run [43983748060](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/43983748060)
used commit `ebb736602c4f5f529a127784c29720f762c59007`. GCC Debug, GCC Release,
and Clang Release builds and tests completed before Deep QML lint failed with
`QML file coverage changed; baseline comparison is not comparable`. The
separate syntax-only QML lint job passed. Clang Debug was cancelled after the
other matrix failures.

The subsequent [Build run 43986024054](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/43986024054)
for `ee25b5662` also reported Deep QML lint failures in all four compiler/mode
jobs; those jobs used the same expanded source inventory. Other jobs were still
running when this repair was reviewed.

The retained GCC Debug qmllint report identifies `qmllint 6.11.1`, JSON
revision 4, two successful chunk processes, and complete coverage of 255 files.
The reviewed baseline contains 244 files. Comparing the CI manifest with that
baseline found exactly 11 additions, all under `docs/modernization-results/`:

- `P4-arch-preflight-data/tools/p4-taskmanager-observer.qml`
- `P4-fedora-aa-data/attempts/run-2-valid/artifacts/control-tools/p4-taskmanager-observer.qml`
- `P4-fedora-aa-data/attempts/run-3-valid/artifacts/control-tools/p4-taskmanager-observer.qml`
- `P4-fedora-aa-data/attempts/run-4-execution-valid-environment-review/artifacts/control-tools/p4-taskmanager-observer.qml`
- `P4-fedora-aa-data/attempts/run-5-valid-6.8.3/artifacts/control-tools/p4-taskmanager-observer.qml`
- `P4-fedora-aa-data/attempts/run-6-valid-6.8.3/artifacts/control-tools/p4-taskmanager-observer.qml`
- `P4-fedora-aa-data/attempts/run-7-valid-6.8.3/artifacts/control-tools/p4-taskmanager-observer.qml`
- `P4-fedora-aa-data/attempts/run-8-valid-6.8.3/artifacts/control-tools/p4-taskmanager-observer.qml`
- `P4-fedora-observer-data/attempts/attempt-5-session/diagnostic-qml/AppletItem.qml`
- `P4-fedora-observer-data/attempts/attempt-6-session/diagnostic-qml/AppletItem.qml`
- `P4-fedora-observer-data/attempts/attempt-7-session/diagnostic-qml/AppletItem.qml`

The archived copies entered the manifest because both lint scripts used
`git ls-files '*.qml'` as their source inventory. Deep lint correctly failed
closed on the file-set mismatch; the baseline was not changed. The 11 files
also changed the visible warning totals, so accepting that report would have
hidden an inventory error rather than reviewed a comparable production run.

## Repair

Both lint scripts now call the Bash helper `scripts/qmllint-files.sh` for their
default tracked QML inventory. This keeps the syntax-only gate's dependency set
at Bash, Git, and qmllint; it does not add Python to that job. The helper
excludes only `docs/modernization-results/`, the repository's archived run
evidence, while retaining production QML, autotest QML, and every other tracked
QML path. The syntax script's explicit file arguments still lint exactly the
requested paths. The canonical P4 observer remains covered by its documented
independent offscreen readiness check.

The focused inventory test creates a temporary Git checkout and exercises the
actual Bash helper. It proves that tracked production QML, autotest QML, a
negative fixture, and ordinary docs QML remain listed, while archived result
QML and untracked build output are omitted. The current shared inventory is 244
files and exactly matches the reviewed baseline manifest. The coordinator
independently compared the helper output with `docs/qmllint-baseline.json`'s
`manifest.attemptedFiles`: 244 entries on each side, with no missing or extra
paths. This proves inventory parity only; it is not a Qt 6.11.1 deep-lint run.
The baseline itself is unchanged.

## Validation and limits

- `python3 autotests/test_qmllint_file_inventory.py`: passed.
- `python3 autotests/test_qmllint_baseline.py`: 19 tests passed.
- GCC Debug and Clang Debug reconfigured and built successfully; no compiler
  warnings appeared in either build output. Commands were
  `cmake -S . -B build/modernization/gcc-debug`,
  `cmake --build build/modernization/gcc-debug -j8`, then the equivalent
  configure/build commands for `build/modernization/clang-debug`. Build output
  was terminal-only and was not saved to a durable log.
- GCC Debug and Clang Debug isolated D-Bus CTest runs: 65/65 passed in each.
  Commands were
  `dbus-run-session -- ctest --test-dir build/modernization/gcc-debug --output-on-failure`
  and the equivalent command for `build/modernization/clang-debug`. CTest saved
  full output in
  `build/modernization/gcc-debug/Testing/Temporary/LastTest.log` and
  `build/modernization/clang-debug/Testing/Temporary/LastTest.log`. The
  coordinator independently counted 65 passed entries, zero failed entries
  and the new inventory test in both logs.
- The Nix flake package adds Git through `nativeCheckInputs` because the new
  inventory test creates a temporary Git repository. This keeps Git check-only
  and out of the installed runtime closure. Nix was unavailable locally, so
  the flake check could not be run here.
- CI installs Git explicitly for its source build job. The Ubuntu, Debian,
  Fedora, Mageia, and Gentoo build containers also install Git. The RPM spec
  does not list Git as a `BuildRequires`; its supported CI packaging path runs
  inside the Fedora/Mageia containers that provide Git, while the source
  archive package recipe does not run the autotest suite itself.
- `bash -n scripts/qmllint.sh scripts/qmllint-deep.sh scripts/qmllint-files.sh`:
  passed.
- `scripts/qmllint.sh`: passed on local qmllint 6.11.3 over 244 files.
- `scripts/qmllint-deep.sh build/modernization/gcc-debug`: linted all 244 files,
  checked modules/imports, then failed only at baseline comparison because the
  local tool fingerprint is qmllint 6.11.3 rather than the reviewed 6.11.1.
- Current shared source inventory exactly matches the 244-file reviewed
  baseline.
- A fresh Build CI run is still needed to verify the 6.11.1 import-resolution
  report against the restored 244-file manifest.
