# T4k: declare QML type metadata install producers

Baseline: `0a6974f5cd1b13116764a2320e3d882674f5fed3`. This slice addresses the four CMake 4.5 author warnings
recorded in [the previous clean install-preparation investigation](T4h-data/arch/cmake45/install-prep-qmltypes.md).

Each of the four generated `*.qmltypes` install rules now names its QML plugin
target as `BUILD_DEPENDS` when CMake is at least 4.5. The same dependency is
used for both the primary and Qt 6 compatibility QML roots. CMake documents
`BUILD_DEPENDS` as added in 4.5 and as an edge to install-preparation targets
([official command reference](https://cmake.org/cmake/help/v4.5/command/install.html)).
The argument is
version-gated because `BUILD_DEPENDS` was added in CMake 4.5 while the project
retains a CMake 3.20 floor. Existing `OPTIONAL` behavior is preserved. Without
the dependency, CMake 4.5 cannot infer the metadata producer for install
preparation and emits an author warning; the previous investigation showed
that a clean staged install still completed successfully.

The initial source scope is limited to the four owning `CMakeLists.txt` files.
Current HEAD at start was `0a6974f5cd1b13116764a2320e3d882674f5fed3`, branch
`main`, with a clean worktree.
No install into the user or system prefix, desktop change, commit, or push was
performed by this T4k slice.

During P4 validation, `main` advanced to
`ebb736602c4f5f529a127784c29720f762c59007`, adding P4 documentation/evidence.
The application and module CMake source diff from the T4k baseline to current
HEAD is empty; the frozen validation snapshot therefore matches current source
apart from the four uncommitted T4k module edits.

## Validation

Status: **Complete** after coordinator review. Genuine CMake 4.5.0 install
preparation and staged installation passed with Fedora GCC and host Clang.

Host validation uses CMake 4.3.4, so it checks that the guarded argument is
absent and that the existing CMake syntax still configures and builds. It does
not validate the new option or eliminate the CMake 4.5 warnings.

- GCC Debug used GCC 16.2.1 and CMake 4.3.4. `cmake --preset gcc-debug` and
  `cmake --build --preset gcc-debug --parallel 8` passed; zero source-located
  C++ warning/error diagnostics were found (build-aware QML diagnostics remain
  in the full log). `dbus-run-session -- ctest --test-dir
  build/modernization/gcc-debug --output-on-failure` passed 64/64 in 36.94 s.
- The first unisolated `ctest --test-dir build/modernization/gcc-debug
  --output-on-failure` stalled at `qmlsmoketest` without a private session bus
  and was interrupted. A sandboxed `dbus-run-session` retry could not bind its
  socket. The approved escalation allowed the private-bus run above. The
  interrupted and failed attempt details are in [host logs](T4k-data/host/).
- Clang Debug used Clang 22.1.8 and CMake 4.3.4. `cmake --preset
  clang-debug` and `cmake --build --preset clang-debug --parallel 8` passed
  with zero source-located C++ warning/error diagnostics. `dbus-run-session --
  ctest --test-dir build/modernization/clang-debug --output-on-failure` passed
  64/64 in 36.54 s. Build-aware QML diagnostics remain in the full log.
- The initial Fedora Release build using system CMake 4.3.0 and GCC 16.2.1
  passed configure/build/staged install and produced a shared P4 artifact.
  It does not validate T4k's 4.5 change; its evidence is under
  [the P4-owned Fedora artifact](T4k-data/fedora/README.md).
- Fedora's CMake 4.5 Release preparation build used GCC 16.2.1. Fedora lacks
  Clang, so a separate clean host Release build used Clang 22.1.8 with the same
  CMake 4.5.0 binary. That host emitted two Plasma QML tooling lookup status
  messages and one non-compiler QML type-registration `Warning:` about
  `PlasmaQuick::Dialog`; there were zero compiler warning/error lines and all
  install checks passed. See [Clang host evidence](T4k-data/cmake45-validation/clang-host/README.md).
  CMake 4.5 package provenance, mirror checks, and official digest verification
  are in the [GCC Fedora evidence](T4k-data/cmake45-validation/README.md).
- Genuine CMake 4.5.0 Fedora validation passed: a fresh Unix Makefiles Release
  tree had all eight target outputs absent before `install_prep/all`; configure
  had zero `CMake Warning` lines, the target completed successfully, and the
  preparation log had zero compiler `warning:`/`error:` lines. Staged install
  exited 0, and both QML roots contain all four nonempty typeinfo/plugin pairs
  with matching `qmldir` entries. The build used GCC 16.2.1. A separate host
  Clang 22.1.8 / CMake 4.5.0 clean Release `install_prep/all` and staged install
  also passed with zero compiler warnings/errors and the same eight metadata
  checks; its non-compiler QML type-registration warning is recorded above.
  The host GCC and Clang CMake 4.3.4 Debug builds both had zero source-located
  C++ warning/error diagnostics. See the [GCC Fedora evidence](T4k-data/cmake45-validation/README.md)
  and [Clang host evidence](T4k-data/cmake45-validation/clang-host/README.md).
- The earlier Arch guest attempt did not obtain a login route; its scoped
  host-key evidence and failure remain in [the Arch access note](T4k-data/arch/README.md).

Before the genuine 4.5 validation, the coordinator independently reviewed the
source diff and raw host/Fedora 4.3 logs, verified both host 64-test results
and zero compiler diagnostics, and checked the P4 artifact's installed
executable SHA-256. That artifact is separate from the CMake 4.5 build. The
coordinator also inspected both genuine 4.5 build logs and independently
verified all eight installed metadata/plugin combinations in each environment.
Both builds have zero compiler diagnostics; the host QML tooling diagnostics
listed above remain visible and are not claimed as passes.

## Handoff

Status: **Complete** after coordinator review of the four-file CMake diff and
[GCC and Clang CMake 4.5 evidence](T4k-data/cmake45-validation/README.md).
No full Release CTest matrix or desktop retest was run; the existing host
Debug CTest matrix passed. This build/install-only slice needs no desktop
retest; no runtime behavior or performance improvement is claimed.

Git authorization state: on `main`. The user authorized commit and push after
the required checks; publication follows coordinator acceptance.
