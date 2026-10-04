# Debian and Ubuntu APT repository

This workflow publishes Debian packages as a signed APT repository on GitHub
Pages. RPM packages are published through Fedora COPR.

The release workflow publishes these amd64 suites from the matching release
assets:

- `trixie`: Debian 13 package (`+deb13u1`)
- `testing`: Debian testing package
- `ubuntu`: package built natively on Ubuntu 26.04

The public repository URL is `https://ruizhi-lab.github.io/latte-dock-ng/`.
It is not available until the maintainer completes the one-time setup below.

## One-time setup

1. Create a dedicated GPG signing key for this repository. Do not reuse a
   personal or package-signing key. For example:

   ```bash
   gpg --quick-generate-key "Latte Dock NG APT Repository" rsa4096 sign 0
   gpg --list-secret-keys --keyid-format long
   gpg --armor --export-secret-keys KEY_ID
   ```

   Copy the armored private key output locally; never commit it or post it in
   an issue or chat. A passphrase is recommended.

2. In GitHub, open **Settings → Secrets and variables → Actions** for this
   repository and add:

   - `APT_SIGNING_PRIVATE_KEY`: the complete armored private key
   - `APT_SIGNING_PASSPHRASE`: the key passphrase; this may be empty only if
     the dedicated key was deliberately created without one

3. Run **Actions → Publish APT Repository → Run workflow**. Set
   `release_tag` to an existing release tag, for example `v1.2.54`. This seeds
   `gh-pages` with the DEBs and signed indexes from that release.

4. Open **Settings → Pages** and set **Build and deployment → Source** to
   **GitHub Actions**. The publisher deploys a Pages artifact after updating
   `gh-pages`; a branch-source deployment would not be triggered by the
   workflow's `GITHUB_TOKEN` push. Wait for the workflow's Pages deployment to
   finish. The public repository URL above should then serve `dists/` and
   `pool/`.

5. Verify that the public key file is reachable at
   `https://ruizhi-lab.github.io/latte-dock-ng/latte-dock-ng-archive-keyring.gpg`
   before announcing the repository.

On each push to `main`, the pre-release gate generates the signed Debian and
Ubuntu indexes in a temporary directory and verifies their signatures and
package entries without deploying them. A missing signing key or failed
preflight blocks the main validation run. The tagged release publishes the
same DEB artifacts that passed package installation and APT preflight; a
failed deployment blocks GitHub Release creation. The workflow can also be
run manually with an existing release tag to republish or seed the repository.

## User installation (after Pages is live)

Install the repository key and choose the suite matching the OS:

```bash
sudo install -d -m 0755 /etc/apt/keyrings
curl -fsSL https://ruizhi-lab.github.io/latte-dock-ng/latte-dock-ng-archive-keyring.gpg \
  | sudo tee /etc/apt/keyrings/latte-dock-ng.gpg >/dev/null
sudo chmod 0644 /etc/apt/keyrings/latte-dock-ng.gpg
```

Debian 13:

```bash
echo 'deb [arch=amd64 signed-by=/etc/apt/keyrings/latte-dock-ng.gpg] https://ruizhi-lab.github.io/latte-dock-ng trixie main' \
  | sudo tee /etc/apt/sources.list.d/latte-dock-ng.list
```

Debian testing:

```bash
echo 'deb [arch=amd64 signed-by=/etc/apt/keyrings/latte-dock-ng.gpg] https://ruizhi-lab.github.io/latte-dock-ng testing main' \
  | sudo tee /etc/apt/sources.list.d/latte-dock-ng.list
```

Ubuntu 26.04+ (the `ubuntu` suite uses the package built and install-checked
natively on Ubuntu 26.04):

```bash
echo 'deb [arch=amd64 signed-by=/etc/apt/keyrings/latte-dock-ng.gpg] https://ruizhi-lab.github.io/latte-dock-ng ubuntu main' \
  | sudo tee /etc/apt/sources.list.d/latte-dock-ng.list
```

Then install or upgrade:

```bash
sudo apt update
sudo apt install latte-dock-ng
```

Do not configure more than one of these suites on the same machine. The
packages target KDE Plasma 6.3+ on Wayland and amd64; Ubuntu support starts at
26.04 as verified by CI.
