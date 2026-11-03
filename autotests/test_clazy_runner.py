#!/usr/bin/env python3
"""Exercise the fail-closed paths of scripts/test-clazy.py without clazy or a real build.

The gate passes only when clazy proves every gated check with its fixture and reports
nothing in the production sources. Each way it could pass without that proof must fail:
a diagnostic it cannot read, a compile error or a non-zero exit, a translation unit that
hangs, a clazy that ignores the check selection, a check list that differs from
GATED_CHECKS, a negative fixture that is not rejected, and a compile database that does
not describe the checkout (sources outside it, no production source, or a non-Clang build).

The runner is copied into a temporary checkout, so its ROOT, fixture directory and
compile database are that checkout's. A fake `clazy-standalone` is put first on PATH; it
answers from a config.json that each test writes (the version, the check listing, the
diagnostics and exit status per source, or a hang) and logs every call. Parser cases call
the runner's functions with the fake's path; end-to-end cases run the runner as a
subprocess. Real clazy is not needed, and nothing is compiled.
"""

import contextlib
import importlib.util
import io
import json
import os
from collections import Counter
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

sys.dont_write_bytecode = True

RUNNER = Path(__file__).resolve().parents[1] / "scripts/test-clazy.py"

# clazy prints "<file>:<line>:<column>: warning: <text> [-Wclazy-<check>]". "{source}" stands
# for the path clazy was asked to analyze; the fake substitutes it.
FIXTURE_FINDING = "{source}:1:1: warning: negative fixture [-Wclazy-%s]"
QGETENV_FINDING = "{source}:9:5: warning: use qgetenv() [-Wclazy-qgetenv]"

FAKE_CLAZY = r"""#!@PYTHON@ -I
# Offline stand-in for clazy-standalone: answers from config.json beside it and logs each call.
import json
import sys
import time
import uuid
from pathlib import Path

HERE = Path(__file__).resolve().parent
config = json.loads((HERE / "config.json").read_text(encoding="utf-8"))
args = sys.argv[1:]
database = []
if "-p" in args:
    compile_commands = Path(args[args.index("-p") + 1], "compile_commands.json")
    database = [entry["file"] for entry in json.loads(compile_commands.read_text(encoding="utf-8"))]
# One file per call: the runner calls clazy from several threads at once.
(HERE / "calls").mkdir(exist_ok=True)
(HERE / "calls" / f"{uuid.uuid4().hex}.json").write_text(
    json.dumps({"args": args, "database": database}), encoding="utf-8"
)

if "--version" in args:
    print(config["version"])
elif "--list-checks" in args:
    sys.stdout.write(config["listing"])
else:
    name = Path(args[-1]).name
    for line in config["reports"].get(name, []):
        print(line.replace("{source}", args[-1]))
    if name in config["hang"]:
        time.sleep(600)
    sys.exit(config["exit"].get(name, 0))
"""


def load_runner(path):
    """Import the runner from `path`, so that its ROOT is the checkout holding `path`."""
    spec = importlib.util.spec_from_file_location("clazy_runner_under_test", path)
    module = importlib.util.module_from_spec(spec)
    # The dataclasses in the runner look their module up in sys.modules while they are built.
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


