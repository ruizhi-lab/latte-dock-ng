#!/usr/bin/env python3
"""Exercise the apt retry helper that .github/workflows/build.yml writes for the KDE Neon steps.

The Build jobs install about 300 packages from the KDE Neon repository. On 2026-11-04 the Neon
origin refused the connections for roughly 290 of them while `apt-get install` ran, and all four
Build jobs ended with "Unable to fetch some archives" after a single attempt. The workflow now
installs through `apt-install-retry`, which tries again after refreshing the indexes.

The helper is not a file of the repository: the "Install KDE Neon repository" step writes it with
a heredoc, because the repository is checked out only after the dependencies are installed. This
test cuts the helper out of the workflow, so it checks the script that the runners execute, and
runs it against a fake `apt-get` and a fake `sleep` that log their calls. Nothing here needs apt,
a network or a container; the helper is run with /bin/sh and, when it exists, with dash, which is
/bin/sh of the Ubuntu image the Build jobs use.
"""

import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

WORKFLOW = Path(__file__).resolve().parents[1] / ".github/workflows/build.yml"

FAKE_APT_GET = r"""#!/bin/sh
# Logs every call as "apt-get [arg] [arg]..." and fails the first FAKE_INSTALL_FAILURES installs.
# Shell builtins only: the helper runs with a PATH that holds nothing but the fakes (see run_helper).
printf 'apt-get' >> "$FAKE_LOG"
printf ' [%s]' "$@" >> "$FAKE_LOG"
printf '\n' >> "$FAKE_LOG"
case "$1" in
update)
    [ "${FAKE_UPDATE_FAILS:-0}" = 1 ] && exit 100
    exit 0
    ;;
install)
    count=0
    [ -f "$FAKE_STATE" ] && read -r count < "$FAKE_STATE"
    count=$((count + 1))
    echo "$count" > "$FAKE_STATE"
    [ "$count" -le "${FAKE_INSTALL_FAILURES:-0}" ] && exit 100
    exit 0
    ;;
esac
exit 2
"""

FAKE_SLEEP = r"""#!/bin/sh
echo "sleep $*" >> "$FAKE_LOG"
"""

INSTALL = "apt-get [install] [-y] [--no-install-recommends] [pkg-a] [pkg b]"
UPDATE = "apt-get [update]"


def helper_script():
    """Return the text of the helper as the runner writes it, without the workflow's indentation."""
    text = WORKFLOW.read_text(encoding="utf-8")
    match = re.search(
        r"^(?P<indent> *)cat > /usr/local/bin/apt-install-retry <<'EOF'\n(?P<body>.*?)^(?P=indent)EOF\n",
        text,
        re.M | re.S,
    )
    if match is None:
        raise AssertionError("build.yml does not write /usr/local/bin/apt-install-retry")

    indent = match["indent"]
    return "".join(line[len(indent):] if line.startswith(indent) else line for line in match["body"].splitlines(keepends=True))


def shells():
    sh = "/bin/sh" if os.path.exists("/bin/sh") else shutil.which("sh")
    found = [("sh", sh)] if sh else []
    dash = shutil.which("dash")
    if dash and os.path.realpath(dash) != os.path.realpath(sh or ""):
        found.append(("dash", dash))
    return found


