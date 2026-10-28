# Release workflow

This document describes the project release pipeline and the current status of
its package repositories. RPM repository setup details are in
[`third-party-rpm-repositories.md`](third-party-rpm-repositories.md); Debian
and Ubuntu APT setup is in [`debian-apt-repository.md`](debian-apt-repository.md).

Debian stable and the current Fedora stable are priority support targets
because of their large user bases. CI pins their concrete releases (currently
Debian 13 and Fedora 44); update the container and package metadata when either
distribution publishes its next stable release. Mageia CI follows Cauldron
because issue [#61](https://github.com/ruizhi-lab/latte-dock-ng/issues/61)
reports a taskbar failure on Mageia 11, so Mageia 10 is not the validation
target.

## Help documentation gate

Before dispatching a release build, review `docs/help/` against the
user-visible changes since the previous release. Update every affected language
page and the language `<select>` entries, verify that UI languages without a
translated guide still fall back to English, and run the help tests. A release
must not ship stale or missing help for changed
settings, dialogs, Advanced-mode visibility, copy/clone behavior, launcher
sharing or supported UI languages. Run `handbookdocumenttest` with the release
autotests and record any language that still only has the English fallback in
the release notes. A passing source-contract test does not prove the guide text
matches the UI; review the visible descriptions as well.

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
   NixOS flake checks and the versioned Nix package build. Gentoo is validated
   on the maintainer's Gentoo system; Build does not provision a second Gentoo
   environment. The signed APT preflight verifies signatures and indexes
   without publishing.
   Docker jobs pull current base images and rebuild dependencies without cache.
4. Require tag version to match the validated source version. After every gate
   passes, Build retains the seven packages and a candidate
   record containing the version, source SHA, Build run ID and package SHA256
   checksums. `Automatic Release` reads that record, requires the source to
   still be the current `main` commit, creates its annotated `vX.Y.Z` tag and
   explicitly dispatches `Release`. Tags are never moved. An existing release
   is skipped; an existing tag at another commit cannot be reused. If main
   advances during validation, dispatch a fresh candidate against the new head.
5. `Release` independently verifies the successful Build run, candidate
   identity, package checksums and tag commit. It publishes APT and GitHub
   Release from those same packages. The Gentoo job downloads the final tag
   archive, validates that it is readable, and calculates the Gentoo Manifest
   checksums for that archive and the current overlay files. It does not install
   Gentoo or compile the ebuild: after the other distro gates pass, it publishes
   the versioned ebuild directly. The overlay update removes older versioned
   ebuilds and keeps `latte-dock-ng-9999.ebuild` byte-for-byte unchanged.
   Release operations are serialized; an overlay revision change during
   preparation blocks publication. Debian 13 uses `+deb13u1`, Debian testing
   uses `-1`, and Ubuntu uses `-1ubuntu1`.
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

The Mageia release package build and container validation target Cauldron,
the development branch for Mageia 11. Mageia 10 stable is not a release
validation target.

## GitHub Actions versions

JavaScript actions in `.github/workflows/` must target Node.js 24. On
2026-10-28 every job of the `Build` run
[`40683601580`](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/40683601580)
carried the annotation "Node.js 20 is deprecated" for `actions/checkout@v4`,
`actions/upload-artifact@v4` and `actions/download-artifact@v4`, and the
`Automatic Release` run for `actions/github-script@v7`: the runner forced them
onto Node 24. The workflows now use the oldest major of each action that runs on
Node 24, so that as little as possible besides the runtime changes:

| Action | Version | Newer majors not taken, and why |
| --- | --- | --- |
| `actions/checkout` | `v5` | `v6` persists credentials to a separate file; the `gentoo-overlay-publish` job of `release.yml` pushes with the token its checkout persisted, so that needs its own check. `v7` blocks checking out fork pull-request code for `pull_request_target` and `workflow_run`; `auto-release.yml` checks out a `workflow_run` head commit |
| `actions/upload-artifact` | `v6` | First major that runs on Node 24 by default. `v7` adds direct (unzipped) uploads and is an ESM rewrite, neither needed |
| `actions/download-artifact` | `v7` | First major that runs on Node 24 by default. `v5` changed the path of single downloads by artifact ID (the workflows download by name and pattern). `v8` fails on a digest mismatch by default and no longer unzips non-zip downloads |
| `actions/github-script` | `v8` | `v9` removes `require('@actions/github')` and injects `getOctokit`; the scripts use only `github.rest`, `context`, `core` and `fs` |
| `actions/upload-pages-artifact` | `v5` | Wraps `actions/upload-artifact@v7`, which runs on Node 24; `v4` wraps `v4.6.2` |
| `actions/deploy-pages` | `v5` | Node 24 |
| `softprops/action-gh-release` | `v3` | Node 24; `v2.6.2` is the last Node 20 line |

The declared inputs and outputs of each action are identical between the old
and the new version, except that `actions/upload-pages-artifact@v5` adds the
optional `include-hidden-files`; every `with:` key in the workflows is therefore
still valid. `build.yml` and `auto-release.yml` run on every push to `main` and
confirm the new versions. `release.yml` (tag push or manual dispatch) and
`publish-apt.yml` (manual dispatch) use the same versions plus the Pages and
release actions and have not run with them yet: read the job annotations of the
first release or publication run after this change.

Before bumping an action again, read the release notes between the versions,
compare its `action.yml` inputs and outputs with the `with:` keys in use, and
check anything that depends on credential persistence, cross-run artifact
downloads or `workflow_run` checkouts.

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

The COPR Mageia Cauldron rebuild (`11114020`, package `1.2.59-1`) also remains
unverified. DNF failed while installing the spec's BuildRequires because the
Cauldron mirror metadata referenced `ntfs-3g-2026.9.18-1.mga11.x86_64.rpm`,
which returned 404 from every configured mirror. The failure occurred before
the source build started, so it does not indicate a spec or application compile
failure. Retry after Mageia's mirror metadata and package files are synchronized;
do not work around this by adding or removing project BuildRequires.

An earlier COPR build (`11067226`) failed separately on Mageia and openSUSE.
Mageia could not resolve Fedora-style `kf6-*-devel` BuildRequires; the spec
now has Mageia-specific package names. The openSUSE package compiled and
installed its files, then `%install` failed because the spec assumed Fedora's
`redhat-linux-build/install_manifest.txt` path. It now finds CMake's manifest
instead of assuming a distribution-specific build directory. Both chroots
need a successful COPR rebuild to validate these fixes.

## User-visible version verification

Before publication, require the candidate record, package metadata, installed
`latte-dock-ng --version`, and **Configure Latte → Help → About Latte Dock NG**
to report the same `X.Y.Z`. The native package job checks the installed binary,
not only the package filename or package-manager version. Check the About
window manually in English and Chinese during desktop validation.

`KAboutData` in `app/main.cpp` supplies both the About dialog and CLI version
from the configured `VERSION`. Do not add a separate translated or hard-coded
About version. A local Debug install without `LATTE_RELEASE_VERSION` deliberately
uses the source default, which can be older than the latest release tag. For a
release-candidate retest, pass the exact candidate version explicitly, for example:

```bash
LATTE_RELEASE_VERSION=1.2.56 bash install.sh --user --jobs 8 Debug
~/.local/bin/latte-dock-ng --version
```

Use the running executable's path when checking the About window: a user-local
Debug binary can shadow a correctly versioned system release. Rebuild with the
intended candidate version rather than changing source defaults or inferring a
release from a moving tag. Local code changes still require their own validation;
a matching version string alone does not make a Debug build a published release.

## Version handling

Do not manually bump package spec versions for each release. The COPR spec
uses the latest `vX.Y.Z` tag to derive the version and commit metadata for
post-release snapshots. Its source archive and extraction directory share
that computed version. The application version in `CMakeLists.txt` and
`default.nix` remains the release source of truth.
