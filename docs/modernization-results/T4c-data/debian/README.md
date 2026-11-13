# T4c Debian 13 Validation

Validation ran in isolated `/tmp` source and build directories on Debian 13 with Qt 6.8.2, KDE Frameworks 6.13, Plasma 6.3.5, CMake 3.31.6, GCC 14.2.0, and Clang 19.1.7. The application and explicit `latte-autotests` target built with both compilers. The build logs contained no C++ `warning:` or `error:` diagnostics. QML lint emitted 3,239 warning lines per compiler.

The final offscreen CTest run passed 63 of 64 tests under both GCC and Clang. The sole failure was `settingsviewunittest`: Qt 6.8.2's offscreen platform kept `QAccessible::isActive()` false after `setActive(true)`. The focused settings view test passed all 5 cases on the VM's Wayland session with `QT_LINUX_ACCESSIBILITY_ALWAYS_ON=1` and `QT_ACCESSIBILITY=1` for both compiler builds. The persistent menu test passed all 5 cases on XCB with both compiler builds.

The menu A/B diagnostic confirmed the fixture issue: one mouse move produced zero toggles for both plain `QMenu` and `PersistentMenu`; a nine-step pointer traverse produced one toggle for each, while the persistent popup remained visible. The production menu veto works with the corrected pointer sequence.

Source basis: commit `64a7f1ae5dbccf5e23c88c8e88816b701f92930a`, with the test fixture updates overlaid. SHA-256 of the final tested test files:

- `settingsviewunittest.cpp`: `05e341299b26160e3ed98f741e8c6e58e1065930a1dcbc7817ec3a5408a035be`
- `persistentmenutest.cpp`: `a7cafc0e099bfb7f8b070adeae4abc46d0987c55226c4e9e36ea0a865e2cf904`

Evidence files:

- [GCC CTest log](latte-ctest-gcc-final.log)
- [Clang CTest log](latte-ctest-clang-final.log)
- [GCC settings view Wayland run](latte-focused-wayland-a11y.log)
- [Clang settings view Wayland run](latte-focused-wayland-clang.log)
- [GCC persistent menu XCB run](latte-persistent-xcb-final.log)
- [Clang persistent menu XCB run](latte-persistent-xcb-clang.log)
- [Single-move QMenu A/B](persistentmenu-ab-xcb.log)
- [Nine-step QMenu A/B](persistentmenu-ab-plain.log)
- [Instrumented nine-step QMenu A/B](persistentmenu-ab-instrumented.log)
