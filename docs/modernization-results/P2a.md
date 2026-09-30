# P2a: Icon invalidation and work characterization

Status: Pending validation. Implementation and available invalidation checks
are complete; moving one live item between screens with different DPRs and
Debian-specific theme/icon invalidation cases remain pending. Raster and
scene-graph traces plus focused image/QIcon tests cover named-theme replacement,
enabled/active state, zero size, fractional DPR, color groups, overlays,
`providesColors` and window recreation.

Recorded main baseline: `e3ef1ddf001aa032cffa62db21cfd97196174746`.
Implementation branch: `codex/modernization-m0-baseline`.

## Change

The Debug-only `latte.iconitem` category records source and invalidation
generations, logical dimensions, effective raster request size, DPR, resulting
raster size, texture creation and destination-rectangle updates. It is disabled
by default; traces contain no source string or path. Release builds do not
contain the added counters or trace statements.

The theme-switch probe exposed that a named `QIcon::fromTheme()` source kept its
old raster after `KIconLoader::iconChanged`: `IconItem` listened to that loader's
settings signal only for implicit-size notifications. `IconItem` now schedules
a raster reload for `iconChanged` only when its current source is theme-backed
(`m_icon.name()` or `m_svgIconName` is non-empty). It also invalidates the cached
color-source identity so `providesColors` is recalculated when the same name
resolves to new pixels. A controlled red-to-blue theme replacement verifies the
new icon pixels and derived background color while an unrelated QImage source
does not reload. This is a bounded correctness fix required before a cache can
safely depend on theme identity.

Focused tests exercise changed QImage and QIcon raster sources, active and
disabled rendering, clear and reappearance at zero size, and scene-graph
rendering across a geometry change and sequential window recreation. The image,
icon and state tests compare produced pixels, not only notifications. The
scene-graph test skips under `offscreen` and `minimal`, where Qt cannot provide
the required platform scene graph.

## Validation

- `cmake --build build/modernization/gcc-debug --target declarativecoreunittest -j8` and `ctest --test-dir build/modernization/gcc-debug -R '^declarativecoreunittest$' --output-on-failure` — passed (1/1).
- `cmake --build build/modernization/clang-debug --target declarativecoreunittest -j8` and `ctest --test-dir build/modernization/clang-debug -R '^declarativecoreunittest$' --output-on-failure` — passed (1/1).
- `cmake --build build/modernization/gcc-release --target declarativecoreunittest -j8` and `ctest --test-dir build/modernization/gcc-release -R '^declarativecoreunittest$' --output-on-failure` — passed (1/1).
- `cmake --build build/modernization/clang-release --target declarativecoreunittest -j8` and `ctest --test-dir build/modernization/clang-release -R '^declarativecoreunittest$' --output-on-failure` — passed (1/1).
- `QT_QPA_PLATFORM=offscreen build/modernization/gcc-debug/bin/declarativecoreunittest iconItemReloadsForEnabledAndActiveState` — passed (3/3 QtTest cases including init/cleanup).
- Fedora 44 Wayland: `declarativecoreunittest iconItemReloadsThemedSourcesOnIconThemeChange` passed. Its temporary themes replace the same named icon from red to blue, add a white overlay, and check both rendered pixels and `providesColors`; an unrelated QImage-backed sibling does not repolish. The trace shows the changed icon's raster and texture reload. QtTest reported 3/3 cases including init/cleanup with no test QWARN; Mesa printed its existing `failed to create dri2 screen` warning while creating the scene-graph window.
- Fedora 44 Wayland: `declarativecoreunittest iconItemReloadsSvgForColorGroupChange` passed (3/3 cases including init/cleanup). The real themed SVG's `colorSet` changed to Button, emitted `repaintNeeded`, and triggered an IconItem polish, raster reload and texture recreation.
- Fedora 44 Wayland: the full `declarativecoreunittest` binary passed all 11 QtTest cases, including the theme, overlay and window recreation probes; no test failed or skipped. Mesa emitted two existing EGL screen warnings from the GUI VM.
- `kscreen-doctor -o` on the Fedora VM reports one connected output, so moving the same item between displays with different scale factors cannot be tested in this VM.
- `QT_QPA_PLATFORM=offscreen QT_LOGGING_RULES='latte.iconitem.debug=true' build/modernization/gcc-debug/bin/declarativecoreunittest` — passed 7, failed 0, skipped 2. The skips are the pre-existing dialog platform case and the new real-scene-graph case.
- Fedora 44 Wayland, GCC 16.2, Qt 6.11.2, KDE Frameworks 6.30, Plasma 6.7.5, kernel 7.2.7: the focused production `declarativecoreunittest iconItemRendersAndResizesInSceneGraph` passed at `QT_SCALE_FACTOR=1` and `1.5` with `QT_LOGGING_RULES=latte.iconitem.debug=true`; both runs exited 0 with no FAIL, Error, Fatal or ASSERT marker. At DPR 1, the 48-logical-pixel request produced a 48x48 raster at DPR 1. At simulated DPR 1.5, it produced a 96x96 raster at DPR 1.33. The same raster and DPR survived the 48x48 to 80x48 geometry change; both runs created a second raster/texture for that destination-only change and a third texture after the first QQuickWindow was destroyed and recreated.
- The first fractional-DPR assertion expected the backing raster to be exactly `48 * DPR` pixels and failed: Qt's QIcon engine returned 96x96 at DPR 1.33 for the 72-pixel request. The test now asserts the raster is at least the requested physical size, and that geometry-only changes preserve the actual raster dimensions and DPR. This is an observed Qt return shape, not a product-test failure.
- Before the scene-graph test, GCC/Clang Debug and Release `lattecoreplugin` builds passed. Their QML type registration reported the existing `dialog.h:25` missing PlasmaQuick::Dialog base metadata warning; C++ compilation had no warnings. This remains a tooling diagnostic, not a clean zero-warning QML metadata result.
- `git diff --check` — passed after formatter output was reviewed and unrelated whole-file style churn was removed.

