#!/usr/bin/env bash

set -euo pipefail

dry_run=false
repo_dir=""
if [[ "${1:-}" == "--dry-run" ]]; then
    [[ $# -eq 2 ]] || { echo "Usage: $0 --dry-run <output-directory>" >&2; exit 2; }
    dry_run=true
    repo_dir=$(mkdir -p "$2" && cd "$2" && pwd)
elif [[ $# -ne 0 ]]; then
    echo "Usage: $0 [--dry-run <output-directory>]" >&2
    exit 2
fi

: "${APT_SIGNING_PRIVATE_KEY:?Set APT_SIGNING_PRIVATE_KEY to the armored repository signing key}"
: "${APT_DEB_DIR:?Set APT_DEB_DIR to the directory containing the release .deb files}"
APT_SIGNING_PASSPHRASE=${APT_SIGNING_PASSPHRASE:-}
export APT_SIGNING_PASSPHRASE

mapfile -t trixie_debs < <(find "$APT_DEB_DIR" -maxdepth 1 -type f -name 'latte-dock-ng_*+deb13u1_amd64.deb' -print)
mapfile -t testing_debs < <(find "$APT_DEB_DIR" -maxdepth 1 -type f -name 'latte-dock-ng_*-1_amd64.deb' ! -name '*+deb13u1*' -print)
mapfile -t ubuntu_debs < <(find "$APT_DEB_DIR" -maxdepth 1 -type f -name 'latte-dock-ng_*-1ubuntu1_amd64.deb' -print)
test "${#trixie_debs[@]}" -eq 1
test "${#testing_debs[@]}" -eq 1
test "${#ubuntu_debs[@]}" -eq 1

if [[ "$dry_run" == true ]]; then
    mkdir -p "$repo_dir"
    cd "$repo_dir"
elif git ls-remote --exit-code origin refs/heads/gh-pages >/dev/null 2>&1; then
    git fetch origin gh-pages
    git switch --force-create gh-pages origin/gh-pages
else
    git switch --orphan gh-pages
    git rm -rf --ignore-unmatch .
fi

for suite in trixie testing ubuntu; do
    mkdir -p "pool/$suite/main/l/latte-dock-ng"
done
install -m 0644 "${trixie_debs[0]}" pool/trixie/main/l/latte-dock-ng/
install -m 0644 "${testing_debs[0]}" pool/testing/main/l/latte-dock-ng/
install -m 0644 "${ubuntu_debs[0]}" pool/ubuntu/main/l/latte-dock-ng/

gpg_home="${RUNNER_TEMP:-${TMPDIR:-/tmp}}/apt-gnupg"
install -d -m 0700 "$gpg_home"
export GNUPGHOME="$gpg_home"
printf '%s' "$APT_SIGNING_PRIVATE_KEY" | gpg --batch --import
fingerprint=$(gpg --batch --with-colons --list-secret-keys | awk -F: '$1 == "fpr" { print $10; exit }')
test -n "$fingerprint"
gpg --batch --export "$fingerprint" > latte-dock-ng-archive-keyring.gpg

for suite in trixie testing ubuntu; do
    index_dir="dists/$suite/main/binary-amd64"
    mkdir -p "$index_dir"
    dpkg-scanpackages --multiversion "pool/$suite" /dev/null > "$index_dir/Packages"
    gzip -n -9 -c "$index_dir/Packages" > "$index_dir/Packages.gz"

    apt_config="${RUNNER_TEMP:-${TMPDIR:-/tmp}}/apt-ftparchive-$suite.conf"
    cat > "$apt_config" <<EOF
APT::FTPArchive::Release {
  Origin "ruizhi-lab";
  Label "Latte Dock NG";
  Suite "$suite";
  Codename "$suite";
  Architectures "amd64";
  Components "main";
  Description "Latte Dock NG packages";
};
EOF
    apt-ftparchive -c "$apt_config" release "dists/$suite" > "dists/$suite/Release"
    gpg --batch --yes --pinentry-mode loopback --passphrase "$APT_SIGNING_PASSPHRASE" \
        --local-user "$fingerprint" --clearsign \
        --output "dists/$suite/InRelease" "dists/$suite/Release"
    gpg --batch --yes --pinentry-mode loopback --passphrase "$APT_SIGNING_PASSPHRASE" \
        --local-user "$fingerprint" --armor --detach-sign \
        --output "dists/$suite/Release.gpg" "dists/$suite/Release"

    # repo_dir is set only for dry runs; publication validates from the
    # checked-out gh-pages worktree, where the keyring is at the repository root.
    gpgv --keyring "latte-dock-ng-archive-keyring.gpg" \
        "dists/$suite/Release.gpg" "dists/$suite/Release"
    gpgv --keyring "latte-dock-ng-archive-keyring.gpg" \
        "dists/$suite/InRelease"
    grep -q '^Package: latte-dock-ng$' "$index_dir/Packages"
done

if [[ "$dry_run" == true ]]; then
    echo "APT repository preflight succeeded in $repo_dir (no remote changes made)."
    exit 0
fi

git config user.name "github-actions[bot]"
git config user.email "41898282+github-actions[bot]@users.noreply.github.com"
git add dists pool latte-dock-ng-archive-keyring.gpg
git commit -m "Publish APT packages for ${APT_RELEASE_TAG:-${GITHUB_REF_NAME:-manual run}}"
git push origin gh-pages
