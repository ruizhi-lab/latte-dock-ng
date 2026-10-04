# Release workflow

This document describes the project release pipeline and the current status of
its package repositories. RPM repository setup details are in
[`third-party-rpm-repositories.md`](third-party-rpm-repositories.md); Debian
and Ubuntu APT setup is in [`debian-apt-repository.md`](debian-apt-repository.md).

## Prepare and publish a release

1. Update `set(VERSION X.Y.Z)` in `CMakeLists.txt` and `version = "X.Y.Z"` in
   `default.nix`. Add the `CHANGELOG.md` section and refresh `flake.lock` when
   advancing the pinned Nix dependencies.
2. Push the release-candidate commit to `main`. `.github/workflows/build.yml`
   runs GCC and Clang builds, autotests, QML lint, native install verification,
   and native package build/install verification for every supported distro.
   It also runs the NixOS flake checks and validates the current Gentoo overlay
   ebuild, metadata, candidate Manifest, and package build against the
   candidate source. The main-branch APT preflight generates all signed suites
   in a temporary directory and verifies signatures and package indexes; it
   does not modify GitHub Pages. Docker jobs pull current base-image tags and
   rebuild dependency layers without cache.
3. Wait for the complete `Build` run for that exact `main` commit to succeed.
   The candidate version in `CMakeLists.txt` must match the future `vX.Y.Z`
   tag. The Build run retains the seven packages that it built and installed.
   `.github/workflows/auto-release.yml` creates the annotated tag and dispatches
   the Release workflow only after that successful Build, when the version has
   not already been released. A tag that already points at another commit is
   never moved. Do not create a release tag to skip this gate.
4. The Release workflow independently checks the exact source commit and
   version, publishes the signed
   APT repository from the validated DEB artifacts, and creates the GitHub
   Release from the same validated package artifacts. It does not rebuild
   release packages after validation. The Gentoo overlay update is prepared
   from that final tag, gets its Manifest regenerated and QA-checked, then is
   pushed to `ruizhi-lab/gentoo-overlay`. It replaces older versioned ebuilds
   while preserving `latte-dock-ng-9999.ebuild` for main-branch installs.
   Debian 13 uses the `+deb13u1`
   revision, Debian testing uses `-1`, and Ubuntu uses `-1ubuntu1`.
5. Curate English release notes and link the preceding tag with
   `compare/vPREV...vX.Y.Z`.

Desktop runtime retesting is not a release gate. Use the GitHub Build and
Release workflow results as the authoritative build and package-install
verification for each release candidate.

The release workflow needs the `RUIZHI_OVERLAY_TOKEN` repository Actions
secret. It must be a fine-grained token limited to `ruizhi-lab/gentoo-overlay`
with Contents read/write access. A missing or insufficient token blocks the
release rather than silently skipping the overlay update.

## Repository publication after the tag

- **Fedora COPR:** the configured GitHub tag webhook requests a rebuild and
  package installation on the Fedora VM has succeeded. The current rpkg
  build used `0.0.git.<count>.<hash>` because the stock rpkg macro did not
  recognize the project's `vX.Y.Z` tags. The spec now uses custom macros to
  map those tags and suffix later commits. Verify a COPR build after this
  change before treating version handling as confirmed.
- **Debian and Ubuntu APT:** the release workflow publishes the Debian 13
  package to `trixie`, the Debian testing package to `testing`, and the
  Ubuntu-native package to `ubuntu`. Ubuntu 26.04 has older Qt dependencies
  than Debian testing, so these packages must be built natively for each
  distro. The APT signing key is stored only in GitHub
  Actions secrets. The initial repository seed and GitHub Pages deployment
  succeeded; future releases use the same automatic publication job.
- **openSUSE Tumbleweed and Mageia:** the COPR project is configured for these
  chroots. Confirm each distribution's build status on the COPR page before
  treating it as installable. GitHub Release RPMs remain available as a
  fallback until both their builds and version handling are verified.

## Current validation notes

As of 2026-10-03, the `Build` workflow run
[`37113104585`](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/37113104585)
failed both openSUSE jobs before RPM compilation. Their dependency container
images could not resolve Tumbleweed packages: `glib2-stage1-devel` required a
missing `this-is-only-for-build-envs` provider, and the solver also found
incompatible `libpcre2-8-0` 10.49 and 10.48 requirements. This is a CI
container dependency-resolution failure, not a COPR build result.
The Build workflow had selected a moving USTC mirror for openSUSE while the
Release workflow used upstream repositories. Both Build workflow jobs now use
the upstream openSUSE repositories to match the Release workflow. Rerun the
openSUSE CI checks to verify that this resolves the mirror inconsistency; do
not mark them verified based on package metadata or repository setup alone.

The later COPR build (`11067226`) failed separately on Mageia and openSUSE.
Mageia could not resolve Fedora-style `kf6-*-devel` BuildRequires; the spec
now has Mageia-specific package names. The openSUSE package compiled and
installed its files, then `%install` failed because the spec assumed Fedora's
`redhat-linux-build/install_manifest.txt` path. It now finds CMake's manifest
instead of assuming a distribution-specific build directory. Both chroots
need a successful COPR rebuild to validate these fixes.

## Version handling

Do not manually bump package spec versions for each release. The COPR spec
uses the latest `vX.Y.Z` tag to derive the version and commit metadata for
post-release snapshots. Its source archive and extraction directory share
that computed version. The application version in `CMakeLists.txt` and
`default.nix` remains the release source of truth.
