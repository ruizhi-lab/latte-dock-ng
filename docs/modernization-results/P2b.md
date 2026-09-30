# P2b: Conservative per-item geometry deduplication

Status: Complete. The bounded destination-only resize optimization and
production tests pass. GCC/Clang Debug and Release targets pass locally, and
Fedora Wayland at DPR 1 and 1.5 verifies both the skip and required reload when
the raster request changes. The user reports remaining theme, hover/zoom and
visual checks passed. Cross-screen movement is accepted by user-requested code
review only; no dual-display runtime test or broader application A/B memory
sampling was performed.

Recorded main baseline: `e3ef1ddf001aa032cffa62db21cfd97196174746`.
Implementation branch: `codex/modernization-m0-baseline`.
Characterization: [P2a](P2a.md).

## Change

`IconItem::geometryChange()` now compares the old and new smaller logical edge,
which is the size input used by its raster loader. When only the destination
rectangle changes, it marks the node geometry dirty and requests a scene-graph
update without scheduling `loadPixmap()`. A changed smaller edge still schedules
the full raster path. Existing window/device-pixel-ratio changes continue to
schedule through `itemChange()`, and source, theme, state, overlay and color
notifications retain their current invalidation routes. There is no global
cache, cross-window texture sharing, worker thread or quality change.

A counting `QIconEngine` in the production scene-graph test proves whether
`IconItem` requests raster work, independently of pixel equality. The test checks
that changing geometry from 48x48 to 80x48 keeps the request count, raster and
DPR unchanged, while increasing the smaller edge to 64 requests a new raster.
It then destroys the first window and verifies a replacement item can create a
fresh window-owned texture.

## Validation

- For each of `gcc-debug`, `clang-debug`, `gcc-release` and `clang-release`:
  `cmake --build build/modernization/<config> --target declarativecoreunittest lattecoreplugin -j8` — passed; `ctest --test-dir build/modernization/<config> -R '^declarativecoreunittest$' --output-on-failure` — passed (1/1).
- Fedora 44 Wayland, GCC 16.2, Qt 6.11.2, KDE Frameworks 6.30, Plasma 6.7.5: `declarativecoreunittest iconItemRendersAndResizesInSceneGraph` passed with `QT_SCALE_FACTOR=1` and `1.5`. At each scale the destination-only width change generated one `destination-update` and no additional `pixmap-load` or `texture-create`. Increasing the smaller edge generated both; destroying/replacing the window generated a new texture. The counting icon engine asserted the raster-request counts at each step.
- At DPR 1 the first raster was 48x48; at simulated DPR 1.5, QIcon returned 108x108 with DPR 1.5 for the initial 72-pixel request and 144x144 for the 96-pixel request after the smaller edge grew. The optimization preserved these returned sizes and DPRs.
- Fedora Wayland full `declarativecoreunittest` run — passed 11/11 QtTest cases, no failures or skips. The VM emitted two known Mesa `failed to create dri2 screen` warnings during scene-graph setup.
- `git diff --check` — passed after reviewing the focused diff.

## Work and resource result

The P2a baseline trace at both DPR 1 and 1.5 showed a pixmap load and texture
creation on a width-only change from 48x48 to 80x48 even though the smaller edge
and rendered pixels were unchanged. After this change the same event updates
only the destination rectangle. The next change to 80x64 reloads the raster and
texture as required. This proves removal of one avoidable raster request and
texture upload per equivalent geometry update. It does not establish a
steady-state RSS or whole-process CPU reduction; the optimization retains the
same one-item pixmap and window-owned texture between updates.

## Handoff

The user reports the Debian and hover/zoom visual checks passed. Cross-screen
movement is accepted by code review only, with no physical dual-display test.
Gather application-level A/B resource samples only if they can isolate this
geometry update without replacing the higher-resolution pixmap-size trace
evidence with VM-wide RSS noise.
