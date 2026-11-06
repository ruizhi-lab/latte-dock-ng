#!/usr/bin/env python3
"""Inventory the test functions of a source-contract test file.

Prints a markdown table with one row per SourceContractTest function of
autotests/sourcecontracttest.cpp (or the file named on the command line):
the first and last line, the repository paths the body names or reads, the
number of lines that call QVERIFY or QCOMPARE, and the first three of those
lines, cut to 100 characters.

Paths are collected inside each body from:
  - LATTE_SOURCE_DIR "/path" literals (QFile and QDirIterator arguments);
  - readSource(QStringLiteral("path")) calls through the local lambda;
  - other string literals shaped like a relative path (no spaces, at least
    one slash);
  - literal lists in `for (const QString &x : {...})` whose body reads
    through x, for example the directory roots "containment" and "shell".
Paths built at run time from other variables are not listed; open those
functions to see what they read.

Only the Python standard library is used. Output depends only on the input
file, so two runs on the same file print identical bytes. The exit status is
1 when the declared test slots and the defined functions do not match.
"""

import argparse
import bisect
import re
import sys

DEFAULT_SOURCE = "autotests/sourcecontracttest.cpp"
MAX_CELL = 100
HEADER = ("| # | Function | First line | Last line | Source paths | QVERIFY/QCOMPARE lines "
          "| Assertion 1 | Assertion 2 | Assertion 3 |")
ASSERT_RE = re.compile(r"\bQ(?:VERIFY2?|COMPARE)\s*\(")
CLASS_RE = re.compile(r"\bclass\s+SourceContractTest\b")
SLOT_RE = re.compile(r"^\s+void\s+(\w+)\s*\(\s*\)\s*;", re.MULTILINE)
DEF_RE = re.compile(r"^(?:void\s+)?SourceContractTest::(\w+)\s*\(\s*\)", re.MULTILINE)
MACRO_RE = re.compile(r'LATTE_SOURCE_DIR\s*(?=")')
READ_RE = re.compile(r'readSource\s*\(\s*(?:QStringLiteral\s*\(\s*)?(?=")')
LOOP_RE = re.compile(r"for\s*\(\s*const\s+QString\s*&\s*(\w+)\s*:\s*\{")
READER_RE = re.compile(r"\b(?:readSource|QFile|QDirIterator|QDir|QTextStream)\b")
PATH_RE = re.compile(r"[A-Za-z0-9_.+-]+(?:/[A-Za-z0-9_.+-]+)+")


def _word_before(text, index):
    return index > 0 and (text[index - 1].isalnum() or text[index - 1] == "_")


def mask(text):
    """Return (code, strict, literals) for a C++ source text.

    code: comments blanked, string literal contents kept.
    strict: comments and literal contents blanked, so braces, macros and
    assertion names inside them are ignored.
    literals: maps the offset of each opening quote of a plain string literal
    to the offset of its closing quote. Raw R"(...)" strings are blanked in
    strict and are not recorded.
    Newlines are kept in every copy, so offsets and line numbers match text.
    """
    code = list(text)
    strict = list(text)
    literals = {}
    count = len(text)

    def blank(chars, start, end):
        for offset in range(start, end):
            if chars[offset] != "\n":
                chars[offset] = " "

    index = 0
    while index < count:
        char = text[index]
        if text.startswith("//", index):
            end = text.find("\n", index)
            end = count if end < 0 else end
            blank(code, index, end)
            blank(strict, index, end)
            index = end
        elif text.startswith("/*", index):
            end = text.find("*/", index + 2)
            end = count if end < 0 else end + 2
            blank(code, index, end)
            blank(strict, index, end)
            index = end
        elif text.startswith('R"(', index) and not _word_before(text, index):
            end = text.find(')"', index + 3)
            end = count if end < 0 else end + 2
            blank(strict, index + 3, max(index + 3, end - 2))
            index = end
        elif char == '"' or (char == "'" and not _word_before(text, index)):
            end = index + 1
            while end < count and text[end] not in (char, "\n"):
                end += 2 if text[end] == "\\" else 1
            end = min(end, count)
            blank(strict, index + 1, end)
            if char == '"' and end < count and text[end] == '"':
                literals[index] = end
            index = end + 1
        else:
            index += 1
    return "".join(code), "".join(strict), literals


def match_brace(strict, open_index):
    """Offset of the brace that closes the one at open_index (strict text only)."""
    depth = 0
    for index in range(open_index, len(strict)):
        if strict[index] == "{":
            depth += 1
        elif strict[index] == "}":
            depth -= 1
            if depth == 0:
                return index
    raise ValueError(f"unbalanced braces at offset {open_index}")


def clip(text):
    """Cut text to MAX_CELL characters and mark the cut with three dots."""
    return text if len(text) <= MAX_CELL else text[:MAX_CELL - 3] + "..."


