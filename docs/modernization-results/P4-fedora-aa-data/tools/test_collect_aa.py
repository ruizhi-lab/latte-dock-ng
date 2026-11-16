"""Focused local tests for /proc process identity parsing and PID reuse checks."""

from pathlib import Path
from contextlib import redirect_stdout
import io
import json
import sys
import tempfile
import unittest
from unittest.mock import patch

from collect_aa import identity, main, proc_stat, sample_deadlines


def stat_line(pid: int, comm: str, starttime: int) -> str:
    fields = ["S", "1", "1", "0", "-1", "0", "0", "0", "0", "0", "0", "0", "0",
              "0", "0", "20", "0", "1", "0", str(starttime), "4096", "2"]
    return f"{pid} ({comm}) " + " ".join(fields) + "\n"


class ProcessIdentityTests(unittest.TestCase):
    def test_stat_parses_comm_with_spaces_and_start_time(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "stat"
            path.write_text(stat_line(42, "latte dock ng", 987654))
            parsed = proc_stat(path)
        self.assertEqual(parsed["starttime"], 987654)
        self.assertEqual(parsed["ppid"], 1)
        self.assertEqual(parsed["utime"], 0)

    def test_identity_rejects_pid_reuse(self):
        with tempfile.TemporaryDirectory() as directory:
            proc_root = Path(directory)
            process = proc_root / "42"
            process.mkdir()
            (process / "stat").write_text(stat_line(42, "latte dock ng", 987655))
            (process / "exe").symlink_to("/tmp/prefix/bin/latte-dock-ng")
            with self.assertRaisesRegex(ValueError, "reused"):
                identity(proc_root, 42, 987654)

    def test_identity_json_mode_needs_no_sampling_paths_and_checks_exe(self):
        with tempfile.TemporaryDirectory() as directory:
            proc_root = Path(directory) / "proc"
            process = proc_root / "42"
            process.mkdir(parents=True)
            (process / "stat").write_text(stat_line(42, "latte dock ng", 987654))
            (process / "exe").symlink_to("/tmp/prefix/bin/latte-dock-ng")
            argv = ["collect_aa.py", "--pid", "42", "--starttime-ticks", "987654",
                    "--exe", "/tmp/prefix/bin/latte-dock-ng", "--proc-root", str(proc_root),
                    "--identity-json"]
            output = io.StringIO()
            with patch.object(sys, "argv", argv), redirect_stdout(output):
                self.assertEqual(main(), 0)
            self.assertEqual(json.loads(output.getvalue())["exe"], "/tmp/prefix/bin/latte-dock-ng")
            argv[argv.index("/tmp/prefix/bin/latte-dock-ng")] = "/tmp/other/latte-dock-ng"
            with patch.object(sys, "argv", argv), redirect_stdout(io.StringIO()):
                with self.assertRaisesRegex(ValueError, "executable changed"):
                    main()

    def test_sixty_second_window_has_sixty_one_boundaries(self):
        deadlines = sample_deadlines(123.0, 60, 1.0)
        self.assertEqual(len(deadlines), 61)
        self.assertEqual(deadlines[0], 123.0)
        self.assertEqual(deadlines[-1] - deadlines[0], 60.0)


if __name__ == "__main__":
    unittest.main()
