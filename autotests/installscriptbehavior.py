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

print(
    "PASS: manifestless cleanup, config/shared-module preservation, "
    "persisted manifest, escaping-path rejection, and build-failure preservation"
)
