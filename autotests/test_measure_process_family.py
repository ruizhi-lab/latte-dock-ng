#!/usr/bin/env python3
"""Exercise the KSycoca validity rule of scripts/measure-process-family.py without a dock.

R5 showed that a dock launched with a shorter XDG_CONFIG_DIRS than its Plasma session
starts a KSycoca rewrite loop, and the performance protocol makes any sampling window
that overlaps a rebuild invalid. The collector must therefore report a window as invalid
when a ~/.cache/ksycoca6_* file's mtime changes during it, including a file that appears
or vanishes. These tests cover that rule in isolation; the collector's process sampling
needs a live process family and is not run here.

The module is loaded from scripts/ by path, so no build, process or desktop is needed.
"""

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path

sys.dont_write_bytecode = True

MODULE_PATH = Path(__file__).resolve().parents[1] / "scripts/measure-process-family.py"


def load_collector():
    spec = importlib.util.spec_from_file_location("measure_process_family", MODULE_PATH)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {MODULE_PATH}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


class SycocaRewriteRule(unittest.TestCase):
    def setUp(self):
        self.collector = load_collector()

    def test_unchanged_snapshot_is_not_a_rewrite(self):
        snapshot = {"ksycoca6_en_x": 100}
        self.assertEqual(self.collector.sycoca_rewrites(snapshot, dict(snapshot)), [])

    def test_changed_mtime_is_a_rewrite(self):
        self.assertEqual(
            self.collector.sycoca_rewrites({"ksycoca6_en_x": 100}, {"ksycoca6_en_x": 101}),
            ["ksycoca6_en_x"],
        )

    def test_vanished_cache_file_is_a_rewrite(self):
        self.assertEqual(self.collector.sycoca_rewrites({"ksycoca6_en_x": 100}, {}), ["ksycoca6_en_x"])

    def test_appeared_cache_file_is_a_rewrite(self):
        self.assertEqual(self.collector.sycoca_rewrites({}, {"ksycoca6_de_y": 5}), ["ksycoca6_de_y"])

    def test_reports_are_sorted_and_deduplicated(self):
        start = {"ksycoca6_b": 1, "ksycoca6_a": 1}
        end = {"ksycoca6_b": 2, "ksycoca6_a": 2}
        self.assertEqual(self.collector.sycoca_rewrites(start, end), ["ksycoca6_a", "ksycoca6_b"])

    def test_lock_files_are_not_cache_versions(self):
        # A .lock file is transient kbuildsycoca6 housekeeping, not a database version, so the
        # snapshot must not include it and cannot treat its churn as a rewrite.
        versions = self._snapshot_of(files={"ksycoca6_en_x": 10, "ksycoca6_en_x.lock": 10})
        self.assertEqual(sorted(versions), ["ksycoca6_en_x"])

    def _snapshot_of(self, files):
        with tempfile.TemporaryDirectory() as home:
            cache = Path(home, ".cache")
            cache.mkdir()
            for name in files:
                (cache / name).write_text("x", encoding="utf-8")
            original = self.collector.KSYCOCA_CACHE_GLOB
            self.collector.KSYCOCA_CACHE_GLOB = str(cache / "ksycoca6_*")
            try:
                return self.collector.sycoca_versions()
            finally:
                self.collector.KSYCOCA_CACHE_GLOB = original


if __name__ == "__main__":
    unittest.main(verbosity=2)
