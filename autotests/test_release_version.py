#!/usr/bin/env python3
"""Exercise release input boundaries and publication identity checks."""

import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.dont_write_bytecode = True

SCRIPT = Path(__file__).resolve().parents[1] / "scripts/release-version.py"
SPEC = importlib.util.spec_from_file_location("release_version", SCRIPT)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class ReleaseVersionTest(unittest.TestCase):
    def test_version_rejects_invalid_input(self):
        for version in ("v1.2.56", "01.2.56", "1.2", "1.2.56-rc1", "1.2.56\n", "1.2.56;echo bad"):
            with self.subTest(version=version), self.assertRaises(ValueError):
                MODULE.validate_version(version)
        self.assertEqual(MODULE.validate_version("1.2.56"), "1.2.56")

    def test_override_does_not_modify_source_defaults(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "CMakeLists.txt").write_text("if(NOT DEFINED VERSION)\n set(VERSION 1.2.55)\nendif()\n")
            (root / "default.nix").write_text('version = "1.2.55";\n')
            self.assertEqual(MODULE.resolve_version(root, "1.2.56"), "1.2.56")
            self.assertEqual(MODULE.resolve_version(root, ""), "1.2.55")
            (root / "default.nix").write_text('version = "1.2.54";\n')
            with self.assertRaises(ValueError):
                MODULE.resolve_version(root, "")

    def test_ebuild_injection_is_idempotent_and_protects_live_version(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "latte-dock-ng-1.2.56.ebuild"
            path.write_text('src_configure() {\n\tlocal mycmakeargs=(\n\t\t-DBUILD_TESTING=OFF\n\t)\n}\n')
            MODULE.configure_ebuild(path)
            first = path.read_bytes()
            MODULE.configure_ebuild(path)
            self.assertEqual(first, path.read_bytes())
            self.assertIn(b'-DVERSION="${PV}"', first)
            live = path.with_name("latte-dock-ng-9999.ebuild")
            live.write_bytes(first)
            with self.assertRaises(ValueError):
                MODULE.configure_ebuild(live)
            self.assertEqual(live.read_bytes(), first)
            path.write_text("src_configure() { cmake_src_configure; }\n")
            with self.assertRaises(ValueError):
                MODULE.configure_ebuild(path)

    def test_metadata_rejects_wrong_version_source_run_and_changed_packages(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            packages = root / "dist"
            packages.mkdir()
            for name in ("a.rpm", "b.rpm", "c.rpm", "d.deb", "e.deb", "f.deb", "g.pkg.tar.zst"):
                (packages / name).write_bytes(name.encode())
            record = root / "candidate.json"
            args = ["--version", "1.2.56", "--sha", "a" * 40, "--run-id", "123",
                    "--packages", str(packages), "--file", str(record)]
            subprocess.run([sys.executable, str(SCRIPT), "metadata", *args], check=True)
            subprocess.run([sys.executable, str(SCRIPT), "verify", *args], check=True)
            for field, value in (("version", "1.2.55"), ("source_sha", "b" * 40), ("build_run_id", "124")):
                original = record.read_text()
                data = json.loads(original)
                data[field] = value
                record.write_text(json.dumps(data))
                result = subprocess.run([sys.executable, str(SCRIPT), "verify", *args], capture_output=True)
                self.assertNotEqual(result.returncode, 0)
                record.write_text(original)
            (packages / "a.rpm").write_bytes(b"changed")
            result = subprocess.run([sys.executable, str(SCRIPT), "verify", *args], capture_output=True)
            self.assertNotEqual(result.returncode, 0)


if __name__ == "__main__":
    unittest.main()