class ClazyRunnerTest(unittest.TestCase):
    def setUp(self):
        self._temp = tempfile.TemporaryDirectory()
        self.addCleanup(self._temp.cleanup)
        self.root = Path(self._temp.name).resolve()
        self.bin = self.root / "bin"
        self.bin.mkdir()
        self.fake = self.bin / "clazy-standalone"
        self.fake.write_text(FAKE_CLAZY.replace("@PYTHON@", sys.executable), encoding="utf-8")
        self.fake.chmod(0o755)

        self.runner_path = self.root / "scripts/test-clazy.py"
        self.runner_path.parent.mkdir()
        shutil.copyfile(RUNNER, self.runner_path)
        self.runner = load_runner(self.runner_path)

        for check in self.runner.GATED_CHECKS:
            self.write(f"autotests/clazy/{check}.cpp", "// negative fixture\n")
        for source in ("app/lattecorona.cpp", "app/view.cpp", "autotests/knscompatunittest.cpp"):
            self.write(source, "// source\n")
        self.write_database(self.default_entries())
        self.write_config()

    def write(self, relative, text):
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")

    def entry(self, source, output, compiler="clang++"):
        """A compile database entry; `source` is relative to the checkout, `output` to the build."""
        file = self.root / source
        return {
            "directory": str(self.root / "build"),
            "file": str(file),
            "output": output,
            "command": f"{compiler} -std=c++20 -fPIC -I{self.root}/app -c {file} -o {output}",
        }

    def default_entries(self, compiler="clang++"):
        return [
            # A test target rebuilds view.cpp and comes first; the production entry must still win.
            self.entry("app/view.cpp", "autotests/CMakeFiles/viewtest.dir/view.cpp.o", compiler),
            self.entry("app/view.cpp", "app/CMakeFiles/latte.dir/view.cpp.o", compiler),
            self.entry("app/lattecorona.cpp", "app/CMakeFiles/latte.dir/lattecorona.cpp.o", compiler),
            self.entry(
                "autotests/knscompatunittest.cpp",
                "autotests/CMakeFiles/knscompatunittest.dir/knscompatunittest.cpp.o",
                compiler,
            ),
            self.entry("build/app/moc_view.cpp", "app/CMakeFiles/latte.dir/moc_view.cpp.o", compiler),
        ]

    def write_database(self, entries):
        self.write("build/compile_commands.json", json.dumps(entries))

    def write_config(self, *, listing=None, production=None, silent=(), hang=(), exit_codes=None):
        """Tell the fake what to answer. Each gated check's fixture is rejected unless it is in `silent`."""
        checks = self.runner.GATED_CHECKS
        reports = {f"{check}.cpp": [FIXTURE_FINDING % check] for check in checks if check not in silent}
        reports.update(production or {})
        if listing is None:
            listing = "Enabled checks:\n" + "".join(f"    {check}\n" for check in checks)
        config = {
            "version": "clazy version 1.17 (offline test double)",
            "listing": listing,
            "reports": reports,
            "hang": list(hang),
            "exit": exit_codes or {},
        }
        (self.bin / "config.json").write_text(json.dumps(config), encoding="utf-8")

    def calls(self):
        return [json.loads(path.read_text(encoding="utf-8")) for path in sorted((self.bin / "calls").glob("*.json"))]

    def analyze(self, source="app/view.cpp", checks="qgetenv"):
        """One source through analyze_unit(), the call run_gate() makes for each production source."""
        return self.runner.analyze_unit(
            str(self.fake), self.root / "build", self.root / source, checks, f"^{self.root}/"
        )

    def run_gate(self):
        with contextlib.redirect_stdout(io.StringIO()):
            self.runner.run_gate(str(self.fake), self.default_entries(), self.root / "build", 4)

    def run_cli(self, *, path=None):
        """Run the runner as a subprocess; `path` replaces PATH, which otherwise puts the fake first."""
        env = dict(os.environ)
        env["PATH"] = path if path is not None else os.pathsep.join([str(self.bin), os.environ.get("PATH", "")])
        command = [
            sys.executable, "-I", str(self.runner_path), "--build-dir", str(self.root / "build"), "--jobs", "4",
        ]
        return subprocess.run(command, env=env, capture_output=True, text=True, timeout=120, check=False)

    # Parsing of clazy output, in-process.

    def test_finding_line_is_parsed_into_check_location_and_message(self):
        self.write_config(production={"view.cpp": [QGETENV_FINDING]})
        result = self.analyze()
        self.assertEqual(result.problems, [])
        self.assertEqual(
            result.findings,
            [self.runner.Finding("qgetenv", str(self.root / "app/view.cpp"), 9, 5, "use qgetenv()")],
        )

    def test_header_spelled_two_ways_is_one_finding(self):
        # clazy names a header as its includer spelled it, so one location can arrive under two names.
        (self.root / "app/sub").mkdir()
        self.write_config(production={"view.cpp": [
            f"{self.root}/app/sub/../util.h:3:1: warning: header finding [-Wclazy-qgetenv]",
            f"{self.root}/app/util.h:3:1: warning: header finding [-Wclazy-qgetenv]",
        ]})
        result = self.analyze()
        self.assertEqual(result.problems, [])
        unique = self.runner.unique(result.findings)
        self.assertEqual([item.path for item in unique], [str(self.root / "app/util.h")])

    def test_diagnostic_the_pattern_cannot_read_is_a_problem(self):
        # The column is missing, so the line matches no FINDING pattern although it names a clazy check.
        self.write_config(production={"view.cpp": ["{source}:9: warning: use qgetenv() [-Wclazy-qgetenv]"]})
        result = self.analyze()
        self.assertEqual(result.findings, [])
        self.assertTrue(
            any("unparsed clazy diagnostic" in problem for problem in result.problems), result.problems
        )

    def test_compiler_error_is_a_problem_even_when_clazy_exits_zero(self):
        self.write_config(production={"view.cpp": ["{source}:3:10: fatal error: 'missing.h' file not found"]})
        result = self.analyze()
        self.assertTrue(
            any("'missing.h' file not found" in problem for problem in result.problems), result.problems
        )

    def test_nonzero_exit_without_diagnostics_is_a_problem(self):
        self.write_config(exit_codes={"view.cpp": 1})
        result = self.analyze()
        self.assertTrue(any("clazy exited with 1" in problem for problem in result.problems), result.problems)

    def test_analysis_that_ignores_the_check_selection_is_a_problem(self):
        self.write_config(production={"view.cpp": ["No checks were requested!"]})
        result = self.analyze()
        self.assertIn("clazy ignored the check selection", result.problems)

    def test_hung_translation_unit_fails_the_run(self):
        # The timeout is 900 s in production; the test lowers the module constant to keep the run short.
        self.write_config(hang=["view.cpp"])
        self.runner.TRANSLATION_UNIT_TIMEOUT = 2
        with self.assertRaises(RuntimeError) as raised:
            self.run_gate()
        self.assertIn("app/view.cpp", str(raised.exception))
        self.assertIn("timed out after 2 seconds", str(raised.exception))

    # Check selection, in-process.

    def test_enabled_checks_reads_the_indented_names(self):
        checks = ",".join(self.runner.GATED_CHECKS)
        self.assertEqual(self.runner.enabled_checks(str(self.fake), checks), list(self.runner.GATED_CHECKS))

    def test_clazy_that_accepts_an_unknown_check_silently_is_rejected(self):
        # clazy prints this and exits 0 for a check name it does not know, so the exit status proves nothing.
        self.write_config(listing="No checks were requested!\n")
        with self.assertRaisesRegex(RuntimeError, "rejected the check selection"):
            self.runner.enabled_checks(str(self.fake), ",".join(self.runner.GATED_CHECKS))

    def test_enabled_checks_without_any_name_is_rejected(self):
        self.write_config(listing="Enabled checks:\n")
        with self.assertRaisesRegex(RuntimeError, "enabled no checks"):
            self.runner.enabled_checks(str(self.fake), ",".join(self.runner.GATED_CHECKS))

    # Translation unit selection and the fixture flags, in-process.

    def test_selection_keeps_production_flags_and_skips_other_units(self):
        selected, skipped = self.runner.select_translation_units(self.default_entries(), self.root / "build")
        self.assertEqual(
            [self.runner.unit_path(entry) for entry in selected],
            [self.root / "app/lattecorona.cpp", self.root / "app/view.cpp"],
        )
        self.assertEqual(selected[1]["output"], "app/CMakeFiles/latte.dir/view.cpp.o")
        self.assertEqual(skipped, Counter({
            "autotest source": 1,
            "duplicate compile command of a selected source": 1,
            "generated source in the build directory": 1,
        }))

    def test_entry_outside_the_checkout_is_rejected(self):
        with tempfile.TemporaryDirectory() as elsewhere:
            entries = self.default_entries() + [
                {"directory": elsewhere, "file": "lib.cpp", "output": "lib.o", "command": "clang++ -c lib.cpp"},
            ]
            with self.assertRaisesRegex(RuntimeError, "outside the source tree"):
                self.runner.select_translation_units(entries, self.root / "build")

    def test_database_without_a_production_source_is_rejected(self):
        entries = [self.entry("autotests/knscompatunittest.cpp", "autotests/CMakeFiles/knscompat.dir/knscompat.o")]
        with self.assertRaisesRegex(RuntimeError, "holds no production source"):
            self.runner.select_translation_units(entries, self.root / "build")

    def test_fixture_is_refused_when_the_flags_do_not_name_their_source(self):
        entry = self.entry("app/view.cpp", "app/view.o")
        entry["command"] = "clang++ -std=c++20 -c somewhere-else.cpp"
        with self.assertRaisesRegex(RuntimeError, "cannot derive a compile command"):
            self.runner.retarget(entry, self.root / "fixture.cpp")

    # The runner as a subprocess, with the fake first on PATH.

    def test_clean_run_passes_and_asks_clazy_for_exactly_the_gated_checks(self):
        result = self.run_cli()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn(
            f"2 production sources: no findings from {len(self.runner.GATED_CHECKS)} gated clazy checks",
            result.stdout,
        )

        gated = "--checks=" + ",".join(self.runner.GATED_CHECKS)
        calls = self.calls()
        listings = [call["args"] for call in calls if "--list-checks" in call["args"]]
        self.assertEqual(listings, [[gated, "--list-checks"]])
        self.assertEqual({arg for call in calls for arg in call["args"] if arg.startswith("--checks=")}, {gated})

        # Only the two production sources are analyzed, and each database holds just those two.
        production = [str(self.root / "app/lattecorona.cpp"), str(self.root / "app/view.cpp")]
        analyzed = [call for call in calls if call["args"][-1] in production]
        self.assertEqual(sorted(call["args"][-1] for call in analyzed), sorted(production))
        for call in analyzed:
            self.assertEqual(sorted(call["database"]), sorted(production))

    def test_finding_in_production_code_fails_the_run(self):
        self.write_config(production={"view.cpp": [QGETENV_FINDING]})
        result = self.run_cli()
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("1 clazy finding(s) in gated checks", result.stderr)
        self.assertIn("app/view.cpp:9:5: [qgetenv] use qgetenv()", result.stderr)

    def test_unreadable_diagnostic_fails_the_run(self):
        self.write_config(production={"view.cpp": ["{source}:9: warning: use qgetenv() [-Wclazy-qgetenv]"]})
        result = self.run_cli()
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("unparsed clazy diagnostic", result.stderr)

    def test_enabled_checks_that_differ_from_gated_checks_fail_the_run(self):
        checks = [check for check in self.runner.GATED_CHECKS if check != "qgetenv"] + ["not-a-gated-check"]
        self.write_config(listing="Enabled checks:\n" + "".join(f"    {check}\n" for check in checks))
        result = self.run_cli()
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("but GATED_CHECKS names", result.stderr)

    def test_gated_check_whose_fixture_is_not_rejected_fails_the_run(self):
        self.write_config(silent=["qgetenv"])
        result = self.run_cli()
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("did not reject its negative fixture(s): qgetenv.cpp", result.stderr)

    def test_clazy_that_ignores_the_check_selection_fails_the_run(self):
        self.write_config(listing="No checks were requested!\n")
        result = self.run_cli()
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("clazy rejected the check selection", result.stderr)

    def test_missing_clazy_is_a_usage_error(self):
        empty = self.root / "empty-bin"
        empty.mkdir()
        result = self.run_cli(path=str(empty))
        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
        self.assertIn("clazy-standalone was not found in PATH", result.stderr)

    def test_database_from_a_non_clang_build_is_a_usage_error(self):
        self.write_database(self.default_entries(compiler="g++"))
        result = self.run_cli()
        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
        self.assertIn("not from a Clang build", result.stderr)


if __name__ == "__main__":
    unittest.main()
