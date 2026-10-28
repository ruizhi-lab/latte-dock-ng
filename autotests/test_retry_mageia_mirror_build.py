#!/usr/bin/env python3
"""Exercise the retry policy of scripts/retry-mageia-mirror-build.sh.

The script wraps `docker build` for the Mageia Cauldron CI images. Cauldron's
mirrors sync independently, so a build can fail on a package that one mirror
lacks, or on a mirror whose metadata is unusable while it updates. Those two
mirror failures are retried; every other failure must surface at once. The tests
run the real script around fake build commands and a fake `sleep`, so they need
neither Docker nor a network and do not wait out the back-off.
"""

import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile
import unittest

sys.dont_write_bytecode = True

SCRIPT = Path(__file__).resolve().parents[1] / "scripts/retry-mageia-mirror-build.sh"

# The two failures of Build run 40703989933, as dnf printed them.
PACKAGE_MIRROR_404 = (
    "Librepo error: Cannot download lib64imobiledevice-glue1.0_0-1.3.3-1.mga11.x86_64.rpm: "
    "All mirrors were tried"
)
METADATA_UNUSABLE = (
    'Failed to download metadata (baseurl: "https://mirrors.kernel.org/mageia/distrib/cauldron/'
    'x86_64/media/core/release/") for repository "cauldron-x86_64": Usable URL not found'
)
UNRELATED_FAILURE = "CMake Error: Could not find a package configuration file provided by Qt6"


class RetryMageiaMirrorBuildTest(unittest.TestCase):
    def setUp(self):
        self._temp = tempfile.TemporaryDirectory()
        self.addCleanup(self._temp.cleanup)
        self.dir = Path(self._temp.name)
        # The script sleeps 20 s and 40 s between attempts; a no-op sleep keeps that out of the test.
        fake_bin = self.dir / "bin"
        fake_bin.mkdir()
        fake_sleep = fake_bin / "sleep"
        fake_sleep.write_text("#!/bin/sh\nexit 0\n")
        fake_sleep.chmod(fake_sleep.stat().st_mode | stat.S_IXUSR)
        self.env = dict(os.environ, PATH=f"{fake_bin}{os.pathsep}{os.environ['PATH']}")

    def run_script(self, outcomes):
        """Run the script around a build that follows `outcomes`, one entry per attempt.

        None is a successful build; a string is printed before the build fails. Returns
        the completed process and the number of attempts the script made.
        """
        counter = self.dir / "attempts"
        counter.write_text("0")
        build = self.dir / "build.sh"
        lines = ["#!/bin/sh", f'n=$(cat "{counter}")', 'n=$((n + 1))', f'echo "$n" > "{counter}"', 'case "$n" in']
        for number, outcome in enumerate(outcomes, start=1):
            lines.append(f"{number}) echo 'build ok'; exit 0 ;;" if outcome is None
                         else f"{number}) echo '{outcome}'; exit 1 ;;")
        lines += ["*) echo 'unexpected extra attempt'; exit 99 ;;", "esac", ""]
        build.write_text("\n".join(lines))
        build.chmod(build.stat().st_mode | stat.S_IXUSR)
        result = subprocess.run(["bash", str(SCRIPT), str(build)], env=self.env,
                                capture_output=True, text=True, timeout=30)
        return result, int(counter.read_text())

    def test_successful_build_runs_once(self):
        result, attempts = self.run_script([None])
        self.assertEqual((result.returncode, attempts), (0, 1), result.stdout + result.stderr)
        self.assertNotIn("retrying", result.stdout)

    def test_package_missing_on_a_mirror_is_retried(self):
        result, attempts = self.run_script([PACKAGE_MIRROR_404, None])
        self.assertEqual((result.returncode, attempts), (0, 2), result.stdout + result.stderr)
        self.assertIn("retrying build (1/3)", result.stdout)

    def test_unusable_mirror_metadata_is_retried(self):
        result, attempts = self.run_script([METADATA_UNUSABLE, None])
        self.assertEqual((result.returncode, attempts), (0, 2), result.stdout + result.stderr)
        self.assertIn("retrying build (1/3)", result.stdout)

    def test_the_failure_sequence_of_run_40703989933_recovers(self):
        # A missing package, then unusable metadata: the second used to end the job as "unrelated".
        result, attempts = self.run_script([PACKAGE_MIRROR_404, METADATA_UNUSABLE, None])
        self.assertEqual((result.returncode, attempts), (0, 3), result.stdout + result.stderr)

    def test_gives_up_after_three_mirror_failures(self):
        result, attempts = self.run_script([PACKAGE_MIRROR_404, METADATA_UNUSABLE, PACKAGE_MIRROR_404])
        self.assertEqual((result.returncode, attempts), (1, 3), result.stdout + result.stderr)
        self.assertNotIn("unexpected extra attempt", result.stdout)

    def test_unrelated_failure_is_not_retried(self):
        result, attempts = self.run_script([UNRELATED_FAILURE, None])
        self.assertEqual((result.returncode, attempts), (1, 1), result.stdout + result.stderr)
        self.assertNotIn("retrying", result.stdout)

    def test_a_mirror_failure_that_turns_into_another_failure_is_not_retried(self):
        result, attempts = self.run_script([PACKAGE_MIRROR_404, UNRELATED_FAILURE, None])
        self.assertEqual((result.returncode, attempts), (1, 2), result.stdout + result.stderr)

    def test_a_build_command_is_required(self):
        result = subprocess.run(["bash", str(SCRIPT)], env=self.env, capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode, 2)
        self.assertIn("Usage:", result.stderr)


if __name__ == "__main__":
    unittest.main()
