#!/usr/bin/env python3
"""Regression tests for stable qmllint report parsing and comparison."""

from __future__ import annotations

import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
FIXTURES = Path(__file__).resolve().parent / "fixtures" / "qmllint-baseline"
MODULE_SPEC = importlib.util.spec_from_file_location(
    "qmllint_baseline", ROOT / "scripts" / "qmllint-baseline.py"
)
assert MODULE_SPEC is not None and MODULE_SPEC.loader is not None
qmllint_baseline = importlib.util.module_from_spec(MODULE_SPEC)
MODULE_SPEC.loader.exec_module(qmllint_baseline)


class QmllintBaselineTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory()
        self.root = Path(self.temp_dir.name).resolve()
        (self.root / "a.qml").write_text("Item {}\n", encoding="utf-8")
        (self.root / "b.qml").write_text("Item {}\n", encoding="utf-8")
        self.expected = json.loads((FIXTURES / "expected-files.json").read_text(encoding="utf-8"))
        self.manifest = json.loads((FIXTURES / "complete-manifest.json").read_text(encoding="utf-8"))
        self.json_fixture = json.loads((FIXTURES / "qmllint-json.json").read_text(encoding="utf-8"))
        self.text_fixture = (FIXTURES / "qmllint-human.log").read_text(encoding="utf-8")

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def build_json_report(self, data=None, manifest=None, tool_version="qmllint 6.11.2"):
        return qmllint_baseline.build_report(
            json.dumps(self.json_fixture if data is None else data), "json",
            self.manifest if manifest is None else manifest, self.expected, self.root,
            tool_version, ["<build-qml>", "<staged-qml>", "system-qml"],
            {"qt": "6.11.2", "plasma": "6.7.5"},
        )

    def build_text_report(self, raw=None, manifest=None):
        return qmllint_baseline.build_report(
            self.text_fixture if raw is None else raw, "text",
            self.manifest if manifest is None else manifest, self.expected, self.root,
            "qmllint 6.11.2", ["<build-qml>", "<staged-qml>", "system-qml"],
            {"qt": "6.11.2", "plasma": "6.7.5"},
        )

    def test_json_preserves_duplicate_multiplicity_and_zero_warning_files(self) -> None:
        report = self.build_json_report()
        self.assertEqual(len(report["diagnostics"]), 1)
        self.assertEqual(report["diagnostics"][0]["count"], 2)
        self.assertEqual(report["diagnostics"][0]["file"], "a.qml")
        self.assertEqual(report["manifest"]["attemptedFiles"], self.expected)

    def test_json_ignores_line_shifts_and_temporary_import_prefixes(self) -> None:
        baseline = self.build_json_report()
        shifted = copy.deepcopy(self.json_fixture)
        shifted["files"][0]["warnings"][0]["line"] = 200
        shifted["files"][0]["warnings"][0]["message"] = "Could not inspect /tmp/build-one/imports/Qt.qml"
        shifted["files"][0]["warnings"][1]["line"] = 201
        shifted["files"][0]["warnings"][1]["message"] = "Could not inspect /tmp/build-one/imports/Qt.qml"
        baseline = self.build_json_report(shifted)
        other = copy.deepcopy(shifted)
        other["files"][0]["warnings"][0]["line"] = 4
        other["files"][0]["warnings"][1]["line"] = 7
        for warning in other["files"][0]["warnings"]:
            warning["message"] = "Could not inspect /tmp/build-two/imports/Qt.qml"
        current = self.build_json_report(other)
        added, removed = qmllint_baseline.compare_reports(baseline, current)
        self.assertEqual(added, [])
        self.assertEqual(removed, [])

    def test_new_identity_fails_even_when_another_identity_disappears(self) -> None:
        baseline = self.build_json_report()
        changed = copy.deepcopy(self.json_fixture)
        changed["files"][0]["warnings"] = [
            {"id": "unused-imports", "type": "info", "message": "Unused import QtQuick", "line": 2}
        ]
        current = self.build_json_report(changed)
        added, removed = qmllint_baseline.compare_reports(baseline, current)
        self.assertEqual(len(added), 1)
        self.assertEqual(len(removed), 1)

    def test_removed_identity_is_a_clean_comparison(self) -> None:
        baseline = self.build_json_report()
        changed = copy.deepcopy(self.json_fixture)
        changed["files"][0]["warnings"] = []
        current = self.build_json_report(changed)
        added, removed = qmllint_baseline.compare_reports(baseline, current)
        self.assertEqual(added, [])
        self.assertEqual(removed[0]["count"], 2)

    def test_meaningful_identifier_change_is_a_new_identity(self) -> None:
        baseline = self.build_json_report()
        changed = copy.deepcopy(self.json_fixture)
        for warning in changed["files"][0]["warnings"]:
            warning["message"] = "Unqualified access to appTitle"
        added, removed = qmllint_baseline.compare_reports(baseline, self.build_json_report(changed))
        self.assertEqual(added[0]["message"], "Unqualified access to appTitle")
        self.assertEqual(removed[0]["message"], "Unqualified access to titleText")

    def test_tool_version_change_requires_a_new_comparable_baseline(self) -> None:
        with self.assertRaisesRegex(qmllint_baseline.BaselineError, "fingerprint changed"):
            qmllint_baseline.compare_reports(self.build_json_report(), self.build_json_report(tool_version="qmllint 6.12.0"))

    def test_environment_or_import_root_change_requires_a_new_baseline(self) -> None:
        baseline = self.build_json_report()
        changed = self.build_json_report()
        changed["environment"]["attributes"]["qt"] = "6.12.0"
        with self.assertRaisesRegex(qmllint_baseline.BaselineError, "environment fingerprint changed"):
            qmllint_baseline.compare_reports(baseline, changed)

    def test_text_parser_counts_categories_and_ignores_source_locations(self) -> None:
        baseline = self.build_text_report()
        shifted = self.text_fixture.replace("a.qml:2:14", "a.qml:200:4").replace("b.qml:9:3", "b.qml:1:99")
        current = self.build_text_report(shifted)
        added, removed = qmllint_baseline.compare_reports(baseline, current)
        self.assertEqual(added, [])
        self.assertEqual(removed, [])
        self.assertEqual([item["category"] for item in baseline["diagnostics"]], ["unqualified", "missing-property"])

    def test_malformed_and_truncated_input_fail_closed(self) -> None:
        with self.assertRaisesRegex(qmllint_baseline.BaselineError, "malformed or truncated"):
            qmllint_baseline.build_report(
                "{\"revision\":4", "json", self.manifest, self.expected, self.root,
                "qmllint 6.11.2", ["<build-qml>"],
            )
        with self.assertRaisesRegex(qmllint_baseline.BaselineError, "final record terminator"):
            self.build_text_report(self.text_fixture.rstrip())
        with self.assertRaisesRegex(qmllint_baseline.BaselineError, "unrecognized"):
            self.build_text_report("Warning: a.qml:broken [unqualified]\n")

    def test_missing_or_failed_coverage_fails_closed(self) -> None:
        missing = copy.deepcopy(self.manifest)
        missing["attemptedFiles"].remove("b.qml")
        missing["fileExitCodes"].pop("b.qml")
        with self.assertRaisesRegex(qmllint_baseline.BaselineError, "incomplete file coverage"):
            self.build_json_report(manifest=missing)
        failed = copy.deepcopy(self.manifest)
        failed["fileExitCodes"]["a.qml"] = 2
        with self.assertRaisesRegex(qmllint_baseline.BaselineError, "non-zero file exit codes"):
            self.build_json_report(manifest=failed)

    def test_malformed_manifest_and_json_coverage_fail_closed(self) -> None:
        invalid = copy.deepcopy(self.manifest)
        invalid["complete"] = False
        with self.assertRaisesRegex(qmllint_baseline.BaselineError, "not marked complete"):
            self.build_json_report(manifest=invalid)
        fewer_results = copy.deepcopy(self.json_fixture)
        fewer_results["files"].pop()
        with self.assertRaisesRegex(qmllint_baseline.BaselineError, "coverage mismatch"):
            self.build_json_report(fewer_results)

    def test_valid_zero_diagnostic_run_is_comparable(self) -> None:
        empty = copy.deepcopy(self.json_fixture)
        for result in empty["files"]:
            result["warnings"] = []
            result["success"] = True
        report = self.build_json_report(empty)
        self.assertEqual(report["diagnostics"], [])
        self.assertEqual(qmllint_baseline.compare_reports(report, report), ([], []))

    def test_empty_text_run_is_valid_when_manifest_proves_full_success(self) -> None:
        report = self.build_text_report("")
        self.assertEqual(report["diagnostics"], [])
        self.assertEqual(qmllint_baseline.compare_reports(report, report), ([], []))

    def test_unknown_json_revision_fails_closed(self) -> None:
        changed = copy.deepcopy(self.json_fixture)
        changed["revision"] = 5
        with self.assertRaisesRegex(qmllint_baseline.BaselineError, "unsupported qmllint JSON revision"):
            self.build_json_report(changed)

    def test_capture_and_compare_cli_return_status_for_regressions(self) -> None:
        with tempfile.TemporaryDirectory() as output_dir:
            directory = Path(output_dir)
            diagnostics_path = directory / "qmllint.json"
            manifest_path = directory / "manifest.json"
            expected_path = directory / "expected.json"
            baseline_path = directory / "baseline.json"
            current_path = directory / "current.json"
            diagnostics_path.write_text(json.dumps(self.json_fixture), encoding="utf-8")
            manifest_path.write_text(json.dumps(self.manifest), encoding="utf-8")
            expected_path.write_text(json.dumps(self.expected), encoding="utf-8")

            common_capture = [
                sys.executable, str(ROOT / "scripts" / "qmllint-baseline.py"), "capture",
                "--input", str(diagnostics_path), "--format", "json",
                "--manifest", str(manifest_path), "--expected-files", str(expected_path),
                "--source-root", str(self.root), "--tool-version", "qmllint 6.11.2",
                "--import-root", "<build-qml>", "--environment", "qt=6.11.2",
            ]
            baseline_capture = subprocess.run(
                common_capture + ["--output", str(baseline_path)], capture_output=True, text=True, check=False
            )
            self.assertEqual(baseline_capture.returncode, 0, baseline_capture.stderr)

            changed = copy.deepcopy(self.json_fixture)
            changed["files"][0]["warnings"].append({
                "id": "missing-property", "type": "warning", "message": "Member missingValue not found", "line": 4
            })
            diagnostics_path.write_text(json.dumps(changed), encoding="utf-8")
            current_capture = subprocess.run(
                common_capture + ["--output", str(current_path)], capture_output=True, text=True, check=False
            )
            self.assertEqual(current_capture.returncode, 0, current_capture.stderr)

            comparison = subprocess.run(
                [sys.executable, str(ROOT / "scripts" / "qmllint-baseline.py"), "compare",
                 "--baseline", str(baseline_path), "--current", str(current_path)],
                capture_output=True, text=True, check=False,
            )
            self.assertEqual(comparison.returncode, 1, comparison.stdout + comparison.stderr)


if __name__ == "__main__":
    unittest.main(verbosity=2)
