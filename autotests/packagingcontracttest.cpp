/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QFile>
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
    QVERIFY(dockerSource.contains(QStringLiteral("dockerfile: Dockerfile.gentoo")));
    QVERIFY(dockerSource.contains(QStringLiteral("bash /src/docker/verify-ebuild-gentoo.sh")));
    QVERIFY(!dockerSource.contains(QStringLiteral("/data/projects/latte-dock-ng:/src:ro")));

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
    QVERIFY(gentooDockerfileSource.contains(QStringLiteral("ARG USE_MIRRORS=true")));
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
    QVERIFY(gentooEbuildVerifySource.contains(QStringLiteral(">=kde-plasma/kscreenlocker-6.5:6")));
    QVERIFY(gentooEbuildVerifySource.contains(QStringLiteral("clean configure compile install")));

    QFile releaseWorkflow(QStringLiteral(LATTE_SOURCE_DIR "/.github/workflows/release.yml"));
    QVERIFY(releaseWorkflow.open(QFile::ReadOnly));
    const QString releaseWorkflowSource = QString::fromUtf8(releaseWorkflow.readAll());
    QVERIFY(releaseWorkflowSource.contains(QStringLiteral("Dockerfile.fedora")));
    QVERIFY(releaseWorkflowSource.contains(QStringLiteral("Dockerfile.debian")));
    QVERIFY(releaseWorkflowSource.contains(QStringLiteral("Dockerfile.arch")));
    QVERIFY(!releaseWorkflowSource.contains(QStringLiteral("Dockerfile.gentoo")));
    QVERIFY(!releaseWorkflowSource.contains(QStringLiteral("verify-ebuild-gentoo.sh")));

    QFile packagingCMake(QStringLiteral(LATTE_SOURCE_DIR "/cmake/LattePackaging.cmake"));
    QVERIFY(packagingCMake.open(QFile::ReadOnly));
    const QString packagingCMakeSource = QString::fromUtf8(packagingCMake.readAll());
    QVERIFY(packagingCMakeSource.contains(QStringLiteral("kf6-kcmutils")));
    QVERIFY(packagingCMakeSource.contains(QStringLiteral("qml6-module-org-kde-kcmutils")));
}

QTEST_MAIN(PackagingContractTest)

#include "packagingcontracttest.moc"
