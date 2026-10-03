# Release workflow

This document describes the project release pipeline and the current status of
its package repositories. RPM repository setup details are in
[`third-party-rpm-repositories.md`](third-party-rpm-repositories.md); Debian
and Ubuntu APT setup is in [`debian-apt-repository.md`](debian-apt-repository.md).

## Prepare and publish a release

1. Update `set(VERSION X.Y.Z)` in `CMakeLists.txt` and `version = "X.Y.Z"` in
   `default.nix`. Add the `CHANGELOG.md` section.
2. Run the required release checks: `nix flake check --print-build-logs`,
   `nix build .#default --no-link --print-build-logs`, and GCC and Clang
   autotests.
3. Commit the release changes and create an annotated `vX.Y.Z` tag on the
   intended commit. Push the commit and tag according to the repository's
   authorization rules.
4. The tag triggers `.github/workflows/release.yml`. It builds release
   artifacts for Fedora, openSUSE Tumbleweed, Mageia, Debian 13, Debian
   testing, and Arch, then publishes them as GitHub Release assets. The
   Debian 13 package has the `+deb13u1` revision; testing uses plain `-1`.
5. Curate English release notes and link the preceding tag with
   `compare/vPREV...vX.Y.Z`.

## Repository publication after the tag

- **Fedora COPR:** the configured GitHub tag webhook requests a rebuild.
  COPR build and package installation on the Fedora VM have succeeded.
- **Debian and Ubuntu APT:** the release workflow publishes the Debian 13
  package to `trixie`, the Debian testing package to `testing`, and the same
  testing package to `ubuntu`. The APT signing key is stored only in GitHub
  Actions secrets. The initial repository seed and GitHub Pages deployment
  succeeded; future releases use the same automatic publication job.
- **openSUSE Tumbleweed OBS:** the OBS package and tag workflow are configured.
  Automatic publication from a new GitHub release tag is still unverified;
  confirm the OBS service and build after the next tag. OBS is an RPM workflow
  independent of the Debian/Ubuntu APT repository.
- **Mageia:** the release workflow creates a Mageia RPM asset, but there is no
  maintained Mageia package repository. Users must obtain the matching RPM
  from GitHub Releases and install it manually.

## Current validation notes

As of 2026-10-03, the `Build` workflow run
[`37113104585`](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/37113104585)
failed both openSUSE jobs before RPM compilation. Their dependency container
images could not resolve Tumbleweed packages: `glib2-stage1-devel` required a
missing `this-is-only-for-build-envs` provider, and the solver also found
incompatible `libpcre2-8-0` 10.49 and 10.48 requirements. This is a CI
container dependency-resolution failure, not an OBS tag-workflow result.
The Build workflow had selected a moving USTC mirror for openSUSE while the
Release workflow used upstream repositories. Both Build workflow jobs now use
the upstream openSUSE repositories to match the Release workflow. Rerun the
openSUSE CI checks to verify that this resolves the mirror inconsistency; do
not mark them verified based on package metadata or repository setup alone.

## Version handling

Do not manually bump package spec versions for each release. The Fedora spec
uses rpkg Git metadata, and the OBS service updates its spec from the fetched
tag. The application version in `CMakeLists.txt` and `default.nix` remains the
release source of truth. Ensure
`packaging/obs/latte-dock-ng.spec` is included in the tagged source so OBS can
extract it.
