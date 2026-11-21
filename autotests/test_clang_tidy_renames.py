#!/usr/bin/env python3
"""Exercise the renamed-check selection of scripts/test-clang-tidy.py without clang-tidy.

LLVM 23 deprecates `performance-faster-string-find` in favour of
`performance-prefer-single-char-overloads` and reports the deprecation as a clang-tidy-config
diagnostic, which `WarningsAsErrors: '*'` turns into a gate failure. LLVM 18 and 22 do not know
the successor at all. The gate therefore keeps the successor in `.clang-tidy` and substitutes the
working name when the installed tool lacks it.

These tests drive that decision directly: the generated configuration, the substitution report,
and the fail-closed paths when neither spelling exists or the configuration does not track the
pair. No clang-tidy binary and no compile database are needed.
"""

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path

sys.dont_write_bytecode = True

RUNNER = Path(__file__).resolve().parents[1] / "scripts/test-clang-tidy.py"


def load_runner():
    spec = importlib.util.spec_from_file_location("latte_test_clang_tidy", RUNNER)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {RUNNER}")
    module = importlib.util.module_from_spec(spec)
    # Registered before it runs: its dataclass annotations resolve through sys.modules.
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


class RenamedCheckSelection(unittest.TestCase):
    SUCCESSOR = "performance-prefer-single-char-overloads"
    PREDECESSOR = "performance-faster-string-find"

    def setUp(self):
        self.runner = load_runner()
        self.original = self.runner.CONFIG.read_text(encoding="utf-8")

    def write(self, known):
        """Run write_check_config against a fake tool that knows exactly `known`."""
        with tempfile.TemporaryDirectory() as directory:
            config, substituted = self.runner.write_check_config(Path(directory), set(known))
            return config.read_text(encoding="utf-8"), substituted

    def test_new_tool_keeps_the_successor_name(self):
        text, substituted = self.write({self.SUCCESSOR, self.PREDECESSOR})
        self.assertEqual(substituted, {})
        self.assertIn(self.SUCCESSOR, text)
        self.assertNotIn(self.PREDECESSOR, text)

    def test_old_tool_gets_the_working_predecessor(self):
        # LLVM 18 and 22: the successor does not exist, the predecessor does.
        text, substituted = self.write({self.PREDECESSOR})
        self.assertEqual(substituted, {self.SUCCESSOR: self.PREDECESSOR})
        self.assertIn(self.PREDECESSOR, text)
        self.assertNotIn(self.SUCCESSOR, text)

    def test_the_rest_of_the_configuration_is_untouched(self):
        text, _ = self.write({self.PREDECESSOR})
        for line in self.original.splitlines():
            if self.SUCCESSOR in line:
                continue
            self.assertIn(line, text)

    def test_unknown_to_both_spellings_fails_closed(self):
        with self.assertRaises(RuntimeError) as caught:
            self.write({"performance-something-else"})
        self.assertIn(self.SUCCESSOR, str(caught.exception))
        self.assertIn(self.PREDECESSOR, str(caught.exception))

    def test_configuration_that_does_not_track_the_pair_fails_closed(self):
        # If .clang-tidy stops naming the successor, the tracking entry is stale and must be loud
        # rather than silently keeping a check the gate thinks it substitutes.
        stripped = self.original.replace(self.SUCCESSOR, "")
        original_path = self.runner.CONFIG
        with tempfile.TemporaryDirectory() as directory:
            config = Path(directory, ".clang-tidy")
            config.write_text(stripped, encoding="utf-8")
            self.runner.CONFIG = config
            try:
                with self.assertRaises(RuntimeError) as caught:
                    self.runner.write_check_config(Path(directory), {self.PREDECESSOR})
                self.assertIn("RENAMED_CHECKS", str(caught.exception))
            finally:
                self.runner.CONFIG = original_path

    def test_the_gate_names_the_successor_not_the_predecessor(self):
        # The checked-in tuple must match .clang-tidy; both spell the successor, and the gate
        # resolves the predecessor only for a tool that lacks it.
        self.assertIn(self.SUCCESSOR, self.runner.GATED_CHECKS)
        self.assertNotIn(self.PREDECESSOR, self.runner.GATED_CHECKS)
        self.assertIn(self.SUCCESSOR, self.original)
        self.assertEqual(list(self.runner.RENAMED_CHECKS), [self.SUCCESSOR])
        self.assertEqual(self.runner.RENAMED_CHECKS[self.SUCCESSOR], self.PREDECESSOR)


if __name__ == "__main__":
    unittest.main(verbosity=2)
