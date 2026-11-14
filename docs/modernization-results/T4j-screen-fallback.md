# Startup screen fallback and drawing follow-up

An explicit secondary-screen dock started while its configured output was not
yet available. `GenericLayout::addView()` selected the primary screen and also
persisted that temporary choice as `onPrimary` and `lastScreen`. Once the output
appeared, the dock continued following the primary screen. Startup now keeps
the configured screen choice and passes its connector to the Positioner while
using the primary screen temporarily. The existing screen change path can
consider that connector when it appears. If a stored screen ID has no connector
mapping, automatic return cannot be established from that ID alone.

The drawing follow-up combines identical Left/Justify horizontal and
Top/Justify vertical origin assignments. Justify retains its full-length
calculation. Image tests call the production drawing function and compare
origins and extents; the remaining branch-clone findings are not gated.

GCC and Clang Debug and Release builds had no compiler warnings, and CTest
passed 64/64 in each configuration. The LLVM 18.1.3 and 22.1.8 gates rejected
all 55 negative fixtures and found no gated findings in 156 production sources.
Focused clazy passed on the changed screen sources and the drawing source.
QML syntax and module checks passed for the drawing follow-up. The first GCC
Debug CTest attempt for the drawing work hit restricted local socket access;
all four configurations passed in isolated XDG and private D-Bus sessions.

On 2026-11-14 the user confirmed both docks and task hover previews worked
after restarting the user-mode Debug build; the debug log contained no warnings
or errors after this retest. The secondary dock was restored to its prior DP-3
assignment after the earlier fallback. The preview issue was resolved by
selecting the hover preview action following a settings reset, without code
changes. Later on 2026-11-14, with the user's agreement, DP-3 was disabled
before a user-mode Debug restart and then re-enabled. The log recorded the
expected temporary fallback to DP-1 and the subsequent `setScreenToFollow()`
for DP-3. The containment retained `lastScreen=11` and `onPrimary=false`;
display geometry, scaling and rotation matched their prior values. The user
confirmed that both docks and hover previews were normal after restoration.
The only warning in that retest log was the expected unavailable-screen
fallback. This verifies one live output-disable/re-enable scenario with a known
connector; it does not cover an unknown connector mapping. Published as `e8ba4ad70`,
[CI run 43545267519](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/43545267519)
passed all 23 jobs.
