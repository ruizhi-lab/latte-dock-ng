# T4i: preserve popup centering integer rounding

`bugprone-integer-division` found the two popup-centering expressions in `Dialog::popupPosition()`. The popup extent is a `QSize` integer while its visual parent extent is `qreal`. The existing expression intentionally divides the popup extent as an integer, then converts the combined coordinate to `int`.

The repeated calculation now lives in the small inline helper `Latte::Core::Geometry::centeredPopupOrigin()` in `declarativeimports/core/popupgeometry.h`. It stores `popupExtent / 2` in an `int` before evaluating the same left-associative qreal expression. The helper's return conversion keeps the original truncation toward zero, including for negative global coordinates. Dividing the popup extent as qreal before conversion would move some odd-sized popups by one logical pixel.

Both horizontal and vertical branches call the helper. Screen and applet geometry clamping, plus `PlasmaQuick::Dialog::adjustGeometry()`, are unchanged.

`CoreUnitTest::popupGeometryKeepsIntegerHalfSizeRounding()` calls the production helper directly and checks even and odd extents, a fractional parent extent, and negative-coordinate truncation. The existing `coreunittest` target already provides Qt Test; this change adds no build dependency or target.

The clang-tidy gate now includes `bugprone-integer-division` as its 55th check. Its standard C++ negative fixture compiles and is required to trigger that check. LLVM 18.1.3 and 22.1.8 rejected all 55 fixtures and found no gated findings in 156 production sources. LLVM 18 reported one excluded SDK finding, independently identified in [the scope audit](T4i-data/tidy18-package-scope.json).

GCC and Clang Debug and Release builds had no compiler warnings and each passed 64/64 CTest targets. Focused clazy passed, as did the syntax check of 243 QML files and module metadata checks in all four builds. The native deep QML baseline was not rerun because the host Qt version differs from the CI baseline. [Validation evidence](T4i-data/validation-summary.json) and [Arch Qt 6.12 evidence](T4i-data/arch/README.md) record the results. The Arch core test passed the new cases with one unrelated desktop-only test skipped.

The coordinate arithmetic and the PlasmaQuick placement route are preserved; actual Wayland popup placement was not re-observed for this slice. No performance reduction is claimed. Remote CI for this slice is pending.
