# Applet item resolver behavior

The offscreen `containmentlayoutunittest` now calls the production applet item
resolver with a cyclic QObject property graph and verifies that it terminates
without a result. A separate acyclic property chain must still resolve its
QQuickItem. Existing source contracts remain in place.

In isolated source copies, removing the visited-object guard and resetting the
visited set in recursive calls each caused the cyclic test to fail by a stack
overflow. Each mutated run used a five-second timeout; neither mutation was
applied to the production working tree.

The affected tests built with GCC and Clang Debug and Release without compiler
warnings. Focused tests and the complete CTest suite passed in each
configuration (64/64), using isolated XDG directories and private D-Bus for
the complete suites. No production behavior changed. Remote CI is pending.
