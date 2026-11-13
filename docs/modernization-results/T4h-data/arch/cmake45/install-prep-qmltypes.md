# CMake 4.5 install preparation and QML type metadata

Validation ran on the Arch Plasma 6.8 VM using CMake 4.5.0, Unix Makefiles, and the isolated source snapshot `/tmp/latte-modernization-20261114-src`. The fresh build directory, install prefix, and `DESTDIR` were all under `/tmp/latte-installprep-repro-20261114`; no system install or source edit was performed.

## Configure warnings

CMake emitted the four expected author warnings:

- `declarativeimports/core/CMakeLists.txt:56`: `qml/org/kde/latte/core/lattecoreplugin.qmltypes`
- `app/declarativeimports/CMakeLists.txt:62`: `qml/org/kde/latte/private/app/latteprivateappplugin.qmltypes`
- `containment/CMakeLists.txt:52`: `qml/org/kde/latte/private/containment/lattecontainmentplugin.qmltypes`
- `plasmoid/CMakeLists.txt:81`: `qml/org/kde/latte/private/tasks/lattetasksplugin.qmltypes`

Each says the generated file is not a unique build-target output, so `install_prep/` targets cannot build it directly with this generator. CMake suggests attaching its producer to one target or naming the producer with `BUILD_DEPENDS`.

## Clean preparation and staged install

The exit status of the first clean `install_prep/all` process was not captured, so it is not inferred from its log. The isolated build was then cleaned and the clean preparation was rerun with an explicit status file:

```text
cmake --build /tmp/latte-installprep-repro-20261114/build --target clean
cmake --build /tmp/latte-installprep-repro-20261114/build --target install_prep/all --parallel 4
CLEAN_INSTALL_PREP_EXIT=0
```

Before that rebuild, all eight checked outputs were absent after `clean`: the four `.qmltypes` files and the four plugin `.so` files in `build/qml/org/kde/latte/{core,private/app,private/containment,private/tasks}`. The absence check recorded `CLEAN_REMOVED_ALL_8=YES`. This clean rebuild used `/usr/bin/c++` (GNU 16.2.1) and produced zero C++ `warning:` lines. The configure log contains exactly four `CMake Warning (author)` lines, listed above.

The subsequent staged install also exited 0:

```text
DESTDIR=/tmp/latte-installprep-repro-20261114/dest \
  cmake --install /tmp/latte-installprep-repro-20261114/build
INSTALL_AFTER_CLEAN_EXIT=0
```

Both installed QML roots were checked:

- `.../prefix/lib/qml`
- `.../prefix/lib/qt6/qml`

In each root, the four `qmldir` files reference the matching nonempty type metadata, and each module's plugin library exists and is nonempty:

| Module | `typeinfo` | Plugin |
| --- | --- | --- |
| `org/kde/latte/core` | `lattecoreplugin.qmltypes` | `liblattecoreplugin.so` |
| `org/kde/latte/private/app` | `latteprivateappplugin.qmltypes` | `liblatteprivateappplugin.so` |
| `org/kde/latte/private/containment` | `lattecontainmentplugin.qmltypes` | `liblattecontainmentplugin.so` |
| `org/kde/latte/private/tasks` | `lattetasksplugin.qmltypes` | `liblattetasksplugin.so` |

Thus this CMake 4.5 Unix Makefiles clean `install_prep/all` run built all four metadata files indirectly through their installed plugin targets, and the staged install copied them successfully despite the direct dependency-inference warnings. The detailed VM logs remain under `/tmp/latte-installprep-repro-20261114/`; they were not copied into this repository.
