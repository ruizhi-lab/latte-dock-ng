# COPR RPM repository

Fedora COPR hosts the project's personal RPM repository for Fedora, Mageia,
and openSUSE Tumbleweed. Package builds use the root
[`latte-dock-ng.spec`](../latte-dock-ng.spec) with `rpkg`; release packages
are also attached to GitHub Releases. All RPM builds target x86_64. openSUSE
Leap is unsupported because its Plasma version is below the project's minimum.
See the [`release workflow status`](release-workflow.md) for per-chroot
validation.

## COPR project configuration

The project is `ruizhi-lab/latte-dock-ng`. Enable the chroots needed for the
supported RPM targets (Fedora 44, Mageia Cauldron for Mageia 11 development,
and openSUSE Tumbleweed, x86_64).
On the project's **Packages** page, its SCM package uses:

- Package name: `latte-dock-ng`
- SCM type: Git
- Clone URL: `https://github.com/ruizhi-lab/latte-dock-ng.git`
- Committish: `main`
- Spec file: `latte-dock-ng.spec`
- SRPM method: `rpkg`
- Automatic rebuilds enabled

The GitHub webhook is configured under COPR **Settings → Integrations** and
GitHub **Settings → Webhooks**. It uses `application/json` and the
**Branch or tag creation** event. Since the release tags are named `vX.Y.Z`,
the webhook URL includes the package name suffix `latte-dock-ng` so COPR knows
which package to rebuild.

The first build can be started from the COPR package page. Later release tag
pushes trigger builds through the webhook. The root spec uses the custom rpkg
macros in `rpkg.macros` to derive a version from the latest `vX.Y.Z` tag and
create a matching source archive. Commits after a release tag get a
`.git.<count>.<hash>` suffix; keep the relevant release tag available to the
COPR checkout.

## Installation

Fedora users can enable COPR and install with:

```bash
sudo dnf copr enable ruizhi-lab/latte-dock-ng
sudo dnf install latte-dock-ng
```

For Mageia and openSUSE, use the distribution-specific repository setup
instructions on the [COPR project page](https://copr.fedorainfracloud.org/coprs/ruizhi-lab/latte-dock-ng/).
Confirm that the selected distribution's latest build succeeded before
installing. The currently published COPR build used `0.0.git.<count>.<hash>`.
The spec now derives versions from project release tags, but needs a successful
COPR rebuild to verify the fix. Until then, use the GitHub Release RPM if you
need a version aligned with a release. openSUSE users should use Tumbleweed
only, not Leap.

## Release version maintenance

Update the canonical application version in `CMakeLists.txt` and
`default.nix` as described in the release workflow. The custom rpkg macros in
`rpkg.macros` map the latest project `vX.Y.Z` tag to its `X.Y.Z` RPM version.
Commits after that tag receive a
`.git.<count>.<hash>` suffix; a build exactly on the release tag has the
release version with no suffix. The source archive and `%setup` directory use
the same computed version. Verify this behavior in COPR after the change is
published. Repository publication is separate from GitHub Release creation
and depends on each COPR chroot build succeeding.