def cell(text):
    """Markdown code span that stays inside one table cell."""
    return "`" + text.replace("`", "'").replace("|", "\\|") + "`"


def inventory(text):
    """Parse a source-contract test file.

    Returns a dict with: functions (source order; each has name, first, last,
    paths, count and asserts, where asserts holds the first three assertion
    lines cut by clip()), declared and defined (name lists), and problems
    (consistency messages; empty when every declared slot has one definition).
    """
    code, strict, literals = mask(text)
    quotes = sorted(literals)
    lines = text.split("\n")
    strict_lines = strict.split("\n")
    newlines = [index for index, char in enumerate(text) if char == "\n"]

    declared = []
    class_match = CLASS_RE.search(strict)
    if class_match:
        stop = strict.find("\n};", class_match.end())
        block = strict[class_match.end(): stop if stop >= 0 else len(strict)]
        declared = SLOT_RE.findall(block)

    functions = []
    defined = []
    for match in DEF_RE.finditer(strict):
        name = match.group(1)
        defined.append(name)
        open_brace = strict.index("{", match.end())
        close = match_brace(strict, open_brace)
        first = bisect.bisect_left(newlines, match.start()) + 1
        last = bisect.bisect_left(newlines, close) + 1

        # (offset, value, direct): direct literals come from a path-reading
        # shape and are kept as they are; other literals must look like paths.
        found = []
        for pattern in (MACRO_RE, READ_RE):
            for hit in pattern.finditer(code, open_brace, close):
                if hit.end() in literals:
                    found.append((hit.end(), text[hit.end() + 1: literals[hit.end()]], True))
        first_quote = bisect.bisect_left(quotes, open_brace)
        end_quote = bisect.bisect_left(quotes, close)
        for quote in quotes[first_quote:end_quote]:
            found.append((quote, text[quote + 1: literals[quote]], False))
        for loop in LOOP_RE.finditer(code, open_brace, close):
            list_open = loop.end() - 1
            list_close = match_brace(strict, list_open)
            body_open = strict.index("{", list_close)
            body = strict[body_open: match_brace(strict, body_open)]
            if READER_RE.search(body) and re.search(rf"\b{loop.group(1)}\b", body):
                start = bisect.bisect_left(quotes, list_open)
                stop = bisect.bisect_left(quotes, list_close)
                for quote in quotes[start:stop]:
                    found.append((quote, text[quote + 1: literals[quote]], True))

        paths = []
        for _, value, direct in sorted(found, key=lambda item: item[0]):
            if not direct and not PATH_RE.fullmatch(value):
                continue
            relative = value.lstrip("/")
            if relative and relative not in paths:
                paths.append(relative)

        asserted = [number for number in range(first, last + 1)
                    if ASSERT_RE.search(strict_lines[number - 1])]
        functions.append({
            "name": name,
            "first": first,
            "last": last,
            "paths": paths,
            "count": len(asserted),
            "asserts": [clip(lines[number - 1].strip()) for number in asserted[:3]],
        })

    problems = []
    for name in sorted(set(declared) - set(defined)):
        problems.append(f"declared without definition: {name}")
    for name in sorted(set(defined) - set(declared)):
        problems.append(f"defined without declaration: {name}")
    if len(defined) != len(set(defined)):
        problems.append("a function is defined more than once")
    return {"functions": functions, "declared": declared, "defined": defined, "problems": problems}


def render(source, result):
    """Markdown report: a summary line and one table row per function."""
    rows = [
        "# Source-contract inventory",
        "",
        (f"Source: `{source}`. Functions: {len(result['functions'])}. "
         f"Declared slots: {len(result['declared'])}. Defined: {len(result['defined'])}."),
        "",
        HEADER,
        "|---|---|---|---|---|---|---|---|---|",
    ]
    for index, function in enumerate(result["functions"], 1):
        shown = [cell(text) for text in function["asserts"]]
        shown += ["-"] * (3 - len(shown))
        paths = ", ".join(cell(path) for path in function["paths"]) or "-"
        rows.append(
            f"| {index} | `{function['name']}` | {function['first']} | {function['last']} "
            f"| {paths} | {function['count']} | {shown[0]} | {shown[1]} | {shown[2]} |")
    return "\n".join(rows) + "\n"


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Print a markdown inventory of the SourceContractTest functions.")
    parser.add_argument("source", nargs="?", default=DEFAULT_SOURCE,
                        help="test source file (default: %(default)s)")
    args = parser.parse_args(argv)
    with open(args.source, encoding="utf-8", errors="replace") as handle:
        text = handle.read()
    result = inventory(text)
    sys.stdout.write(render(args.source, result))
    for problem in result["problems"]:
        print(f"list_contracts: {problem}", file=sys.stderr)
    return 1 if result["problems"] else 0


if __name__ == "__main__":
    sys.exit(main())
