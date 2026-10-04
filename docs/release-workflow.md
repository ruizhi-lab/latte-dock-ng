# Release workflow

This document describes the project release pipeline and the current status of
its package repositories. RPM repository setup details are in
[`third-party-rpm-repositories.md`](third-party-rpm-repositories.md); Debian
and Ubuntu APT setup is in [`debian-apt-repository.md`](debian-apt-repository.md).

## Prepare and publish a release

1. Push the intended source changes to `main`. Maintain `CHANGELOG.md` and
   refresh `flake.lock` when advancing the pinned Nix dependencies. A release
   does not require changing the default versions in `CMakeLists.txt` or
   `default.nix`.
2. In GitHub Actions, select **Build → Run workflow**, choose **main**, and
   supply `release_version`, for example `1.2.56` (without `v`). The CLI
   equivalent is:
   ```bash
   gh workflow run build.yml --ref main -f release_version=1.2.56
   ```
   The input must be a stable `X.Y.Z` version. CI uses it for the application,
   native packages, Nix derivations and the versioned Gentoo recipe. The
   source checkout's default versions remain unchanged. Ordinary push/PR
   builds continue using the matching CMake and Nix source defaults.
3. The complete `Build` run performs GCC/Clang builds and autotests, QML lint,
   native install/uninstall checks, seven native package build/install checks,
   NixOS flake checks and the versioned Nix package build. Gentoo preflight
   validates the current overlay template against the candidate source. The
   signed APT preflight verifies signatures and indexes without publishing.
   Docker jobs pull current base images and rebuild dependencies without cache.
4. After every gate passes, Build retains the seven packages and a candidate
   record containing the version, source SHA, Build run ID and package SHA256
   checksums. `Automatic Release` reads that record, requires the source to
   still be the current `main` commit, creates its annotated `vX.Y.Z` tag and
   explicitly dispatches `Release`. Tags are never moved. An existing release
   is skipped; an existing tag at another commit cannot be reused. If main
   advances during validation, dispatch a fresh candidate against the new head.
5. `Release` independently verifies the successful Build run, candidate
   identity, package checksums and tag commit. It publishes APT and GitHub
   Release from those same packages. Gentoo's Manifest must use the final tag
   archive, so the actual tag archive receives another QA/build check before
   publication. The overlay update removes older versioned ebuilds and keeps
   `latte-dock-ng-9999.ebuild` byte-for-byte unchanged. Release operations are
   serialized; an overlay revision change during validation blocks publication.
   Debian 13 uses `+deb13u1`, Debian testing uses `-1`, and Ubuntu uses `-1ubuntu1`.
6. Review the generated English release notes. The formal release includes
   a comparison with the preceding tag.

Main push builds also retain candidate records and can automatically publish
an as-yet-untagged source default version. The parameterized manual trigger
is the normal way to choose a new release version without editing those defaults.

For retrying publication, run **Release → Run workflow** with `release_tag`,
`source_sha`, and the successful `build_run_id`. These must match the immutable
candidate record. Rerunning a failed Build is also supported; formal publication
always uses a successful run and its validated artifacts. Publication across
GitHub Pages and the Gentoo repository is not transactional: permission,
network or service failures can still require a retry after preflight succeeds.

Source builds from a tag archive retain the development default unless the
packager supplies the tag version: use `cmake -DVERSION=X.Y.Z`, or set
`LATTE_RELEASE_VERSION=X.Y.Z` for `install.sh` and Nix builds. For flakes,
use `--impure` to allow the explicit environment input; pure flake evaluation
keeps the source default. Gentoo passes its `PV` and COPR passes its computed
RPM version to CMake automatically.

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
