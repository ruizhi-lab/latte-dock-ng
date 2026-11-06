"""Unit tests for compare_compile_commands.py (Python standard library unittest).

Run from the repository root with:
    python3 -I docs/modernization-results/R8e-tools/test_compare_compile_commands.py -v
"""

from __future__ import annotations

import contextlib
import io
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import compare_compile_commands as cc  # noqa: E402

LEFT_SOURCE = "/left/src"
LEFT_BUILD = "/left/src/build/gcc"
RIGHT_SOURCE = "/right/tree"
RIGHT_BUILD = "/right/tree/build/gcc"

MAIN_OBJECT = "app/CMakeFiles/latte-dock-ng.dir/main.cpp.o"
MENU_OBJECT = "containmentactions/contextmenu/CMakeFiles/plasma_containmentactions_lattecontextmenu.dir/menu.cpp.o"
MOCS_OBJECT = "app/CMakeFiles/latte-dock-ng.dir/latte-dock-ng_autogen/mocs_compilation.cpp.o"
LAUNCHER_OBJECT = "app/CMakeFiles/latte-dock-ng-add-launcher.dir/launcherhelper.cpp.o"


def command(source_root: str, build_root: str, object_path: str, source: str, extra: str = "") -> str:
    """Return a compile command shaped like CMake's output (escaped quotes, -o and depfile flags)."""
    return (
        f"/usr/bin/g++ -I{source_root}/app -I{build_root}/app {extra}"
        f"-DTRANSLATION_DOMAIN=\\\"latte\\\" -std=gnu++20 -MD -MT {object_path} "
        f"-MF {object_path}.d -o {object_path} -c {source}"
    )


def entry(source_root: str, build_root: str, object_path: str, source: str, extra: str = "") -> dict[str, str]:
    return {
        "directory": build_root,
        "file": source,
        "command": command(source_root, build_root, object_path, source, extra),
        "output": object_path,
    }


def left_entries() -> list[dict[str, str]]:
    return [
        entry(LEFT_SOURCE, LEFT_BUILD, MAIN_OBJECT, f"{LEFT_SOURCE}/app/main.cpp"),
        entry(LEFT_SOURCE, LEFT_BUILD, MOCS_OBJECT, f"{LEFT_BUILD}/app/latte-dock-ng_autogen/mocs_compilation.cpp"),
        entry(LEFT_SOURCE, LEFT_BUILD, MENU_OBJECT, f"{LEFT_SOURCE}/containmentactions/contextmenu/menu.cpp"),
        entry(LEFT_SOURCE, LEFT_BUILD, LAUNCHER_OBJECT, f"{LEFT_SOURCE}/app/launcherhelper.cpp"),
    ]


def right_entries() -> list[dict[str, str]]:
    """The same units as left_entries, under another source and build root."""
    return [
        entry(RIGHT_SOURCE, RIGHT_BUILD, MAIN_OBJECT, f"{RIGHT_SOURCE}/app/main.cpp"),
        entry(RIGHT_SOURCE, RIGHT_BUILD, MOCS_OBJECT, f"{RIGHT_BUILD}/app/latte-dock-ng_autogen/mocs_compilation.cpp"),
        entry(RIGHT_SOURCE, RIGHT_BUILD, MENU_OBJECT, f"{RIGHT_SOURCE}/containmentactions/contextmenu/menu.cpp"),
        entry(RIGHT_SOURCE, RIGHT_BUILD, LAUNCHER_OBJECT, f"{RIGHT_SOURCE}/app/launcherhelper.cpp"),
    ]


def normalizer(source: str, build: str) -> cc.Normalizer:
    return cc.Normalizer(source_root=source, build_root=build)


class NormalizerTests(unittest.TestCase):
    def setUp(self) -> None:
        self.normalize = normalizer("/tree", "/tree/build/gcc")

    def test_build_root_is_replaced_before_the_source_root(self) -> None:
        self.assertEqual(self.normalize("-I/tree/build/gcc/app"), "-I<BUILD>/app")
        self.assertEqual(self.normalize("-I/tree/app"), "-I<SRC>/app")

    def test_root_never_matches_a_longer_sibling_name(self) -> None:
        self.assertEqual(self.normalize("/tree-other/app"), "/tree-other/app")
        self.assertEqual(self.normalize("/tree.bak/app"), "/tree.bak/app")

    def test_root_followed_by_an_escaped_quote_is_replaced(self) -> None:
        self.assertEqual(self.normalize('-DLATTE_SOURCE_DIR="/tree"'), '-DLATTE_SOURCE_DIR="<SRC>"')

    def test_root_at_end_of_token_is_replaced(self) -> None:
        self.assertEqual(self.normalize("/tree"), "<SRC>")


