#!/usr/bin/env python3
"""Contracts for the two KDE Neon bootstrap steps in .github/workflows/build.yml.

Both steps run before actions/checkout, so they cannot share a file from the repository and
each carries its own copy of the Neon bootstrap. That duplication is a real drift risk: the
2026-11-21 CI runs lost jobs at these steps, and an edit applied to only one copy would leave
the other silently weaker.

These tests read the workflow as text and enforce three properties:
  1. both copies of the Neon bootstrap are byte-identical;
  2. each copy tries both mirrors with three attempts and fails loudly when neither answers;
  3. the apt retry policy is written before the first apt-get update in each step, which is the
     ordering defect that failed the QML lint job on 2026-11-21.

No network, Docker or GitHub access is needed.
"""

import re
import unittest
from pathlib import Path

sys_dont_write = True  # documentation of intent; no bytecode is written for this module anyway

WORKFLOW = Path(__file__).resolve().parents[1] / ".github/workflows/build.yml"

# The bootstrap begins at neon_try() and ends at the host report. Both steps must carry this
# exact block, minus the indentation of the enclosing YAML block scalar.
BOOTSTRAP = re.compile(
    r"^ *(?P<body>neon_try\(\) \{.*?^ *echo \"KDE Neon repository: \$neon_host\"\n)",
    re.M | re.S,
)

MIRRORS = ("archive.neon.kde.org", "origin.archive.neon.kde.org")


def workflow_text():
    return WORKFLOW.read_text(encoding="utf-8")


def normalize(text):
    """Strip the YAML block indentation so two copies can be compared literally."""
    indent = min((len(line) - len(line.lstrip()) for line in text.splitlines() if line.strip()), default=0)
    return "".join(line[indent:] if line.strip() else "\n" for line in text.splitlines(keepends=True))


def bootstraps():
    return [normalize(match["body"]) for match in BOOTSTRAP.finditer(workflow_text())]


def step_blocks():
    """Every `- name: ...` step's shell body, as text."""
    steps = re.split(r"^      - name: ", workflow_text(), flags=re.M)[1:]
    return steps


def command_lines(block):
    """The actual shell commands of a step body, with comment lines and blank lines removed."""
    return [
        line.strip()
        for line in block.splitlines()[1:]
        if line.strip() and not line.strip().startswith("#")
    ]


class NeonBootstrapContracts(unittest.TestCase):
    def test_both_steps_carry_a_bootstrap(self):
        self.assertEqual(len(bootstraps()), 2, "expected one Neon bootstrap per Neon step")

    def test_the_two_copies_are_identical(self):
        first, second = bootstraps()
        self.assertEqual(first, second, "the two Neon bootstrap copies have drifted apart")

    def test_each_copy_tries_both_mirrors_three_times(self):
        for body in bootstraps():
            for mirror in MIRRORS:
                self.assertIn(mirror, body, f"{mirror} is no longer tried")
            self.assertIn("for attempt in 1 2 3; do", body)

    def test_each_copy_fails_loudly_when_no_mirror_answers(self):
        for body in bootstraps():
            self.assertIn('echo "::error::', body)
            self.assertIn("exit 1", body)

    def test_a_partial_index_update_is_still_an_error(self):
        # --error-on=any is what keeps a half-fetched index from passing as success.
        for body in bootstraps():
            self.assertIn("apt-get update --error-on=any", body)


class RetryPolicyOrdering(unittest.TestCase):
    """Acquire::Retries must precede the first apt-get update in every Neon step."""

    def test_every_neon_step_writes_the_policy_before_updating(self):
        neon_steps = [block for block in step_blocks() if "neon_try()" in block]
        self.assertEqual(len(neon_steps), 2, "expected two Neon steps")

        for block in neon_steps:
            name = block.splitlines()[0].strip()
            commands = command_lines(block)
            policy = next(
                (index for index, line in enumerate(commands) if "Acquire::Retries" in line), None
            )
            update = next((index for index, line in enumerate(commands) if line == "apt-get update"), None)
            self.assertIsNotNone(policy, f"{name}: no Acquire::Retries policy")
            self.assertIsNotNone(update, f"{name}: no bare apt-get update")
            self.assertLess(policy, update, f"{name}: apt-get update runs before the retry policy is set")

    def test_the_first_update_is_not_left_unretried(self):
        # A bare `apt-get update` as the first command of a Neon step is the 2026-11-21 defect.
        for block in step_blocks():
            if "neon_try()" not in block:
                continue
            name = block.splitlines()[0].strip()
            commands = command_lines(block)
            self.assertNotEqual(
                commands[0],
                "apt-get update",
                f"{name}: the first command updates the indexes with no retry policy in place",
            )


if __name__ == "__main__":
    unittest.main(verbosity=2)
