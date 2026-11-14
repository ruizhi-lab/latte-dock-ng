# Scheduled-destruction repair behavior

`containmentlayoutunittest` exercises the production
`LayoutManager::repairAppletContainers()` path with a backend applet marked for
scheduled destruction. It asserts that repair does not create a replacement
visual item. In an isolated source copy, removing the scheduled-destruction
guard caused the same test to fail because the item factory was called once.
The existing source contract remains in place for the separate backend-destroyed
guard; this test does not claim to exercise that branch.

The affected test and source-contract targets built without compiler warnings
on GCC and Clang Debug and Release. Their focused tests passed in all four
configurations, as did the complete CTest suite (64/64 each), using isolated XDG
directories and private D-Bus sessions. No production behavior changed. Remote
CI is pending.
