#!/usr/bin/env python3
"""Unit tests for tidy_inventory.py (R8c clang-tidy inventory).

Run it by hand; it is not registered in CTest:

    python3 docs/modernization-results/R8c-tools/test_tidy_inventory.py

The end-to-end case needs clang-tidy in PATH and skips without it. It builds a small
fake checkout (a copy of scripts/test-clazy.py, two production sources sharing a
header, an autotest, a .clang-tidy that would turn every finding into an error) and
runs the whole inventory on it.
"""

from __future__ import annotations

import contextlib
import io
import json
import shutil
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import tidy_inventory  # noqa: E402

REPO = Path(__file__).resolve().parents[3]


class ParseOutputTest(unittest.TestCase):
    def test_single_check(self) -> None:
        findings, problems = tidy_inventory.parse_output(
            "/src/app/a.cpp:3:7: warning: use nullptr [modernize-use-nullptr]\n"
            "    int *p = 0;\n"
            "             ^\n"
        )
        self.assertEqual(problems, [])
        self.assertEqual(len(findings), 1)
        self.assertEqual((findings[0].check, findings[0].line, findings[0].column), ("modernize-use-nullptr", 3, 7))
        self.assertEqual(findings[0].message, "use nullptr")

    def test_aliases_count_under_every_name(self) -> None:
        findings, problems = tidy_inventory.parse_output(
            "/src/app/a.h:10:5: warning: member variable 'x' has public visibility "
            "[cppcoreguidelines-non-private-member-variables-in-classes,misc-non-private-member-variables-in-classes]\n"
        )
        self.assertEqual(problems, [])
        self.assertEqual(
            sorted(item.check for item in findings),
            ["cppcoreguidelines-non-private-member-variables-in-classes", "misc-non-private-member-variables-in-classes"],
        )

    def test_analyzer_names_and_brackets_in_the_message(self) -> None:
        findings, _ = tidy_inventory.parse_output(
            "/src/app/a.cpp:1:2: warning: Called C++ object pointer is null [x] [clang-analyzer-core.CallAndMessage]\n"
        )
        self.assertEqual(findings[0].check, "clang-analyzer-core.CallAndMessage")
        self.assertEqual(findings[0].message, "Called C++ object pointer is null [x]")

    def test_compiler_error_is_a_problem_not_a_finding(self) -> None:
        findings, problems = tidy_inventory.parse_output(
            "/src/app/a.cpp:1:1: error: unknown type name 'Foo' [clang-diagnostic-error]\n"
            "Error while processing /src/app/a.cpp.\n"
        )
        self.assertEqual(findings, [])
        self.assertEqual(len(problems), 2)

    def test_compiler_warning_is_neither(self) -> None:
        findings, problems = tidy_inventory.parse_output(
            "/src/app/a.cpp:1:1: warning: unused variable 'x' [clang-diagnostic-unused-variable]\n"
        )
        self.assertEqual((findings, problems), ([], []))

    def test_notes_and_counters_are_ignored(self) -> None:
        findings, problems = tidy_inventory.parse_output(
            "/src/app/a.cpp:2:3: note: expanded from macro 'X'\n12 warnings generated.\nSuppressed 12 warnings.\n"
        )
        self.assertEqual((findings, problems), ([], []))


class CountTest(unittest.TestCase):
    def test_zero_checks_stay_listed(self) -> None:
        findings = [
            tidy_inventory.Finding("a-check", "/src/x.cpp", 1, 1, "m"),
            tidy_inventory.Finding("a-check", "/src/y.cpp", 2, 1, "m"),
            tidy_inventory.Finding("a-check", "/src/y.cpp", 3, 1, "m"),
        ]
        counts = tidy_inventory.per_check_counts(["a-check", "b-check"], findings)
        self.assertEqual(counts, {"a-check": {"findings": 3, "files": 2}, "b-check": {"findings": 0, "files": 0}})

    def test_config_is_json_with_an_escaped_root(self) -> None:
        config = json.loads(tidy_inventory.tidy_config("-*,modernize-*", Path("/data/latte-dock-ng")))
        self.assertEqual(config["Checks"], "-*,modernize-*")
        self.assertEqual(config["WarningsAsErrors"], "")
        self.assertEqual(config["HeaderFilterRegex"], "^/data/latte\\-dock\\-ng/")


@unittest.skipUnless(shutil.which("clang-tidy"), "clang-tidy is not in PATH")
class EndToEndTest(unittest.TestCase):
    def test_inventory_of_a_fake_checkout(self) -> None:
        with tempfile.TemporaryDirectory(prefix="latte-tidy-test-") as temp:
            root = Path(temp, "repo")
            (root / "scripts").mkdir(parents=True)
            shutil.copy(REPO / "scripts/test-clazy.py", root / "scripts/test-clazy.py")
            (root / "app").mkdir()
            (root / "autotests").mkdir()
            build = root / "build"
            build.mkdir()
            # The gate's file must be ignored: with it, every finding would be an error
            # and only one check would run.
            (root / ".clang-tidy").write_text("Checks: '-*,bugprone-use-after-move'\nWarningsAsErrors: '*'\n")
            (root / "app/shared.h").write_text("inline int *shared() { return 0; }\n")
            (root / "app/one.cpp").write_text('#include "shared.h"\nint *one() { return 0; }\n')
            (root / "app/two.cpp").write_text('#include "shared.h"\nint *two() { return shared(); }\n')
            (root / "autotests/three.cpp").write_text("int *three() { return 0; }\n")

            def entry(source: Path, output: str) -> dict:
                return {"directory": str(build), "file": str(source), "output": output,
                        "command": f"clang++ -std=c++20 -c {source} -o {output}"}

            database = [
                entry(root / "app/one.cpp", "app/one.o"),
                entry(root / "app/one.cpp", "autotests/one.o"),  # a test target rebuilding one.cpp
                entry(root / "app/two.cpp", "app/two.o"),
                entry(root / "autotests/three.cpp", "autotests/three.o"),
            ]
            (build / "compile_commands.json").write_text(json.dumps(database))
            report = Path(temp, "report.json")
            summary = Path(temp, "summary.tsv")

            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                status = tidy_inventory.main(
                    ["--build-dir", str(build), "--source-root", str(root), "--checks=-*,modernize-use-nullptr",
                     "--jobs", "2", "--report", str(report), "--summary", str(summary)]
                )

            self.assertEqual(status, 0, output.getvalue())
            document = json.loads(report.read_text())
            self.assertEqual(document["checks"], ["modernize-use-nullptr"])
            self.assertEqual(document["sources"], ["app/one.cpp", "app/two.cpp"])
            self.assertEqual(document["skipped"], {"autotest source": 1, "duplicate compile command of a selected source": 1})
            # one.cpp and two.cpp each once, and the shared header once although two units include it.
            self.assertEqual(
                sorted((item["file"], item["line"]) for item in document["findings"]),
                [("app/one.cpp", 2), ("app/shared.h", 1)],
            )
            self.assertEqual(document["per_check"], {"modernize-use-nullptr": {"findings": 2, "files": 2}})
            self.assertEqual(document["failed_sources"], {})
            self.assertEqual(summary.read_text().splitlines(), ["check\tfindings\tfiles", "modernize-use-nullptr\t2\t2"])


if __name__ == "__main__":
    unittest.main()
