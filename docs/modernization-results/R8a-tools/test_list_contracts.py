#!/usr/bin/env python3
"""Synthetic-sample tests for list_contracts.py.

Run by hand, not from CTest:
    python3 docs/modernization-results/R8a-tools/test_list_contracts.py -v
"""

import contextlib
import io
import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import list_contracts  # noqa: E402

SAMPLE = r'''class SourceContractTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void alpha();
    void beta();
    void gamma();
    void delta();
};

void SourceContractTest::alpha()
{
    // QVERIFY(commented) is not an assertion, and a brace } in a comment does not close it.
    /* QVERIFY(blockCommented); */
    QFile file(QStringLiteral(LATTE_SOURCE_DIR "/app/alpha.cpp"));
    QVERIFY(file.open(QFile::ReadOnly));
    const QString text = QString::fromUtf8(file.readAll());
    QVERIFY(text.contains(QStringLiteral("{")));
    QCOMPARE(text.count(QStringLiteral("x")), 1);
}

void
SourceContractTest::beta()
{
    const auto readSource = [](const QString &relativePath) {
        QFile file(QStringLiteral(LATTE_SOURCE_DIR "/") + relativePath);
        return file.open(QFile::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
    };
    const QString qml = readSource(QStringLiteral("shell/package/a.qml"));
    for (const QString &root : {QStringLiteral("containment"), QStringLiteral("shell")}) {
        QDirIterator it(QStringLiteral(LATTE_SOURCE_DIR "/") + root, {QStringLiteral("*.qml")}, QDir::Files);
        QVERIFY(it.hasNext() || !it.hasNext());
    }
    QVERIFY(qml.contains(QStringLiteral("}")));
}

void SourceContractTest::gamma()
{
    const QString raw = QString::fromUtf8(R"(QVERIFY(not real) )");
    QVERIFY(raw.size() > 0);
}

void SourceContractTest::delta()
{
    QVERIFY(one && "a | pipe and an assertion line that is long enough to be cut at one hundred characters");
    QVERIFY(two);
    QCOMPARE(three, 3);
    QVERIFY(four);
    QVERIFY(five);
}
'''

MISMATCH = r'''class SourceContractTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void orphan();
    void kept();
};

void SourceContractTest::kept()
{
    QVERIFY(true);
}

void SourceContractTest::extra()
{
    QVERIFY(true);
}
'''

HEADER = ("| # | Function | First line | Last line | Source paths | QVERIFY/QCOMPARE lines "
          "| Assertion 1 | Assertion 2 | Assertion 3 |")


class InventoryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.result = list_contracts.inventory(SAMPLE)
        cls.functions = {function["name"]: function for function in cls.result["functions"]}
        cls.lines = SAMPLE.split("\n")

    def closing_line(self, first):
        """1-based number of the first column-0 closing brace at or after line first."""
        return self.lines.index("}", first) + 1

    def test_functions_are_listed_in_source_order(self):
        self.assertEqual([f["name"] for f in self.result["functions"]], ["alpha", "beta", "gamma", "delta"])
        self.assertEqual(self.result["problems"], [])

    def test_first_and_last_lines_follow_the_definition(self):
        alpha = self.functions["alpha"]
        self.assertEqual(alpha["first"], self.lines.index("void SourceContractTest::alpha()") + 1)
        self.assertEqual(alpha["last"], self.closing_line(alpha["first"]))
        # beta's return type sits on the line before its name: the row starts at the "void" line.
        beta = self.functions["beta"]
        self.assertEqual(beta["first"], self.lines.index("void") + 1)
        self.assertEqual(self.lines[beta["first"]], "SourceContractTest::beta()")
        self.assertEqual(beta["last"], self.closing_line(beta["first"]))

    def test_paths_come_from_macros_readers_and_loops(self):
        self.assertEqual(self.functions["alpha"]["paths"], ["app/alpha.cpp"])
        # The bare "/" prefix of readSource and QDirIterator is not a path; the loop roots are.
        self.assertEqual(self.functions["beta"]["paths"], ["shell/package/a.qml", "containment", "shell"])
        self.assertEqual(self.functions["gamma"]["paths"], [])

    def test_assertions_skip_comments_literals_and_raw_strings(self):
        self.assertEqual(self.functions["alpha"]["count"], 3)
        self.assertEqual(self.functions["beta"]["count"], 2)
        self.assertEqual(self.functions["gamma"]["count"], 1)
        self.assertEqual(self.functions["delta"]["count"], 5)
        self.assertEqual(self.functions["alpha"]["asserts"], [
            "QVERIFY(file.open(QFile::ReadOnly));",
            'QVERIFY(text.contains(QStringLiteral("{")));',
            'QCOMPARE(text.count(QStringLiteral("x")), 1);',
        ])

    def test_only_the_first_three_assertions_are_shown(self):
        self.assertEqual(len(self.functions["delta"]["asserts"]), 3)
        self.assertEqual(self.functions["delta"]["asserts"][1:], ["QVERIFY(two);", "QCOMPARE(three, 3);"])

    def test_long_and_piped_assertions_are_cut_and_escaped(self):
        first = self.functions["delta"]["asserts"][0]
        self.assertEqual(len(first), 100)
        self.assertTrue(first.endswith("..."))
        self.assertEqual(list_contracts.clip("x" * 150), "x" * 97 + "...")
        self.assertEqual(list_contracts.clip("short"), "short")
        self.assertEqual(list_contracts.cell("a|b`c"), "`a\\|b'c`")

    def test_declared_and_defined_mismatch_is_reported(self):
        result = list_contracts.inventory(MISMATCH)
        self.assertEqual(result["problems"], [
            "declared without definition: orphan",
            "defined without declaration: extra",
        ])

    def test_rendering_is_deterministic_and_has_the_header(self):
        first = list_contracts.render("sample.cpp", self.result)
        second = list_contracts.render("sample.cpp", list_contracts.inventory(SAMPLE))
        self.assertEqual(first, second)
        self.assertIn(HEADER, first)
        self.assertIn("Functions: 4. Declared slots: 4. Defined: 4.", first)
        self.assertIn("\\|", first)

    def test_command_line_prints_the_table_and_exits_zero(self):
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "sample.cpp")
            with open(path, "w", encoding="utf-8") as handle:
                handle.write(SAMPLE)
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                status = list_contracts.main([path])
        self.assertEqual(status, 0)
        self.assertIn("| 4 | `delta` |", output.getvalue())


if __name__ == "__main__":
    unittest.main()