The Fedora trace establishes that this geometry-only change currently reloads
the pixmap and creates a new texture even though the source raster and effective
minimum raster request remain unchanged. The trace output is retained in the
task's temporary test log, not committed because it contains runtime object
addresses.

## Candidate P2b key and dirty rules

Do not use `QVariant::toString()` or a theme icon name as the cache key by
itself: the same name can resolve to different pixels after a theme change.
The candidate one-item raster key must represent source revision/content,
effective raster dimensions and DPR, plus every option that changes rendering:
icon mode/state (including enabled and active), color group/palette or Plasma
theme generation, overlays, `usesPlasmaTheme`, and `providesColors` where its
behavior affects loading. Geometry's destination rectangle is separate from
the raster key. A window or scene-graph recreation must always create a new GPU
texture from the retained valid CPU raster; textures must not be shared across
windows. This key remains provisional until the outstanding invalidation cases
below are observed.

| Invalidation case | Evidence now | Remaining check |
| --- | --- | --- |
| QImage replacement, same empty string identity | Red-to-blue output test passes | Confirm Debug generation trace for replacement |
| QIcon replacement, same empty string identity | Red-to-blue output test passes | Theme-backed QIcon resolution and theme signal |
| Named theme icon replacement under the same source identity | Fedora Wayland controlled red-to-blue switch passes after `KIconLoader::iconChanged`; ordinary QImage source stays idle | Verify on Debian Plasma 6.3 and through a real system icon-theme switch |
| Zero size then non-zero | Raster clears and green pixels return | Scene-graph node removal and recreation on a live window |
| Geometry-only change with same effective raster size | Fedora Wayland DPR 1 and 1.5: raster dimensions/DPR remain stable; destination changes and texture/raster work repeats | Visually compare hover/zoom sharpness at integer and fractional scale |
| SVG source repaint and Plasma theme change | Setter/signal path reviewed; named icon theme signal tested | Exercise same-URL Plasma SVG content/theme refresh and Plasma theme switch |
| SVG color-group change | Fedora Wayland: KSvg color set, repaint signal, raster and texture all update | Verify color-group output on Debian Plasma 6.3 |
| Overlay list change | Temporary theme overlay changes production pixels and triggers raster/texture reload | Verify overlay output on Debian Plasma 6.3 |
| Enabled/active icon states | Blue source pixels change for active and disabled production rasters | Verify state-change texture output on the live scene graph |
| `providesColors` | Same-name red-to-blue theme switch updates the derived background color | Verify derived color output on Debian Plasma 6.3 |
| Fractional DPR | Fedora Wayland test at simulated DPR 1.5 reports 96x96 backing at DPR 1.33 for the 72-pixel QIcon request | Confirm on a physical/non-simulated fractional-scale display |
| Moving between screens | Not tested; `kscreen-doctor -o` reports one connected Fedora VM output | Move the live dock between outputs with different DPRs |
| QQuickWindow / scene graph recreation | Sequential Fedora Wayland windows each render; trace shows a new texture for the replacement window | Confirm compositor-driven scene-graph invalidation/recovery if a safe trigger is available |

The actual task delegate uses `Kirigami.Icon` in `plasmoid/package/contents/ui/task/TaskIcon.qml`;
it does not use this `Latte::IconItem`. Therefore the IconItem trace is a
production component characterization, not evidence of task-hover preview or
highlight behavior. The final functional retest must independently exercise
PreviewWindows, HighlightWindows and PreviewAndHighlightWindows.

## Handoff

Complete the remaining invalidation observations and add focused regression
coverage where production behavior is testable without copying its algorithm.
Keep source identity, theme generation and destination geometry distinct in
the P2b design. Revisit the candidate key against each observed invalidation
before implementing a cache; do not proceed on the current partial matrix.
