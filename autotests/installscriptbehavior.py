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

    # The generated dev-env.sh must not turn an unset XDG_CONFIG_DIRS into the
    # bare "kdedefaults:/etc/xdg" overlay: a dock started from a bare SSH shell
    # with that value made KSycoca clients rewrite the database against each
    # other. Render the heredoc exactly as install.sh writes it, then source it
    # in env -i shells whose first PATH entry is a fake user manager.
    install_lines = (repo / "install.sh").read_text(encoding="utf-8").splitlines()
    heredoc_start = next(index for index, line in enumerate(install_lines)
                         if line.strip() == 'cat > "$env_file" <<ENVEOF')
    heredoc_end = next(index for index in range(heredoc_start + 1, len(install_lines))
                       if install_lines[index] == "ENVEOF")
    dev_env_template = "\n".join(install_lines[heredoc_start + 1:heredoc_end]) + "\n"

    dev_env_root = test_root / "dev-env"
    dev_env_root.mkdir()
    render = dev_env_root / "render.sh"
    render.write_text("kde_install_qmldir=/x\ncat <<ENVEOF\n" + dev_env_template + "ENVEOF\n", encoding="utf-8")
    dev_env = dev_env_root / "dev-env.sh"
    dev_env.write_text(
        subprocess.run(["bash", str(render)], check=True, capture_output=True, text=True).stdout,
        encoding="utf-8",
    )
    subprocess.run(["bash", "-n", str(dev_env)], check=True)

    dev_env_home = dev_env_root / "home"
    dev_env_home.mkdir()
    fake_bin = dev_env_root / "bin"
    fake_bin.mkdir()
    session_value = "/usr/share/kde-settings/kde-profile/default/xdg"
    defaults_overlay = f"{dev_env_home}/.config/kdedefaults:/etc/xdg"

    def fake_user_manager(lines, status=0):
        body = "".join(f"{line}\n" for line in lines)
        systemctl = fake_bin / "systemctl"
        systemctl.write_text(
            '#!/bin/bash\n[[ "$*" == "--user show-environment" ]] || exit 2\n'
            f"cat <<'MANAGER'\n{body}MANAGER\nexit {status}\n",
            encoding="utf-8",
        )
        systemctl.chmod(0o755)

    def sourced_xdg_config_dirs(variables):
        """XDG_CONFIG_DIRS after sourcing dev-env.sh in an env -i shell; None when unset."""
        result = subprocess.run(
            ["env", "-i", f"PATH={fake_bin}{os.pathsep}{os.environ['PATH']}", f"HOME={dev_env_home}", *variables,
             "bash", "-c", 'source "$1"; printf "%s" "${XDG_CONFIG_DIRS-__unset__}"', "bash", str(dev_env)],
            capture_output=True,
            text=True,
            check=True,
        )
        return None if result.stdout == "__unset__" else result.stdout

    # A set value keeps today's overlay in front of it; the user manager is not read.
    fake_user_manager([f"XDG_CONFIG_DIRS={session_value}"])
    value = sourced_xdg_config_dirs(["XDG_CONFIG_DIRS=/custom/config:/etc/alt"])
    assert value == f"{defaults_overlay}:/custom/config:/etc/alt", value

    # Unset: the user manager's value is used exactly as reported.
    fake_user_manager(["PATH=/usr/bin", f"XDG_CONFIG_DIRS={session_value}", "LANG=zh_CN.UTF-8"])
    value = sourced_xdg_config_dirs([])
    assert value == session_value, value
    # An empty value counts as unset (XDG Base Directory specification).
    value = sourced_xdg_config_dirs(["XDG_CONFIG_DIRS="])
    assert value == session_value, value

    # Unset and no value from the user manager, whether it omits the line or
    # fails: the variable stays unset instead of becoming the overlay alone.
    fake_user_manager(["PATH=/usr/bin", "LANG=zh_CN.UTF-8"])
    value = sourced_xdg_config_dirs([])
    assert value is None, value
    value = sourced_xdg_config_dirs(["XDG_CONFIG_DIRS="])
    assert value is None, value
    fake_user_manager([], status=1)
    value = sourced_xdg_config_dirs([])
    assert value is None, value

print(
    "PASS: manifestless cleanup, config/shared-module preservation, "
    "persisted manifest, escaping-path rejection, build-failure preservation, "
    "downloaded-indicator preservation, and dev-env XDG_CONFIG_DIRS handling"
)
