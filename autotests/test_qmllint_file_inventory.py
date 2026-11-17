#!/usr/bin/env python3
"""Check that both QML lint gates share a narrow, tracked-source inventory."""

from __future__ import annotations

from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]
INVENTORY_SCRIPT = ROOT / "scripts" / "qmllint-files.sh"


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="qmllint-inventory-") as temporary:
        root = Path(temporary)
        subprocess.run(["git", "init", "-q", str(root)], check=True)
        tracked = (
            "containment/AppletItem.qml",
            "autotests/qml/tst_keyboard.qml",
            "autotests/fixtures/qmllint-baseline/negative.qml",
            "docs/help/example.qml",
            "docs/modernization-results/run/diagnostic.qml",
        )
        for relative in tracked:
            path = root / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("not required to parse; this test checks only inventory\n", encoding="utf-8")
        subprocess.run(["git", "-C", str(root), "add", "--", *tracked], check=True)

        # Untracked build output stays out, while production, test, fixture and
        # ordinary documentation QML remain covered. Only archived results go.
        untracked = root / "containment/generated.qml"
        untracked.write_text("Item {}\n", encoding="utf-8")
        expected = sorted(path for path in tracked
                          if not path.startswith("docs/modernization-results/"))
        result = subprocess.run(
            ["bash", str(INVENTORY_SCRIPT), str(root)],
            check=True,
            capture_output=True,
            text=True,
        )
        actual = result.stdout.splitlines()
        if actual != expected:
            raise AssertionError(f"unexpected tracked QML inventory: {actual!r}")

        cli = subprocess.run(
            ["bash", str(INVENTORY_SCRIPT), str(root)],
            check=True,
            capture_output=True,
            text=True,
        )
        if cli.stdout.splitlines() != expected:
            raise AssertionError("qmllint-files.sh output is not deterministic")

        failure = subprocess.run(
            ["bash", str(INVENTORY_SCRIPT), str(root / "missing-checkout")],
            capture_output=True,
            text=True,
        )
        if failure.returncode == 0:
            raise AssertionError("qmllint-files.sh accepted a non-Git root")

    print("QML lint inventory: PASS (tracked non-result QML retained; archived result copies excluded)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