class AptInstallRetryTest(unittest.TestCase):
    def setUp(self):
        self.assertTrue(shells(), "no POSIX shell found to run the helper")
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        root = Path(self.temp.name)
        self.log = root / "calls.log"
        self.state = root / "installs"
        bin_dir = root / "bin"
        bin_dir.mkdir()

        for name, content in (("apt-get", FAKE_APT_GET), ("sleep", FAKE_SLEEP)):
            tool = bin_dir / name
            tool.write_text(content, encoding="utf-8")
            tool.chmod(0o755)

        self.helper = root / "apt-install-retry"
        self.helper.write_text(helper_script(), encoding="utf-8")
        self.helper.chmod(0o755)
        self.bin_dir = bin_dir

    def run_helper(self, shell, install_failures=0, update_fails=False):
        self.log.write_text("", encoding="utf-8")
        self.state.unlink(missing_ok=True)
        env = {
            # Nothing but the fakes: a build sandbox such as Nix has no coreutils in /usr/bin or /bin, and the
            # helper must work with shell builtins and these two commands alone. An external command used by
            # the helper (or by a fake) fails here on every system instead of passing where one is installed.
            "PATH": str(self.bin_dir),
            "FAKE_LOG": str(self.log),
            "FAKE_STATE": str(self.state),
            "FAKE_INSTALL_FAILURES": str(install_failures),
            "FAKE_UPDATE_FAILS": "1" if update_fails else "0",
        }
        result = subprocess.run([shell, str(self.helper), "pkg-a", "pkg b"], env=env, capture_output=True, text=True, timeout=30)
        return result, self.log.read_text(encoding="utf-8").splitlines()

    def test_success_on_the_first_attempt_installs_once(self):
        for name, shell in shells():
            with self.subTest(shell=name):
                result, calls = self.run_helper(shell)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual(calls, [INSTALL])

    def test_package_arguments_are_passed_through_unchanged(self):
        for name, shell in shells():
            with self.subTest(shell=name):
                _, calls = self.run_helper(shell)
                # "pkg b" stays one argument, and the flags apt-get needs for an unattended install come first.
                self.assertEqual(calls[0], INSTALL)

    def test_a_failed_attempt_refreshes_the_indexes_and_retries_after_a_delay(self):
        for name, shell in shells():
            with self.subTest(shell=name):
                result, calls = self.run_helper(shell, install_failures=2)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual(calls, [INSTALL, "sleep 30", UPDATE, INSTALL, "sleep 60", UPDATE, INSTALL])
                self.assertIn("retrying in 30 s", result.stdout)

    def test_gives_up_after_five_attempts_with_a_workflow_error(self):
        for name, shell in shells():
            with self.subTest(shell=name):
                result, calls = self.run_helper(shell, install_failures=99)
                self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
                self.assertEqual(calls.count(INSTALL), 5)
                self.assertEqual([call for call in calls if call.startswith("sleep")], ["sleep 30", "sleep 60", "sleep 90", "sleep 120"])
                self.assertEqual(calls.count(UPDATE), 4)
                self.assertEqual(calls[-1], INSTALL)
                self.assertIn("::error::apt-get install failed after 5 attempts", result.stdout)

    def test_a_failing_index_refresh_does_not_stop_the_retries(self):
        for name, shell in shells():
            with self.subTest(shell=name):
                result, calls = self.run_helper(shell, install_failures=1, update_fails=True)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual(calls, [INSTALL, "sleep 30", UPDATE, INSTALL])

    def test_the_last_attempt_is_not_followed_by_a_wait(self):
        for name, shell in shells():
            with self.subTest(shell=name):
                _, calls = self.run_helper(shell, install_failures=99)
                self.assertNotEqual(calls[-1].split()[0], "sleep")
                self.assertNotEqual(calls[-1], UPDATE)


class WorkflowWiringTest(unittest.TestCase):
    def setUp(self):
        self.text = WORKFLOW.read_text(encoding="utf-8")

    def test_dependency_and_clazy_steps_install_through_the_helper(self):
        helper = self.text.index("cat > /usr/local/bin/apt-install-retry")
        dependencies = self.text.index("apt-install-retry \\\n            build-essential")
        clazy = self.text.index("run: apt-install-retry clazy")
        # The helper is written by the first Neon step, before the steps that use it.
        self.assertLess(helper, dependencies)
        self.assertLess(dependencies, clazy)

    def test_no_neon_install_bypasses_the_helper(self):
        # These two commands resolve packages from the Neon repository; a plain apt-get would give up after one attempt.
        self.assertNotIn("apt-get install -y --no-install-recommends \\\n            build-essential", self.text)
        self.assertNotIn("apt-get install -y --no-install-recommends clazy", self.text)

    def test_the_helper_is_executable(self):
        self.assertIn("chmod +x /usr/local/bin/apt-install-retry", self.text)


if __name__ == "__main__":
    unittest.main()