class LoadUnitsTests(unittest.TestCase):
    def setUp(self) -> None:
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)

    def write(self, name: str, content: object) -> Path:
        path = self.root / name
        path.write_text(json.dumps(content), encoding="utf-8")
        return path

    def test_keeps_only_executable_units_unless_all_units_is_requested(self) -> None:
        path = self.write("db.json", left_entries())
        left = normalizer(LEFT_SOURCE, LEFT_BUILD)
        executable = cc.load_units(path, left)
        self.assertEqual(sorted(executable), sorted([MAIN_OBJECT, MOCS_OBJECT]))
        everything = cc.load_units(path, left, all_units=True)
        self.assertEqual(len(everything), 4)
        self.assertIn(MENU_OBJECT, everything)

    def test_flags_drop_outputs_and_the_source_but_keep_escaped_defines(self) -> None:
        path = self.write("db.json", left_entries())
        unit = cc.load_units(path, normalizer(LEFT_SOURCE, LEFT_BUILD))[MAIN_OBJECT]
        self.assertEqual(unit.source, "<SRC>/app/main.cpp")
        self.assertIn('-DTRANSLATION_DOMAIN="latte"', unit.flags)
        self.assertIn("-I<SRC>/app", unit.flags)
        self.assertIn("-I<BUILD>/app", unit.flags)
        self.assertIn("-c", unit.flags)  # compile-only mode is a real flag, kept on both sides
        for excluded in ("-o", "-MF", "-MT", "<SRC>/app/main.cpp", f"{MAIN_OBJECT}.d"):
            self.assertNotIn(excluded, unit.flags)

    def test_malformed_databases_are_rejected(self) -> None:
        normalize = normalizer(LEFT_SOURCE, LEFT_BUILD)
        cases = {
            "not_a_list.json": {"file": "x"},
            "no_command.json": [{"directory": LEFT_BUILD, "file": "a.cpp", "output": MAIN_OBJECT}],
            "duplicate.json": left_entries()[:1] * 2,
        }
        for name, content in cases.items():
            with self.subTest(name=name):
                with self.assertRaises(cc.InputError):
                    cc.load_units(self.write(name, content), normalize)
        broken = self.root / "broken.json"
        broken.write_text("{not json", encoding="utf-8")
        with self.assertRaises(cc.InputError):
            cc.load_units(broken, normalize)


