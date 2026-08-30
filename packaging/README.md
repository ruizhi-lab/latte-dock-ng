# Distribution channels

The release workflow builds packages for all supported distribution channels.
The `publish-channels.yml` workflow publishes them after a GitHub Release is
created.

## Required repository configuration

Configure GitHub Pages to use **GitHub Actions** as its source, then add these
optional repository secrets:

- `AUR_SSH_PRIVATE_KEY`: an SSH key authorized for the `latte-dock-ng` AUR
  package.
- `COPR_WEBHOOK_URL`: the custom webhook URL for the Fedora COPR project.
- `APT_GPG_PRIVATE_KEY`: an ASCII-armored signing key for the Debian archive.
- `APT_GPG_KEY_ID`: the key fingerprint or signing key ID.

For Fedora, create a COPR project whose package source is this GitHub
repository, configure it to build the release tags, and put its custom
webhook URL in `COPR_WEBHOOK_URL`. The release workflow triggers that webhook
after the GitHub Release is published.

The AUR and COPR jobs are skipped when their secrets are absent. The Debian
job publishes two suites because Debian 13 and Debian unstable use different
`libplasma` SONAMEs:

- `trixie`: Debian 13 stable
- `sid`: Debian unstable/testing and compatible Ubuntu releases

The Debian repository is available at:

```text
https://ruizhi-lab.github.io/latte-dock-ng/
```

To configure Debian 13:

```bash
repo=https://ruizhi-lab.github.io/latte-dock-ng
curl -fsSL "$repo/latte-dock-ng-archive-keyring.asc" \
  | gpg --dearmor \
  | sudo tee /usr/share/keyrings/latte-dock-ng.gpg >/dev/null
echo "deb [signed-by=/usr/share/keyrings/latte-dock-ng.gpg] $repo trixie main" \
  | sudo tee /etc/apt/sources.list.d/latte-dock-ng.list
sudo apt update
sudo apt install latte-dock-ng
```

For Debian unstable/testing, use `sid` instead of `trixie` in the source line.
The two suites must not be mixed because they use different `libplasma`
SONAMEs.

The Gentoo package remains maintained in the personal
[`ruizhi-overlay`](https://github.com/ruizhi-lab/gentoo-overlay) repository.
The NixOS package remains available through this repository's flake and
`overlays.default`.
