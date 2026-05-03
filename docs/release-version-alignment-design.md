# Latte Dock NG Release Version Alignment Design

## Document Info
- Author: Ruizhi Zhong
- Contact: ruizhi.zhong88@gmail.com
- Date: 2026-05-03
- License: GPL-3.0
- Target release: `v1.0.1`

## Background
Latte Dock NG already exposes version data in multiple surfaces:
- Build metadata (`CMakeLists.txt`)
- About dialog (`app/main.cpp`)
- AppStream metadata (`app/org.kde.latte-dock.appdata.xml.cmake`)
- Git release tags

Historically, these values drifted (`0.10.77` in build metadata vs `v1.0.0` in Git tag), which causes user confusion and weakens release traceability.

## Goals
1. Keep runtime About dialog version identical to release tag version.
2. Define a single source of truth for versioning.
3. Make release operations deterministic and repeatable.

## Non-Goals
1. No feature behavior changes.
2. No packaging policy changes outside version metadata.
3. No migration of historical tags.

## Design Decisions
1. **Source of truth**: `set(VERSION ...)` in `CMakeLists.txt`.
2. **Runtime display**: About dialog must use `VERSION` macro (already in place).
3. **Tag format**: Use semantic version tag `vMAJOR.MINOR.PATCH` (for this release: `v1.0.1`).
4. **AppStream consistency**: Keep `<release version="@VERSION@" .../>` so generated metadata follows build version.

## Implementation
1. Bump `CMakeLists.txt` version from `0.10.77` to `1.0.1`.
2. Keep About dialog version binding unchanged (`QStringLiteral(VERSION)`).
3. Keep AppStream dynamic version entry unchanged (`@VERSION@`).
4. Create and push release tag `v1.0.1` on the commit containing this metadata update.

## Validation
1. Build and launch Latte Dock NG.
2. Open About dialog and verify displayed version is `1.0.1`.
3. Verify Git tag exists and points to the same commit:
   - `git show-ref --tags v1.0.1`
4. Verify generated AppStream includes `1.0.1` after configure/install.

## Release Workflow (Recommended)
1. Update `CMakeLists.txt` `VERSION`.
2. Commit release metadata changes.
3. Push branch.
4. Create annotated tag `vX.Y.Z`.
5. Push tag.
6. Publish GitHub Release from the pushed tag.

