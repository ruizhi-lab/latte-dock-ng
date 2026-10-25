from pathlib import Path
import os
import shutil
import subprocess
import tempfile


repo = Path(__file__).resolve().parents[1]

with tempfile.TemporaryDirectory(prefix="latte-script-test-") as temporary_directory:
    test_root = Path(temporary_directory)
    home = test_root / "home"
    home.mkdir()
    prefix = home / ".local"
    environment = dict(os.environ, HOME=str(home), USER="isolated", SUDO_USER="")

    files = [
        "lib64/plugins/kf6/packagestructure/latte_indicator.so",
        "lib64/plugins/plasma/containmentactions/org.kde.latte.contextmenu.so",
        "share/icons/hicolor/16x16/apps/latte-dock-ng.svg",
        "share/locale/zh_CN/LC_MESSAGES/latte-dock.mo",
        "share/icons/hicolor/16x16/apps/unrelated.svg",
        "lib64/qt6/qml/org/kde/plasma/private/taskmanager/qmldir",
    ]
    for relative_path in files:
        path = prefix / relative_path
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("fixture")

    guide = prefix / "share/latte-dock-ng/help/zh_CN/index.html"
    guide.parent.mkdir(parents=True)
    guide.write_text("offline help")

    cache_files = [
        home / ".cache/latte-dock/legacy-cache",
        home / ".cache/lattedock/qmlcache/compiled.qmlc",
    ]
    for cache_file in cache_files:
        cache_file.parent.mkdir(parents=True, exist_ok=True)
        cache_file.write_text("rebuildable")

    config = home / ".config/lattedockrc"
    config.parent.mkdir(parents=True, exist_ok=True)
    config.write_text("keep")

    source_copy = test_root / "src"
    source_copy.mkdir()
    shutil.copy(repo / "uninstall.sh", source_copy)
    subprocess.run(
        ["bash", str(source_copy / "uninstall.sh"), "--user", "--no-purge-user-data"],
        env=environment,
        check=True,
        stdout=subprocess.DEVNULL,
    )
    for relative_path in files[:4]:
        assert not (prefix / relative_path).exists(), relative_path
    for relative_path in files[4:]:
        assert (prefix / relative_path).exists(), relative_path
    assert config.exists()
    assert not guide.exists()
    assert all(not cache_file.parent.exists() for cache_file in cache_files)

    # Only explicitly marked legacy taskmanager modules belong to Latte.
    legacy_module = prefix / "lib64/qml/org/kde/plasma/private/taskmanager"
    legacy_module.mkdir(parents=True)
    (legacy_module / ".latte-fallback-module").write_text("owned")
    subprocess.run(
        ["bash", str(source_copy / "uninstall.sh"), "--user", "--no-purge-user-data", "--dry-run"],
        env=environment,
        check=True,
        stdout=subprocess.DEVNULL,
    )
    assert legacy_module.exists()
    subprocess.run(
        ["bash", str(source_copy / "uninstall.sh"), "--user", "--no-purge-user-data"],
        env=environment,
        check=True,
        stdout=subprocess.DEVNULL,
    )
    assert not legacy_module.exists()

    # Persisted manifests support custom build directories and reject escapes.
    outside_file = test_root / "unrelated"
    outside_file.write_text("keep")
    custom_file = prefix / "share/custom-artifact"
    custom_file.parent.mkdir(parents=True, exist_ok=True)
    custom_file.write_text("remove")
    manifest = prefix / "share/latte-dock-ng/install-manifest.txt"
    manifest.parent.mkdir(parents=True, exist_ok=True)
    manifest.write_text(f"{custom_file}\n{outside_file}\n")
    subprocess.run(
        ["bash", str(source_copy / "uninstall.sh"), "--user", "--no-purge-user-data"],
        env=environment,
        check=True,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    assert not custom_file.exists()
    assert outside_file.exists()

    # Configure and compile failures must not invoke pre-install cleanup.
    shutil.copy(repo / "install.sh", source_copy)
    (source_copy / "uninstall.sh").write_text('#!/bin/bash\ntouch "${HOME}/cleanup-called"\n')
    fake_tools = test_root / "tools"
    fake_tools.mkdir()
    cmake = fake_tools / "cmake"
    cmake.write_text("#!/bin/bash\nexit 73\n")
    cmake.chmod(0o755)
    environment["PATH"] = f"{fake_tools}:{environment['PATH']}"

    install_command = [
        "bash", str(source_copy / "install.sh"), "--user",
        "--build-dir", str(test_root / "build"), "--jobs", "1",
    ]
    result = subprocess.run(
        install_command,
        env=environment,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    assert result.returncode == 73
    assert not (home / "cleanup-called").exists()

    cmake.write_text('#!/bin/bash\n[[ "$1" != "--build" ]] || exit 74\nexit 0\n')
    result = subprocess.run(
        install_command,
        env=environment,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    assert result.returncode == 74
    assert not (home / "cleanup-called").exists()

    # latte/indicators also holds indicators installed through "Get New
    # Indicators" (KNewStuff). Install, pre-install cleanup and uninstall may
    # only replace Latte's own packages there, never the downloaded ones.
    install_source = test_root / "install-src"
    install_source.mkdir()
    shutil.copy(repo / "install.sh", install_source)
    shutil.copy(repo / "uninstall.sh", install_source)
    shutil.copytree(repo / "indicators", install_source / "indicators")
    for package in ["containment", "plasmoid", "separator", "shell"]:
        (install_source / package / "package").mkdir(parents=True)
    install_tools = test_root / "install-tools"
    install_tools.mkdir()
    succeeding_tools = {
        # Configure, build and install succeed; install writes the manifest
        # that install.sh copies under the prefix.
        "cmake": '#!/bin/bash\n[[ "${1:-}" != "--install" ]] || : > install_manifest.txt\nexit 0\n',
        "kbuildsycoca6": "#!/bin/bash\nexit 0\n",
        "update-desktop-database": "#!/bin/bash\nexit 0\n",
    }
    for name, script in succeeding_tools.items():
        tool = install_tools / name
        tool.write_text(script)
        tool.chmod(0o755)
    install_environment = dict(environment, PATH=f"{install_tools}:{os.environ['PATH']}")

    indicators = prefix / "share/latte/indicators"
    downloaded = indicators / "org.example.downloaded/metadata.json"
    stale_shipped_file = indicators / "default/package/stale.qml"
    stray_build_files = [indicators / "CMakeLists.txt", indicators / "Messages.sh"]

    # Older install.sh versions copied the build files into the data directory;
    # cleanup removes them, and --no-clean must at least not create them.
    def seed_indicators(with_stray_build_files=True):
        seeded = [downloaded, stale_shipped_file]
        if with_stray_build_files:
            seeded += stray_build_files
        for path in seeded:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("fixture")

    def assert_installed_indicators():
        assert downloaded.exists()
        assert (indicators / "default/metadata.json").exists()
        assert (indicators / "org.kde.latte.plasmatabstyle/metadata.json").exists()
        assert not stale_shipped_file.exists()
        assert all(not path.exists() for path in stray_build_files)

    for clean_option in ["--clean", "--no-clean"]:
        seed_indicators(with_stray_build_files=(clean_option == "--clean"))
        subprocess.run(
            ["bash", str(install_source / "install.sh"), "--user", clean_option,
             "--build-dir", str(test_root / "install-build"), "--jobs", "1"],
            env=install_environment,
            check=True,
            stdout=subprocess.DEVNULL,
        )
        assert_installed_indicators()

    seed_indicators()
    subprocess.run(
        ["bash", str(install_source / "uninstall.sh"), "--user", "--no-purge-user-data"],
        env=install_environment,
        check=True,
        stdout=subprocess.DEVNULL,
    )
    assert downloaded.exists()
    assert not (indicators / "default").exists()
    assert not (indicators / "org.kde.latte.plasmatabstyle").exists()
    assert all(not path.exists() for path in stray_build_files)

    # An explicit purge still removes all user data, downloads included.
    subprocess.run(
        ["bash", str(install_source / "uninstall.sh"), "--user", "--purge-user-data"],
        env=install_environment,
        check=True,
        stdout=subprocess.DEVNULL,
    )
    assert not (prefix / "share/latte").exists()

print(
    "PASS: manifestless cleanup, config/shared-module preservation, "
    "persisted manifest, escaping-path rejection, build-failure preservation, "
    "and downloaded-indicator preservation"
)
