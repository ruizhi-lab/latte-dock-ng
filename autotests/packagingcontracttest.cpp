/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QFile>
#include <QStringList>
#include <QTest>

class PackagingContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void distroInstallPackagingContractsStayInSync();
};

void PackagingContractTest::distroInstallPackagingContractsStayInSync()
{
    QFile uninstallScript(QStringLiteral(LATTE_SOURCE_DIR "/uninstall.sh"));
    QVERIFY(uninstallScript.open(QFile::ReadOnly));
    const QString uninstallSource = QString::fromUtf8(uninstallScript.readAll());
    QVERIFY(uninstallSource.contains(QStringLiteral("org.kde.latte.contextmenu.so")));
    QVERIFY(uninstallSource.contains(QStringLiteral("plasma_containmentactions_lattecontextmenu.so")));
    QVERIFY(uninstallSource.contains(QStringLiteral("latte-dock-ng-add-launcher")));
    QVERIFY(uninstallSource.contains(QStringLiteral("org.kde.latte-dock.kickeractions.desktop")));
    QVERIFY(uninstallSource.contains(QStringLiteral("lib/x86_64-linux-gnu/qt6/qml")));

    QFile installScript(QStringLiteral(LATTE_SOURCE_DIR "/install.sh"));
    QVERIFY(installScript.open(QFile::ReadOnly));
    const QString installSource = QString::fromUtf8(installScript.readAll());
    QVERIFY(installSource.contains(QStringLiteral("--build-dir <path>")));
    QVERIFY(installSource.contains(QStringLiteral("LATTE_BUILD_DIR")));
    QVERIFY(installSource.contains(QStringLiteral("lib/x86_64-linux-gnu/qt6/qml")));
    QVERIFY(installSource.contains(QStringLiteral("refresh_service_caches()")));
    QVERIFY(installSource.contains(QStringLiteral("update-desktop-database \"${install_prefix}/share/applications\"")));
    QVERIFY(installSource.contains(QStringLiteral("kbuildsycoca6")));
    QVERIFY(installSource.contains(QStringLiteral("--noincremental")));

    QFile mainSourceFile(QStringLiteral(LATTE_SOURCE_DIR "/app/main.cpp"));
    QVERIFY(mainSourceFile.open(QFile::ReadOnly));
    const QString mainSource = QString::fromUtf8(mainSourceFile.readAll());
    QVERIFY(mainSource.contains(QStringLiteral("lib/x86_64-linux-gnu/qt6/qml")));

    QFile knsCompat(QStringLiteral(LATTE_SOURCE_DIR "/app/knscompat.cpp"));
    QVERIFY(knsCompat.open(QFile::ReadOnly));
    const QString knsCompatSource = QString::fromUtf8(knsCompat.readAll());
    QVERIFY(knsCompatSource.contains(QStringLiteral("userLocalQmlBase")));
    // Hardcoded ~/.local QML paths are only allowed inside the legacy-cleanup
    // block that removes pre-1.2.x overrides; override creation itself must
    // resolve the target through userLocalQmlBase().
    const QString hardcodedLegacyPath = QStringLiteral("QDir::homePath() + QStringLiteral(\"/.local/lib64/qt6/qml\")");
    const int legacyCleanupComment = knsCompatSource.indexOf(QStringLiteral("Clean up overrides from the old user-local Qt QML path"));
    QVERIFY(legacyCleanupComment >= 0);
    QCOMPARE(knsCompatSource.count(hardcodedLegacyPath), 1);
    QVERIFY(knsCompatSource.indexOf(hardcodedLegacyPath) > legacyCleanupComment);
    QVERIFY(knsCompatSource.contains(QStringLiteral("const QString qmlBase = userLocalQmlBase(systemQmlBase);")));

    QFile dockerCompose(QStringLiteral(LATTE_SOURCE_DIR "/docker/docker-compose.yml"));
    QVERIFY(dockerCompose.open(QFile::ReadOnly));
    const QString dockerSource = QString::fromUtf8(dockerCompose.readAll());
    QVERIFY(dockerSource.contains(QStringLiteral("${LATTE_SRC:-..}:/src:ro")));
    QVERIFY(dockerSource.contains(QStringLiteral("bash /src/docker/verify-install.sh")));
    QVERIFY(dockerSource.contains(QStringLiteral("LATTE_DEBIAN_IMAGE:-docker.m.daocloud.io/library/debian")));
    QVERIFY(dockerSource.contains(QStringLiteral("bash /src/docker/verify-install.sh debian13")));
    QVERIFY(dockerSource.contains(QStringLiteral("dockerfile: Dockerfile.gentoo")));
    QVERIFY(dockerSource.contains(QStringLiteral("bash /src/docker/verify-ebuild-gentoo.sh")));
    QVERIFY(dockerSource.contains(QStringLiteral(
            "NIXPKGS_CHANNEL_URL: https://mirrors.tuna.tsinghua.edu.cn/nix-channels/nixpkgs-unstable")));
    QVERIFY(dockerSource.contains(QStringLiteral(
            "substituters = https://mirrors.tuna.tsinghua.edu.cn/nix-channels/store https://cache.nixos.org/")));
    QCOMPARE(dockerSource.count(QStringLiteral("USE_MIRRORS: \"true\"")), 7);
    QVERIFY(!dockerSource.contains(QStringLiteral("/data/projects/latte-dock-ng:/src:ro")));

    const QStringList distroDockerfiles{
        QStringLiteral("arch"), QStringLiteral("debian"), QStringLiteral("debian-testing"), QStringLiteral("fedora"),
        QStringLiteral("gentoo"), QStringLiteral("mageia"), QStringLiteral("opensuse"), QStringLiteral("ubuntu"),
    };
    for (const QString &distro : distroDockerfiles) {
        QFile dockerfile(QStringLiteral(LATTE_SOURCE_DIR "/docker/Dockerfile.") + distro);
        QVERIFY(dockerfile.open(QFile::ReadOnly));
        const QString dockerfileSource = QString::fromUtf8(dockerfile.readAll());
        QVERIFY2(dockerfileSource.contains(QStringLiteral("ARG USE_MIRRORS=false")), qPrintable(distro));
    }

    QFile mageiaDockerfile(QStringLiteral(LATTE_SOURCE_DIR "/docker/Dockerfile.mageia"));
    QVERIFY(mageiaDockerfile.open(QFile::ReadOnly));
    const QString mageiaDockerfileSource = QString::fromUtf8(mageiaDockerfile.readAll());
    QVERIFY(mageiaDockerfileSource.contains(QStringLiteral("dnf install -y --refresh")));
    QVERIFY(!mageiaDockerfileSource.contains(QStringLiteral("dnf upgrade")));

    QFile dockerVerify(QStringLiteral(LATTE_SOURCE_DIR "/docker/verify-install.sh"));
    QVERIFY(dockerVerify.open(QFile::ReadOnly));
    const QString dockerVerifySource = QString::fromUtf8(dockerVerify.readAll());
    QVERIFY(dockerVerifySource.contains(QStringLiteral("install.sh --system --build-dir")));
    QVERIFY(dockerVerifySource.contains(QStringLiteral("install.sh --user --build-dir")));
    QVERIFY(dockerVerifySource.contains(QStringLiteral("manifestless uninstall fallback")));
    QVERIFY(dockerVerifySource.contains(QStringLiteral("system helper binary")));
    QVERIFY(dockerVerifySource.contains(QStringLiteral("system kicker action")));
    QVERIFY(dockerVerifySource.contains(QStringLiteral("user helper binary")));
    QVERIFY(dockerVerifySource.contains(QStringLiteral("user kicker action")));
    QVERIFY(dockerVerifySource.contains(QStringLiteral("-perm -111")));
    QVERIFY(dockerVerifySource.contains(QStringLiteral("record_build_stack")));
    QVERIFY(dockerVerifySource.contains(QStringLiteral("verify_debian13_build_stack")));
    QVERIFY(dockerVerifySource.contains(QStringLiteral("cmake:3.31.6")));
    QVERIFY(dockerVerifySource.contains(QStringLiteral("qt6-base-dev:6.8.2")));
    QVERIFY(dockerVerifySource.contains(QStringLiteral("libkf6coreaddons-dev:6.13.0")));
    QVERIFY(dockerVerifySource.contains(QStringLiteral("libplasma-dev:6.3.5")));
    QVERIFY(dockerVerifySource.contains(QStringLiteral("plasma-workspace-dev:6.3.6")));
    QVERIFY(dockerVerifySource.contains(QStringLiteral("locale -a | grep -Eiq '^C[.]utf-?8$'")));

    QFile archPackage(QStringLiteral(LATTE_SOURCE_DIR "/docker/package-arch.sh"));
    QVERIFY(archPackage.open(QFile::ReadOnly));
    const QString archPackageSource = QString::fromUtf8(archPackage.readAll());
    QVERIFY(archPackageSource.contains(QStringLiteral("depend = kirigami")));
    QVERIFY(archPackageSource.contains(QStringLiteral("depend = kcmutils")));
    QVERIFY(!archPackageSource.contains(QStringLiteral("depend = kf6-kirigami")));

    QFile archDockerfile(QStringLiteral(LATTE_SOURCE_DIR "/docker/Dockerfile.arch"));
    QVERIFY(archDockerfile.open(QFile::ReadOnly));
    const QString archDockerfileSource = QString::fromUtf8(archDockerfile.readAll());
    QVERIFY(archDockerfileSource.contains(QStringLiteral("zstd")));

    QFile gentooDockerfile(QStringLiteral(LATTE_SOURCE_DIR "/docker/Dockerfile.gentoo"));
    QVERIFY(gentooDockerfile.open(QFile::ReadOnly));
    const QString gentooDockerfileSource = QString::fromUtf8(gentooDockerfile.readAll());
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("gentoo/stage3")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("ARG USE_MIRRORS=false")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("GENTOO_MIRRORS=")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("binrepos.conf")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("GENTOO_BINHOST_URI_CN")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("rm -f /etc/portage/binrepos.conf/")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("emerge-webrsync --quiet || emerge-webrsync --quiet --no-pgp-verify")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("FEATURES=\"getbinpkg -usersandbox -network-sandbox -pid-sandbox -ipc-sandbox\"")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("--getbinpkg --usepkg --binpkg-respect-use=y")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("--autounmask-write=y --autounmask-continue=y")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("USE=\"X cups dbus elogind gui opengl qml wayland widgets\"")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("LLVM_SLOT=\"21\"")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("package.use/latte-dock-ng")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("app-text/xmlto text")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("dev-qt/qt5compat icu qml")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("dev-qt/qtbase cups icu libproxy opengl wayland")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("dev-qt/qtdeclarative opengl")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("dev-qt/qtlocation opengl")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("dev-qt/qtmultimedia opengl")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("dev-qt/qtquick3d opengl")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("dev-qt/qttools opengl")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("emerge --oneshot --noreplace")));
    QVERIFY(!gentooDockerfileSource.contains(QStringLiteral("ACCEPT_KEYWORDS=\"~amd64\"")));
    QVERIFY(!gentooDockerfileSource.contains(QStringLiteral("emerge --update --newuse --deep")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("kde-frameworks/kcoreaddons dbus")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("kde-plasma/plasma-workspace -fontconfig -handbook")));
    QVERIFY(!gentooDockerfileSource.contains(QStringLiteral("kde-plasma/plasma-workspace -fontconfig -handbook -X")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("media-libs/libglvnd X")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("media-libs/mesa llvm_slot_21")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("sys-libs/minizip-ng compat")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("sys-libs/zlib minizip")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("sys-apps/dbus X")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("sys-apps/accountsservice elogind")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("x11-libs/libxkbcommon X")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("x11-base/xwayland libei")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("extra-cmake-modules")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("emerge --oneshot --noreplace --usepkgonly --nodeps \\\n      kde-plasma/kscreenlocker:6")));
    const int kscreenlockerPreinstall = gentooDockerfileSource.indexOf(QStringLiteral("emerge --oneshot --noreplace --usepkgonly --nodeps"));
    const int plasmaWorkspaceInstall = gentooDockerfileSource.indexOf(QStringLiteral("kde-plasma/plasma-workspace:6"));
    QVERIFY(kscreenlockerPreinstall >= 0);
    QVERIFY(plasmaWorkspaceInstall > kscreenlockerPreinstall);
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("emerge --oneshot --noreplace \\\n      kde-plasma/kwayland:6")));
    QVERIFY(!gentooDockerfileSource.contains(QStringLiteral("sys-devel/gcc")));
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("dev-vcs/git")));

    QFile gentooEbuildVerify(QStringLiteral(LATTE_SOURCE_DIR "/docker/verify-ebuild-gentoo.sh"));
    QVERIFY(gentooEbuildVerify.open(QFile::ReadOnly));
    const QString gentooEbuildVerifySource = QString::fromUtf8(gentooEbuildVerify.readAll());
    QVERIFY(gentooEbuildVerifySource.contains(QStringLiteral("version=\"${VERSION:-9999}\"")));
    QVERIFY(gentooEbuildVerifySource.contains(QStringLiteral("if [[ \"${version}\" == \"9999\" ]]")));
    QVERIFY(gentooEbuildVerifySource.contains(QStringLiteral("inherit ecm git-r3 xdg")));
    QVERIFY(gentooEbuildVerifySource.contains(QStringLiteral("EGIT_REPO_URI=\"https://github.com/ruizhi-lab/latte-dock-ng.git\"")));
    QVERIFY(gentooEbuildVerifySource.contains(QStringLiteral("EGIT_BRANCH=\"main\"")));
    QVERIFY(gentooEbuildVerifySource.contains(QStringLiteral("PROPERTIES=\"live\"")));
    QVERIFY(gentooEbuildVerifySource.contains(QStringLiteral("latte-dock-ng-${version}.ebuild")));
    QVERIFY(gentooEbuildVerifySource.contains(QStringLiteral("ebuild")));
    QVERIFY(!gentooEbuildVerifySource.contains(QStringLiteral(">=kde-plasma/kscreenlocker-6.3:6")));
    QVERIFY(gentooEbuildVerifySource.contains(QStringLiteral("clean configure compile install")));
    QVERIFY(gentooEbuildVerifySource.contains(QStringLiteral("profiles/repo_name")));
    QVERIFY(gentooEbuildVerifySource.contains(QStringLiteral("pkgdev manifest -d")));
    QVERIFY(gentooEbuildVerifySource.contains(QStringLiteral("pkgcheck scan --repo")));

    QFile releaseWorkflow(QStringLiteral(LATTE_SOURCE_DIR "/.github/workflows/release.yml"));
    QVERIFY(releaseWorkflow.open(QFile::ReadOnly));
    const QString releaseWorkflowSource = QString::fromUtf8(releaseWorkflow.readAll());
    QVERIFY(releaseWorkflowSource.contains(QStringLiteral("Require successful main pre-release gate")));
    QVERIFY(releaseWorkflowSource.contains(QStringLiteral("Require tag version to match the validated candidate version")));
    QVERIFY(releaseWorkflowSource.contains(QStringLiteral("scripts/release-version.py verify")));
    QVERIFY(releaseWorkflowSource.contains(QStringLiteral("inputs.build_run_id")));
    QVERIFY(releaseWorkflowSource.contains(QStringLiteral("tag_name: ${{ inputs.release_tag || github.ref_name }}")));
    QVERIFY(releaseWorkflowSource.contains(QStringLiteral("run-id: ${{ needs.main-validation.outputs.build_run_id }}")));
    QVERIFY(releaseWorkflowSource.contains(QStringLiteral("pattern: release-package-*")));
    QVERIFY(releaseWorkflowSource.contains(QStringLiteral("needs: [main-validation, apt-repository, gentoo-overlay-publish]")));
    QVERIFY(!releaseWorkflowSource.contains(QStringLiteral("target_commitish:")));
    QVERIFY(releaseWorkflowSource.contains(QStringLiteral("RUIZHI_OVERLAY_TOKEN")));
    QVERIFY(releaseWorkflowSource.contains(QStringLiteral("ruizhi-lab/gentoo-overlay")));
    QVERIFY(releaseWorkflowSource.contains(QStringLiteral("scripts/prepare-gentoo-overlay.sh")));
    QVERIFY(releaseWorkflowSource.contains(QStringLiteral("gentoo-overlay-prepare:")));
    QVERIFY(releaseWorkflowSource.contains(QStringLiteral("Publication scripts come from current main")));
    QVERIFY(releaseWorkflowSource.contains(QStringLiteral("needs: [main-validation]")));
    QVERIFY(releaseWorkflowSource.contains(QStringLiteral("without a Gentoo package build")));
    QVERIFY(!releaseWorkflowSource.contains(QStringLiteral("Dockerfile.gentoo")));
    QVERIFY(!releaseWorkflowSource.contains(QStringLiteral("pkgcheck scan")));
    QVERIFY(!releaseWorkflowSource.contains(QStringLiteral("ebuild \"$new_ebuild\" clean configure compile install")));
    QVERIFY(releaseWorkflowSource.contains(QStringLiteral("workflow_dispatch:")));
    QVERIFY(releaseWorkflowSource.contains(QStringLiteral("inputs.source_sha || github.sha")));
    QVERIFY(releaseWorkflowSource.contains(QStringLiteral("! -name 'latte-dock-ng-9999.ebuild' -delete")));
    QVERIFY(releaseWorkflowSource.contains(QStringLiteral("sha256sum --check \"$RUNNER_TEMP/9999.before\"")));
    QVERIFY(!releaseWorkflowSource.contains(QStringLiteral("rm -rf gentoo-overlay/kde-misc/latte-dock-ng")));

    QFile automaticReleaseWorkflow(QStringLiteral(LATTE_SOURCE_DIR "/.github/workflows/auto-release.yml"));
    QVERIFY(automaticReleaseWorkflow.open(QFile::ReadOnly));
    const QString automaticReleaseWorkflowSource = QString::fromUtf8(automaticReleaseWorkflow.readAll());
    QVERIFY(automaticReleaseWorkflowSource.contains(QStringLiteral("workflows: [Build]")));
    QVERIFY(automaticReleaseWorkflowSource.contains(QStringLiteral("workflow_run.conclusion == 'success'")));
    QVERIFY(automaticReleaseWorkflowSource.contains(QStringLiteral("github.rest.repos.getBranch")));
    QVERIFY(automaticReleaseWorkflowSource.contains(QStringLiteral("github.rest.actions.createWorkflowDispatch")));
    QVERIFY(automaticReleaseWorkflowSource.contains(QStringLiteral("release.yml")));

    QFile buildWorkflow(QStringLiteral(LATTE_SOURCE_DIR "/.github/workflows/build.yml"));
    QVERIFY(buildWorkflow.open(QFile::ReadOnly));
    const QString buildWorkflowSource = QString::fromUtf8(buildWorkflow.readAll());
    QCOMPARE(buildWorkflowSource.count(QStringLiteral("use_mirrors=false")), 2);
    QCOMPARE(buildWorkflowSource.count(QStringLiteral("use_mirrors=true")), 0);
    QCOMPARE(buildWorkflowSource.count(QStringLiteral("--build-arg \"USE_MIRRORS=${use_mirrors}\"")), 2);
    QVERIFY(!buildWorkflowSource.contains(QStringLiteral("--build-arg USE_MIRRORS=${{ matrix.distro == 'opensuse' }}")));
    QVERIFY(buildWorkflowSource.contains(QStringLiteral("name: Retain validated release package")));
    QVERIFY(buildWorkflowSource.contains(QStringLiteral("retention-days: 90")));
    QVERIFY(buildWorkflowSource.contains(QStringLiteral("nixos-release-check:")));
    QVERIFY(buildWorkflowSource.contains(QStringLiteral("nix flake check --impure --print-build-logs")));
    QVERIFY(buildWorkflowSource.contains(QStringLiteral("nix build .#default --impure --no-link --print-build-logs")));
    QVERIFY(buildWorkflowSource.contains(QStringLiteral("nix-env --install \"$NIX_PACKAGE_PATH\"")));
    QVERIFY(buildWorkflowSource.contains(QStringLiteral("nix-env --uninstall latte-dock-ng")));
    QVERIFY(!buildWorkflowSource.contains(QStringLiteral("distro: nixos")));
    QVERIFY(!buildWorkflowSource.contains(QStringLiteral("gentoo-ebuild-preflight:")));
    QVERIFY(!buildWorkflowSource.contains(QStringLiteral("Dockerfile.gentoo")));
    QVERIFY(buildWorkflowSource.contains(QStringLiteral("apt-repository-preflight:")));
    QVERIFY(buildWorkflowSource.contains(QStringLiteral("publish-apt-repository.sh --dry-run")));
    QVERIFY(buildWorkflowSource.contains(QStringLiteral("release_version:")));
    QVERIFY(buildWorkflowSource.contains(QStringLiteral("release-candidate-metadata:")));
    QVERIFY(buildWorkflowSource.contains(QStringLiteral("LATTE_RELEASE_VERSION=\"$LATTE_RELEASE_VERSION\"")));
    QVERIFY(buildWorkflowSource.contains(QStringLiteral("scripts/release-version.py metadata")));

    QFile nixFlake(QStringLiteral(LATTE_SOURCE_DIR "/flake.nix"));
    QVERIFY(nixFlake.open(QFile::ReadOnly));
    const QString nixFlakeSource = QString::fromUtf8(nixFlake.readAll());
    QVERIFY(nixFlakeSource.contains(QStringLiteral("default = package.overrideAttrs")));
    QVERIFY(nixFlakeSource.contains(QStringLiteral("autotests = self.packages.${system}.default")));

    QFile nixosVerifier(QStringLiteral(LATTE_SOURCE_DIR "/docker/verify-nix-nixos.sh"));
    QVERIFY(nixosVerifier.open(QFile::ReadOnly));
    const QString nixosVerifierSource = QString::fromUtf8(nixosVerifier.readAll());
    QVERIFY(nixosVerifierSource.contains(QStringLiteral("nix-build \"${nixpkgs_args[@]}\"")));
    QVERIFY(nixosVerifierSource.contains(QStringLiteral("nix-env \"${nixpkgs_args[@]}\" -if")));
    QVERIFY(nixosVerifierSource.contains(QStringLiteral("nixpkgs=${NIXPKGS_TARBALL_URL}")));
    QVERIFY(nixosVerifierSource.contains(QStringLiteral("develop --impure --command")));
    QVERIFY(!nixosVerifierSource.contains(QStringLiteral("flake check --impure")));
    QVERIFY(!nixosVerifierSource.contains(QStringLiteral("build .#default")));

    QFile aptPublisher(QStringLiteral(LATTE_SOURCE_DIR "/scripts/publish-apt-repository.sh"));
    QVERIFY(aptPublisher.open(QFile::ReadOnly));
    const QString aptPublisherSource = QString::fromUtf8(aptPublisher.readAll());
    QVERIFY(aptPublisherSource.contains(QStringLiteral("--dry-run <output-directory>")));
    QVERIFY(aptPublisherSource.contains(QStringLiteral("APT repository preflight succeeded")));
    QVERIFY(aptPublisherSource.contains(QStringLiteral("gpgv --keyring")));
    QVERIFY(aptPublisherSource.contains(QStringLiteral("gpgv --keyring \"$PWD/latte-dock-ng-archive-keyring.gpg\"")));
    QVERIFY(!aptPublisherSource.contains(QStringLiteral("gpgv --keyring \"$repo_dir/")));

    QFile overlayPublisher(QStringLiteral(LATTE_SOURCE_DIR "/scripts/prepare-gentoo-overlay.sh"));
    QVERIFY(overlayPublisher.open(QFile::ReadOnly));
    const QString overlayPublisherSource = QString::fromUtf8(overlayPublisher.readAll());
    QVERIFY(overlayPublisherSource.contains(QStringLiteral("gentoo_manifest.py")));
    QVERIFY(overlayPublisherSource.contains(QStringLiteral("curl --fail --location --retry 3")));
    QVERIFY(overlayPublisherSource.contains(QStringLiteral("tar -tzf \"$archive\"")));
    QVERIFY(!overlayPublisherSource.contains(QStringLiteral("pkgdev manifest")));
    QVERIFY(!overlayPublisherSource.contains(QStringLiteral("pkgcheck scan")));
    QVERIFY(!overlayPublisherSource.contains(QStringLiteral("ebuild \"$new_ebuild\" clean configure compile install")));
    QVERIFY(overlayPublisherSource.contains(QStringLiteral("${DISTDIR:-$(mktemp -d")));
    QVERIFY(overlayPublisherSource.contains(QStringLiteral("! -name 'latte-dock-ng-9999.ebuild'")));

    QFile packagingCMake(QStringLiteral(LATTE_SOURCE_DIR "/cmake/LattePackaging.cmake"));
    QVERIFY(packagingCMake.open(QFile::ReadOnly));
    const QString packagingCMakeSource = QString::fromUtf8(packagingCMake.readAll());
    QVERIFY(packagingCMakeSource.contains(QStringLiteral("Native RPM packages required by the QML modules")));
    QVERIFY(packagingCMakeSource.contains(QStringLiteral("lib64* package names")));
    QVERIFY(packagingCMakeSource.contains(QStringLiteral("kf6-* packages")));
    QVERIFY(packagingCMakeSource.contains(QStringLiteral("qml6-module-org-kde-kcmutils")));
}

QTEST_MAIN(PackagingContractTest)

#include "packagingcontracttest.moc"