class CompareTests(unittest.TestCase):
    def loaded(self, entries: list[dict[str, str]], source: str, build: str, **kwargs: bool) -> dict[str, cc.Unit]:
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        path = Path(directory.name) / "db.json"
        path.write_text(json.dumps(entries), encoding="utf-8")
        return cc.load_units(path, normalizer(source, build), **kwargs)

    def test_same_units_under_other_roots_and_order_are_identical(self) -> None:
        baseline = self.loaded(left_entries(), LEFT_SOURCE, LEFT_BUILD)
        candidate = self.loaded(list(reversed(right_entries())), RIGHT_SOURCE, RIGHT_BUILD)
        result = cc.compare(baseline, candidate)
        self.assertTrue(result.identical)
        self.assertIn("result: identical", cc.render(result))

    def test_an_added_include_directory_is_a_flag_change(self) -> None:
        extra = "-I/right/tree/app/extra "
        candidate_entries = right_entries()
        candidate_entries[0] = entry(RIGHT_SOURCE, RIGHT_BUILD, MAIN_OBJECT, f"{RIGHT_SOURCE}/app/main.cpp", extra)
        result = cc.compare(
            self.loaded(left_entries(), LEFT_SOURCE, LEFT_BUILD),
            self.loaded(candidate_entries, RIGHT_SOURCE, RIGHT_BUILD),
        )
        self.assertFalse(result.identical)
        self.assertEqual([change[0] for change in result.flag_changes], [MAIN_OBJECT])
        self.assertIn("+ -I<SRC>/app/extra", cc.render(result))

    def test_flag_order_only_is_a_note_unless_strict(self) -> None:
        reordered = [
            entry(RIGHT_SOURCE, RIGHT_BUILD, MAIN_OBJECT, f"{RIGHT_SOURCE}/app/main.cpp", "-DFIRST=1 -DSECOND=2 ")
            if item["output"] == MAIN_OBJECT else item
            for item in right_entries()
        ]
        swapped = [
            entry(LEFT_SOURCE, LEFT_BUILD, MAIN_OBJECT, f"{LEFT_SOURCE}/app/main.cpp", "-DSECOND=2 -DFIRST=1 ")
            if item["output"] == MAIN_OBJECT else item
            for item in left_entries()
        ]
        candidate = self.loaded(reordered, RIGHT_SOURCE, RIGHT_BUILD)
        baseline = self.loaded(swapped, LEFT_SOURCE, LEFT_BUILD)
        relaxed = cc.compare(baseline, candidate)
        self.assertTrue(relaxed.identical)
        self.assertEqual(relaxed.order_only, (MAIN_OBJECT,))
        strict = cc.compare(baseline, candidate, strict_order=True)
        self.assertFalse(strict.identical)
        self.assertEqual(len(strict.flag_changes), 1)

    def test_units_present_on_one_side_are_reported(self) -> None:
        baseline = self.loaded(left_entries(), LEFT_SOURCE, LEFT_BUILD, all_units=True)
        candidate_entries = right_entries()[:-1]
        candidate = self.loaded(candidate_entries, RIGHT_SOURCE, RIGHT_BUILD, all_units=True)
        result = cc.compare(baseline, candidate)
        self.assertEqual(result.only_baseline, (LAUNCHER_OBJECT,))
        self.assertEqual(result.only_candidate, ())
        self.assertFalse(result.identical)

    def test_a_different_source_on_the_same_output_is_reported(self) -> None:
        candidate_entries = right_entries()
        candidate_entries[2] = entry(
            RIGHT_SOURCE, RIGHT_BUILD, MENU_OBJECT, f"{RIGHT_SOURCE}/app/settings/menu-copy.cpp"
        )
        result = cc.compare(
            self.loaded(left_entries(), LEFT_SOURCE, LEFT_BUILD, all_units=True),
            self.loaded(candidate_entries, RIGHT_SOURCE, RIGHT_BUILD, all_units=True),
        )
        self.assertEqual(result.only_baseline, ())
        self.assertEqual(
            result.source_changes,
            ((MENU_OBJECT, "<SRC>/containmentactions/contextmenu/menu.cpp", "<SRC>/app/settings/menu-copy.cpp"),),
        )
        self.assertFalse(result.identical)


class MainTests(unittest.TestCase):
    def setUp(self) -> None:
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)

    def database(self, name: str, entries: list[dict[str, str]]) -> Path:
        path = self.root / name
        path.write_text(json.dumps(entries), encoding="utf-8")
        return path

    def run_main(self, baseline: Path, candidate: Path) -> tuple[int, str, str]:
        arguments = [
            str(baseline), str(candidate),
            "--baseline-source-root", LEFT_SOURCE, "--baseline-build-root", LEFT_BUILD,
            "--candidate-source-root", RIGHT_SOURCE, "--candidate-build-root", RIGHT_BUILD,
        ]
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            status = cc.main(arguments)
        return status, out.getvalue(), err.getvalue()

    def test_exit_status_reports_identical_different_and_unreadable(self) -> None:
        left = self.database("left.json", left_entries())
        same = self.database("same.json", right_entries())
        changed_entries = right_entries()
        changed_entries[0] = entry(RIGHT_SOURCE, RIGHT_BUILD, MAIN_OBJECT, f"{RIGHT_SOURCE}/app/other.cpp")
        changed = self.database("changed.json", changed_entries)

        status, output, _ = self.run_main(left, same)
        self.assertEqual(status, 0)
        self.assertIn("result: identical", output)

        status, output, _ = self.run_main(left, changed)
        self.assertEqual(status, 1)
        self.assertIn("result: different", output)

        status, _, error = self.run_main(left, self.root / "missing.json")
        self.assertEqual(status, 2)
        self.assertIn("cannot read compile database", error)


if __name__ == "__main__":
    unittest.main()
